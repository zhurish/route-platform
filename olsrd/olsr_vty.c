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
#include "vty.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "memory.h"
#include "command.h"
#include "stream.h"
#include "sockopt.h"
#include "sockunion.h"
#include "log.h"

#include "olsrd/olsrd.h"
#include "olsrd/olsr_vty.h"
#include "olsrd/olsr_packet.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_debug.h"
#include "olsrd/olsr_route.h"
#include "olsrd/olsr_time.h"


DEFUN (router_olsr,
       router_olsr_cmd,
       "router olsr",
       "Enable a routing process\n"
       "Start OLSR configuration\n")
{
  vty->node = OLSR_NODE;
  vty->index = olsr_get();
  
  return CMD_SUCCESS;
}



struct cmd_node olsr_interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
  1 /* vtysh ? yes */
};

/* OLRSD's router node. */
struct cmd_node olsr_router_node =
{
  OLSR_NODE,
  "%s(config-router)# ",
  1 /* vtysh ? yes */
};


DEFUN (olsr_network,
       olsr_network_cmd,
       "network A.B.C.D/M",
       "Enable routing on an IP network\n"
       "OLSR network prefix\n")
{
  struct olsr *olsr = vty->index;
  struct prefix_ipv4 p;
  int ret;

  /* Get network prefix */
  VTY_GET_IPV4_PREFIX ("network prefix", p, argv[0]);

  ret = olsr_network_set (olsr, &p);
  if (ret == 0)
    {
      vty_out (vty, "There is already same network statement.%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  return CMD_SUCCESS;
}


DEFUN (show_ip_olsr_neighbor,
       show_ip_olsr_neighbor_cmd,
       "show ip olsr neighbor",
       SHOW_STR
       IP_STR
       "OLSR information\n"
       "Neighbor database.\n")
{
  struct olsr *olsr;
  struct listnode *node;
  struct olsr_neigh *neigh;

  olsr = olsr_lookup ();
  if (olsr == NULL)
    return CMD_SUCCESS;

  vty_out (vty, "MAIN ADDR\t\tSTATUS\t\tWILLINGNGESS\tMPR\tMPRS%s", VTY_NEWLINE); 

  for (node = listhead (olsr->neighset); node; nextnode (node))
    {
      neigh = getdata (node);
      vty_out(vty, "%s\t\t%s\t\t%s\t\t%s\t%s%s",
	      inet_ntoa (neigh->main_addr), olsr_nt_str (neigh->status),
	      olsr_will_str (neigh->will), BOOL_STR (neigh->is_mpr), 
	      BOOL_STR (neigh->is_mprs), VTY_NEWLINE);
    }
  
  return CMD_SUCCESS;
}


DEFUN (show_ip_olsr_linkset,
       show_ip_olsr_linkset_cmd,
       "show ip olsr linkset",
       SHOW_STR
       IP_STR
       "OLSR information\n"
       "Link Set summary\n")
{
  struct olsr *olsr;
  struct olsr_interface *oi;
  struct olsr_link *ol;
  struct listnode *node, *lnode;
  char addr[17];

  olsr = olsr_lookup ();
  if (olsr == NULL)
    return CMD_SUCCESS;

  vty_out (vty, "Local addr\tNeighbor addr\tSYM expired\tASYM expired%s", VTY_NEWLINE); 

  for (node = listhead (olsr->oiflist); node; nextnode (node))
    {
      oi = getdata (node);

      strcpy (addr, inet_ntoa (OLSR_IF_ADDR (oi)));

      for (lnode = listhead (oi->linkset); lnode; nextnode (lnode))
	{
	  ol = getdata (lnode);
	  

	  vty_out (vty, "%s\t%s\t\t%s\t\t%s%s", addr, inet_ntoa (ol->neigh_addr),
		   BOOL_STR (ol->sym_expired), BOOL_STR (ol->asym_expired),
		   VTY_NEWLINE);
	}
    }
  
  return CMD_SUCCESS;
}

DEFUN (show_ip_olsr_topset,
       show_ip_olsr_topset_cmd,
       "show ip olsr topset",
       SHOW_STR
       IP_STR
       "OLSR information\n"
       "Topology Set database\n")
{
  struct olsr *olsr;
  struct olsr_top *top;
  struct listnode *node;
  char addr[17];

  olsr = olsr_lookup ();
  if (olsr == NULL)
    return CMD_SUCCESS;

  vty_out (vty, "Dest addr\tLast addr\tSeq. num.%s", VTY_NEWLINE); 

  LIST_LOOP (olsr->topset, top, node)
    {
      strcpy (addr, inet_ntoa (top->last_addr));
      vty_out (vty, "%s\t%s\t%d%s", inet_ntoa (top->dest_addr), addr,
	       top->seq, VTY_NEWLINE);
    }

  return CMD_SUCCESS;
}

DEFUN (show_ip_olsr_routes,
       show_ip_olsr_routes_cmd,
       "show ip olsr routes",
       SHOW_STR
       IP_STR
       "OLSR information\n"
       "Routing table\n")
{
  struct olsr *olsr;
  struct route_node *rn;
  struct olsr_route *on;
  char addr1[17];
  char addr2[17];


  olsr = olsr_lookup ();
  if (olsr == NULL)
    return CMD_SUCCESS;

  vty_out (vty, "Dest addr\tNext hop\tDist\tIface%s", VTY_NEWLINE); 

  for (rn = route_top (olsr->table); rn; rn = route_next (rn))
    if ((on = (struct olsr_route*)rn->info) != NULL)
      {
	strcpy (addr1, inet_ntoa (rn->p.u.prefix4));
	strcpy (addr2, inet_ntoa (on->next_hop));
	vty_out (vty, "%s\t%s\t%d\t%s%s", addr1, addr2,
		 on->dist, inet_ntoa (on->iface_addr),
		 VTY_NEWLINE);
      }

  return CMD_SUCCESS;
}

DEFUN (show_ip_olsr_hop2,
       show_ip_olsr_hop2_cmd,
       "show ip olsr hop2",
       SHOW_STR
       IP_STR
       "OLSR information\n"
       "2 Hop Neighbor Database\n")
{
  struct olsr *olsr;
  char addr[17];
  struct olsr_hop2 *hop2;
  struct listnode *node;

  olsr = olsr_lookup ();
  if (olsr == NULL)
    return CMD_SUCCESS;

  vty_out (vty, "2 Hop Neighbor\t\tNeighbor%s", VTY_NEWLINE);

  LIST_LOOP (olsr->n2hopset, hop2, node)
    {
      strcpy (addr, inet_ntoa (hop2->hop2_addr));      
      vty_out (vty, "%s\t\t%s%s", addr, inet_ntoa (hop2->neigh_addr),
	       VTY_NEWLINE);
    }

  return CMD_SUCCESS;
}

DEFUN (show_ip_olsr_mid,
       show_ip_olsr_mid_cmd,
       "show ip olsr mid",
       SHOW_STR
       IP_STR
       "OLSR information\n"
       "Multiple Interface Database\n")
{
  struct olsr *olsr;
  char addr[17];
  struct olsr_mid *mid;
  struct listnode *node;

  olsr = olsr_lookup ();
  if (olsr == NULL)
    return CMD_SUCCESS;

  vty_out (vty, "Main address\t\tAddress%s", VTY_NEWLINE);

  LIST_LOOP (olsr->midset, mid, node)
    {
      strcpy (addr, inet_ntoa (mid->addr));      
      vty_out (vty, "%s\t\t%s%s", inet_ntoa (mid->main_addr),
	       addr, VTY_NEWLINE);
    }

  return CMD_SUCCESS;
}



DEFUN (ip_olsr_hello_interval,
       ip_olsr_hello_interval_cmd,
       "ip olsr hello-interval <1-65535>",
       "IP Information\n"
       "OLSR interface commands\n"
       "Time between HELLO packets\n"
       "Milliseconds\n")
{
  struct interface *ifp = vty->index;
  struct olsr_if_info *oii = ifp->info;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "Hello Interval is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  oii->hello_interval = seconds;

  return CMD_SUCCESS;
}

DEFUN (no_ip_olsr_hello_interval,
       no_ip_olsr_hello_interval_cmd,
       "no ip olsr hello-interval",
       NO_STR
       "IP Information\n"
       "OLSR interface commands\n"
       "Time between HELLO packets\n")
{
  struct interface *ifp = vty->index;
  struct olsr_if_info *oii = ifp->info;

  oii->hello_interval = OLSR_HELLO_INTERVAL_DEFAULT;

  return CMD_SUCCESS;
}

DEFUN (show_ip_olsr,
       show_ip_olsr_cmd,
       "show ip olsr",
       SHOW_STR
       IP_STR
       "OLSR information\n")
{
  struct olsr *olsr;

  /* Check OLSR is enable. */
  olsr = olsr_lookup ();
  if (olsr == NULL)
    {
      vty_out (vty, " OLSR Routing Process not enabled%s", VTY_NEWLINE);
      return CMD_SUCCESS;
    }

  /* Show Router Main address. */
  vty_out (vty, " OLSR Routing Process, Main Address: %s%s",
           inet_ntoa (olsr->main_addr),
           VTY_NEWLINE);

  /* Show Willingness and neighbor hold time. */
  vty_out (vty, " Willingness: %s%s Neighbor hold time: %.3f s%s", 
	   olsr_will_str (olsr->will), VTY_NEWLINE,
	   olsr->neighb_hold_time, VTY_NEWLINE);

  /* Show duplicate hold time. */
  vty_out (vty, " Duplicate hold time: %.3f s%s",
	   olsr->dup_hold_time, VTY_NEWLINE);

  /* Show topology hold time. */
  vty_out (vty, " Topology hold time: %.3f s%s",
	   olsr->top_hold_time, VTY_NEWLINE);

  /* Show mpr update hold time. */
  vty_out (vty, " MPR update time: %.3f s%s",
	   olsr->mpr_update_time, VTY_NEWLINE);

  /* Show rt update hold time. */
  vty_out (vty, " Routing table update time: %.3f s%s",
	   olsr->rt_update_time, VTY_NEWLINE);


  return CMD_SUCCESS;
}

DEFUN (ip_olsr_mid_interval,
       ip_olsr_mid_interval_cmd,
       "ip olsr mid-interval <1-65535>",
       "IP Information\n"
       "OLSR interface commands\n"
       "Time between MID packets\n"
       "Milliseconds\n")
{
  struct interface *ifp = vty->index;
  struct olsr_if_info *oii = ifp->info;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "Mid Interval is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  oii->mid_interval = seconds;

  return CMD_SUCCESS;
}


DEFUN (no_ip_olsr_mid_interval,
       no_ip_olsr_mid_interval_cmd,
       "no ip olsr mid-interval",
       NO_STR
       "IP Information\n"
       "OLSR interface commands\n"
       "Time between MID packets\n")
{
  struct interface *ifp = vty->index;
  struct olsr_if_info *oii = ifp->info;
  
  oii->mid_interval = OLSR_MID_INTERVAL_DEFAULT;

  return CMD_SUCCESS;
}

DEFUN (ip_olsr_tc_interval,
       ip_olsr_tc_interval_cmd,
       "ip olsr tc-interval <1-65535>",
       "IP Information\n"
       "OLSR interface commands\n"
       "Time between TC packets\n"
       "Milliseconds\n")
{
  struct interface *ifp = vty->index;
  struct olsr_if_info *oii = ifp->info;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "Tc Interval is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  oii->tc_interval = seconds;

  return CMD_SUCCESS;
}


DEFUN (no_ip_olsr_tc_interval,
       no_ip_olsr_tc_interval_cmd,
       "no ip olsr tc-interval",
       NO_STR
       "IP Information\n"
       "OLSR interface commands\n"
       "Time between TC packets\n")
{
  struct interface *ifp = vty->index;
  struct olsr_if_info *oii = ifp->info;

  oii->tc_interval = OLSR_TC_INTERVAL_DEFAULT;

  return CMD_SUCCESS;
}

DEFUN (olsr_will,
       olsr_will_cmd,
       "olsr willingness (never|low|default|high|always|<0-7>)",
       "OLSR specific commands\n"
       "Willingness for this router\n"
       "Never select me as MPR.\n"
       "Select me only if necesar.\n"
       "Default willingness.\n"
       "In case of tide select me.\n"
       "Always select me as MPR.\n"
       "A number between 0 and 7.\n")
{
  struct olsr *olsr =  vty->index;
  int will;

  if (strncmp (argv[0], "n", 1) == 0)
    will = OLSR_WILL_NEVER;
  
  else if (strncmp (argv[0], "l", 1) == 0)
    will = OLSR_WILL_LOW;

  else if (strncmp (argv[0], "d", 1) == 0)
    will = OLSR_WILL_DEFAULT;

  else if (strncmp (argv[0], "h", 1) == 0)
    will = OLSR_WILL_HIGH;

  else if (strncmp (argv[0], "a", 1) == 0)
    will = OLSR_WILL_ALWAYS;

  else
    will = atoi (argv[0]);

  if (will < 0 || will > 7)
    {
      vty_out (vty, "Invalid willingness%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  olsr->will = will;

  return CMD_SUCCESS;
}

DEFUN (no_olsr_will,
       no_olsr_will_cmd,
       "no olsr willingness",
       NO_STR
       "OLSR specific commands\n"
       "Willingness of this router.\n")
{
  struct olsr *olsr =  vty->index;

  olsr->will = OLSR_WILL_DEFAULT;

  return CMD_SUCCESS;
}


DEFUN (olsr_neighb_hold_time,
       olsr_neighb_hold_time_cmd,
       "olsr neighb-hold-time <1-65535>",
       "OLSR specific commands\n"
       "Time too keep a neighbor.\n"
       "Milliseconds\n")
{
  struct olsr *olsr =  vty->index;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "Neighbor hold time is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  olsr->neighb_hold_time = seconds;

  return CMD_SUCCESS;
}

DEFUN (no_olsr_neighb_hold_time,
       no_olsr_neighb_hold_time_cmd,
       "no olsr neighb-hold-time",
       NO_STR
       "OLSR specific commands\n"
       "Time too keep a neighbor.\n")
{
  struct olsr *olsr =  vty->index;

  olsr->neighb_hold_time = OLSR_NEIGHB_HOLD_TIME_DEFAULT;

  return CMD_SUCCESS;
}

DEFUN (olsr_dup_hold_time,
       olsr_dup_hold_time_cmd,
       "olsr dup-hold-time <1-65535>",
       "OLSR specific commands\n"
       "Time too remember a duplicate message.\n"
       "Milliseconds\n")
{
  struct olsr *olsr =  vty->index;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "Duplicate hold time is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  olsr->dup_hold_time = seconds;

  return CMD_SUCCESS;
}

DEFUN (no_olsr_dup_hold_time,
       no_olsr_dup_hold_time_cmd,
       "no olsr dup-hold-time",
       NO_STR
       "OLSR specific commands\n"
       "Time too keep a duplicate message.\n")
{
  struct olsr *olsr =  vty->index;

  olsr->dup_hold_time = OLSR_DUP_HOLD_TIME_DEFAULT;

  return CMD_SUCCESS;
}

DEFUN (olsr_top_hold_time,
       olsr_top_hold_time_cmd,
       "olsr top-hold-time <1-65535>",
       "OLSR specific commands\n"
       "Time too remember a topology control message.\n"
       "Milliseconds\n")
{
  struct olsr *olsr =  vty->index;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "Topology hold time is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  olsr->top_hold_time = seconds;

  return CMD_SUCCESS;
}

DEFUN (no_olsr_top_hold_time,
       no_olsr_top_hold_time_cmd,
       "no olsr top-hold-time",
       NO_STR
       "OLSR specific commands\n"
       "Time too keep a topology control message.\n")
{
  struct olsr *olsr =  vty->index;

  olsr->top_hold_time = OLSR_TOP_HOLD_TIME_DEFAULT;

  return CMD_SUCCESS;
}

DEFUN (olsr_mpr_update_time,
       olsr_mpr_update_time_cmd,
       "olsr mpr-update-time <1-65535>",
       "OLSR specific commands\n"
       "Delay time for MPR update.\n"
       "Milliseconds\n")
{
  struct olsr *olsr =  vty->index;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "MPR update time is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  olsr->mpr_update_time = seconds;

  return CMD_SUCCESS;
}

DEFUN (no_olsr_mpr_update_time,
       no_olsr_mpr_update_time_cmd,
       "no olsr top-hold-time",
       NO_STR
       "OLSR specific commands\n"
       "Delay time for MPR update.\n")
{
  struct olsr *olsr =  vty->index;

  olsr->mpr_update_time = OLSR_MPR_UPDATE_TIME_DEFAULT;

  return CMD_SUCCESS;
}


DEFUN (olsr_rt_update_time,
       olsr_rt_update_time_cmd,
       "olsr rt-update-time <1-65535>",
       "OLSR specific commands\n"
       "Delay time for routing table update.\n"
       "Milliseconds\n")
{
  struct olsr *olsr =  vty->index;
  float seconds;
  long msecs;


  if (argc < 1)
    return CMD_ERR_INCOMPLETE;

  msecs = strtol (argv[0], NULL, 10);
  seconds = (float)msecs / 1000.0;

  if (seconds < 1)
    {
      vty_out (vty, "RT update time is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  olsr->rt_update_time = seconds;

  return CMD_SUCCESS;
}

DEFUN (no_olsr_rt_update_time,
       no_olsr_rt_update_time_cmd,
       "no olsr top-hold-time",
       NO_STR
       "OLSR specific commands\n"
       "Delay time for routing table update.\n")
{
  struct olsr *olsr =  vty->index;

  olsr->rt_update_time = OLSR_RT_UPDATE_TIME_DEFAULT;

  return CMD_SUCCESS;
}


int 
olsr_router_config_write (struct vty *vty)
{
  struct olsr *olsr;
  struct route_node *rn;
  int write = 0;
  olsr = olsr_lookup();

  if (olsr != NULL)
    {
      /* `router olsr' print. */
      vty_out (vty, "router olsr%s", VTY_NEWLINE);
      write++;

      /* router willingness */
      if (olsr->will != OLSR_WILL_DEFAULT)
	vty_out (vty, " olsr willingness %s%s", 
		 olsr_will_str (olsr->will), VTY_NEWLINE);

      /* Neighbor hold time. */
      if (olsr->neighb_hold_time != OLSR_NEIGHB_HOLD_TIME_DEFAULT)
	vty_out (vty, " olsr neighb-hold-time %d%s",
		 FLOAT_2_MSECS (olsr->neighb_hold_time),
		 VTY_NEWLINE);

      /* Dup hold time. */
      if (olsr->dup_hold_time != OLSR_DUP_HOLD_TIME_DEFAULT)
	vty_out (vty, " olsr dup-hold-time %d%s",
		 FLOAT_2_MSECS (olsr->dup_hold_time),
		 VTY_NEWLINE);

      /* Top hold time. */
      if (olsr->top_hold_time != OLSR_TOP_HOLD_TIME_DEFAULT)
	vty_out (vty, " olsr top-hold-time %d%s",
		 FLOAT_2_MSECS (olsr->top_hold_time),
		 VTY_NEWLINE);

      /* Mpr update time. */
      if (olsr->mpr_update_time != OLSR_MPR_UPDATE_TIME_DEFAULT)
	vty_out (vty, " olsr mpr-update-time %d%s",
		 FLOAT_2_MSECS (olsr->mpr_update_time),
		 VTY_NEWLINE);

      /* Rt update time. */
      if (olsr->rt_update_time != OLSR_RT_UPDATE_TIME_DEFAULT)
	vty_out (vty, " olsr rt-update-time %d%s",
		 FLOAT_2_MSECS (olsr->rt_update_time),
		 VTY_NEWLINE);

      /* `network' print. */
      for (rn = route_top (olsr->networks); rn; rn = route_next (rn))
	if (rn->info)
	  {
	    /* Network print. */
	    vty_out (vty, " network %s/%d%s",
		     inet_ntoa (rn->p.u.prefix4), rn->p.prefixlen,
		     VTY_NEWLINE);
	  }

    }

  return write;
}

int 
olsr_write_interface (struct vty *vty)
{
  struct listnode *node;
  struct interface *ifp;
  struct olsr_if_info *oii;
  int write = 0;

  
  LIST_LOOP (iflist, ifp, node)
    {

      vty_out (vty, "!%s", VTY_NEWLINE);
      vty_out (vty, "interface %s%s", ifp->name,
               VTY_NEWLINE);
      
      if (ifp->desc)
        vty_out (vty, " description %s%s", ifp->desc,
		 VTY_NEWLINE);
      
      write ++;
      oii = ifp->info;
      
      /* Hello Interval print. */
      if (oii->hello_interval != OLSR_HELLO_INTERVAL_DEFAULT)
	vty_out (vty, " ip olsr hello-interval %u%s", 
		 FLOAT_2_MSECS(oii->hello_interval),
		 VTY_NEWLINE);

      /* Mid Interval print. */
      if (oii->mid_interval != OLSR_MID_INTERVAL_DEFAULT)
	vty_out (vty, " ip olsr mid-interval %u%s", 
		 FLOAT_2_MSECS(oii->mid_interval),
		 VTY_NEWLINE);

      /* Tc Interval print. */
      if (oii->tc_interval != OLSR_TC_INTERVAL_DEFAULT)
	vty_out (vty, " ip olsr tc-interval %u%s", 
		 FLOAT_2_MSECS(oii->tc_interval),
		 VTY_NEWLINE);

    }
  
  return write;
}


/* 
 * Install OLSR commands.
 */
void olsr_vty_init()
{
  /*
   * Router commands.
   */
	
  /* Install router nodes. */
  install_node(&olsr_router_node, olsr_router_config_write);

  /* "router olsr" commands. */
  install_element (CONFIG_NODE, &router_olsr_cmd);
  install_default (OLSR_NODE);

  /* "olsr network" commands */
  install_element (OLSR_NODE, &olsr_network_cmd);

  /* "olsr willingness" commands.*/
  install_element (OLSR_NODE, &olsr_will_cmd);
  install_element (OLSR_NODE, &no_olsr_will_cmd);

  /* "olsr neighb-hold-time" commands.*/
  install_element (OLSR_NODE, &olsr_neighb_hold_time_cmd);
  install_element (OLSR_NODE, &no_olsr_neighb_hold_time_cmd);

  /* "olsr dup-hold-time" commands.*/
  install_element (OLSR_NODE, &olsr_dup_hold_time_cmd);
  install_element (OLSR_NODE, &no_olsr_dup_hold_time_cmd);

  /* "olsr top-hold-time" commands.*/
  install_element (OLSR_NODE, &olsr_top_hold_time_cmd);
  install_element (OLSR_NODE, &no_olsr_top_hold_time_cmd);

  /* "olsr mpr-update-time" commands.*/
  install_element (OLSR_NODE, &olsr_mpr_update_time_cmd);
  install_element (OLSR_NODE, &no_olsr_mpr_update_time_cmd);

  /* "olsr rt-update-time" commands.*/
  install_element (OLSR_NODE, &olsr_rt_update_time_cmd);
  install_element (OLSR_NODE, &no_olsr_rt_update_time_cmd);

  /* "show ip olsr" comands.*/
  install_element (VIEW_NODE, &show_ip_olsr_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_cmd);
  install_element (OLSR_NODE, &show_ip_olsr_cmd);
  
  /* "show ip olsr neighbor" commands. */
  install_element (VIEW_NODE, &show_ip_olsr_neighbor_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_neighbor_cmd);

  /* "show ip olsr linkset" commands. */
  install_element (VIEW_NODE, &show_ip_olsr_linkset_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_linkset_cmd);

  /* "show ip olsr topset" commands. */
  install_element (VIEW_NODE, &show_ip_olsr_topset_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_topset_cmd);

  /* "show ip olsr mid" commands. */
  install_element (VIEW_NODE, &show_ip_olsr_mid_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_mid_cmd);

  /* "show ip olsr hop2" commands. */
  install_element (VIEW_NODE, &show_ip_olsr_hop2_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_hop2_cmd);

  /* "show ip olsr routes" commands. */
  install_element (VIEW_NODE, &show_ip_olsr_routes_cmd);
  install_element (ENABLE_NODE, &show_ip_olsr_routes_cmd);

  /* 
   * Interface commands. 
   */

  /* Install interface node. */
  install_node (&olsr_interface_node, olsr_write_interface);

  install_element (CONFIG_NODE, &interface_cmd);
  install_element (CONFIG_NODE, &no_interface_cmd);
  install_default (INTERFACE_NODE);

  /* "ip olsr hello-interval" commands. */
  install_element (INTERFACE_NODE, &ip_olsr_hello_interval_cmd);
  install_element (INTERFACE_NODE, &no_ip_olsr_hello_interval_cmd);

  /* "ip olsr mid-interval" commands. */
  install_element (INTERFACE_NODE, &ip_olsr_mid_interval_cmd);
  install_element (INTERFACE_NODE, &no_ip_olsr_mid_interval_cmd);

  /* "ip olsr tc-interval" commands. */
  install_element (INTERFACE_NODE, &ip_olsr_tc_interval_cmd);
  install_element (INTERFACE_NODE, &no_ip_olsr_tc_interval_cmd);

}
