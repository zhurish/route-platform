/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include <zebra.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <lib/version.h>
#include "command.h"
#include "memory.h"
#include "thread.h"
#include "log.h"
#include "vty.h"
#include "bgpd/bgp_vty.h"


#ifdef IMISH_IMI_MODULE

#include "imi-cli/imi-sh.h"



static struct thread *imish_connect = NULL;
/* IMI shell client structure. */
struct imish_client
{
  int fd;
  const char *name;
  const int flag;
  const char *path;
  const char *pid_path; 
#define IMISH_CLIENT_TABLE(a,b,c) .flag = IMISH_ ## a, .path = b ## _VTYSH_PATH, .pid_path = PATH_ ## c ## _PID 
  pid_t	pid;//保存子进程PID信息
} imish_client[] =
{	
  { .fd = -1, .name = "zebra", 	.flag = IMISH_ZEBRA, .path = ZEBRA_VTYSH_PATH, .pid_path = PATH_ZEBRA_PID, .pid = 0},
  { .fd = -1, .name = "ripd", 	.flag = IMISH_RIPD, .path = RIP_VTYSH_PATH, .pid_path = PATH_RIPD_PID, .pid = 0},
  { .fd = -1, .name = "ripngd", .flag = IMISH_RIPNGD, .path = RIPNG_VTYSH_PATH, .pid_path = PATH_RIPNGD_PID, .pid = 0},
  { .fd = -1, .name = "ospfd", 	.flag = IMISH_OSPFD, .path = OSPF_VTYSH_PATH, .pid_path = PATH_OSPFD_PID, .pid = 0},
  { .fd = -1, .name = "ospf6d", .flag = IMISH_OSPF6D, .path = OSPF6_VTYSH_PATH, .pid_path = PATH_OSPF6D_PID, .pid = 0},
  { .fd = -1, .name = "bgpd", 	.flag = IMISH_BGPD, .path = BGP_VTYSH_PATH, .pid_path = PATH_BGPD_PID, .pid = 0},
  { .fd = -1, .name = "isisd", 	.flag = IMISH_ISISD, .path = ISIS_VTYSH_PATH, .pid_path = PATH_ISISD_PID, .pid = 0},
  { .fd = -1, .name = "pimd", 	.flag = IMISH_PIMD, .path = PIM_VTYSH_PATH, .pid_path = PATH_PIMD_PID, .pid = 0},
/* 2016年7月2日 22:04:29  zhurish: 扩展路由协议后增加链接到路由协议客户端的定义  */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM    
#ifdef OLSR_VTYSH_PATH
  { .fd = -1, .name = "olsrd", .flag = IMISH_OLSRD, .path = OLSR_VTYSH_PATH, .pid_path = PATH_OLSRD_PID, .pid = 0},  
#endif
  #endif /* HAVE_EXPAND_ROUTE_PLATFORM */
/* 2016年7月2日 22:04:29  zhurish: 扩展路由协议后增加链接到路由协议客户端的定义  */
};

static pid_t imish_pid_input (const char *fullfath)
{
  FILE *fp;
  char buf[64];
  if(fullfath == NULL)
	  return -1;
  fp = fopen (fullfath, "r");
  if (fp != NULL) 
  {
	  memset(buf, 0, sizeof(buf));
      if(fgets (buf, sizeof(buf), fp))
      {
    	  fclose (fp);
    	  return atoi(buf);
      } 
  }
  return -1;
}
/* Making connection to protocol daemon. */
static int imish_module_connect (struct imish_client *vclient)
{
  int ret;
  int sock, len;
  struct sockaddr_un addr;
  struct stat s_stat;

  /* Stat socket to see if we have permission to access it. */
  ret = stat (vclient->path, &s_stat);
  if (ret < 0 && errno != ENOENT)
  {
      fprintf  (stderr, "imish_connect(%s): stat = %s\n", vclient->path, safe_strerror(errno)); 
      exit(1);
  }
  
  if (ret >= 0)
  {
    if (! S_ISSOCK(s_stat.st_mode))
	  {
	  	fprintf (stderr, "imish_connect(%s): Not a socket\n",vclient->path);
	  	exit (1);
	  }
  }
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
#ifdef DEBUG
      fprintf(stderr, "imish_module_connect(%s): socket = %s\n", vclient->path,safe_strerror(errno));
#endif /* DEBUG */
      return -1;
    }

  memset (&addr, 0, sizeof (struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, vclient->path, strlen (vclient->path));
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
  len = addr.sun_len = SUN_LEN(&addr);
#else
  len = sizeof (addr.sun_family) + strlen (addr.sun_path);
#endif /* HAVE_STRUCT_SOCKADDR_UN_SUN_LEN */

  ret = connect (sock, (struct sockaddr *) &addr, len);
  if (ret < 0)
  {
#ifdef DEBUG
      fprintf(stderr, "imish_module_connect(%s): connect = %s\n", vclient->path, safe_strerror(errno));
#endif /* DEBUG */
      close (sock);
      return -1;
  }
  vclient->fd = sock;
  ret = imish_pid_input (vclient->pid_path);
  if(ret != -1)
	  vclient->pid = ret;
  zlog_debug("IMISH connect to %s",vclient->name);
  return 0;
}
//连接路由服务端
int imish_module_connect_all(const char *daemon_name)
{
  u_int i;
  int rc = 0;
  int matches = 0;

  for (i = 0; i < array_size(imish_client); i++)
  {
    if (!daemon_name || !strcmp(daemon_name, imish_client[i].name))
		{
	  	matches++;
	  	if (imish_module_connect(&imish_client[i]) == 0)
	    	rc++;
		}
  }
  if (!matches)
    fprintf(stderr, "Error: no daemons match name %s!\n", daemon_name);
  return rc;
}
//关闭路由服务端
static void imish_module_vclient_close (struct imish_client *vclient)
{
  if (vclient->fd >= 0)
    {
      //fprintf(stderr,
	    //  "Warning: closing connection to %s because of an I/O error!\n",vclient->name);
	  zlog_debug("IMISH disconnect to %s",vclient->name);
      close (vclient->fd);
      vclient->fd = -1;
    }
}

static int imish_module_close_all(void)
{
  u_int i; 
  for (i = 0; i < array_size(imish_client); i++)
  {
    if(imish_client[i].fd >= 0)
	  	imish_module_vclient_close(&imish_client[i]);
  }
  return 0;
}

static int imish_module_connect_thread(struct thread *thread)
{
  int ret = 0;	
  u_int i;
  for (i = 0; i < array_size(imish_client); i++)
  {
	ret = imish_pid_input (imish_client[i].pid_path);
    if( (imish_client[i].pid > 0)&&(imish_client[i].pid != ret) )
	  	imish_module_vclient_close(&imish_client[i]);
  }   
  for (i = 0; i < array_size(imish_client); i++)
  {
    if(imish_client[i].fd == -1)
	  	imish_module_connect(&imish_client[i]);
  }
  imish_connect = thread_add_timer (master, imish_module_connect_thread, NULL, 2);
  return 0;
}

int imish_module_client_put (struct vty *vty, const char *buf, int out)
{
  char *begin;
  char *pnt;
  int len = 0;
  begin = pnt = buf;
  while (*pnt != '\0')
	{
    if (*pnt == '\n')
		{
	  	*pnt++ = '\0';
	  	//imish_config_parse_line (begin);
	  	len = pnt - begin;
	  	buffer_put (vty->obuf, (u_char *)begin, len);
	  	buffer_put (vty->obuf, "\r\n", 2);
	  	//vty_out(vty, begin,);
	  	begin = pnt;
		}
    else
		{
	  	pnt++;
		}
  }
  return 0;
}
//执行路由服务端命令
#define ERR_WHERE_STRING "imish(): imish_module_client_execute_one(): "
static int imish_module_client_execute_one (struct imish_client *vclient, char *cmd, struct vty *vty, int out)
{
  int ret;
  char *buf;
  size_t bufsz;
  char *pbuf;
  size_t left;
  char *eoln;
  int nbytes;
  int i;
  int readln;
  int numnulls = 0;

  if (vclient->fd < 0)
    return CMD_SUCCESS;
    
  ret = write (vclient->fd, cmd, strlen (cmd) + 1);
  if (ret <= 0)
  {
      imish_module_vclient_close (vclient);
      return CMD_SUCCESS;
  }
	
  /* Allow enough room for buffer to read more than a few pages from socket. */
  bufsz = 5 * getpagesize() + 1;
  buf = XMALLOC(MTYPE_TMP, bufsz);
  memset(buf, 0, bufsz);
  pbuf = buf;

  while (1)
  {
    if (pbuf >= ((buf + bufsz) -1))
		{
	  	vty_out (vty, ERR_WHERE_STRING "warning - pbuf beyond buffer end.%s",VTY_NEWLINE);
	  	return CMD_WARNING;
		}
    readln = (buf + bufsz) - pbuf - 1;
    nbytes = read (vclient->fd, pbuf, readln);
    if (nbytes <= 0)
		{
	  	if (errno == EINTR)
	    	continue;
	  	vty_out (vty, ERR_WHERE_STRING "(%u)%s", errno,VTY_NEWLINE);
	  	if (errno == EAGAIN || errno == EIO)
	    	continue;

	  	imish_module_vclient_close (vclient);
	  	XFREE(MTYPE_TMP, buf);
	  	return CMD_SUCCESS;
		}
    /* If we have already seen 3 nulls, then current byte is ret code */
    if ((numnulls == 3) && (nbytes == 1))
    {
			ret = pbuf[0];
			break;
    }
    pbuf[nbytes] = '\0';

       /* If the config needs to be written in file or stdout */
       /*
       if (fp)
       {
         fputs(pbuf, fp);
         fflush (fp);
       }
       */
       //vty_out (vty,"%s%s",pbuf,VTY_NEWLINE);
       //fputs(pbuf, stdout);
       //fflush (stdout);
       /* At max look last four bytes */
       if (nbytes >= 4)
       {
         i = nbytes - 4;
         numnulls = 0;
       }
       else
         i = 0;

       /* Count the numnulls */ 
       while (i < nbytes && numnulls <3)
       {
         if (pbuf[i++] == '\0')
            numnulls++;
         else
            numnulls = 0;
       }
       /* We might have seen 3 consecutive nulls so store the ret code before updating pbuf*/
       ret = pbuf[nbytes-1];
       pbuf += nbytes;

       /* See if a line exists in buffer, if so parse and consume it, and
        * reset read position. If 3 nulls has been encountered consume the buffer before 
        * next read.
        */
       if (((eoln = strrchr(buf, '\n')) == NULL) && (numnulls<3))
         continue;

       if (eoln >= ((buf + bufsz) - 1))
       {
          vty_out (vty, ERR_WHERE_STRING "warning - eoln beyond buffer end.%s",VTY_NEWLINE);
       }

       /* If the config needs parsing, consume it */
       if(out == 0)
       	imish_module_config_parse(buf);
			 else if(out == 1)
			 	imish_module_client_put(vty, buf, 0);
			 	//vty_out(vty,"%s%s",buf,VTY_NEWLINE);
       eoln++;
       left = (size_t)(buf + bufsz - eoln);
       /*
        * This check is required since when a config line split between two consecutive reads, 
        * then buf will have first half of config line and current read will bring rest of the 
        * line. So in this case eoln will be 1 here, hence calculation of left will be wrong. 
        * In this case we don't need to do memmove, because we have already seen 3 nulls.  
        */
       if(left < bufsz)
         memmove(buf, eoln, left);

       buf[bufsz-1] = '\0';
       pbuf = buf + strlen(buf);
       /* got 3 or more trailing NULs? */
       if ((numnulls >=3) && (i < nbytes))
       {
          break;
       }
    }
  //if(!fp)
  if(buf)
  {
  	if(out == 0)
    	imish_module_config_parse(buf);
		else if(out == 1)
			imish_module_client_put(vty, buf, 0);
			//vty_out(vty,"%s%s",buf,VTY_NEWLINE);
  	XFREE(MTYPE_TMP, buf);
  }
  return ret;
}
//执行某个客户端路由的命令
int imish_module_client_execute(int client, const char *cmd, struct vty *vty, int out)
{
  unsigned int i;
  int ret = CMD_SUCCESS;
  for (i = 0; i < array_size(imish_client); i++)
    if ( imish_client[i].fd >= 0 )
      {
        if(client & imish_client[i].flag)
          ret |= imish_module_client_execute_one (&imish_client[i], cmd, vty, out);
        if(ret != CMD_SUCCESS) 
          break;//return CMD_WARNING;
      }
  return ret;
} 

