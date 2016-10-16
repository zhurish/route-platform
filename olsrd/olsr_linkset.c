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
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_debug.h"

/* Why aren't this in thread.h? */
#define THREAD_SANDS(X) ((X)->u.sands)
#define THREAD_TIMER_MSEC_ON(master,thread,func,arg,time) \
  do { \
    if (! thread) \
      thread = thread_add_timer_msec (master, func, arg, time); \
  } while (0)


struct olsr_link * 
olsr_linkset_add (struct olsr *olsr, struct olsr_interface *oi,
		  struct ip *iph, struct olsr_header *oh,
		  struct olsr_hello_header *ohh)
{
  struct olsr_link *ol = XCALLOC (MTYPE_OLSR_LINK, sizeof (struct olsr_link));
  char s_addr[17];

  if (IS_DEBUG_EVENT (LINKS))
    {
      strcpy (s_addr, inet_ntoa (iph->ip_src));
      zlog_debug ("linkset add (%s - %s)",
		  inet_ntoa (oi->connected->address->u.prefix4), 
		  s_addr);
    }

  ol->oi = oi;
  
  memcpy (&ol->neigh_addr, &iph->ip_src, sizeof(struct in_addr));
  
  ol->sym_expired = TRUE;
  ol->asym_expired = TRUE;

  ol->t_sym_time = NULL;
  ol->t_asym_time = NULL;

  listnode_add (oi->linkset, ol);
  
  ol->node = listtail (oi->linkset);
  ol->neigh = olsr_neigh_get (olsr, ol, oh, ohh);
  
  OLSR_TIMER_ON (ol->t_time, olsr_linkset_del, ol, oh->vtime);

  return ol;
}

int 
olsr_linkset_del (struct thread *thread)
{
  struct olsr_link *ol;
  struct olsr_interface *oi;
  char s_addr[17];

  ol = THREAD_ARG (thread);
  oi = ol->oi;

  if (IS_DEBUG_EVENT (LINKS))
    {
      strcpy (s_addr, inet_ntoa (ol->neigh_addr));
      zlog_debug ("linkset del (%s - %s)",
		  inet_ntoa (oi->connected->address->u.prefix4), 
		  s_addr);
    }

  THREAD_TIMER_OFF (ol->t_sym_time);
  THREAD_TIMER_OFF (ol->t_asym_time);

  ol->t_time = NULL;
  
  olsr_neigh_link_del (oi->olsr, ol->neigh, ol);

  list_delete_node (oi->linkset, ol->node);

  XFREE (MTYPE_OLSR_LINK, ol);

  return 0;
}

void 
olsr_linkset_clean (struct olsr_interface *oi)
{
  struct olsr_link *ol;
  struct listnode *node, *next;

  for (node = listhead (oi->linkset); node; node = next)
    {
      next = node->next;
      ol = getdata (node);
      if (ol)
	{
	  THREAD_TIMER_OFF (ol->t_sym_time);
	  THREAD_TIMER_OFF (ol->t_asym_time);
	  THREAD_TIMER_OFF (ol->t_time);

	  olsr_neigh_link_del (oi->olsr, ol->neigh, ol);

	  list_delete_node (oi->linkset, ol->node);      

	  XFREE (MTYPE_OLSR_LINK, ol);
	}
    }
}

struct olsr_link *
olsr_linkset_search (struct olsr_interface *oi, struct in_addr *ip_src)
{
  struct listnode *node;
  
  for (node = listhead (oi->linkset); node; nextnode (node))
    {
      struct olsr_link *ol = getdata (node);

      if (memcmp (ip_src, &ol->neigh_addr, sizeof (struct in_addr)) == 0)
	return ol;
    }
  
  return NULL;
}

int
olsr_asym_expired (struct thread *thread)
{
  struct olsr_link *ol;

  ol = THREAD_ARG (thread);
  ol->asym_expired = TRUE;
  ol->t_asym_time = NULL;

  return 0;
}

int
olsr_sym_expired (struct thread *thread)
{
  struct olsr_link *ol;

  ol = THREAD_ARG (thread);
  ol->sym_expired = TRUE;
  ol->t_sym_time = NULL;

  olsr_neigh_status_update (ol->neigh);

  return 0;
}


/*
  RFC 3626 7.1.

   2    The tuple (existing or new) with:
               L_neighbor_iface_addr == Source Address
          is then modified as follows:

   2.1  L_ASYM_time = current time + validity time;
 */
void 
olsr_linkset_update_asym (struct olsr_link *ol, struct olsr_header *oh)
{

  
  THREAD_TIMER_OFF (ol->t_asym_time);
  ol->asym_expired = FALSE;

  OLSR_TIMER_ON (ol->t_asym_time, olsr_asym_expired, ol, oh->vtime);

  olsr_neigh_status_update (ol->neigh);
}

/*
  RFC 3626 7.1.

  2.2  if the node finds the address of the interface which
       received the HELLO message among the addresses listed in
       the link message then the tuple is modified as follows:

  2.2.1  if Link Type is equal to LOST_LINK then

            L_SYM_time = current time - 1 (i.e., expired)

  2.2.2   else if Link Type is equal to SYM_LINK or ASYM_LINK
            then
	    
            L_SYM_time = current time + validity time,
            L_time     = L_SYM_time + NEIGHB_HOLD_TIME
 */
void 
olsr_linkset_update_sym_time (struct olsr *olsr, struct olsr_link *ol, 
			      struct olsr_link_header *olh, struct olsr_header * oh)
{

  if (olh->lt == OLSR_LINK_LOST)
    {
      ol->sym_expired = TRUE;
      THREAD_TIMER_OFF (ol->t_sym_time);
      
      olsr_neigh_status_update (ol->neigh);
    }
  else if (olh->lt == OLSR_LINK_ASYM || olh->lt == OLSR_LINK_SYM)
    {
      if (!ol->sym_expired)
	THREAD_TIMER_OFF (ol->t_sym_time);
      
      ol->sym_expired = FALSE;
      OLSR_TIMER_ON (ol->t_sym_time, olsr_sym_expired, ol, oh->vtime);

      THREAD_TIMER_OFF (ol->t_time);
      OLSR_TIMER_ON (ol->t_time, olsr_linkset_del, ol, 
		     oh->vtime + olsr->neighb_hold_time);

    }
  
  olsr_neigh_status_update (ol->neigh);

}


/*
  RFC 3626 7.1.

  2.3  L_time = max(L_time, L_ASYM_time)
 */
void 
olsr_linkset_update_time (struct olsr_link *ol) 
{
  struct timeval now;
  struct timeval relative;
    

  if (tv_cmp (THREAD_SANDS (ol->t_time), THREAD_SANDS (ol->t_asym_time)) < 0)
    {
      THREAD_TIMER_OFF (ol->t_time);

      gettimeofday (&now, NULL);
      relative = tv_sub (THREAD_SANDS (ol->t_asym_time), now);

      THREAD_TIMER_MSEC_ON (olm->master, ol->t_time,
			    olsr_linkset_del, ol, TV_2_MSECS (relative));
    }
}
