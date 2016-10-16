/* RSVP-TE Network Library Procedures
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
 * RSVP Network Module : Thsi module provides connectivity with the Network.
 *--------------------------------------------------------------------------*/
#include <zebra.h>

#include "sockunion.h"
#include "log.h"
#include "thread.h"
#include "vty.h"
#include "prefix.h"
#include "avl.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_network.h"


/*----------------------------------------------------------------------------
 * Function : rsvp_sock_init
 * Input    : None
 * Output   : Returns 0 if successful, else returns -1
 * Synopsis : This function creates a Raw Socket for RSVP to communicate with
 *            the external world. RSVP uses this single socket to send and 
 *            receive all packets to/from any interface.
 * Callers  : Called during stack initialization in rsvp_create () in rsvpd.c
 *---------------------------------------------------------------------------*/
int
rsvp_sock_init (void)
{
   int rsvp_sock;
   int ret, tos, hincl, ralert = 1;

   /*------------------------------------------------------------------------- 
    * Create a RAW Socket. Although RFC 2205 provides alternative communication
    * over UDP. we use only RAW sockets for sending/receiving all packets.
    * So right now we don't interoperate with RSVP implementation that uses
    * UDP for its messages. This extension will be done later.
    *-----------------------------------------------------------------------*/ 
    rsvp_sock = socket (AF_INET, SOCK_RAW, IPPROTO_RSVP);

   if (rsvp_sock < 0)
   {
      //zlog_warn 
      printf("[rsvp_sock_init] : socket : %s\n", strerror (errno));
      return -1; 
   }

   /*-------------------------------------------------------------------------
    * Set the TOS field of of IP packets going out of this socket to 
    * control priority.
    *-----------------------------------------------------------------------*/
#ifdef IPTOS_PREC_INTERNETCONTROL
   tos = IPTOS_PREC_INTERNETCONTROL;
   ret = setsockopt (rsvp_sock, IPPROTO_IP, IP_TOS,
                     (char *)&tos, sizeof (int));

   if (ret < 0)
   {
      zlog_warn ("[rsvp_sock_init] : Can't set sockopt IP_TOS %d to socket\
                 %d\n", tos, rsvp_sock);
      close (rsvp_sock);
      return ret;
   }
#endif /* IPTOS_PREC_INTERNETCONTROL */

   /*-------------------------------------------------------------------------
    * We will ourselves include IP Header with the packet. Because RSVP-TE
    * may take its own decision to route a packet (disobeying te Route FIB in 
    * kernel) so we don't depend on IP layer to attach the IP header
    *-----------------------------------------------------------------------*/
   ret = setsockopt (rsvp_sock, IPPROTO_IP, IP_HDRINCL, &hincl, 
                     sizeof (hincl));
   if (ret < 0)
      zlog_warn ("[rsvp_sock_init] Can't set IP_HDRINCL option\n");

   /*------------------------------------------------------------------------
    * We need to set the IP_ROUTER_ALERT option to trap the Path, Path Tear 
    * and Resv Conf messages as well as we need to send those packets with
    * the option set.
    *----------------------------------------------------------------------*/
   ret = setsockopt (rsvp_sock, IPPROTO_IP, IP_ROUTER_ALERT, &ralert,
                     sizeof (ralert));
   /*-------------------------------------------------------------------------
    * We would like to receive the interface information for a received 
    * packet because the socket is not bound to any interface. Set the 
    * following appropriate action depending on the OS.
    *-----------------------------------------------------------------------*/
#ifdef IP_PKTINFO
   ret = setsockopt (rsvp_sock, IPPROTO_IP, IP_PKTINFO, &hincl, sizeof (hincl));
   if (ret < 0)
      zlog_warn ("[rsvp_sock_init] Can't set IP_HDRINCL option\n");

#elif defined IP_RECVIF
   ret = setsockopt (rsvp_sock, IPPROTO_IP, IP_RECIF, &hincl, sizeof (hincl));
   if (ret < 0)
      zlog_warn ("[rsvp_sock_init] Can't set IP_RECVIF option\n");
#else
#warning "[rsvp_sock_init] Can't receive link information on this OS"
#endif
 
   return rsvp_sock; 
} 
