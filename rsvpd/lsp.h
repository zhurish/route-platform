/* Header for common FEC rules and LSP abstractions
**
** This file is part of GNU zMPLS.
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

/* FEC Rule Types Supported by zMPLS.*/
#define FEC_RULE_IPV4_HOST              1
#define FEC_RULE_IPV4_PREFIX            2
#define FEC_RULE_IPV4_TUN		3
#define FEC_RULE_IPV6_HOST              4
#define FEC_RULE_IPV6_PREFIX            5
#define FEC_RULE_IPV6_TUN               6
#define FEC_RULE_ETH                    7
#define FEC_RULE_ETH_VLAN               8
#define FEC_RULE_ATM                    9
#define FEC_RULE_FR                    10
#define FEC_RULE_HDLC                  11
#define FEC_RULE_PPP                   12 

/*--------------------------------------------------------------------------
 * The following struct is the FEC classification rules (Policy) for applying
 * it to a IPv4 LSP Tunnel.
 *------------------------------------------------------------------------*/
struct fec_rule_tun_ipv4
{
   u_char 	  flags;
   u_char	  prot;
   u_char         dscp;
   u_char         mtu;
   struct in_addr src_addr;
   struct in_addr dst_addr;
   u_int16_t      src_port;
   u_int16_t      dst_port;
} __attribute__ ((aligned (8)));

#ifdef HAVE_IPV6
/*--------------------------------------------------------------------------
 * The following struct is the FEC classification rules (Policy) for applying
 * it to a IPv6 LSP Tunnel.
 *------------------------------------------------------------------------*/
struct fec_rule_tun_ipv6
{
   u_char          flags;
   u_char          prot;
   u_char          dscp;
   u_char          mtu;
   struct in6_addr src_addr;
   struct in6_addr dst_addr;
   u_char          src_port;
   u_char          dst_port;
} __attribute__ ((aligned (8)));

#endif /*HAVE_IPV6 */

/*-------------------------------------------------------------------------
 * The unified holder for the FEC Rules .
 *-----------------------------------------------------------------------*/
struct fec_rule
{
   u_char type;
   /* Filters/Rules for FEC Types.*/
   union
   {
      struct in_addr      	      ipv4_host;
      struct prefix_ipv4       	      ipv4_prefix;
      struct fec_rule_tun_ipv4        ipv4_tun;
#ifdef HAVE_IPV6 
      struct in6_addr                 ipv6_host;
      struct prefix_ipv6              ipv6_prefix;
      struct fec_rule_tun_ipv6        ipv6_tun;
#endif /* HAVE_IPV6 */
      /* We will add PW ruless later.*/
   } rule __attribute__ ((aligned (8)));
};


/* zMPLS LSP  Flags. */
#define ZMPLS_LSP_FLAG_LSPID            (1 << 0)
#define ZMPLS_LSP_FLAG_IFINDEX_IN       (1 << 1)
#define ZMPLS_LSP_FLAG_IFINDEX_OUT      (1 << 2)
#define ZMPLS_LSP_FLAG_LABEL_IN         (1 << 3)
#define ZMPLS_LSP_FLAG_LABEL_OUT        (1 << 4)
#define ZMPLS_LSP_FLAG_FEC              (1 << 5)

/*-------------------------------------------------------------------------
 * The struct the defines the LSP Information in ZMPLS Protocol Packet.
 *------------------------------------------------------------------------*/
struct lsp
{
   u_int16_t		 flags;
   u_int16_t 		 lsp_id;
   u_int16_t 		 ifindex_in; /* For platform wide labels it is 0. */
   u_int16_t		 ifindex_out;/* The outgoing interface.*/

   u_int32_t  		 label_in;   /* The local label binding.*/
   u_int32_t 		 label_out;  /* outgoing label. */
   struct fec_rule       fec; /* No alignment is necessary as its at last.*/
};


/*----------------------------------------------------------------------------
 * Prototypes Exported by this module.
 *--------------------------------------------------------------------------*/
