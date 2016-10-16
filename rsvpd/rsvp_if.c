/* RSVP Interface Handling Procedure Libraries.
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

#include <zebra.h>

#include "memory.h"
#include "linklist.h"
#include "if.h"
#include "command.h"
#include "vty.h"
#include "prefix.h"
#include "avl.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_if.h"
#include "rsvpd/rsvp_vty.h"

/*----------------------------------------------------------------------------
 * Static Function Prototypes.
 *--------------------------------------------------------------------------*/
static int rsvp_if_create (struct interface *);
static int rsvp_if_destroy (struct interface *);
static int rsvp_if_up (struct interface *);
static int rsvp_if_down (struct interface *);
static int rsvp_if_ip_en (struct interface *);
static int rsvp_if_ip_dis (struct interface *);

/*----------------------------------------------------------------------------
 * Function : rsvp_if_event_handle
 * Input    : event = type of event
 *            if = Pointer to the interface on which the event is received.
 * Output   : returns -0 if event is successful, else return false.  
 * Synopsis : This gateway function that traps an interface specific event 
 *            from if module.
 * Callers  : The event functions in rsvp_zebra.c
 *--------------------------------------------------------------------------*/
int
rsvp_if_event_handle (rsvp_if_event event,
                      struct interface *ifp)
{
   /* Handle Each Event separately. */
   switch (event)
   {
      case RSVP_IF_EVENT_IF_ADD:
         return rsvp_if_create (ifp);        

      case RSVP_IF_EVENT_IF_DEL:
         return rsvp_if_destroy (ifp);

      case RSVP_IF_EVENT_UP:
         return rsvp_if_up (ifp);

      case RSVP_IF_EVENT_DOWN:
         return rsvp_if_down (ifp);

      case RSVP_IF_EVENT_IP_ADD:
         return rsvp_if_ip_en (ifp);

      case RSVP_IF_EVENT_IP_DEL:
         return rsvp_if_ip_dis (ifp);

      default: 
         return 0;
   }  

}

/*----------------------------------------------------------------------------
 * Function : rsvp_if_up
 * Input    : ifp = Pointer to struct interface
 * Output   : On success returns 0, else return -1
 * Synopsis : This function handles the interface UP event.
 * Callers  : rsvp_if_event_handle in rsvp_if.c
 *---------------------------------------------------------------------------*/
static int 
rsvp_if_up (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   /* Sanity Check. */
   if (!ifp)
      return -1;

   rsvp_if = (struct rsvp_if *)ifp->info;
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_UP);

   /* Do further Handling. */
   return 0;
};

/*----------------------------------------------------------------------------
 * Function : rsvp_if_down
 * Input    ; ifp = Pointer to struct interface
 * Output   : Returns 0 on success, on error return -1
 * Synopsis : This function handles the interface DOWN event.
 * Callers  : rsvp_if_event_handle in rsvp_if.c
 *--------------------------------------------------------------------------*/
static int
rsvp_if_down (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   /* Sanity Check. */
   if (!ifp)
      return -1;

   rsvp_if = (struct rsvp_if *)ifp->info;
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_UP);

   /* Do futher handling. */
   return 0;

};

/*-----------------------------------------------------------------------------
 * Function ; rsvp_if_ip_en 
 * Input    : ifp = Pointer to struct interface
 * Output   : Returns 0 on success , on error returns -1
 * Synopsis : This functions handles when interface is enabled for IP event.
 * Callers  : rsvp_if_event_handle in rsvp_if.c
 *---------------------------------------------------------------------------*/
static int
rsvp_if_ip_en (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   /* Sanity Check. */
   if (!ifp)
      return -1;
   rsvp_if = (struct rsvp_if *)ifp->info;
   
   /* If already IP enabled then return from here. */
   if (!CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_IP))
      return 0;

   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_IP);

   /* Do further handling. */
   return 0;
}

/*----------------------------------------------------------------------------
 * Function ; rsvp_if_ip_dis
 * Input    : ifp = Pointer to struct interface
 * Output   : Returns 0 on success, else returns -1.
 * Synopsis : This function handles when a interface is disabled for IP.
 * Callers  : rsvp_if_event_handle in rsvp_if.c
 *--------------------------------------------------------------------------*/
static int
rsvp_if_ip_dis (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   /* Sanity Check. */
   if (!ifp)
      return -1;
   rsvp_if = (struct rsvp_if *)ifp->info;

   /* If this is the last IP address deleted then disable IP. */
   if (CHECK_FLAG(rsvp_if->sflags, RSVP_IF_SFLAG_IP) && !ifp->connected)
      UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_IP);

   /* Do further handling. */
   return 0;
};

/*----------------------------------------------------------------------------
 * Function : rsvp_if_create 
 * Input    : ifp = Pointer to struct interface
 * Output   : Returns 0 on success else returns -1.
 * Synopsis ; This function allocates a new rsvp_if object on if->info.
 * Callers  : rsvp_if_enable_rsvp in rsvp_if.c
 *--------------------------------------------------------------------------*/
