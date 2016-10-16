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
#include "memory.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "sockunion.h"
#include "stream.h"
#include "log.h"
#include "sockopt.h"
/* 2016年7月3日 15:33:04 zhurish: 修改头文件名，原来是md5-gnu.h  */
#include "md5.h"
/* 2016年7月3日 15:33:04  zhurish: 修改头文件名，原来是md5-gnu.h  */

#include "olsrd/olsrd.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_mpr.h"
#include "olsrd/olsr_dup.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_vty.h"
#include "olsrd/olsr_debug.h"
#include "olsrd/olsr_route.h"
#include "olsrd/olsr_zebra.h"


/*
 * Returns TRUE if we have recorded a TC packet with a newer sequence 
 * number from the same node.
 */
int
olsr_top_exists_newer (struct olsr *olsr, struct in_addr *last_addr, 
		       u_int16_t ansn)
{
  struct olsr_top *top;
  struct listnode *node;

  LIST_LOOP (olsr->topset, top, node)
    if (memcmp (&top->last_addr, last_addr, 4) == 0 &&
	top->seq > ansn)
      return TRUE;

  return FALSE;
}

/*
 * Deletes all topology tuples from the given node older than ansn.
 */
void 
olsr_top_cleanup_older (struct olsr *olsr, struct in_addr *last_addr, 
		       u_int16_t ansn)
{
  struct olsr_top *top;
  struct listnode *node, *next;

  for (node = listhead (olsr->topset); node; node = next)
    {
      next = node->next;
      top = getdata (node);

      if (memcmp (&top->last_addr, last_addr, 4) == 0 &&
	  top->seq < ansn)
	{
	  list_delete_node (olsr->topset, node);
	  THREAD_TIMER_OFF (top->t_time);
	  XFREE (MTYPE_OLSR_TOP, top);
	}
    }
}

int
olsr_top_del (struct thread *thread)
{
  struct olsr_top *top = THREAD_ARG (thread);
  struct listnode *node = top->node;
  struct olsr *olsr = top->olsr;
  
  XFREE (MTYPE_OLSR_TOP, top);

  list_delete_node (olsr->topset, node);

  return 0;
}

void
olsr_top_add (struct olsr *olsr, float vtime, u_int16_t ansn, 
	      struct in_addr *dest_addr, struct in_addr *last_addr)
{
  struct olsr_top *top = XCALLOC (MTYPE_OLSR_TOP, sizeof (struct olsr_top));

  memcpy (&top->dest_addr, dest_addr, 4);
  memcpy (&top->last_addr, last_addr, 4);
  top->seq = ansn;

  listnode_add (olsr->topset, top);
  top->node = listtail (olsr->topset);
  top->olsr = olsr;

  OLSR_TIMER_ON (top->t_time, olsr_top_del, top, vtime);
}

void
olsr_top_update_time (struct olsr_top *top, float vtime)
{
  THREAD_TIMER_OFF (top->t_time);
  OLSR_TIMER_ON (top->t_time, olsr_top_del, top, vtime);
}


struct olsr_top *
olsr_top_lookup (struct olsr *olsr, struct in_addr *dest_addr,
		 struct in_addr *last_addr)
{
  struct olsr_top *top;
  struct listnode *node;

  LIST_LOOP (olsr->topset, top, node)
    if (memcmp (&top->last_addr, last_addr, 4) == 0 &&
	memcmp (&top->dest_addr, dest_addr, 4) == 0)
      return top;

  return NULL;
}

int
olsr_tc_stop (struct thread *thread)
{
  struct olsr *olsr = THREAD_ARG (thread);
  struct olsr_interface *oi;
  struct listnode *node;

  /* Cancel tc timers. */
  LIST_LOOP (olsr->oiflist, oi, node)
    THREAD_TIMER_OFF (oi->t_tc);

  return 0;
}

void
olsr_top_rem_adv (struct olsr *olsr, struct olsr_neigh *on)
{
  struct listnode *node, *next;

  for (node = listhead (olsr->advset); node; node = next)
    {
      next = node->next;
      if (on == getdata (node))
	{

	  list_delete_node (olsr->advset, node);

	  if (list_isempty (olsr->advset))
	    {
	      /* Stop TC packet generation after a while. */
      
	      /* RFC 3626 
		 When the advertised link set of a node becomes empty, this node
		 SHOULD still send (empty) TC-messages during the a duration equal to
		 the "validity time" (typically, this will be equal to TOP_HOLD_TIME)
		 of its previously emitted TC-messages, in order to invalidate the
		 previous TC-messages.
	      */
	      OLSR_TIMER_ON (olsr->t_delay_stc, olsr_tc_stop, olsr,
			     olsr->top_hold_time);
	    }

	  /* RFC 3626 9.1.
	     A sequence number is associated with the advertised neighbor
	     set.  Every time a node detects a change in its advertised
	     neighbor set, it increments this sequence number
	  */
	  olsr->ansn ++;
	  break;
	}
    }
} 

