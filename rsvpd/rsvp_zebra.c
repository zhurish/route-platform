/* This Module defines procedures for RSVP-TE and Zebra Interactions. It
** acts as a client for zebra.
**
** Copyright (C) 2003 Pranjal Kumar Dutta <prdutta@uers.sourceforge.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <zebra.h>

#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "lsp.h"
#include "stream.h"
#include "zclient.h"
#include "vty.h"
#include "avl.h"
#include "table.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_if.h"
#include "rsvpd/rsvp_route.h"

/*---------------------------------------------------------------------------
 * All information exchange wit zebrad is done through this object
 *-------------------------------------------------------------------------*/
static struct zclient *zclient = NULL; 

/*--------------------------------------------------------------------------
 * Function : rsvp_interface_add 
 * Input    : command = Command from Zebra
 *          : zclient = Pointer to the zclient 
 *          " length  = Length of the zebra command
 * Output   : Always returns 0
 * Synopsis : This procedure is the handler for interface addition message
 *            from zebra.
 * Callers  : This procedure is registered as Callback function in zclient
 *            structure and is invoked when a message for interface addition
 *            is received.
 *--------------------------------------------------------------------------*/
int 
rsvp_interface_add (int command, 
                    struct zclient *zclient,
                    zebra_size_t length)
{
   struct interface *ifp;
   /* Create if object and add to iflist in if.c. */
   ifp = zebra_interface_add_read (zclient->ibuf);
   if (!ifp)
      return -1;
   if (!CHECK_FLAG (ifp->status, ZEBRA_INTERFACE_ACTIVE))
      SET_FLAG (ifp->status, ZEBRA_INTERFACE_ACTIVE);

   /* Create a rsvp_if object and add to this interface. */
   rsvp_if_event_handle (RSVP_IF_EVENT_IF_ADD, ifp);

   return 0;
}

  
/*--------------------------------------------------------------------------
 * Function : rsvp_interface_delete
 * Input    : command = Command from Zebra
 *          : zclient = Pointer to the zclient 
 *          " length  = Length of the zebra command
 * Output   : Always returns 0
 * Synopsis : This procedure is the handler for interface deletion message
 *            from zebra.
 * Callers  : This procedure is registered as Callback function in zclient
 *            structure and is invoked when a message for interface deletion
 *            is received.
 *--------------------------------------------------------------------------*/