static int
rsvp_if_create (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   
   /* Sanity Check. */
   rsvp_if = XCALLOC (MTYPE_RSVP_IF, sizeof (struct rsvp_if));
   if (!rsvp_if)
      return -1;
   ifp->info = rsvp_if;

   /* Set all default values. */
   rsvp_if->v_refresh_interval = RSVP_REFRESH_INTERVAL_DEFAULT;
   rsvp_if->v_refresh_multiple = RSVP_REFRESH_MULTIPLE_DEFAULT;
   rsvp_if->v_hello_interval   = RSVP_HELLO_INTERVAL_DEFAULT;
   rsvp_if->v_hello_persist    = RSVP_HELLO_PERSIST_DEFAULT;
   rsvp_if->v_hello_tolerance  = RSVP_HELLO_TOLERANCE_DEFAULT;

   return 0;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_if_destroy
 * Input    : ifp = Pointer to struct interface.
 * Output   : Returns 0 on success else returns -1.
 * Synopsis : This function gracefully cleans up a rsvp_if and the event
 *            is generated.
 * Callers  : rsvp_if_handle_event in rsvp_if.c
 *--------------------------------------------------------------------------*/
static int
rsvp_if_destroy (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;

   /* Sanity Check. */
   if (!ifp)
      return -1;

   /* Start the Hello Timer. */
      //TBD

   /* Signal event to all modules interested. */
   XFREE (MTYPE_RSVP_IF, rsvp_if);
   
   return 0;
}
/*-----------------------------------------------------------------------------
 * Function : rsvp_if_enable_rsvp
 * Input    : ifp = Pointer to the physical interface.
 * Output   : None
 * Synopsis : This function enables RSVP pn a physical interface. It creates
 *            a new rsvp_if, initializes it and attaches.
 * Callers  : 
 *---------------------------------------------------------------------------*/
void
rsvp_if_enable_rsvp (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   /* Sanity Check. */
   if (!ifp)
      return;
   rsvp_if = (struct rsvp_if *)ifp->info;
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RSVP);

   /* Generate Events for SoftState Module. */
   return;
}

/*------------------------------------------------------------------------------
 * Function : rsvp_if_disable_rsvp
 * Input    : ifp = Pointer to the physical interface
 * Output   : None.
 * Synopsis : This function disables RSVP on this interface.
 * Callers  :
 *----------------------------------------------------------------------------*/
void 
rsvp_if_disable_rsvp (struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   /* Sanity Check. */
   if (!ifp)
      return;
   rsvp_if = (struct rsvp_if *)ifp->info;
   
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RSVP);

   /* Generate Events for Soft State Module. */

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_if_get_addr
 * Input    : rsvp_if = Pointer to object of type struct rsvp_if.
 * Output   : Pointer to struct prefix 
 * Synopsis : This utility function extracts the IP address of the interface
 *            (rsvp_if->ifp).
 * Callers  : (TBD)..
 *---------------------------------------------------------------------------*/