void
olsr_top_add_adv (struct olsr *olsr, struct olsr_neigh *on)
{
  struct olsr_interface *oi;
  struct listnode *node;

  if (!listnode_lookup (olsr->advset, on))
    {

      listnode_add (olsr->advset, on);

      if (listcount (olsr->advset) == 1)
	{
	  THREAD_TIMER_OFF (olsr->t_delay_stc);

	  /* Start oi tc timers. */
	  LIST_LOOP (olsr->oiflist, oi, node)
	    OLSR_TIMER_ON (oi->t_tc, olsr_tc_timer, oi, OLSR_IF_TC (oi));
	}

      /* RFC 3626 9.1.
	 A sequence number is associated with the advertised neighbor
	 set.  Every time a node detects a change in its advertised
	 neighbor set, it increments this sequence number
      */
      olsr->ansn ++;
    }
}


void 
olsr_rt_update (struct olsr *olsr)
{
  OLSR_TIMER_ON (olsr->t_rt_update, olsr_rt_timer, olsr, 
		 olsr->rt_update_time);
}


struct route_node *
route_node_lookup_ipv4 (struct route_table *table, struct in_addr *addr)
{
  struct prefix_ipv4 p;

  memset (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.prefix = *addr;

  return route_node_lookup (table, (struct prefix *) &p);
}


void 
olsr_new_route_add (struct route_table *table, struct in_addr *dest_addr,
		    struct in_addr *next_hop, int dist,
		    struct in_addr *iface_addr)
{
  struct route_node *rn;
  struct olsr_route *on;
  struct prefix_ipv4 p;

  memset (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.prefix = *dest_addr;

  rn = route_node_get (table, (struct prefix *)&p);

  on = XCALLOC (MTYPE_OLSR_ROUTE, sizeof (struct olsr_route));
  on->next_hop = *next_hop;
  on->dist = dist;
  on->iface_addr = *iface_addr;

  rn->info = on;
}

/*
 * Build routing table in an iterative fashion.
 */
struct route_table *
olsr_route_calculate (struct olsr *olsr)
{
  struct route_table *table;
  struct listnode *node, *node2;
  struct olsr_neigh *on;
  struct olsr_hop2 *hop2;
  struct olsr_link *ol = NULL;	            /* avoid a warning. */
  struct route_node *route;
  struct olsr_route *rinfo;
  struct olsr_top *top;
  struct olsr_mid *mid;
  int found, change = TRUE;
  int hop;

  table = route_table_init ();

  /* RFC 3626 10.
     2    The new routing entries are added starting with the
     symmetric neighbors (h=1) as the destination nodes.
  */
  LIST_LOOP (olsr->neighset, on, node)
    if (on->status == OLSR_NEIGH_SYM || on->status == OLSR_NEIGH_MPR)
      {
	found = FALSE;

	/* For each associated link tuple. */
	LIST_LOOP (on->assoc_links, ol, node2)
	  {
	    olsr_new_route_add (table,
				&ol->neigh_addr,          /* Dest addr.     */
				&ol->neigh_addr,          /* Next hop addr. */
				1,                        /* Distance.      */
				&OLSR_IF_ADDR (ol->oi));  /* Iface addr.    */

	    if (memcmp (&ol->neigh_addr, &on->main_addr, 4) == 0)
	      found = TRUE;
	  }

	/*
	  If in the above, no R_dest_addr is equal to the main address
          of the neighbor, then another new routing entry with MUST be
          added.
	*/
	if (!found)
	  olsr_new_route_add (table,
			      &on->main_addr,           /* Dest addr.     */
			      &ol->neigh_addr,          /* Next hop addr. */
			      1,                        /* Distance.      */
			      &OLSR_IF_ADDR (ol->oi));  /* Iface addr.    */
      }
  
  /*   3    for each node in N2. */
  LIST_LOOP (olsr->n2hopset, hop2, node)
    {
      /* Get the route to its neighbor. */
      route = route_node_lookup_ipv4 (table, &hop2->neigh_addr);

      if (route == NULL)
	{
	  zlog_warn ("olsr_route_calculate: Hop 2 neighbor friend  %s not found in"
		     "the routing table", inet_ntoa (hop2->neigh_addr));
	  continue;
	}

      rinfo = (struct olsr_route*) route->info;
      
      olsr_new_route_add (table,
			  &hop2->hop2_addr,         /* Dest addr.     */
			  &rinfo->next_hop,         /* Next hop addr. */
			  2,                        /* Distance.      */
			  &rinfo->iface_addr);      /* Iface addr.    */
      
    }
  
  /*
    3    The new route entries for the destination nodes h+1 hops away
    are recorded in the routing table.  The following procedure
    MUST be executed for each value of h, starting with h=2 and
    incrementing it by 1 each time.  The execution will stop if no
    new entry is recorded in an iteration.
  */
  hop = 2;
  while (change)
    {
      change = FALSE;

      /* For each topology entry. */
      LIST_LOOP (olsr->topset, top, node)
	{
	  /*   if  T_dest_addr does not correspond to R_dest_addr of any
               route entry in the routing table AND its T_last_addr
               corresponds to R_dest_addr
	  */
	  if (!route_node_lookup_ipv4 (table, &top->dest_addr) &&
	      (route = route_node_lookup_ipv4 (table, &top->last_addr)) != NULL)
	    {
	      rinfo = (struct olsr_route*) route->info;

	      olsr_new_route_add (table,
				  &top->dest_addr,          /* Dest addr.     */
				  &rinfo->next_hop,         /* Next hop addr. */
				  hop + 1,                  /* Distance.      */
				  &rinfo->iface_addr);      /* Iface addr.    */
	      
	      change = TRUE;
	    }
	}
    }
  

  /*
    4    For each entry in the multiple interface association base.
  */
  LIST_LOOP (olsr->midset, mid, node)
    {
      /* if there exists a routing entry such that R_dest_addr  == I_main_addr
         AND there is no routing entry such that R_dest_addr  == I_iface_addr
      */
      if (!route_node_lookup_ipv4 (table, &mid->addr) &&
	  (route = route_node_lookup_ipv4 (table, &mid->main_addr)) != NULL)
	{
	  rinfo = (struct olsr_route*) route->info;

	  olsr_new_route_add (table,
			      &mid->addr,               /* Dest addr.     */
			      &rinfo->next_hop,         /* Next hop addr. */
			      rinfo->dist,              /* Distance.      */
			      &rinfo->iface_addr);      /* Iface addr.    */
	}
      
    }
  
  
  
  /* Print routing table obtained so far. */
  if (IS_DEBUG_EVENT (ROUTE_TABLE))
    olsr_dump_routing_table (table);

  return table;
}


void 
olsr_terminate() 
{
  struct olsr *olsr;
  struct listnode *node;
  struct route_node *rn;
  struct olsr_route *rinfo;

  LIST_LOOP (olm->olsr, olsr, node)
    {
      for (rn = route_top (olsr->table); rn; rn = route_next (rn))
	if ((rinfo = (struct olsr_route*)rn->info) != NULL)
	  {
	    olsr_zebra_ipv4_delete ((struct prefix_ipv4 *)&rn->p, &rinfo->next_hop,
				    rinfo->dist);
	  }
    }

  return;
}


int
olsr_rt_timer (struct thread *thread)
{
  struct olsr *olsr;
  struct route_table *table;
  struct route_node *rn;
  struct olsr_route *rinfo;

  olsr = THREAD_ARG (thread);
  olsr->t_rt_update = NULL;

  table = olsr_route_calculate (olsr);

  /* All nodes that are in the old table but not in the new one must
     be removed. */
  for (rn = route_top (olsr->table); rn; rn = route_next (rn))
    if ((rinfo = (struct olsr_route*)rn->info) != NULL &&
	! route_node_lookup (table, &rn->p))
      {
	olsr_zebra_ipv4_delete ((struct prefix_ipv4 *)&rn->p, &rinfo->next_hop,
				rinfo->dist);
      }

  /* All nodes that are in the new table but not in the old one must
   added. */
  for (rn = route_top (table); rn; rn = route_next (rn))
    if ((rinfo = (struct olsr_route*)rn->info) != NULL &&
	! route_node_lookup (olsr->table, &rn->p))
      {
	int ifindex = olsr_get_ifindex_by_addr (olsr, &rinfo->iface_addr);

	if (ifindex == -1)
	  zlog_warn ("Can't find ifindex for address %s found in routing table. Skipping "
		     "this route", inet_ntoa (rinfo->iface_addr));
	else
	  olsr_zebra_ipv4_add ((struct prefix_ipv4 *)&rn->p, &rinfo->next_hop,
				  rinfo->dist, ifindex);
      }

  /* Free old table and replace it with the new one. */
  for (rn = route_top (olsr->table); rn; rn = route_next (rn))
    if ((rinfo = (struct olsr_route*)rn->info) != NULL)
	{
	  XFREE (MTYPE_OLSR_ROUTE, rinfo);

	  rn->info = NULL;
	  route_unlock_node (rn);
	}

  route_table_finish (olsr->table);

  olsr->table = table;

  return 0;
}
