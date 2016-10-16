/*
 * OLSR Rout(e)ing protocol
 *
 * Copyright (C) 2005        Tudor Golubenco
 *                           Polytechnics University of Bucharest 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public Licenseas published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
 * more details.

 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <zebra.h>
#include "version.h"
#include "thread.h"
#include "command.h"
#include "network.h"
#include "prefix.h"
#include "routemap.h"
#include "table.h"
#include "stream.h"
#include "memory.h"
#include "zclient.h"
#include "filter.h"
#include "plist.h"
#include "log.h"

#include "olsrd/olsrd.h"
#include "olsrd/olsr_zebra.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_debug.h"


/* Zebra structure to hold current status. */
struct zclient *zclient = NULL;

/* Send new route to the zebra daemon.*/
void
olsr_zebra_ipv4_add (struct prefix_ipv4 *p, struct in_addr *nexthop,
		     u_int32_t metric, u_int32_t ifindex)
{
  struct zapi_ipv4 api;

  if (zclient->redist[ZEBRA_ROUTE_OLSR])
    {
      api.type = ZEBRA_ROUTE_OLSR;
      api.flags = 0;
      api.message = 0;
      SET_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP);
      api.nexthop_num = 1;
      api.nexthop = &nexthop;
      api.ifindex_num = 0;
      SET_FLAG (api.message, ZAPI_MESSAGE_METRIC);
      api.metric = metric;

      zapi_ipv4_route (ZEBRA_IPV4_ROUTE_ADD, zclient, p, &api);
    }
}

/* Tell zebra to delete a route. */
void
olsr_zebra_ipv4_delete (struct prefix_ipv4 *p, struct in_addr *nexthop, 
		       u_int32_t metric)
{
  struct zapi_ipv4 api;

  if (zclient->redist[ZEBRA_ROUTE_OLSR])
    {
      api.type = ZEBRA_ROUTE_OLSR;
      api.flags = 0;
      api.message = 0;
      SET_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP);
      api.nexthop_num = 1;
      api.nexthop = &nexthop;
      api.ifindex_num = 0;
      SET_FLAG (api.message, ZAPI_MESSAGE_METRIC);
      api.metric = metric;

      zapi_ipv4_route (ZEBRA_IPV4_ROUTE_DELETE, zclient, p, &api);
    }
}



/* Inteface addition message from zebra. */
int
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
olsr_interface_add (int command, struct zclient *zclient, zebra_size_t length, vrf_id_t vrf_id)
#else
olsr_interface_add (int command, struct zclient *zclient, zebra_size_t length)
#endif
{
  struct interface *ifp;
  struct olsr *olsr;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  ifp = zebra_interface_add_read (zclient->ibuf, vrf_id);
#else
  ifp = zebra_interface_add_read (zclient->ibuf);
#endif
  if (IS_DEBUG_EVENT (ZEBRA))
    zlog_debug ("Zebra interface add %s index %d flags %ld metric %d mtu %d",
		ifp->name, ifp->ifindex, ifp->flags, ifp->metric, ifp->mtu);

  olsr = olsr_lookup ();
  if (olsr != NULL)
    olsr_if_update (olsr);
  
  return 0;
}


/* Interface deletion message from zebra */
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
int olsr_interface_delete (int command, struct zclient *zclient,
		       zebra_size_t length, vrf_id_t vrf_id)
#else
int olsr_interface_delete (int command, struct zclient *zclient,
		       zebra_size_t length)
#endif
{
  struct interface *ifp;
  struct olsr *olsr;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  ifp = zebra_interface_state_read (zclient->ibuf, vrf_id);
#else
  ifp = zebra_interface_state_read (zclient->ibuf);
#endif
  if (ifp == NULL)
    return 0;

  if (if_is_up (ifp))
      zlog_warn ("Zebra: got delete of %s, but interface is still up",
		 ifp->name);

  if (IS_DEBUG_EVENT (ZEBRA))
    zlog_debug ("Zebra: interface delete %s index %d flags %lu metric %d mtu %d",
	       ifp->name, ifp->ifindex, ifp->flags, ifp->metric, ifp->mtu);

  if_delete(ifp);

  olsr = olsr_lookup ();
  if (olsr != NULL)
    olsr_if_update (olsr);

  return 0;
}

