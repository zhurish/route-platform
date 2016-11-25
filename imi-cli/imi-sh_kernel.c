#include <zebra.h>
#include "memory.h"
#include "log.h"
#include <lib/version.h>
#include "thread.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "workqueue.h"
#include "linklist.h"
#include "if.h"
#include "prefix.h"

#ifdef IMISH_IMI_MODULE  

#include "imi-cli/imi-sh.h"
#include "imi-cli/imi-sh_kernel.h"
/******************************************************************/
/******************************************************************/
struct imish_kernel
{
  pid_t pid;//�ӽ���ID
  int status;//״̬
  int pfd[2];//�ܵ�
  int ptid;//��ȡ�ӽ��̷�����Ϣ�߳�pid
  pthread_t pthid;//��ȡ�ӽ��̷�����Ϣ�߳�tid  
  struct vty *vty;
};
/******************************************************************/
/******************************************************************/
static struct imish_kernel imish_kernel;
/******************************************************************/
/******************************************************************/
/******************************************************************/
//ִ��ϵͳ�ű�������ȡ�ű����
int imish_kernel_system(int dir, const char *cmd, char *buf, int size)
{
  FILE *fd = NULL;
  int ret= 0;	
  if(dir == 1)
  {
    	char cmdstring[128];
  		memset(cmdstring, 0, sizeof(cmdstring));
  		sprintf(cmdstring,"%s/%s",DAEMON_VTY_DIR,cmd);
			fd = popen(cmdstring,"r");
  }
  else
  {
  		fd = popen(cmd,"r");
  }
  if(fd)
  {
    if(buf)
    {
    		memset(buf, 0, size);
       	ret = fread(buf, 1, size, fd);
    }
    fclose(fd);
    return ret;
  }
  return -1;
}
/******************************************************************/
/******************************************************************/
/******************************************************************/
static int imish_kernel_response_string(struct vty *vty, const char *src, char *dest, int size)
{
	int i =0,j = 0;
	char buf[2048];
	memset(buf, 0, sizeof(buf));
	while(i < size)
	{
		if(src[i] == '\n')
		{
			if(vty->type == VTY_TERM)
			{
				buf[j] = '\r';
				j++;
			}
			buf[j] = '\n';
			j++;
		}
		else
		{
			buf[j] = src[i];
			j++;
		}
		i++;
	}
	memcpy(dest, buf, j);
	return j;
}
/******************************************************************/
/******************************************************************/
static void * imish_kernel_pthread(void *arg)
{
  int ret;
  char buf[2048];
  struct vty *vty = imish_kernel.vty;

  if(vty == NULL)
    return 0;
    
  while(imish_kernel.status)
  {
    memset(buf, 0, sizeof(buf));
    ret = read(imish_kernel.pfd[0], buf, sizeof(buf));
    if(ret > 0)
    {
      ret = imish_kernel_response_string(vty, buf, buf, ret);
      if(ret > 0)
      {
        //buffer_put (vty->obuf, VTY_NEWLINE, strlen(VTY_NEWLINE));
      	buffer_put (vty->obuf, buf, ret);
        //buffer_put (vty->obuf, VTY_NEWLINE, strlen(VTY_NEWLINE));
        buffer_flush_available(vty->obuf, vty->fd);
      }
    }
  }
}  
/******************************************************************/
/* Execute command in child process. */
static int imish_kernel_execute_command (struct vty *vty, const char *command, int argc, const char *argv[])
{
  int ret;
  int status;

  /* Call fork(). */
  imish_kernel.pid = fork ();

  if (imish_kernel.pid < 0)
    {
      /* Failure of fork(). */
      fprintf (stderr, "Can't fork: %s\n", safe_strerror (errno));
      exit (1);
    }
  else if (imish_kernel.pid == 0)
    {
      /* This is child process. */
      //extern int execlp (__const char *__file, __const char *__arg, ...)__THROW __nonnull ((1, 2));
      //close(execlp_linux.pfd[0]);
      dup2(imish_kernel.pfd[1], 0);
      dup2(imish_kernel.pfd[1], 1);
      dup2(imish_kernel.pfd[1], 2);
      //dup2(vty->fd, 2);
      switch (argc)
			{
			case 0:
			  ret = execlp (command, command, (const char *)NULL);
			  break;
			case 1:
			  ret = execlp (command, command, argv[0], (const char *)NULL);
			  break;
			case 2:
			  ret = execlp (command, command, argv[0], argv[1], (const char *)NULL);
			  break;
			case 3:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], (const char *)NULL);
			  break;
			case 4:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], (const char *)NULL);
			  break;
			case 5:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], argv[4], (const char *)NULL);
			  break;
			case 6:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], (const char *)NULL);
			  break;		
			case 7:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6],(const char *)NULL);
			  break;		
			case 8:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], (const char *)NULL);
			  break;		
			case 9:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], (const char *)NULL);
			  break;		
			case 10:
			  ret = execlp (command, command, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9], (const char *)NULL);
			  break;			  			  
			}
      /* When execlp suceed, this part is not executed. */
      fprintf (stderr, "Can't execute %s: %s\n", command, safe_strerror (errno));
      exit (1);
    }
  else
    {
      /* This is parent. */
      //ret = wait4 (imish_kernel.pid, &status, 0, NULL);
      //imish_kernel.pid = -1;
    }
  return 0;
}
/******************************************************************/
static int imish_kernel_start_shell(struct vty *vty)
{
	if(imish_kernel.ptid > 0)
		return CMD_SUCCESS;
		
  imish_kernel.pid = -1;
  if(pipe(imish_kernel.pfd))
	{
			vty_out (vty, "pipe failed : %s%s", safe_strerror (errno),VTY_NEWLINE);
			return CMD_WARNING;
	}
  imish_kernel.ptid = pthread_create(&imish_kernel.pthid, NULL, &imish_kernel_pthread, &imish_kernel);
  if(imish_kernel.ptid < 0)
	{
			vty_out (vty, "pthread create befor fork: %s%s", safe_strerror (errno),VTY_NEWLINE);
			return CMD_WARNING;
	}
  imish_kernel.status = 1;
  imish_kernel.vty = vty;
  return CMD_SUCCESS;
}
/******************************************************************/
static int imish_kernel_stop_shell(struct vty *vty)
{
	int status = 0;
	if(imish_kernel.ptid <= 0)
		return CMD_SUCCESS;	
  if(imish_kernel.pid != -1)
  {
    wait4 (imish_kernel.pid, &status, 0, NULL);
  }
  if( (imish_kernel.ptid >= 0)&&(imish_kernel.pthid >= 0) )
  {
    pthread_cancel(imish_kernel.pthid);
  }
  if(imish_kernel.pfd[0] > 0)
    close(imish_kernel.pfd[0]);
  if(imish_kernel.pfd[1] > 0)    
    close(imish_kernel.pfd[1]);
  imish_kernel.pid = -1;
  imish_kernel.status = 0;
  imish_kernel.vty = NULL;
  return CMD_SUCCESS;
}
/******************************************************************/
DEFUN (start_shell,
       start_shell_cmd,
       "start-shell",
       "Start UNIX shell\n")
{
	if(imish_kernel_start_shell(vty)==CMD_SUCCESS)
	{
  	vty->node = LINUX_SHELL_NODE;
  	return CMD_SUCCESS;
	}
	return CMD_WARNING;
}


