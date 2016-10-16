/* RSVP LSP Module Definitions
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

/*--------------------------------------------------------------------------
 * Type of a RSVP LSP.
 *-------------------------------------------------------------------------*/
typedef enum
{
   RSVP_LSP_TYPE_INGRESS = 1,
   RSVP_LSP_TYPE_TRANSIT = 2,
   RSVP_LSP_TYPE_EGRESS  = 3
} rsvp_lsp_type_t;

/*--------------------------------------------------------------------------
 * The Status of a RSVP LSP.
 *--------------------------------------------------------------------------*/
typedef enum
{
   RSVP_LSP_STATUS_DOWN			= 1,/* Down due to signal failure.*/
   RSVP_LSP_STATUS_DOWN_RETRY_EXPIRED	= 2,/* Down due to max, retry */
   RSVP_LSP_STATUS_UP			= 3,/* PSB/RSB Installed.*/
   RSVP_LSP_STATUS_PSB_ACTIVE		= 4,/* Only PSB is installed.*/
   RSVP_LSP_STATUS_RSB_ACTIVE		= 5,/* Only RSB is installed.*/
   RSVP_LSP_STATUS_MBB			= 6,/* Make Before Break in progress.*/
   RSVP_LSP_STATUS_CSPF			= 7 /* CSPF computaion going on. */
} rsvp_lsp_status_t;
 			
    
/*----------------------------------------------------------------------------
 * Data Structure describes a MPLS LSP Signalled by RSVP-TE.
 *---------------------------------------------------------------------------*/
struct rsvp_lsp
{
   rsvp_lsp_type_t type;
   rsvp_lsp_status_t status;
   
   /* Info extracted from PSB/RSB which remains constant throughout the life
      on a LSP. */
   int l3pid;

   /* Source Point of the Tunnel. */
   union
   {
      struct in_addr  lsp_ipv4_src_addr;
      struct in6_addr lsp_ipv6_src_addr;
   } u1;
#define lsp_ipv4_src_addr	u1.lsp_ipv4_src_addr
#define lsp_ipv6_src_addr	u1.lsp_ipv6_src_addr

   /* Destination of Point of the Tunnel.*/
   union
   {
      struct in_addr  lsp_ipv4_dst_addr;
      struct in6_addr lsp_ipv6_dst_addr;
   } u2;
#define lsp_ipv4_dst_addr	u2.lsp_ipv4_dst_addr
#define lsp_ipv6_dst_addr	u2.lsp_ipv6_dst_addr
  
   /* Unique identifiers of the Tunnel. */
   u_int32_t tunnel_id;
   u_int32_t lsp_id;

   /* Extended Tunnel ID of the LSP. */
   union
   {
      u_int32_t lsp_ipv4_ext_tunnel_id;
      u_int32_t lsp_ipv6_ext_tunnel_id;
   } u3;
#define lsp_ipv4_ext_tunnel_id	u3.lsp_ipv4_ext_tunnel_id
#define lsp_ipv6_ext_tunnel_id	u3.lsp_ipv6_ext_tunnel_id

   /* Timer threads for this LSP. */
   struct thread *t_retry_timer;
   struct thread *t_reopt_timer;
   u_int32_t retry_count;

   /* Upstream Interface,Hop and Label.*/
   int ifindex_up;
   struct in_addr prev_hop_addr;
   u_int32_t label_up;

   /* Downstream Interface, Hop and Label. */
   int ifindex_dn;
   struct in_addr next_hop_addr;
   u_int32_t label_dn;

   time_t uptime;

   /* Hooks to PSB/RSBs for this LSP. */
   void *psb_handle;
   void *rsb_handle;

   /* If this LSP is ingress then set the following to user(parent) handle*/
   void *ingress_user_handle;
};


/*----------------------------------------------------------------------------
 * Function Prototypes Exported by this module.
 *--------------------------------------------------------------------------*/
void rsvp_lsp_init (void);
void rsvp_lsp_start (struct rsvp_lsp *);
void rsvp_lsp_stop  (struct rsvp_lsp *);
void rsvp_lsp_reoptimize (struct rsvp_lsp *);
 
