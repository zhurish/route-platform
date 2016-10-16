/* The Command Handlers for RSVP-TE Daemon.
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

#include <zebra.h>

#include "memory.h"
#include "thread.h"
#include "vty.h"
#include "command.h"
#include "prefix.h"
#include "if.h"
#include "avl.h"
#include "table.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_vty.h"
#include "rsvpd/rsvp_route.h"

/* Return route type string for VTY output.  */
const char *
route_type_str (u_char type)
{
  switch (type)
    {
    case ZEBRA_ROUTE_SYSTEM:
      return "system";
    case ZEBRA_ROUTE_KERNEL:
      return "kernel";
    case ZEBRA_ROUTE_CONNECT:
      return "connected";
    case ZEBRA_ROUTE_STATIC:
      return "static";
    case ZEBRA_ROUTE_RIP:
      return "rip";
    case ZEBRA_ROUTE_RIPNG:
      return "rip";
    case ZEBRA_ROUTE_OSPF:
      return "ospf";
    case ZEBRA_ROUTE_OSPF6:
      return "ospf";
    case ZEBRA_ROUTE_BGP:
      return "bgp";
    default:
      return "unknown";
    }
};

/* Return route type string for VTY output.  */
const char
route_type_char (u_char type)
{
  switch (type)
    {
    case ZEBRA_ROUTE_SYSTEM:
      return 'S';
    case ZEBRA_ROUTE_KERNEL:
      return 'K';
    case ZEBRA_ROUTE_CONNECT:
      return 'C';
    case ZEBRA_ROUTE_STATIC:
      return 'S';
    case ZEBRA_ROUTE_RIP:
      return 'R';
    case ZEBRA_ROUTE_RIPNG:
      return 'R';
    case ZEBRA_ROUTE_OSPF:
      return 'O';
    case ZEBRA_ROUTE_OSPF6:
      return 'O';
    case ZEBRA_ROUTE_BGP:
      return 'B';
    default:
      return '?';
    }
};

/* New RIB.  Detailed information for IPv4 route. */
void
vty_show_ip_route_detail (struct vty *vty, struct route_node *rn)
{
   return;
}

/*---------------------------------------------------------------------------
 * Function : vty_show_ip_route.
 * Input    : s  = Pointer to object of type struct stream.
 *          : rn = Pointer to object of type struct route_node.
 * Output   : None.
 * Synopsis : This function shows a IP route info from RSVP-TE's RIB.
 * Callers  : show_ip_route CLI handler in rsvp_vty.c.
 *--------------------------------------------------------------------------*/
void
vty_show_ip_route (struct vty *vty, struct route_node *rn)
{
   struct rsvp_route_info *rsvp_route_info;
   int len = 0;
   char buf[BUFSIZ];

   /* Sanity Check. */
   if (!vty || !rn)
      return;
   /* Extract the Routing Info from the route_node.*/
   if((rsvp_route_info = (struct rsvp_route_info *)rn->info) == NULL)
      return;
   /* Prefix Information. */
   len = vty_out (vty, "%c%c%c %s%c%d",
                  route_type_char (rsvp_route_info->type),
                  '>',
                  '*',
                  inet_ntop (AF_INET, &rn->p.u.prefix, buf, BUFSIZ),
                  '/',
                  rn->p.prefixlen); 
   /* Print the nexthop address. */
   if (rsvp_route_info->nexthop.s_addr == 0)
      vty_out (vty, " is directly connected");
   else 
      vty_out (vty, "  via %s", inet_ntoa (rsvp_route_info->nexthop));
   /* Print the outgoing interface. */
   if (rsvp_route_info->ifindex)
      vty_out (vty, ", %s%s", ifindex2ifname (rsvp_route_info->ifindex),
               VTY_NEWLINE);
    
   return;
}

/*---------------------------------------------------------------------------
 * Funtion : rsvp_statistics_dump_vty 
 * Input   : vty = Pointer to struct vty
 *           statistics = Pointer to struct rsvp_statistics
 * Output  : None
 * Synopsis: This is a utility function to print rsvp_statitics information
 *           into vty.
 * Callers : CLI utilities (TBD)
 *----------------------------------------------------------------------*/
void
rsvp_statistics_dump_vty (struct vty *vty,
                          struct rsvp_statistics *statistics)
{
   /* Sanity Check. */
   if (!vty || !statistics)
      return;
   
   /* Headline. */
   vty_out (vty, "%s", VTY_NEWLINE);
   vty_out (vty, "  IN Packet Statistics %s", VTY_NEWLINE);
   vty_out (vty, "  ==================== %s", VTY_NEWLINE);
   vty_out (vty, "  Path      = %d%s", statistics->path_in, VTY_NEWLINE);
   vty_out (vty, "  Resv      = %d%s", statistics->resv_in, VTY_NEWLINE);
   vty_out (vty, "  Path Err  = %d%s", statistics->path_err_in, VTY_NEWLINE);
   vty_out (vty, "  Resv Err  = %d%s", statistics->resv_err_in, VTY_NEWLINE);
   vty_out (vty, "  Path Tear = %d%s", statistics->path_tear_in, VTY_NEWLINE);
   vty_out (vty, "  Resv Tear = %d%s", statistics->resv_tear_in, VTY_NEWLINE);
   vty_out (vty, "  Resv Conf = %d%s", statistics->resv_conf_in, VTY_NEWLINE);
   vty_out (vty, "  Hello     = %d%s", statistics->resv_conf_in, VTY_NEWLINE);
   vty_out (vty, "%s", VTY_NEWLINE);

   vty_out (vty, "  OUT Packet Statistics %s", VTY_NEWLINE);
   vty_out (vty, "  ===================== %s", VTY_NEWLINE);   
   vty_out (vty, "  Path      = %d%s", statistics->path_out, VTY_NEWLINE);
   vty_out (vty, "  Resv      = %d%s", statistics->resv_out, VTY_NEWLINE);
   vty_out (vty, "  Path Err  = %d%s", statistics->path_err_out, VTY_NEWLINE);
   vty_out (vty, "  Resv Err  = %d%s", statistics->resv_err_out, VTY_NEWLINE);
   vty_out (vty, "  Path Tear = %d%s", statistics->path_tear_out, VTY_NEWLINE);
   vty_out (vty, "  Resv Tear = %d%s", statistics->resv_tear_out, VTY_NEWLINE);
   vty_out (vty, "  Resv Conf = %d%s", statistics->resv_conf_out, VTY_NEWLINE);
   vty_out (vty, "  Hello     = %d%s", statistics->hello_out, VTY_NEWLINE);
   vty_out (vty, "%s", VTY_NEWLINE); 
          
   return;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_ip_path_dump_vty
 * Input    : vty = Pointer to object of type struct vty.
 *          : rsvp_ip_path = Pointer to object of type struct rsvp_ip_path.
 * Output   : void
 * Synopsis : This is a utility function that dumps all info of a ip explicit
 *            path to vty.
 *-------------------------------------------------------------------------*/
void
rsvp_ip_path_dump_vty (struct vty *vty,
                       struct rsvp_ip_path *rsvp_ip_path)
{
    struct listnode *node;
    int count;
   /* Sanity Check. */
    if (!vty || !rsvp_ip_path)
       return;