DEFUN (no_start_shell,
       no_start_shell_cmd,
       "no start-shell",
       NO_STR
       "Start UNIX shell\n")
{
	if(imish_kernel_stop_shell(vty)==CMD_SUCCESS)
	{
  	vty->node = CONFIG_NODE;
  	return CMD_SUCCESS;
	}
	return CMD_WARNING;	
}

DEFUN (start_shell_ls,
       start_shell_ls_cmd,
       "ls [DIR]",
       "list information of dir in detail\n"
       "dir name\n")
{
	char *argv_v[2] = {NULL, NULL};
	if(argv[0] == NULL)
		argv_v[0] = strdup("./");
	else
		argv_v[0] = strdup(argv[0]);
	argv_v[1] = strdup("-l");
  imish_kernel_execute_command (vty, "ls", 2, argv_v);
  if(argv_v[0])
  	free(argv_v[0]);
  if(argv_v[1])
  	free(argv_v[1]);
  return CMD_SUCCESS;
}

DEFUN (start_shell_ping,
       start_shell_ping_cmd,
       "ping A.B.C.D COUNT",
       "IPv4 Send echo messages\n"
       "Ping destination address or hostname\n"
       "Number of echo messages sending\n")
{
	char *argv_v[3] = {NULL, NULL, NULL};
	if( (argv[0] == NULL)||(argv[1] == NULL) )
		return CMD_WARNING;
	argv_v[0] = strdup(argv[0]);
	argv_v[1] = strdup("-c");
	argv_v[2] = strdup(argv[1]);
  imish_kernel_execute_command (vty, "ping", 3, argv_v);
  if(argv_v[0])
  	free(argv_v[0]);
  if(argv_v[1])
  	free(argv_v[1]);
  if(argv_v[2])
  	free(argv_v[2]);
  return CMD_SUCCESS;
}

