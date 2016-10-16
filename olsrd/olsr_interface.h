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

#ifndef _ZEBRA_OLSR_INTERFACE_H
#define _ZEBRA_OLSR_INTERFACE_H

#include <zebra.h>

#include "thread.h"
#include "vty.h"
#include "command.h"
#include "linklist.h"
#include "prefix.h"
#include "table.h"
#include "if.h"
#include "memory.h"
#include "stream.h"
#include "log.h"
#include "zclient.h"

#include "olsrd/olsrd.h"
#include "olsrd/olsr_packet.h"

/* OLSR interface structure. */
struct olsr_interface
{
  /* Parent olsr instance. */
  struct olsr *olsr;

  /* Interface data from zebra. */
  struct interface *ifp;

  /* Packet send buffer. */
  struct olsr_fifo *fifo;

  /* Output socket. */
  int sock;

  struct prefix *address;	/* Interface prefix. */
  struct connected *connected;	/* Pointer to conected. */

  u_int16_t psn;		/* Packet Sequence Number. */

  /* Links connected to this interface. */
  struct list *linkset;

  /* Threads. */
  struct thread *t_hello;	/* Timer. */
  struct thread *t_mid;	        /* Timer. */
  struct thread *t_tc;	        /* Timer. */

  struct thread *t_write;	/* Write to output socket. */
};

/* OLSR Interface specific parameters. */
struct olsr_if_info
{
  float hello_interval;
  float mid_interval; 
  float tc_interval; 
};

/* Macros. */
#define OLSR_IF_BROADCAST(o) ((o)->connected->destination->u.prefix4)

#define OLSR_IF_ADDR(o) ((o)->address->u.prefix4)

#define OLSR_IF_HELLO(o) ((struct olsr_if_info*)(o)->ifp->info)->hello_interval
#define OLSR_IF_MID(o) ((struct olsr_if_info*)(o)->ifp->info)->mid_interval
#define OLSR_IF_TC(o) ((struct olsr_if_info*)(o)->ifp->info)->tc_interval

/* Prototypes. */
struct olsr_interface *
olsr_if_lookup_by_addr (struct olsr *olsr, struct in_addr *addr);

int 
olsr_if_check_address (struct in_addr addr);

int olsr_if_up (struct olsr_interface *oi);
struct olsr_interface* olsr_if_new (struct olsr *olsr, struct interface *ifp,
				    struct prefix *p);
struct olsr_interface *
olsr_oi_lookup_by_ifp (struct olsr *olsr, struct interface *ifp);

void
olsr_if_free (struct olsr_interface *oi);

struct olsr_interface *
olsr_if_is_configured (struct olsr *olsr, struct prefix *address);

int 
olsr_get_ifindex_by_addr (struct olsr *olsr, struct in_addr *addr);

void olsr_if_init ();

#endif /* _ZEBRA_OLSR_INTERFACE_H */
