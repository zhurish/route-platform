/*
 * ip_tunnel.c
 *
 *  Created on: Oct 14, 2016
 *      Author: zhurish
 */



#include <zebra.h>

#include "if.h"
#include "vty.h"
#include "sockunion.h"
#include "prefix.h"
#include "command.h"
#include "memory.h"
#include "log.h"
#include "zclient.h"
#include "thread.h"
#include "privs.h"
#include "sigevent.h"


#include "system-utils/utils.h"

#ifdef HAVE_UTILS_TUNNEL



/**************************************************************/
/**************************************************************/

/* mode */
#define TUNNEL_GRE	1
#define TUNNEL_IPIP	2
#define TUNNEL_VTI	3
#define TUNNEL_SIT	4
/* active */
#define TUNNEL_SETUP	1
#define TUNNEL_ACTIVE	2

/**************************************************************/
/**************************************************************/
struct ip_tunnel_table
{
	int ifindex;
	int index;
	int mode;
	int ttl;//change: ip tunnel change tunnel0 ttl
	int mtu;//change: ip link set dev tunnel0 mtu 1400
    struct in_addr local;//ip tunnel change tunnel1 local 192.168.122.1
    struct in_addr remote;//change: ip tunnel change tunnel1 remote 19.1.1.1
    int active;
};

