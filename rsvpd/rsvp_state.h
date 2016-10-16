/* The RSVP Softstate Module Header. 
**  
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

/*----------------------------------------------------------------------------
 * What this module is about ?
 * This module will handle the PSB (Path State Blocks) and RSB 
 * (Reservation State Blocks). It generates events for RSVP LSP Module 
 * "indirectly" (LSP module registers callbacks here) and this module has no 
 * clue about LSPs.
 *--------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
 * The Path State Block Descriptor. At present we have the mandatory
 *  parameters isn Path Messages.
 *---------------------------------------------------------------------------*/
struct rsvp_psb
{
     /* Configuration Flag that tracks the parameters configured on this PSB.*/
   u_int32_t 				cflags;
#define RSVP_PSB_CFLAG_INTEGRITY	(1 << 0)   /* Integrity Info.*/
#define RSVP_PSB_CFLAG_SESSION		(1 << 1)   /* Session Info. */ 
#define RSVP_PSB_CFLAG_HOP		(1 << 2)   /* RSVP PHop Info. */ 
#define RSVP_PSB_CFLAG_TIMEVAL 		(1 << 3)   /* Time Values Info. */
#define RSVP_PSB_CFLAG_ERO		(1 << 4)   /* ERO Info. */
#define RSVP_PSB_CFLAG_LRO		(1 << 5)   /* LRO Info. */
#define RSVP_PSB_CFLAG_SA		(1 << 6)   /* Session Attribute Info.*/
#define RSVP_PSB_CFLAG_PD		(1 << 7)   /* Policy Data Info. */
#define RSVP_PSB_CFLAG_ST	 	(1 << 8)   /* Sender Template.*/
#define RSVP_PSB_CFLAG_SENDER_TSPEC	(1 << 9)   /* Snder TSPEC. */
#define RSVP_PSB_CFLAG_ADSPEC		(1 << 10)   /* Adspec */
#define RSVP_PSB_CFLAG_RRO		(1 << 11)  /* RRO Info.*/		

   struct rsvp_integrity_info 		integrity_info;/* Integrity Info.*/  
   struct rsvp_session_info		session_info; /* Session Info. */ 
   /* Upstream Interface and Previous Hop Info as received from the path 
      message. For ingress PSB, the interface index = 0. */
   int 					if_index_phop;
   struct rsvp_hop_info 		phop_info;
   /* Downstream Interface and Next Hop as decided by this router either
      based on IGP routes or on ERO. For egress PSB the interface index = 0 */
   int 					if_index_nhop;
   struct rsvp_hop_info 		nhop_info;
   struct rsvp_timeval_info		timeval_info;
   struct rsvp_ero_info_list		ero_info_list;
   struct rsvp_lro_info			lro_info;
   struct rsvp_sa_info			sa_info;
   struct rsvp_policy_info		policy_info;
   struct rsvp_st_info			st_info;
   struct rsvp_sender_tspec_info	sender_tspec_info;
   struct rsvp_adspec_info		adspec_info;

/* Timer fired when this PSB times out  if no refresh message is received
   from upstream. This timer is OFF if the PSB is at ingress LSR. */
   struct thread *t_psb_cleanup_timer;
/* Timer initiates a Path Refresh Message. This timer is not active at egress*/
   struct thread *t_path_refresh_timer;
/* Pointer to corresponding RSB if installed. */
   struct rsvp_rsb *rsb;
};

/*----------------------------------------------------------------------------
 * The Reservation State Block (RSB) Holder.
 *--------------------------------------------------------------------------*/
struct rsvp_rsb
{
   /* Backpointer to its Parent PSB. */
   struct rsvp_psb *psb;

   /* Parameters at RSB required to maintain at MPLS LSPs. */
   struct rsvp_session_info   		session_info;
   /* The phop info is obtained from RSVP HOP in the Reservation Message.*/
   int					if_index_phop;
   struct rsvp_hop_info			phop_info;
   /* The nhop info is nothing but the phop info in corresponding rsvp_psb */
   int					if_index_nop;
   struct rsvp_hop_info			nhop_info;
   struct rsvp_timeval_info		timeval_info;
   u_int32_t 				label_info;
   struct rsvp_rro_info_list		rro_info_list;

   /* Timer that is fired when this RSB times out and this RSB is deleted.
      This timer is active only when RSB is installed in the system and its
      a transit or Ingress LSR */
   struct thread 			*t_rsb_cleanup_timer;
   /* Timer that is fired after every RSB refresh interval. This timer is
      active only if this RSB is installed in egress LSR. */
   struct thread 			*t_resv_refresh_timer;
}; 



