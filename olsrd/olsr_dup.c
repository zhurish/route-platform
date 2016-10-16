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
#include <math.h>

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

#include "olsrd/olsrd.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_dup.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_vty.h"


/*
 * Search for a duplicate message with a given address and sequence 
 * number.
 */
struct olsr_dup *
olsr_dup_lookup (struct olsr *olsr, struct in_addr *addr, u_int16_t msn)
{
  struct listnode *node;
  struct olsr_dup *dup;

  for (node = listhead (olsr->dupset); node; nextnode (node))
    {
      dup = getdata (node);

      if (memcmp (&dup->addr, addr, 4) == 0 && dup->msn == msn)
	return dup;
    }
  return NULL;
}

/*
 * Return TRUE if addr is in dup->D_iface_list.
 */
int
olsr_dup_has_if (struct olsr_dup *dup, struct in_addr *addr)
{
  struct listnode *node;

  for (node = listhead (dup->iface_list); node; nextnode (node))
    if (memcmp (getdata (node), addr, 4) == 0)
      return TRUE;

  return FALSE;
}

/*
 * Add new duplicate data.
 */
void
olsr_dup_add (struct olsr *olsr, struct in_addr *addr, u_int16_t msn, 
	      struct in_addr *recv_addr, u_char retransmitted)
{
  struct olsr_dup *dup = XCALLOC (MTYPE_OLSR_DUP, sizeof (struct olsr_dup));
  struct in_addr *r_addr = XCALLOC (MTYPE_PREFIX_IPV4, sizeof (struct in_addr));
  memcpy (&dup->addr, addr, 4);
  dup->msn = msn;
  dup->retransmitted = retransmitted;
  
  dup->iface_list = list_new ();
  memcpy (r_addr, recv_addr, 4);
  listnode_add (dup->iface_list, r_addr);

  listnode_add (olsr->dupset, dup);
  dup->node = olsr->dupset->tail;
  dup->olsr = olsr;
}

/*
 * Delete duplicate node.
 */
int
olsr_dup_del (struct thread *thread)
{
  struct olsr_dup *dup;
  struct listnode *node, *n1, *n2;
  struct olsr *olsr;

  dup = THREAD_ARG (thread);
  node = dup->node;
  olsr = dup->olsr;
/* 2016年7月3日 15:30:45 zhurish: 修改链表操作 */
  #if 1
  for (n1 = listhead(dup->iface_list); n1; n1 = n2)
    {
      n2 = n1->next;
      if(n1->data)
        XFREE (MTYPE_PREFIX_IPV4,  n1->data); //getdata
      list_delete_node (dup->iface_list, n1);
    }
    #else
  for (n1 = listhead(dup->iface_list); n1; n1 = n2)
    {
      n2 = n1->next;

      XFREE (MTYPE_PREFIX_IPV4, getdata (n1));
      list_delete_node (dup->iface_list, n1);
    }
#endif
/* 2016年7月3日 15:30:45  zhurish: 修改链表操作 */
  list_free (dup->iface_list);

  XFREE (MTYPE_OLSR_DUP, dup);
  list_delete_node (olsr->dupset, node);
  
  return 0;
}

/*
 * Implements "Default Forwarding Algorithm" as in RFC 3626 3.4.1
 */
int
olsr_default_forwarding (struct olsr *olsr, struct olsr_dup *dup,
			 struct olsr_interface *oi, struct olsr_header *oh,
			 struct in_addr *addr)
{
  int ret;
  struct in_addr main_addr, *paddr;
  struct olsr_neigh *on;
  olsr_mid_get_main_addr (olsr, addr, &main_addr);

  /* 1    If the sender interface address of the message is not detected
          to be in the symmetric 1-hop neighborhood of the node, the
          forwarding algorithm MUST silently stop here (and the message
          MUST NOT be forwarded).
  */
  if ((on = olsr_neigh_lookup_addr (olsr, &main_addr)) == NULL)
    return FALSE;

  /* 2    If there exists a tuple in the duplicate set where:

               D_addr    == Originator Address
               D_seq_num == Message Sequence Number

          Then the message will be further considered for forwarding if
          and only if:

               D_retransmitted is false, AND
               the (address of the) interface which received the message
               is not included among the addresses in D_iface_list
  */
  if (dup && (dup->retransmitted || olsr_dup_has_if (dup, &OLSR_IF_ADDR (oi))))
    return FALSE;
  
  /*  3   Otherwise, if such an entry doesn't exist, the message is
          further considered for forwarding.
	  
      4   If the sender interface address is an interface address of a
          MPR selector of this node and if the time to live of the
          message is greater than '1', the message MUST be retransmitted
          (as described later in steps 6 to 8).
  */
  ret = on->is_mprs && oh->ttl > 1;

  /* 5    Update Duplicate set. */
  if (dup)
    {
      dup->retransmitted = ret;

      THREAD_TIMER_OFF (dup->t_time);
      OLSR_TIMER_ON (dup->t_time, olsr_dup_del, dup, olsr->dup_hold_time);
      
      paddr = XCALLOC (MTYPE_PREFIX_IPV4, sizeof (struct in_addr));
      memcpy (paddr, &OLSR_IF_ADDR (oi), sizeof (struct in_addr));
      listnode_add (dup->iface_list, paddr);
    }
  else
    olsr_dup_add (olsr, &oh->oaddr, oh->msn, &OLSR_IF_ADDR (oi), ret);

  return ret;
}

