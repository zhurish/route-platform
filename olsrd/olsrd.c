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
#include "command.h"
#include "linklist.h"
#include "prefix.h"
#include "table.h"
#include "if.h"
#include "memory.h"
#include "stream.h"
#include "log.h"
#include "zclient.h"
#include "sockopt.h"
#include "sockunion.h"
#include "privs.h"
#include "plist.h"

#include "olsrd/olsrd.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_time.h"

/* OLSR process wide configuration */
static struct olsr_master olsr_master;

/* OLSR process wide configuration pointer to export. */
struct olsr_master *olm;


/*
 * Starts OLSR routing on the network given by p.
 */
void 
olsr_network_run (struct olsr *olsr, struct prefix *p)
{
  struct interface *ifp;
  struct listnode *node;

  /* Get target interface. */
  for (node = listhead (olm->iflist); node; nextnode (node))
    {
      struct listnode *cn;
      
      if ((ifp = getdata (node)) == NULL)
	continue;
      
      if (memcmp (ifp->name, "VLINK", 5) == 0)
	continue;
      

      /* Iterate interface's connected addresses */
      for (cn = listhead (ifp->connected); cn; nextnode (cn))
	{
	  struct connected *co = getdata (cn);
	  struct prefix *addr;

	  if (CHECK_FLAG (co->flags, ZEBRA_IFA_SECONDARY))
            continue;

/* 2016年7月3日 15:27:30 zhurish: 修改判断是否是点对点链路的判断，适配新版本quagga */
	  if (CONNECTED_PEER(co))//(CONNECTED_POINTOPOINT_HOST(co))
/* 2016年7月3日 15:27:30  zhurish: 修改判断是否是点对点链路的判断，适配新版本quagga */
	    addr = co->destination;
	  else 
	    addr = co->address;

	  /* if interface prefix is match specified prefix,
	     then create socket. */
	  if (p->family == co->address->family &&
	      prefix_match (co->address, p) &&
	      ! olsr_if_is_configured (olsr, co->address))
	    {
	      struct olsr_interface *oi;
	      
	      oi = olsr_if_new (olsr, ifp, co->address);
	      oi->connected = co;

	      oi->olsr = olsr;
	      olsr_if_up (oi);

	      break;
	    }
	}
    }
}


int olsr_network_set (struct olsr *olsr, struct prefix_ipv4 *p)
{
  struct route_node *rn;

  rn = route_node_get (olsr->networks, (struct prefix *)p);

  if (rn->info != (void*)0)
    {
      /* There is already same network statement. */
      route_unlock_node (rn);
      return 0;
    }
  
  rn->info = (void*)1;			/* for the time being. */

  /* Run network config */
  olsr_network_run (olsr, (struct prefix*)p);

  return 1;
}


/*
 * Called whenever an interface state changes or a new address is added.
 */
void olsr_if_update (struct olsr *olsr)
{
  struct route_node *rn;
  struct listnode *node, *next;
  struct olsr_interface *oi;
  struct connected *co;
  int found;

  if (olsr != NULL)
    {
      /* Find interfaces that are not configured already.  */
      for (node = listhead (olsr->oiflist); node; node = next)
	{
	  found = 0;
	  oi = getdata (node);
	  co = oi->connected;
	  
	  next = nextnode (node);

	  for (rn = route_top (olsr->networks); rn; rn = route_next (rn))
	    {
	      if (prefix_match (co->address, &rn->p))
		{
		  found = 1;
		  route_unlock_node (rn);
		  break;
		}
	    }

	  if (found == 0)
	    olsr_if_free (oi);
	}
	
      /* Run each interface. */
      for (rn = route_top (olsr->networks); rn; rn = route_next (rn))
	if (rn->info)
	  olsr_network_run (olsr, &rn->p);

      /* If the node has more than one OLSR interface, start mid timer. */
      if (listcount(olsr->oiflist) > 1)
	{
	  LIST_LOOP (olsr->oiflist, oi, node)
	    OLSR_TIMER_ON (oi->t_mid, olsr_mid_timer, oi, OLSR_IF_MID(oi));
	}
      else
	{
	  LIST_LOOP (olsr->oiflist, oi, node)
	    THREAD_TIMER_OFF (oi->t_mid);
	}

    }
}

/* RFC 3626 3.2
  The main address of a node, which will be used in OLSR control
  traffic as the "originator address" of all messages emitted by
  this node.  It is the address of one of the OLSR interfaces of
  the node.
*/
void
olsr_main_addr_update (struct olsr *olsr)
{
  struct olsr_interface *oi;

  if (olsr->main_addr_is_set == FALSE && ! list_isempty (olsr->oiflist))
    {
      oi  = getdata (listhead (olsr->oiflist));

      memcpy (&olsr->main_addr, &OLSR_IF_ADDR (oi), sizeof (struct in_addr));
      olsr->main_addr_is_set = TRUE;
    }
}

/* Utility function to set boradcast option to the socket.*/
int
sockopt_broadcast (int sock)
{
  int ret;
  int on = 1;

  ret = setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char *) &on, sizeof on);
  if (ret < 0)
    {
      zlog_warn ("can't set sockopt SO_BROADCAST to socket %d", sock);
      return -1;
    }
  return 0;
}


