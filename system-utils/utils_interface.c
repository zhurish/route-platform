/*
 * utils_interface.c
 *
 *  Created on: Oct 16, 2016
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
#include "system-utils/ip_tunnel.h"
#include "system-utils/ip_brctl.h"

static struct list *utils_iflist = NULL;
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
int if_get_ifindex (struct utils_interface *ifp)
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
/******************************************************************************/
int utils_interface_status (struct utils_interface *ifp, int value)
{
	  int ret;
	  struct ifreq ifreq;

	  memset (&ifreq, 0, sizeof(struct ifreq));
	  strncpy (ifreq.ifr_name, ifp->name, IFNAMSIZ);

	  ret = if_ioctl (SIOCGIFFLAGS, (caddr_t) &ifreq);
	  if (ret < 0)
	  {
	        zlog_err("if_ioctl(SIOCGIFFLAGS) failed: %s", safe_strerror(errno));
	        return -1;
	  }
	  if(value)
		  ifreq.ifr_flags |= IFF_UP|IFF_RUNNING;
	  else
		  ifreq.ifr_flags &= ~(IFF_UP|IFF_RUNNING);

	  ifp->flags = ifreq.ifr_flags;

	  ret = if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq);

	  if (ret < 0)
	    {
	      zlog_info ("can't set interface flags");
	      return ret;
	    }
	  return 0;
}
/******************************************************************************/
static int ifname_get_index(const char *name)
{
	const char *sp;
	int dots = 0, nums = 0;
	char buf[4];

	if (name == NULL)
		return -1;
	sp = name;
	memset (buf, 0, sizeof (buf));

	while (sp[dots] != '\0')
	{
		if (isdigit ((int) sp[dots]))
			return buf[nums++] = sp[dots];
		dots++;
	}

	return atoi(buf);
}
/******************************************************************************/
static int utils_interface_new_hook (struct interface *ifp)
{
  //int index = 0;
  //char *pt = NULL;
  struct utils_interface *uifp;
  if(ifp->info)
	  return 0;
  uifp = utils_interface_get_by_name (ifp->name);
  zlog_debug("create interface %s point to utils interface :%s",ifp->name,uifp->name);
  ifp->info = uifp;
  if(uifp)
  {
	  uifp->ifp = ifp;
	  uifp->ifindex = ifp->ifindex;
	  uifp->flags = ifp->flags;
	  if(uifp->vty)
	  	  uifp->vty->index = ifp;
  }
  return 0;
}

/* Called when interface structure deleted. */
static int utils_interface_delete_hook (struct interface *ifp)
{
  if(ifp->info)
  {
	  XFREE (MTYPE_IF, ifp->info);
  }
  ifp->info = NULL;
  return 0;
}


struct utils_interface * utils_interface_lookup_by_ifindex (int index)
{
  struct listnode *node;
  struct utils_interface *ifp;

