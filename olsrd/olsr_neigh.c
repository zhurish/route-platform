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
#include "if.h"
#include "memory.h"
#include "stream.h"
#include "log.h"
#include "zclient.h"

#include "olsrd/olsrd.h"
#include "olsrd/olsr_packet.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_dup.h"
#include "olsrd/olsr_mpr.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_route.h"
#include "olsrd/olsr_debug.h"


struct olsr_neigh *
olsr_neigh_lookup (struct olsr *olsr, struct in_addr *addr)
{
  struct listnode *node;
  
  for (node = listhead (olsr->neighset); node; nextnode (node))
    {
      struct olsr_neigh *on = getdata (node);

      if (memcmp (&on->main_addr, addr, sizeof (struct in_addr)) == 0)
	return on;
    }

  return NULL;
}

struct olsr_neigh *
olsr_neigh_add_new (struct olsr *olsr, struct olsr_link *ol,
		    struct olsr_header *oh, struct olsr_hello_header *ohh)
{
  struct olsr_neigh *on = XCALLOC (MTYPE_OLSR_NEIGH, sizeof (struct olsr_neigh));

  if (IS_DEBUG_EVENT (NEIGH))
    zlog_debug ("neighb %s added.", inet_ntoa (oh->oaddr));
  
  memcpy (&on->main_addr, &oh->oaddr, 4);
  on->will = ohh->will;
  
  on->assoc_links = list_new ();
  listnode_add (on->assoc_links, ol);
  olsr_neigh_status_update (on);

  listnode_add (olsr->neighset, on);
  on->node = olsr->neighset->tail;
  
  on->olsr = olsr;
  
  olsr_mpr_update (olsr);
  olsr_rt_update (olsr);

  return on;
}

struct olsr_neigh *
olsr_neigh_get (struct olsr *olsr, struct olsr_link *ol,
		    struct olsr_header *oh, struct olsr_hello_header *ohh)
{
  struct olsr_neigh *on;

  on = olsr_neigh_lookup (olsr, &oh->oaddr);

  if (on != NULL)
    {
      listnode_add (on->assoc_links, ol);
      olsr_neigh_status_update (on);
      return on;
    }
  else
    return olsr_neigh_add_new (olsr, ol, oh, ohh);
}


void
olsr_neigh_status_update (struct olsr_neigh *on)
{
  struct listnode *node;
  struct olsr_link *ol;
  u_char old_status = on->status;
  

  for (node = listhead (on->assoc_links); node; nextnode (node))
    {
      ol = getdata (node);
      
      if (ol->sym_expired == FALSE)
	{
	  on->status = OLSR_NEIGH_SYM;

	  if (old_status != on->status)
	    {
	      olsr_mpr_update (on->olsr);
	      olsr_rt_update (on->olsr);
	    }
	  
	  return;
	}
    }
  on->status = OLSR_NEIGH_NOT;

  if (old_status != on->status)
    {
      olsr_mpr_update (on->olsr);
      olsr_rt_update (on->olsr);
    }
}


void olsr_neigh_link_del (struct olsr *olsr, struct olsr_neigh *on, 
			  struct olsr_link *ol)
{
  
  listnode_delete (on->assoc_links, ol);
  

  if (list_isempty (on->assoc_links))
    {
      if (IS_DEBUG_EVENT (NEIGH))
	zlog_debug ("neighb %s deleted.", inet_ntoa (on->main_addr));      
      list_free (on->assoc_links);

      /* RFC 3626 8.5.
	 In case of neighbor loss, all 2-hop tuples with
	 N_neighbor_main_addr == Main Address of the neighbor MUST be
	 deleted.
      */
      olsr_hop2_del_all (olsr, &on->main_addr);

      list_delete_node (olsr->neighset, on->node);
      
      XFREE (MTYPE_OLSR_NEIGH, on);
    }

  olsr_mpr_update (olsr);
  olsr_rt_update (olsr);
}

/*
 * Multiple Interface Association functions.
 */
int
olsr_mid_del (struct thread *thread)
{
  struct olsr_mid *mid = THREAD_ARG (thread);
  struct listnode *node = mid->node;
  struct olsr *olsr = mid->olsr;
  
  XFREE (MTYPE_OLSR_MID, mid);

  list_delete_node (olsr->midset, node);

  /* Update mpr and route tables. */
  olsr_mpr_update (olsr);
  olsr_rt_update (olsr);

  return 0;
}