DEFUN (start_shell_traceroute,
       start_shell_traceroute_cmd,
       "traceroute A.B.C.D",
       "Trace route to destination\n"
       "Trace route to destination address or hostname\n")
{
	if( (argv[0] == NULL)/*||(argv[1] == NULL)*/ )
		return CMD_WARNING;
  imish_kernel_execute_command (vty, "traceroute", 1, argv);
  return CMD_SUCCESS;
}


#ifdef HAVE_IPV6
DEFUN (start_shell_ping6,
       start_shell_ping6_cmd,
       "ping6 X:X::X:X CONUT",
       "IPv6 echo Send echo messages\n"
       "Ping destination address or hostname\n"
       "Number of echo messages sending\n")
{
	char *argv_v[3] = {NULL, NULL, NULL};
	if( (argv[0] == NULL)||(argv[1] == NULL) )
		return CMD_WARNING;
	argv_v[0] = strdup(argv[0]);
	argv_v[1] = strdup("-c");
	argv_v[2] = strdup(argv[1]);
  imish_kernel_execute_command (vty, "ping6", 3, argv_v);
  if(argv_v[0])
  	free(argv_v[0]);
  if(argv_v[1])
  	free(argv_v[1]);
  if(argv_v[2])
  	free(argv_v[2]);
  //execute_command (vty, "ping6", argc, argv);
  return CMD_SUCCESS;
}

DEFUN (start_shell_traceroute6,
       start_shell_traceroute6_cmd,
       "traceroute6 X:X::X:X",
       "IPv6 Trace route to destination\n"
       "Trace route to destination address or hostname\n")
{
	if( (argv[0] == NULL)/*||(argv[1] == NULL)*/ )
		return CMD_WARNING;
  imish_kernel_execute_command (vty, "traceroute6", 1, argv);
  return CMD_SUCCESS;
}
#endif

/*
DEFUN (start_shell_ps,
       start_shell_ps_cmd,
       "show process",
       SHOW_STR
       "report a snapshot of the current processes\n")
{
	char *argv_v[2];
	argv_v[0] = strdup("-Al");
  execute_command (vty, "ps", 1, argv_v);
  if(argv_v[0])
  	free(argv_v[0]);  
  return CMD_SUCCESS;
}
*/
DEFUN (start_shell_show_process_name,
       start_shell_show_process_name_cmd,
       "show process NAME",
       SHOW_STR
       "report a snapshot of the current processes\n"
       "processes of name\n")
{
	char *argv_v[2];
	if( (argv[0] == NULL)/*||(argv[1] == NULL)*/ )
		return CMD_WARNING;
	if(argv[0])
	{
		argv_v[0] = strdup("-lfC");
		argv_v[1] = strdup(argv[0]);		
  	imish_kernel_execute_command (vty, "ps", 2, argv_v);
	}
  if(argv_v[0])
  	free(argv_v[0]);
  if(argv_v[1])
  	free(argv_v[1]);  
  return CMD_SUCCESS;
}

DEFUN (start_shell_show_process,
       start_shell_show_process_cmd,
       "show process",
       SHOW_STR
       "report a snapshot of the current processes\n")
{
	char *argv_v[2];
	argv_v[0] = strdup("-Al");
	imish_kernel_execute_command (vty, "ps", 1, argv_v);
  if(argv_v[0])
  	free(argv_v[0]);
  return CMD_SUCCESS;
}

DEFUN (start_shell_show_netstat,
       start_shell_show_netstat_cmd,
       "show netstat TYPE",
       SHOW_STR
       "Print network connections\n"
       "Print Type\n")
{
	if(argv[0])
	{		
  	imish_kernel_execute_command (vty, "netstat", 1, argv); 	
  	return CMD_SUCCESS;
	}
  return CMD_WARNING;
}
DEFUN (start_shell_show_route,
       start_shell_show_route_cmd,
       "show kernel route",
       SHOW_STR
       "linux kernel \n"
       "routing table\n")
{
	char *argv_v[2];
	argv_v[0] = strdup("-n");
	imish_kernel_execute_command (vty, "route", 1, argv); 	
  if(argv_v[0])
  	free(argv_v[0]);
  return CMD_SUCCESS;
}

DEFUN (start_shell_show_ifconfig,
       start_shell_show_ifconfig_name_cmd,
       "show kernel interface NAME",
       SHOW_STR
       "linux kernel\n"
       "network interface\n"
       "interface name\n")
{
	if(argv[0])
		imish_kernel_execute_command (vty, "ifconfig", 1, argv); 	
  else
  {
  	char *argv_v[2];
  	argv_v[0] = strdup("-a");
  	imish_kernel_execute_command (vty, "ifconfig", 1, argv_v); 
  	if(argv_v[0])
  		free(argv_v[0]);  	
  }
  return CMD_SUCCESS;
}
ALIAS (start_shell_show_ifconfig,
       start_shell_show_ifconfig_cmd,
       "show kernel interface",
       SHOW_STR
       "linux kernel\n"
       "network interface\n")
