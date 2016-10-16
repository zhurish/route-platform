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

#ifndef _ZEBRA_OLSR_DEBUG_H
#define _ZEBRA_OLSR_DEBUG_H



/* Extern print vectors. */
extern char olsr_msg_types[][10];
extern char olsr_will_strs[][10];
extern char olsr_lt_strs[][10];
extern char olsr_nt_strs[][10];

/* Debug Flags. */
#define OLSR_DEBUG_HELLO       0x1
#define OLSR_DEBUG_TC          0x2
#define OLSR_DEBUG_MID         0x4
#define OLSR_DEBUG_HNA         0x8
#define OLSR_DEBUG_ALL         0xf

#define OLSR_DEBUG_ZEBRA       0x1
#define OLSR_DEBUG_ROUTE_TABLE 0x2
#define OLSR_DEBUG_NEIGH       0x4
#define OLSR_DEBUG_LINKS       0x8

extern unsigned int g_debug_olsr_packet;
extern unsigned int g_debug_olsr_event;

/* Macros for setting debug option. */
#define DEBUG_PACKET_ON(a)        g_debug_olsr_packet |= (a)
#define DEBUG_PACKET_OFF(a)       g_debug_olsr_packet &= ~(a)

#define DEBUG_EVENT_ON(a)        g_debug_olsr_event |= (a)
#define DEBUG_EVENT_OFF(a)       g_debug_olsr_event &= ~(a)


#define IS_DEBUG_OLSR_PACKET(a) \
       (g_debug_olsr_packet & (1 << ((a)-1)))

#define IS_DEBUG_EVENT(a) \
        (g_debug_olsr_event & OLSR_DEBUG_ ## a)

#define OLSR_MSG_STR(a) (((a) >= 1 && (a) <= 4) ? olsr_msg_types[(a) - 1] : "unknown")

#define olsr_will_str(a) ((a) <= 7 ? olsr_will_strs[(a)] : "unknown")

#define olsr_lt_str(a) ((a) <= 3 ? olsr_lt_strs[(a)] : "unknown")

#define olsr_nt_str(a) ((a) <= 2 ? olsr_nt_strs[(a)] : "unknown")

#define BOOL_STR(a) (a) ? "Yes" : "Nope"


/* Prototypes. */
void
olsr_debug_init ();

void
olsr_dump_routing_table (struct route_table *rt);


#endif /* _ZEBRA_OLSR_DEBUG_H */
