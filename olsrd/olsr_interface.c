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

#include "thread.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "memory.h"
#include "command.h"
#include "stream.h"
#include "sockopt.h"
#include "sockunion.h"
#include "log.h"

#include "olsrd/olsrd.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_time.h"

/*
 * Look for an OLSR interface with the given address.
 */
struct olsr_interface *
olsr_if_lookup_by_addr (struct olsr *olsr, struct in_addr *addr)
{
  struct listnode *node;
  struct olsr_interface *oi;

  LIST_LOOP (olsr->oiflist, oi, node)
    if (memcmp (&OLSR_IF_ADDR (oi), addr, 4) == 0)
      return oi;
  
  return NULL;
}

int 
olsr_get_ifindex_by_addr (struct olsr *olsr, struct in_addr *addr)
{
  struct olsr_interface *oi;

  oi = olsr_if_lookup_by_addr (olsr, addr);

  if (oi == NULL)
    return -1;
  else
    return oi->ifp->ifindex;
}


/* 
 * Does this address belongs to me ? (from ripd)
 */
int 
olsr_if_check_address (struct in_addr addr)
{
  struct listnode *node;

  for (node = listhead (olm->iflist); node; nextnode (node))
    {
      struct listnode *cnode;
      struct interface *ifp;

      ifp = getdata (node);

      for (cnode = listhead (ifp->connected); cnode; nextnode (cnode))
	{
	  struct connected *connected;
	  struct prefix_ipv4 *p;

	  connected = getdata (cnode);
	  p = (struct prefix_ipv4 *) connected->address;

	  if (p->family != AF_INET)
	    continue;

	  if (IPV4_ADDR_CMP (&p->prefix, &addr) == 0)
	    return 1;
	}
    }
  return 0;
}

struct olsr_interface *
olsr_oi_lookup_by_ifp (struct olsr *olsr, struct interface *ifp)
{
  struct listnode *node;
  struct olsr_interface *oi;

  for (node = listhead (olsr->oiflist); node; nextnode (node))
    {
      oi = getdata (node);
      if (oi->ifp == ifp)
	return oi;
    }
  return NULL;
}


/*
 * Init output UDP socket for an interface.
 */
int
olsr_output_sock_init (struct olsr_interface *oi)
{
  int sock;
  struct sockaddr_in addr;

  /* UDP socket is okay for output. */
  sock = socket (AF_INET, SOCK_DGRAM, 0);

  if (sock < 0)
    {
      zlog_err ("olsr_output_sock_init: cannot create socket: %s",
		safe_strerror (errno));
      exit (1);
    }

  sockopt_broadcast (sock); 
  sockopt_reuseaddr (sock); 
  sockopt_reuseport (sock); 

  /* Bind to this olsr interface. */
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, &oi->address->u.prefix4, 4);
  addr.sin_port = htons (oi->olsr->port);

  if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)))
    {
      zlog_err ("olsr_output_sock_init: cannot bind socket: %s",
		safe_strerror (errno));
      exit (1);
    }

  return sock;
}


struct olsr_interface * 
olsr_if_new (struct olsr *olsr, struct interface *ifp, struct prefix *p)
{
  struct olsr_interface *oi;


  /* TODO: check if the interface is already created.  */

  oi = XCALLOC (MTYPE_OLSR_IF, sizeof (struct olsr_interface));
  memset (oi, 0, sizeof (struct olsr_interface));

  oi->ifp = ifp;
  oi->address = p;
  
  oi->linkset = list_new ();

  listnode_add (olsr->oiflist, oi);

  if (!olsr->main_addr_is_set)
    {
      memcpy (&olsr->main_addr, &p->u.prefix4, sizeof (struct in_addr));
      olsr->main_addr_is_set = TRUE;
    }

  oi->olsr = olsr;

  return oi;

}

int 
olsr_if_up (struct olsr_interface *oi)
{
  if (oi == NULL)
    return 0;

  /* Create output fifo. */
  if (oi->fifo == NULL)
    oi->fifo = olsr_fifo_new();

  /* Create output socket. */
  oi->sock = olsr_output_sock_init (oi);

  /* Start threads. */
  oi->t_hello = NULL;
  OLSR_TIMER_ON (oi->t_hello, olsr_hello_timer, oi, OLSR_IF_HELLO (oi));

  if (listcount (oi->olsr->advset) > 0)
    OLSR_TIMER_ON (oi->t_tc, olsr_tc_timer, oi, OLSR_IF_TC (oi));

  /* If the node has more than one OLSR interface, start mid timer. */
  if (oi->olsr->oiflist->count > 1)
    OLSR_TIMER_ON (oi->t_mid, olsr_mid_timer, oi, OLSR_IF_MID(oi));
  
  return 1;
}

int
olsr_if_down (struct olsr_interface *oi)
{
  if (oi == NULL)
    return 0;

  /* Shut down senfing socket. */
  close (oi->sock);

  THREAD_TIMER_OFF (oi->t_hello);
  THREAD_TIMER_OFF (oi->t_mid);
  THREAD_TIMER_OFF (oi->t_tc);
  
  THREAD_WRITE_OFF (oi->t_write);

  return 0;
}


void
olsr_if_free (struct olsr_interface *oi)
{
  if (oi == NULL)
    return;
  
  olsr_if_down (oi);

  olsr_linkset_clean (oi);
  list_free (oi->linkset);

  olsr_fifo_clean (oi->fifo);
  olsr_fifo_free (oi->fifo);

  listnode_delete (oi->olsr->oiflist, oi);

  if (memcmp (&oi->olsr->main_addr, &OLSR_IF_ADDR (oi), 4) == 0)
    {
      oi->olsr->main_addr_is_set = FALSE;
      olsr_main_addr_update (oi->olsr);
    }

  memset (oi, 0, sizeof (*oi));
  XFREE (MTYPE_OLSR_IF, oi);
}

/*
 * Check if interface with the given address is configured and return
 * it if yes.
 */
struct olsr_interface *
olsr_if_is_configured (struct olsr *olsr, struct prefix *address)
{
  struct listnode *node;
  struct olsr_interface *oi;

  for (node = listhead (olsr->oiflist); node; nextnode (node))
    {
      oi = getdata (node);

      if (prefix_match (oi->address, address))
	return oi;
    }
  return NULL;
}

/*
 * Add OLSR specific data.
 */
int
olsr_if_new_hook (struct interface *ifp)
{
  struct olsr_if_info *oii;

  oii = XCALLOC (MTYPE_OLSR_IF_INFO, sizeof (struct olsr_if_info));

  oii->hello_interval = OLSR_HELLO_INTERVAL_DEFAULT;
  oii->mid_interval = OLSR_MID_INTERVAL_DEFAULT;
  oii->tc_interval = OLSR_TC_INTERVAL_DEFAULT;
  
  ifp->info = oii;

  return 0;
}

int
olsr_if_delete_hook (struct interface *ifp)
{
  XFREE (MTYPE_OLSR_IF_INFO, ifp->info);
  ifp->info = NULL;
  return 0;
}

void olsr_if_init ()
{
  /* Initialize Zebra interface data structure. */
/* 2016年7月3日 15:31:34 zhurish: 修改接口表初始化操作 */
  //if_init ();
  if_init (VRF_DEFAULT, &iflist);
/* 2016年7月3日 15:31:34  zhurish: 修改接口表初始化操作 */
  olm->iflist = iflist;

  if_add_hook (IF_NEW_HOOK, olsr_if_new_hook);
  if_add_hook (IF_DELETE_HOOK, olsr_if_delete_hook);

}
