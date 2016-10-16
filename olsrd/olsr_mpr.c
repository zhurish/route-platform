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
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_packet.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_dup.h"
#include "olsrd/olsr_mpr.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_route.h"
#include "olsrd/olsr_debug.h"

int
olsr_is_mprs_reset (struct thread *thread)
{
  struct olsr_neigh *on = THREAD_ARG (thread);

  on->t_mprs = NULL;
  on->is_mprs = FALSE;
  
  /* Stop advertising this neighbor. */
  olsr_top_rem_adv (on->olsr, on);

  return 0;
}

void
olsr_is_mprs_set (struct olsr_neigh *on, float vtime)
{
  on->is_mprs = TRUE;

  THREAD_TIMER_OFF (on->t_mprs);
  OLSR_TIMER_ON (on->t_mprs, olsr_is_mprs_reset, on, vtime);

  /* Advertise this neighbor.. */
  olsr_top_add_adv (on->olsr, on);
}


int 
olsr_is_neigh_of_if (struct olsr_neigh *on, struct olsr_interface *oi)
{
  struct listnode *node;
  struct olsr_link *link;

  LIST_LOOP (on->assoc_links, link, node)
    if (link->oi == oi)
      return TRUE;
  return FALSE;
}

struct list *
olsr_mpr_get_if_neigh (struct olsr *olsr, struct olsr_interface *oi)
{
  struct list *N;
  struct listnode *node;
  struct olsr_neigh *on;
  struct olsr_1N *neigh;

  N = list_new ();

  LIST_LOOP (olsr->neighset, on, node)
    {
      if (on->status == OLSR_NEIGH_SYM && on->will != OLSR_WILL_NEVER &&
	  olsr_is_neigh_of_if (on, oi))
	{
	  neigh = XCALLOC (MTYPE_OLSR_1N, sizeof (struct olsr_1N));
	  neigh->on = on;
	  neigh->hop2lst = list_new ();
	  listnode_add (N, neigh);
	  neigh->node = listtail (N);
	}
    }
  return N;
}

struct olsr_2N *
olsr_mpr_2hop_get (struct list *N2, struct in_addr *addr)
{
  struct olsr_2N *n2hop;
  struct listnode *node;

  LIST_LOOP (N2, n2hop, node)
    if (memcmp (&n2hop->addr, addr, 4) == 0)
      return n2hop;

  n2hop = XCALLOC (MTYPE_OLSR_2N, sizeof (struct olsr_2N));
  memcpy (&n2hop->addr, addr, 4);
  n2hop->hop1lst = list_new ();

  return n2hop;
}


struct list *
olsr_mpr_get_2hop_neigh (struct olsr *olsr, struct list *N)
{
  struct listnode *n1, *n2;
  struct olsr_1N *neigh;
  struct olsr_2N *n2hop;
  struct olsr_hop2 *hop2;
  struct list *N2;
  int found;
  
  N2 = list_new ();
  LIST_LOOP (olsr->n2hopset, hop2, n1)
    {
      /* Exclude 1 hop sym neighborhood. */
      if (olsr_neigh_lookup_addr (olsr, &hop2->hop2_addr))
	continue;

      
      n2hop = olsr_mpr_2hop_get (N2, &hop2->hop2_addr);

      found = FALSE;
      
      /* Verify that is reachable from N. */
      LIST_LOOP (N, neigh, n2)
	{
	  if (memcmp (&neigh->on->main_addr, &hop2->neigh_addr, 4) == 0)
	    {
	      found = TRUE;
	      
	      /* Cross references. */
	      listnode_add (neigh->hop2lst, n2hop);
	      listnode_add (n2hop->hop1lst, neigh);

	      break;
	    }
	}

      if (n2hop->lock == 0)
	{
	  if (!found)
	    XFREE (MTYPE_OLSR_2N, n2hop);
	  else
	    {
	      listnode_add (N2, n2hop);
	      n2hop->node = listtail (N2);
	      n2hop->lock = 1;
	    }
	}
    }

  return N2;
}


int
olsr_mpr_timer (struct thread *thread)
{
  struct olsr *olsr;
  struct olsr_interface *oi;
  struct olsr_neigh *on;
  struct listnode *node;

  olsr = THREAD_ARG (thread);
  olsr->t_mpr_update = NULL;

  /* Reset all `is_mpr' flags. */
  LIST_LOOP (olsr->neighset, on, node)
    on->is_mpr = FALSE;

  /* Update all interfaces mpr set. */
  LIST_LOOP (olsr->oiflist, oi, node)
    olsr_mpr_update_if (oi);

  return 0;
}

void 
olsr_mpr_update (struct olsr *olsr)
{
  OLSR_TIMER_ON (olsr->t_mpr_update, olsr_mpr_timer, olsr, 
		 olsr->mpr_update_time);
}


void 
olsr_mpr_N2_cleanup (struct list *N, struct list *N2, struct olsr_2N *n2)
{
  struct listnode *node, *next, *node1;
  struct olsr_1N *n1;


  /* First delete n2 from all references lists of n1 nodes. */
  LIST_LOOP (n2->hop1lst, n1, node1)
    for (node = listhead (n1->hop2lst); node; node = next)
      {
	next = node->next;
      
	if (n2 == getdata (node))
	  list_delete_node (n1->hop2lst, node);
      }

  /* Free memory and remove from N2 list. */
  list_free (n2->hop1lst);
  node = n2->node;
  XFREE (MTYPE_OLSR_2N, n2);

  list_delete_node (N2, node);
}