int
rsvp_interface_delete (int command,
                       struct zclient *zclient,
                       zebra_size_t length)
{
   struct stream *s;
   struct interface *ifp;

   s = zclient->ibuf;
   ifp = zebra_interface_state_read (s);

   /* If the interface is found then do clean up. */
   if (ifp)
   {
      /* Generate rsvp_if delete event. */
      rsvp_if_event_handle (RSVP_IF_EVENT_IF_DEL, ifp);
      /* Delete the interface from if.c */
      if_delete (ifp);
   }
   return 0;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_interface_up
 * Input    : command = Command from Zebra
 *          : zclient = Pointer to the zclient 
 *          " length  = Length of the zebra command
 * Output   : Always returns 0
 * Synopsis : This procedure is the handler for interface up message
 *            from zebra.
 * Callers  : This procedure is registered as Callback function in zclient
 *            structure and is invoked when a message for interface up event
 *            is received.
 *--------------------------------------------------------------------------*/
int
rsvp_interface_up (int command,
                   struct zclient *zclient,
                   rsvp_size_t length)
{
   struct stream *s;
   struct interface *ifp;

   s = zclient->ibuf;
   ifp = zebra_interface_state_read (s);

   /* Add the event handler here if the interface is enabled for RSVP */
   rsvp_if_event_handle (RSVP_IF_EVENT_UP, ifp);

   return 0;
} 

/*--------------------------------------------------------------------------
 * Function : rsvp_interface_down
 * Input    : command = Command from Zebra
 *          : zclient = Pointer to the zclient 
 *          " length  = Length of the zebra command
 * Output   : Always returns 0
 * Synopsis : This procedure is the handler for interface down message
 *            from zebra.
 * Callers  : This procedure is registered as Callback function in zclient
 *            structure and is invoked when a message for interface down event
 *            is received.
 *--------------------------------------------------------------------------*/
int 
rsvp_interface_down (int command,
                     struct zclient *zclient,
                     rsvp_size_t length)
{
   struct stream *s;
   struct interface *ifp;

   s = zclient->ibuf;
   ifp = zebra_interface_state_read (s);

   /* Add the event handle here if the interface is enabled for RSVP */
   rsvp_if_event_handle (RSVP_IF_EVENT_DOWN, ifp);

   return 0;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_interface_address_add 
 * Input    : command = command from zebra
 *          : zclient = Pointer to zclient
 *          : length = size of the message sent by zebra
 * Output   : Always returns 0
 * Synopsis : This procedure is registered as callback at zclient and is 
 *            invoked whenever a new IPV4/V6 address is added to the interface.
 * Callers  : Callbacks at zclient.
 *---------------------------------------------------------------------------*/ 
int
rsvp_interface_address_add (int command, 
                            struct zclient *zclient,
			    zebra_size_t length)
{
  struct connected *ifc;
  struct interface *ifp;
  ifc = zebra_interface_address_read (command, zclient->ibuf);
  //ifc = zebra_interface_address_add_read (zclient->ibuf);
  ifp = ifc->ifp;
  /* Add the handler that is called when an address is added to the interface*/
  rsvp_if_event_handle (RSVP_IF_EVENT_IP_ADD, ifp);

  return 0;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_interface_address_delete
 * Input    : command = command received from zebra
 *          : zclient = Pointer to the zclient that receives this command.
 *          : length  = Length of the command message received from zebra
 * Output   : Always returns 0
 * Synopsis : This procedure is registered as callback at zclient that is 
 *            invoked when an IPv4/V6 address is deleted from an interface.
 * Callers  : Callbacks at zclient.
 *-------------------------------------------------------------------------*/
int
rsvp_interface_address_delete (int command, 
                               struct zclient *zclient,
			       zebra_size_t length)
{
  struct connected *ifc;
  struct interface *ifp;
  ifc = zebra_interface_address_read (command, zclient->ibuf);
  //ifc = zebra_interface_address_delete_read (zclient->ibuf);
  ifp = ifc->ifp;
  /* Add Handler when an address is deleted from Interface */
  rsvp_if_event_handle (RSVP_IF_EVENT_IP_DEL, ifp);

  return 0;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_zebra_read_ipv4.
 * Input    : command = The Command from zmpls daemon.
 *          : zclient = Pointer to object of type struct zclient.
 *          : length = The length of the message.
 * Output   : Returns 0 always.
 * Synopsis : This is the callback function of zclient for RSVP-TED. This 
 *            adds/del a route sent by zmpls daemon into/from the local route 
 *            database.
 * Callers  : zclient->ipv4_route_add callback.
 *---------------------------------------------------------------------------*/
int
rsvp_zebra_read_ipv4 (int command, 
                      struct zclient *zclient, 
                      zebra_size_t length)
{
  struct stream *s;
  struct zapi_ipv4 api;
  unsigned long ifindex;
  struct in_addr nexthop;
  struct prefix p;

  /* Get the zClient.*/
  s = zclient->ibuf;
  /* Initialize the route info fillers*/
  ifindex = 0;
  nexthop.s_addr = 0;

  /* Type, flags, message. */
  api.type = stream_getc (s);
  api.flags = stream_getc (s);
  api.message = stream_getc (s);

  /* IPv4 prefix. */
  memset (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = stream_getc (s);
  stream_get (&p.u.prefix4, s, PSIZE (p.prefixlen));

  /* Nexthop, ifindex, distance, metric. */
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP))
    {
      api.nexthop_num = stream_getc (s);
      nexthop.s_addr = stream_get_ipv4 (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_IFINDEX))
    {
      api.ifindex_num = stream_getc (s);
      ifindex = stream_getl (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_DISTANCE))
    api.distance = stream_getc (s);
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_METRIC))
    api.metric = stream_getl (s);
  else
    api.metric = 0;

  /* If the Command is to add a route then add.*/
  if (command == ZEBRA_IPV4_ROUTE_ADD)
    rsvp_route_add (api.type, &p, ifindex, &nexthop);
  /* Else delete the Route. */
  else
    rsvp_route_delete (api.type, &p, ifindex, &nexthop);
  return 0;
}

#ifdef HAVE_IPV6
/* Zebra route add and delete treatment. */
int
rsvp_zebra_read_ipv6 (int command, 
                 struct zclient *zclient, 
                 zebra_size_t length)
{
  struct stream *s;
  struct zapi_ipv6 api;
  unsigned long ifindex;
  struct in6_addr nexthop;
  struct prefix_ipv6 p;

  s = zclient->ibuf;
  ifindex = 0;
  memset (&nexthop, 0, sizeof (struct in6_addr));

  /* Type, flags, message. */
  api.type = stream_getc (s);
  api.flags = stream_getc (s);
  api.message = stream_getc (s);

  /* IPv6 prefix. */
  memset (&p, 0, sizeof (struct prefix_ipv6));
  p.family = AF_INET6;
  p.prefixlen = stream_getc (s);
  stream_get (&p.prefix, s, PSIZE (p.prefixlen));

  /* Nexthop, ifindex, distance, metric. */
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP))
    {
      api.nexthop_num = stream_getc (s);
      stream_get (&nexthop, s, 16);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_IFINDEX))
    {
      api.ifindex_num = stream_getc (s);
      ifindex = stream_getl (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_DISTANCE))
    api.distance = stream_getc (s);
  else
    api.distance = 0;
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_METRIC))
    api.metric = stream_getl (s);
  else
    api.metric = 0;

  /* Simply ignore link-local address. */
  if (IN6_IS_ADDR_LINKLOCAL (&p.prefix))
    return 0;