static struct ip_tunnel_table ip_tun_table[TUNNEL_TABLE_MAX];
/**************************************************************/
static const char * ip_tun_mode_str[] =
{
		"mode unknow",
		"mode gre",
		"mode ipip",
		"mode vti",
		"mode sit",
		"mode unknow",
};
/**************************************************************/
/* call ioctl system call */
static int if_ioctl (u_long request, caddr_t buffer)
{
  int sock;
  int ret;
  int err;

  //if (vpn_privs.change(ZPRIVS_RAISE))
  //  zlog (NULL, LOG_ERR, "Can't raise privileges");
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      int save_errno = errno;
      //if (vpn_privs.change(ZPRIVS_LOWER))
      //  zlog (NULL, LOG_ERR, "Can't lower privileges");
      zlog_err("Cannot create UDP socket: %s", safe_strerror(save_errno));
      exit (1);
    }
  if ((ret = ioctl (sock, request, buffer)) < 0)
    err = errno;
  //if (vpn_privs.change(ZPRIVS_LOWER))
  //  zlog (NULL, LOG_ERR, "Can't lower privileges");
  close (sock);

  if (ret < 0)
    {
      errno = err;
      return ret;
    }
  return 0;
}
static int if_get_ifindex (struct interface *ifp)
{
#ifdef HAVE_IF_NAMETOINDEX
	ifp->ifindex = if_nametoindex (ifp->name);
#endif
#ifdef SIOCGIFINDEX
  struct ifreq ifreq;
  strncpy (ifreq.ifr_name, ifp->name, IFNAMSIZ);
  if (if_ioctl (SIOCGIFINDEX, (caddr_t) &ifreq) < 0)
    return CMD_WARNING;
  ifp->ifindex = ifreq.ifr_ifindex;
#endif /* SIOCGIFINDEX */
  return CMD_SUCCESS;
}
/**************************************************************/
/**************************************************************/
static int ip_tunnel_setting(struct interface *ifp, int type, struct ip_tunnel_table *table)
{
	char src[32];
	char dest[32];
	char cmd[256];
	if( table->index &&
		table->mode &&
		table->mtu &&
		table->ttl &&
		table->local.s_addr &&
		table->remote.s_addr)
	{
		if(type)
		{
			//ip tunnel add tunnel2 mode gre remote 192.168.1.130 local 192.168.1.19 ttl 250
			UTILS_DEBUG_LOG ("add tunnel%d",table->index);
			memset(src, 0, sizeof(src));
			memset(dest, 0, sizeof(dest));
			memset(cmd, 0, sizeof(cmd));
			sprintf(src, "local %s", inet_ntoa(table->local));
			sprintf(dest, "remote %s", inet_ntoa(table->remote));

			sprintf(cmd, "ip tunnel add tunnel%d %s %s %s ttl %d",table->index,
					ip_tun_mode_str[table->mode], src, dest, table->ttl);
			super_system(cmd);

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "ip link set dev tunnel%d mtu %d up",table->index,table->mtu);
			super_system(cmd);
			if(ifp)
			{
				if_get_ifindex (ifp);
				table->ifindex = ifp->ifindex;
			}
			table->active = TUNNEL_ACTIVE;
			return CMD_SUCCESS;
		}
		else
		{
			if(table->active != TUNNEL_ACTIVE)
			{
				table->active = 0;
				return CMD_SUCCESS;
			}
			UTILS_DEBUG_LOG ("del tunnel%d",table->index);
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "ip link set dev tunnel%d down",table->index);
			super_system(cmd);

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "ip tunnel del tunnel%d",table->index);
			super_system(cmd);

			table->active = 0;
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
/**************************************************************/
static int ip_tunnel_name_to_index (char *name)
{
	const char *sp;
	char buf[4];
	int i = 0;
	if (name == NULL)
		return 0;
	memset (buf, 0, sizeof (buf));
	sp = name;
	while (*sp != '\0')
	{
		if (isdigit ((int) *sp))
			buf[i++] = *sp;
		sp++;
	}
	return atoi(buf);
}
static int ip_tunnel_num_to_ifindex(int num)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].index == num)
		{
			return ip_tun_table[i].ifindex;
		}
	}
	return 0;
}
static int ip_tunnel_ifindex_to_num(int ifindex)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].ifindex == ifindex)
		{
			return ip_tun_table[i].index;
		}
	}
	return 0;
}
static int ip_tunnel_create(int index)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].active == 0)
		{
			ip_tun_table[i].index = index;
			ip_tun_table[i].mtu = TUNNEL_MTU_DEFAULT;
			ip_tun_table[i].ttl = TUNNEL_TTL_DEFAULT;
			ip_tun_table[i].active = TUNNEL_SETUP;
			ip_tun_table[i].mode = TUNNEL_GRE;
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
static int ip_tunnel_delete(int index)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].active && ip_tun_table[i].index == index)
		{
			if(ip_tun_table[i].active == TUNNEL_ACTIVE)
				ip_tunnel_setting(NULL, 0, &ip_tun_table[i]);
			ip_tun_table[i].index = index;
			ip_tun_table[i].active = 0;
			ip_tun_table[i].mode = 0;
			ip_tun_table[i].local.s_addr = 0;
			ip_tun_table[i].remote.s_addr = 0;
			ip_tun_table[i].mtu = 0;
			ip_tun_table[i].ttl = 0;
			//ip tunnel add tunnel2
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
static int ip_tunnel_mode(int index, int mode)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].active = TUNNEL_SETUP && ip_tun_table[i].index == index)
		{
			ip_tun_table[i].mode = mode;
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
static int ip_tunnel_address(int index, char *src, char *dest)
{
	int i = 0;
	char cmd[256];
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].active = TUNNEL_SETUP && ip_tun_table[i].index == index)
		{
			UTILS_DEBUG_LOG ("setting tunnel%d address",ip_tun_table[i].index);
			if(src)
			{
				if(ip_tun_table[i].active == TUNNEL_ACTIVE)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip tunnel change tunnel%d local %s", index, src);
					super_system(cmd);
				}
				ip_tun_table[i].local.s_addr = inet_addr(src);
			}
			if(dest)
			{
				if(ip_tun_table[i].active == TUNNEL_ACTIVE)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip tunnel change tunnel%d remote %s", index, dest);
					super_system(cmd);
				}
				ip_tun_table[i].remote.s_addr = inet_addr(dest);
			}
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
static int ip_tunnel_mtu_ttl(int index, int mtu, int ttl)
{
	int i = 0;
	char cmd[256];
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(ip_tun_table[i].active = TUNNEL_SETUP && ip_tun_table[i].index == index)
		{
			if(mtu)
			{
				UTILS_DEBUG_LOG ("setting tunnel%d mtu",ip_tun_table[i].index);
				if( ip_tun_table[i].active == TUNNEL_ACTIVE &&
					ip_tun_table[i].mtu != 0 &&
					ip_tun_table[i].mtu != mtu)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip link set dev tunnel%d mtu %d",index, ip_tun_table[i].mtu);
					super_system(cmd);
				}
				if(ip_tun_table[i].active == TUNNEL_ACTIVE)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip link set dev tunnel%d mtu %d", index, mtu);
					super_system(cmd);
				}
				ip_tun_table[i].mtu = mtu;
			}
			if(ttl)
			{
				UTILS_DEBUG_LOG ("setting tunnel%d ttl",ip_tun_table[i].index);
				if( ip_tun_table[i].active == TUNNEL_ACTIVE &&
					ip_tun_table[i].ttl != 0 &&
					ip_tun_table[i].ttl != ttl)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip tunnel change tunnel%d ttl %d",index, ip_tun_table[i].ttl);
					super_system(cmd);
				}
				if(ip_tun_table[i].active == TUNNEL_ACTIVE)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip tunnel change tunnel%d ttl %d", index, ttl);
					super_system(cmd);
				}
				ip_tun_table[i].ttl = ttl;
			}
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
static int ip_tunnel_update(struct interface *ifp, int index)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if( ip_tun_table[i].index == index )
		{
			UTILS_DEBUG_LOG ("update tunnel%d",ip_tun_table[i].index);
			if(ip_tun_table[i].active == TUNNEL_ACTIVE)
				ip_tunnel_setting(NULL, 0, &ip_tun_table[i]);
			if(ip_tun_table[i].active == TUNNEL_SETUP)
				ip_tunnel_setting(ifp, 1, &ip_tun_table[i]);
			//ip_tunnel_setting(ifp, 1, &ip_tun_table[i]);
			//return ip_tun_table[i].active;
		}
	}
	return 0;
}
static int ip_tunnel_is_active(int index)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if( ip_tun_table[i].index == index )
		{
			return ip_tun_table[i].active;
		}
	}
	return 0;
}
static int ip_tunnel_is_create(int index)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if( ip_tun_table[i].index == index )
		{
			return 1;
		}
	}
	return 0;
}
static int ip_tunnel_active_setting(struct interface *ifp, int index)
{
	int i = 0;
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if( ip_tun_table[i].index == index &&
			ip_tun_table[i].mode &&
			ip_tun_table[i].mtu &&
			ip_tun_table[i].ttl &&
			ip_tun_table[i].local.s_addr &&
			ip_tun_table[i].remote.s_addr)
		{
			UTILS_DEBUG_LOG ("active setting tunnel%d mtu",ip_tun_table[i].index);
			//if(ip_tun_table[i].active != TUNNEL_ACTIVE)
			if(ip_tun_table[i].active == TUNNEL_SETUP)
				return ip_tunnel_setting(ifp, 1, &ip_tun_table[i]);
		}
	}
	return CMD_SUCCESS;
}
static int ip_tunnel_is(char *name)
{
	if(memcmp(name, "tunnel",3) == 0)
	{
		return 1;
	}
	return 0;
}
/**************************************************************/
/**************************************************************/
/**************************************************************/
DEFUN (ip_tunnel_src,
		ip_tunnel_src_cmd,
	    "ip tunnel local A.B.C.D",
		IP_STR
	    "tunnel Interface\n"
		"Select local ip address\n"
		"IP address information\n")
{
	int index;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s %s",ifp->name,VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_address(index, argv[0], NULL);
	else
	{
		ip_tunnel_address(index, argv[0], NULL);
		return ip_tunnel_active_setting(ifp, index);
	}
}
DEFUN (ip_tunnel_dest,
		ip_tunnel_dest_cmd,
	    "ip tunnel remote A.B.C.D",
		IP_STR
	    "tunnel Interface\n"
		"Select remote ip address\n"
		"IP address information\n")
{
	int index;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_address(index, NULL, argv[0]);
	else
	{
		ip_tunnel_address(index, NULL, argv[0]);
		return ip_tunnel_active_setting(ifp, index);
	}
}
DEFUN (ip_tunnel_ttl,
		ip_tunnel_ttl_cmd,
	    "ip tunnel ttl <16-255>",
		IP_STR
	    "tunnel Interface\n"
		"configure ttl\n"
		"ttl value\n")
{
	int ttl = 0;
	int index;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	ttl = atoi(argv[0]);
	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(index, 0, ttl);
	else
	{
		ip_tunnel_mtu_ttl(index, 0, ttl);
		return ip_tunnel_active_setting(ifp, index);
	}
}
DEFUN (no_ip_tunnel_ttl,
		no_ip_tunnel_ttl_cmd,
	    "no ip tunnel ttl",
		NO_STR
		IP_STR
	    "tunnel Interface\n"
		"configure ttl\n")
{
	int index;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(index, 0, TUNNEL_TTL_DEFAULT);
	return CMD_SUCCESS;
}
DEFUN (ip_tunnel_mtu,
		ip_tunnel_mtu_cmd,
	    "ip tunnel mtu <64-1500>",
		IP_STR
	    "tunnel Interface\n"
		"configure mtu\n"
		"mtu value\n")
{
	int mtu = 0;
	int index;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	mtu = atoi(argv[0]);
	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(index, mtu, 0);
	else
	{
		ip_tunnel_mtu_ttl(index, mtu, 0);
		return ip_tunnel_active_setting(ifp, index);
	}
}
DEFUN (no_ip_tunnel_mtu,
		no_ip_tunnel_mtu_cmd,
	    "no ip tunnel mtu",
		NO_STR
		IP_STR
	    "tunnel Interface\n"
		"configure mtu\n")
{
	int index;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(index, TUNNEL_MTU_DEFAULT, 0);
	return CMD_SUCCESS;
}
DEFUN (ip_tunnel_c_mode,
		ip_tunnel_mode_cmd,
	    "ip tunnel mode (gre|ipip|vti|sit)",
		IP_STR
	    "tunnel Interface\n"
		"configure mode\n"
		"gre tunnel mode\n"
		"ipip tunnel mode\n"
		"vti tunnel mode\n"
		"sit tunnel mode\n")
{
	int index = 0;
	int mode = TUNNEL_GRE;
	struct interface *ifp;
	ifp = vty->index;
	if(!ip_tunnel_is(ifp->name))
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = ip_tunnel_name_to_index(ifp->name);

	vty_out (vty, " selete %s %d %s",ifp->name,index,VTY_NEWLINE);

	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	if(memcmp(argv[0],"gre",2)==0)
		mode = TUNNEL_GRE;
	else if(memcmp(argv[0],"ipip",2)==0)
		mode = TUNNEL_IPIP;
	else if(memcmp(argv[0],"vti",2)==0)
		mode = TUNNEL_VTI;
	else if(memcmp(argv[0],"sit",2)==0)
		mode = TUNNEL_SIT;
	if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		return ip_tunnel_mode(index, mode);
	else
	{
		ip_tunnel_mode(index, mode);
		return ip_tunnel_active_setting(ifp, index);
	}
}