struct prefix *
rsvp_if_get_addr (struct rsvp_if *rsvp_if)
{
   struct listnode *node;
   /* Sanity Check. */
   if (!rsvp_if)
      return NULL;
   struct interface *ifp = rsvp_if->ifp;

   node = listhead (ifp->connected);
   
   if (node)
      return ((struct prefix *)getdata (node));
   else
      return NULL;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_if_lookup_by_local_addr
 * Input    ; addr = Value of in_addr
 * Output   : Returns pointer to object of type struct rsvp_if.
 * Synopsis : Given an in_addr this API returns the rsvp_if that has this IP 
 *            as local address.
 * Callers  : rsvp_read.
 *--------------------------------------------------------------------------*/
struct rsvp_if *
rsvp_if_lookup_by_local_addr (struct in_addr addr)
{
   struct interface *ifp;
     
   ifp = if_lookup_address (addr);
   
   if(!ifp)
      return NULL;
   
   return (struct rsvp_if *)ifp->info; 
}
 
/*----------------------------------------------------------------------------
 * Function : rsvp_if_flag_dump_vty
 * Input    : vty = Pointer to struct vty
 *            flag = unsigned long
 * Output   : None
 * Synopsis : Printout flag information into vty 
 *---------------------------------------------------------------------------*/
void
rsvp_if_flag_dump_vty (struct vty *vty, 
                       unsigned long flag)
{
  int separator = 0;

#define IFF_OUT_VTY(X, Y) \
  if ((X) && (flag & (X))) \
    { \
      if (separator) \
	vty_out (vty, ","); \
      else \
	separator = 1; \
      vty_out (vty, Y); \
    }

  vty_out (vty, "<");
  IFF_OUT_VTY (IFF_UP, "UP");
  IFF_OUT_VTY (IFF_BROADCAST, "BROADCAST");
  IFF_OUT_VTY (IFF_DEBUG, "DEBUG");
  IFF_OUT_VTY (IFF_LOOPBACK, "LOOPBACK");
  IFF_OUT_VTY (IFF_POINTOPOINT, "POINTOPOINT");
  IFF_OUT_VTY (IFF_NOTRAILERS, "NOTRAILERS");
  IFF_OUT_VTY (IFF_RUNNING, "RUNNING");
  IFF_OUT_VTY (IFF_NOARP, "NOARP");
  IFF_OUT_VTY (IFF_PROMISC, "PROMISC");
  IFF_OUT_VTY (IFF_ALLMULTI, "ALLMULTI");
  IFF_OUT_VTY (IFF_OACTIVE, "OACTIVE");
  IFF_OUT_VTY (IFF_SIMPLEX, "SIMPLEX");
  IFF_OUT_VTY (IFF_LINK0, "LINK0");
  IFF_OUT_VTY (IFF_LINK1, "LINK1");
  IFF_OUT_VTY (IFF_LINK2, "LINK2");
  IFF_OUT_VTY (IFF_MULTICAST, "MULTICAST");
  vty_out (vty, ">");
}

/*----------------------------------------------------------------------------
 * Function : rsvp_if_prefix_vty_out 
 * Input    : vty = Pointer to struct vty
 * Output   : p = Pointer to struct vty
 * Synopsis : Output prefix string to vty. 
 *--------------------------------------------------------------------------*/
int
rsvp_if_prefix_vty_out (struct vty *vty, 
                        struct prefix *p)
{
  char str[INET6_ADDRSTRLEN];

  inet_ntop (p->family, &p->u.prefix, str, sizeof (str));
  vty_out (vty, "%s", str);
  return strlen (str);
}

/*----------------------------------------------------------------------------
 * Function : rsvp_if_connected_dump_vty
 * Input    : vty = Pointer to struct vty
 *            connected = Pointer to struct connected
 * Output   : None
 * Synopsis : This function dumps IP information on an interface to vty.
 *--------------------------------------------------------------------------*/
void
rsvp_if_connected_dump_vty (struct vty *vty, 
                            struct connected *connected)
{
  struct prefix *p;
  struct interface *ifp;

  /* Set interface pointer. */
  ifp = connected->ifp;

  /* Print interface address. */
  p = connected->address;
  vty_out (vty, "  %s ", prefix_family_str (p));
  rsvp_if_prefix_vty_out (vty, p);
  vty_out (vty, "/%d", p->prefixlen);

  /* If there is destination address, print it. */
  p = connected->destination;
  if (p)
    {
      if (p->family == AF_INET)
	if (ifp->flags & IFF_BROADCAST)
	  {
	    vty_out (vty, " broadcast ");
	    rsvp_if_prefix_vty_out (vty, p);
	  }

      if (ifp->flags & IFF_POINTOPOINT)
	{
	  vty_out (vty, " pointopoint ");
	  rsvp_if_prefix_vty_out (vty, p);
	}
    }

  if (CHECK_FLAG (connected->flags, ZEBRA_IFA_SECONDARY))
    vty_out (vty, " secondary");

  if (connected->label)
    vty_out (vty, " %s", connected->label);

  vty_out (vty, "%s", VTY_NEWLINE);
}

#ifdef RTADV
/*----------------------------------------------------------------------------
 * Function : rsvp_if_nd_dump_vty
 * Input    : vty = Pointer to struct vty
 *          : ifp = Pointer to struct interface
 * Output   : None
 * Synopsis : Dump interface ND information to vty. 
 *--------------------------------------------------------------------------*/
void
rsvp_if_nd_dump_vty (struct vty *vty, 
                     struct interface *ifp)
{
  struct zebra_if *zif;
  struct rtadvconf *rtadv;

  zif = (struct zebra_if *) ifp->info;
  rtadv = &zif->rtadv;

  if (rtadv->AdvSendAdvertisements)
    {
      vty_out (vty, "  ND advertised reachable time is %d milliseconds%s",
	       rtadv->AdvReachableTime, VTY_NEWLINE);
      vty_out (vty, "  ND advertised retransmit interval is %d milliseconds%s",
	       rtadv->AdvRetransTimer, VTY_NEWLINE);
      vty_out (vty, "  ND router advertisements are sent every %d seconds%s",
	       rtadv->MaxRtrAdvInterval, VTY_NEWLINE);
      vty_out (vty, "  ND router advertisements live for %d seconds%s",
	       rtadv->AdvDefaultLifetime, VTY_NEWLINE);
      if (rtadv->AdvManagedFlag)
	vty_out (vty, "  Hosts use DHCP to obtain routable addresses.%s",
		 VTY_NEWLINE);
      else
	vty_out (vty, "  Hosts use stateless autoconfig for addresses.%s",
		 VTY_NEWLINE);
    }
}
#endif /* RTADV */

/*---------------------------------------------------------------------------
 * Function : rsvp_if_dump_vty
 * Input    : vty = Pointer to struct vty
 *          : ifp = Pointer to struct interface
 * Output   : None
 * Synopsis : This function dumps an interface's info onto vty.
 * Callers  : show_interface_cmd in rsvp_if.c
 *--------------------------------------------------------------------------*/
void
rsvp_if_dump_vty (struct vty *vty, 
                  struct interface *ifp)
{
#ifdef HAVE_SOCKADDR_DL
  struct sockaddr_dl *sdl;
#endif /* HAVE_SOCKADDR_DL */
  struct connected *connected;
  struct listnode *node;

  vty_out (vty, "Interface %s%s", ifp->name,
	   VTY_NEWLINE);
  if (ifp->desc)
    vty_out (vty, "  Description: %s%s", ifp->desc,
	     VTY_NEWLINE);
  if (ifp->ifindex <= 0)
    {
      vty_out(vty, "  index %d pseudo interface%s", ifp->ifindex, VTY_NEWLINE);
      return;
    }
  else if (! CHECK_FLAG (ifp->status, ZEBRA_INTERFACE_ACTIVE))
    {
      vty_out(vty, "  index %d inactive interface%s", 
	      ifp->ifindex, 
	      VTY_NEWLINE);
      return;
    }

  vty_out (vty, "  index %d metric %d mtu %d ",
	   ifp->ifindex, ifp->metric, ifp->mtu);
  rsvp_if_flag_dump_vty (vty, ifp->flags);
  vty_out (vty, "%s", VTY_NEWLINE);

  /* Hardware address. */
#ifdef HAVE_SOCKADDR_DL
  sdl = &ifp->sdl;
  if (sdl != NULL && sdl->sdl_alen != 0)
    {
      int i;
      u_char *ptr;

      vty_out (vty, "  HWaddr: ");
      for (i = 0, ptr = LLADDR (sdl); i < sdl->sdl_alen; i++, ptr++)
	vty_out (vty, "%s%02x", i == 0 ? "" : ":", *ptr);
      vty_out (vty, "%s", VTY_NEWLINE);
    }
#else
  if (ifp->hw_addr_len != 0)
    {
      int i;

      vty_out (vty, "  HWaddr: ");
      for (i = 0; i < ifp->hw_addr_len; i++)
	vty_out (vty, "%s%02x", i == 0 ? "" : ":", ifp->hw_addr[i]);
      vty_out (vty, "%s", VTY_NEWLINE);
    }
#endif /* HAVE_SOCKADDR_DL */
  
  /* Bandwidth in kbps */
  if (ifp->bandwidth != 0)
    {
      vty_out(vty, "  bandwidth %u kbps", ifp->bandwidth);
      vty_out(vty, "%s", VTY_NEWLINE);
    }

  for (node = listhead (ifp->connected); node; nextnode (node))
    {
      connected = getdata (node);
      if (CHECK_FLAG (connected->conf, ZEBRA_IFC_REAL))
	rsvp_if_connected_dump_vty (vty, connected);
    }

#ifdef RTADV
  rsvp_if_nd_dump_vty (vty, ifp);
#endif /* RTADV */

#ifdef HAVE_PROC_NET_DEV
  /* Statistics print out using proc file system. */
  vty_out (vty, "    input packets %lu, bytes %lu, dropped %lu,"
	   " multicast packets %lu%s",
	   ifp->stats.rx_packets, ifp->stats.rx_bytes, 
	   ifp->stats.rx_dropped, ifp->stats.rx_multicast, VTY_NEWLINE);

  vty_out (vty, "    input errors %lu, length %lu, overrun %lu,"
	   " CRC %lu, frame %lu, fifo %lu, missed %lu%s",
	   ifp->stats.rx_errors, ifp->stats.rx_length_errors,
	   ifp->stats.rx_over_errors, ifp->stats.rx_crc_errors,
	   ifp->stats.rx_frame_errors, ifp->stats.rx_fifo_errors,
	   ifp->stats.rx_missed_errors, VTY_NEWLINE);

  vty_out (vty, "    output packets %lu, bytes %lu, dropped %lu%s",
	   ifp->stats.tx_packets, ifp->stats.tx_bytes,
	   ifp->stats.tx_dropped, VTY_NEWLINE);

  vty_out (vty, "    output errors %lu, aborted %lu, carrier %lu,"
	   " fifo %lu, heartbeat %lu, window %lu%s",
	   ifp->stats.tx_errors, ifp->stats.tx_aborted_errors,
	   ifp->stats.tx_carrier_errors, ifp->stats.tx_fifo_errors,
	   ifp->stats.tx_heartbeat_errors, ifp->stats.tx_window_errors,
	   VTY_NEWLINE);

  vty_out (vty, "    collisions %lu%s", ifp->stats.collisions, VTY_NEWLINE);
#endif /* HAVE_PROC_NET_DEV */

#ifdef HAVE_NET_RT_IFLIST
#if defined (__bsdi__) || defined (__NetBSD__)
  /* Statistics print out using sysctl (). */
  vty_out (vty, "    input packets %qu, bytes %qu, dropped %qu,"
	   " multicast packets %qu%s",
	   ifp->stats.ifi_ipackets, ifp->stats.ifi_ibytes,
	   ifp->stats.ifi_iqdrops, ifp->stats.ifi_imcasts,
	   VTY_NEWLINE);

  vty_out (vty, "    input errors %qu%s",
	   ifp->stats.ifi_ierrors, VTY_NEWLINE);

  vty_out (vty, "    output packets %qu, bytes %qu, multicast packets %qu%s",
	   ifp->stats.ifi_opackets, ifp->stats.ifi_obytes,
	   ifp->stats.ifi_omcasts, VTY_NEWLINE);

  vty_out (vty, "    output errors %qu%s",
	   ifp->stats.ifi_oerrors, VTY_NEWLINE);

  vty_out (vty, "    collisions %qu%s",
	   ifp->stats.ifi_collisions, VTY_NEWLINE);
#else
  /* Statistics print out using sysctl (). */
  vty_out (vty, "    input packets %lu, bytes %lu, dropped %lu,"
	   " multicast packets %lu%s",
	   ifp->stats.ifi_ipackets, ifp->stats.ifi_ibytes,
	   ifp->stats.ifi_iqdrops, ifp->stats.ifi_imcasts,
	   VTY_NEWLINE);

  vty_out (vty, "    input errors %lu%s",
	   ifp->stats.ifi_ierrors, VTY_NEWLINE);

  vty_out (vty, "    output packets %lu, bytes %lu, multicast packets %lu%s",
	   ifp->stats.ifi_opackets, ifp->stats.ifi_obytes,
	   ifp->stats.ifi_omcasts, VTY_NEWLINE);

  vty_out (vty, "    output errors %lu%s",
	   ifp->stats.ifi_oerrors, VTY_NEWLINE);

  vty_out (vty, "    collisions %lu%s",
	   ifp->stats.ifi_collisions, VTY_NEWLINE);
#endif /* __bsdi__ || __NetBSD__ */
#endif /* HAVE_NET_RT_IFLIST */
}

/*--------------------------------------------------------------------------
 * Function : rsvp_if_dump_private_info_vty
 * Input    : vty = Pointer to struct vty
 *            ifp = Pointer to struct interface
 * Output   : None
 * Synopsis : This function prints ifp->info (struct rsvp_if) informtation
 *            into vty. Information will be shown only for "mpls" interfaces,
 *            that is the interfaces on which RSVP-TE is enabled.
 * Callers  : Command Handler for "show mpls interfaces" defined i rsvp_if.c
 *-------------------------------------------------------------------------*/
void
rsvp_if_dump_private_info_vty (struct vty *vty,
                               struct interface *ifp)
{
   struct rsvp_if *rsvp_if;
   struct rsvp_statistics *rsvp_if_statistics;
 
   /* Sanity Check. */
   if (!vty || !ifp)
      return;
   
   rsvp_if = (struct rsvp_if *)ifp->info;

   /* If RSVP-TE is not enabled on this interface then return from here. */
   if (!CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RSVP))
      return; 

   /* Show the physical interface name. */
   vty_out (vty, "%s", VTY_NEWLINE);
   vty_out (vty, "Interface %s%s", ifp->name, VTY_NEWLINE);
   vty_out (vty, "============%s", VTY_NEWLINE);

   /* Show the status of the RSVP-TE. */
   if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_UP))
   {
      /* Check also if IP address is configured on this interface. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_IP))   
         vty_out (vty, "  STATUS : UP %s", VTY_NEWLINE);
      else
         vty_out (vty, "  STATUS : DOWN (IP not configured) %s", VTY_NEWLINE); 
   }
   else
      vty_out (vty, "  STATUS : DOWN (Link Down) %s", VTY_NEWLINE);

   vty_out (vty, "  Number of sessions : %d%s", rsvp_if->session_num, 
            VTY_NEWLINE);
    
   vty_out (vty, "  RSVP-TE Parameters %s", VTY_NEWLINE);
   vty_out (vty, "  ================== %s", VTY_NEWLINE);    
   /* Display Refresh Interval. */
   vty_out (vty, "  Refresh Interval : %d s", rsvp_if->v_refresh_interval);
   vty_out (vty, CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_INT) ? 
                 "%s" : " (Default)%s", VTY_NEWLINE);

   /* Display Refresh Multiple. */
   vty_out (vty, "  Refresh Multiple : %d  ", rsvp_if->v_refresh_multiple);
   vty_out (vty, CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_MUL) ?
                 "%s" : " (Default)%s", VTY_NEWLINE);

   /* Display Hello Interval. */
   vty_out (vty, "  Hello Interval   : %d s", rsvp_if->v_hello_interval);
   vty_out (vty, CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_INT) ?
                 "%s" : " (Default)%s", VTY_NEWLINE);

   /* Display Hello Persistance. */
   vty_out (vty, "  Hello Persist    : %d  ", rsvp_if->v_hello_persist);
   vty_out (vty, CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_PER) ?
                 "%s" : " (Default)%s", VTY_NEWLINE);

   /* Display Hello Tolerance. */
   vty_out (vty, "  Hello Tolerance  : %d  ", rsvp_if->v_hello_tolerance);
   vty_out (vty, CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_TOL) ?
                 "%s" : " (Default)%s", VTY_NEWLINE);

   /* Display if Message Bundling is enabled. */
   if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_MSG_AGGR))
      vty_out (vty, "  Message Bundling Enabled %s", VTY_NEWLINE);
   
   /* Display  if Refresh Reduction is enabled. */
   if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RR))
      vty_out (vty, "  Refresh Reduction is enabled %s", VTY_NEWLINE);

   /* Display RSVP-TE Statistics Information on this interface. */
   rsvp_if_statistics = &rsvp_if->statistics;
   rsvp_statistics_dump_vty (vty, rsvp_if_statistics);

   vty_out (vty, "%s%s", VTY_NEWLINE, VTY_NEWLINE);

   return;
}