#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
int olsr_interface_state_up (int command, struct zclient *zclient,
			     zebra_size_t length, vrf_id_t vrf_id)
#else
int olsr_interface_state_up (int command, struct zclient *zclient,
			     zebra_size_t length)
#endif
{
  struct interface *ifp;
  struct olsr *olsr;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  ifp = zebra_interface_state_read (zclient->ibuf, vrf_id);
#else
  ifp = zebra_interface_state_read (zclient->ibuf);
#endif
  if (ifp == NULL)
    return 0;

  if (IS_DEBUG_EVENT (ZEBRA))
    zlog_debug ("Zebra: Interface[%s] state change to up.", ifp->name);

  olsr = olsr_lookup ();
  if (olsr != NULL)
    olsr_if_update (olsr);

  return 0;
}
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
int olsr_interface_state_down (int command, struct zclient *zclient,
			       zebra_size_t length, vrf_id_t vrf_id)
#else
int olsr_interface_state_down (int command, struct zclient *zclient,
			       zebra_size_t length)
#endif
{
  struct interface *ifp;
  struct olsr *olsr;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  ifp = zebra_interface_state_read (zclient->ibuf, vrf_id);
#else
  ifp = zebra_interface_state_read (zclient->ibuf);
#endif
  if (ifp == NULL)
    return 0;

  if (IS_DEBUG_EVENT (ZEBRA))
    zlog_debug ("Zebra: Interface[%s] state change to down.", ifp->name);

  olsr = olsr_lookup ();
  if (olsr != NULL)
    olsr_if_update (olsr);


  return 0;
}
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
int olsr_interface_address_add (int command, struct zclient *zclient,
                            zebra_size_t length, vrf_id_t vrf_id)
#else
int olsr_interface_address_add (int command, struct zclient *zclient,
                            zebra_size_t length)
#endif
{
  struct connected *ifc;
  struct prefix *p;
  struct olsr *olsr;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  ifc = zebra_interface_address_read (command, zclient->ibuf, vrf_id);
#else
  ifc = zebra_interface_address_read (command, zclient->ibuf);
#endif
  if (ifc == NULL)
    return 0;
  
  p = ifc->address;
  if (p->family == AF_INET)
    {
      if (IS_DEBUG_EVENT (ZEBRA))
	zlog_debug ("Zebra: new IPv4 address %s/%d added on interface %s.",
		    inet_ntoa(p->u.prefix4), p->prefixlen, ifc->ifp->name);

      olsr = olsr_lookup ();
      if (olsr != NULL)
	olsr_if_update (olsr);
    }
  return 0;
}
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
int olsr_interface_address_delete (int command, struct zclient *zclient,
                            zebra_size_t length, vrf_id_t vrf_id)
#else
int olsr_interface_address_delete (int command, struct zclient *zclient,
                            zebra_size_t length)
#endif
{
  struct connected *ifc;
  struct olsr *olsr;
  struct olsr_interface *oi;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  ifc = zebra_interface_address_read (command, zclient->ibuf, vrf_id);
#else
  ifc = zebra_interface_address_read (command, zclient->ibuf);
#endif
  if (ifc == NULL)
    return 0;

  olsr = olsr_lookup ();
  if (olsr != NULL)
    {
      oi = olsr_if_lookup_by_addr (olsr, &ifc->address->u.prefix4);
      olsr_if_free (oi);

      olsr_if_update (olsr);
    }

  if (IS_DEBUG_EVENT (ZEBRA))
    zlog_debug ("Zebra: Address deleted.");
  connected_free (ifc);

  return 0;
}


/* Deserilize zebra protocol stream for route management. */
void zclient_read_ipv4( struct zclient* zclient,
			     struct zapi_ipv4 *zapi, struct prefix_ipv4* p,
			     unsigned long* ifindex,  struct in_addr* nexthop) 
{
  struct stream *s;

  s = zclient->ibuf;

  /* read the header */
  zapi->type = stream_getc (s);
  zapi->flags = stream_getc (s);
  zapi->message = stream_getc (s);

  /* and the prefix */
  memset (p, 0, sizeof (struct prefix_ipv4));
  p->family = AF_INET;
  p->prefixlen = stream_getc (s);
  stream_get (&p->prefix, s, PSIZE (p->prefixlen));
  
  /* Nexthop, ifindex, distance, metric. */
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_NEXTHOP))
    {
      zapi->nexthop_num = stream_getc (s);
      nexthop->s_addr = stream_get_ipv4 (s);
    }
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_IFINDEX))
    {
      zapi->ifindex_num = stream_getc (s);
      *ifindex = stream_getl (s);
    }
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_DISTANCE))
    zapi->distance = stream_getc (s);
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_METRIC))
    zapi->metric = stream_getl (s);

}

