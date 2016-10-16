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

#include <zebra.h>

#include "memory.h"
#include "prefix.h"
#include "table.h"
#include "avl.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_route.h"

/*---------------------------------------------------------------------------
 * Function : rsvp_route_info_new
 * Input    : None.
 * Output   : Returns pointer to struct rsvp_route_info if created
 *            successfully, else returns NULL.
 * Synopsis : This function creates a struct rsvp_route_info. This function 
 *            is only internally used iin this module.
 * Callers  : rsvp_route_add in rsvp_route.c
 *-------------------------------------------------------------------------*/
struct rsvp_route_info *
rsvp_route_info_new (void)
{
   struct rsvp_route_info *rsvp_route_info;

   /* Allocate a new rsvp_route_info. */
   rsvp_route_info = XCALLOC (MTYPE_RSVP_ROUTE_INFO, 
                              sizeof (struct rsvp_route_info));
   
   return rsvp_route_info;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_route_info_delete
 * Input    : rsvp_route_info = Pointer to the struct rsvp_route_info that 
 *            needs to be freed.
 * Output   : None.
 * Synopsis : This utility function frees up a rsvp_route_info. This function
 *            is used internally only by this module.
 * Callers  : rsvp_route_delete in rsvp_route.c
 *--------------------------------------------------------------------------*/
void
rsvp_route_info_delete (struct rsvp_route_info *rsvp_route_info)
{
   /* Sanity Check.*/
   if (!rsvp_route_info)
      return;

   XFREE (MTYPE_RSVP_ROUTE_INFO, rsvp_route_info);
   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_route_table_init 
 * Input    : None
 * Output   : None
 * Synopsis : This is a Wrapper Function around route_table_init defined in 
 *            table,h/c.
 * Callers  : rsvp_init in rsvpd.c
 *--------------------------------------------------------------------------*/
void
rsvp_route_table_init ()
{
   /* Check that RSVP Main module is initialized before this.*/
   assert (rm->rsvp);
   /* Initialize the route table. */
   rm->rsvp->route_table = route_table_init ();
  
   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_route_add 
 * Input    : type = The type of the route (defined by zebra).
 *          : prefix = Pointer to struct prefix. 
 *          : ifindex = The outgoing interface index.
 *          : nexthop = Pointer to struct in_addr that defines the next hop.`
 * Output   : Returns 0 if added successfully, else returns -1.
 * Synopsis : This function adds a IPV4 Route into the Routing Database.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_route_add (int type,
                struct prefix *prefix,
                unsigned int ifindex,
                struct in_addr *nexthop)
{
   struct route_table *route_table;
   struct route_node *route_node;
   struct rsvp_route_info *rsvp_route_info;

   /* Sanity Check. */
   assert (rm->rsvp);
   /* Get the route table hook. */
   route_table = (struct route_table *)rm->rsvp->route_table;   
   /* Add the Route. */
   if ((route_node = route_node_get (route_table, prefix)) == NULL)
      return RSVP_ROUTE_ADD_FAIL;
  
   /* Create the rsvp_route_info. */
   if ((rsvp_route_info = rsvp_route_info_new ()) == NULL)
      return RSVP_ROUTE_ADD_FAIL;

   /* Fill up all rsvp_route_info parameters.*/
   rsvp_route_info->type = type;
   memcpy (&rsvp_route_info->nexthop, nexthop, sizeof (struct in_addr));
   rsvp_route_info->ifindex = ifindex;    

  /* Hook this rsvp_route_info to the route_node added.*/
   route_node->info = rsvp_route_info;

   return RSVP_ROUTE_ADD_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * Function : rsvp_route_delete
 * Input    : type = The type of route.
 *          : prefix = The prefix to be deleted.
 *          : ifindex = Iifindex of the outgoing interface.
 *          : nexthop = The nexthop address.
 * Output   : Returns 0 if the prefix deleted successfully, else return -1.
 * Synopsis : This function deletes a Route from RSVP's routing database.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_route_delete (int type,
                   struct prefix *prefix,
                   unsigned int ifindex,
                   struct in_addr *nexthop)
{
   struct route_table *route_table;
   struct route_node *route_node;
   struct rsvp_route_info *rsvp_route_info;

   /* Sanity Check. */
   assert (rm->rsvp);
   /* Get the Routing database.*/
   route_table = (struct route_table *)rm->rsvp->route_table;

   /* Lookup the route_node that matches the "exact" prefix.*/
   if ((route_node = route_node_lookup (route_table, prefix)) == NULL)
      return RSVP_ROUTE_DELETE_FAIL;

   /* Get the Route Info.*/
   rsvp_route_info = (struct rsvp_route_info *)route_node->info;  

   /* Match all the route infos.*/
   if ((rsvp_route_info->type != type) || 
       (rsvp_route_info->ifindex != ifindex) ||
       memcmp (&rsvp_route_info->nexthop, nexthop, sizeof (struct in_addr)))
      return RSVP_ROUTE_DELETE_FAIL;
       
   /* Delete the route_node from route_table.*/
   route_node_delete (route_node);
   /* Free the rsvp_route_info.*/
   rsvp_route_info_delete (rsvp_route_info);

   return RSVP_ROUTE_DELETE_SUCCESS;     
}

/*----------------------------------------------------------------------------
 * Function : rsvp_route_lookup
 * Input    : prefix = Pointer to struct prefix.
 * Output   : Returns the pointer to the struct rsvp_route_info.
 * Synopsis : This function looks up the route_info that exactly matches the
 *            input prefix. 
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
struct rsvp_route_info *
rsvp_route_lookup (struct prefix *prefix)
{
   struct route_table *route_table;
   struct route_node *route_node;

   /* Sanity Check.*/
   if (!prefix)
      return NULL;
   /* Get the route table.*/
   route_table = (struct route_table *)rm->rsvp->route_table;
   
   /* Get the exact matched route_node.*/
   if ((route_node = route_node_lookup (route_table, prefix)) == NULL) 
      return NULL;
   
   /* Return the route_info */ 
   return route_node->info; 
}

/*----------------------------------------------------------------------------
 * Function : rsvp_route_match
 * Input    : prefix = Pointer to struct prefix for which LPM search is needed.
 * Output   : None.
 * Synopsis : This function does LPM search for the input prefix on routing
 *            database maintained by RSVP-TE.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
struct rsvp_route_info *
rsvp_route_match (struct prefix *prefix)
{
   struct route_table *route_table;
   struct route_node *route_node;

   /* Get the routing database maintained by RSVP-TE.*/   
   route_table = rm->rsvp->route_table;

   if ((route_node = route_node_match (route_table, prefix)) == NULL)
      return NULL;
   
   return route_node->info;   
}

/*----------------------------------------------------------------------------
 * Function : rsvp_route_match_ipv4
 * Input    : in_addr = Pointer to struct in_addr .
 * Output   : Returns the pointer to struct rsvp_route_info if LPM match is
 *            found.
 * Synopsis : This function does LPM search for the input in_addr.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
struct rsvp_route_info *
rsvp_route_match_ipv4 (struct in_addr *addr)
{
   struct route_table *route_table;
   struct route_node *route_node;

   /* Get the route table.*/   
   route_table = (struct route_table *)rm->rsvp->route_table;

   /* Get the matched node.*/
   if ((route_node = route_node_match_ipv4 (route_table, addr)) == NULL)
      return NULL;

   return route_node->info;
}

#ifdef HAVE_IPV6
/*-----------------------------------------------------------------------------
 * Function : rsvp_route_match_ipv6.
 * Input    : addr = Pointer to struct in6_addr.
 * Output   : Returns matched struct rsvp_route_info.
 * Synopsis : This function does LPM search for IPv6 Route on the routing 
 *            database maintained by RSVP-TE.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
struct rsvp_route_info *
rsvp_route_match_ipv6 (struct in6_addr *addr)
{
   struct route_table *route_table;
   struct route_node *route_node;
   
   /* Get the Routing Table Hook.*/
   route_table = (struct route_table *)rm->rsvp->route_table;
   /* LPM search by IPV6 Addr.*/
   if ((route_node = route_node_match_ipv6 (route_table, addr)) == NULL)
      return NULL;

   /* Return the rsvp_route_info.*/
   return route_node->info;
}
#endif
