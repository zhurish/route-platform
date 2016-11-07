/*******************************************************************************
  Copyright (C), 2001-2004, CETC7.
  �ļ���:  vrrpd.c
  ����:     (W0039)
  �汾:    1.0.0          
  ����:    2005-08-30
  ����:    VRRPģ���ʵ���ļ�
  �����б�:  
    1. -------
  �޸���ʷ:         
    1.����:
      ����:
      �汾:
      �޸�:
    2.-------
*******************************************************************************/
#include "zvrrpd.h"
#include "zvrrp_if.h"



#ifndef ZVRRPD_ON_ROUTTING
/*******************************************************************************/
static struct interface *zvrrp_iflist = NULL;
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_ifadd(struct interface *ifp)
{
	struct interface *p;
	if(zvrrp_iflist == NULL)
	{
		zvrrp_iflist = ifp;
		zvrrp_iflist->next = NULL;
		zvrrp_iflist->prev = NULL;
	}
	else
	{
		p = zvrrp_iflist;
		while(p->next)
		{
			p = p->next;
		}
		p->next = ifp;
		ifp->prev = p;
	}
	return OK;
}
/*******************************************************************************/
static int zvrrp_ifdel(int ifindex)
{
	struct interface *p;
	struct interface *next;
	struct interface *prev;
	if(zvrrp_iflist == NULL)
		return ERROR;
	else
	{
		for(p = zvrrp_iflist; p; p = p->next)
		{
			if(p->ifindex == ifindex)
				break;
			
		}
		if(p)
		{
			next = p->next;
			prev = p->prev;
			prev->next = next;
			free(p->connected);
			free(p);
			return OK;
		}
	}
	return ERROR;
}
#endif// ZVRRPD_ON_ROUTTING
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
struct interface * zif_lookup_by_name(const char *ifname)
{
#ifdef ZVRRPD_ON_ROUTTING	
	return if_lookup_by_name (ifname);
#else //ZVRRPD_ON_ROUTTING	
	struct interface *next;
	if(zvrrp_iflist == NULL)
		return NULL;
	next = zvrrp_iflist;
	while(next)
	{
		if(memcmp(ifname, next->name, INTERFACE_NAMSIZ)==0)
			return next;
		next = zvrrp_iflist->next;
	}
	return NULL;
#endif// ZVRRPD_ON_ROUTTING	
}
/*******************************************************************************/
struct prefix *zif_prefix_get_by_name (struct interface *ifp)
{
#ifdef ZVRRPD_ON_ROUTTING	
	struct connected *connected;
	if(ifp == NULL)
		return NULL;
	connected = (struct connected *)listnode_head (ifp->connected);
	if(connected == NULL)
		return NULL;
	return (struct prefix *)connected->address;
#else
	if(ifp == NULL)
		return NULL;
	return (struct prefix *)ifp->connected;
#endif
}
/*******************************************************************************/
vrrp_if * zvrrp_if_lookup_by_ifindex(int ifindex)
{
	int index;
	vrrp_rt * vsrv = NULL;
	for(index = 0; index < VRRP_VSRV_SIZE_MAX; index++)
	{
	    	vsrv = &(gVrrpMatser->gVrrp_vsrv[index]);
	    	//vsrv->vif
	    	if((vsrv->vif.ifp) && (vsrv->vif.ifp->ifindex == ifindex))
	    		return &(vsrv->vif);
	}
	return NULL;
}
/*******************************************************************************/
#ifdef ZVRRPD_ON_ROUTTING
#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
static int vrrp_interface_config_write (struct vty *vty)
{
  struct listnode *node;
  struct interface *ifp;
  for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
  {
      vty_out (vty, "interface %s%s", ifp->name, VTY_NEWLINE);
      if (ifp->desc)
    	  vty_out (vty, " description %s%s", ifp->desc,VTY_NEWLINE);
  	  vty_out (vty, "!%s", VTY_NEWLINE);
  }
  return CMD_SUCCESS;
}
static struct cmd_node interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
  1,
};
#endif//(ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
#endif// ZVRRPD_ON_ROUTTING
/*******************************************************************************/
int vrrp_interface_init()
{
#ifdef ZVRRPD_ON_ROUTTING
#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
	if_init();
	//if_add_hook (IF_NEW_HOOK, lldp_interface_new_hook);
	//if_add_hook (IF_DELETE_HOOK, lldp_interface_delete_hook);
	install_node (&interface_node, vrrp_interface_config_write);
	install_default (INTERFACE_NODE);

	//install_element (VIEW_NODE, &show_lldp_interface_cmd);
	//install_element (ENABLE_NODE, &show_lldp_interface_cmd);
	install_element (CONFIG_NODE, &interface_cmd);
	install_element (CONFIG_NODE, &no_interface_cmd);
	install_element (INTERFACE_NODE, &interface_desc_cmd);
	install_element (INTERFACE_NODE, &no_interface_desc_cmd);
#endif//(ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
#endif// ZVRRPD_ON_ROUTTING
}
/*******************************************************************************/
/*******************************************************************************/
#ifndef ZVRRPD_ON_ROUTTING
int zvrrp_ifinit_test(int i)
{
	struct interface *ifp;
	struct prefix *pp;
	ifp = malloc(sizeof(struct interface));
	if(ifp)
	{
		memset(ifp, 0, sizeof(struct interface));
		ifp->next = NULL;
		ifp->prev = NULL;	
		strcpy(ifp->name, "vnet0");
		ifp->ifindex = 3;
		/* Interface flags. */
		ifp->flags = 0;
		/* Hardware address. */
		ifp->hw_type = 0;
		//ifp->hw_addr;
		ifp->hw_addr_len = 6;
		/* Connected address list. */
		pp = malloc(sizeof(struct prefix));
		memset(pp, 0, sizeof(struct prefix));
		pp->family = AF_INET;
		pp->prefixlen = 24;
		pp->u.prefix4.s_addr = 0XC0010100 + i;
		ifp->connected = pp;
		
		zvrrp_ifadd(ifp);
	}
}
#endif// ZVRRPD_ON_ROUTTING
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