    vty_out (vty, "%s", VTY_NEWLINE);
    vty_out (vty, "Path Name : %s%s", rsvp_ip_path->name, VTY_NEWLINE);
    vty_out (vty, "===========%s", VTY_NEWLINE);
    vty_out (vty, "Number of hops : %d%s", rsvp_ip_path->num_hops, VTY_NEWLINE);
 
    for (node = listhead ((struct list *)rsvp_ip_path->ip_hop_list), count = 0;
         node;
         nextnode (node), count++)
    {
       struct rsvp_ip_hop *rsvp_ip_hop = (struct rsvp_ip_hop *)node;
       char hop_addr_str[INET6_ADDRSTRLEN] ;

       inet_ntop (rsvp_ip_hop->hop.family,
                  &rsvp_ip_hop->hop.u.prefix,
                  hop_addr_str,
                  sizeof (hop_addr_str));
                   
       vty_out (vty, "Hop %d  : %s ", ++count, hop_addr_str);
       
       if (rsvp_ip_hop->type == RSVP_IP_HOP_STRICT)
          vty_out (vty, "strict%s", VTY_NEWLINE);
       else 
          vty_out (vty, "loose%s", VTY_NEWLINE);     
    }   
    /* Check if this path is being used. */
    vty_out (vty, "In Use : ");

    if(rsvp_ip_path->ref_cnt)
        vty_out (vty, "Yes%s", VTY_NEWLINE);
    else
        vty_out (vty, "No%s", VTY_NEWLINE);
    
    vty_out (vty, "%s", VTY_NEWLINE);
    return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_dump_vty
 * Input    : rsvp_tunnel_if = Pointer to struct rsvp_tunnel_if object.
 * Output   : None
 * Synopsis : This function displays all rsvp_tunnel_if to VTY in CLI Context.
 * Callers  : CLI Handler show_rsvp_tunnel_if in rsvp_vty.c
 *--------------------------------------------------------------------------*/
void
rsvp_tunnel_if_dump_vty (struct vty *vty,
                         struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   /* Sanity Check. */
   if (!vty || !rsvp_tunnel_if)
      return;

   vty_out (vty, "Interface TUNNEL %d%s", rsvp_tunnel_if->num, VTY_NEWLINE);
   vty_out (vty, "=====================%s", VTY_NEWLINE);
        
   return;
}

/*---------------------------------------------------------------------------
 * Function : ip_config_write_explicit_path
 * Input    : vty = Pointer to object of type struct vty
 *          : rsvp_ip_path = Pointer to object of struct rsvp_ip_path
 * Output   : Returns the number of lines written.
 * Synopsis : This utility function writes a explicit path config into congih 
 *            file.
 * Callers : ip_config_write function in rsvp_vty.c
 *-------------------------------------------------------------------------*/
int
ip_config_write_explicit_path (struct vty *vty,
                               struct rsvp_ip_path *rsvp_ip_path)
{
   int write = 0;
   struct listnode *node;   

   vty_out (vty, "  explicit-path name %s%s", rsvp_ip_path->name, VTY_NEWLINE);
   write++;
   
   for (node = listhead (rsvp_ip_path->ip_hop_list); node; nextnode (node))
   {
      char hop_addr_str[INET6_ADDRSTRLEN];
      struct rsvp_ip_hop *rsvp_ip_hop = (struct rsvp_ip_hop *)getdata (node);
      char *hop_type_str = (rsvp_ip_hop->type == RSVP_IP_HOP_STRICT) ? 
                           "strict" : "loose";
   
      /* Extract the next hop address. */
      inet_ntop (rsvp_ip_hop->hop.family, 
                 &rsvp_ip_hop->hop.u.prefix,
                 hop_addr_str, sizeof (hop_addr_str)); 
      vty_out (vty, "    next-address %s %s%s",hop_addr_str, hop_type_str,
               VTY_NEWLINE);
      write++;       
   }

   return write;        
}

/*----------------------------------------------------------------------------
 * Function : rsvp_tunnel_if_config_write 
 * Input    : vty = Pointer to the object of type struct vty
 *          : rsvp_tunnel_if = Pointer to the objectstruct rsvp_tunnel_if
 * Output   : Returns number of lines written to vty.
 * Synopsis : This function writes rsvp_tunnel_if config into config file.
 *            This function is always called with vty set to the config file
 *            object.
 * Callers  : interface_tunnel in rsvp_vty.c
 *--------------------------------------------------------------------------*/ 
int
rsvp_tunnel_if_config_write (struct vty *vty,
                             struct rsvp_tunnel_if *rsvp_tunnel_if)
{
   int write = 0;
   char addr[INET6_ADDRSTRLEN];
   int i;
   /* Sanity Check. */
   if (!vty || !rsvp_tunnel_if)
      return write;

   vty_out (vty, "interface tunnel %d%s", rsvp_tunnel_if->num, VTY_NEWLINE);
   write++;

   if (CHECK_FLAG (rsvp_tunnel_if->cflags, RSVP_TUNNEL_IF_CFLAG_RSVP))
   {
      vty_out (vty, "   tunnel mode mpls traffic-eng%s", VTY_NEWLINE);
      write++;
   }

   if (CHECK_FLAG (rsvp_tunnel_if->cflags, RSVP_TUNNEL_IF_CFLAG_DST))
   {
      int size = 0;

      prefix2str (&rsvp_tunnel_if->destination, addr, INET6_ADDRSTRLEN);
      /* As destinatio address is always a host address, so don't print the 
         prefix length.*/
      size =  ((char *)strchr (addr, '/')) - addr ;  
      strncpy (addr, addr, size);
      addr[size] = '\0';            
      vty_out (vty, "   tunnel destination %s%s", addr, VTY_NEWLINE);
      write++;
   }

