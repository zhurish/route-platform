/*
** Copyright (C) 2002-2006 Pranjal Kumar Dutta <prdutta@users.sourceforge.net>
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

/*-----------------------------------------------------------------------------
 * RSVP Best Effort Route Information.
 *---------------------------------------------------------------------------*/
struct rsvp_route_info
{
   int			type; 	 /* This Routes's Type.*/
   struct in_addr 	nexthop; /* Next IP Hop. */
   unsigned int 	ifindex; /* Outgoing Interface Index. */
   struct route_node	*node;   /* Node that baers this info. */
};

/*----------------------------------------------------------------------------
 * RSVP API Return Values.
 *--------------------------------------------------------------------------*/
#define RSVP_ROUTE_ADD_SUCCESS		0
#define RSVP_ROUTE_ADD_FAIL		-1
#define RSVP_ROUTE_DELETE_SUCCESS	0
#define RSVP_ROUTE_DELETE_FAIL		-1


/*-----------------------------------------------------------------------------
 * Protypes exported by this module.
 *---------------------------------------------------------------------------*/
void rsvp_route_table_init (void);

int rsvp_route_add (int, 
                    struct prefix *, 
                    unsigned int, 
                    struct in_addr *);

int rsvp_route_delete (int, 
                       struct prefix *, 
                       unsigned int,
                       struct in_addr *);

struct rsvp_route_info *rsvp_route_lookup (struct prefix *);
struct rsvp_route_info *rsvp_route_match (struct prefix *);
struct rsvp_route_info *rsvp_route_match_ipv4 (struct in_addr *);
#ifdef HAVE_IPV6
struct rsvp_route_info *rsvp_route_match_ipv6 (struct in6_addr *);
#endif /*HAVE_IPV6 */
