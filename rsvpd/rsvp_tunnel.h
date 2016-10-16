/* The RSVP-TE Tunnel Module.
**
** This file is part of GNU zMPLS Project.
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
 * This module provides abstractions of a TE-Tunnel (as per RFC 2702).
 * A MPLS Tunnel has one or more LSP (label switched paths). The Tunnel traffic
 * may be load balances among the LSPs or may be using only LSP at a time and
 * others as backups. This module interacts with rsvp_lsp below and 
 * rsvpd (rsvp_tunnel_if) above in the functional hierarchy.
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * MAROCs in this module.
 *-------------------------------------------------------------------------*/
#define MAX_LSP_PER_TUNNEL	6
#define TUNNEL_NAME_LEN		128

/*--------------------------------------------------------------------------
 * This enum defines the types of RSVP Tunnels in terms of its positioning
 * in the LSR.
 *-------------------------------------------------------------------------*/
typedef enum
{
   RSVP_TUNNEL_INGRESS = 1,
   RSVP_TUNNEL_TRANSIT = 2,
   RSVP_TUNNEL_EGRESS  = 3
} rsvp_tunnel_type_t;

/*---------------------------------------------------------------------------
 * The RSVP TE-Tunnel abstraction.
 *-------------------------------------------------------------------------*/
struct rsvp_tunnel
{
   char 		name[TUNNEL_NAME_LEN]; /* Name of the tunnel.*/
   rsvp_tunnel_type_t	type; /* Type of the tunnel.*/			
   u_int32_t		num;  /* Number of the tunnel (type specific) */
   struct prefix	dst;  /* Destination IPV4/IPV6 Address of the tunnel.*/
   u_char		status_admin; /* Admin status of the tunnel.*/
   u_char		status_oper;  /* Operational Status of the Tunnel.*/
   u_char		num_lsps;     /* Number of LSPs for this tunnel.*/
   struct rsvp_lsp	*lsp [MAX_LSP_PER_TUNNEL]; /* Pointers to the tunnels*/
};


/*----------------------------------------------------------------------------
 * Prototypes exported by this module.
 *--------------------------------------------------------------------------*/
struct rsvp_tunnel *rsvp_tunnel_create (void);

void rsvp_tunnel_free (struct rsvp_tunnel *);

void rsvp_tunnel_add (char *,
                      u_char,
                      struct prefix *);

void rsvp_tunnel_delete (struct rsvp_tunnel *);

void rsvp_tunnel_add_lsp (struct rsvp_tunnel *,
                          struct rsvp_lsp *);

void rsvp_tunnel_delete_lsp (struct rsvp_tunnel *,
                             struct rsvp_lsp *);

void rsvp_tunnel_start (struct rsvp_tunnel *);

void rsvp_tunnel_stop (struct rsvp_tunnel *);

int rsvp_tunnel_dump_vty (struct rsvp_tunnel *); 
 