  for (ALL_LIST_ELEMENTS_RO(utils_iflist, node, ifp))
    {
      if (ifp->ifindex == index)
	return ifp;
    }
  return NULL;
}
struct utils_interface *utils_interface_lookup_by_name (const char *name)
{
  struct listnode *node;
  struct utils_interface *ifp;
  char ifname[INTERFACE_NAMSIZ + 1];
  memset(ifname, 0, INTERFACE_NAMSIZ + 1);
  strcpy(ifname, name);
  if (name)
    for (ALL_LIST_ELEMENTS_RO (utils_iflist, node, ifp))
      {
        if (strncmp(ifname, ifp->name, INTERFACE_NAMSIZ) == 0)
          return ifp;
      }
  return NULL;
}
struct utils_interface *utils_interface_get_by_name (const char *name)
{
  struct utils_interface *uifp;
  uifp = utils_interface_lookup_by_name (name);
  if(uifp)
	  return uifp;
  if(uifp == NULL)
  {
	  int i = 0;
	  zlog_debug("can not find utils interface by :%s",name);
	  uifp = XMALLOC (MTYPE_IF, sizeof(struct utils_interface));
	  if(uifp)
		  memset(uifp, 0, sizeof(struct utils_interface));
	  else
		  return NULL;

	  strncpy (uifp->name, name, strlen(name));

	  uifp->name[strlen(name)] = '\0';
	  //uifp->ifp = ifp;
	  i = ifname_get_index(name);

#ifdef HAVE_UTILS_BRCTL
	  if(ip_bridge_startup_config(uifp)==CMD_SUCCESS)
	  //if(memcmp(uifp->name, "bridge", 2)==0)
	  {
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is bridge interface",uifp->name);
	  }
	  ip_bridge_port_startup_config(uifp);
#endif
#ifdef HAVE_UTILS_TUNNEL
	  if(memcmp(uifp->name, "tun", 3)==0)
	  {
		  uifp->type = UTILS_IF_TUNNEL;
		  if(i != -1)
			  uifp->tun_index = i;
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is tunnel interface",uifp->name);
	  }
	  else if(memcmp(uifp->name, "gre", 3)==0)
	  {
		  uifp->type = UTILS_IF_TUNNEL;
		  if(i != -1)
			  uifp->tun_index = i;
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is tunnel interface",uifp->name);
	  }
	  else if(memcmp(uifp->name, "sit", 3)==0)
	  {
		  uifp->type = UTILS_IF_TUNNEL;
		  if(i != -1)
			  uifp->tun_index = i;
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is tunnel interface",uifp->name);
	  }
	  else if(memcmp(uifp->name, "vit", 3)==0)
	  {
		  uifp->type = UTILS_IF_TUNNEL;
		  if(i != -1)
			  uifp->tun_index = i;
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is tunnel interface",uifp->name);
	  }
	  else if(memcmp(uifp->name, "trap", 3)==0)
	  {
		  uifp->type = UTILS_IF_TUNNEL;
		  if(i != -1)
			  uifp->tun_index = i;
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is tunnel interface",uifp->name);
	  }
	  uifp->tun_ttl = TUNNEL_TTL_DEFAULT;//change: ip tunnel change tunnel0 ttl
	  uifp->tun_mtu = TUNNEL_MTU_DEFAULT;//change: ip link set dev tunnel0 mtu 1400
#endif
#ifdef HAVE_UTILS_VLAN
	  if(ip_vlan_startup_config(uifp)==CMD_SUCCESS)
	  {
		  listnode_add_sort (utils_iflist, uifp);
		  UTILS_DEBUG_LOG("interface %s is vlan interface",uifp->name);
	  }
#endif
  }
  return uifp;
}
struct utils_interface *utils_interface_lookup (const char *name, int index)
{
  struct listnode *node;
  struct utils_interface *ifp;
  char ifname[INTERFACE_NAMSIZ + 1];
  memset(ifname, 0, INTERFACE_NAMSIZ + 1);
  strcpy(ifname, name);
  if (name)
    for (ALL_LIST_ELEMENTS_RO (utils_iflist, node, ifp))
      {
        if ( (strncmp(ifname, ifp->name, INTERFACE_NAMSIZ) == 0) &&
        	 (ifp->ifindex == index) )
        	 //(ifp->index == index) )
          return ifp;
      }
  return NULL;
}
struct utils_interface *utils_interface_create (const char *name)
{
  struct utils_interface *ifp;

  ifp = XCALLOC (MTYPE_IF, sizeof (struct utils_interface));
  assert (ifp);
  memset(ifp, 0, sizeof(struct utils_interface));
  ifp->ifindex = IFINDEX_INTERNAL;

  assert (name);
  assert (strlen(name) <= INTERFACE_NAMSIZ);	/* Need space for '\0' at end. */
  strncpy (ifp->name, name, strlen(name));
  ifp->name[strlen(name)] = '\0';
  if (if_lookup_by_name(ifp->name) == NULL)
    listnode_add_sort (utils_iflist, ifp);
  else
    zlog_err("utils_if_create(%s): corruption detected -- interface with this "
	     "name exists already!", ifp->name);
  return ifp;
}