/*---------------------------------------------------------------------------
 * The RSB/PSB Data Structure Organization
 *
 * The keys for searching PSB/RSB is tunnel_id + lsp_id per sender. Those
 * are organized in a hierarchy as expalined below.
 * 
 * First the PSB/RSBs are organized in groups per sender address (e.g ipv4).
 * It is assumed that there could be maximum 1000 LSRs in a MPLS domain. So 
 * the PSB groups are kept in a hash with sender address as key with hash of 
 * size 1000 (10 bits hash mask) to have almost a "perfect hashing". I use Bob 
 * Jenkin's hash which is the most efficient hash to generate 32 bit keys from 
 * any arbitrary length of keys.
 *
 * Per sender PSB/RSB Group are organized in a AVL tree with tunnel_id as key.
 * Tunnel IDs are unique with respect to a source. Assuming there could be 
 * max. 10,000 (~ 2^13) Tunnels per source, so worst case search in a AVL tree 
 * will take 13 node lookups. Each node of AVL tree has list of PSB/RSBss with 
 * same session (tunnel_id + sender_addr), but diff sender template (lsp-id).So
 *
 * So assuming that in a transit LSR there could be 1000 sources, and 10000 
 * tunnels per source, the worst case search will take lookup in 16 nodes. 
 * I guess for my phase 1 this should be OK :-).
 *---------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------- 
 * Hash Entry with PSB's Sender IPV4 address as Key. Each entry contains
 * AVL tree of sessions. 
 *--------------------------------------------------------------------------*/
struct psb_ipv4_sender_hash_entry
{
   struct in_addr    sender_addr;  /* The IPV4 Sender of this PSB.*/
   struct avl_table  *session_avl; /* The AVL tree of sessions on this sender*/
};

#ifdef HAVE_IPV6
/*--------------------------------------------------------------------------
 * Hash Entry with PSB's Sender IPV6 address as key. Each entry contains 
 * AVL tree of sessions.
 *-------------------------------------------------------------------------*/
struct psb_ipv6_sender_hash_entry
{
   struct in6_addr   sender_addr;   /* The IPV6 Sender of this PSB.*/
   struct avl_table  *session_avl;  /* The AVL Tree of sessions from sender.*/
};
#endif /*HAVE_IPV6 */
/*---------------------------------------------------------------------------
 * The PSB Sender Hash Parameters.
 *--------------------------------------------------------------------------*/
#define PSB_SENDER_HASH_SIZE	(1 << 10) 
#define PSB_SENDER_HASH_MASK	(PSB_SENDER_HASH_SIZE - 1)

/*---------------------------------------------------------------------------
 * AVL Tree Entry with rsvp_psb's Tunnel ID as key
 *-------------------------------------------------------------------------*/
struct psb_session_avl_entry
{
   u_int16_t   tun_id;    /*The TunnelID.*/
   struct list *psb_list; /* List of rsp_psbs taht share the same tunnel
                             id but different lsp_ids.*/
};
/*----------------------------------------------------------------------------
 * Prototypes exported by this module.
 *--------------------------------------------------------------------------*/
void rsvp_psb_dump_vty (struct vty *, struct rsvp_psb *);
void rsvp_state_init (void);     

/*--------------------------------------------------------------------------
 * APIs exported by this module tp rsvp_packet layer for processing all 
 * types of message receive events.
 *-------------------------------------------------------------------------*/ 
void rsvp_state_recv_path (struct rsvp_ip_hdr_info *,
                           struct rsvp_path_msg_info *,
                           struct rsvp_if *);

void rsvp_state_recv_resv (struct rsvp_ip_hdr_info *,
                           struct rsvp_resv_msg_info *,
                           struct rsvp_if *);

void rsvp_state_recv_path_err (struct rsvp_ip_hdr_info *,
                               struct rsvp_path_err_msg_info *,
                               struct rsvp_if *);

void rsvp_state_recv_resv_err (struct rsvp_ip_hdr_info *,
                               struct rsvp_resv_err_msg_info *,
                               struct rsvp_if *);

void rsvp_state_recv_path_tear (struct rsvp_ip_hdr_info *,
                                struct rsvp_path_tear_msg_info *,
                                struct rsvp_if *);

void rsvp_state_recv_resv_tear (struct rsvp_ip_hdr_info *,
                                struct rsvp_resv_tear_msg_info *,
                                struct rsvp_if *);

void rsvp_state_recv_resv_conf (struct rsvp_ip_hdr_info *,
                                struct rsvp_resv_conf_msg_info *,
                                struct rsvp_if *);

void rsvp_state_recv_hello_req (struct rsvp_ip_hdr_info *,
                                struct rsvp_hello_msg_info *,
                                struct rsvp_if *); 

void rsvp_state_recv_hello_ack (struct rsvp_ip_hdr_info *,
                                struct rsvp_hello_msg_info *,
                                struct rsvp_if *);
