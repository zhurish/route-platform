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

#ifndef _ZEBRA_OLSR_ROUTE_H
#define _ZEBRA_OLSR_ROUTE_H


/* RFC 3626 4.4.
   Thus, for each destination in the network, at least one "Topology
   Tuple" (T_dest_addr, T_last_addr, T_seq, T_time) is recorded.
   T_dest_addr is the main address of a node, which may be reached in
   one hop from the node with the main address T_last_addr.  Typically,
   T_last_addr is a MPR of T_dest_addr.  T_seq is a sequence number, and
   T_time specifies the time at which this tuple expires and *MUST* be
   removed.
*/
struct olsr_top
{
  struct in_addr dest_addr;
  struct in_addr last_addr;

  u_int16_t seq;	

  struct thread *t_time;
  struct listnode *node;
  struct olsr *olsr;
};


/* RF3626 10.
   Each entry in the table consists of R_dest_addr, R_next_addr, R_dist,
   and R_iface_addr.  Such entry specifies that the node identified by
   R_dest_addr is estimated to be R_dist hops away from the local node,
   that the symmetric neighbor node with interface address R_next_addr
   is the next hop node in the route to R_dest_addr, and that this
   symmetric neighbor node is reachable through the local interface with
   the address R_iface_addr.
*/
/* Below is the info structure from the route nodes stored in 
   the route radix tree table. R_dest_addres is in the prefix field from
   route_node (lib/table.h).
*/
struct olsr_route
{
  struct in_addr next_hop;
  int dist;
  struct in_addr iface_addr;
};

/* Prototypes. */

/* Topology set functions. */
int
olsr_top_exists_newer (struct olsr *olsr, struct in_addr *last_addr, 
		       u_int16_t ansn);

void 
olsr_top_cleanup_older (struct olsr *olsr, struct in_addr *last_addr, 
			u_int16_t ansn);

int
olsr_top_del (struct thread *thread);

void
olsr_top_add (struct olsr *olsr, float vtime, u_int16_t ansn, 
	      struct in_addr *dest_addr, struct in_addr *last_addr);

struct olsr_top *
olsr_top_lookup (struct olsr *olsr, struct in_addr *dest_addr,
		 struct in_addr *last_addr);

void
olsr_top_update_time (struct olsr_top *top, float vtime);

/* Advertised set functions. */
void
olsr_top_add_adv (struct olsr *olsr, struct olsr_neigh *on);

void
olsr_top_rem_adv (struct olsr *olsr, struct olsr_neigh *on);


/* Route update functions. */
struct route_table *
olsr_route_calculate (struct olsr *olsr);

void 
olsr_rt_update (struct olsr *olsr);

int
olsr_rt_timer (struct thread *thread);

void 
olsr_terminate();

#endif /* _ZEBRA_OLSR_ROUTE_H */