   if (CHECK_FLAG (rsvp_tunnel_if->cflags, RSVP_TUNNEL_IF_CFLAG_CSPF))
   {
      vty_out (vty, "   tunnel mpls traffic-eng cspf%s", VTY_NEWLINE);  
      write++;
   }
   /* Print the paths. */
   for (i = 0; i < 6 ; i++)
   {
      char *path_type;

      struct rsvp_tunnel_if_path_option *path_option = 
                                               rsvp_tunnel_if->path_option + i;
      /* If this path-option is not valid then go to next option.*/
      if(!path_option->valid_flag)
         continue;
 
      path_type = (path_option->type == RSVP_TUNNEL_IF_PATH_OPTION_EXPLICIT) ?
                  "explicit" : "dynamic";
      vty_out (vty, "   tunnel mpls traffic-eng path-option %d %s",
               (i + 1), path_type);
      /* Print Standby Information. */      
      if (path_option->standby_flag)
         vty_out (vty, " standby");
      else
         vty_out (vty, " no-standby");

      /* If its explicit path option then print name of the path.*/
      if (path_type == RSVP_TUNNEL_IF_PATH_OPTION_EXPLICIT)
         vty_out (vty, " %s", path_option->path->name);
      
      vty_out (vty, "%s", VTY_NEWLINE) ;
      write++;
   }
   /* If the Tunnel is dministraively down then display it. */
   if (!RSVP_TUNNEL_IF_IS_ADMIN_UP (rsvp_tunnel_if))
   {   
      vty_out (vty, "   shutdown%s", VTY_NEWLINE);
      write++;
   }
   return write;
} 
/*---------------------------------------------------------------------------
 * Following all are command handlers exported by this to lib/command.c  
 * module. This command handlers are invoked from CLI/vty.
 *-------------------------------------------------------------------------*/

#define SHOW_ROUTE_V4_HEADER "Codes: K - kernel route, C - connected, S - static, R - RIP, O - OSPF,%s       B - BGP, > - selected route, * - FIB route%s%s"

DEFUN (show_ip_route,
       show_ip_route_cmd,
       "show ip route",
       SHOW_STR
       IP_STR
       "IP routing table\n")
{
  struct route_table *rtable;
  struct route_node *rn;
  int first = 1;

  /* Sanity Check.*/
  assert (rm->rsvp);
  /* Get the routing table of RSVP-TE Daemon. */
  if ((rtable = (struct route_table *)rm->rsvp->route_table) == NULL)
     return CMD_SUCCESS;

  /* Show all IPV4 routes. */
  for (rn = route_top (rtable); rn ; rn = route_next (rn))
  {
     if (first)
     {
        vty_out (vty, SHOW_ROUTE_V4_HEADER, VTY_NEWLINE, VTY_NEWLINE,
                 VTY_NEWLINE);
        first = 0;
     }
     vty_show_ip_route (vty, rn);
  }

  return CMD_SUCCESS;
}

/*--------------------------------------------------------------------------
 * Handlers for command "mpls" in config mode. It will set the command node
 * to MPLS_NODE.
 *------------------------------------------------------------------------*/
/*
DEFUN (mpls,
       mpls_cmd,
       "mpls",
       MPLS_STR)
{
   vty->node = MPLS_NODE;
   vty->index = rm->rsvp;

   return CMD_SUCCESS;
}
*/
/*---------------------------------------------------------------------------
 * Handler for command "mpls router-id loopback". It will set the Router-ID
 * for RSVP-TE to the loopback interface. This command is executed in 
 * CONFIG_NODE.
 *-------------------------------------------------------------------------*/
DEFUN (mpls_router_id_loopback,
       mpls_router_id_loopback_cmd,
       "mpls router-id loopback",
       MPLS_STR
       "Set the router-id for RSVP-TE\n"
       "Select the loopback interface\n")
{
   struct interface *ifp;   
   struct connected *ifc;
   struct rsvp *rsvp = rm->rsvp;
   struct listnode *node;

   /* Parse the interface list.*/
   for (node = (struct listnode *)listhead (rm->rsvp->iflist); 
        node; 
        nextnode (node))
   {
      ifp = (struct interface *)getdata (node);
   
      /* Check if its a loopback interface. We assume that there would be
         only one loopback interface in the system. Else the logic will fail.*/
      if (if_is_loopback (ifp))
      {
         /* Get the first configured address on this loopback interface.*/
         for (node = listhead (ifp->connected); node; nextnode (node))
         { 
            ifc = (struct connected *)getdata (node);
            
            /* Check if this is the default loopback address 127.0.0.1.*/
            if (ifc->address->u.prefix4.s_addr == 0x3F000001)
               continue;

            memcpy (ifc->address, &rsvp->router_id, sizeof (struct prefix));
            /* Indicate that the loopback address is in use as router-id.*/
            SET_FLAG (rm->rsvp->cflags, RSVP_CFLAG_ROUTER_ID_LOOPBACK);
            /* In case the router-id at present is some other address then 
               reset that flag.*/
            UNSET_FLAG (rm->rsvp->cflags, RSVP_CFLAG_ROUTER_ID_ADDR);
            return CMD_SUCCESS;
         } /* End of  for (node = listhead (ifp..*/

         /* This loop breaks if no address found to be configured.*/
         vty_out (vty, "%% No Address Configured on interface %s%s",
                  ifp->name, VTY_NEWLINE);
         return CMD_WARNING;               
      } /* End of  if (if_is_loopback (ifp)).*/
   } /* End of  for (node = listhead (rm->rsvp .. */ 
   
   /* This portion is reached when no loopback interface is found.*/
   vty_out (vty, "%% No loopback interface is found in RSVP-TE Daemon%s",
            VTY_NEWLINE);
   return CMD_WARNING;
}

/*--------------------------------------------------------------------------
 * Handler for "no mpls router-id loopback".
 *------------------------------------------------------------------------*/
DEFUN (no_mpls_router_id_loopback,
       no_mpls_router_id_loopback_cmd,
       "no mpls router-id loopback",
       NO_STR
       MPLS_STR
       "router-id to be used by RSVP-TE\n"
       "loopback interface\n")
{
   /* Get the RSVP Hook. */
   struct rsvp *rsvp = rm->rsvp;

   /* Check if the command is enabled earlier.*/
   if (! CHECK_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_LOOPBACK))
   {
      vty_out (vty, "%%The command is already not set%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
  
   /* Unset the router-id */
   bzero (&rsvp->router_id, sizeof (struct prefix));
   UNSET_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_LOOPBACK);

   return CMD_SUCCESS; 
}
/*---------------------------------------------------------------------------
 * Handler for command "router-id loopback X". It will set the Router-ID
 * for RSVP-TE to the loopback interface specified. This command is executed
 * in MPLS_NODE and the affect is same as the command above. 
 *-------------------------------------------------------------------------*/
ALIAS (mpls_router_id_loopback,
       router_id_loopback_cmd,
       "router-id loopback",
       MPLS_STR
       "Set Router-ID for RSVP-TE\n"
       "The loopback interface\n");

/*---------------------------------------------------------------------------
 * "no router-id loopback N" command.
 *-------------------------------------------------------------------------*/
ALIAS (no_mpls_router_id_loopback,
       no_router_id_loopback_cmd,
       "no router-id loopback",
       NO_STR
       "Set router-id for RSVP-TE\n"
       "Interface number\n");

/*--------------------------------------------------------------------------
 * CLI handler for "mpls router-id A.B.C.D". This is called from CONFIG_NODE
 *-------------------------------------------------------------------------*/
DEFUN (mpls_router_id_addr,
       mpls_router_id_addr_cmd,
       "mpls router-id A.B.C.D",
       MPLS_STR
       "Set router-id\n"
       "IPv4 Address\n")
{
   struct rsvp *rsvp = (struct rsvp *)rm->rsvp;

   /* Check if IP v4address is specified.*/
   if (argc == 0)
   {
      vty_out (vty, "%% IPv4 Address not specified%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* Fill the router_id.*/
   str2prefix (argv[0], & rsvp->router_id);
   SET_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_ADDR);
   UNSET_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_LOOPBACK);
   
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * CLI Handler for "router-id A.B.C.D" hooked into MPLS_NODE.
 *--------------------------------------------------------------------------*/
ALIAS (mpls_router_id_addr,
       router_id_addr_cmd,
       "router-id A.B.C.D",
       "Router-id used by RSVP-TE\n"
       "IPV4 Address\n");

/*----------------------------------------------------------------------------
 * CLI Handler for "no mpls router-id A.B.C.D". This command is hooked into
 * CONFIG_NODE.
 *--------------------------------------------------------------------------*/
DEFUN (no_mpls_router_id_addr,
       no_mpls_router_id_addr_cmd,
       "no mpls router-id A.B.C.D",
       NO_STR
       MPLS_STR
       "Set router-id"
       "IPV4 Address")
{
   struct rsvp *rsvp = (struct rsvp *)rm->rsvp;
   struct prefix router_id;
   
   /* Check that IPV4 Address is specified.*/
   if (argc == 0)
   {
      vty_out (vty, "%% IPV4 Address Not Specified%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* Check if the router-id is set by external IPV4 address.*/
   if (!CHECK_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_ADDR))
   {
      vty_out (vty, "%% The command was not set earlier%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   str2prefix (argv[0], &router_id);
   
   /* Compare if the addresses are same.*/
   if (memcmp (&rsvp->router_id, &router_id, sizeof (struct prefix)))
   {
      vty_out (vty, "%% The IPV4 address doesn't match %s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   
   /* So the IPV4 address matched.*/
   bzero (&rsvp->router_id, sizeof (struct prefix));
   UNSET_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_ADDR);

   return CMD_WARNING;
}

/*----------------------------------------------------------------------------
 * CLI Handler for "no router-id A.B.C.D" hooked into MPLS_NODE.
 *--------------------------------------------------------------------------*/
ALIAS (no_mpls_router_id_addr,
       no_router_id_addr_cmd,
       "no router-id A.B.C.D",
       NO_STR
       "Router-id used by RSVP-TE\n"
       "IPV4 Address\n");

/*---------------------------------------------------------------------------
 * handlers for "rsvp" command in config mode. It will set the config node to
 * RSVP_NODE.
 *-------------------------------------------------------------------------*/
DEFUN (rsvp,
       rsvp_cmd,
       "rsvp",
       RSVP_STR)
{
   vty->node = RSVP_NODE;
   vty->index = rm->rsvp; 

   return CMD_SUCCESS;
}

/*--------------------------------------------------------------------------
 * Handler for "ip" command. The command switches to config-ip# node.
 *------------------------------------------------------------------------*/
DEFUN (ip,
       ip_cmd,
       "ip",
       "IPv4/v6 Information\n")
{
   //vty->node = IP_NODE;

   return CMD_SUCCESS;
}

/*--------------------------------------------------------------------------
 * Handler for "ip explicit-path name xyz" command. This is called by 
 * config_node.
 *------------------------------------------------------------------------*/
DEFUN (ip_explicit_path,
       ip_explicit_path_cmd,
       "ip explicit-path name WORD",
       IP_STR
       IP_EXPLICIT_PATH_STR
       "Name of the path\n"
       "In Alphanumeric\n")
{
   struct rsvp_ip_path *rsvp_ip_path;

   /* When this command is executed switch to "ip explicit-path" node. */
   //vty->node = IP_EXPLICIT_PATH_NODE;

   /* Search if the path already exists with the name. */
   rsvp_ip_path = rsvp_ip_path_search (argv[0]);
   
   /* If not found, then set create the path with this new name */
   if (!rsvp_ip_path)
   {
       /* This will create the rsvp_ip_path and will hook into the 
        * ternary serach trie at hook rm->rsvp->ip_path_obj. */
       rsvp_ip_path = rsvp_ip_path_create (argv[0]);
       
       /* Sanity Check.*/
       if (!rsvp_ip_path)
       {
          vty_out (vty, "ip explicit-path creation failed%s", VTY_NEWLINE);
          vty->node = CONFIG_NODE;
          return CMD_WARNING;
       }
   }
   /* Set the vty->index to rsvp_ip_path created. */
   vty->index = rsvp_ip_path;
   
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "no ip explicit-path name xyz" command.
 *-------------------------------------------------------------------------*/
DEFUN (no_ip_explicit_path,
       no_ip_explicit_path_cmd,
       "no ip explicit-path name WORD",
       NO_STR
       IP_STR
       IP_EXPLICIT_PATH_STR
       "Name of the path\n"
       "In Alphanumeric\n")
{
   struct rsvp_ip_path *rsvp_ip_path;

   /* Search if ip explicit path with that object exiists\n */
   rsvp_ip_path = rsvp_ip_path_search (argv[0]);

   /* If ip explicit-path with that name doesn't exist then error out. */
   if (!rsvp_ip_path) 
   {
      vty_out (vty, "ip explicit-path %s doesn't exist%s", argv[0], 
               VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* If this path is being used by tunnels then ref_cnt won't be zero.
      We can't delete and so send error. */
   if (rsvp_ip_path->ref_cnt)
   {
      vty_out (vty, "Can't delete ip explicit-path %s%s", argv[0], 
               VTY_NEWLINE);
      vty_out (vty, "It is being used by Tunnels. Remove the path from Tunnel \
               first%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* If found then delete the rsvp_ip_path . */
   if(rsvp_ip_path_delete (argv[0]))
   {
      vty_out (vty, "Can't delete ip explicit-path %s%s", argv[0], 
               VTY_NEWLINE);
      return CMD_WARNING;
   }

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "explicit-path name xyz" command installed in ip node. This
 * Changes the node to "config-ip-path#" and same handling as ip_explicit_path.
 *-------------------------------------------------------------------------*/
DEFUN (explicit_path,
       explicit_path_cmd,
       "explicit-path name WORD",
       "IP Explicit Path Information\n"
       "Name of the path\n"
       "In Alphanumeric\n")
{
  struct rsvp_ip_path *rsvp_ip_path;

   /* When this command is executed switch to "ip explicit-path" node. */
   //vty->node = IP_EXPLICIT_PATH_NODE;

   /* Search if the path already exists with the name. */
   rsvp_ip_path = rsvp_ip_path_search (argv[0]);
   
   /* If not found, then set create the path with this new name */
   if (!rsvp_ip_path)
   {
       /* This will create the rsvp_ip_path and will hook into the 
        * ternary serach trie at hook rm->rsvp->ip_path_obj. */
       rsvp_ip_path = rsvp_ip_path_create (argv[0]);
       
       /* Sanity Check.*/
       if (!rsvp_ip_path)
       {
          vty_out (vty, "ip explicit-path creation failed%s", VTY_NEWLINE);
          vty->node = CONFIG_NODE;
          return CMD_WARNING;
       }
   }
   /* Set the vty->index to rsvp_ip_path created. */
   vty->index = rsvp_ip_path;

   return CMD_SUCCESS;
 
}

/*---------------------------------------------------------------------------
 * Handler for "no explicit-path name xyz" command installed in ip node.
 *-------------------------------------------------------------------------*/
DEFUN (no_explicit_path,
       no_explicit_path_cmd,
       "no explicit-path name WORD",
       NO_STR
       "IP Explicit Path Information\n"
       "Name Of The Path\n"
       "In Alphanumeric\n")
{

   struct rsvp_ip_path *rsvp_ip_path;

   /* Search if ip explicit path with that object exiists\n */
   rsvp_ip_path = rsvp_ip_path_search (argv[0]);

   /* If ip explicit-path with that name doesn't exist then error out. */
   if (!rsvp_ip_path) 
   {
      vty_out (vty, "ip explicit-path %s doesn't exist%s", argv[0], 
               VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* If this path is being used by tunnels then ref_cnt won't be zero.
      We can't delete and so send error. */
   if (rsvp_ip_path->ref_cnt)
   {
      vty_out (vty, "Can't delete ip explicit-path %s%s", argv[0], 
               VTY_NEWLINE);
      vty_out (vty, "It is being used by Tunnels. Remove the path from Tunnel \
               first%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* If found then delete the rsvp_ip_path . */
   if(rsvp_ip_path_delete (argv[0]))
   {
      vty_out (vty, "Can't delete ip explicit-path %s%s", argv[0], 
               VTY_NEWLINE);
      return CMD_WARNING;
   }

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Handler for "next-address .." command.
 *--------------------------------------------------------------------------*/
DEFUN (next_address,
       next_address_cmd,
       "next-address (A.B.C.D) (strict|loose)",
       "Set next hop information for this path\n"
       "IPv4 address of this hop\n"
       "Type of hop is strict\n"
       "Type of hop is loose\n")
{
   /* Get the rsvp_ip_path as now we are at RSVP_IP_PATH_NODE. */
   struct rsvp_ip_path *rsvp_ip_path = vty->index;
   struct prefix hop;
   int hop_type;
  
   if (strncmp (argv[1], "s", 1) == 0)
      hop_type = RSVP_IP_HOP_STRICT;
   else
      hop_type = RSVP_IP_HOP_LOOSE;

   /* Get the address and convert into prefix. */
   if (!str2prefix (argv[0], &hop))
   {
      vty_out (vty, "%% Command execution failed%s", VTY_NEWLINE);
      return CMD_WARNING;
   }

   if(rsvp_ip_path_insert_hop (rsvp_ip_path, hop_type, &hop))
   { 
      vty_out (vty, "%% Insertion of next-address failed%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   return CMD_SUCCESS;
} 

/*--------------------------------------------------------------------------
 * Handler for "no next-addess.." command.
 *-------------------------------------------------------------------------*/
DEFUN (no_next_address,
       no_next_address_cmd,
       "no next-address (A.B.C.D) (strict|loose)",
       NO_STR
       "Delete this next-address from this path\n"
       "IPv4/v6 address of the hop\n"
       "Type of hop is strict\n"
       "Type of hop is loose\n")
{
   struct rsvp_ip_path *rsvp_ip_path = vty->index;
   struct prefix hop;
   int hop_type;
   
   if (strncmp (argv[1], "s", 1) == 0)
      hop_type = RSVP_IP_HOP_STRICT;
   else
      hop_type = RSVP_IP_HOP_LOOSE;

   if (!str2prefix (argv[0], &hop))
   {
      vty_out (vty, "%%Command execution failed%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
  
   if (rsvp_ip_path_delete_hop (rsvp_ip_path, hop_type, &hop))
   {
      vty_out (vty, "%%The hop to be deleted is not found%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
  
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "show ip explicit-path Name" command.
 *-------------------------------------------------------------------------*/
DEFUN (show_ip_explicit_path,
       show_ip_explicit_path_cmd,
       "show ip explicit-path [IFNAME]",
        SHOW_STR
        IP_STR
        "IP Explicit Path Information\n"
        "Path Name (Optional)\n")
{
   struct listnode *node;
   struct rsvp_ip_path *rsvp_ip_path;

   /* Check if specific path name is gievn.*/
   if (argc !=0)
   {
      rsvp_ip_path = rsvp_ip_path_search (argv[0]);
      
      /* If Path is not found then return error.*/
      if (!rsvp_ip_path) 
      {
         vty_out (vty, "Path with name %s not found %s", argv[0], VTY_NEWLINE);
         return CMD_WARNING;
      }
      /* Path is found. So display path information */
      rsvp_ip_path_dump_vty (vty, rsvp_ip_path);
      
      return CMD_SUCCESS;
   }
   
   /* No path name is specified, so dump all path names. */
   for (node = listhead ((struct list *)rm->rsvp->ip_path_list);
        node;
        nextnode (node))
   {
      rsvp_ip_path = (struct rsvp_ip_path *)getdata (node);
      rsvp_ip_path_dump_vty (vty, rsvp_ip_path);
   }
   
   return CMD_SUCCESS;
} 

/*---------------------------------------------------------------------------
 * Handler for 'interface tunnel N" command.
 *-------------------------------------------------------------------------*/
DEFUN (interface_tunnel,
       interface_tunnel_cmd,
       "interface tunnel [Number]",
       INTERFACE_STR
       RSVP_TUNNEL_STR
       "Interface number\n")
{
   u_int32_t tunnel_num;
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   //vty->node = INTERFACE_TUNNEL_NODE;
   vty->index = rm->rsvp;

   /* Check that interface number is provided. */
   if (argc == 0)
   {
      vty_out (vty, "Interface number not given%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* Get the Tunnel Number. */
   tunnel_num = atol (argv[1]);
   rsvp_tunnel_if = rsvp_tunnel_if_search (tunnel_num);
   
   /* If the Tunnel already exists. */
   if (rsvp_tunnel_if)
   {
      /* Set the vty->index to the rsvp_tunnel_if found. */
      vty->index = rsvp_tunnel_if;
      return CMD_SUCCESS;
   } 
   /* Create the Tunnel as its not present. */
   rsvp_tunnel_if = rsvp_tunnel_if_create (tunnel_num);

   if (!rsvp_tunnel_if)
   {
      vty_out (vty, "%%Interface Tunnel %d creation failed%s", tunnel_num,
               VTY_NEWLINE);   
      return CMD_WARNING;
   }

   vty->index = rsvp_tunnel_if;

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "no interface tunnel N" command.
 *-------------------------------------------------------------------------*/
DEFUN (no_interface_tunnel,
       no_interface_tunnel_cmd,
       "no interface tunnel [NUM]",
       NO_STR
       INTERFACE_STR
       RSVP_TUNNEL_STR
       "The tunnel number\n")
{
    u_int32_t tunnel_num;

    /* Check the Tunnel Interface is given. */
    if (argc == 0)
    {
       vty_out (vty, "%% Tunnel Interface Number Not Provided%s", VTY_NEWLINE);
       return CMD_WARNING;
    }

    tunnel_num = atol (argv[0]);
    
    if (rsvp_tunnel_if_delete (tunnel_num))
    {
       vty_out (vty, "%%Tunnel Interface %d Not Found%s", tunnel_num, 
                VTY_NEWLINE);
       return CMD_WARNING;
    }
    return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "tunnel mode mpls traffic-eng" command.
 *-------------------------------------------------------------------------*/
DEFUN (tunnel_mode_mpls_traffic_eng,
       tunnel_mode_mpls_traffic_eng_cmd,
       "tunnel mode mpls traffic-eng",
       RSVP_TUNNEL_STR
       "Mode of the tunnel\n"
       "MPLS Tunnel\n"
       "Setup this tunnel through RSVP-TE\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   /* Get the Tunnel Interface from vty. */
   rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* Enable RSVP-TE on this interface. */
   rsvp_tunnel_if_set_rsvp (rsvp_tunnel_if);

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Handler for "no tunnel mode mpls traffic-eng" command.
 *--------------------------------------------------------------------------*/
DEFUN (no_tunnel_mode_mpls_traffic_eng,
       no_tunnel_mode_mpls_traffic_eng_cmd,
       "no tunnel mode mpls traffic-eng",
       NO_STR
       RSVP_TUNNEL_STR
       "Mode of the tunnel\n"
       "MPLS Tunnel\n" 
       "Disable RSVP-TE on this tunnel\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   /* Get the rsvp_tunnel_if from vty. */
   rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* Disable RSVP-TE on this interface. */
   rsvp_tunnel_if_unset_rsvp (rsvp_tunnel_if);
 
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "tunnel destination A.B.C.D" command.
 *--------------------------------------------------------------------------*/
DEFUN (tunnel_destination,
       tunnel_destination_cmd,
       "tunnel destination (A.B.C.D)",
       RSVP_TUNNEL_STR
       "IPv4/v6 address of destination of the tunnel\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if;
   struct prefix dst;

   /* Get the tunnel interface rom vty->index. */
   rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* If destination address is not specified. in command */
   if (argc == 0)  
   {
      vty_out (vty, "%% Destination Address Value Not Specified%s", 
               VTY_NEWLINE);
      vty_out (vty, "Syntax :e.g  tunnel destination 10.10.10.1%s", 
               VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* Check if destination is already set and if yes, error out. */
   if (CHECK_FLAG (rsvp_tunnel_if->cflags, RSVP_TUNNEL_IF_CFLAG_DST))
   {
      vty_out (vty, "%%Destination Address Already Set%s", VTY_NEWLINE);
      return CMD_WARNING;
   } 
   /* Initialize the destination holder (prefix). */
   bzero (&dst, sizeof (struct prefix)); 
   /* Fill the prefix from the address string received from vty. */
   if(!str2prefix (argv[0], &dst))
   {
      vty_out (vty, "%%Command Execution Failed Due To Internal Error%s",
               VTY_NEWLINE);
      return CMD_WARNING;
   } 
   /* Set the destination. Config Checks are done within this. */
   rsvp_tunnel_if_set_dst (rsvp_tunnel_if, &dst);

   return CMD_SUCCESS;   
}       

/*--------------------------------------------------------------------------
 * Handler for "no tunnel destination A.B.C.D" command.
 *------------------------------------------------------------------------*/
DEFUN (no_tunnel_destination,
       no_tunnel_destination_cmd,
       "no tunnel destination (A.B.C.D)",
       NO_STR
       RSVP_TUNNEL_STR
       "IPv4/v6 address of destination of the tunnel\n")
{
   struct prefix dst;
   struct rsvp_tunnel_if *rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* Give error if no address is specified. */ 
   if (argc == 0)
   {
      vty_out (vty, "%% Destination Address Value Not Specified%s", 
               VTY_NEWLINE);
      vty_out (vty, "Syntax : e.g, no tunnel destination 10.10.10.1%s",
               VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* Check if any destination is set already.If not nothing to negate.*/
   if (RSVP_TUNNEL_IF_IS_DST_SET (rsvp_tunnel_if))
   {
      vty_out (vty, "%%No Destination Configured On This Tunnel%s",
               VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* Get the destination address into prefix. */
   bzero (&dst, sizeof (struct prefix));
   /* Get the destination address in the form of prefix. */
   if (!str2prefix (argv[0], &dst))
   {
      vty_out (vty, "%%Command Failed Due To Internal Error%s", VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* Check that this is indeed the destination address on rsvp_tunnel_if.*/
   if(memcmp (&dst, &rsvp_tunnel_if->destination, sizeof (struct prefix)))
   {
      vty_out (vty, "%%No destination address %s found on Tunnel %d%s",
               argv[0], rsvp_tunnel_if->num, VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* Unset the existing destination on the rsvp_tunnel_if. */
   rsvp_tunnel_if_unset_dst (rsvp_tunnel_if);
   
   return CMD_SUCCESS;   
}

/*----------------------------------------------------------------------------
 * Handler for "tunnel mpls traffic-eng cspf" command.
 *--------------------------------------------------------------------------*/
DEFUN (tunnel_mpls_traffic_eng_cspf,
       tunnel_mpls_traffic_eng_cspf_cmd,
       "tunnel mpls traffic-eng cspf",
       RSVP_TUNNEL_STR
       MPLS_STR
       RSVP_TE_STR
       "Enable CSPF (Constrained Shortest Path First)\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   /* Get the rsvp_tunnel_if from vty. */
   rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;
   
   /* Enabled CSPF on this interface. */
   rsvp_tunnel_if_set_cspf (rsvp_tunnel_if);
  
   return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * Handler for "no tunnel mpls traffic-eng cspf" command.
 *---------------------------------------------------------------------------*/
DEFUN (no_tunnel_mpls_traffic_eng_cspf,
       no_tunnel_mpls_traffic_eng_cspf_cmd,
       "no tunnel mpls traffic-eng cspf",
       NO_STR
       RSVP_TUNNEL_STR
       MPLS_STR
       RSVP_TE_STR
       "Disable CSPF (Constrained Shortest Path First)\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   /* Get the rsvp_tunnel_if from vty. */
   rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* Disable CSPF on this tunnel. */  
   rsvp_tunnel_if_unset_cspf (rsvp_tunnel_if);

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Handler for "tunnel mpls traffic-eng path-option N explicit xyz" command.
 *--------------------------------------------------------------------------*/
DEFUN (tunnel_mpls_traffic_eng_path_option,
       tunnel_mpls_traffic_eng_path_option_cmd,
       "tunnel mpls traffic-eng path-option <1-6> (explicit|dynamic) \
       (standby|no-standby) [PATHNAME]",
       RSVP_TUNNEL_STR
       MPLS_STR
       RSVP_TE_STR
       "Path Option Information \n"
       "Option Number\n"
       "Type Of Path Explicit\n"
       "Type Of Path Dynamic\n"
       "LSP in standby mode\n"
       "Explicit Path Name\n")

{
   struct rsvp_ip_path *path = NULL;
   struct rsvp_tunnel_if *rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;
   int path_option_num = 0;
   int path_option_type;
   int standby_flag = 0;
  
   /* Get the path option number from input argument. */
   path_option_num = atoi (argv[0]);

   /* Check if this path-option is already set. */
   if (rsvp_tunnel_if_path_option_is_valid (rsvp_tunnel_if, path_option_num))
   {
      vty_out (vty, "%%Path Option %d is already set on this Tunnel%s",
               path_option_num, VTY_NEWLINE);
      return CMD_WARNING;
   } 
   /* Get the path_option_type. */
   if (!strncmp (argv[1], "e", 1)) 
   { 
      path_option_type = RSVP_TUNNEL_IF_PATH_OPTION_EXPLICIT;

      /* If its explicit path then make sure that name is specified. */
      if (argc != 4)
      {
         vty_out (vty, "%%Explicit IP Path Name Not Specified%s", VTY_NEWLINE);
         return CMD_WARNING;
      } 
      /* As its explicit path search for path in the system.*/
      if (!(path = rsvp_ip_path_search (argv[3])))
      {
         vty_out (vty, "%% Explicit Path %s not found%s", argv[2], 
                  VTY_NEWLINE);
         return CMD_WARNING;
      }
      vty_out (vty, "Explicit path found%s", VTY_NEWLINE);
   }
   else
   {
      path_option_type = RSVP_TUNNEL_IF_PATH_OPTION_DYNAMIC;

      /* If its Dynamic Path, make sure name is NOT specified. */
      if (argc == 4)
      {
         vty_out (vty, "%%No Name Needed for Dynamic Path%s", VTY_NEWLINE);
         return CMD_WARNING;
      }
   }

   /* Check if its set as standby mode. */
   if (!strncmp (argv[2], "s", 1))
      standby_flag = 1;
   
 
   rsvp_tunnel_if_set_path_option (rsvp_tunnel_if,
                                   path_option_num,
                                   path_option_type,
                                   standby_flag,
                                   path);
   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Handler for "no tunnel mpls traffic-eng path-option N explicit xyx" command.
 *--------------------------------------------------------------------------*/
DEFUN (no_tunnel_mpls_traffic_eng_path_option,
       no_tunnel_mpls_traffic_eng_path_option_cmd,
       "no tunnel mpls traffic-eng path-option <1-6>",
       NO_STR
       RSVP_TUNNEL_STR
       MPLS_STR
       RSVP_TE_STR
       "Path Option Information\n"
       "Option Number\n")
{
   int path_option_num = 0;
   /* Get the Tunnel Interface From VTY. */
   struct rsvp_tunnel_if *rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* Get the path option from the option number as input argument.*/
   path_option_num = atoi (argv[0]);
   
   /* Check if the Tunnel contains a path option with this number. */
   if (!rsvp_tunnel_if_path_option_is_valid (rsvp_tunnel_if, path_option_num))
   {
      vty_out (vty, "%%Can't delete path option %d%s", path_option_num,
               VTY_NEWLINE);
      vty_out (vty, "It is not set on this Tunnel%s", VTY_NEWLINE);
      
      return CMD_WARNING;
   }  

   /* Delete the path option from this Tunnel.*/
   rsvp_tunnel_if_unset_path_option (rsvp_tunnel_if, path_option_num);

   return CMD_SUCCESS;
}        

/*----------------------------------------------------------------------------
 * Handler for "shutdown" command on Interface Tunnel Node.
 *---------------------------------------------------------------------------*/
DEFUN (interface_tunnel_shutdown,
       interface_tunnel_shutdown_cmd,
       "shutdown",
       "Set This Tunnel Administraively DOWN\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;
   
   /* Check if it is already down.then don't do anuthing. */
   if (RSVP_TUNNEL_IF_IS_ADMIN_UP (rsvp_tunnel_if))
      /* Set the Interface Tunnel as administraively down. */
      rsvp_tunnel_if_admin_down (rsvp_tunnel_if);

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Handler for "no shutdown" command on Interface Tunnel Node.
 *--------------------------------------------------------------------------*/
DEFUN (no_interface_tunnel_shutdown,
       no_interface_tunnel_shutdown_cmd,
       "no shutdown",
       "Set This Tunnel Administratively UP\n")
{
   struct rsvp_tunnel_if *rsvp_tunnel_if = (struct rsvp_tunnel_if *)vty->index;

   /* Check if the Tunnel is ADMIN down , if yes make it UP. */
   if (!RSVP_TUNNEL_IF_IS_ADMIN_UP (rsvp_tunnel_if))
      rsvp_tunnel_if_admin_up (rsvp_tunnel_if);

   return CMD_SUCCESS;  
}
/*-----------------------------------
 * This is a wrapper function. 
 *----------------------------------*/
int 
rsvp_interface_tunnel_from_vty (struct vty *vty,
                                int argc,
                                char **argv)
{
   int ret;

   /* We expect 2 arguments for "interface tunnel x" command. */
   if (argc < 2 || argc > 2)
   {
      vty_out (vty, "Command Syntax Invalid%s", VTY_NEWLINE);
      vty_out (vty, "Usage: inrerface tunnel [tunnel number]%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   /* Call the actual command handler now. */
   ret = interface_tunnel_cmd.func (&interface_tunnel_cmd, vty, argc, argv);
   return ret;
}

/*----------------------------------------------------------------------------
 * Handler for "show interface Tunnel x" command. This will not be called 
 * directly, rather will be called by rsvp_show_interface_tunnel_from_vty
 * defined in rsvp_vty.c. That function in turn will ba called by 
 * rsvp_show_interface defined in rsvp_if.c while doing "show interface"
 * Well, this is not a very good design, but need this workaround to provide
 * a uniform view of Tunnels as interfaces. This implementation can be 
 * improved to alternate ones later.
 *--------------------------------------------------------------------------*/
DEFUN (show_interface_tunnel,
       show_interface_tunnel_cmd,
       "show interface tunnel [IFNUM]",
       SHOW_STR
       "Interface Information\n"
       "Tunnel Interface Information\n"
       "Tunnel number")
{
   u_int32_t tunnel_num;
   struct rsvp_tunnel_if *rsvp_tunnel_if;

   /* Check if only specific Tunnel Interface is to be shown or all. */
   if (argc != 2)
      return CMD_WARNING;
      
   tunnel_num = atol (argv[1]);
   printf ("[show_interface_tunnel_cmd] tunnel number is %d\n", tunnel_num);
   rsvp_tunnel_if = rsvp_tunnel_if_search (tunnel_num);

   /* Error if the tunnel interface is not found. */     
   if (!rsvp_tunnel_if)
   {
      vty_out (vty, "%%The Tunnel Interface %d Not Found%s", tunnel_num,
               VTY_NEWLINE);
      return CMD_WARNING;
   }
     
   rsvp_tunnel_if_dump_vty (vty, rsvp_tunnel_if);
   
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_show_interface_tunnel_from_vty
 * Input    : vty = Pointer to object of type struct vty
 *          : argc = Number of arguments.
 *          : argv = arguments vector 
 * Output   : Returns CMD_SUCCESS if the command is executed successfully, else
 *            returns CMD_WARNING.
 * Synopsis : This function is called to when a rrequest to show a RSVP
 *            tunnel interface is received from CLI.
 * Callers  ; rsvp_interface function in rsvp_if.c
 *-------------------------------------------------------------------------*/
int 
rsvp_show_interface_tunnel_from_vty (struct vty *vty,
                                     int argc,
                                     char **argv)
{
   int ret;

   /* Error Check we expect minimum 2 arguments "tunne" and number "x". */
   if (argc != 2)
   {
      vty_out (vty, "%%Invalid Syntax %s", VTY_NEWLINE);
      vty_out (vty, "[Usage] show interface tunnel [tunnel number]%s",
               VTY_NEWLINE);
      return CMD_WARNING;
   }
  
   ret = show_interface_tunnel_cmd.func (&show_interface_tunnel_cmd,
                                         vty, 
                                         argc, 
                                         argv);

   return ret; 
}
/*---------------------------------------------------------------------------
 * Command Nodes installed by this module.
 *-------------------------------------------------------------------------*/
/*
struct cmd_node mpls_node =
{
   MPLS_NODE,
   "%s(config-mpls)# ",
   1
};
*/
struct cmd_node rsvp_node =
{
   RSVP_NODE,
   "%s(config-rsvp)# ",
   1
};

struct cmd_node ip_node =
{
   IP_NODE,
   "%s(config)# ",
   1
};
/*
struct cmd_node ip_explicit_path_node =
{
   IP_EXPLICIT_PATH_NODE,
   "%s(config-ip-path)# ",
   1
};
*/
struct cmd_node interface_tunnel_node =
{
   INTERFACE_NODE,
   "%s(config-if)# ",
   1
};


/*----------------------------------------------------------------------------
 * Function : mpls_config_write
 * Input    : vty = Pointer to stryct vty
 * Output   : Always returns 0.
 * Synopsis : This function writes mpls global configuratio into file.
 * Callers  : mpls global command handlers in rsvpd.c
 *--------------------------------------------------------------------------*/
int
mpls_config_write (struct vty *vty)
{
   struct rsvp *rsvp = rm->rsvp;
   char addr[32];

   if (CHECK_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_LOOPBACK))
      vty_out (vty, "mpls router-id loopback %s", VTY_NEWLINE);
   else if (CHECK_FLAG (rsvp->cflags, RSVP_CFLAG_ROUTER_ID_ADDR))
   {
      prefix2str (&rsvp->router_id, addr, 32);
      vty_out (vty, "mpls router-id %s%s", addr, VTY_NEWLINE);
   }

   return 0;
}

int 
rsvp_config_write (struct vty *vty)
{
   return 0;
} 

/*----------------------------------------------------------------------------
 * Function : ip_config_write 
 * Input    : vty = Pointer to object of struct vty.
 * Output   : Returns number of lines written to config file.
 * Synopsis : This is the main function for writing configs into config file.
 *            It is called for IP_NODE and all its subnodes. Currently we have
 *            one subnode IP_EXPLICIT_PATH_NODE.
 * Callers  : The config write function for IP_NODE and IP_EXPLICIT_PATH_NODE.
 *--------------------------------------------------------------------------*/
int 
ip_config_write (struct vty *vty)
{
   int write = 0;
   struct rsvp_ip_path *rsvp_ip_path = NULL;
   struct listnode *node = NULL;
   struct list *ip_path_list = (struct list *)(rm->rsvp->ip_path_list);
 
   vty_out (vty, "ip%s", VTY_NEWLINE);
 
   for (node = listhead (ip_path_list); node; nextnode (node))
   {
      rsvp_ip_path = (struct rsvp_ip_path *) getdata (node); 
      write += ip_config_write_explicit_path (vty, rsvp_ip_path);
   }

   vty_out (vty, "!%s", VTY_NEWLINE);
   return write;
}

int ip_explicit_path_config_write (struct vty *vty)
{

   return 0;
}

/*---------------------------------------------------------------------------
 * Configuration write function for "interface Tunnel X".
 *-------------------------------------------------------------------------*/
int
interface_tunnel_config_write (struct vty *vty)
{
   struct rsvp_tunnel_if *rsvp_tunnel_if = NULL;
   int write = 0;
   int flag = 1;  
  
   vty_out (vty, "!%s", VTY_NEWLINE);
   write++;
   /* Traverse through all rsvp tunnel interfaces and sent to vty for write. */
   while (1)
   {
      rsvp_tunnel_if = rsvp_tunnel_if_traverse (flag);
   
      /* Check if traverse ends. */
      if (!rsvp_tunnel_if) 
         break;
      write = rsvp_tunnel_if_config_write (vty, rsvp_tunnel_if);
      vty_out (vty, "%s", VTY_NEWLINE);
      write++;
      if (flag)
         flag = 0;
   }
   return 0; 
}

/*----------------------------------------------------------------------------
 * Function : rsvp_vty_init
 * Input    : None
 * Output   : None
 * Synopsis : This fuction installs commands for configuring this module. It 
 *            is called only once during initialization of RSVP-TE.
 * Callers  : rsvp_init in rsvpd.c  
 *--------------------------------------------------------------------------*/
void
rsvp_vty_init ()
{
   /* Install the nodes from this module. */

   install_node (&interface_tunnel_node, interface_tunnel_config_write);

   install_element (CONFIG_NODE, &interface_cmd);
   install_element (CONFIG_NODE, &no_interface_cmd);
   install_default (INTERFACE_NODE);

   /* "description" commands. */
   install_element (INTERFACE_NODE, &interface_desc_cmd);
   install_element (INTERFACE_NODE, &no_interface_desc_cmd);


   //install_node (&mpls_node, mpls_config_write);
   install_node (&rsvp_node, rsvp_config_write);
   //install_node (&ip_node, ip_config_write);
   //install_node (&ip_explicit_path_node, NULL);
   //install_node (&interface_tunnel_node, interface_tunnel_config_write);

   /* Install default VTY commands to these new nodes. */
   //install_default (MPLS_NODE);
   install_default (RSVP_NODE);
   install_default (IP_NODE);
   //install_default (IP_EXPLICIT_PATH_NODE);
   //install_default (INTERFACE_TUNNEL_NODE);

   install_element (VIEW_NODE, &show_ip_route_cmd);
   install_element (ENABLE_NODE, &show_ip_route_cmd);
   install_element (CONFIG_NODE, &show_ip_route_cmd);

   /* Install the config node commands. */
   //install_element (CONFIG_NODE, &mpls_cmd);
   install_element (CONFIG_NODE, &rsvp_cmd);
   //install_element (CONFIG_NODE, &ip_cmd);

   install_element (CONFIG_NODE, &mpls_router_id_loopback_cmd);
   install_element (CONFIG_NODE, &no_mpls_router_id_loopback_cmd);
   install_element (CONFIG_NODE, &mpls_router_id_addr_cmd);
   install_element (CONFIG_NODE, &no_mpls_router_id_addr_cmd);
   /*
   install_element (MPLS_NODE,   &router_id_loopback_cmd);
   install_element (MPLS_NODE,   &no_router_id_loopback_cmd);
   install_element (MPLS_NODE, 	 &router_id_addr_cmd);
   install_element (MPLS_NODE,	 &no_router_id_addr_cmd);
   */
   //install_element (CONFIG_NODE, &no_mpls__router_id_loopback_cmd);

   install_element (IP_NODE, &explicit_path_cmd);
   install_element (IP_NODE, &no_explicit_path_cmd);

   install_element (CONFIG_NODE, &ip_explicit_path_cmd);
   install_element (CONFIG_NODE, &no_ip_explicit_path_cmd);

   install_element (VIEW_NODE,   &show_ip_explicit_path_cmd);
   install_element (ENABLE_NODE, &show_ip_explicit_path_cmd);
   install_element (CONFIG_NODE, &show_ip_explicit_path_cmd);
#if 0
   install_element (CONFIG_NODE, &rsvp_interface_tunnel_cmd);
#endif
  
   install_element (CONFIG_NODE, &next_address_cmd);
   install_element (CONFIG_NODE, &no_next_address_cmd);

   install_element (CONFIG_NODE, &interface_tunnel_cmd);
   install_element (CONFIG_NODE, &no_interface_tunnel_cmd);

   install_element (CONFIG_NODE,
                    &tunnel_mode_mpls_traffic_eng_cmd);
   install_element (CONFIG_NODE,
                    &no_tunnel_mode_mpls_traffic_eng_cmd);

   install_element (CONFIG_NODE, &tunnel_destination_cmd);
   install_element (CONFIG_NODE, &no_tunnel_destination_cmd);

   install_element (CONFIG_NODE,
                    &tunnel_mpls_traffic_eng_cspf_cmd);
   install_element (CONFIG_NODE,
                    &no_tunnel_mpls_traffic_eng_cspf_cmd);

   install_element (CONFIG_NODE,
                    &tunnel_mpls_traffic_eng_path_option_cmd);
   install_element (CONFIG_NODE,
                    &no_tunnel_mpls_traffic_eng_path_option_cmd);
   install_element (CONFIG_NODE, &interface_tunnel_shutdown_cmd);
   install_element (CONFIG_NODE, &no_interface_tunnel_shutdown_cmd);
   return;
}