/*--------------------------------------------------------------------------
 * Following are command handlers.
 *------------------------------------------------------------------------*/
DEFUN (rsvp_interface,
       rsvp_interface_cmd,
       "interface IFNAME [num]",
       "Select an interface to configure\n"
       "Interface Name = eth0/tunnel/mpls\n")
{
   struct interface *ifp;
   
   /* Check if its a Tunnel Interface. */
   if (!strncmp(argv[0],"tunnel", sizeof(argv[0])))
      /* We call handler at rsvp_vty.c to further handle all. As we are 
         treating RSVP-TE Tunnels as interfaces (defined in rsvpd.c so we
         need to sort out the ambiguity here.*/ 
      return (rsvp_interface_tunnel_from_vty (vty, argc, argv));
      
   /* If its a VPLS Pseudowire. */
   else if (argv[0] == "mpls")
   {
      return CMD_SUCCESS;
   }  
   /* Nows its an existing physical interface in kernel. */
   else
   { 
      /* Search for existing interface in RSVP-TE that were obtained from 
         zMPLS.*/
      ifp = if_lookup_by_name (argv[0]);
      /* If rsvp_if is not created on ifp->info then through error. WE don't 
         create dummy interface, rather depend on zMPLS to inform us about
         an interface. So if the config file has the interfaces and are
         invoked while initialization (before getting interfaces from zmpls)
         then we shouldn't through error. */
      if (!ifp && rm->rsvp->init_flag)
      {
         vty_out (vty, "%% Interface doesn't exist%s", VTY_NEWLINE);
         return CMD_WARNING;
      }
      /* If the interface is found, set the vty->index.*/
      vty->index = ifp;
      vty->node = INTERFACE_NODE;
   }

   return CMD_SUCCESS;
}

