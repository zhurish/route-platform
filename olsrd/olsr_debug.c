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

#include <zebra.h>

#include "thread.h"
#include "memory.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "sockunion.h"
#include "stream.h"
#include "log.h"
#include "sockopt.h"
/* 2016年7月3日 15:30:06 zhurish: 修改头文件名，原来是md5-gnu.h */
#include "md5.h"
/* 2016年7月3日 15:30:06  zhurish: 修改头文件名，原来是md5-gnu.h */

#include "olsrd/olsrd.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_mpr.h"
#include "olsrd/olsr_dup.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_vty.h"
#include "olsrd/olsr_debug.h"
#include "olsrd/olsr_route.h"



char olsr_msg_types[][10] = {"HELLO", "TC", "MID", "HNA"};
char olsr_will_strs[][10] = {"never", "low", "2", "default",  "4", "high", 
			  "6", "always"};
char olsr_lt_strs[][10] = {"unspec", "asym", "sym", "lost"};
char olsr_nt_strs[][10] = {"not", "sym", "mpr"};


unsigned int g_debug_olsr_packet = 0;
unsigned int g_debug_olsr_event = 0;

DEFUN (debug_olsr_packet,
       debug_olsr_packet_all_cmd,
       "debug olsr packet (hello|mid|tc|hna|all)",
       DEBUG_STR
       "Optimized Link State Routing Protocol\n"
       "OLSR packets\n"
       "OLSR Hello\n"
       "OLSR Multiple Interface Descriptions\n"
       "OSPF Topollogy Control\n"
       "OSPF Host Network Associations\n"
       "OSPF all packets\n")
{
  assert (argc > 0);

  if (strncmp (argv[0], "he", 2) == 0)
    DEBUG_PACKET_ON (OLSR_DEBUG_HELLO);

  else if (strncmp (argv[0], "m", 1) == 0)
    DEBUG_PACKET_ON (OLSR_DEBUG_MID);

  else if (strncmp (argv[0], "t", 1) == 0)
    DEBUG_PACKET_ON (OLSR_DEBUG_TC);

  else if (strncmp (argv[0], "hn", 2) == 0)
    DEBUG_PACKET_ON (OLSR_DEBUG_HNA);

  else if (strncmp (argv[0], "a", 1) == 0)
    DEBUG_PACKET_ON (OLSR_DEBUG_ALL);

  return CMD_SUCCESS;
}


DEFUN (no_debug_olsr_packet,
       no_debug_olsr_packet_all_cmd,
       "no debug olsr packet (hello|mid|tc|hna|all)",
       NO_STR
       DEBUG_STR
       "Optimized Link State Routing Protocol\n"
       "OLSR packets\n"
       "OLSR Hello\n"
       "OLSR Multiple Interface Descriptions\n"
       "OSPF Topollogy Control\n"
       "OSPF Host Network Associations\n"
       "OSPF all packets\n")
{
  assert (argc > 0);

  if (strncmp (argv[0], "he", 2) == 0)
    DEBUG_PACKET_OFF (OLSR_DEBUG_HELLO);

  else if (strncmp (argv[0], "m", 1) == 0)
    DEBUG_PACKET_OFF (OLSR_DEBUG_MID);

  else if (strncmp (argv[0], "t", 1) == 0)
    DEBUG_PACKET_OFF (OLSR_DEBUG_TC);

  else if (strncmp (argv[0], "hn", 2) == 0)
    DEBUG_PACKET_OFF (OLSR_DEBUG_HNA);

  else if (strncmp (argv[0], "a", 1) == 0)
    DEBUG_PACKET_OFF (OLSR_DEBUG_ALL);

  return CMD_SUCCESS;
}


DEFUN (debug_olsr_event,
       debug_olsr_event_cmd,
       "debug olsr event (zebra|route-table|neigh|links)",
       DEBUG_STR
       "Optimized Link State Routing Protocol\n"
       "OLSR events\n"
       "Events announced by Zebra.\n"
       "Routing table changes.\n"
       "Neighborhood changes.\n"
       "Link set changes.\n")
{
  assert (argc > 0);
  
  if (strncmp (argv[0], "z", 1) == 0)
    DEBUG_EVENT_ON (OLSR_DEBUG_ZEBRA);

  else if (strncmp (argv[0], "r", 1) == 0)
    DEBUG_EVENT_ON (OLSR_DEBUG_ROUTE_TABLE);

  else if (strncmp (argv[0], "n", 1) == 0)
    DEBUG_EVENT_ON (OLSR_DEBUG_NEIGH);

  else if (strncmp (argv[0], "l", 1) == 0)
    DEBUG_EVENT_ON (OLSR_DEBUG_LINKS);
  
  return CMD_SUCCESS;
}

DEFUN (no_debug_olsr_event,
       no_debug_olsr_event_cmd,
       "no debug olsr event (zebra|route-table|neigh|links)",
       NO_STR
       DEBUG_STR
       "Optimized Link State Routing Protocol\n"
       "OLSR events\n"
       "Events announced by Zebra.\n"
       "Routing table changes.\n"
       "Neighborhood changes.\n")
{
  assert (argc > 0);
  
  if (strncmp (argv[0], "z", 1) == 0)
    DEBUG_EVENT_OFF (OLSR_DEBUG_ZEBRA);

  else if (strncmp (argv[0], "r", 1) == 0)
    DEBUG_EVENT_OFF (OLSR_DEBUG_ROUTE_TABLE);

  else if (strncmp (argv[0], "n", 1) == 0)
    DEBUG_EVENT_OFF (OLSR_DEBUG_NEIGH);

  else if (strncmp (argv[0], "l", 1) == 0)
    DEBUG_EVENT_OFF (OLSR_DEBUG_LINKS);
  
  return CMD_SUCCESS;
}


void
olsr_dump_routing_table (struct route_table *rt)
{
  struct route_node *rn;
  struct olsr_route *on;
  char addr1[17];
  char addr2[17];

  zlog_debug ("==============================================");
  zlog_debug ("Dest addr\tNext hop\tDist\tIface");

  for (rn = route_top (rt); rn; rn = route_next (rn))
    if ((on = (struct olsr_route*)rn->info) != NULL)
      {
	strcpy (addr1, inet_ntoa (rn->p.u.prefix4));
	strcpy (addr2, inet_ntoa (on->next_hop));
	zlog_debug ("%s\t%s\t%d\t%s", addr1, addr2,
		    on->dist, inet_ntoa (on->iface_addr));
      }
  zlog_debug ("==============================================");
}


void
olsr_debug_init ()
{
  /* Debug Packets. */
  install_element (ENABLE_NODE, &debug_olsr_packet_all_cmd);
  install_element (CONFIG_NODE, &debug_olsr_packet_all_cmd);

  install_element (ENABLE_NODE, &no_debug_olsr_packet_all_cmd);
  install_element (CONFIG_NODE, &no_debug_olsr_packet_all_cmd);

  /* Debug events. */
  install_element (ENABLE_NODE, &debug_olsr_event_cmd);
  install_element (CONFIG_NODE, &debug_olsr_event_cmd);

  install_element (ENABLE_NODE, &no_debug_olsr_event_cmd);
  install_element (CONFIG_NODE, &no_debug_olsr_event_cmd);

}