/*
DEFUN (start_shell_telnet_port,
       start_shell_telnet_port_cmd,
       "telnet WORD PORT",
       "Open a telnet connection\n"
       "IP address or hostname of a remote system\n"
       "TCP Port number\n")
{
  execute_command (vty, "telnet", argc, argv);
  return CMD_SUCCESS;
}

ALIAS (start_shell_telnet_port,
       start_shell_telnet_cmd,
       "telnet WORD",
       "Open a telnet connection\n"
       "IP address or hostname of a remote system\n")

DEFUN (start_shell_ssh,
       start_shell_ssh_cmd,
       "ssh WORD",
       "Open an ssh connection\n"
       "[user@]host\n")
{
  execute_command (vty, "ssh", 1, argv);
  return CMD_SUCCESS;
}
*/
/*
# dhcpd
# Command line options here
DHCPDARGS=eth0

# dhcpd.conf
#
# Sample configuration file for ISC dhcpd
#
default-lease-time 600;
max-lease-time 7200;
# Use this to send dhcp log messages to a different log file (you also
# have to hack syslog.conf to complete the redirection).
log-facility local7;

subnet 10.254.239.0 netmask 255.255.255.224 {
range 10.254.239.10 10.254.239.20;
option routes 192.168.128.129
}
*/

DEFUN (ip_dhcp_enable,
       ip_dhcp_enable_cmd,
       "ip dhcp enable",
       IP_STR
       "dhcp server config\n"
       "enable dhcp\n")
{
  char cmd[128];	
  unsigned char *buf;
  struct listnode *addrnode;
  struct connected *ifc;
  struct prefix *p;
  struct interface *ifp = (struct interface *)vty->index;
  
  addrnode = (struct listnode *)(ifp->connected->head);
  ifc= (struct connected *)(addrnode->data);
  if(ifc)	
  {
		p = (struct prefix *)(ifc->address);
		buf = (unsigned char *)(&p->u.prefix);
  	memset(cmd, 0, sizeof(cmd));
  	sprintf(cmd, "dhcpd-setup.sh %s  %d.%d.%d.%d", ifp->name, buf[0], buf[1], buf[2], buf[3]);
  	imish_kernel_system(1, cmd, NULL, 0);
	return CMD_SUCCESS;
  }
  return CMD_WARNING;
}

static struct cmd_node linux_shell_node =
{
  LINUX_SHELL_NODE,
  "linux-sh> ",
  1
};

//����ִ��linux��ϵͳ�������̣�NAT,NSTP,DHCP,FTP,QOS,DNS,IPTABLE, 
static int imish_kernel_service_cmd_init(void)
{
	return 0;	
}


int imish_kernel_shell_cmd_init(void)
{
  memset(&imish_kernel, 0, sizeof(imish_kernel));
  //install_node (&linux_shell_node, NULL);
  
  install_element (CONFIG_NODE, &start_shell_cmd);
  //install_element (CONFIG_NODE, &no_start_shell_cmd); 
  install_element (LINUX_SHELL_NODE, &no_start_shell_cmd);
  
  //install_element (LINUX_SHELL_NODE, &config_list_cmd);
  //install_element (LINUX_SHELL_NODE, &config_exit_cmd);
  //install_element (LINUX_SHELL_NODE, &config_quit_cmd);
  //install_element (LINUX_SHELL_NODE, &config_help_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_ls_cmd);  
  
  install_element (LINUX_SHELL_NODE, &start_shell_ping_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_traceroute_cmd);
#ifdef HAVE_IPV6
  install_element (LINUX_SHELL_NODE, &start_shell_ping6_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_traceroute6_cmd);
#endif//HAVE_IPV6
  
  install_element (LINUX_SHELL_NODE, &start_shell_show_process_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_show_process_name_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_show_netstat_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_show_route_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_show_ifconfig_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_show_ifconfig_name_cmd);    
/*
  install_element (LINUX_SHELL_NODE, &start_shell_telnet_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_telnet_port_cmd);
  install_element (LINUX_SHELL_NODE, &start_shell_ssh_cmd);
*/
  //install_element (INTERFACE_NODE, &ip_dhcp_enable_cmd);
	imish_kernel_service_cmd_init();
  return 0;
}
int imish_kernel_shell_cmd_exit(void)
{
	return imish_kernel_stop_shell(NULL);
}
#endif// IMISH_IMI_MODULE  

