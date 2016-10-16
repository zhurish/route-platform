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


#ifndef _ZEBRA_OLSR_PACKET_H
#define _ZEBRA_OLSR_PACKET_H


/* Message types. RFC 3626 18.4.*/
#define OLSR_HELLO_MSG 1
#define OLSR_TC_MSG    2
#define OLSR_MID_MSG   3
#define OLSR_HNA_MSG   4

struct olsr_oi_link
{
  u_char lt;
  u_char nt;
  struct in_addr neigh_addr;
};


/* OLSR output message. */
struct olsr_msg
{
  struct in_addr dst;		/* Detination IP. */
  struct stream *obuf;		/* Output buffer. */

  struct olsr_msg *next;	/* Next message in FIFO. */
};


/* OLSR message queue structure. One for each interface.*/
struct olsr_fifo
{
  unsigned long count;

  struct olsr_msg *head;
  struct olsr_msg *tail;
};


/* OLSR message header structure. */
struct olsr_header
{
  u_char mtype;			/* Message type. */
  float vtime;			/* Validity time. */
  u_int16_t m_size;		/* Message Size. */
  
  struct in_addr oaddr;		/* Originator Address. */
  
  u_char ttl;			/* Time To Live. */
  u_char hops;			/* Hop Count. */
  u_int16_t msn;		/* Message Sequence Number. */
};

/* OLSR HELLO packet header. */
struct olsr_hello_header
{
  float htime;			/* HELLO emission interval. */
  u_char will;		        /* The willigness of a node to forward traffic. */
};


/* OLSR HELLO Link packet header. */
struct olsr_link_header
{
  u_char lt;		        /* Link Type. */
  u_char nt;                    /* Network Type.*/
  u_int16_t lm_size;	        /* Link message size. */
};


/* 
   RFC 3632 3.4
   or such a message, a node records a
   "Duplicate Tuple" (D_addr, D_seq_num, D_retransmitted, D_iface_list,
   D_time), where D_addr is the originator address of the message,
   D_seq_num is the message sequence number of the message,
   D_retransmitted is a boolean indicating whether the message has been
   already retransmitted, D_iface_list is a list of the addresses of the
   interfaces on which the message has been received and D_time
   specifies the time at which a tuple expires and *MUST* be removed.
 */
struct duplicate
{
  struct in_addr addr;
  u_int16_t seq_num;
  u_char retransmitted;
  
  struct list *iface_list;
  struct thread *t_delete;
};

/* Macros. */
#define OLSR_FIFO_HEAD(F) ((F)->head)

#define OLSR_MSG_SIZE(M) stream_getw_from ((M)->obuf, 2)

#define OLSR_MSG_TYPE(a) (((a) >= 1 && (a) <= 4) ? olsr_msg_types[(a) - 1] : "unknown")
#define olsr_will_str(a) ((a) <= 7 ? olsr_will_strs[(a)] : "unknown")
#define olsr_lt_str(a) ((a) <= 3 ? olsr_lt_strs[(a)] : "unknown")
#define olsr_nt_str(a) ((a) <= 2 ? olsr_nt_strs[(a)] : "unknown")

/* Prototypes. */
int 
olsr_read(struct thread *thread);

int 
olsr_write (struct thread *thread);

void
olsr_forward (struct olsr *olsr, struct olsr_header *oh, 
	      struct stream *stream, u_int16_t msg_off);

int 
olsr_hello_timer(struct thread *thread);

int 
olsr_mid_timer (struct thread *thread);

struct olsr_fifo*
olsr_fifo_new ();

void
olsr_msg_fifo_push (struct olsr_fifo *fifo, struct olsr_msg *msg);

struct olsr_msg *
olsr_fifo_pop (struct olsr_fifo *fifo);

void
olsr_fifo_clean (struct olsr_fifo *fifo);

void
olsr_fifo_free (struct olsr_fifo *fifo);

int 
olsr_tc_timer (struct thread *thread);

#endif /* _ZEBRA_OLSR_PACKET_H */