static int olsr_external_route_add(struct olsr* olsr,struct in_addr *dest, struct in_addr *nexthop, int metric, int index)
{
	struct interface *ifp = NULL;
	struct connected *ifc = NULL;
	struct listnode *node = NULL;
	struct in_addr *ifaddr = NULL;
	if(olsr == NULL)
		return 0;
	if(olsr->table == NULL)	
		olsr->table = route_table_init ();	

	ifp = if_lookup_by_index (index);
	if(ifp)
	{
		node = listhead(ifp->connected);
		if(node)
			ifc = listgetdata(node);
		if( (ifc)&&(ifc->address) )	
			ifaddr = &ifc->address->u.prefix4;	
	}
	olsr_new_route_add (olsr->table, dest, nexthop, metric, ifaddr);
	return 0;
}
static int olsr_delete_route(struct route_table *table, struct in_addr *dest,
		    struct in_addr *next_hop, int index) 
{
  struct route_node *route = NULL;
  struct olsr_route *rinfo = NULL;  
  route = route_node_lookup_ipv4 (table, dest);
  if (route)
    rinfo = (struct olsr_route*) route->info;
  if(rinfo)
  {
    //if(rinfo->next_hop.s_addr == next_hop->s_addr)
    {
      XFREE(MTYPE_OLSR_ROUTE, rinfo);
      route_unlock_node (route);
    }
    //route_node_delete (node);
  }
  return 0;
}
static int olsr_external_route_del(struct olsr* olsr,struct in_addr *dest, struct in_addr *nexthop, int metric, int index)
{
	if( (olsr == NULL)||(olsr->table == NULL) )
		return 0;	
	olsr_delete_route(olsr->table, dest, nexthop, index);
	return 0;
}
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
static int olsr_zebra_read_ipv4 (int command, struct zclient *zclient, zebra_size_t length, vrf_id_t vrf_id)
#else
static int olsr_zebra_read_ipv4 (int command, struct zclient *zclient, zebra_size_t length)
#endif
{
  struct stream *s;
  struct zapi_ipv4 api;
  unsigned long ifindex;
  struct in_addr nexthop;
  struct prefix_ipv4 p;
  s = zclient->ibuf;
  ifindex = 0;
  nexthop.s_addr = 0;

  /* Type, flags, message. */
  api.type = stream_getc (s);
  api.flags = stream_getc (s);
  api.message = stream_getc (s);

  /* IPv4 prefix. */
  memset (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = stream_getc (s);
  stream_get (&p.prefix, s, PSIZE (p.prefixlen));

  if (IPV4_NET127(ntohl(p.prefix.s_addr)))
    return 0;

  /* Nexthop, ifindex, distance, metric. */
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP))
    {
      api.nexthop_num = stream_getc (s);
      nexthop.s_addr = stream_get_ipv4 (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_IFINDEX))
    {
      api.ifindex_num = stream_getc (s);
      /* XXX assert(api.ifindex_num == 1); */
      ifindex = stream_getl (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_DISTANCE))
    api.distance = stream_getc (s);
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_METRIC))
    api.metric = stream_getl (s);

  if (command == ZEBRA_IPV4_ROUTE_ADD)
    {
    	olsr_external_route_add(olsr_lookup(), &p.prefix, &nexthop, api.metric, ifindex);
    }
  else/* if (command == ZEBRA_IPV4_ROUTE_DELETE) */
    {
			olsr_external_route_del(olsr_lookup(), &p.prefix, &nexthop, api.metric, ifindex);
    }

  return 0;
}


int olsr_is_type_redistributed (int type)
{
  return (zclient->redist[type]);
}