struct cmd_node rsvp_interface_node = 
{
   INTERFACE_NODE,
   "%s(config-if)# ",
   1
};

/*---------------------------------------------------------------------------
 * Shows all or specified interface to vty. 
 *-------------------------------------------------------------------------*/
DEFUN (show_rsvp_interface, 
       show_rsvp_interface_cmd,
       "show interface [IFNAME] [IFNUM] ",
       SHOW_STR
       "Interface status and configuration\n"
       "Interface name (eth0, tunnel, mpls etc)\n")
{
   struct listnode *node;
   struct interface *ifp;

   /* Specified interface print. */
   if (argc != 0)
   {
      /* Check if its a Tunnel Interface. */
      if (!strcmp (argv[0], "tunnel"))
          return rsvp_show_interface_tunnel_from_vty (vty, argc, argv);
      ifp = if_lookup_by_name (argv[0]);

      if (ifp == NULL)
      {
         vty_out (vty, "%% Can't find interface %s%s", argv[0],
                  VTY_NEWLINE);
         return CMD_WARNING;
      }
      rsvp_if_dump_vty (vty, ifp);
      return CMD_SUCCESS;
   }

   /* All interface print. */
   for (node = listhead (iflist); node; nextnode (node))
      rsvp_if_dump_vty (vty, getdata (node));

   return CMD_SUCCESS; 
}

