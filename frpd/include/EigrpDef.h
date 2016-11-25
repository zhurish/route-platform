#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#ifndef _EIGRP_PLAT_MODULE
#include <sys/time.h>
#else
#include <sys/times.h>
#endif//_EIGRP_PLAT_MODULE

#ifndef _EIGRP_PLAT_MODULE

#define	FAILURE	-1
/*#define FRP_MODE		0
#define FRP_VTY_PORT	0
#define	IPI_PROTO_FRP	0
#define	IP_ADD_MEMBERSHIP_INDEX */
/* tigerwh 120518 #define	IP_MULTICAST_IFINDEX	IP_MULTICAST_IF*/
#define	IP_RECVDSTADDR	IP_PKTINFO

#if	(BYTE_ORDER	== LITTLE_ENDIAN)
#define HTONL(x)	((((x) & 0x000000ff) << 24) | \
			(((x) & 0x0000ff00) <<  8) | \
			(((x) & 0x00ff0000) >>  8) | \
			(((x) & 0xff000000) >> 24)).
#define HTONS(x)	((((x) & 0x00ff) << 8) | \
			(((x) & 0xff00) >> 8))
#else 
#define HTONL(x)	x
#define HTONS(x)	x
#endif

#define	NTOHL(ip)	HTONL((ip))
#define	NTOHS(ip)	HTONS((ip))

#define	EIGRP_RR_FUNC_ENTER(a)		\
/*if(gpEigrp->funcCnt == gpEigrp->funcMax - 1){	\
		gpEigrp->funcCnt	= 0;\
}	\
EigrpPortMemSet(gpEigrp->funcStack[gpEigrp->funcCnt], 0, 64);	\
sprintf(gpEigrp->funcStack[gpEigrp->funcCnt], "%s %d", #a,  __LINE__);	\
gpEigrp->funcCnt++;	*/
#define	EIGRP_RR_FUNC_LEAVE(a)	/*	EIGRP_RR_FUNC_ENTER(a)*/

#else /*_EIGRP_PLAT_MODULE */

#ifndef FAILURE
#define	FAILURE		(-1)
#endif
#ifndef SUCCESS
#define	SUCCESS	(1)
#endif

	#undef HTONL
	#define	HTONL	htonl

	#undef NTOHL
	#define	NTOHL	ntohl

	#undef HTONS
	#define	HTONS	htons

	#undef NTOHS
	#define	NTOHS	ntohs


#define	EIGRP_RR_FUNC_ENTER(a)
#define	EIGRP_RR_FUNC_LEAVE(a)

#endif/* _EIGRP_PLAT_MODULE */