int olsr_redistribute_set (struct vty *vty, int type)
{
  if (olsr_is_type_redistributed (type))
    return CMD_SUCCESS;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  zclient_redistribute (ZEBRA_REDISTRIBUTE_ADD, zclient, type, VRF_DEFAULT);
#else
  zclient_redistribute (ZEBRA_REDISTRIBUTE_ADD, zclient, type);
#endif
	zclient->redist[type] = 1;
  return CMD_SUCCESS;
}

int
olsr_redistribute_unset (struct vty *vty, int type)
{
  if (type == zclient->redist_default)
    return CMD_SUCCESS;

  if (!olsr_is_type_redistributed (type))
    return CMD_SUCCESS;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  zclient_redistribute (ZEBRA_REDISTRIBUTE_DELETE, zclient, type, VRF_DEFAULT);
#else
  zclient_redistribute (ZEBRA_REDISTRIBUTE_DELETE, zclient, type);
#endif

  zclient->redist[type] = 0;

  return CMD_SUCCESS;
}

int
olsr_redistribute_default_set (struct vty *vty, int originate)
{
  /* Redistribute defauilt. */
  if(zclient->default_information)
  	return CMD_SUCCESS;
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  zclient_redistribute_default (ZEBRA_REDISTRIBUTE_DEFAULT_ADD, zclient, VRF_DEFAULT);
#else
  zclient_redistribute_default (ZEBRA_REDISTRIBUTE_DEFAULT_ADD, zclient);
#endif
  zclient->default_information = 1;
  return CMD_SUCCESS;
}

int
olsr_redistribute_default_unset (struct vty *vty)
{
  if(zclient->default_information == 0)
  	return CMD_SUCCESS;	
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  zclient_redistribute_default (ZEBRA_REDISTRIBUTE_DEFAULT_DELETE, zclient, VRF_DEFAULT);
#else
  zclient_redistribute_default (ZEBRA_REDISTRIBUTE_DEFAULT_DELETE, zclient);
#endif
  zclient->default_information = 0;
  return CMD_SUCCESS;
}
DEFUN (olsr_redistribute,
       olsr_redistribute_cmd,
       "redistribute " QUAGGA_REDIST_STR_OLSRD,
       REDIST_STR
       QUAGGA_REDIST_HELP_STR_OLSRD)
{

  int type = -1;
  /* Get distribute source. */
  type = proto_redistnum(AFI_IP, argv[0]);
  if (type < 0 || type == ZEBRA_ROUTE_OLSR)
    return CMD_WARNING;

  return olsr_redistribute_set (vty,type);
}
DEFUN (no_olsr_redistribute,
       no_olsr_redistribute_cmd,
       "no redistribute " QUAGGA_REDIST_STR_OLSRD,
       NO_STR
       REDIST_STR
       QUAGGA_REDIST_HELP_STR_OLSRD)
{

  int type = -1;
  /* Get distribute source. */
  type = proto_redistnum(AFI_IP, argv[0]);
  if (type < 0 || type == ZEBRA_ROUTE_OLSR)
    return CMD_WARNING;

  return olsr_redistribute_unset (vty,type);
}

int  olsr_redistribute_write (struct vty *vty)
{
  int type;
  for(type = 0; type < ZEBRA_ROUTE_MAX; type++)
  {
    if (type != zclient->redist_default && zclient->redist[type])
      vty_out (vty, " redistribute %s%s", zebra_route_string(type), VTY_NEWLINE);
      //zclient_config_printf (vty, "%s", VTY_NEWLINE);
  }
}


void olsr_zebra_init()
{
  /* Allocate zebra structure. */
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  extern struct thread_master *master;
  zclient = zclient_new(master);
#else
  zclient = zclient_new();
#endif
  zclient_init(zclient, ZEBRA_ROUTE_OLSR);

  /* Fill in the callbacks. */
  zclient->interface_add = olsr_interface_add;
  zclient->interface_delete = olsr_interface_delete;
  zclient->interface_up = olsr_interface_state_up;
  zclient->interface_down = olsr_interface_state_down;
  zclient->interface_address_add = olsr_interface_address_add;
  zclient->interface_address_delete = olsr_interface_address_delete;
  
  zclient->ipv4_route_add = olsr_zebra_read_ipv4;
  zclient->ipv4_route_delete = olsr_zebra_read_ipv4;
  
  install_element (OLSR_NODE, &olsr_redistribute_cmd);
  install_element (OLSR_NODE, &no_olsr_redistribute_cmd);  
}