/*--------------------------------------------------------------------------
 * "Command Handler for "show mpls interfaces".
 *-------------------------------------------------------------------------*/
DEFUN (show_mpls_interfaces,
       show_mpls_interfaces_cmd,
       "show mpls interfaces [IFNAME]",
       SHOW_STR
       MPLS_STR
       "Shows RSVP-TE Information on this interface\n")
{
   struct interface *ifp;
   struct listnode *node;

   /* If specific interface name is apecified. */ 
   if (argc != 0)
   {
      ifp = if_lookup_by_name (argv[0]);

      if (ifp == NULL)
      {
         vty_out (vty, "%% Can't find interface %s%s", argv[0], VTY_NEWLINE);
         return CMD_WARNING;
      }

      rsvp_if_dump_private_info_vty (vty, ifp);
      return CMD_SUCCESS;
   }
  
   /* Else display all RSVP-TE Enabled interfaces. */
   for (node = listhead (iflist); node; nextnode (node))
      rsvp_if_dump_private_info_vty (vty, getdata (node)); 
   
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command Handler for enabling "mpls traffic-eng tunnels"
 *--------------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_tunnels,
       mpls_traffic_eng_tunnels_cmd,
       "mpls traffic-eng tunnels",
       MPLS_STR
       RSVP_STR
       "Enable RSVP-TE on this interface\n")
{
   struct interface *ifp;
   
   ifp = (struct interface *)vty->index;
   
   /* Eanble RSVP-TE on this interface. */
   rsvp_if_enable_rsvp (ifp);
   
   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command Handler for "no mpls traffic-eng tunnels"
 *--------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_tunnels,
       no_mpls_traffic_eng_tunnels_cmd,
       "no mpls traffic-eng tunnels",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Disable RSVP-TE on this interface\n")
{
   struct interface *ifp;

   ifp = vty->index;

   rsvp_if_disable_rsvp (ifp);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command Handler for "mpls traffic-eng refresh-interval". 
 *--------------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_refresh_interval,
       mpls_traffic_eng_refresh_interval_cmd,
       "mpls traffic-eng refresh-interval <1-1000>",
       MPLS_STR
       RSVP_STR
       "Set refresh-interval on this interface\n"
       "refresh-interval in seconds\n")
{
   u_int32_t value;
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;
   value = strtol (argv[0], NULL, 10);

   if (value < 1 || value > 1000)
   {
      vty_out (vty, "Value is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* Set the refresh interval. */
   rsvp_if->v_refresh_interval = value;
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_INT);

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Command Handler for "no mpls traffic-eng refresh-interval ".
 *--------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_refresh_interval,
       no_mpls_traffic_eng_refresh_interval_cmd,
       "no mpls traffic-eng refresh-interval\n",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Set the refresh-interval to default value")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   /* Reset the refrsh interval to default value. */
   rsvp_if->v_refresh_interval = RSVP_REFRESH_INTERVAL_DEFAULT;
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_INT);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Handler for "mpls traffic-eng refresh-multiple".
 *-------------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_refresh_multiple,
       mpls_traffic_eng_refresh_multiple_cmd,
       "mpls traffic-eng refresh-multiple <1-10>",
       MPLS_STR
       RSVP_STR
       "Set refresh-multiple for this interface\n"
       "Value\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   rsvp_if->v_refresh_multiple = strtol (argv[0], NULL, 10);
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_MUL);
 
   return CMD_SUCCESS;
}

/*-----------------------------------------------------------------------------
 * Handler for "no mpls traffic-eng refresh-multiple".
 *---------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_refresh_multiple,
       no_mpls_traffic_eng_refresh_multiple_cmd,
       "no mpls traffic-eng refresh-multiple",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Reset the refresh-multiple on this interface to default value\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   rsvp_if->v_refresh_multiple = RSVP_REFRESH_MULTIPLE_DEFAULT;
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_MUL);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command Handler "mpls traffic-eng hello-interval".
 *-------------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_hello_interval,
       mpls_traffic_eng_hello_interval_cmd,
       "mpls traffic-eng hello-interval <1-10000>",
       MPLS_STR
       RSVP_STR
       "Set interval for hello messages on this interface\n"
       "Value in milliseconds\n")
{
   u_int32_t value;
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   value = strtol (argv[0], NULL, 10);

   /* Check that value is within range. */
   if (value < 1 || value > 10000)
   {
      vty_out (vty, "Value is invalid%s", VTY_NEWLINE);
      return CMD_WARNING;
   }
   rsvp_if->v_hello_interval = value;
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_INT);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command handler for "no mpls traffic-eng hello-interval".
 *-------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_hello_interval,
       no_mpls_traffic_eng_hello_interval_cmd,
       "no mpls traffic-eng hello-interval",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Reset hello-interval to default value (5 seconds)\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   rsvp_if->v_hello_interval = RSVP_HELLO_INTERVAL_DEFAULT;
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_INT);

   return CMD_SUCCESS;
}

