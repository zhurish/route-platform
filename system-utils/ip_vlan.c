/*
 * ip_vlan.c
 *
 *  Created on: Oct 17, 2016
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
#include "system-utils/ip_vlan.h"
#ifdef HAVE_UTILS_VLAN
#include "linux/if_vlan.h"
#include "linux/sockios.h"
/*
vconfig set_name_type VLAN_PLUS_VID_NO_PAD
vconfig add eth1 3
Usage: add             [interface-name] [vlan_id]
       rem             [vlan-name]
       set_flag        [interface-name] [flag-num]       [0 | 1]
       set_egress_map  [vlan-name]      [skb_priority]   [vlan_qos]
       set_ingress_map [vlan-name]      [skb_priority]   [vlan_qos]
       set_name_type   [name-type]
*/
static int vlan_sock_fd = -1;
static char *base_name = NULL;
static int vlan_sock_init(void)
{
	if ((vlan_sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return errno;
	return vlan_sock_fd;
}

static int ip_vlan_create(const char *name, int vid)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = ADD_VLAN_CMD;
	strcpy(v_req.device1,name);
	v_req.u.VID = vid;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
static int ip_vlan_delete(const char *name, int vid)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = DEL_VLAN_CMD;
	strcpy(v_req.device1,name);
	v_req.u.VID = vid;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
static int ip_vlan_name_type(int type)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = SET_VLAN_NAME_TYPE_CMD;
	v_req.u.name_type = type;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
/*
static int ip_vlan_flag(const char *name, int flag)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = SET_VLAN_FLAG_CMD;
	strcpy(v_req.device1,name);
	v_req.u.flag = flag;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
*/
static int ip_vlan_egress(const char *vname, int skb_priority, int vlan_qos)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = SET_VLAN_EGRESS_PRIORITY_CMD;
	strcpy(v_req.device1,vname);
	v_req.u.skb_priority = skb_priority;
	v_req.vlan_qos = vlan_qos;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
static int ip_vlan_ingress(const char *vname, int skb_priority, int vlan_qos)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = SET_VLAN_INGRESS_PRIORITY_CMD;
	strcpy(v_req.device1,vname);
	v_req.u.skb_priority = skb_priority;
	v_req.vlan_qos = vlan_qos;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
/*
static int ip_vlan_bind_type(int type)
{
	int ret;
	struct vlan_ioctl_args v_req;
	v_req.cmd = SET_VLAN_NAME_TYPE_CMD;
	v_req.u.bind_type = type;
	ret = ioctl(vlan_sock_fd, SIOCSIFVLAN, &v_req);
	return ret < 0 ? errno : 0;
}
*/
/*
[root@localhost sbin]# cat /proc/net/vlan/config
VLAN Dev name	 | VLAN ID
Name-Type: VLAN_NAME_TYPE_PLUS_VID_NO_PAD
vlan3          | 3  | enp0s25
[root@localhost sbin]# cat /proc/net/vlan/vlan3
vlan3  VID: 3	 REORDER_HDR: 1  dev->priv_flags: 1
         total frames received            0
          total bytes received            0
      Broadcast/Multicast Rcvd            0

      total frames transmitted            1
       total bytes transmitted           90
Device: enp0s25
INGRESS priority mappings: 0:0  1:0  2:0  3:0  4:0  5:0  6:0 7:0
 EGRESS priority mappings: 0:0 33:4 3:0
[root@localhost sbin]#
 */
static int ip_vlan_id_get(const char *name, int *flag)
{
	int ret;
	struct vlan_ioctl_args v_req;
	memset(&v_req, 0, sizeof(struct vlan_ioctl_args));
	v_req.cmd = GET_VLAN_VID_CMD;
	strcpy(v_req.device1,name);
	strcpy(v_req.u.device2,name);
	errno=0;
	ret = ioctl(vlan_sock_fd, SIOCGIFVLAN, &v_req);
	if(flag)
	{
		*flag = v_req.u.VID;
	}
	return ret < 0 ? errno : 0;
}

static int ip_vlan_skb_qos_get(struct utils_interface *uifp, const char *name)
{
	FILE *fp = NULL;
	char filename[64];
	char ingress[64];
	int eg = 0;
	char *sp = NULL;
	memset(ingress, 0, sizeof(ingress));
	memset(filename, 0, sizeof(filename));
	sprintf(filename,"%s%s",VLAN_DEV_PROC,name);
	fp=fopen(filename,"r");
	if(fp == NULL)
	{
		UTILS_DEBUG_LOG("can not fopen file %s",filename);
		return -1;
	}
	while(feof(fp)==0)
	{
		//INGRESS priority mappings: 0:0  1:0  2:0  3:0  4:0  5:0  6:0 7:0
		// EGRESS priority mappings: 0:0 33:4 3:0
		memset(ingress, 0, sizeof(ingress));
		fscanf(fp,"%s",ingress);

		if(memcmp(ingress, "Device", strlen("Device"))==0)
		{
			fscanf(fp,"%s",ingress);
			strcpy(uifp->vlan_base_name, ingress);
		}
		if(memcmp(ingress, "INGRESS", strlen("INGRESS"))==0)
			eg = 1;
		if(memcmp(ingress, "EGRESS", strlen("EGRESS"))==0)
			eg = 2;
		sp = strtok(ingress,":");
		if(sp && eg)
		{
			int skb = 0,qos = 0;

			if (isdigit ((int) *sp))
			{
				skb = atoi(sp);
			}
			sp = strtok(NULL,":");
			if(sp)
			{
				qos = atoi(sp);
				if(eg == 1 && skb < SKB_QOS_MAX && skb >=0 )
				{
					uifp->ingress_vlan_qos[skb] = qos;
					UTILS_DEBUG_LOG("INGRESS:%d:%d\n",skb,qos);
				}
				else if(eg == 2 && skb < SKB_QOS_MAX && skb >=0 )
				{
					uifp->egress_vlan_qos[skb] = qos;
					UTILS_DEBUG_LOG("EGRESS:%d:%d\n",skb,qos);
				}
			}
		}
	}
	fclose(fp);
	return CMD_SUCCESS;
}

int ip_vlan_startup_config(struct utils_interface *uifp)
{
	int value = 0,ret = 1;
	if(vlan_sock_fd==-1)
		vlan_sock_init();
	ret = ip_vlan_id_get(uifp->name, &value);
	if(ret != 0)
	{
		if(vlan_sock_fd)
			close(vlan_sock_fd);
		vlan_sock_fd = -1;
		return CMD_WARNING;
	}
	uifp->type = UTILS_IF_VLAN;
	uifp->vlanid = value;
	ip_vlan_skb_qos_get(uifp, uifp->name);
	if(vlan_sock_fd)
		close(vlan_sock_fd);
	vlan_sock_fd = -1;
	return CMD_SUCCESS;
}
static int ip_vlan_dev_cmd(struct vty *vty, int cmd, const char *dev, int value, int skb_priority, int vlan_qos)
{
	int ret = CMD_WARNING;
	if(vlan_sock_fd == -1)
		vlan_sock_init();
	switch(cmd)
	{
	case ADD_VLAN_CMD:
		ret = ip_vlan_create(dev, value);
		break;
	case DEL_VLAN_CMD:
		ret = ip_vlan_delete(dev, value);
		break;
	case SET_VLAN_INGRESS_PRIORITY_CMD:
		ret = ip_vlan_ingress(dev, skb_priority,  vlan_qos);
		break;
	case SET_VLAN_EGRESS_PRIORITY_CMD:
		ret = ip_vlan_egress(dev, skb_priority,  vlan_qos);
		break;
	case SET_VLAN_NAME_TYPE_CMD:
		ret = ip_vlan_name_type(value);
		break;
	//case SET_VLAN_FLAG_CMD:
	//	ret = ip_vlan_flag(dev, value);
	//	break;
	}
	if(vlan_sock_fd)
		close(vlan_sock_fd);
	vlan_sock_fd = -1;
	if(ret != CMD_SUCCESS)
		return CMD_WARNING;
	return CMD_SUCCESS;
}
/***********************************************************************************/
int no_vlan_interface(struct vty *vty, const char *ifname)
{
	struct interface *ifp = NULL;
	struct utils_interface *bifp = NULL;
	if(ifname == NULL)
	{
		if(vty)
	      vty_out (vty, "%% invalid input vlan interface name is null %s",VTY_NEWLINE);
		else
			zlog_debug("%% invalid input vlan interface name is null\n");
	    return CMD_WARNING;
	}
	//查找接口
	ifp = if_lookup_by_name (ifname);
	if(!ifp)
	{
		if(vty)
			vty_out (vty, "%% Can't lookup vlan interface %s %s",ifname,VTY_NEWLINE);
		else
			zlog_debug("%% Can't lookup vlan interface %s",ifname);
		return CMD_WARNING;
	}
	//当前接口不是VLAN接口，返回
	bifp = (struct utils_interface *)ifp->info;
	if(bifp->type != UTILS_IF_VLAN)
	{
		return CMD_SUCCESS;
	}
	utils_interface_status (bifp, 0);
	if(ip_vlan_dev_cmd(vty, DEL_VLAN_CMD, bifp->vlan_base_name, bifp->vlanid, 0, 0)!=CMD_SUCCESS)
	{
		if(vty)
			vty_out (vty, "%% Can't delete vlan interface %s%s",ifname,VTY_NEWLINE);
		else
			zlog_debug("%% Can't delete vlan interface %s",ifname);
		return CMD_WARNING;
	}
	return CMD_SUCCESS;
}
/***********************************************************************************/
DEFUN (interface_vlan,
		interface_vlan_cmd,
		"interface vlan <0-32>",
		INTERFACE_STR
	    "vlan Interface\n"
		"Interface name num\n")
{
	int vid;
	char ifname[INTERFACE_NAMSIZ];
	struct utils_interface *bifp = NULL;
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input vlan interface name is null %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	vid = atoi(argv[0]);
	memset(ifname, 0, sizeof(ifname));
	sprintf(ifname, "vlan%d", vid);
	bifp = utils_interface_lookup_by_name (ifname);
	if(!bifp)
	{
		bifp = utils_interface_create(ifname);
		if(!bifp)
		{
			vty_out (vty, "%% Can't create vlan interface %s %s",ifname,VTY_NEWLINE);
			return CMD_WARNING;
		}
		if(base_name)
			strcpy(bifp->vlan_base_name, base_name);
		else
			strcpy(bifp->vlan_base_name, VLAN_PARENT_NAME);
		if(ip_vlan_dev_cmd(vty, ADD_VLAN_CMD, bifp->vlan_base_name, vid, 0, 0)!=CMD_SUCCESS)
		{
			utils_interface_delete(bifp);
			vty_out (vty, "%% Can't create vlan interface %s%s",ifname,VTY_NEWLINE);
			return CMD_WARNING;
		}
		if_get_ifindex(bifp);
	}
	//bifp->index = atoi(argv[0]);
	bifp->type = UTILS_IF_VLAN;
	//bifp->mode = 0;
	bifp->vlanid = vid;
	bifp->vty = vty;
	vty->index = bifp->ifp;
	vty->node = INTERFACE_NODE;
	return CMD_SUCCESS;
}
DEFUN (no_interface_vlan,
		no_interface_vlan_cmd,
		"no interface vlan <0-32>",
		NO_STR
		INTERFACE_STR
	    "vlan Interface\n"
		"Interface name num\n")
{
	char ifname[INTERFACE_NAMSIZ];
	if(argv[0] == NULL)
	{
	      vty_out (vty, "%% invalid input vlan interface name is null %s",VTY_NEWLINE);
	      return CMD_WARNING;
	}
	memset(ifname, 0, sizeof(ifname));
	sprintf(ifname, "vlan%d", atoi(argv[0]));
	return no_vlan_interface(vty, ifname);
}
/*
DEFUN (ip_vlan_header_flag,
		ip_vlan_header_flag_cmd,
		"ip vlan header (on|off)",
		IP_STR
	    "vlan Interface\n"
		"ethernet  header\n"
		"include header\n"
		"exclude header\n")
{
	int flag = 0;
	struct utils_interface *bifp = NULL;
	bifp = ((struct interface *)vty->index)->info;
	//接口不是网桥
	if(bifp->type != UTILS_IF_VLAN)
	{
		vty_out (vty, "%% this interface is not bridge interface  %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(memcmp(argv[0],"on",2)==0)
		flag = 1;
	if(memcmp(argv[0],"off",2)==0)
		flag = 0;

	if(ip_vlan_dev_cmd(vty, ADD_VLAN_CMD, bifp->ifp->name, flag, 0, 0)!=CMD_SUCCESS)
	{
		vty_out (vty, "%% Can't create vlan interface %s%s",bifp->ifp->name,VTY_NEWLINE);
		return CMD_WARNING;
	}
	bifp->flag = flag;
	return CMD_SUCCESS;
}
*/
DEFUN (ip_vlan_skb_qos,
		ip_vlan_skb_qos_cmd,
		"ip vlan (egress|ingress) skb-priority <0-7> qos <0-7>",
		IP_STR
	    "vlan Interface\n"
		"outbound  config\n"
		"inbound  config\n"
		"skb priority\n"
		"skb priority value\n"
		"qos priority\n"
		"802.1q priority value\n")
{
	int cmd = 0;
	int skb = 0,qos = 0;
	struct utils_interface *bifp = NULL;
	bifp = ((struct interface *)vty->index)->info;
	//接口不是网桥
	if(bifp->type != UTILS_IF_VLAN)
	{
		vty_out (vty, "%% this interface is not bridge interface  %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(memcmp(argv[0],"egress",2)==0)
		cmd = SET_VLAN_EGRESS_PRIORITY_CMD;
	if(memcmp(argv[0],"ingress",2)==0)
		cmd = SET_VLAN_INGRESS_PRIORITY_CMD;
	skb = atoi(argv[1]);
	qos = atoi(argv[2]);
	if(ip_vlan_dev_cmd(vty, cmd, bifp->ifp->name, 0, skb, qos)!=CMD_SUCCESS)
	{
		vty_out (vty, "%% Can't create vlan interface %s%s",bifp->ifp->name,VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(cmd == SET_VLAN_EGRESS_PRIORITY_CMD)
		bifp->egress_vlan_qos[skb] = qos;
	if(cmd == SET_VLAN_INGRESS_PRIORITY_CMD)
		bifp->ingress_vlan_qos[skb] = qos;
	return CMD_SUCCESS;
}

DEFUN (no_ip_vlan_skb_qos,
		no_ip_vlan_skb_qos_cmd,
		"no ip vlan (egress|ingress) skb-priority <0-7>",
		NO_STR
		IP_STR
	    "vlan Interface\n"
		"outbound  config\n"
		"inbound  config\n"
		"skb priority\n"
		"skb priority value\n")
{
	int cmd = 0;
	int skb = 0;
	struct utils_interface *bifp = NULL;
	bifp = ((struct interface *)vty->index)->info;
	//接口不是网桥
	if(bifp->type != UTILS_IF_VLAN)
	{
		vty_out (vty, "%% this interface is not bridge interface  %s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(memcmp(argv[0],"egress",2)==0)
		cmd = SET_VLAN_EGRESS_PRIORITY_CMD;
	if(memcmp(argv[0],"ingress",2)==0)
		cmd = SET_VLAN_INGRESS_PRIORITY_CMD;
	skb = atoi(argv[1]);
	if(ip_vlan_dev_cmd(vty, cmd, bifp->ifp->name, 0, skb, 0)!=CMD_SUCCESS)
	{
		vty_out (vty, "%% Can't create vlan interface %s%s",bifp->ifp->name,VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(cmd == SET_VLAN_EGRESS_PRIORITY_CMD)
		bifp->egress_vlan_qos[skb] = 0;
	if(cmd == SET_VLAN_INGRESS_PRIORITY_CMD)
		bifp->ingress_vlan_qos[skb] = 0;
	return CMD_SUCCESS;
}

static int show_vlan_interface_detail(struct vty *vty, struct interface *ifp)
{
	int i=0;
	struct utils_interface *uifp = ifp->info;
	//show_utils_interface(vty, ifp);
	if(uifp->type == UTILS_IF_VLAN)
	{
		show_utils_interface(vty, ifp);
		vty_out (vty, "  vlan base on %s ID %d%s",uifp->vlan_base_name,uifp->vlanid,VTY_NEWLINE);
		vty_out (vty, "  vlan egress skb priority qos map:");
		for(i = 0; i < SKB_QOS_MAX; i++)
			vty_out (vty, "%d:%d ",i,uifp->egress_vlan_qos[i]);
		vty_out (vty, "%s",VTY_NEWLINE);
		vty_out (vty, "  vlan ingress skb priority qos map:");
		for(i = 0; i < SKB_QOS_MAX; i++)
			vty_out (vty, "%d:%d ",i,uifp->ingress_vlan_qos[i]);
		vty_out (vty, "%s",VTY_NEWLINE);
	}
	return CMD_SUCCESS;
}

DEFUN (show_vlan_interface,
		show_vlan_interface_cmd,
		"show vlan interface [NAME]",
		SHOW_STR
	    "vlan Interface\n"
		INTERFACE_STR
		"Interface name\n")
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
		show_vlan_interface_detail(vty, ifp);
	}
	else
	{
		struct listnode *node;
		for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
		{
			show_vlan_interface_detail(vty, ifp);
		}
	}
	return CMD_SUCCESS;
}
DEFUN (base_vlan_interface,
		base_vlan_interface_cmd,
		"base vlan [NAME]",
		"base Interface\n"
	    "vlan Interface\n"
		"Interface name\n")
{
	if(argv[0] != NULL && argc == 1)
	{
		struct interface *ifp = NULL;
		struct utils_interface *uifp = NULL;
		ifp = if_lookup_by_name (argv[0]);
		if(!ifp)
		{
			vty_out (vty, "%% Can't lookup interface %s %s",argv[0],VTY_NEWLINE);
			return CMD_WARNING;
		}
		uifp = ifp->info;
		if( !uifp || uifp->type == UTILS_IF_VLAN
#ifdef HAVE_UTILS_BRCTL
			|| uifp->type == UTILS_IF_BRIDGE
			|| uifp->br_mode == ZEBRA_INTERFACE_SUB
#endif
#ifdef HAVE_UTILS_TUNNEL
			|| uifp->type == UTILS_IF_TUNNEL
#endif
			)
		{
			vty_out (vty, "%% Can't setting base vlan interface on %s %s",argv[0],VTY_NEWLINE);
			return CMD_WARNING;
		}
		if(base_name)
			free(base_name);
		base_name = strdup(argv[0]);
		return CMD_SUCCESS;
	}
	return CMD_WARNING;
}

int vlan_interface_config_write (struct vty *vty, struct interface *ifp)
{
	int i=0;
	struct utils_interface *uifp = ifp->info;
	//if(uifp->status & ZEBRA_INTERFACE_BRIDGE)
	if( (uifp && uifp->type == UTILS_IF_VLAN) )
	{
		vty_out (vty, " ip vlan access vlan %d%s", uifp->vlanid,VTY_NEWLINE);
		for(i = 0; i < SKB_QOS_MAX; i++)
			if(uifp->egress_vlan_qos[i])
			{
				vty_out (vty, " ip vlan egress skb-priority %d qos %d %s", i, uifp->egress_vlan_qos[i],VTY_NEWLINE);
			}
		for(i = 0; i < SKB_QOS_MAX; i++)
			if(uifp->ingress_vlan_qos[i])
			{
				vty_out (vty, " ip vlan ingress skb-priority %d qos %d %s", i, uifp->ingress_vlan_qos[i],VTY_NEWLINE);
			}
	}
	return CMD_SUCCESS;
}

void utils_vlan_cmd_init (void)
{
	ip_vlan_dev_cmd(NULL, SET_VLAN_NAME_TYPE_CMD, NULL, VLAN_NAME_TYPE_PLUS_VID_NO_PAD, 0, 0);
	install_element (CONFIG_NODE, &interface_vlan_cmd);
	install_element (CONFIG_NODE, &no_interface_vlan_cmd);
	install_element (ENABLE_NODE, &show_vlan_interface_cmd);

	//install_element (INTERFACE_NODE, &ip_vlan_header_flag_cmd);
	install_element (CONFIG_NODE, &base_vlan_interface_cmd);
	install_element (INTERFACE_NODE, &ip_vlan_skb_qos_cmd);
	install_element (INTERFACE_NODE, &no_ip_vlan_skb_qos_cmd);
}

#endif
