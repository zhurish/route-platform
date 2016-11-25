/* sntpcLib.c - Simple Network Time Protocol (SNTP) client library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
//#include "copyright_wrs.h"

/*
modification history 
--------------------
01k,07jan02,vvv  doc: added errnos for sntpcTimeGet and sntpcFetch (SPR #71557)
01j,16mar99,spm  doc: removed references to configAll.h (SPR #25663)
01e,14dec97,jdi  doc: cleanup.
01d,10dec97,kbw  making man page changes
01c,27aug97,spm  corrections for man page generation
01b,15jul97,spm  code cleanup, documentation, and integration; entered in
                 source code control
01a,24may97,kyc  written
*/

/* 
DESCRIPTION
This library implements the client side of the Simple Network Time 
Protocol (SNTP), a protocol that allows a system to maintain the 
accuracy of its internal clock based on time values reported by one 
or more remote sources.  The library is included in the VxWorks image 
if INCLUDE_SNTPC is defined at the time the image is built.

USER INTERFACE
The sntpcTimeGet() routine retrieves the time reported by a remote source and
converts that value for POSIX-compliant clocks.  The routine will either send a 
request and extract the time from the reply, or it will wait until a message is
received from an SNTP/NTP server executing in broadcast mode.

INCLUDE FILES: sntpcLib.h

SEE ALSO: clockLib, RFC 1769
*/

/* includes */
#include "utils.h"
#include "sntpcLib.h"


/* globals */
#ifdef HAVE_UTILS_SNTP


static struct sntp_client *sntp_client = NULL;

/* forward declarations */
/*******************************************************************************
*
* sntpcFractionToNsec - convert time from the NTP format to POSIX time format
*
* This routine converts the fractional part of the NTP timestamp format to a 
* value in nanoseconds compliant with the POSIX clock.  While the NTP time 
* format provides a precision of about 200 pico-seconds, rounding error in the 
* conversion routine reduces the precision to tenths of a micro-second.
* 
* RETURNS: Value for struct timespec corresponding to NTP fractional part
*
* ERRNO:   N/A
*
* INTERNAL
*
* Floating-point calculations can't be used because some boards (notably
* the SPARC architectures) disable software floating point by default to 
* speed up context switching. These boards abort with an exception when
* floating point operations are encountered.
*
* NOMANUAL
*/

LOCAL ULONG sntpcFractionToNsec
    (
    ULONG sntpFraction      /* base 2 fractional part of the NTP timestamp */
    )
    {
    ULONG factor = 0x8AC72305; /* Conversion factor from base 2 to base 10 */
    ULONG divisor = 10;        /* Initial exponent for mantissa. */
    ULONG mask = 1000000000;   /* Pulls digits of factor from left to right. */
    int loop;
    ULONG nsec = 0;
    BOOL shift = FALSE;        /* Shifted to avoid overflow? */
    /* 
     * Adjust large values so that no intermediate calculation exceeds 
     * 32 bits. (This test is overkill, since the fourth MSB can be set 
     * sometimes, but it's fast).
     */
    if (sntpFraction & 0xF0000000)
    {
        sntpFraction /= 10;
        shift = TRUE;
    }
    /* 
     * In order to increase portability, the following conversion avoids
     * floating point operations, so it is somewhat obscure.
     *
     * Incrementing the NTP fractional part increases the corresponding
     * decimal value by 2^(-32). By interpreting the fractional part as an
     * integer representing the number of increments, the equivalent decimal
     * value is equal to the product of the fractional part and 0.2328306437.
     * That value is the mantissa for 2^(-32). Multiplying by 2.328306437E-10
     * would convert the NTP fractional part into the equivalent in seconds.
     *
     * The mask variable selects each digit from the factor sequentially, and
     * the divisor shifts the digit the appropriate number of decimal places. 
     * The initial value of the divisor is 10 instead of 1E10 so that the 
     * conversion produces results in nanoseconds, as required by POSIX clocks.
     */
    for (loop = 0; loop < 10; loop++)    /* Ten digits in mantissa */
    {
		nsec += sntpFraction * (factor/mask)/divisor;  /* Use current digit. */
		factor %= mask;    /* Remove most significant digit from the factor. */
		mask /= 10;        /* Reduce length of mask by one. */
		divisor *= 10;     /* Increase preceding zeroes by one. */
    }
    /* Scale result upwards if value was adjusted before processing. */
    if (shift)
        nsec *= 10;

    return (nsec);
}
/*******************************************************************************/
static int sntp_socket_init(struct sntp_client *client)
{
    int result;
    struct sockaddr_in srcAddr;

    client->sock = socket (AF_INET, SOCK_DGRAM, 0);
    if (client->sock == -1)
        return (ERROR);

    if(client->address.s_addr)
    {
    	int optval;
		optval = 1;
		result = setsockopt (client->sock, SOL_SOCKET, SO_BROADCAST,
							 (char *)&optval, sizeof (optval));
    }
    else
    {
		/* Initialize source address. */
		bzero ( (char *)&srcAddr, sizeof (srcAddr));
		srcAddr.sin_addr.s_addr = INADDR_ANY;
		srcAddr.sin_family = AF_INET;
		srcAddr.sin_port = client->sntpcPort;

		result = bind (client->sock, (struct sockaddr *)&srcAddr, sizeof (srcAddr));
		if (result == -1)
		{
			close (client->sock);
			return (ERROR);
		}
    }
    return client->sock;
}