DEFUN (mpls_traffic_eng_hello_persist,
       mpls_traffic_eng_hello_persist_cmd,
       "mpls traffic-eng hello-persist <1-100>",
       MPLS_STR
       RSVP_STR
       "Set hello persist value on this interface\n"
       "Value in number\n")
{
   u_int32_t value;
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   /* Extract the value and check that its within range. */
   value = strtol (argv[0], NULL, 10);

   if (value < 1 || value > 100)
   {
      vty_out (vty, "Invalid value%s", VTY_NEWLINE);
      return CMD_WARNING;
   }

   rsvp_if->v_hello_persist = value;
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_PER);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command Handler for "no mpls traffic-eng hello-persist".
 *-------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_hello_persist,
       no_mpls_traffic_eng_hello_persist_cmd,
       "no mpls traffic-eng hello-persist",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Reset hello persist value to default 2\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;
   
   /* Reset to default value. */
   rsvp_if->v_hello_persist = RSVP_HELLO_PERSIST_DEFAULT;
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_PER);

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Command Handler for "mpls traffic-eng hello-tolerance".
 *--------------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_hello_tolerance,
       mpls_traffic_eng_hello_tolerance_cmd,
       "mpls traffic-eng hello-tolerance <1-100>",
       MPLS_STR
       RSVP_STR
       "Set value of hello tolerance on this interface\n"
       "Value\n")
{
   u_int32_t value;
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   value = strtol (argv[0], NULL, 10);

   /* Check that value is within range. */
   if (value < 1 || value > 100)
   {
      vty_out (vty, "Invalid value%s", VTY_NEWLINE);
      return CMD_WARNING;
   }

   rsvp_if->v_hello_tolerance = value;
   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_TOL);

   return CMD_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Command handler for "no mpls traffic-eng hello-tolerance".
 *--------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_hello_tolerance,
       no_mpls_traffic_eng_hello_tolerance_cmd,
       "no mpls traffic-eng hello-tolerance",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Reset hello tolerance on this interface to default value 3\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   /* Reset hello tolerance value to default. */
   rsvp_if->v_hello_tolerance = RSVP_HELLO_TOLERANCE_DEFAULT;
   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_TOL);

   return CMD_SUCCESS;
}

/*--------------------------------------------------------------------------
 * Command handler for "mpls traffic-eng message-bundling".
 * -----------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_message_bundling,
       mpls_traffic_eng_message_bundling_cmd,
       "mpls traffic-eng message-bundling",
       MPLS_STR
       RSVP_STR
       "Enable Message Bundling on this interface\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_MSG_AGGR);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Command handler for "no mpls traffic-eng message-bundling".
 *-------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_message_bundling,
       no_mpls_traffic_eng_message_bundling_cmd,
       "no mpls traffic-eng message-bundling",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Disable Message Bundling on this interface\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_MSG_AGGR);
  
   return CMD_SUCCESS; 
}

/*----------------------------------------------------------------------------
 * Command Handler for "mpls traffic-eng refresh-reduction".
 *--------------------------------------------------------------------------*/
DEFUN (mpls_traffic_eng_refresh_reduction,
       mpls_traffic_eng_refresh_reduction_cmd,
       "mpls traffic-eng refresh-reduction",
       MPLS_STR
       RSVP_STR
       "Enable Refresh Reduction on this interface\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   SET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RR);

   return CMD_SUCCESS;
}      

/*----------------------------------------------------------------------------
 * Command Handler for "no mpls trafic-eng refresh-reduction."
 *--------------------------------------------------------------------------*/
DEFUN (no_mpls_traffic_eng_refresh_reduction,
       no_mpls_traffic_eng_refresh_reduction_cmd,
       "no mpls traffic-eng refresh-reduction",
       NO_STR
       MPLS_STR
       RSVP_STR
       "Disable Refresh Reduction on this interface\n")
{
   struct interface *ifp = vty->index;
   struct rsvp_if *rsvp_if = (struct rsvp_if *)ifp->info;

   UNSET_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RR);

   return CMD_SUCCESS;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_if_config_write 
 * Input    : vty = Pointer to struct vty
 * Output   : Always returns 0
 * Synopsis : This function writes interface specific configuration 
 * Callers  : Callback when interface config is saved.
 *--------------------------------------------------------------------------*/
