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

#ifndef _ZEBRA_OLSRD_ZEBRA_H
#define _ZEBRA_OLSRD_ZEBRA_H

void 
olsr_zebra_init();

void
olsr_zebra_ipv4_add (struct prefix_ipv4 *p, struct in_addr *nexthop,
		     u_int32_t metric, u_int32_t ifindex);


void
olsr_zebra_ipv4_delete (struct prefix_ipv4 *p, struct in_addr *nexthop, 
			u_int32_t metric);


#endif /* _ZEBRA_OLSRD_ZEBRA_H */