static int sntp_socket_close(struct sntp_client *client)
{
	if(client->sock)
		close (client->sock);
    return OK;
}

static int sntp_socket_write(struct sntp_client *client)
{
    struct sockaddr_in dstAddr;
    SNTP_PACKET sntpRequest;     /* sntp request packet for */
    //int servAddrLen = 0;
  
    /* Set destination for request. */
  
    bzero ( (char *)&dstAddr, sizeof (dstAddr));
    dstAddr.sin_addr.s_addr = client->address.s_addr;
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(client->sntpcPort);

    /* Initialize SNTP message buffers. */
  
    bzero ( (char *)&sntpRequest, sizeof (sntpRequest));

    sntpRequest.leapVerMode = SNTP_CLIENT_REQUEST;
  
    bzero ( (char *) &dstAddr, sizeof (dstAddr));
    //servAddrLen = sizeof (dstAddr);
  
    /* Transmit SNTP request. */
    if (sendto (client->sock, (caddr_t)&sntpRequest, sizeof(sntpRequest), 0,
                (struct sockaddr *)&dstAddr, sizeof (dstAddr)) == -1) 
    {
        //close (client->sock);
        if(client->time_debug)
        	zlog_err("Transmit SNTP equest from %s:%d(%s)",inet_ntoa(client->address),client->sntpcPort,safe_strerror(errno));
        return (ERROR);
    }
    if(client->time_debug)
    	zlog_debug("Transmit SNTP request from %s:%d",inet_ntoa(client->address),client->sntpcPort);
    return OK;
}
static int sntp_socket_read(struct sntp_client *client)
{
    SNTP_PACKET sntpMessage;    /* buffer for message from server */
    struct sockaddr_in srcAddr;
    int result;
    int srcAddrLen;
    char ver[32];
    /* Wait for broadcast message from server. */
    /* Wait for reply at the ephemeral port selected by the sendto () call. */
    result = recvfrom (client->sock, (caddr_t) &sntpMessage, sizeof(sntpMessage),
                       0, (struct sockaddr *) &srcAddr, (socklen_t *)&srcAddrLen);
    if (result == -1) 
    {
        //close (client->sock);
    	if(client->time_debug)
    		zlog_err("SNTP receive from %s:%d(%s)",inet_ntoa(srcAddr.sin_addr),ntohs(srcAddr.sin_port),safe_strerror(errno));
        return (ERROR);
    }

    //close (client->sock);
    switch(sntpMessage.leapVerMode & SNTP_VN_MASK)
    {
    case SNTP_VN_0:           /* not supported */
    	sprintf(ver,"%s","version:0");
    	break;
    case SNTP_VN_1:           /* the earliest version */
    	sprintf(ver,"%s","version:1");
    	break;
    case SNTP_VN_2:
    	sprintf(ver,"%s","version:2");
    	break;
    case SNTP_VN_3:           /* VxWorks implements this version */
    	sprintf(ver,"%s","version:3");
    	break;
    case SNTP_VN_4:           /* the latest version, implemented if INET6 is defined */
    	sprintf(ver,"%s","version:4");
    	break;
    case SNTP_VN_5:          /* reserved */
    	sprintf(ver,"%s","version:5");
    	break;
    case SNTP_VN_6:           /* reserved */
    	sprintf(ver,"%s","version:6");
    	break;
    case SNTP_VN_7:           /* reserved */
    	sprintf(ver,"%s","version:7");
    	break;
    default:
    	sprintf(ver,"%s","unknow version");
    	break;
    }
    if(client->time_debug)
    {
    	zlog_debug("SNTP receive SNTP %s from %s:%d",ver,inet_ntoa(srcAddr.sin_addr),ntohs(srcAddr.sin_port));
    }
    /*
     * Return error if the server clock is unsynchronized, or the version is 
     * not supported.
     */

    if ( (sntpMessage.leapVerMode & SNTP_LI_MASK) == SNTP_LI_3 ||
        sntpMessage.transmitTimestampSec == 0)
    {
        if(client->time_debug)
        	zlog_warn("SNTP server clock unsynchronized");
        //errnoSet (S_sntpcLib_SERVER_UNSYNC);
        return (ERROR);
    }

    if ( (sntpMessage.leapVerMode & SNTP_VN_MASK) == SNTP_VN_0 ||
        (sntpMessage.leapVerMode & SNTP_VN_MASK) > SNTP_VN_3)
    {
        if(client->time_debug)
        	zlog_warn("SNTP version (%s) unsupported",ver);
        //errnoSet (S_sntpcLib_VERSION_UNSUPPORTED);
        return (ERROR);
    }
    /* Convert the NTP timestamp to the correct format and store in clock. */
    /* Add test for 2036 base value here! */

    sntpMessage.transmitTimestampSec = 
                                     ntohl (sntpMessage.transmitTimestampSec) -
                                     SNTP_UNIX_OFFSET;
    /*
     * Adjust returned value if leap seconds are present.
     * This needs work!
     */
    /* if ( (sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_1)
            sntpReply.transmitTimestampSec += 1;
     else if ((sntpReply.leapVerMode & SNTP_LI_MASK) == SNTP_LI_2)
              sntpReply.transmitTimestampSec -= 1;
    */
    sntpMessage.transmitTimestampFrac = ntohl (sntpMessage.transmitTimestampFrac);
    client->sntpTime.tv_sec = sntpMessage.transmitTimestampSec;
    client->sntpTime.tv_nsec = sntpcFractionToNsec (sntpMessage.transmitTimestampFrac);
    return (OK);
}