void
olsr_mid_add (struct olsr *olsr, float vtime,
	      struct in_addr *addr, struct in_addr *main_addr)
{
  struct olsr_mid *mid = XCALLOC (MTYPE_OLSR_MID, sizeof (struct olsr_mid));

  memcpy (&mid->addr, addr, 4);
  memcpy (&mid->main_addr, main_addr, 4);

  listnode_add (olsr->midset, mid);
  mid->node = listtail (olsr->midset);
  mid->olsr = olsr;

  /* Not in the RFC but I think this is useful. Clean up the
   * garbage from hop2 set by removing the tuples containing fake 
   * main addresses.
   */
  olsr_hop2_delete_by_addr (olsr, addr);

  /* And update mpr and route tables. */
  olsr_mpr_update (olsr);
  olsr_rt_update (olsr);

  OLSR_TIMER_ON (mid->t_time, olsr_mid_del, mid, vtime);
}

struct olsr_mid *
olsr_mid_lookup (struct olsr *olsr, struct in_addr *addr, 
		 struct in_addr *main_addr)
{
  struct listnode *node;
  struct olsr_mid *mid;

  LIST_LOOP (olsr->midset, mid, node)
    {
      if (memcmp (&mid->addr, addr, 4) == 0 && 
	  memcmp (&mid->main_addr, main_addr, 4) == 0)
	return mid;
    }
  return NULL;
}

void
olsr_mid_update_time (struct olsr_mid *mid, float vtime)
{
  THREAD_TIMER_OFF (mid->t_time);
  OLSR_TIMER_ON (mid->t_time, olsr_mid_del, mid, vtime);
}


/*
 * 2 Hop Neighbor functions.
 */
int
olsr_hop2_del (struct thread *thread)
{
  struct olsr_hop2 *hop2 = THREAD_ARG (thread);
  struct listnode *node = hop2->node;
  struct olsr *olsr = hop2->olsr;
  
  XFREE (MTYPE_OLSR_HOP2, hop2);

  list_delete_node (olsr->n2hopset, node);

  /* 2 hop neighborhood change detected. */
  olsr_mpr_update (olsr);
  olsr_rt_update (olsr);

  return 0;
}

void
olsr_hop2_del_all (struct olsr *olsr, struct in_addr *neigh_addr)
{
  struct olsr_hop2 *hop2;
  struct listnode *node, *next;
  int deleted = FALSE;

  for (node = listhead (olsr->n2hopset); node; node = next)
    {
      next = node->next;
      hop2 = getdata (node);

      if (memcmp (&hop2->neigh_addr, neigh_addr, 4) == 0)
	{
	  deleted = TRUE;
	  list_delete_node (olsr->n2hopset, node);
	  THREAD_TIMER_OFF (hop2->t_time);
	  XFREE (MTYPE_OLSR_HOP2, hop2);
	}
    }

  /* 2 hop neighborhood change detected. */
  if (deleted)
    {
      olsr_mpr_update (olsr);
      olsr_rt_update (olsr);
    }
}

void
olsr_hop2_delete_by_addr (struct olsr *olsr, struct in_addr *addr)
{
  struct olsr_hop2 *hop2;
  struct listnode *node, *next;
  int deleted = FALSE;

  for (node = listhead (olsr->n2hopset); node; node = next)
    {
      next = node->next;
      hop2 = getdata (node);

      if (memcmp (&hop2->hop2_addr, addr, 4) == 0)
	{
	  deleted = TRUE;
	  list_delete_node (olsr->n2hopset, node);
	  THREAD_TIMER_OFF (hop2->t_time);
	  XFREE (MTYPE_OLSR_HOP2, hop2);
	}
    }

  /* 2 hop neighborhood change detected. */
  if (deleted)
    {
      olsr_mpr_update (olsr);
      olsr_rt_update (olsr);
    }
}

void
olsr_hop2_add (struct olsr *olsr, float vtime,
	       struct in_addr *neigh_addr, struct in_addr *hop2_addr)
{
  struct olsr_hop2 *hop2 = XCALLOC (MTYPE_OLSR_HOP2, sizeof (struct olsr_hop2));

  memcpy (&hop2->neigh_addr, neigh_addr, 4);
  memcpy (&hop2->hop2_addr, hop2_addr, 4);