void 
olsr_mpr_N_cleanup (struct list *N, struct list *N2, struct olsr_1N *n1)
{
  struct listnode *node, *next;
  struct olsr_2N *n2;

  /* Delete all n2 nodes referenced by n1. */
  for (node = listhead (n1->hop2lst); node; node = next)
    {
      n2 = getdata (node);
      next = nextnode (node);
      
      olsr_mpr_N2_cleanup (N, N2, n2);
    }
  
  /* Mark node as MPR before removing from N list. */
  n1->on->is_mpr = TRUE;

  /* Free memory and remove from N list. */
  list_free (n1->hop2lst);
  node = n1->node;
  XFREE (MTYPE_OLSR_1N, n1);

  list_delete_node (N, node);
}

void 
olsr_mpr_N_N2_cleanup (struct list *N, struct list *N2)
{
  struct listnode *node, *next;
  struct olsr_1N *n1;

  
  for (node = listhead (N); node; node = next)
    {
      n1 = getdata (node);
      next = nextnode (node);

      if (n1->marked)
	olsr_mpr_N_cleanup (N, N2, n1);
    }
}

/*
 * Decides which of the two neighbors is preferable as an MPR.
 *
 * RFC 3626 8.3.
 * 4.2  Select as a MPR the node with highest N_willingness among
 * the nodes in N with non-zero reachability.  In case of
 * multiple choice select the node which provides
 * reachability to the maximum number of nodes in N2.  In
 * case of multiple nodes providing the same amount of
 * reachability, select the node as MPR whose D(y) is
 * greater.  Remove the nodes from N2 which are now covered
 * by a node in the MPR set.
 */
int
olsr_mpr_neigh_cmp (struct olsr_1N *n1, struct olsr_1N *n2)
{
  if (n1->on->will > n2->on->will)
    return 1;

  if (n1->on->will < n2->on->will)
    return -1;

  if (listcount (n1->hop2lst) > listcount (n2->hop2lst))
    return 1;

  if (listcount (n1->hop2lst) < listcount (n2->hop2lst))
    return -1;

  if (n1->D > n2->D)
    return 1;

  if (n1->D < n2->D)
    return -1;

  return 0;
}



/*
 * This is the key component of OLSR. It computes the MPR set using 
 * the heuristic described in RFC.
 */
void
olsr_mpr_update_if (struct olsr_interface *oi)
{
  struct olsr *olsr = oi->olsr;
  struct list *N;                        /* Neighbors of interface oi */
  struct list *N2;                       /* 2-hop neigbors reachable via nodes 
					  * from N */
  struct listnode *node1, *node2;
  struct olsr_1N *n1, *chosen;
  struct olsr_2N *n2;


  
  N = olsr_mpr_get_if_neigh (olsr, oi);
  N2 = olsr_mpr_get_2hop_neigh (olsr, N);
 
  /* Some debugging in here. */
  if (IS_DEBUG_EVENT (NEIGH))
    {
      zlog_debug ("-------------------------------------------------");
      LIST_LOOP (N, n1, node1)
	{
	  zlog_debug ("Neighbor N1: %s", inet_ntoa (n1->on->main_addr));
	  LIST_LOOP (n1->hop2lst, n2, node2)
	    zlog_debug ("N2: %s", inet_ntoa (n2->addr));
	}


      LIST_LOOP (N2, n2, node2)
	{
	  zlog_debug ("Neighbor N2: %s", inet_ntoa (n2->addr));      
	  LIST_LOOP (N, n1, node1)
	    zlog_debug ("N1: %s", inet_ntoa (n1->on->main_addr));
	}
    }

  /* RFC 3626 8.3.
     1    Start with an MPR set made of all members of N with
     N_willingness equal to WILL_ALWAYS
  */
  LIST_LOOP (N, n1, node1)
    if (n1->on->will == OLSR_WILL_ALWAYS)
      n1->marked = TRUE;

  /*
    2    Calculate D(y), where y is a member of N, for all nodes in N.
  */
  LIST_LOOP (N, n1, node1)
    n1->D = listcount (n1->hop2lst);

  /* 
     3    Add to the MPR set those nodes in N, which are the *only*
     nodes to provide reachability to a node in N2.  For example,
     if node b in N2 can be reached only through a symmetric link
     to node a in N, then add node a to the MPR set.
  */
  LIST_LOOP (N2, n2, node2)
    if (listcount (n2->hop1lst) == 1)
      {
	n1 = getdata (listhead (n2->hop1lst));
	n1->marked = TRUE;
      }  
  
  /*
    Remove the nodes from N2 which are now covered by a node in the 
    MPR set.
  */
  olsr_mpr_N_N2_cleanup (N, N2);


  /*
    4    While there exist nodes in N2 which are not covered by at
    least one node in the MPR set.
  */
  while (! list_isempty (N2))
    {
      
      /*
	4.1  For each node in N, calculate the reachability, i.e., the
	number of nodes in N2 which are not yet covered by at
	least one node in the MPR set, and which are reachable
	through this 1-hop neighbor; 

	(Done in cleanup.)
      */
      
      chosen = getdata (listhead (N));
      assert (chosen);

      LIST_LOOP (N, n1, node1)
	if (olsr_mpr_neigh_cmp(chosen, n1) < 0)
	  chosen = n1;
      
      olsr_mpr_N_cleanup (N, N2, chosen);
    }

  /* Free what remains of N and N2 lists.. */
  list_free (N2);

  LIST_LOOP (N, n1, node1)
    {
      list_free (n1->hop2lst);
      XFREE (MTYPE_OLSR_1N, n1);
    }
  list_delete_all_node (N);
  list_free (N);
}