/* Command execution over the imi interface. */
static int imish_module_execute_callbak (struct vty *vty, const char *line, int out)
{
  int ret, cmd_stat;
  vector vline;
  struct cmd_element *cmd = NULL;
  int tried = 0;
  int saved_ret, saved_node;
  /* Split readline string up into the vector. */
  vline = cmd_make_strvec (line);
  if (vline == NULL)
    return CMD_SUCCESS;
  saved_ret = ret = cmd_execute_command (vline, vty, &cmd, 1);
  saved_node = vty->node;
  /* If command doesn't succeeded in current node, try to walk up in node tree.
   * Changing imi->node is enough to try it just out without actual walkup in
   * the imish. */
  while (ret != CMD_SUCCESS && ret != CMD_SUCCESS_DAEMON && ret != CMD_WARNING
	 && vty->node > CONFIG_NODE)
  {
      vty->node = node_parent(vty->node);
      ret = cmd_execute_command (vline, vty, &cmd, 1);
      tried++;
  }//end while
  vty->node = saved_node;
  /* If command succeeded in any other node than current (tried > 0) we have
   * to move into node in the imish where it succeeded. */
  if (ret == CMD_SUCCESS || ret == CMD_SUCCESS_DAEMON || ret == CMD_WARNING)
  {
    if ((saved_node == BGP_VPNV4_NODE /*|| saved_node == BGP_VPNV6_NODE
	   	|| saved_node == BGP_ENCAP_NODE || saved_node == BGP_ENCAPV6_NODE*/
           || saved_node == BGP_IPV4_NODE
	   	|| saved_node == BGP_IPV6_NODE || saved_node == BGP_IPV4M_NODE
	   	|| saved_node == BGP_IPV6M_NODE)
	  	&& (tried == 1))
		{
	  	//imish_module_execute(vty, "exit-address-family", out);
	  	imish_module_client_execute(IMISH_BGPD, "exit-address-family", vty, out);
	  	vty->node = BGP_NODE;//CONFIG_NODE;
		}//end if
    else if ((saved_node == KEYCHAIN_KEY_NODE) && (tried == 1))
		{
	  	//imish_module_execute(vty, "exit", out);
	  	imish_module_client_execute(IMISH_RIPD, "exit", vty, out);
	  	vty->node = KEYCHAIN_NODE;//CONFIG_NODE;
		}// end else if
    else if (tried)
		{
	  	//imish_module_execute (vty, "end", out);
	  	//imish_module_execute (vty, "configure terminal", out);
	  	imish_module_client_execute(IMISH_ALL, "end", vty, out);
	  	imish_module_client_execute(IMISH_ALL, "configure terminal", vty, out);
	  	vty->node = CONFIG_NODE;
		}
  }//end if
  /* If command didn't succeed in any node, continue with return value from first try. */
  else if (tried)
  {
      ret = saved_ret;
  }
  cmd_free_strvec (vline);
  cmd_stat = ret;
  switch (ret)
  {
  	case CMD_WARNING:
			if (vty->type == VTY_FILE)
	  		vty_out (vty, "Warning...%s", VTY_NEWLINE);
			break;
    case CMD_ERR_AMBIGUOUS:
			vty_out (vty, "%% Ambiguous command.%s", VTY_NEWLINE);
			break;
    case CMD_ERR_NO_MATCH:
			vty_out (vty, "%% Unknown command: %s%s", vty->buf,VTY_NEWLINE);
			break;
    case CMD_ERR_INCOMPLETE:
			vty_out (vty, "%% Command incomplete.%s", VTY_NEWLINE);
			break;
    case CMD_SUCCESS_DAEMON:
  
		cmd_stat = CMD_SUCCESS;
		cmd_stat = imish_module_client_execute(cmd->daemon, line, vty, out);

		if (cmd_stat != CMD_SUCCESS)
	  	break;

		if (cmd && cmd->func)
	  	(*cmd->func) (cmd, vty, 0, NULL);
	  	break;
    }//switch
  //}
  return cmd_stat;
}


//提供给外部调用执行;
int imish_module_execute (struct vty *vty, const char *line, int out)
{
  return imish_module_execute_callbak (vty, line, out);
}
//模块安装到vty的钩子函数
static int imish_sh_execute (struct vty *vty, const char *line)
{
  return imish_module_execute_callbak (vty, line, 1);
}

/* Vty node structures. */
static struct cmd_node bgp_node =
{
  BGP_NODE,
  "%s(config-router)# ",
};

static struct cmd_node rip_node =
{
  RIP_NODE,
  "%s(config-router)# ",
};

static struct cmd_node isis_node =
{
  ISIS_NODE,
  "%s(config-router)# ",
};
/* 2016年7月2日 22:07:39 zhurish: 扩展路由协议增加命令节点 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
static struct cmd_node olsr_node =
{
  OLSR_NODE,
  "%s(config-router)# ",
};
#endif
/* 2016年7月2日 22:07:39  zhurish: 扩展路由协议增加命令节点 */
static struct cmd_node interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
};

static struct cmd_node rmap_node =
{
  RMAP_NODE,
  "%s(config-route-map)# "
};

static struct cmd_node zebra_node =
{
  ZEBRA_NODE,
  "%s(config-router)# "
};

static struct cmd_node bgp_vpnv4_node =
{
  BGP_VPNV4_NODE,
  "%s(config-router-af)# "
};
/*
static struct cmd_node bgp_vpnv6_node =
{
  BGP_VPNV6_NODE,
  "%s(config-router-af)# "
};

static struct cmd_node bgp_encap_node =
{
  BGP_ENCAP_NODE,
  "%s(config-router-af)# "
};

static struct cmd_node bgp_encapv6_node =
{
  BGP_ENCAPV6_NODE,
  "%s(config-router-af)# "
};
*/
static struct cmd_node bgp_ipv4_node =
{
  BGP_IPV4_NODE,
  "%s(config-router-af)# "
};

static struct cmd_node bgp_ipv4m_node =
{
  BGP_IPV4M_NODE,
  "%s(config-router-af)# "
};

static struct cmd_node bgp_ipv6_node =
{
  BGP_IPV6_NODE,
  "%s(config-router-af)# "
};

static struct cmd_node bgp_ipv6m_node =
{
  BGP_IPV6M_NODE,
  "%s(config-router-af)# "
};

static struct cmd_node ospf_node =
{
  OSPF_NODE,
  "%s(config-router)# "
};

static struct cmd_node ripng_node =
{
  RIPNG_NODE,
  "%s(config-router)# "
};

static struct cmd_node ospf6_node =
{
  OSPF6_NODE,
  "%s(config-ospf6)# "
};

static struct cmd_node babel_node =
{
  BABEL_NODE,
  "%s(config-babel)# "
};

static struct cmd_node keychain_node =
{
  KEYCHAIN_NODE,
  "%s(config-keychain)# "
};

static struct cmd_node keychain_key_node =
{
  KEYCHAIN_KEY_NODE,
  "%s(config-keychain-key)# "
};

/* When '^Z' is received from imi, move down to the enable mode. */
static int
imish_end (struct vty *vty)
{
  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
    case RESTRICTED_NODE:    	
      /* Nothing to do. */
      break;
    default:
      vty_config_unlock (vty);    	
      vty->node = ENABLE_NODE;
      break;
    }
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 imish_end_all,
	 imish_end_all_cmd,
	 "end",
	 "End current mode and change to enable mode\n")
{
  return imish_end (vty);
}

DEFUNSH (IMISH_BGPD,
	 router_bgp,
	 router_bgp_cmd,
	 "router bgp " CMD_AS_RANGE,
	 ROUTER_STR
	 BGP_STR
	 AS_STR)
{
  vty->node = BGP_NODE;
  return CMD_SUCCESS;
}

