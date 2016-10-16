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

#ifndef _ZEBRA_OLSR_LINKSET_H
#define _ZEBRA_OLSR_LINKSET_H

#include <zebra.h>

#include "olsrd/olsrd.h"
#include "olsrd/olsr_packet.h"

#define OLSR_LINK_UNSPEC     0
#define OLSR_LINK_ASYM       1
#define OLSR_LINK_SYM        2
#define OLSR_LINK_LOST       3

#define OLSR_LINK_IMPOSIBLE  4 	/* Helpful in implementation. */

/*
  RFC 3632 4.2.1 Link Set

   A node records a set of "Link Tuples" (L_local_iface_addr,
   L_neighbor_iface_addr, L_SYM_time, L_ASYM_time, L_time).
   L_local_iface_addr is the interface address of the local node (i.e.,
   one endpoint of the link), L_neighbor_iface_addr is the interface
   address of the neighbor node (i.e., the other endpoint of the link),
   L_SYM_time is the time until which the link is considered symmetric,
   L_ASYM_time is the time until which the neighbor interface is
   considered heard, and L_time specifies the time at which this record
   expires and *MUST* be removed.  When L_SYM_time and L_ASYM_time are
   expired, the link is considered lost.
*/
struct olsr_link
{
  struct olsr_interface *oi;     /* Associated interface. */

  /* Link. */
  struct in_addr neigh_addr;
	
  u_char sym_expired;
  u_char asym_expired;

  struct olsr_neigh *neigh;	/* Associated neighbor. */
	
  /* Timers. */
  struct thread *t_sym_time;
  struct thread *t_asym_time;
  struct thread *t_time;
  
  struct listnode *node;
};


/* Prototypes. */
struct olsr_link * 
olsr_linkset_add (struct olsr *olsr, struct olsr_interface *oi,
		  struct ip *iph, struct olsr_header *oh,
		  struct olsr_hello_header *ohh);

struct olsr_link *
olsr_linkset_search (struct olsr_interface *oi, struct in_addr *ip_src);

int 
olsr_linkset_del (struct thread *thread);

void 
olsr_linkset_clean (struct olsr_interface *oi);

void 
olsr_linkset_update_asym (struct olsr_link *ol, struct olsr_header *oh);

void 
olsr_linkset_update_time (struct olsr_link *ol);

void 
olsr_linkset_update_sym_time (struct olsr *olsr, struct olsr_link *ol, 
			      struct olsr_link_header *olh, struct olsr_header * oh);




#endif /* _ZEBRA_OLSR_LINKSET_H */