static int sntp_read(struct thread *thread)
{
	int ret = 0;
	struct sntp_client *client;
	client = THREAD_ARG (thread);
	//sock = THREAD_FD (thread);
	client->t_read = thread_add_read (master, sntp_read, client, client->sock);
	ret = sntp_socket_read(client);
	if(ret == OK)
	{
		time_t	time_sec = 0;
		clock_settime(CLOCK_REALTIME, &client->sntpTime);//SET SYSTEM LOCAL TIME
		time_sec = time(NULL);
		if(client->time_debug)
			zlog_debug("SNTP receive and set sys time:%s",ctime(&time_sec));
		time_sec += LOCAL_GMT_OFSET;
		//if(client->time_debug)
		//	zlog_debug("SNTP receive and set sys time:%s",ctime(&time_sec));
	}
	else
	{
		if(client->time_debug)
			zlog_warn("SNTP protocol can't receive sys time");
	}
	return OK;
}

static int sntp_write(struct thread *thread)
{
	struct sntp_client *client;
	client = THREAD_ARG (thread);
	sntp_socket_write(client);
	return OK;
}

static int sntp_time(struct thread *thread)
{
	struct sntp_client *client;
	client = THREAD_ARG (thread);
	client->t_time = thread_add_timer (master, sntp_time, client, client->sntpc_time);
	client->t_write = thread_add_write (master, sntp_write, client, client->sock);
	return OK;
}
/*******************************************************************************/
static int sntpcEnable(u_short port, char *ip, int time_interval, int mode)
{
	int ret = 0;
	if(sntp_client)
	{
		if(port)
			sntp_client->sntpcPort =  (port);
		else
			sntp_client->sntpcPort =  (SNTPC_SERVER_PORT);
		sntp_client->enable = 1;
		if(ip)
			sntp_client->address.s_addr = inet_addr(ip);
		if(time_interval)
			sntp_client->sntpc_time = time_interval;
		else
			sntp_client->sntpc_time = LOCAL_UPDATE_GMT;

		sntp_client->mode = mode;
		ret = sntp_socket_init(sntp_client);
		if(ret > 0)
		{
			if(sntp_client->mode != SNTP_PASSIVE)
			{
				sntp_client->t_time = thread_add_timer (master, sntp_time, sntp_client, sntp_client->sntpc_time);
				sntp_client->t_write = thread_add_write (master, sntp_write, sntp_client, sntp_client->sock);
			}
			sntp_client->t_read = thread_add_read (master, sntp_read, sntp_client, sntp_client->sock);
		}
		return (OK);
	}
    return (ERROR);
}
static int sntpcDisable(void)
{
	if(sntp_client)
	{
		sntp_client->enable = 0;
		if(sntp_client->t_time)
			thread_cancel(sntp_client->t_time);
		if(sntp_client->t_read)
			thread_cancel(sntp_client->t_read);
		if(sntp_client->t_write)
			thread_cancel(sntp_client->t_write);
		sntp_socket_close(sntp_client);
		memset(sntp_client, 0, sizeof(struct sntp_client));
		return (OK);
	}
    return (ERROR);
}