/*
 * Initiates the receiving socket. It is a raw socket that also reads the
 * IP header as ip source and destination are important in OLSR. It listens
 * on all interfaces. Inspired from ../ospfd/ospfd.c.
 */
int 
olsr_sock_init (void)
{
  int olsr_sock;
  int ret, hincl = 1;

#ifndef IP_HDRINCL
#ifdef IPTOS_PREC_INTERNETCONTROL
  int tos;
#endif /* IPTOS_PREC_INTERNETCONTROL */
#endif /* IP_HDRINCL */

  if ( olsrd_privs.change (ZPRIVS_RAISE) )
    zlog_err ("olsr_sock_init: could not raise privs, %s",
               safe_strerror (errno) );
    
  olsr_sock = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
  if (olsr_sock < 0)
    {
      int save_errno = errno;
      if ( olsrd_privs.change (ZPRIVS_LOWER) )
        zlog_err ("olsr_sock_init: could not lower privs, %s",
                   safe_strerror (errno) );
      zlog_err ("olsr_read_sock_init: socket: %s", safe_strerror (save_errno));
      exit(1);
    }
    
#ifdef IP_HDRINCL
  /* we will include IP header with packet */
  ret = setsockopt (olsr_sock, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof (hincl));
  if (ret < 0)
    {
      int save_errno = errno;
      if ( olsrd_privs.change (ZPRIVS_LOWER) )
        zlog_err ("olsr_sock_init: could not lower privs, %s",
                   safe_strerror (errno) );
      zlog_warn ("Can't set IP_HDRINCL option for fd %d: %s",
      		 olsr_sock, safe_strerror(save_errno));
    }
#elif defined (IPTOS_PREC_INTERNETCONTROL)
#warning "IP_HDRINCL not available on this system"
#warning "using IPTOS_PREC_INTERNETCONTROL"
  /* Set precedence field. */
  tos = IPTOS_PREC_INTERNETCONTROL;
  ret = setsockopt (olsr_sock, IPPROTO_IP, IP_TOS,
		    (char *) &tos, sizeof (int));
  if (ret < 0)
    {
      int save_errno = errno;
      if ( olsrd_privs.change (ZPRIVS_LOWER) )
        zlog_err ("olsr_sock_init: could not lower privs, %s",
                   safe_strerror (errno) );
      zlog_warn ("can't set sockopt IP_TOS %d to socket %d: %s",
      		 tos, olsr_sock, safe_strerror(save_errno));
      close (olsr_sock);	/* Prevent sd leak. */
      return ret;
    }
#else /* !IPTOS_PREC_INTERNETCONTROL */
#warning "IP_HDRINCL not available, nor is IPTOS_PREC_INTERNETCONTROL"
  zlog_warn ("IP_HDRINCL option not available");
#endif /* IP_HDRINCL */

  ret = setsockopt_ifindex (AF_INET, olsr_sock, 1);

  if (ret < 0)
     zlog_warn ("Can't set pktinfo option for fd %d", olsr_sock);

  if (olsrd_privs.change (ZPRIVS_LOWER))
    {
      zlog_err ("olsr_sock_init: could not lower privs, %s",
               safe_strerror (errno) );
    }
 
  return olsr_sock;
}




/*
 * Allocate new olsr structure.
 */
struct olsr* olsr_new()
{
  struct olsr *new = XCALLOC(MTYPE_OLSR_TOP, sizeof(struct olsr));

  new->port = OLSR_PORT_DEFAULT;
  new->C = OLSR_C_DEFAULT;
  new->will = OLSR_WILL_DEFAULT;
  new->dup_hold_time = OLSR_DUP_HOLD_TIME_DEFAULT;
  new->top_hold_time = OLSR_TOP_HOLD_TIME_DEFAULT;
  new->mpr_update_time = OLSR_MPR_UPDATE_TIME_DEFAULT;
  new->rt_update_time = OLSR_RT_UPDATE_TIME_DEFAULT;

  new->main_addr_is_set = FALSE;
  new->oiflist = list_new();

  new->neighset = list_new();
  new->n2hopset = list_new();
  new->dupset = list_new();
  new->midset = list_new();
  new->topset = list_new();
  new->advset = list_new();

  new->networks = route_table_init ();
  new->table = route_table_init ();

  new->neighb_hold_time = OLSR_NEIGHB_HOLD_TIME_DEFAULT;

  new->fd = olsr_sock_init();
  
  if (new->fd >= 0)
    new->t_read = thread_add_read (master, olsr_read, new, new->fd);

  return new;
}

/*
 * Just get the first element from the list.
 */
struct olsr* olsr_lookup ()
{
  if (listcount (olm->olsr) == 0)
    return NULL;

  return getdata (listhead (olm->olsr));
}

/*
 * Simple wrappers.
 */
void olsr_add (struct olsr *olsr)
{
  listnode_add (olm->olsr, olsr);
}

void olsr_delete (struct olsr *olsr)
{
  listnode_delete (olm->olsr, olsr);
}


struct olsr *olsr_get ()
{
  struct olsr *olsr;
  
  olsr = olsr_lookup ();
  if (olsr == NULL)
    {
      olsr = olsr_new ();
      olsr_add (olsr);
    }
  return olsr;
}



void
olsr_master_init ()
{
  memset (&olsr_master, 0, sizeof (struct olsr_master));

  olm = &olsr_master;
  olm->master = thread_master_create ();
  olm->start_time = time (NULL);
}
