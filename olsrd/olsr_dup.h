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

#ifndef _ZEBRA_OLSR_DUP_H
#define _ZEBRA_OLSR_DUP_H


/* RFC 3626 3.4
   "Duplicate Tuple" (D_addr, D_seq_num, D_retransmitted, D_iface_list,
   D_time), where D_addr is the originator address of the message,
   D_seq_num is the message sequence number of the message,
   D_retransmitted is a boolean indicating whether the message has been

   already retransmitted, D_iface_list is a list of the addresses of the
   interfaces on which the message has been received and D_time
   specifies the time at which a tuple expires and *MUST* be removed.
 */
struct olsr_dup
{
  struct in_addr addr;		/* D_addr. */
  u_int16_t msn;		/* D_seq_num. */
  u_char retransmitted;		/* D_retransmitted. */
  struct list *iface_list;	/* D_iface_list. */

  /* Timer. */
  struct thread *t_time;	/* D_time. */

  struct listnode *node;
  struct olsr *olsr;
};

/* Prototypes. */
struct olsr_dup *
olsr_dup_lookup (struct olsr *olsr, struct in_addr *addr, u_int16_t msn);

int
olsr_dup_has_if (struct olsr_dup *dup, struct in_addr *addr);

void
olsr_dup_add (struct olsr *olsr, struct in_addr *addr, u_int16_t msn, 
	      struct in_addr *recv_addr, u_char retransmitted);

int
olsr_default_forwarding (struct olsr *olsr, struct olsr_dup *dup,
			 struct olsr_interface *oi, struct olsr_header *oh,
			 struct in_addr *addr);



#endif /* _ZEBRA_OLSR_DUP_H */
