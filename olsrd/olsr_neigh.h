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

#ifndef _ZEBRA_OLSR_NEIGH_H
#define _ZEBRA_OLSR_NEIGH_H

#include "olsrd/olsr_linkset.h"

#define OLSR_NEIGH_NOT       0
#define OLSR_NEIGH_SYM       1
#define OLSR_NEIGH_MPR       2

#define OLSR_NEIGH_IMPOSSIBLE 3 /* Helpful in implementation. */

/* RFC 3626 4.3.1
 A node records a set of "neighbor tuples" (N_neighbor_main_addr,
 N_status, N_willingness), describing neighbors.  N_neighbor_main_addr
 is the main address of a neighbor, N_status specifies if the node is
 NOT_SYM or SYM.  N_willingness in an integer between 0 and 7, and
 specifies the node's willingness to carry traffic on behalf of other
 nodes.
*/
struct olsr_neigh
{
  struct in_addr main_addr;
  u_char status;
  u_char will;

  u_char is_mpr;

  u_char is_mprs;
  struct thread *t_mprs;		/* Expire time for mprs. */

  struct list *assoc_links;
  struct listnode *node;

  struct olsr *olsr;
};

/* RFC 3626 4.3.2
   A node records a set of "2-hop tuples" (N_neighbor_main_addr,
   N_2hop_addr, N_time), describing symmetric (and, since MPR links by
   definition are also symmetric, thereby also MPR) links between its
   neighbors and the symmetric 2-hop neighborhood.  N_neighbor_main_addr
   is the main address of a neighbor, N_2hop_addr is the main address of
   a 2-hop neighbor with a symmetric link to N_neighbor_main_addr, and
   N_time specifies the time at which the tuple expires and *MUST* be
   removed.
 */
struct olsr_hop2
{
  struct in_addr neigh_addr;
  struct in_addr hop2_addr;
  
  struct thread *t_time;
  struct listnode *node;
  struct olsr *olsr;
};


/* RFC 3626 4.1
   (I_iface_addr, I_main_addr, I_time) are recorded.  I_iface_addr is an
   interface address of a node, I_main_addr is the main address of this
   node.  I_time specifies the time at which this tuple expires and
   *MUST* be removed.
 */
struct olsr_mid
{
  struct in_addr addr;
  struct in_addr main_addr;

  struct thread *t_time;
  struct listnode *node;
  struct olsr *olsr;
};


/* Prototypes. */
struct olsr_neigh *
olsr_neigh_get (struct olsr *olsr, struct olsr_link *ol,
		struct olsr_header *oh, struct olsr_hello_header *ohh);
struct olsr_neigh *
olsr_neigh_add_new (struct olsr *olsr, struct olsr_link *ol,
		    struct olsr_header *oh, struct olsr_hello_header *ohh);
void
olsr_neigh_status_update (struct olsr_neigh *on);
void olsr_neigh_link_del (struct olsr *olsr, struct olsr_neigh *on, 
			  struct olsr_link *ol);

/* Mid functions. */
struct olsr_mid *
olsr_mid_lookup (struct olsr *olsr, struct in_addr *addr, 
		 struct in_addr *main_addr);
void
olsr_mid_add (struct olsr *olsr, float vtime,
	      struct in_addr *addr, struct in_addr *main_addr);

void
olsr_mid_update_time (struct olsr_mid *mid, float vtime);

void
olsr_mid_get_main_addr (struct olsr *olsr,
			struct in_addr *addr, struct in_addr *main_addr);

/* 2 hop functions. */
void
olsr_hop2_remove (struct olsr *olsr, struct olsr_neigh *n1hop,
		  struct in_addr *a2hop);
void
olsr_hop2_del_all (struct olsr *olsr, struct in_addr *neigh_addr);

void
olsr_hop2_delete_by_addr (struct olsr *olsr, struct in_addr *addr);

void
olsr_hop2_update (struct olsr *olsr, struct olsr_neigh *n1hop,
		  struct in_addr *a2hop, float vtime);

struct olsr_neigh *
olsr_neigh_lookup_addr (struct olsr *olsr, struct in_addr *addr);

struct olsr_neigh *
olsr_mprs_lookup_addr (struct olsr *olsr, struct in_addr *addr);



#endif /* _ZEBRA_OLSR_NEIGH_H */
