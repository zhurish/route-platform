/* Header for RSVP-TE and Zebra Intercations
**
** Copyright (C) 2003 Pranjal Kumar Dutta <prdutta@uers.sourceforge.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*---------------------------------------------------------------------------
 * Function Prototypes
 *-------------------------------------------------------------------------*/
void rsvp_zebra_init (void);
struct interface *if_lookup_by_ipv4 (struct in_addr *);
struct interface *if_lookup_by_ipv4_exact (struct in_addr *);
#ifdef HAVE_IPV6
struct interface *if_lookup_by_ipv6 (struct in6_addr *);
struct interface *if_lookup_by_ipv6_exact (struct in6_addr *);
#endif
