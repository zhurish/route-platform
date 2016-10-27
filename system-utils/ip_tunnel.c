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
#include "system-utils/utils_interface.h"
#include "system-utils/ip_tunnel.h"

#ifdef HAVE_UTILS_TUNNEL



/**************************************************************/
/**************************************************************/

/**************************************************************/
/**************************************************************/
//change: ip tunnel change tunnel0 ttl
//change: ip link set dev tunnel0 mtu 1400
//ip tunnel change tunnel1 local 192.168.122.1
//change: ip tunnel change tunnel1 remote 19.1.1.1

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
/**************************************************************/

static int ip_tunnel_setting(struct utils_interface *ifp, int type)
{
	char src[32];
	char dest[32];
	char cmd[256];
	if( /*ifp->tun_index &&*/
			ifp->tun_mode &&
			ifp->tun_mtu &&
			ifp->tun_ttl &&
			ifp->source.s_addr &&
			ifp->remote.s_addr)
	{
		if(type)
		{
			//ip tunnel add tunnel2 mode gre remote 192.168.1.130 local 192.168.1.19 ttl 250
			UTILS_DEBUG_LOG ("add tunnel%d",ifp->tun_index);
			memset(src, 0, sizeof(src));
			memset(dest, 0, sizeof(dest));
			memset(cmd, 0, sizeof(cmd));
			sprintf(src, "local %s", inet_ntoa(ifp->source));
			sprintf(dest, "remote %s", inet_ntoa(ifp->remote));

			sprintf(cmd, "ip tunnel add %s %s %s %s ttl %d",ifp->name,
					ip_tun_mode_str[ifp->tun_mode], src, dest, ifp->tun_ttl);
			super_system(cmd);

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "ip link set dev %s mtu %d up",ifp->name,ifp->tun_mtu);
			super_system(cmd);
			if(ifp)
			{
				if_get_ifindex (ifp);
			}
			ifp->active = TUNNEL_ACTIVE;
			return CMD_SUCCESS;
		}
		else
		{
			if(ifp->active != TUNNEL_ACTIVE)
			{
				ifp->active = 0;
				return CMD_SUCCESS;
			}
			UTILS_DEBUG_LOG ("del tunnel%d",ifp->tun_index);
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "ip link set dev %s down",ifp->name);
			super_system(cmd);

			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "ip tunnel del %s",ifp->name);
			super_system(cmd);

			ifp->active = 0;
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
/**************************************************************/
static int ip_tunnel_mtu_ttl(struct utils_interface *ifp, int mtu, int ttl)
{
	//int i = 0;
	char cmd[256];
	//for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		//if(ip_tun_table[i].active = TUNNEL_SETUP && ip_tun_table[i].index == index)
		{
			if(mtu)
			{
				UTILS_DEBUG_LOG ("setting tunnel%d mtu",ifp->tun_index);
				if( ifp->active == TUNNEL_ACTIVE &&
						ifp->tun_mtu != 0 &&
						ifp->tun_mtu != mtu)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip link set dev %s mtu %d",ifp->name, ifp->tun_mtu);
					super_system(cmd);
				}
				if(ifp->active == TUNNEL_ACTIVE)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip link set dev %s mtu %d", ifp->name, mtu);
					super_system(cmd);
				}
				ifp->tun_mtu = mtu;
			}
			if(ttl)
			{
				UTILS_DEBUG_LOG ("setting tunnel%d ttl",ifp->tun_index);
				if( ifp->active == TUNNEL_ACTIVE &&
					ifp->tun_ttl != 0 &&
					ifp->tun_ttl != ttl)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip tunnel change %s ttl %d",ifp->name, ifp->tun_ttl);
					super_system(cmd);
				}
				if(ifp->active == TUNNEL_ACTIVE)
				{
					memset(cmd, 0, sizeof(cmd));
					sprintf(cmd, "ip tunnel change %s ttl %d", ifp->name, ttl);
					super_system(cmd);
				}
				ifp->tun_ttl = ttl;
			}
			return CMD_SUCCESS;
		}
	}
	return CMD_WARNING;
}
static int ip_tunnel_active_setting(struct utils_interface *ifp)
{
	//int i = 0;
	//for(i = 0; i < TUNNEL_TABLE_MAX; i++)
	{
		if(/* ifp->index == index && */
			ifp->tun_mode &&
			ifp->tun_mtu &&
			ifp->tun_ttl &&
			ifp->source.s_addr &&
			ifp->remote.s_addr)
		{
			UTILS_DEBUG_LOG ("active setting tunnel%d mtu",ifp->tun_index);
			//if(ifp->active != TUNNEL_ACTIVE)
			if(ifp->active != TUNNEL_ACTIVE)
				return ip_tunnel_setting(ifp, 1);
		}
	}
	return CMD_SUCCESS;
}
//#endif
/**************************************************************/
/**************************************************************/
/**************************************************************/
DEFUN (ip_tunnel_src,
		ip_tunnel_src_cmd,
	    "ip tunnel source A.B.C.D",
		IP_STR
	    "tunnel Interface\n"
		"Select source ip address\n"
		"IP address information\n")
{
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s %s",ifp->name,VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	ifp->source.s_addr = inet_addr(argv[0]);
	if(ifp->active == TUNNEL_ACTIVE)
		return 0;//ip_tunnel_address(ifp, argv[0], NULL);
	else
	{
		return ip_tunnel_active_setting(ifp);
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
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	ifp->remote.s_addr = inet_addr(argv[0]);
	if(ifp->active == TUNNEL_ACTIVE)
		return 0;//ip_tunnel_address(ifp, NULL, argv[0]);
	else
	{
		//ip_tunnel_address(ifp, NULL, argv[0]);
		return ip_tunnel_active_setting(ifp);
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
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	ifp->tun_ttl = atoi(argv[0]);
	if(ifp->active == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(ifp, 0, ifp->tun_ttl);
	else
	{
		//ip_tunnel_mtu_ttl(ifp, 0, ttl);
		return ip_tunnel_active_setting(ifp);
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
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ifp->tun_ttl = TUNNEL_TTL_DEFAULT;
	if(ifp->active == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(ifp, 0, TUNNEL_TTL_DEFAULT);
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
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	ifp->tun_mtu = atoi(argv[0]);
	if(ifp->active == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(ifp, ifp->tun_mtu, 0);
	else
	{
		return ip_tunnel_active_setting(ifp);
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
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ifp->tun_mtu = TUNNEL_MTU_DEFAULT;

	if(ifp->active == TUNNEL_ACTIVE)
		return ip_tunnel_mtu_ttl(ifp, TUNNEL_MTU_DEFAULT, 0);
	return CMD_SUCCESS;
}
DEFUN (ip_tunnel_mode,
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
	//int mode = TUNNEL_GRE;
	struct utils_interface *ifp;
	ifp = ((struct interface *)vty->index)->info;
	if(ifp->type != UTILS_IF_TUNNEL)
	{
		vty_out (vty, "%% only tunnel Interface can execute this cmd %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	if(memcmp(argv[0],"gre",2)==0)
		ifp->tun_mode = TUNNEL_GRE;
	else if(memcmp(argv[0],"ipip",2)==0)
		ifp->tun_mode = TUNNEL_IPIP;
	else if(memcmp(argv[0],"vti",2)==0)
		ifp->tun_mode = TUNNEL_VTI;
	else if(memcmp(argv[0],"sit",2)==0)
		ifp->tun_mode = TUNNEL_SIT;

	if(ifp->active == TUNNEL_ACTIVE)
		return 1;//return ip_tunnel_mode(ifp, mode);
	else
	{
		return ip_tunnel_active_setting(ifp);
	}
}

DEFUN (interface_tunnel,
		interface_tunnel_cmd,
	    "interface tunnel <0-32>",
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")
{
	int index = 0;
	char ifname[INTERFACE_NAMSIZ];
	struct interface *ifp = NULL;
	struct utils_interface *uifp = NULL;
	memset(ifname, 0, sizeof(ifname));
	if(argv[0] == NULL)
	{
		vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	index = atoi(argv[0]);
	sprintf(ifname, "tunnel%d", index);
	uifp = utils_interface_lookup_by_name (ifname);
	if(!uifp)
	{
		//uifp = utils_interface_create (ifname);
		ifp = if_create(ifname, strlen(ifname));
		if(!ifp)
		{
			vty_out (vty, "%% invalid input argv %s",VTY_NEWLINE);
			return CMD_WARNING;
		}
		uifp = ifp->info;
		//create
	}
	uifp->tun_index = index;
	uifp->type = UTILS_IF_TUNNEL;
	uifp->vty = vty;
	vty->index = ifp;
	ifp->info = uifp;
	vty->node = INTERFACE_NODE;
	return CMD_SUCCESS;
}
int no_tunnel_interface(struct vty *vty, const char *ifname)
{
	struct interface *ifp = NULL;
	struct utils_interface *tifp = NULL;
	ifp = if_lookup_by_name (ifname);
	if(ifp)
	{
		tifp = ifp->info;
		if(tifp && tifp->active == TUNNEL_ACTIVE)
			return ip_tunnel_setting(tifp, 0);
	}
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
	char ifname[INTERFACE_NAMSIZ];
	struct interface *ifp = NULL;
	struct utils_interface *tifp = NULL;
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input bridge interface name is null %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	memset(ifname, 0, sizeof(ifname));
	sprintf(ifname, "tunnel%d", atoi(argv[0]));
	//查找接口
	ifp = if_lookup_by_name (ifname);
	if(!ifp)
	{
		vty_out (vty, "%% Can't lookup bridge interface %s %s",ifname,VTY_NEWLINE);
		return CMD_WARNING;
	}
	//当前接口不是tun接口，返回
	tifp = (struct utils_interface *)ifp->info;
	if(tifp->type != UTILS_IF_TUNNEL)
	{
		return CMD_SUCCESS;
		//vty_out (vty, "%% this is not bridge interface %s",VTY_NEWLINE);
		//return CMD_WARNING;
	}
	//delete
	no_tunnel_interface(vty, ifname);
	//删除tun接口
	//utils_interface_delete(tifp);
	return CMD_SUCCESS;
}
static int show_tunnel_interface_detail(struct vty *vty, struct interface *ifp)
{
	struct utils_interface *tifp = ifp->info;
	//show_utils_interface(vty, ifp);
	if(tifp->type == UTILS_IF_TUNNEL)
	{
		show_utils_interface(vty, ifp);
		vty_out (vty, "  tunnel source %s", inet_ntoa(tifp->source));
		vty_out (vty, "  remote %s", inet_ntoa(tifp->remote));
		vty_out (vty, "  tunnel mode %s", ip_tun_mode_str[tifp->tun_mode]);
		vty_out (vty, "  mtu %d", tifp->tun_mtu);
		vty_out (vty, "  ttl %d%s", tifp->tun_ttl,VTY_NEWLINE);
	}
	return CMD_SUCCESS;
}
DEFUN (show_tunnel_interface,
		show_tunnel_interface_cmd,
	    "show tunnel interface [NAME]",
		SHOW_STR
	    "tunnel Interface\n"
		INTERFACE_STR
		"Interface Name\n")
{
	struct interface *ifp = NULL;
	if(argv[0] != NULL && argc == 1)
	{
		ifp = if_lookup_by_name (argv[0]);
		if(!ifp)
		{
			vty_out (vty, "%% Can't lookup interface %s %s",argv[0],VTY_NEWLINE);
			return CMD_WARNING;
		}
		show_tunnel_interface_detail(vty, ifp);
	}
	else
	{
		struct listnode *node;
		for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
		{
			show_tunnel_interface_detail(vty, ifp);
		}
	}
	return CMD_SUCCESS;
}
int tunnel_interface_config_write (struct vty *vty, struct interface *ifp)
{
  struct utils_interface * tifp = (struct utils_interface *)ifp->info;
  if(tifp->type != UTILS_IF_TUNNEL)
  {
		return 0;
  }
  if(tifp->active == TUNNEL_ACTIVE)
  {
	vty_out (vty, " ip tunnel source %s%s", inet_ntoa(tifp->source),VTY_NEWLINE);
	vty_out (vty, " ip tunnel remote %s%s", inet_ntoa(tifp->remote),VTY_NEWLINE);
	vty_out (vty, " ip tunnel %s%s", ip_tun_mode_str[tifp->tun_mode],VTY_NEWLINE);
	vty_out (vty, " ip tunnel mtu %d%s", tifp->tun_mtu,VTY_NEWLINE);
	vty_out (vty, " ip tunnel ttl %d%s", tifp->tun_ttl,VTY_NEWLINE);
  }
  return 0;
}



int utils_tunnel_cmd_init (void)
{
	install_element (ENABLE_NODE, &show_tunnel_interface_cmd);
	install_element (CONFIG_NODE, &interface_tunnel_cmd);
	install_element (CONFIG_NODE, &no_interface_tunnel_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_mode_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_src_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_dest_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_ttl_cmd);
	install_element (INTERFACE_NODE, &no_ip_tunnel_ttl_cmd);
	install_element (INTERFACE_NODE, &ip_tunnel_mtu_cmd);
	install_element (INTERFACE_NODE, &no_ip_tunnel_mtu_cmd);
	return 0;
}
#endif /* HAVE_UTILS_TUNNEL */