void utils_interface_delete (struct utils_interface *ifp)
{
  listnode_delete (utils_iflist, ifp);
  XFREE (MTYPE_IF, ifp);
}


int show_utils_interface(struct vty *vty, struct interface *ifp)
{
	vty_out (vty, "Interface %s is ", ifp->name);
	if (if_is_up(ifp))
	{
		 vty_out (vty, "up, line protocol ");
		 if (CHECK_FLAG(ifp->status, ZEBRA_INTERFACE_LINKDETECTION))
		 {
		      if (if_is_running(ifp))
		    	  vty_out (vty, "is up%s", VTY_NEWLINE);
		      else
		    	  vty_out (vty, "is down%s", VTY_NEWLINE);
		 }
		 else
			 vty_out (vty, "detection is disabled%s", VTY_NEWLINE);
	}
	else
		vty_out (vty, "down%s", VTY_NEWLINE);

	if (ifp->desc)
		vty_out (vty, "  Description: %s%s", ifp->desc,VTY_NEWLINE);
	if (ifp->ifindex == IFINDEX_INTERNAL)
		vty_out(vty, "  pseudo interface%s", VTY_NEWLINE);

	else if (! CHECK_FLAG (ifp->status, ZEBRA_INTERFACE_ACTIVE))
	{
		vty_out(vty, "  index %d inactive interface%s", ifp->ifindex, VTY_NEWLINE);
	}
	vty_out (vty, "  flags: %s%s",
		           if_flag_dump (ifp->flags), VTY_NEWLINE);
	return CMD_SUCCESS;
}

static int utils_interface_config_write (struct vty *vty)
{
  struct listnode *node;
  struct interface *ifp;

  for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
    {
      vty_out (vty, "interface %s%s", ifp->name, VTY_NEWLINE);

      if (ifp->desc)
    	  vty_out (vty, " description %s%s", ifp->desc,VTY_NEWLINE);

#ifdef HAVE_UTILS_BRCTL
	  bridge_interface_config_write (vty, ifp);
#endif
#ifdef HAVE_UTILS_VLAN
	  vlan_interface_config_write (vty, ifp);
#endif
#ifdef HAVE_UTILS_TUNNEL
      tunnel_interface_config_write (vty, ifp);
#endif

      vty_out (vty, "!%s", VTY_NEWLINE);
    }
  return CMD_SUCCESS;
}

DEFUN (no_utils_interface,
		no_utils_interface_cmd,
        "no interface IFNAME",
        NO_STR
        "Delete a pseudo interface's configuration\n"
        "Interface's name\n")
{
	  struct interface *ifp;

	  ifp = if_lookup_by_name (argv[0]);

	  if (ifp == NULL)
	    {
	      vty_out (vty, "%% Interface %s does not exist%s", argv[0], VTY_NEWLINE);
	      return CMD_WARNING;
	    }

	  //delete
#ifdef HAVE_UTILS_BRCTL
	  no_bridge_interface(NULL, argv[0]);
#endif
#ifdef HAVE_UTILS_VLAN
	  no_vlan_interface(NULL, argv[0]);
#endif
#ifdef HAVE_UTILS_TUNNEL
	  no_tunnel_interface(NULL, argv[0]);
#endif
	  //if_delete(ifp);

	  return CMD_SUCCESS;
}

static struct cmd_node interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
  1,
};

int utils_interface_init(void)
{
	if(utils_iflist == NULL)
		utils_iflist = list_new ();
	if_init();
	if_add_hook (IF_NEW_HOOK, utils_interface_new_hook);
	if_add_hook (IF_DELETE_HOOK, utils_interface_delete_hook);

	install_node (&interface_node, utils_interface_config_write);
	install_default (INTERFACE_NODE);

	install_element (CONFIG_NODE, &interface_cmd);
	//install_element (CONFIG_NODE, &no_interface_cmd);
	install_element (CONFIG_NODE, &no_utils_interface_cmd);

	install_element (INTERFACE_NODE, &interface_desc_cmd);
	install_element (INTERFACE_NODE, &no_interface_desc_cmd);
	return 0;
}