DEFUN (interface_tunnel,
		interface_tunnel_cmd,
	    "interface tunnel <0-32>",
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")
{
	int ret = 0;
	size_t sl;
	int index = 0;
	char ifname[INTERFACE_NAMSIZ];
	struct interface *ifp;
	memset(ifname, 0, sizeof(ifname));
	if(argv[0] == NULL)
	{
		vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = atoi(argv[0]);
	sprintf(ifname, "tunnel%d", index);
	sl = strlen(ifname);
#ifdef SUNOS_5
	ifp = if_sunwzebra_get (ifname, sl);
#else
	ifp = if_get_by_name_len(ifname, sl);
#endif /* SUNOS_5 */
	if(!ip_tunnel_is_create(index))
	{
		ret = ip_tunnel_create(index);
		if(ret != CMD_SUCCESS)
		{
			if_delete(ifp);
			vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
			return CMD_WARNING;
		}
	}
	vty->index = ifp;
	vty->node = INTERFACE_NODE;
	return CMD_SUCCESS;
}

DEFUN (no_interface_tunnel,
		no_interface_tunnel_cmd,
	    "no interface tunnel <0-32>",
		NO_STR
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")
{
	  struct interface *ifp;
	  char ifname[IFNAMSIZ];
	  int num = -1;
	  if(argv[0])
		  num = atoi(argv[0]);
	  if(num != -1)
	  {
		  memset(ifname, 0, sizeof(ifname));
		  sprintf(ifname,"tunnel%d",num);
		  ifp = if_lookup_by_name (ifname);

		  if (ifp == NULL)
			{
			  vty_out (vty, "%% Interface %s does not exist%s", ifname, VTY_NEWLINE);
			  return CMD_WARNING;
			}
		  //if(ip_tunnel_is_active(index) == TUNNEL_ACTIVE)
		  ip_tunnel_delete(num);

		  if_delete(ifp);
	  }
	  return CMD_SUCCESS;
}
static int tunnel_interface_config_write (struct vty *vty)
{
  int i = 0;
  int index = 0;
  struct listnode *node;
  struct interface *ifp;

  for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
    {
	  index = ip_tunnel_name_to_index(ifp->name);

      vty_out (vty, "interface %s%s", ifp->name, VTY_NEWLINE);

      if (ifp->desc)
    	  vty_out (vty, " description %s%s", ifp->desc,VTY_NEWLINE);

  	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
  	{
  		if( ip_tun_table[i].index == index && ip_tun_table[i].active == TUNNEL_ACTIVE)
  		{
			vty_out (vty, " ip tunnel local %s%s", inet_ntoa(ip_tun_table[i].local),VTY_NEWLINE);
			vty_out (vty, " ip tunnel remote %s%s", inet_ntoa(ip_tun_table[i].remote),VTY_NEWLINE);
			vty_out (vty, " ip tunnel %s%s", ip_tun_mode_str[ip_tun_table[i].mode],VTY_NEWLINE);
			vty_out (vty, " ip tunnel mtu %d%s", ip_tun_table[i].mtu,VTY_NEWLINE);
			vty_out (vty, " ip tunnel ttl %d%s", ip_tun_table[i].ttl,VTY_NEWLINE);
  		}
  	}
      vty_out (vty, "!%s", VTY_NEWLINE);
    }
  return 0;
}
#ifdef TUNNEL_DEBUG
DEFUN (show_interface_tunnel,
		show_interface_tunnel_cmd,
	    "show interface tunnel",
		SHOW_STR
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")
{
	  int i = 0;
	  int index = 0;
  	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
  	{
  		if( ip_tun_table[i].mtu/*ip_tun_table[i].index == index && ip_tun_table[i].active == TUNNEL_ACTIVE*/)
  		{
  			vty_out (vty, " ip tunnel index %d ifindex %d %s", ip_tun_table[i].index,ip_tun_table[i].ifindex,VTY_NEWLINE);
			vty_out (vty, " ip tunnel local %s%s", inet_ntoa(ip_tun_table[i].local),VTY_NEWLINE);
			vty_out (vty, " ip tunnel remote %s%s", inet_ntoa(ip_tun_table[i].remote),VTY_NEWLINE);
			vty_out (vty, " ip tunnel %s %d%s", ip_tun_mode_str[ip_tun_table[i].mode],ip_tun_table[i].mode,VTY_NEWLINE);
			vty_out (vty, " ip tunnel mtu %d%s", ip_tun_table[i].mtu,VTY_NEWLINE);
			vty_out (vty, " ip tunnel ttl %d%s", ip_tun_table[i].ttl,VTY_NEWLINE);
			vty_out (vty, " ip tunnel active %d%s", ip_tun_table[i].active,VTY_NEWLINE);
  		}
  	}
	  return CMD_SUCCESS;
}
DEFUN (test_interface_tunnel,
		test_interface_tunnel_cmd,
	    "test tunnel <1-32>",
		SHOW_STR
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")
{
		size_t sl;
		int index = 0;
		char ifname[INTERFACE_NAMSIZ];
		struct interface *ifp;
		memset(ifname, 0, sizeof(ifname));
		if(argv[0] == NULL)
		{
			vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
			return CMD_WARNING;
		}
		index = atoi(argv[0]);
		sprintf(ifname, "tunnel%d", index);
		sl = strlen(ifname);
	#ifdef SUNOS_5
		ifp = if_sunwzebra_get (ifname, sl);
	#else
		ifp = if_get_by_name_len(ifname, sl);
	#endif /* SUNOS_5 */

	  ip_tunnel_active_setting(ifp,  index);
	  return CMD_SUCCESS;
}
#endif
static struct cmd_node interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
  1,
};

int ip_tunnel_cmd_init (void)
{
	int i =0;
	if_init();
	//if_add_hook (IF_NEW_HOOK, rip_interface_new_hook);
	//if_add_hook (IF_DELETE_HOOK, rip_interface_delete_hook);

	memset(ip_tun_table, 0, sizeof(ip_tun_table));
	for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		ip_tun_table[i].index = -1;

	}
	install_node (&interface_node, tunnel_interface_config_write);
	install_default (INTERFACE_NODE);

#ifdef TUNNEL_DEBUG
	install_element (ENABLE_NODE, &show_interface_tunnel_cmd);
	install_element (INTERFACE_NODE, &test_interface_tunnel_cmd);
#endif
	install_element (CONFIG_NODE, &interface_cmd);
	install_element (CONFIG_NODE, &no_interface_cmd);
	install_element (INTERFACE_NODE, &interface_desc_cmd);
	install_element (INTERFACE_NODE, &no_interface_desc_cmd);

	install_element (CONFIG_NODE, &interface_tunnel_cmd);
	install_element (CONFIG_NODE, &no_interface_tunnel_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_mode_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_src_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_dest_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_ttl_cmd);
	install_element (INTERFACE_NODE, &no_ip_tunnel_ttl_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_mtu_cmd);
	install_element (INTERFACE_NODE, &no_ip_tunnel_mtu_cmd);
}
#endif /* HAVE_UTILS_TUNNEL */