#if 0
  if (command == ZEBRA_IPV6_ROUTE_ADD)
    bgp_redistribute_add ((struct prefix *)&p, NULL, api.metric, api.type);
  else
    bgp_redistribute_delete ((struct prefix *) &p, api.type);
#endif  
  return 0;
}
#endif /* HAVE_IPV6 */


/*---------------------------------------------------------------------------
 * Function : if_lookup_by_ipv4
 * Input    : addr = pointer to struct in_addr
 * Output   : Returns pointer to struct interface if success else returns NULL
 * Synopsis : Yhis function is a libray utility that returns the interface
 *            matching with the input IP address.
 * Callers  :
 *-------------------------------------------------------------------------*/  
struct interface *
if_lookup_by_ipv4 (struct in_addr *addr)
{
  struct listnode *ifnode;
  struct listnode *cnode;
  struct interface *ifp;
  struct connected *connected;
  struct prefix_ipv4 p;
  struct prefix *cp; 
  
  p.family = AF_INET;
  p.prefix = *addr;
  p.prefixlen = IPV4_MAX_BITLEN;

  for (ifnode = listhead (iflist); ifnode; nextnode (ifnode))
    {
      ifp = getdata (ifnode);

      for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
	{
	  connected = getdata (cnode);
	  cp = connected->address;
	    
	  if (cp->family == AF_INET)
	    if (prefix_match (cp, (struct prefix *)&p))
	      return ifp;
	}
    }
  return NULL;
}

struct interface *
if_lookup_by_ipv4_exact (struct in_addr *addr)
{
  struct listnode *ifnode;
  struct listnode *cnode;
  struct interface *ifp;
  struct connected *connected;
  struct prefix *cp; 
  
  for (ifnode = listhead (iflist); ifnode; nextnode (ifnode))
    {
      ifp = getdata (ifnode);

      for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
	{
	  connected = getdata (cnode);
	  cp = connected->address;
	    
	  if (cp->family == AF_INET)
	    if (IPV4_ADDR_SAME (&cp->u.prefix4, addr))
	      return ifp;
	}
    }
  return NULL;
}

