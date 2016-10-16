/* Header for RSVP Interface Specific Library Module
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

#ifndef _ZMPLS_RSVP_IF
#define _ZMPLS_RSVP_IF


/*----------------------------------------------------------------------------
 * RSVP Interface Data Structure. This is a private structure hooked into
 * the struct interface->data and is used for maintaining RSVP-TE Sessions.
 *--------------------------------------------------------------------------*/
struct rsvp_if
{
   /* Status Flags. */
   u_int32_t sflags;
#define RSVP_IF_SFLAG_UP	(1 << 1) /* If the corresponding if is up. */
#define RSVP_IF_SFLAG_IP        (1 << 2) /* If IP is created on the parent if*/
#define RSVP_IF_SFLAG_RSVP      (1 << 3) /* If RSVP is admin enabled. */
#define RSVP_IF_SFLAG_REF_INT   (1 << 4) /* If User has set refresh interval.*/
#define RSVP_IF_SFLAG_REF_MUL   (1 << 5) /* If user has set refresh multiple.*/
#define RSVP_IF_SFLAG_HELLO_INT (1 << 6) /* If user has set hello-interval */
#define RSVP_IF_SFLAG_HELLO_PER (1 << 7) /* If user has set hello-persist. */
#define RSVP_IF_SFLAG_HELLO_TOL (1 << 8) /* If user has set hello-tolerance.*/ 
#define RSVP_IF_SFLAG_MSG_AGGR	(1 << 9) /* If Message Aggr. is enabled. */
#define RSVP_IF_SFLAG_RR        (1 << 10) /* If Refresh Reduction is enabled.*/
 
   /*----------------------------------------------------------
    * Parameters configured by user. Else uses default values. 
    *--------------------------------------------------------*/
   u_int32_t v_refresh_interval;
   u_int32_t v_refresh_multiple;
   u_int32_t v_hello_interval;
   u_int32_t v_hello_persist;
   u_int32_t v_hello_tolerance;
   
   /* Back Pointers. */
   struct rsvp *rsvp;
   struct interface *ifp;

   /* If write queue is empty it is 0 else, 1 */
   int on_write_q;
   /* The Packet Queue for writing packets on this interface. */
   struct rsvp_packet_fifo *obuf;

   /* Hello Timer Thread on this interface. */
   struct thread *t_hello_timer;

   /* Number of sessions (orr LSPs on this interface. */
   u_int32_t session_num;

   /* RSVP Packet Statistics on this interface. */
   struct rsvp_statistics statistics;
};

/*----------------------------------------------------------------------------
 * RSVP Interface events received from if module
 *--------------------------------------------------------------------------*/
typedef enum 
{
   RSVP_IF_EVENT_IF_ADD = 0, /* When interface is added by zebra. */
   RSVP_IF_EVENT_IF_DEL = 1, /* When interface is deleted by zebra. */
   RSVP_IF_EVENT_UP     = 2, /* When physical inrerface is UP. */
   RSVP_IF_EVENT_DOWN   = 3, /* When physical interface is down. */
   RSVP_IF_EVENT_IP_ADD = 4, /* When IP addressed is added. */
   RSVP_IF_EVENT_IP_DEL = 5  /* When IP address is deleted. */
} rsvp_if_event;

/*----------------------------------------------------------------------------
 * Function Prototypes for this module
 *--------------------------------------------------------------------------*/
int rsvp_if_event_handle (rsvp_if_event, struct interface *);
void rsvp_if_enable_rsvp (struct interface *);
void rsvp_if_disable_rsvp (struct interface *);
struct prefix *rsvp_if_get_addr (struct rsvp_if *);
struct rsvp_if *rsvp_if_lookup_by_local_addr (struct in_addr);
void rsvp_if_init (void);

#endif