ALIAS_SH (IMISH_BGPD,
	  router_bgp,
	  router_bgp_view_cmd,
	  "router bgp " CMD_AS_RANGE " view WORD",
	  ROUTER_STR
	  BGP_STR
	  AS_STR
	  "BGP view\n"
	  "view name\n")

DEFUNSH (IMISH_BGPD,
	 address_family_vpnv4,
	 address_family_vpnv4_cmd,
	 "address-family vpnv4",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_VPNV4_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_vpnv4_unicast,
	 address_family_vpnv4_unicast_cmd,
	 "address-family vpnv4 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_VPNV4_NODE;
  return CMD_SUCCESS;
}
/*
DEFUNSH (IMISH_BGPD,
	 address_family_vpnv6,
	 address_family_vpnv6_cmd,
	 "address-family vpnv6",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_VPNV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_vpnv6_unicast,
	 address_family_vpnv6_unicast_cmd,
	 "address-family vpnv6 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_VPNV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_encap,
	 address_family_encap_cmd,
	 "address-family encap",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_ENCAP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_encapv4,
	 address_family_encapv4_cmd,
	 "address-family encapv4",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_ENCAP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_encapv6,
	 address_family_encapv6_cmd,
	 "address-family encapv6",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_ENCAPV6_NODE;
  return CMD_SUCCESS;
}
*/
DEFUNSH (IMISH_BGPD,
	 address_family_ipv4_unicast,
	 address_family_ipv4_unicast_cmd,
	 "address-family ipv4 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV4_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv4_multicast,
	 address_family_ipv4_multicast_cmd,
	 "address-family ipv4 multicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV4M_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv6,
	 address_family_ipv6_cmd,
	 "address-family ipv6",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_IPV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv6_unicast,
	 address_family_ipv6_unicast_cmd,
	 "address-family ipv6 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv6_multicast,
	 address_family_ipv6_multicast_cmd,
	 "address-family ipv6 multicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV6M_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RIPD,
	 key_chain,
	 key_chain_cmd,
	 "key chain WORD",
	 "Authentication key management\n"
	 "Key-chain management\n"
	 "Key-chain name\n")
{
  vty->node = KEYCHAIN_NODE;
  return CMD_SUCCESS;
}	 

DEFUNSH (IMISH_RIPD,
	 key,
	 key_cmd,
	 "key <0-2147483647>",
	 "Configure a key\n"
	 "Key identifier number\n")
{
  vty->node = KEYCHAIN_KEY_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RIPD,
	 router_rip,
	 router_rip_cmd,
	 "router rip",
	 ROUTER_STR
	 "RIP")
{
  vty->node = RIP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RIPNGD,
	 router_ripng,
	 router_ripng_cmd,
	 "router ripng",
	 ROUTER_STR
	 "RIPng")
{
  vty->node = RIPNG_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_OSPFD,
	 router_ospf,
	 router_ospf_cmd,
	 "router ospf",
	 "Enable a routing process\n"
	 "Start OSPF configuration\n")
{
  vty->node = OSPF_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_OSPF6D,
	 router_ospf6,
	 router_ospf6_cmd,
	 "router ospf6",
	 OSPF6_ROUTER_STR
	 OSPF6_STR)
{
  vty->node = OSPF6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ISISD,
	 router_isis,
	 router_isis_cmd,
	 "router isis WORD",
	 ROUTER_STR
	 "ISO IS-IS\n"
	 "ISO Routing area tag")
{
  vty->node = ISIS_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RMAP,
	 route_map,
	 route_map_cmd,
	 "route-map WORD (deny|permit) <1-65535>",
	 "Create route-map or enter route-map command mode\n"
	 "Route map tag\n"
	 "Route map denies set operations\n"
	 "Route map permits set operations\n"
	 "Sequence to insert to/delete from existing route-map entry\n")
{
  vty->node = RMAP_NODE;
  return CMD_SUCCESS;
}
/* 2016年7月2日 22:07:55 zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
DEFUNSH (IMISH_OLSRD,
	 router_olsr,
	 router_olsr_cmd,
	 "router olsr",
	 ROUTER_STR
   "Start OLSR configuration\n")
{
  vty->node = OLSR_NODE;
  return CMD_SUCCESS;
}
#endif
/* 2016年7月2日 22:07:55  zhurish: 扩展路由协议增加命令 */
       