#ifdef HAVE_IPV6
struct interface *
if_lookup_by_ipv6 (struct in6_addr *addr)
{
  struct listnode *ifnode;
  struct listnode *cnode;
  struct interface *ifp;
  struct connected *connected;
  struct prefix_ipv6 p;
  struct prefix *cp; 
  
  p.family = AF_INET6;
  p.prefix = *addr;
  p.prefixlen = IPV6_MAX_BITLEN;

  for (ifnode = listhead (iflist); ifnode; nextnode (ifnode))
    {
      ifp = getdata (ifnode);

      for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
	{
	  connected = getdata (cnode);
	  cp = connected->address;
	    
	  if (cp->family == AF_INET6)
	    if (prefix_match (cp, (struct prefix *)&p))
	      return ifp;
	}
    }
  return NULL;
}

struct interface *
if_lookup_by_ipv6_exact (struct in6_addr *addr)
{
  struct listnode *ifnode;
  struct listnode *cnode;
  struct interface *ifp;
  struct connected *connected;
  struct prefix *cp; 

  for (ifnode = listhead (iflist); ifnode; nextnode (ifnode))
    {
      ifp = getdata (ifnode);

      for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
	{
	  connected = getdata (cnode);
	  cp = connected->address;
	    
	  if (cp->family == AF_INET6)
	    if (IPV6_ADDR_SAME (&cp->u.prefix6, addr))
	      return ifp;
	}
    }
  return NULL;
}

int
if_get_ipv6_global (struct interface *ifp, 
                    struct in6_addr *addr)
{
  struct listnode *cnode;
  struct connected *connected;
  struct prefix *cp; 
  
  for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
    {
      connected = getdata (cnode);
      cp = connected->address;
	    
      if (cp->family == AF_INET6)
	if (! IN6_IS_ADDR_LINKLOCAL (&cp->u.prefix6))
	  {
	    memcpy (addr, &cp->u.prefix6, IPV6_MAX_BYTELEN);
	    return 1;
	  }
    }
  return 0;
}

int
if_get_ipv6_local (struct interface *ifp, 
                   struct in6_addr *addr)
{
  struct listnode *cnode;
  struct connected *connected;
  struct prefix *cp; 
  
  for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
    {
      connected = getdata (cnode);
      cp = connected->address;
	    
      if (cp->family == AF_INET6)
	if (IN6_IS_ADDR_LINKLOCAL (&cp->u.prefix6))
	  {
	    memcpy (addr, &cp->u.prefix6, IPV6_MAX_BYTELEN);
	    return 1;
	  }
    }
  return 0;
}
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Function : rsvp_zclient_reset 
 * Input    : None
 * Output   : None
 * Synopsis : This procedure resets the zclient
 *--------------------------------------------------------------------------*/
void
rsvp_zclient_reset ()
{
   zclient_reset (zclient);
   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_zebra_init 
 * Input    : enable
 * Output   : None
 * Synopsis : This is the initialization function of this module. It 
 *          : initializes the zClient for communication with zebra.
 * Callers  : rsvp_init() in rsvpd.c
 *--------------------------------------------------------------------------*/
void
rsvp_zebra_init (int enable)
{
   int i;
   /* Set Defaukt Values. */
   zclient = zclient_new ();
   zclient_init (zclient, ZEBRA_ROUTE_RSVP);

   /* Set that I am interedted in MPLS.*/
   //zclient->mpls_enable = 1;   
   /* Set redistribution for all routes. */
   for (i = 0; i < ZEBRA_ROUTE_MAX; i++)
   	   zclient_redistribute (ZEBRA_REDISTRIBUTE_ADD, zclient, i);

   //   zclient_redistribute_set (zclient, i);

   zclient->interface_add = rsvp_interface_add;
   zclient->interface_delete = rsvp_interface_delete;
   zclient->interface_address_add = rsvp_interface_address_add;
   zclient->interface_address_delete = rsvp_interface_address_delete;
   zclient->ipv4_route_add = rsvp_zebra_read_ipv4;
   zclient->ipv4_route_delete = rsvp_zebra_read_ipv4;
   zclient->interface_up = rsvp_interface_up;
   zclient->interface_down = rsvp_interface_down;
#ifdef HAVE_IPV6
   zclient->ipv6_route_add = rsvp_zebra_read_ipv6;
   zclient->ipv6_route_delete = rsvp_zebra_read_ipv6;
#endif /* HAVE_IPV6 */

   /* Inserts hooks into main rsvp. */
   rm->rsvp->zclient = zclient;
}