  listnode_add (olsr->n2hopset, hop2);
  hop2->node = olsr->n2hopset->tail;
  hop2->olsr = olsr;

  OLSR_TIMER_ON (hop2->t_time, olsr_hop2_del, hop2, vtime);

  olsr_mpr_update (olsr);
  olsr_rt_update (olsr);
}

struct olsr_hop2 *
olsr_hop2_lookup (struct olsr *olsr, struct in_addr *neigh_addr, 
		  struct in_addr *hop2_addr)
{
  struct listnode *node;
  struct olsr_hop2 *hop2;

  LIST_LOOP (olsr->n2hopset, hop2, node)
    {
      if (memcmp (&hop2->neigh_addr, neigh_addr, 4) == 0 && 
	  memcmp (&hop2->hop2_addr, hop2_addr, 4) == 0)
	return hop2;
    }
  return NULL;
}


void
olsr_hop2_update (struct olsr *olsr, struct olsr_neigh *n1hop,
		  struct in_addr *a2hop, float vtime)
{
  struct in_addr hop2_maddr;	/* 2 hop main address. */
  struct olsr_hop2 *hop2;

  olsr_mid_get_main_addr (olsr, a2hop, &hop2_maddr);


  /* RFC 3626 8.2.1
     1.1  if the main address of the 2-hop neighbor address = main
     address of the receiving node:
     silently discard the 2-hop neighbor address.
     (in other words: a node is not its own 2-hop neighbor).
  */
  if (olsr_if_lookup_by_addr (olsr, a2hop))
    return ;

  /* Also search in one hop neighborhood.  */
  if (olsr_neigh_lookup_addr (olsr, &hop2_maddr))
    return ;

  hop2 = olsr_hop2_lookup (olsr, &n1hop->main_addr, &hop2_maddr);
  if (!hop2) 
    {
      olsr_hop2_add (olsr, vtime, &n1hop->main_addr, &hop2_maddr);
      olsr_mpr_update (olsr);
      olsr_rt_update (olsr);
    }
  else
    {
      THREAD_TIMER_OFF (hop2->t_time);
      OLSR_TIMER_ON (hop2->t_time, olsr_hop2_del, hop2, vtime);
    }
}

void
olsr_hop2_remove (struct olsr *olsr, struct olsr_neigh *n1hop,
		  struct in_addr *a2hop)
{
  struct in_addr hop2_maddr;	/* 2 hop main address. */
  struct olsr_hop2 *hop2;
  struct listnode *node;

  olsr_mid_get_main_addr (olsr, a2hop, &hop2_maddr);
  hop2 = olsr_hop2_lookup (olsr, &n1hop->main_addr, &hop2_maddr); 

  if (hop2)
    {
      node = hop2->node;
      THREAD_TIMER_OFF (hop2->t_time);
      XFREE (MTYPE_OLSR_HOP2, hop2);
      list_delete_node (olsr->n2hopset, node);

      olsr_mpr_update (olsr);
      olsr_rt_update (olsr);
    }
}


/* RFC 3626 5.5.
   Given an interface address:
     1    if there exists some tuple in the interface association set
          where:

               I_iface_addr == interface address

          then the result of the main address search is the originator
          address I_main_addr of the tuple.

     2    Otherwise, the result of the main address search is the
          interface address itself.
 */
void
olsr_mid_get_main_addr (struct olsr *olsr,
			struct in_addr *addr, struct in_addr *main_addr)
{
  struct listnode *node;
  struct olsr_mid *mid;

  LIST_LOOP (olsr->midset, mid, node)
    {
      if (memcmp (&mid->addr, addr, 4) == 0)
	{
	  memcpy(main_addr, &mid->main_addr, 4);
	  return;
	}
    }
  
  memcpy(main_addr, addr, 4);
}


/*
 * Finds if the given address is the main address of one of the 
 * 1-hop symetric neigbors.
 */
struct olsr_neigh *
olsr_neigh_lookup_addr (struct olsr *olsr, struct in_addr *addr)
{
  struct listnode *node;
  struct olsr_neigh *on;

  for (node = listhead (olsr->neighset); node; nextnode (node))
    {
      on = getdata (node);

      if ((on->status == OLSR_NEIGH_SYM || on->status == OLSR_NEIGH_MPR) && 
	  memcmp (&on->main_addr, addr, 4) == 0)
	return on;
    }
  return NULL;
}