static int sntpcIsEnable(void)
{
	if(sntp_client == NULL)
		return -1;
	return sntp_client->enable;
}

DEFUN (sntp_enable,
		sntp_enable_cmd,
	    "sntp-server A.B.C.D port <100-65536> interval <30-3600>",
		"sntp server configure\n"
		"sntp server ip address format A.B.C.D \n"
		"sntp server port configure\n"
		"udp port of sntp server\n"
		"sntp server send request interval\n"
		"time interval of sec\n")
{
	int ret;
	u_short port = 0;
	int time_interval = 0;
	struct in_addr host_id;
	if(sntp_client == NULL)
		return CMD_WARNING;
	if(argv[1])
		port = atoi(argv[1]);
	if(argv[2])
		time_interval = atoi(argv[2]);

	ret = inet_aton (argv[0], &host_id);
	if (!ret)
	{
	      vty_out (vty, "Please specify Router ID by A.B.C.D%s", VTY_NEWLINE);
	      return CMD_WARNING;
	}
	if( (port > SNTPC_SERVER_PORT_MAX)||(port < SNTPC_SERVER_PORT_MIN) )
	{

		vty_out(vty,"%% Interval port, may be 64-65535 %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (LOCAL_UPDATE_GMT_MAX > 65536)||(LOCAL_UPDATE_GMT_MIN < 30) )
	{

		vty_out(vty,"%% Interval port, may be 30-65535 %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(sntpcIsEnable())
	{
		vty_out(vty,"sntp server is already enable, please disable  first %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	sntpcEnable((u_short)port, NULL, time_interval, sntp_client->mode);
	return CMD_SUCCESS;
}

DEFUN (sntp_passive,
		sntp_passive_cmd,
	    "sntp-server passive",
		"sntp server configure\n"
		"sntp server passive type\n")
{
	if(sntp_client == NULL)
		return CMD_WARNING;
	sntp_client->mode = SNTP_PASSIVE;
	if(sntpcIsEnable())
		sntpcDisable();
	sntpcEnable(sntp_client->sntpcPort, NULL, sntp_client->sntpc_time, sntp_client->mode);
	return CMD_SUCCESS;
}
DEFUN (no_sntp_passive,
		no_sntp_passive_cmd,
	    "no sntp-server passive",
		NO_STR
		"sntp server configure\n"
		"sntp server passive type\n")
{
	if(sntp_client == NULL)
		return CMD_WARNING;
	sntp_client->mode = SNTP_PASSIVE;
	if(sntpcIsEnable())
		sntpcDisable();
	sntpcEnable(sntp_client->sntpcPort, NULL, sntp_client->sntpc_time, sntp_client->mode);
	return CMD_SUCCESS;
}
DEFUN (no_sntp_enable,
		no_sntp_enable_cmd,
	    "no sntp-server",
		NO_STR
		"sntp server configure\n")
{
	if(sntp_client == NULL)
		return CMD_WARNING;
	if(sntpcIsEnable())
		sntpcDisable();
	return CMD_SUCCESS;
}

DEFUN (sntp_debug,
		sntp_debug_cmd,
	    "debug sntp time",
		DEBUG_STR
		"sntp server configure\n"
		"SNTP timespec infomation\n")
{
	if(sntp_client == NULL)
		return CMD_WARNING;
	sntp_client->time_debug = 1;
	return CMD_SUCCESS;
}
DEFUN (no_sntp_debug,
		no_sntp_debug_cmd,
	    "no debug sntp time",
		NO_STR
		DEBUG_STR
		"sntp server configure\n"
		"SNTP timespec infomation\n")
{
	if(sntp_client == NULL)
		return CMD_WARNING;
	sntp_client->time_debug = 0;
	return CMD_SUCCESS;
}
int sntpc_config(struct vty *vty)
{
	//"sntp-server A.B.C.D port <162-65536> interval <30-3600>",
	if(sntp_client == NULL)
		return CMD_SUCCESS;
	if(sntp_client->enable == 0)
		return CMD_SUCCESS;

	vty_out(vty, "sntp-server %s port %d interval %d%s",
		inet_ntoa(sntp_client->address),
		sntp_client->sntpcPort,
		sntp_client->sntpc_time,VTY_NEWLINE);

	if(sntp_client->mode == SNTP_PASSIVE)
	{
		vty_out(vty, "sntp-server passive%s",VTY_NEWLINE);
	}
	return 1;
}
int sntpc_debug_config(struct vty *vty)
{
	//"sntp-server A.B.C.D port <162-65536> interval <30-3600>",
	if(sntp_client == NULL)
		return CMD_SUCCESS;
	if(sntp_client->enable == 0)
		return CMD_SUCCESS;

	if(sntp_client->time_debug == SNTP_PASSIVE)
		vty_out(vty, "debug sntp time%s",VTY_NEWLINE);

	return 1;
}
static struct cmd_node service_node =
{
	SERVICE_NODE,
	"%s(config)# ",
	1,
};

int sntpcInit(void)
{
	if(sntp_client == NULL)
		sntp_client = (struct sntp_client *)malloc(sizeof(struct sntp_client));
	memset(sntp_client, 0, sizeof(struct sntp_client));

	install_node (&service_node, sntpc_config);
	install_default (SERVICE_NODE);

	install_element (CONFIG_NODE, &sntp_enable_cmd);
	install_element (CONFIG_NODE, &sntp_passive_cmd);
	install_element (CONFIG_NODE, &no_sntp_passive_cmd);
	install_element (CONFIG_NODE, &no_sntp_enable_cmd);
	install_element (CONFIG_NODE, &sntp_debug_cmd);
	install_element (CONFIG_NODE, &no_sntp_debug_cmd);

	return 0;
}

#endif /*HAVE_UTILS_SNTP*/