//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_enable, 
	 imish_enable_cmd,
	 "enable",
	 "Turn on privileged mode command\n")
{
  /* If enable password is NULL, change to ENABLE_NODE */
  if ((host.enable == NULL && host.enable_encrypt == NULL) ||
      vty->type == VTY_SHELL_SERV)
    vty->node = ENABLE_NODE;
  else
    vty->node = AUTH_ENABLE_NODE;

  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_disable, 
	 imish_disable_cmd,
	 "disable",
	 "Turn off privileged mode command\n")
{
  if (vty->node == ENABLE_NODE)
    vty->node = VIEW_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 imish_config_terminal,
	 imish_config_terminal_cmd,
	 "configure terminal",
	 "Configuration from imi interface\n"
	 "Configuration terminal\n")
{
  if (vty_config_lock (vty))
    vty->node = CONFIG_NODE;
  else
    {
      vty_out (vty, "VTY configuration is locked by other VTY%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  return CMD_SUCCESS;	
}

static int imish_exit (struct vty *vty)
{
  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
    case RESTRICTED_NODE:
    	
      if (vty_shell (vty))
			exit (0);
      else
				vty->status = VTY_CLOSE;
      break;
    case CONFIG_NODE:
      vty->node = ENABLE_NODE;
      vty_config_unlock (vty);
      break;
    case INTERFACE_NODE:
    case ZEBRA_NODE:
    case BGP_NODE:
    case RIP_NODE:
    case RIPNG_NODE:
    case OSPF_NODE:
    case OSPF6_NODE:
    case BABEL_NODE:
    case ISIS_NODE:
    case MASC_NODE:
    case RMAP_NODE:
    case VTY_NODE:
    case KEYCHAIN_NODE:
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
    case HSLS_NODE:		/* HSLS protocol node. */
    case OLSR_NODE:			/* OLSR protocol node. */
    case ICRP_NODE:		/* ICRP protocol node. */
    case FRP_NODE:                /* FRP protocol node */

    case LDP_NODE:
    case LDP_IF_NODE:
    case RSVP_NODE:
    case LLDP_NODE:
    case MPLS_NODE:
    //case IP_EXPLICIT_PATH_NODE:
    //case INTERFACE_TUNNEL_NODE:
#endif /* HAVE_EXPAND_ROUTE_PLATFORM */
      //imish_execute(vty,"end");
      //imish_execute(vty,"configure terminal");
      vty->node = CONFIG_NODE;
      break;
    case BGP_VPNV4_NODE:
    //case BGP_VPNV6_NODE:
    //case BGP_ENCAP_NODE:
    //case BGP_ENCAPV6_NODE:
    case BGP_IPV4_NODE:
    case BGP_IPV4M_NODE:
    case BGP_IPV6_NODE:
    case BGP_IPV6M_NODE:
      vty->node = BGP_NODE;
      break;
    case KEYCHAIN_KEY_NODE:
      vty->node = KEYCHAIN_NODE;
      break;
    default:
      break;
    }
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 imish_exit_all,
	 imish_exit_all_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_all,
       imish_quit_all_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
       
DEFUN (
	 imish_exit_imish,
	 imish_exit_imish_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}
ALIAS (imish_exit_imish,
       imish_quit_imish_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
       
DEFUNSH (IMISH_BGPD,
	 exit_address_family,
	 exit_address_family_cmd,
	 "exit-address-family",
	 "Exit from Address Family configuration mode\n")
{
  if (vty->node == BGP_IPV4_NODE
      || vty->node == BGP_IPV4M_NODE
      || vty->node == BGP_VPNV4_NODE
      //|| vty->node == BGP_VPNV6_NODE
      //|| vty->node == BGP_ENCAP_NODE
      //|| vty->node == BGP_ENCAPV6_NODE
      || vty->node == BGP_IPV6_NODE
      || vty->node == BGP_IPV6M_NODE)
    vty->node = BGP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ZEBRA,
	 imish_exit_zebra,
	 imish_exit_zebra_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_zebra,
       imish_quit_zebra_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_RIPD,
	 imish_exit_ripd,
	 imish_exit_ripd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ripd,
       imish_quit_ripd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_RIPNGD,
	 imish_exit_ripngd,
	 imish_exit_ripngd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ripngd,
       imish_quit_ripngd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_RMAP,
	 imish_exit_rmap,
	 imish_exit_rmap_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_rmap,
       imish_quit_rmap_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_BGPD,
	 imish_exit_bgpd,
	 imish_exit_bgpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_bgpd,
       imish_quit_bgpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_OSPFD,
	 imish_exit_ospfd,
	 imish_exit_ospfd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ospfd,
       imish_quit_ospfd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_OSPF6D,
	 imish_exit_ospf6d,
	 imish_exit_ospf6d_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ospf6d,
       imish_quit_ospf6d_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_ISISD,
	 imish_exit_isisd,
	 imish_exit_isisd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_isisd,
       imish_quit_isisd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_ALL,
         imish_exit_line_vty,
         imish_exit_line_vty_cmd,
         "exit",
         "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}
/* 2016年7月2日 22:08:08 zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
DEFUNSH (IMISH_OLSRD,
	 imish_exit_olsr,
	 imish_exit_olsr_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_olsr,
       imish_quit_olsr_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* 2016年7月2日 22:08:08  zhurish: 扩展路由协议增加命令 */
ALIAS (imish_exit_line_vty,
       imish_quit_line_vty_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUNSH (IMISH_INTERFACE,
	 imish_interface,
	 imish_interface_cmd,
	 "interface IFNAME",
	 "Select an interface to configure\n"
	 "Interface's name\n")
{
  vty->node = INTERFACE_NODE;
  return CMD_SUCCESS;
}
/*
ALIAS_SH (IMISH_ZEBRA,
	 imish_interface,
	 imish_interface_vrf_cmd,
	 "interface IFNAME " VRF_CMD_STR,
	 "Select an interface to configure\n"
	 "Interface's name\n"
	 VRF_CMD_HELP_STR)
*/
/* TODO Implement "no interface command in isisd. */
DEFSH (IMISH_ZEBRA|IMISH_RIPD|IMISH_RIPNGD|IMISH_OSPFD|IMISH_OSPF6D,
       imish_no_interface_cmd,
       "no interface IFNAME",
       NO_STR
       "Delete a pseudo interface's configuration\n"
       "Interface's name\n")
/*
DEFSH (IMISH_ZEBRA,
       imish_no_interface_vrf_cmd,
       "no interface IFNAME " VRF_CMD_STR,
       NO_STR
       "Delete a pseudo interface's configuration\n"
       "Interface's name\n"
       VRF_CMD_HELP_STR)
*/
/* TODO Implement interface description commands in ripngd, ospf6d
 * and isisd. */
DEFSH (IMISH_ZEBRA|IMISH_RIPD|IMISH_OSPFD,
       interface_desc_cmd,
       "description .LINE",
       "Interface specific description\n"
       "Characters describing this interface\n")
       
DEFSH (IMISH_ZEBRA|IMISH_RIPD|IMISH_OSPFD,
       no_interface_desc_cmd,
       "no description",
       NO_STR
       "Interface specific description\n")

DEFUNSH (IMISH_INTERFACE,
	 imish_exit_interface,
	 imish_exit_interface_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_interface,
       imish_quit_interface_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

DEFUN (imish_show_thread,
       imish_show_thread_cmd,
       "show thread cpu [FILTER]",
      SHOW_STR
      "Thread information\n"
      "Thread CPU usage\n"
      "Display filter (rwtexb)\n")
{
  unsigned int i;
  int ret = CMD_SUCCESS;
  char line[100];
  sprintf(line, "show thread cpu %s\n", (argc == 1) ? argv[0] : "");  
  for (i = 0; i < array_size(imish_client); i++)
    if ( imish_client[i].fd >= 0 )
      {
      	imish_module_config_format("Thread statistics for %s:\n", imish_client[i].name);
        ret |= imish_module_client_execute_one (&imish_client[i], line, vty, 0);
        imish_module_config_format("\n");
      }
	imish_module_config_show(vty);
  return ret;
}

DEFUN (imish_show_work_queues,
       imish_show_work_queues_cmd,
       "show work-queues",
       SHOW_STR
       "Work Queue information\n")
{
  unsigned int i;
  int ret = CMD_SUCCESS; 
  for (i = 0; i < array_size(imish_client); i++)
    if ( imish_client[i].fd >= 0 )
      {
      	imish_module_config_format("Work queue statistics for  %s:\n", imish_client[i].name);
        ret |= imish_module_client_execute_one (&imish_client[i], "show work-queues\n", vty, 0);
        imish_module_config_format("\n");
      }
	imish_module_config_show(vty);
  return ret;
}

/* Memory */
DEFUN (imish_show_memory,
       imish_show_memory_cmd,
       "show memory",
       SHOW_STR
       "Memory statistics\n")
{
  unsigned int i;
  int ret = CMD_SUCCESS; 
  for (i = 0; i < array_size(imish_client); i++)
    if ( imish_client[i].fd >= 0 )
      {
      	imish_module_config_format("Memory statistics for  %s:\n", imish_client[i].name);
      	ret |= imish_module_client_execute_one (&imish_client[i], "show memory\n", vty, 0);
        imish_module_config_format("\n"); 
      }
	imish_module_config_show(vty);
  return ret;	
}

/* Logging commands. */
static const struct imish_facility_map {
  int facility;
  const char *name;
  size_t match;
} imish_syslog_facilities[] = 
  {
    { LOG_KERN, "kern", 1 },
    { LOG_USER, "user", 2 },
    { LOG_MAIL, "mail", 1 },
    { LOG_DAEMON, "daemon", 1 },
    { LOG_AUTH, "auth", 1 },
    { LOG_SYSLOG, "syslog", 1 },
    { LOG_LPR, "lpr", 2 },
    { LOG_NEWS, "news", 1 },
    { LOG_UUCP, "uucp", 2 },
    { LOG_CRON, "cron", 1 },
#ifdef LOG_FTP
    { LOG_FTP, "ftp", 1 },
#endif
    { LOG_LOCAL0, "local0", 6 },
    { LOG_LOCAL1, "local1", 6 },
    { LOG_LOCAL2, "local2", 6 },
    { LOG_LOCAL3, "local3", 6 },
    { LOG_LOCAL4, "local4", 6 },
    { LOG_LOCAL5, "local5", 6 },
    { LOG_LOCAL6, "local6", 6 },
    { LOG_LOCAL7, "local7", 6 },
    { 0, NULL, 0 },
  };

static const char *imish_facility_name(int facility)
{
  const struct imish_facility_map *fm;

  for (fm = imish_syslog_facilities; fm->name; fm++)
    if (fm->facility == facility)
      return fm->name;
  return "";
}
static int imish_facility_match(const char *str)
{
  const struct imish_facility_map *fm;

  for (fm = imish_syslog_facilities; fm->name; fm++)
    if (!strncmp(str,fm->name,fm->match))
      return fm->facility;
  return -1;
}
static int imish_level_match(const char *s)
{
  int level ;
  
  for ( level = 0 ; zlog_priority [level] != NULL ; level ++ )
    if (!strncmp (s, zlog_priority[level], 2))
      return level;
  return ZLOG_DISABLED;
}
static int imish_set_log_file(struct vty *vty, const char *fname, int loglevel)
{
  int ret;
  char *p = NULL;
  const char *fullpath;
  
  /* Path detection. */
  if (! IS_DIRECTORY_SEP (*fname))
    {
      char cwd[MAXPATHLEN+1];
      cwd[MAXPATHLEN] = '\0';
      
      if (getcwd (cwd, MAXPATHLEN) == NULL)
        {
          zlog_err ("config_log_file: Unable to alloc mem!");
          return CMD_WARNING;
        }
      
      if ( (p = XMALLOC (MTYPE_TMP, strlen (cwd) + strlen (fname) + 2))
          == NULL)
        {
          zlog_err ("config_log_file: Unable to alloc mem!");
          return CMD_WARNING;
        }
      sprintf (p, "%s/%s", cwd, fname);
      fullpath = p;
    }
  else
    fullpath = fname;

  ret = zlog_set_file (NULL, fullpath, loglevel);

  if (p)
    XFREE (MTYPE_TMP, p);

  if (!ret)
    {
      vty_out (vty, "can't open logfile %s\n", fname);
      return CMD_WARNING;
    }

  if (host.logfile)
    XFREE (MTYPE_HOST, host.logfile);

  host.logfile = XSTRDUP (MTYPE_HOST, fname);

  return CMD_SUCCESS;
}


DEFUNSH (IMISH_ALL,
	 imish_log_stdout,
	 imish_log_stdout_cmd,
	 "log stdout",
	 "Logging control\n"
	 "Set stdout logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_STDOUT, zlog_default->default_lvl);	
  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_stdout_level,
	 imish_log_stdout_level_cmd,
	 "log stdout "LOG_LEVELS,
	 "Logging control\n"
	 "Set stdout logging level\n"
	 LOG_LEVEL_DESC)
{
  int level;
	char line[128];
	
  if ((level = imish_level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  sprintf(line,"log stdout %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING; 
  zlog_set_level (NULL, ZLOG_DEST_STDOUT, level);
  	
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_stdout,
	 no_imish_log_stdout_cmd,
	 "no log stdout [LEVEL]",
	 NO_STR
	 "Logging control\n"
	 "Cancel logging to stdout\n"
	 "Logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_STDOUT, ZLOG_DISABLED);	
  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_file,
	 imish_log_file_cmd,
	 "log file FILENAME",
	 "Logging control\n"
	 "Logging to file\n"
	 "Logging filename\n")
{
	char line[128];
  sprintf(line,"log file %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING; 
  return imish_set_log_file(vty, argv[0], zlog_default->default_lvl);	
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_file_level,
	 imish_log_file_level_cmd,
	 "log file FILENAME "LOG_LEVELS,
	 "Logging control\n"
	 "Logging to file\n"
	 "Logging filename\n"
	 LOG_LEVEL_DESC)
{
  int level;
	char line[128];
  if ((level = imish_level_match(argv[1])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;

  sprintf(line,"log file %s %s\n",argv[0],argv[1]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING; 
  	
  return imish_set_log_file(vty, argv[0], level);
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_file,
	 no_imish_log_file_cmd,
	 "no log file [FILENAME]",
	 NO_STR
	 "Logging control\n"
	 "Cancel logging to file\n"
	 "Logging file name\n")
{
  zlog_reset_file (NULL);

  if (host.logfile)
    XFREE (MTYPE_HOST, host.logfile);

  host.logfile = NULL;

  return CMD_SUCCESS;
}

ALIAS_SH (IMISH_ALL,
	  no_imish_log_file,
	  no_imish_log_file_level_cmd,
	  "no log file FILENAME LEVEL",
	  NO_STR
	  "Logging control\n"
	  "Cancel logging to file\n"
	  "Logging file name\n"
	  "Logging level\n")

DEFUNSH (IMISH_ALL,
	 imish_log_monitor,
	 imish_log_monitor_cmd,
	 "log monitor",
	 "Logging control\n"
	 "Set terminal line (monitor) logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_MONITOR, zlog_default->default_lvl);	
  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_monitor_level,
	 imish_log_monitor_level_cmd,
	 "log monitor "LOG_LEVELS,
	 "Logging control\n"
	 "Set terminal line (monitor) logging level\n"
	 LOG_LEVEL_DESC)
{
  int level;
	char line[128];
  if ((level = imish_level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  sprintf(line,"log monitor %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING; 
  zlog_set_level (NULL, ZLOG_DEST_MONITOR, level);
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_monitor,
	 no_imish_log_monitor_cmd,
	 "no log monitor [LEVEL]",
	 NO_STR
	 "Logging control\n"
	 "Disable terminal line (monitor) logging\n"
	 "Logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_MONITOR, ZLOG_DISABLED);	
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 imish_log_syslog,
	 imish_log_syslog_cmd,
	 "log syslog",
	 "Logging control\n"
	 "Set syslog logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, zlog_default->default_lvl);	
  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_syslog_level,
	 imish_log_syslog_level_cmd,
	 "log syslog "LOG_LEVELS,
	 "Logging control\n"
	 "Set syslog logging level\n"
	 LOG_LEVEL_DESC)
{
  int level;
	char line[128];
  if ((level = imish_level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  sprintf(line,"log syslog %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING;
  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, level);
  return CMD_SUCCESS;	
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_syslog,
	 no_imish_log_syslog_cmd,
	 "no log syslog [LEVEL]",
	 NO_STR
	 "Logging control\n"
	 "Cancel logging to syslog\n"
	 "Logging level\n")
{
  zlog_set_level (NULL, ZLOG_DEST_SYSLOG, ZLOG_DISABLED);	
  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_facility,
	 imish_log_facility_cmd,
	 "log facility "LOG_FACILITIES,
	 "Logging control\n"
	 "Facility parameter for syslog messages\n"
	 LOG_FACILITY_DESC)

{
  int facility;
	char line[128];
  if ((facility = imish_facility_match(argv[0])) < 0)
    return CMD_ERR_NO_MATCH;
  sprintf(line,"log facility %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING;    
  zlog_default->facility = facility;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_facility,
	 no_imish_log_facility_cmd,
	 "no log facility [FACILITY]",
	 NO_STR
	 "Logging control\n"
	 "Reset syslog facility to default (daemon)\n"
	 "Syslog facility\n")

{
  zlog_default->facility = LOG_DAEMON;	
  return CMD_SUCCESS;
}

//DEFUNSH_DEPRECATED (IMISH_ALL,
DEFUN(
		    imish_log_trap,
		    imish_log_trap_cmd,
		    "log trap "LOG_LEVELS,
		    "Logging control\n"
		    "(Deprecated) Set logging level and default for all destinations\n"
		    LOG_LEVEL_DESC)

{
  int new_level ;
  int i;
	char line[128];
  if ((new_level = imish_level_match(argv[0])) == ZLOG_DISABLED)
    return CMD_ERR_NO_MATCH;
  sprintf(line,"log trap %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING; 
  zlog_default->default_lvl = new_level;
  for (i = 0; i < ZLOG_NUM_DESTS; i++)
    if (zlog_default->maxlvl[i] != ZLOG_DISABLED)
      zlog_default->maxlvl[i] = new_level;
  return CMD_SUCCESS;
}

DEFUNSH_DEPRECATED (IMISH_ALL,
		    no_imish_log_trap,
		    no_imish_log_trap_cmd,
		    "no log trap [LEVEL]",
		    NO_STR
		    "Logging control\n"
		    "Permit all logging information\n"
		    "Logging level\n")
{
  zlog_default->default_lvl = LOG_DEBUG;	
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 imish_log_record_priority,
	 imish_log_record_priority_cmd,
	 "log record-priority",
	 "Logging control\n"
	 "Log the priority of the message within the message\n")
{
  zlog_default->record_priority = 1 ;	
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_record_priority,
	 no_imish_log_record_priority_cmd,
	 "no log record-priority",
	 NO_STR
	 "Logging control\n"
	 "Do not log the priority of the message within the message\n")
{
  zlog_default->record_priority = 0 ;	
  return CMD_SUCCESS;
}

//DEFUNSH (IMISH_ALL,
DEFUN (
	 imish_log_timestamp_precision,
	 imish_log_timestamp_precision_cmd,
	 "log timestamp precision <0-6>",
	 "Logging control\n"
	 "Timestamp configuration\n"
	 "Set the timestamp precision\n"
	 "Number of subsecond digits\n")
{
	char line[128];
  if (argc != 1)
    {
      vty_out (vty, "Insufficient arguments%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  sprintf(line,"log timestamp precision %s\n",argv[0]);  
  if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!= CMD_SUCCESS)
  	return CMD_WARNING; 
  	
  VTY_GET_INTEGER_RANGE("Timestamp Precision",
  			zlog_default->timestamp_precision, argv[0], 0, 6);
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,
	 no_imish_log_timestamp_precision,
	 no_imish_log_timestamp_precision_cmd,
	 "no log timestamp precision",
	 NO_STR
	 "Logging control\n"
	 "Timestamp configuration\n"
	 "Reset the timestamp precision to the default value of 0\n")
{
  zlog_default->timestamp_precision = 0 ;	
  return CMD_SUCCESS;
}


DEFUN (imish_show_logging,
       imish_show_logging_cmd,
       "show logging",
       SHOW_STR
       "Show current logging configuration\n")
{
/*	
  unsigned int i;
  int ret = CMD_SUCCESS; 
  for (i = 0; i < array_size(imish_client); i++)
    if ( imish_client[i].fd >= 0 )
      {
      	imish_module_config_format("Logging configuration for %s:\n", imish_client[i].name);
      	ret |= imish_module_client_execute_one (&imish_client[i], "show logging\n", vty, 0);
        imish_module_config_format("\n");
      }
	imish_module_config_show(vty);
  return ret;	
*/
  struct zlog *zl = zlog_default;

  vty_out (vty, "Syslog logging: ");
  if (zl->maxlvl[ZLOG_DEST_SYSLOG] == ZLOG_DISABLED)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s, facility %s, ident %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_SYSLOG]],
	     imish_facility_name(zl->facility), zl->ident);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "Stdout logging: ");
  if (zl->maxlvl[ZLOG_DEST_STDOUT] == ZLOG_DISABLED)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_STDOUT]]);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "Monitor logging: ");
  if (zl->maxlvl[ZLOG_DEST_MONITOR] == ZLOG_DISABLED)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_MONITOR]]);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "File logging: ");
  if ((zl->maxlvl[ZLOG_DEST_FILE] == ZLOG_DISABLED) ||
      !zl->fp)
    vty_out (vty, "disabled");
  else
    vty_out (vty, "level %s, filename %s",
	     zlog_priority[zl->maxlvl[ZLOG_DEST_FILE]],
	     zl->filename);
  vty_out (vty, "%s", VTY_NEWLINE);

  vty_out (vty, "Protocol name: %s%s",
  	   zlog_proto_names[zl->protocol], VTY_NEWLINE);
  vty_out (vty, "Record priority: %s%s",
  	   (zl->record_priority ? "enabled" : "disabled"), VTY_NEWLINE);
  vty_out (vty, "Timestamp precision: %d%s",
	   zl->timestamp_precision, VTY_NEWLINE);

  return CMD_SUCCESS;  
}

/*
DEFUN (banner_motd_file,
       banner_motd_file_cmd,
       "banner motd file [FILE]",
       "Set banner\n"
       "Banner for motd\n"
       "Banner from a file\n"
       "Filename\n")
{
  if (host.motdfile)
    XFREE (MTYPE_HOST, host.motdfile);
  host.motdfile = XSTRDUP (MTYPE_HOST, argv[0]);

  return CMD_SUCCESS;
}
static const char *default_motd =
"\r\n\
Hello, this is " QUAGGA_PROGNAME " (version " QUAGGA_VERSION ").\r\n\
" QUAGGA_COPYRIGHT "\r\n\
" GIT_INFO "\r\n";

DEFUN (banner_motd_default,
       banner_motd_default_cmd,
       "banner motd default",
       "Set banner string\n"
       "Strings for motd\n"
       "Default string\n")
{
  host.motd = default_motd;
  return CMD_SUCCESS;
}

DEFUN (no_banner_motd,
       no_banner_motd_cmd,
       "no banner motd",
       NO_STR
       "Set banner string\n"
       "Strings for motd\n")
{
  host.motd = NULL;
  if (host.motdfile) 
    XFREE (MTYPE_HOST, host.motdfile);
  host.motdfile = NULL;
  return CMD_SUCCESS;
}
*/
DEFUN (
	 imish_config_password,
	 imish_password_cmd,
	 "password (8|) WORD",
	 "Assign the terminal connection password\n"
	 "Specifies a HIDDEN password will follow\n"
	 "dummy string \n"
	 "The HIDDEN line password string\n")
{
	char line[128];
  /* Argument check. */
  if (argc == 0)
    {
      vty_out (vty, "Please specify password.%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (argc == 2)
    {
      if (*argv[0] == '8')
	{
		sprintf(line,"password 8 %s",argv[1]);
		if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!=CMD_SUCCESS)
			return CMD_WARNING;
		if(host.password)
	    XFREE (MTYPE_HOST, host.password);
	  host.password = NULL;
	  if (host.password_encrypt)
	    XFREE (MTYPE_HOST, host.password_encrypt);
	  host.password_encrypt = XSTRDUP (MTYPE_HOST, argv[1]);
	  return CMD_SUCCESS;
	}
      else
	{
	  vty_out (vty, "Unknown encryption type.%s", VTY_NEWLINE);
	  return CMD_WARNING;
	}
    }

  if (!isalnum ((int) *argv[0]))
    {
      vty_out (vty, 
	       "Please specify string starting with alphanumeric%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
	sprintf(line,"password %s",argv[0]);
	if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!=CMD_SUCCESS)
		return CMD_WARNING;
			
  if (host.password)
    XFREE (MTYPE_HOST, host.password);
  host.password = NULL;

  if (host.encrypt)
    {
      if (host.password_encrypt)
	XFREE (MTYPE_HOST, host.password_encrypt);
 //     host.password_encrypt = XSTRDUP (MTYPE_HOST, zencrypt (argv[0]));
    }
  else
    host.password = XSTRDUP (MTYPE_HOST, argv[0]);

  return CMD_SUCCESS;	
}

ALIAS (imish_config_password, 
			imish_password_text_cmd,
       "password LINE",
       "Assign the terminal connection password\n"
       "The UNENCRYPTED (cleartext) line password\n")
       

DEFUN (
	 imish_config_enable_password,
	 imish_enable_password_cmd,
	 "enable password (8|) WORD",
	 "Modify enable password parameters\n"
	 "Assign the privileged level password\n"
	 "Specifies a HIDDEN password will follow\n"
	 "dummy string \n"
	 "The HIDDEN 'enable' password string\n")
{
	char line[128];	
  /* Argument check. */
  if (argc == 0)
    {
      vty_out (vty, "Please specify password.%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  /* Crypt type is specified. */
  if (argc == 2)
    {
      if (*argv[0] == '8')
	{
		sprintf(line,"enable password 8 %s",argv[1]);
		if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!=CMD_SUCCESS)
			return CMD_WARNING;		
	  if (host.enable)
	    XFREE (MTYPE_HOST, host.enable);
	  host.enable = NULL;

	  if (host.enable_encrypt)
	    XFREE (MTYPE_HOST, host.enable_encrypt);
	  host.enable_encrypt = XSTRDUP (MTYPE_HOST, argv[1]);

	  return CMD_SUCCESS;
	}
      else
	{
	  vty_out (vty, "Unknown encryption type.%s", VTY_NEWLINE);
	  return CMD_WARNING;
	}
    }

  if (!isalnum ((int) *argv[0]))
    {
      vty_out (vty, 
	       "Please specify string starting with alphanumeric%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
	sprintf(line,"enable password %s",argv[0]);
	if(imish_module_client_execute(IMISH_ALL, line, vty, 1)!=CMD_SUCCESS)
		return CMD_WARNING;
		
  if (host.enable)
    XFREE (MTYPE_HOST, host.enable);
  host.enable = NULL;

  /* Plain password input. */
  if (host.encrypt)
    {
      if (host.enable_encrypt)
	XFREE (MTYPE_HOST, host.enable_encrypt);
      //host.enable_encrypt = XSTRDUP (MTYPE_HOST, zencrypt (argv[0]));
    }
  else
    host.enable = XSTRDUP (MTYPE_HOST, argv[0]);

  return CMD_SUCCESS;
}
ALIAS (imish_config_enable_password,
       imish_enable_password_text_cmd,
       "enable password LINE",
       "Modify enable password parameters\n"
       "Assign the privileged level password\n"
       "The UNENCRYPTED (cleartext) 'enable' password\n")

DEFUN (
	 no_imish_config_enable_password,
	 no_imish_enable_password_cmd,
	 "no enable password",
	 NO_STR
	 "Modify enable password parameters\n"
	 "Assign the privileged level password\n")
{
	if(imish_module_client_execute(IMISH_ALL, "no enable password", vty, 1)!=CMD_SUCCESS)
		return CMD_WARNING;	
  if (host.enable)
    XFREE (MTYPE_HOST, host.enable);
  host.enable = NULL;

  if (host.enable_encrypt)
    XFREE (MTYPE_HOST, host.enable_encrypt);
  host.enable_encrypt = NULL;

  return CMD_SUCCESS;
}

DEFUN (imish_write_terminal,
       imish_write_terminal_cmd,
       "write terminal",
       "Write running configuration to memory, network, or terminal\n"
       "Write to terminal\n")
{
  char line[] = "write terminal\n";
  vty_out (vty, "Building configuration...%s", VTY_NEWLINE);
  vty_out (vty, "%sCurrent configuration:%s", VTY_NEWLINE,VTY_NEWLINE);
  vty_out (vty, "!%s", VTY_NEWLINE);
	imish_module_client_execute(IMISH_ALL, line, vty, 0); 
	imish_module_config_show (vty);
  vty_out (vty, "end%s", VTY_NEWLINE); 
  return CMD_SUCCESS; 
}


DEFUN (imish_write_memory,
       imish_write_memory_cmd,
       "write memory",
       "Write running configuration to memory, network, or terminal\n"
       "Write configuration to the file (same as write file)\n")
{
  char line[] = "write memory\n";
  vty_out (vty, "Building configuration...%s", VTY_NEWLINE);   
  imish_module_client_execute(IMISH_ALL, line, vty, 0);
  imish_module_config_show(vty);
  vty_out (vty,"[OK]%s",VTY_NEWLINE);  
  return CMD_SUCCESS; 
}
DEFUN (imish_write_memory_imish,
       imish_write_memory_imish_cmd,
       "write memory integrated",
       "Write running configuration to memory, network, or terminal\n"
       "Write configuration to the file (same as write file)\n"
	   "Write configuration to imish configure file\n")
{
  int fd;
  char *config_file = NULL;
  char *config_file_sav = NULL;
  struct vty *file_vty;
  char line[] = "write terminal\n";
  
  if (host.config == NULL)
    {
      vty_out (vty, "Can't save to configuration file %s",VTY_NEWLINE);
      return CMD_WARNING;
    }
  vty_out (vty, "Building configuration...%s", VTY_NEWLINE);   
  /* Get filename. */
  config_file = host.config;

  config_file_sav = malloc (strlen (config_file) +
			  strlen (CONF_BACKUP_EXT) + 1);
  strcpy (config_file_sav, config_file);
  strcat (config_file_sav, CONF_BACKUP_EXT);

  /* Move current configuration file to backup config file. */
  unlink (config_file_sav);
  rename (config_file, config_file_sav);
  free (config_file_sav);
  
  /* Open file to configuration write. */
  //fd = mkstemp (config_file);
  fd = open(config_file, O_WRONLY|O_CREAT, CONFIGFILE_MASK);
  if (fd < 0)
    {
      vty_out (vty, "Can't open configuration file %s%s", config_file, VTY_NEWLINE);
      return CMD_WARNING;
    }
  
  /* Make vty for configuration file. */
  file_vty = vty_new ();
  file_vty->fd = fd;
  file_vty->type = VTY_FILE;

  /* Config file header print. */
  vty_out (file_vty, "!\n! Zebra configuration \n!   ");
  vty_time_print (file_vty, 1);
  vty_out (file_vty, "!\n");
  
	imish_module_client_execute(IMISH_ALL, line, file_vty, 0);
	imish_module_config_show(file_vty);
  vty_out (file_vty, "end%s", VTY_NEWLINE);
	vty_close (file_vty);
  sync ();
  if (chmod (config_file, CONFIGFILE_MASK) != 0)
    {
      vty_out (vty,"%% Can't chmod configuration file %s: %s (%d)%s", config_file, safe_strerror(errno), errno,VTY_NEWLINE);
      return CMD_WARNING;
    }
    
  vty_out (vty, "Configuration saved to %s%s", config_file,VTY_NEWLINE);
  vty_out (vty,"[OK]%s",VTY_NEWLINE);  
  return CMD_SUCCESS; 
}

ALIAS (imish_write_memory,
       imish_copy_runningconfig_startupconfig_cmd,
       "copy running-config startup-config",  
       "Copy from one file to another\n"
       "Copy from current system configuration\n"
       "Copy to startup configuration\n")

ALIAS (imish_write_memory,
       imish_write_file_cmd,
       "write file",
       "Write running configuration to memory, network, or terminal\n"
       "Write configuration to the file (same as write memory)\n")

ALIAS (imish_write_memory,
       imish_write_cmd,
       "write",
       "Write running configuration to memory, network, or terminal\n")

ALIAS (imish_write_terminal,
       imish_show_running_config_cmd,
       "show running-config",
       SHOW_STR
       "Current operating configuration\n")

DEFUN (imish_terminal_length,
       imish_terminal_length_cmd,
       "terminal length <0-512>",
       "Set terminal line parameters\n"
       "Set number of lines on a screen\n"
       "Number of lines on screen (0 for no pausing)\n")
{
  int lines;
  char *endptr = NULL;

  lines = strtol (argv[0], &endptr, 10);
  if (lines < 0 || lines > 512 || *endptr != '\0')
    {
      vty_out (vty, "length is malformed%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  vty->lines = lines;
  return CMD_SUCCESS;
}

DEFUN (imish_terminal_no_length,
       imish_terminal_no_length_cmd,
       "terminal no length",
       "Set terminal line parameters\n"
       NO_STR
       "Set number of lines on a screen\n")
{
  vty->lines = -1;
  return CMD_SUCCESS;
}
DEFUN (service_terminal_length, service_terminal_length_cmd,
       "service terminal-length <0-512>",
       "Set up miscellaneous service\n"
       "System wide terminal length configuration\n"
       "Number of lines of VTY (0 means no line control)\n")
{
  int lines;
  char *endptr = NULL;

  lines = strtol (argv[0], &endptr, 10);
  if (lines < 0 || lines > 512 || *endptr != '\0')
    {
      vty_out (vty, "length is malformed%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  host.lines = lines;

  return CMD_SUCCESS;
}

DEFUN (no_service_terminal_length, no_service_terminal_length_cmd,
       "no service terminal-length [<0-512>]",
       NO_STR
       "Set up miscellaneous service\n"
       "System wide terminal length configuration\n"
       "Number of lines of VTY (0 means no line control)\n")
{
  host.lines = -1;
  return CMD_SUCCESS;
}
DEFUN (imish_show_daemons,
       imish_show_daemons_cmd,
       "show daemons",
       SHOW_STR
       "Show list of running daemons\n")
{
  u_int i;

  for (i = 0; i < array_size(imish_client); i++)
    if ( imish_client[i].fd >= 0 )
      vty_out(vty, " %s", imish_client[i].name);
  vty_out(vty, "%s", VTY_NEWLINE);

  return CMD_SUCCESS;
}

/* Move to vty configuration mode. */
DEFUNSH (IMISH_ALL,line_vty,
       line_vty_cmd,
       "line vty",
       "Configure a terminal line\n"
       "Virtual terminal\n")
{
  vty->node = VTY_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,terminal_monitor,
       terminal_monitor_cmd,
       "terminal monitor",
       "Set terminal line parameters\n"
       "Copy debug output to the current terminal line\n")
{
  vty->monitor = 1;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ALL,terminal_no_monitor,
       no_terminal_monitor_cmd,
       "no terminal monitor",
       NO_STR
       "Set terminal line parameters\n"
       "Copy debug output to the current terminal line\n")
{
  vty->monitor = 0;
  return CMD_SUCCESS;
}
/* Hostname configuration */
DEFUN (
       imish_config_hostname, 
       imish_hostname_cmd,
       "hostname WORD",
       "Set system's network name\n"
       "This system's network name\n")
{
  int ret=0;
  char line[100];

  if((argc != 1) && (argv[0]==NULL ))
  {
    vty_out (vty, "Please Input hostname String %s", VTY_NEWLINE);
    return CMD_WARNING;
  }
  if (!isalpha((int) *argv[0]))
    {
      vty_out (vty, "Please specify string starting with alphabet%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  sprintf(line, "hostname %s\n", (argc == 1) ? argv[0] : "");
  ret = imish_module_client_execute(IMISH_ALL, line, vty, 0); 
  //ret = vtysh_client_execute_cmd(VTYSH_ALL, line, stdout);
  if(ret != CMD_SUCCESS)
  {
    vty_out (vty, "Please Input hostname String:%s %s", argv[0],VTY_NEWLINE);
    return CMD_WARNING;
  }
  if (host.name)
    XFREE (MTYPE_HOST, host.name);
    
  host.name = XSTRDUP (MTYPE_HOST, argv[0]);
  return CMD_SUCCESS;

}

//DEFUNSH (VTYSH_ALL,这个定义的命令是不携带参数进来的，argc 和argv都是0(NULL)
//原因是 	  vtysh_execute_func 函数执行 (*cmd->func) (cmd, vty, argc, argv);
DEFUN (
       imish_config_no_hostname, 
       imish_no_hostname_cmd,
       "no hostname [HOSTNAME]",
       NO_STR
       "Reset system's network name\n"
       "Host name of this router\n")
{
  int ret= 0;
  char line[100];
  sprintf(line, "no hostname %s\n", (argc == 1) ? argv[0] : "");
  ret = imish_module_client_execute(IMISH_ALL, line, vty, 0);
  //ret = vtysh_client_execute_cmd(VTYSH_ALL, line, stdout);
  if(ret != CMD_SUCCESS)
  {
    vty_out (vty, "Please Input hostname String:%s %s", argv[0],VTY_NEWLINE);
    return CMD_WARNING;
  }
  if (host.name)
    XFREE (MTYPE_HOST, host.name);
  host.name = NULL;
  return CMD_SUCCESS;
}
/* Write startup configuration into the terminal. */
DEFUN (show_startup_config,
       show_startup_config_cmd,
       "show startup-config",
       SHOW_STR
       "Contentes of startup configuration\n")
{
  char buf[BUFSIZ];
  FILE *confp;

  confp = fopen (host.config, "r");
  if (confp == NULL)
    {
      vty_out (vty, "Can't open configuration file [%s]%s",
	       host.config, VTY_NEWLINE);
      return CMD_WARNING;
    }

  while (fgets (buf, BUFSIZ, confp))
    {
      char *cp = buf;

      while (*cp != '\r' && *cp != '\n' && *cp != '\0')
	cp++;
      *cp = '\0';

      vty_out (vty, "%s%s", buf, VTY_NEWLINE);
    }

  fclose (confp);

  return CMD_SUCCESS;
}


static void
imish_install_default (enum node_type node)
{
  install_element (node, &config_list_cmd);
}

void imish_module_init (void)
{
	char oem[] = SYSCONFDIR IMISH_OEM_DEFAULT;
	extern void imi_shell_init (struct thread_master *master_thread);
	extern int (*imi_sh_execute)(struct vty *vty, char *buf);
	extern struct cmd_node vty_node;
  /* Initialize commands. */
  memset(&host, 0, sizeof(struct host));
  cmd_init (0);
	
	imi_sh_execute = imish_sh_execute;
	host.motd = NULL;
  if (host.motdfile)
    XFREE (MTYPE_HOST, host.motdfile);
  host.motdfile = XSTRDUP (MTYPE_HOST, oem);
  
  imish_connect = thread_add_timer (master, imish_module_connect_thread, NULL, 5);
  
  /* Install nodes. */
  install_node (&bgp_node, NULL);
  install_node (&rip_node, NULL);
  install_node (&interface_node, NULL);
  install_node (&rmap_node, NULL);
  install_node (&zebra_node, NULL);
  install_node (&bgp_vpnv4_node, NULL);
  //install_node (&bgp_vpnv6_node, NULL);
  //install_node (&bgp_encap_node, NULL);
  //install_node (&bgp_encapv6_node, NULL);
  install_node (&bgp_ipv4_node, NULL);
  install_node (&bgp_ipv4m_node, NULL);
/* #ifdef HAVE_IPV6 */
  install_node (&bgp_ipv6_node, NULL);
  install_node (&bgp_ipv6m_node, NULL);
/* #endif */
  install_node (&ospf_node, NULL);
/* #ifdef HAVE_IPV6 */
  install_node (&ripng_node, NULL);
  install_node (&ospf6_node, NULL);
/* #endif */
  install_node (&babel_node, NULL);
  install_node (&keychain_node, NULL);
  install_node (&keychain_key_node, NULL);
  install_node (&isis_node, NULL);
  install_node (&vty_node, NULL);
  
  imi_shell_init (master);
/* 2016年7月2日 22:08:37 zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
  install_node (&olsr_node, NULL);
#endif
/* 2016年7月2日 22:08:37  zhurish: 扩展路由协议增加命令 */
  imish_install_default (VIEW_NODE);
  imish_install_default (ENABLE_NODE);
  imish_install_default (CONFIG_NODE);
  imish_install_default (BGP_NODE);
  imish_install_default (RIP_NODE);
  imish_install_default (INTERFACE_NODE);
  imish_install_default (RMAP_NODE);
  imish_install_default (ZEBRA_NODE);
  imish_install_default (BGP_VPNV4_NODE);
  //imish_install_default (BGP_VPNV6_NODE);
  //imish_install_default (BGP_ENCAP_NODE);
  //imish_install_default (BGP_ENCAPV6_NODE);
  imish_install_default (BGP_IPV4_NODE);
  imish_install_default (BGP_IPV4M_NODE);
  imish_install_default (BGP_IPV6_NODE);
  imish_install_default (BGP_IPV6M_NODE);
  imish_install_default (OSPF_NODE);
  imish_install_default (RIPNG_NODE);
  imish_install_default (OSPF6_NODE);
  imish_install_default (BABEL_NODE);
  imish_install_default (ISIS_NODE);
  imish_install_default (KEYCHAIN_NODE);
  imish_install_default (KEYCHAIN_KEY_NODE);
  imish_install_default (VTY_NODE);
/* 2016年7月2日 22:08:45 zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
  imish_install_default (OLSR_NODE);
#endif
/* 2016年7月2日 22:08:45  zhurish: 扩展路由协议增加命令 */

  /* "exit" command. */
  install_element (VIEW_NODE, &imish_exit_imish_cmd);
  install_element (VIEW_NODE, &imish_quit_imish_cmd);
  install_element (ENABLE_NODE, &imish_exit_imish_cmd);
  install_element (ENABLE_NODE, &imish_quit_imish_cmd);
  //install_element (VIEW_NODE, &imish_exit_all_cmd);
  //install_element (VIEW_NODE, &imish_quit_all_cmd);
  //install_element (ENABLE_NODE, &imish_exit_all_cmd);
  //install_element (ENABLE_NODE, &imish_quit_all_cmd);
  
  install_element (CONFIG_NODE, &imish_exit_all_cmd);
  install_element (RIP_NODE, &imish_exit_ripd_cmd);
  install_element (RIP_NODE, &imish_quit_ripd_cmd);
  install_element (RIPNG_NODE, &imish_exit_ripngd_cmd);
  install_element (RIPNG_NODE, &imish_quit_ripngd_cmd);
  install_element (OSPF_NODE, &imish_exit_ospfd_cmd);
  install_element (OSPF_NODE, &imish_quit_ospfd_cmd);
  install_element (OSPF6_NODE, &imish_exit_ospf6d_cmd);
  install_element (OSPF6_NODE, &imish_quit_ospf6d_cmd);
  install_element (BGP_NODE, &imish_exit_bgpd_cmd);
  install_element (BGP_NODE, &imish_quit_bgpd_cmd);
  install_element (BGP_VPNV4_NODE, &imish_exit_bgpd_cmd);
  install_element (BGP_VPNV4_NODE, &imish_quit_bgpd_cmd);
#ifdef HAVE_IPV6
  //install_element (BGP_VPNV6_NODE, &imish_exit_bgpd_cmd);
  //install_element (BGP_VPNV6_NODE, &imish_quit_bgpd_cmd);
#endif
  //install_element (BGP_ENCAP_NODE, &imish_exit_bgpd_cmd);
  //install_element (BGP_ENCAP_NODE, &imish_quit_bgpd_cmd);
#ifdef HAVE_IPV6
 // install_element (BGP_ENCAPV6_NODE, &imish_exit_bgpd_cmd);
 // install_element (BGP_ENCAPV6_NODE, &imish_quit_bgpd_cmd);
#endif
  install_element (BGP_IPV4_NODE, &imish_exit_bgpd_cmd);
  install_element (BGP_IPV4_NODE, &imish_quit_bgpd_cmd);
  install_element (BGP_IPV4M_NODE, &imish_exit_bgpd_cmd);
  install_element (BGP_IPV4M_NODE, &imish_quit_bgpd_cmd);
#ifdef HAVE_IPV6
  install_element (BGP_IPV6_NODE, &imish_exit_bgpd_cmd);
  install_element (BGP_IPV6_NODE, &imish_quit_bgpd_cmd);
  install_element (BGP_IPV6M_NODE, &imish_exit_bgpd_cmd);
  install_element (BGP_IPV6M_NODE, &imish_quit_bgpd_cmd);
#endif  
  install_element (ISIS_NODE, &imish_exit_isisd_cmd);
  install_element (ISIS_NODE, &imish_quit_isisd_cmd);
  install_element (KEYCHAIN_NODE, &imish_exit_ripd_cmd);
  install_element (KEYCHAIN_NODE, &imish_quit_ripd_cmd);
  install_element (KEYCHAIN_KEY_NODE, &imish_exit_ripd_cmd);
  install_element (KEYCHAIN_KEY_NODE, &imish_quit_ripd_cmd);
  install_element (RMAP_NODE, &imish_exit_rmap_cmd);
  install_element (RMAP_NODE, &imish_quit_rmap_cmd);
  install_element (VTY_NODE, &imish_exit_line_vty_cmd);
  install_element (VTY_NODE, &imish_quit_line_vty_cmd);

  /* "end" command. */
  install_element (CONFIG_NODE, &imish_end_all_cmd);
  install_element (ENABLE_NODE, &imish_end_all_cmd);
  install_element (RIP_NODE, &imish_end_all_cmd);
  install_element (RIPNG_NODE, &imish_end_all_cmd);
  install_element (OSPF_NODE, &imish_end_all_cmd);
  install_element (OSPF6_NODE, &imish_end_all_cmd);
  install_element (BABEL_NODE, &imish_end_all_cmd);
  install_element (BGP_NODE, &imish_end_all_cmd);
  install_element (BGP_IPV4_NODE, &imish_end_all_cmd);
  install_element (BGP_IPV4M_NODE, &imish_end_all_cmd);
  install_element (BGP_VPNV4_NODE, &imish_end_all_cmd);
  //install_element (BGP_VPNV6_NODE, &imish_end_all_cmd);
  //install_element (BGP_ENCAP_NODE, &imish_end_all_cmd);
  //install_element (BGP_ENCAPV6_NODE, &imish_end_all_cmd);
  install_element (BGP_IPV6_NODE, &imish_end_all_cmd);
  install_element (BGP_IPV6M_NODE, &imish_end_all_cmd);
  install_element (ISIS_NODE, &imish_end_all_cmd);
  install_element (KEYCHAIN_NODE, &imish_end_all_cmd);
  install_element (KEYCHAIN_KEY_NODE, &imish_end_all_cmd);
  install_element (RMAP_NODE, &imish_end_all_cmd);
  install_element (VTY_NODE, &imish_end_all_cmd);

  install_element (INTERFACE_NODE, &interface_desc_cmd);
  install_element (INTERFACE_NODE, &no_interface_desc_cmd);
  install_element (INTERFACE_NODE, &imish_end_all_cmd);
  install_element (INTERFACE_NODE, &imish_exit_interface_cmd);
  install_element (INTERFACE_NODE, &imish_quit_interface_cmd);
  install_element (CONFIG_NODE, &router_rip_cmd);
#ifdef HAVE_IPV6
  install_element (CONFIG_NODE, &router_ripng_cmd);
#endif
  install_element (CONFIG_NODE, &router_ospf_cmd);
#ifdef HAVE_IPV6
  install_element (CONFIG_NODE, &router_ospf6_cmd);
#endif
/* 2016年7月2日 22:08:58 zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
  install_element (CONFIG_NODE, &router_olsr_cmd);
  install_element (OLSR_NODE, &imish_exit_olsr_cmd);
  install_element (OLSR_NODE, &imish_quit_olsr_cmd);
  install_element (OLSR_NODE, &imish_end_all_cmd);  
#endif
/* 2016年7月2日 22:08:58  zhurish: 扩展路由协议增加命令 */
  install_element (CONFIG_NODE, &router_isis_cmd);
  install_element (CONFIG_NODE, &router_bgp_cmd);
  install_element (CONFIG_NODE, &router_bgp_view_cmd);
  install_element (BGP_NODE, &address_family_vpnv4_cmd);
  install_element (BGP_NODE, &address_family_vpnv4_unicast_cmd);
  //install_element (BGP_NODE, &address_family_vpnv6_cmd);
  //install_element (BGP_NODE, &address_family_vpnv6_unicast_cmd);
  //install_element (BGP_NODE, &address_family_encap_cmd);
  //install_element (BGP_NODE, &address_family_encapv6_cmd);
  install_element (BGP_NODE, &address_family_ipv4_unicast_cmd);
  install_element (BGP_NODE, &address_family_ipv4_multicast_cmd);
#ifdef HAVE_IPV6
  install_element (BGP_NODE, &address_family_ipv6_cmd);
  install_element (BGP_NODE, &address_family_ipv6_unicast_cmd);
#endif
  install_element (BGP_VPNV4_NODE, &exit_address_family_cmd);
  //install_element (BGP_VPNV6_NODE, &exit_address_family_cmd);
  //install_element (BGP_ENCAP_NODE, &exit_address_family_cmd);
  //install_element (BGP_ENCAPV6_NODE, &exit_address_family_cmd);
  install_element (BGP_IPV4_NODE, &exit_address_family_cmd);
  install_element (BGP_IPV4M_NODE, &exit_address_family_cmd);
  install_element (BGP_IPV6_NODE, &exit_address_family_cmd);
  install_element (BGP_IPV6M_NODE, &exit_address_family_cmd);
  install_element (CONFIG_NODE, &key_chain_cmd);
  install_element (CONFIG_NODE, &route_map_cmd);
  install_element (CONFIG_NODE, &line_vty_cmd);
  install_element (KEYCHAIN_NODE, &key_cmd);
  install_element (KEYCHAIN_NODE, &key_chain_cmd);
  install_element (KEYCHAIN_KEY_NODE, &key_chain_cmd);
  install_element (CONFIG_NODE, &imish_interface_cmd);
  install_element (CONFIG_NODE, &imish_no_interface_cmd);
  //install_element (CONFIG_NODE, &imish_interface_vrf_cmd);
  //install_element (CONFIG_NODE, &imish_no_interface_vrf_cmd);
  
  
  install_element (VIEW_NODE, &imish_enable_cmd);
  install_element (ENABLE_NODE, &imish_config_terminal_cmd);
  install_element (ENABLE_NODE, &imish_disable_cmd);
  
  install_element (CONFIG_NODE, &imish_hostname_cmd);
  install_element (CONFIG_NODE, &imish_no_hostname_cmd);
  /* password command */
  //install_element (CONFIG_NODE, &imish_service_password_encrypt_cmd);
  //install_element (CONFIG_NODE, &no_imish_service_password_encrypt_cmd);

  install_element (CONFIG_NODE, &imish_password_cmd);
  install_element (CONFIG_NODE, &imish_password_text_cmd);
  install_element (CONFIG_NODE, &imish_enable_password_cmd);
  install_element (CONFIG_NODE, &imish_enable_password_text_cmd);
  install_element (CONFIG_NODE, &no_imish_enable_password_cmd);
  /*
  install_element (CONFIG_NODE, &banner_motd_default_cmd);
  install_element (CONFIG_NODE, &banner_motd_file_cmd);
  install_element (CONFIG_NODE, &no_banner_motd_cmd);
  */    
  install_element (ENABLE_NODE, &terminal_monitor_cmd);
  install_element (ENABLE_NODE, &no_terminal_monitor_cmd);
  
  install_element (ENABLE_NODE, &service_terminal_length_cmd);
  install_element (ENABLE_NODE, &no_service_terminal_length_cmd);
  install_element (VIEW_NODE, &imish_terminal_length_cmd);
  install_element (ENABLE_NODE, &imish_terminal_length_cmd);
  install_element (VIEW_NODE, &imish_terminal_no_length_cmd);
  install_element (ENABLE_NODE, &imish_terminal_no_length_cmd);

  /* "write terminal" command. */
  install_element (ENABLE_NODE, &imish_write_terminal_cmd);
  install_element (ENABLE_NODE, &imish_show_running_config_cmd);  
  install_element (ENABLE_NODE, &show_startup_config_cmd);
  
  /* "write memory" command. */
  install_element (ENABLE_NODE, &imish_write_memory_cmd);
  install_element (ENABLE_NODE, &imish_copy_runningconfig_startupconfig_cmd);
  install_element (ENABLE_NODE, &imish_write_file_cmd);
  install_element (ENABLE_NODE, &imish_write_cmd);
  install_element (ENABLE_NODE, &imish_write_memory_imish_cmd);  
  
  /* "show xxxx" command*/
  install_element (VIEW_NODE, &imish_show_daemons_cmd);
  install_element (ENABLE_NODE, &imish_show_daemons_cmd);

  install_element (VIEW_NODE, &imish_show_memory_cmd);
  install_element (ENABLE_NODE, &imish_show_memory_cmd);

  install_element (VIEW_NODE, &imish_show_work_queues_cmd);
  install_element (ENABLE_NODE, &imish_show_work_queues_cmd);

  install_element (VIEW_NODE, &imish_show_thread_cmd);
  install_element (ENABLE_NODE, &imish_show_thread_cmd);

  /* Logging */
  install_element (ENABLE_NODE, &imish_show_logging_cmd);
  install_element (VIEW_NODE, &imish_show_logging_cmd);
  install_element (CONFIG_NODE, &imish_log_stdout_cmd);
  install_element (CONFIG_NODE, &imish_log_stdout_level_cmd);
  install_element (CONFIG_NODE, &no_imish_log_stdout_cmd);
  install_element (CONFIG_NODE, &imish_log_file_cmd);
  install_element (CONFIG_NODE, &imish_log_file_level_cmd);
  install_element (CONFIG_NODE, &no_imish_log_file_cmd);
  install_element (CONFIG_NODE, &no_imish_log_file_level_cmd);
  install_element (CONFIG_NODE, &imish_log_monitor_cmd);
  install_element (CONFIG_NODE, &imish_log_monitor_level_cmd);
  install_element (CONFIG_NODE, &no_imish_log_monitor_cmd);
  install_element (CONFIG_NODE, &imish_log_syslog_cmd);
  install_element (CONFIG_NODE, &imish_log_syslog_level_cmd);
  install_element (CONFIG_NODE, &no_imish_log_syslog_cmd);
  install_element (CONFIG_NODE, &imish_log_trap_cmd);
  install_element (CONFIG_NODE, &no_imish_log_trap_cmd);
  install_element (CONFIG_NODE, &imish_log_facility_cmd);
  install_element (CONFIG_NODE, &no_imish_log_facility_cmd);
  install_element (CONFIG_NODE, &imish_log_record_priority_cmd);
  install_element (CONFIG_NODE, &no_imish_log_record_priority_cmd);
  install_element (CONFIG_NODE, &imish_log_timestamp_precision_cmd);
  install_element (CONFIG_NODE, &no_imish_log_timestamp_precision_cmd);
}
void imish_module_exit (void)
{
  if(imish_connect)
    thread_cancel(imish_connect);	
  imish_module_close_all();
	vty_terminate ();
	cmd_terminate ();
	imish_connect = NULL;
}
#endif /* IMISH_IMI_MODULE */ 