int
rsvp_if_config_write (struct vty *vty)
{
  struct listnode *node;
  struct interface *ifp;
  char buf[BUFSIZ];

  for (node = listhead (iflist); node; nextnode (node))
  {
      struct rsvp_if *rsvp_if;
      struct listnode *addrnode;
      struct connected *ifc;
      struct prefix *p;

      ifp = getdata (node);
      
      vty_out (vty, "interface %s%s", ifp->name, VTY_NEWLINE);

      if (ifp->desc)
	vty_out (vty, "  description %s%s", ifp->name, VTY_NEWLINE);

      /* Assign bandwidth here to avoid unnecessary interface flap
	 while processing config script */
      if (ifp->bandwidth != 0)
	vty_out(vty, "  bandwidth %u%s", ifp->bandwidth, VTY_NEWLINE); 

      for (addrnode = listhead (ifp->connected); addrnode; nextnode (addrnode))
      {
         ifc = getdata (addrnode);
	    
         if (CHECK_FLAG (ifc->conf, ZEBRA_IFC_CONFIGURED))
	 {
            p = ifc->address;
            vty_out (vty, "  ip%s address %s/%d",
	             p->family == AF_INET ? "" : "v6",
	             inet_ntop (p->family, &p->u.prefix, buf, BUFSIZ),
		     p->prefixlen);

            if (CHECK_FLAG (ifc->flags, ZEBRA_IFA_SECONDARY))
	       vty_out (vty, "  secondary");
		  
            if (ifc->label)
	       vty_out (vty, "  label %s", ifc->label);

   	    vty_out (vty, "%s", VTY_NEWLINE);
         }
      }
      
      rsvp_if = (struct rsvp_if *)ifp->info;

      /* If RSVP is enabled by user. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RSVP))
         vty_out (vty, "  mpls traffic-eng tunnels %s", VTY_NEWLINE);
          
      /* If user has set own refresh interval from CLI. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_INT))
         vty_out (vty, "  mpls traffic-eng refresh-interval %d%s",
                  rsvp_if->v_refresh_interval, VTY_NEWLINE);

      /* If user has set refresh multiple from CLI. */          
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_REF_MUL))
          vty_out (vty, "  mpls traffic-eng refresh-multiple %d%s",
                   rsvp_if->v_refresh_multiple, VTY_NEWLINE);
      /* If user has set hello interval from CLI. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_INT))
          vty_out (vty, "  mpls traffic-eng hello-interval %d%s",
                   rsvp_if->v_hello_interval, VTY_NEWLINE);
      /* If user has set hello persist from CLI. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_PER))
          vty_out (vty, "  mpls traffic-eng hello-persist %d%s",
                   rsvp_if->v_hello_persist, VTY_NEWLINE);

      /* If user has set Hello tolerance from CLI. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_HELLO_TOL))
          vty_out (vty, "  mpls traffic-eng hello-tolerance %d%s",
                   rsvp_if->v_hello_tolerance, VTY_NEWLINE);
          
      /* If Message Bundling is enabled by user from CLI. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_MSG_AGGR))
          vty_out (vty, "  mpls traffic-eng message-bundling%s", VTY_NEWLINE);

      /* If Refresh Reduction is enabled from CLI. */
      if (CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RR))
          vty_out (vty, "  mpls traffic-eng message-bundling%s", VTY_NEWLINE);
    
      /* Space between individual  interface configs. */
      vty_out (vty,"%s", VTY_NEWLINE);     
   }

#ifdef RTADV
      rtadv_config_write (vty, ifp);
#endif /* RTADV */

#ifdef HAVE_IRDP
      irdp_config_write (vty, ifp);
#endif /* IRDP */

      vty_out (vty, "!%s", VTY_NEWLINE);

  return 0;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_if_init 
 * Input    : None
 * Output   : None
 * Synopsis : This function initializes the rsvp_if module and installs the
 *            interface level commands. It is called only once during 
 *            initialization of RSVP-TE.
 * Callers  : rsvp_init () in rsvpd.c
 *---------------------------------------------------------------------------*/
void
rsvp_if_init (void)
{
   /*------------------------------------------------
    * Initialize interfaces and hook it to rsvp->list
    *-----------------------------------------------*/
    if_init ();
    rm->rsvp->iflist = iflist;

    /*-----------------------------------------------
     * Install interface configuration write function.
     *----------------------------------------------*/
    install_node (&rsvp_interface_node, rsvp_if_config_write);

    /*----------------------------------------------------
     * Install  "show interface" command.
     *--------------------------------------------------*/
    install_element (VIEW_NODE,   &show_rsvp_interface_cmd);
    install_element (ENABLE_NODE, &show_rsvp_interface_cmd);
    install_element (CONFIG_NODE, &show_rsvp_interface_cmd);

    /*----------------------------------------------------
     * Install Config mode "interface xyz" command.
     *---------------------------------------------------*/
    install_element (CONFIG_NODE, &rsvp_interface_cmd);

    /*----------------------------------------------------
     * Install "show mpls interfaces" command.
     *--------------------------------------------------*/
    install_element (VIEW_NODE, &show_mpls_interfaces_cmd); 
    install_element (ENABLE_NODE, &show_mpls_interfaces_cmd);
    install_element (CONFIG_NODE, &show_mpls_interfaces_cmd);
    install_default (INTERFACE_NODE);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_tunnels_cmd);
    install_element (INTERFACE_NODE, &no_mpls_traffic_eng_tunnels_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_refresh_interval_cmd);
    install_element (INTERFACE_NODE, 
                     &no_mpls_traffic_eng_refresh_interval_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_refresh_multiple_cmd);
    install_element (INTERFACE_NODE, 
                     &no_mpls_traffic_eng_refresh_multiple_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_hello_interval_cmd);
    install_element (INTERFACE_NODE, &no_mpls_traffic_eng_hello_interval_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_hello_persist_cmd);
    install_element (INTERFACE_NODE, &no_mpls_traffic_eng_hello_persist_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_hello_tolerance_cmd);
    install_element (INTERFACE_NODE, &no_mpls_traffic_eng_hello_tolerance_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_message_bundling_cmd);
    install_element (INTERFACE_NODE, 
                     &no_mpls_traffic_eng_message_bundling_cmd);
    install_element (INTERFACE_NODE, &mpls_traffic_eng_refresh_reduction_cmd);
    install_element (INTERFACE_NODE, 
                     &no_mpls_traffic_eng_refresh_reduction_cmd);
    return ; 
}
