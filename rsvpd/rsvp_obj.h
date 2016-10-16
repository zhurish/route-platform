/* The Header for RSVP Objects Utility Module
** This module describes the templates of various RSVP-TE Objects/Subobjects
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

#define RSVP_OBJ_LEN_MAX		65535
#define RSVP_OBJ_LEN_MIN		4
#define RSVP_OBJ_HDR_LEN		4
#define RSVP_ERO_SUBOBJ_HDR_LEN		2
#define RSVP_RRO_SUBOBJ_HDR_LEN		2
#define RSVP_ERO_HOP_MAX		255 /* Max ERO hops we support.*/
#define RSVP_RRO_HOP_MAX		255 /* Max RRO Hops we support.*/
/*-----------------------------------------------------------------------------
 * The RSVP Class Types.
 *---------------------------------------------------------------------------*/
#define RSVP_CLASS_NULL		 	0
#define RSVP_CLASS_SESSION		1
#define RSVP_CLASS_HOP			3
#define RSVP_CLASS_INTEGRITY		4
#define RSVP_CLASS_TIMEVAL		5
#define RSVP_CLASS_ERR_SPEC		6
#define RSVP_CLASS_SCOPE		7
#define RSVP_CLASS_STYLE		8
#define RSVP_CLASS_FLOW_SPEC		9
#define RSVP_CLASS_FLT_SPEC		10
#define RSVP_CLASS_ST			11
#define RSVP_CLASS_SENDER_TSPEC		12	
#define RSVP_CLASS_ADSPEC		13
#define RSVP_CLASS_POLICY_DATA		14
#define RSVP_CLASS_RESV_CONF		15
/* RSVP-TE (RFC 3209) new Objects. */
#define RSVP_CLASS_LABEL		16
#define RSVP_CLASS_LRO			19
#define RSVP_CLASS_ERO			20
#define RSVP_CLASS_RRO			21
#define RSVP_CLASS_HELLO		22
/* RFC 2961 Extensions Refresh Reduction */
#define RSVP_CLASS_MSG_ID		23
#define RSVP_CLASS_MSG_ID_ACK		24
#define RSVP_CLASS_MSG_ID_LIST		25
/* RSVP-TE (RFC 3209) Object. */
#define RSVP_CLASS_SA			207
/* RSVP-TE FRR (RFC 4090) Object.*/
#define RSVP_CLASS_FRR			205
#define RSVP_CLASS_DETOUR	        63	


/*----------------------------------------------------------------------------
 * RSVP Object Common Header.
 *             0          1          2           3
 *       +----------+----------+-----------+-----------+ 
 *       |Length(bytes)	       | Class-Num | C-Type    |
 *       +----------+----------+-----------+-----------+		     	
 *       |                                             |
 *       //        (Object contents)		       //	
 *       |                                             |
 *       +----------+----------+-----------+-----------+
 *--------------------------------------------------------------------------*/
struct rsvp_obj_hdr
{
   u_int16_t 		length; /*Length including the object header + object*/
   u_char		class_num; /* The Class Number */
   u_char		ctype;     /* Ctype in the Claa Number. */
} __attribute__((packed));

/*--------------------------------------------------------------------------
 * The RSVP-TE  ERO Subobject Header. RFC 3209. Each subobject of the ERO 
 * object describes one hop in the explicit route.
 * RFC 3209 Section 4.3.3 
 *     +----------------------------------------------+ 
 *     |L| Type	    | Length    | Subobject Contents  |  
 *     +----------------------------------------------+
 *------------------------------------------------------------------------*/
struct rsvp_obj_ero_subobj_hdr
{
   u_char	l   :1; /* The Loose Hop Bit. */
   u_char	type:7; /* Type of the hop. */
   u_char	length; /* Length of the subobject. */
} __attribute__((packed));

/*--------------------------------------------------------------------------
 * The RSVP-TE RRO Subobject Header. RFC 3209 Section 4.4.1.
 *
 *      +----------------------------------------------+
 *      | Type      |  Length   |  Subobject contents  |
 *      +----------------------------------------------+
 *------------------------------------------------------------------------*/
struct rsvp_obj_rro_subobj_hdr 
{
   u_char	type; 	/* Type of the RRO Subobject*/
   u_char	length; /* Length of the RRO Subobject. */
} __attribute__((packed));

/*---------------------------------------------------------------------------/
 * various C-type definitions per class types.
 *--------------------------------------------------------------------------*/
#define RSVP_CLASS_SESSION_CTYPE_IPV4_UDP		1
#define RSVP_CLASS_SESSION_CTYPE_IPV6_UDP		2
#define RSVP_CLASS_SESSION_CTYPE_LSP_TUN_IPV4		7
#ifdef HAVE_IPV6
#define RSVP_CLASS_SESSION_CTYPE_LSP_TUN_IPV6		8
#endif /* HAVE_IPV6. */

#define RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV4		7
#ifdef  HAVE_IPV6 
#define RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV6		8
#endif /*HAVE_IPV6 */

#define RSVP_CLASS_LRO_CTYPE_GEN			1
#define RSVP_CLASS_LRO_CTYPE_ATM			2
#define RSVP_CLASS_LRO_CTYPE_FR				3

#define RSVP_CLASS_LABEL_CTYPE_GEN			1

#define RSVP_CLASS_ERO_CTYPE_GEN			1
#define RSVP_CLASS_ERO_SUBOBJ_IPV4_PREFIX		1
#ifdef HAVE_IPV6
#define RSVP_CLASS_ERO_SUBOBJ_IPV6_PREFIX		2	
#endif /* HAVE_IPV6 */
#define RSVP_CLASS_ERO_SUBOBJ_AS			3

#define RSVP_CLASS_RRO_CTYPE_GEN			1
#define RSVP_CLASS_RRO_SUBOBJ_IPV4_ADDR			1
#ifdef HAVE_IPV6
#define RSVP_CLASS_RRO_SUBOBJ_IPV6_ADDR			2
#endif /* HAVE_IPV6 */
#define RSVP_CLASS_RRO_SUBOBJ_LABEL			3

#define RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV4		7
#ifdef HAVE_IPV6
#define RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV6		8
#endif /* HAVE_IPV6 */

#define RSVP_CLASS_FLT_SPEC_CTYPE_LSP_TUN_IPV4		7
#ifdef HAVE_IPV6
#define RSVP_CLASS_FLT_SPEC_CTYPE_LSP_TUN_IPV6		8
#endif /* HAVE_IPV6 */

#define RSVP_CLASS_SA_CTYPE_LSP_TUN			7
#define RSVP_CLASS_SA_CTYPE_LSP_TUN_RA			8

#define RSVP_CLASS_HOP_CTYPE_IPV4			1
#ifdef HAVE_IPV6
#define RSVP_CLASS_HOP_CTYPE_IPV6			2
#endif /* HAVE_IPV6. */

#define RSVP_CLASS_TIMEVAL_CTYPE_GEN			1

#define RSVP_CLASS_STYLE_CTYPE_GEN			1

#define RSVP_CLASS_SCOPE_CTYPE_IPV4			1
#define RSVP_CLASS_SCOPE_CTYPE_IPV6			2

#define RSVP_CLASS_RESV_CONF_CTYPE_IPV4			1
#define RSVP_CLASS_RESV_CONF_CTYPE_IPV6			2

#define RSVP_CLASS_SENDER_TSPEC_CTYPE_GEN		1

#define RSVP_CLASS_POLICY_DATA_CTYPE_GEN		1

#define RSVP_CLASS_ADSPEC_CTYPE_INTSERV			2

#define RSVP_CLASS_SENDER_TSPEC_CTYPE_INTSERV		2

#define RSVP_CLASS_FLOWSPEC_CTYPE_INTSERV		2

/*----------------------------------------------------------------------------
 * RFC 4090 RSVP FRR Extensions Ctypes
 *--------------------------------------------------------------------------*/
#define RSVP_CLASS_FRR_CTYPE_GEN			1
#define RSVP_CLASS_DETOUR_CTYPE_IPV4			7
#define RSVP_CLASS_DETOUR_CTYPE_IPV6			8

#define RSVP_CLASS_ERR_SPEC_CTYPE_IPV4			1
#define RSVP_CLASS_ERR_SPEC_CTYPE_IPV6			2

/*---------------------------------------------------------------------------
 * HELLO Objects RFC 3209 RSVP-TE Extensions
 *-------------------------------------------------------------------------*/
#define RSVP_CLASS_HELLO_CTYPE_REQ			1
#define RSVP_CLASS_HELLO_CTYPE_ACK			2



/*---------------------------------------------------------------------------
 * RSVP ERR_SPEC Object Error Codes.
 *-------------------------------------------------------------------------*/
#define RSVP_ERR_CODE_CONF				0
#define RSVP_ERR_CODE_ADM_CTRL_FAIL			1
#define RSVP_ERR_CODE_POL_CTRL_FAIL			2
#define RSVP_ERR_CODE_NO_PATH				3
#define RSVP_ERR_CODE_NO_SENDER				4
#define RSVP_ERR_CODE_NO_RESV_STYLE_CONFLICT		5
#define RSVP_ERR_CODE_UNKNOWN_RESV_STYLE		6
#define RSVP_ERR_CODE_DEST_PORT_CONFLICT		7
#define RSVP_ERR_CODE_SENDER_PORT_CONFLICT		8
#define RSVP_ERR_CODE_SERVICE_PREEMPTED			12
#define RSVP_ERR_CODE_UNKNOWN_CLASS			13
#define RSVP_ERR_CODE_UNKNOWN_CTYPE			14
#define RSVP_ERR_CODE_TC_ERR				21
#define RSVP_ERR_CODE_TC_SYS_ERR			22
#define RSVP_ERR_CODE_RSVP_SYS_ERR			23

/*-----------------------------------------------------------------------------
 * The following structures contain infos extracted from various objects. 
 *---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Info received from INTEGRITY Object.
 *--------------------------------------------------------------------------*/
struct rsvp_integrity_info
{
   /* TBD.*/
   int type ;
};

/*----------------------------------------------------------------------------
 * Session Object Types as defined in section 4.6.1 in RFC 3209
 *---------------------------------------------------------------------------*/
typedef enum
{
   SESSION_INFO_UDP_IPV4     = 1,
#ifdef HAVE_IPV6
   SESSION_INFO_UDP_IPV6     = 2,
#endif /*HAVE_IPV6 */
   SESSION_INFO_LSP_TUN_IPV4 = 3,
#ifdef HAVE_IPV6
   SESSION_INFO_LSP_TUN_IPV6 = 4
#endif /*HAVE_IPV6.*/
} rsvp_session_info_type_t;

/*----------------------------------------------------------------------------
 * IPV4 UDP Session Info from legacy RSVP RFC 2205 (Not used for MPLS.
 *--------------------------------------------------------------------------*/
struct rsvp_session_info_udp_ipv4
{
   struct in_addr addr;
   u_char         proto_id;
   u_char         e_police;
   u_int16_t      dst_port;
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * IPV6 UDP Session Info from legacy RSVP RFC 2205. (Not used for MPLS.)
 *--------------------------------------------------------------------------*/
struct rsvp_session_info_udp_ipv6
{
   struct in6_addr addr;
   u_char          proto_id;
   u_char          e_police;
   u_int16_t       dst_port;
};
#endif
/*----------------------------------------------------------------------------
 * IPV4 LSP Tunnel Info to hold LSP_TUN_IPV4 Session Object as defined
 * in section 4.6.1.1. in RFC 3209
 *---------------------------------------------------------------------------*/
struct rsvp_session_info_lsp_tun_ipv4
{
   struct in_addr addr;
   u_int16_t tun_id;
   u_int32_t ext_tun_id;   
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * IPV6 LSP Tunnel Info to hold LSP_TUNNEL_IPV6 Session Object as defined in
 * section 4.6.1.2, in RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_session_info_lsp_tun_ipv6
{
   struct in6_addr addr;
   u_int32_t tun_id;
   u_int32_t ext_tun_id[4];
};
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Holder of both IPv4 or IPV4 LSP Session Info.
 *--------------------------------------------------------------------------*/
struct rsvp_session_info
{
   rsvp_session_info_type_t type;
   
   union
   {
      struct rsvp_session_info_udp_ipv4     session_info_udp_ipv4;
#ifdef HAVE_IPV6
      struct rsvp_session_info_udp_ipv6     session_info_udp_ipv6;
#endif
      struct rsvp_session_info_lsp_tun_ipv4 session_info_lsp_tun_ipv4;
#ifdef HAVE_IPV6
      struct rsvp_session_info_lsp_tun_ipv6 session_info_lsp_tun_ipv6;
#endif /* HAVE_IPV6 */
   } u;

#define session_info_udp_ipv4_t		u.session_info_udp_ipv4
#ifdef HAVE_IPV6
#define session_info_udp_ipv6_t		u.session_info_udp_ipv6
#endif
#define session_info_lsp_tun_ipv4_t	u.session_info_lsp_tun_ipv4
#ifdef HAVE_IPV6
#define session_info_lsp_tun_ipv6_t	u.session_info_lsp_tun_ipv6
#endif /* HAVE_IPV6 */
};

/*--------------------------------------------------------------------------
 * Sender Template Info Type. Defined in Section 4.6.2 in RFC 3209.
 *------------------------------------------------------------------------*/
typedef enum
{
   ST_INFO_LSP_TUN_IPV4 = 1,
#ifdef HAVE_IPV6
   ST_INFO_LSP_TUN_IPV6 = 2   
#endif /*HAVE_IPV6 */
} rsvp_st_info_type_t;

/*----------------------------------------------------------------------------
 * Sender Template Info of Type LSP_TUNNEL_IPV4. Defined in section 4.6.2.1 in 
 * RFC 3209. 
 *--------------------------------------------------------------------------*/
struct rsvp_st_info_lsp_tun_ipv4
{
   struct in_addr sender_addr;
   u_int16_t	  lsp_id;	
};

#ifdef HAVE_IPV6
/*---------------------------------------------------------------------------
 * Sender Template Info of Type LSP_TUNNEL_IPV6. Defined in section 4.6.2.2 in
 * RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_st_info_lsp_tun_ipv6
{
   struct in6_addr sender_addr;
   u_int16_t	   lsp_id;
};
#endif /* HAVE_IPV6 */

/*--------------------------------------------------------------------------
 * Unified Info of the Sender Template 
 *------------------------------------------------------------------------*/
struct rsvp_st_info
{
   rsvp_st_info_type_t type;
   union 
   {
      struct rsvp_st_info_lsp_tun_ipv4 st_info_lsp_tun_ipv4;
#ifdef HAVE_IPV6
      struct rsvp_st_info_lsp_tun_ipv6 st_info_lsp_tun_ipv6;
#endif /* HAVE_IPV6 */
   } u;

#define st_info_lsp_tun_ipv4_t u.st_info_lsp_tun_ipv4
#ifdef HAVE_IPV6
#define st_info_lsp_tun_ipv6_t u.st_info_lsp_tun_ipv6
#endif /* HAVE_IPV6 */
};

/*--------------------------------------------------------------------------
 * Holder of info from from SENDER_TPEC Object. (RFC 2210 Section 3.1)
 *-------------------------------------------------------------------------*/
struct rsvp_sender_tspec_info
{
   u_int32_t tb_rate;   	/* Token Bucket Rate.*/
   u_int32_t tb_size;   	/* Token Bucket Size.*/
   u_int32_t peak_rate; 	/* Peak Data Rate.*/
   u_int32_t min_policed_unit;  /* Minimum Policed Unit.*/
   u_int32_t max_packet_size;   /* Max Pacjet Size.*/
};

/*---------------------------------------------------------------------------
 * Types of Flow Spec - From RFC 2210. 
 *-------------------------------------------------------------------------*/
typedef enum
{
   FLOWSPEC_INFO_CL = 1,
   FLOWSPEC_INFO_G  = 2
} rsvp_flowspec_info_type_t;

/*---------------------------------------------------------------------------
 * Holder of info from FLOWSPEC Object requesting controlled load service.
 * from Section 3.2 RFC 2210.
 *-------------------------------------------------------------------------*/
struct rsvp_flowspec_cl_info
{
   u_int32_t tb_rate;   	/* Token Bucket Rate.*/
   u_int32_t tb_size;   	/* Token Bucket Size.*/
   u_int32_t peak_rate; 	/* Peak Data Rate.*/
   u_int32_t min_policed_unit;  /* Minimum Policed Unit.*/
   u_int32_t max_packet_size;   /* Maximum Packet Size.*/
};

/*--------------------------------------------------------------------------
 * Holder of info from FLOWSPEC Object requesting guaranteed service.
 *------------------------------------------------------------------------*/
struct rsvp_flowspec_g_info
{
   u_int32_t tb_rate;		/* Token Bucket Rate.*/
   u_int32_t tb_size;		/* Token Bucket Size.*/
   u_int32_t peak_rate;		/* Peak Data Rate.*/
   u_int32_t min_policed_unit;	/* Minimum Policed Unit.*/
   u_int32_t max_packet_size;	/* Maximum Packet Size.*/
   u_int32_t rate;		/* Rate of teh packet.*/
   u_int32_t slack_term;	/* Slack Term.*/
};

/*--------------------------------------------------------------------------
 * Unified for flowspec info. (RFC 2210).
 *-------------------------------------------------------------------------*/
struct rsvp_flowspec_info
{
   rsvp_flowspec_info_type_t type;
   
   union 
   {
      struct rsvp_flowspec_cl_info  flowspec_cl_info;
      struct rsvp_flowspec_g_info   flowspec_g_info;
   } u;

#define flowspec_cl_info_t  u.flowspec_cl_info
#define flowspec_g_info_t   u.flowspec_g_info
};

/*--------------------------------------------------------------------------
 * The Optional Fragment type in RSVP ADSPEC Object - RFC 2210.
 *------------------------------------------------------------------------*/
typedef enum 
{
   ADSPEC_OPT_FRAG_TYPE_CL = 1,
   ADSPEC_OPT_FRAG_TYPE_G  = 2
} rsvp_adspec_opt_frag_type_t;

/*--------------------------------------------------------------------------
 * Holder for the Controlled Load Opt Fragment type in ADSPEC Object.
 *-------------------------------------------------------------------------*/
struct rsvp_adspec_opt_frag_cl_info
{
   u_int32_t ctot;
   u_int32_t dtot;
   u_int32_t csum;
   u_int32_t dsum;
};

/*--------------------------------------------------------------------------
 * Holder for Guaranteed Service Opt. Fragment type in ADSPEC Object.
 *------------------------------------------------------------------------*/
struct rsvp_adspec_opt_frag_g_info
{
   /* TBD.*/
   ;
};

/*--------------------------------------------------------------------------
 * Unified holder for RSVP Adspec Info.
 *-------------------------------------------------------------------------*/
struct rsvp_adspec_info 
{
   u_int32_t is_hop_cnt;
   u_int32_t path_bw_est;
   u_int32_t min_path_latency;
   u_int32_t composed_mtu;
  
   rsvp_adspec_opt_frag_type_t opt_frag_type;
   
   union 
   {
      struct rsvp_adspec_opt_frag_cl_info opt_frag_cl_info;
      struct rsvp_adspec_opt_frag_g_info  opt_frag_g_info;
   } u;

#define opt_flag_cl_info_t  u.opt_flag_cl_info
#define opt_flag_g_info_t   u.opt_flag_g_info 
};

/*---------------------------------------------------------------------------
 * The Holder for POLICY DATA INFO.
 *--------------------------------------------------------------------------*/
struct rsvp_policy_info
{
    ;
};

/*---------------------------------------------------------------------------
 * The Label Request Object (LRO) Types Defined in Section 4.2 in RFC 3209
 *-------------------------------------------------------------------------*/
typedef enum
{
   LRO_INFO_TYPE_GEN	= 1,
   LRO_INFO_TYPE_FR	= 2,
   LRO_INFO_TYPE_ATM	= 3
} rsvp_lro_info_type_t;

/*----------------------------------------------------------------------------
 * Generic LRO Info.
 *--------------------------------------------------------------------------*/
struct rsvp_lro_info_gen
{
   u_int16_t l3pid;
};

/*----------------------------------------------------------------------------
 * Holder for LRO Info with ATM Label Range as defined in Section 4.2.2. in
 * RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_lro_info_atm
{
   u_int16_t l3pid;
   u_char    m;
   u_int16_t min_vpi;
   u_int16_t min_vci;
   u_int16_t max_vpi;
   u_int16_t max_vci;
};
 
/*----------------------------------------------------------------------------
 * Holder for LRO Info with FR Label Range as defined in Section 4.2.3 in 
 * RFC 3209
 *--------------------------------------------------------------------------*/
struct rsvp_lro_info_fr
{
   u_int16_t l3pid;
   u_char    dlci_len;
   u_int32_t dlci_min;
   u_int32_t dlci_max;
};

/*---------------------------------------------------------------------------
 * Holder for the Unified LRO Info. This can hold all three types of LRO
 * defined in Section 4.2 in RFC 3209 - 1) LRO without label range
 * 2) LRO with ATM Label Range and, 3) LRO with FR Label Range. LRO Information
 * is stored in PSB in this form.
 *-------------------------------------------------------------------------*/
struct rsvp_lro_info
{
   rsvp_lro_info_type_t type;
   /* Gen/ATM/FR Specific Info. */
   union
   {
      struct rsvp_lro_info_gen lro_info_gen;
      struct rsvp_lro_info_atm lro_info_atm;
      struct rsvp_lro_info_fr  lro_info_fr;
   } u;
#define lro_info_gen_t  u.lro_info_gen
#define lro_info_atm_t	u.lro_info_atm
#define lro_info_fr_t   u.lro_info_fr 
};

/*---------------------------------------------------------------------------
 * Label Info Types.
 *--------------------------------------------------------------------------*/
typedef enum
{
   LABEL_INFO_TYPE_GEN = 1
} rsvp_label_info_type_t;

/*---------------------------------------------------------------------------
 * RSVP Label Info Holder.
 *-------------------------------------------------------------------------*/
struct rsvp_label_info
{
   u_int32_t	label;
};
 
/*----------------------------------------------------------------------------
 * ERO Info Types (corresponding to ERO subobject types.
 *--------------------------------------------------------------------------*/
typedef enum
{
   ERO_INFO_TYPE_IPV4_PREFIX = 1,
#ifdef HAVE_IPV6
   ERO_INFO_TYPE_IPV6_PREFIX = 2,
#endif
   ERO_INFO_TYPE_AS	     = 3	
} rsvp_ero_info_type_t; 

/*----------------------------------------------------------------------------
 * The ERO Subobject info of type IPv4 Prefix.
 *--------------------------------------------------------------------------*/
struct rsvp_ero_info_ipv4_prefix
{
   struct in_addr  addr;
   u_char          prefix_len;		
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * ERO Subobject info of type IPv6 Prefix.
 *--------------------------------------------------------------------------*/
struct rsvp_ero_info_ipv6_prefix
{
   struct in6_addr  addr;
   u_char	    prefix_len;   
};
#endif /* HAVE_IPV^. */

/*--------------------------------------------------------------------------
 * ERO Subobject info of type AS Number.
 *------------------------------------------------------------------------*/
struct rsvp_ero_info_as
{
   u_int16_t    as_num;
};

/*--------------------------------------------------------------------------
 * The unified info holder of all types of ERO.
 *------------------------------------------------------------------------*/
struct rsvp_ero_info
{
   u_char  l; /* Loose bit, set of the hop is loose.*/
   rsvp_ero_info_type_t type;
   union
   {
      struct rsvp_ero_info_ipv4_prefix ero_info_ipv4_prefix;
#ifdef HAVE_IPV6
      struct rsvp_ero_info_ipv6_prefix ero_info_ipv6_prefix;
#endif /* HAVE_IPV6 */
      struct rsvp_ero_info_as	  ero_info_as;
   } u;

#define ero_info_ipv4_prefix_t u.ero_info_ipv4_prefix
#ifdef HAVE_IPV6
#define ero_info_ipv6_prefix_t u.ero_info_ipv6_prefix
#endif /* HAVE_IPV6 */
#define ero_info_as_t	       u.ero_info_as
};

/*---------------------------------------------------------------------------
 * Structure to hold the ERO info list.
 *-------------------------------------------------------------------------*/
struct rsvp_ero_info_list
{
   u_char	num;
   struct rsvp_ero_info ero_info [RSVP_ERO_HOP_MAX];
};
/*----------------------------------------------------------------------------
 * The RRO Object Info Types. Defined in section 4.4 in RFC 3209. 
 *--------------------------------------------------------------------------*/
typedef enum
{
   RRO_INFO_IPV4_ADDR 		= 1,
#ifdef HAVE_IPV6
   RRO_INFO_IPV6_ADDR 		= 2,
#endif /* HAVE_IPV6 */
   RRO_INFO_LABEL		= 3
} rsvp_rro_info_type_t; 

/*---------------------------------------------------------------------------
 * Holder for RRO subobject info of type IPV4 Address. Defined in section 
 * 4.4.1.1 in RFC 3209.
 *-------------------------------------------------------------------------*/
struct rsvp_rro_info_ipv4_addr
{
   struct in_addr addr;
   u_char prefix_len;
   u_char local_prot_avail;
   u_char local_prot_inuse;
   u_char bandwidth_prot; /* RFC 4090 Params.*/
   u_char node_prot;      /* RFC 4090 Param.*/	
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Holder for RRO subobject info of type IPV6 Address. Defined in section
 * 4.4.1.2 in RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_rro_info_ipv6_addr
{
   struct in6_addr  addr;
   u_char prefix_len;
   u_char local_prot_avail;
   u_char local_prot_inuse;
   u_char bandwidth_prot; /* RFC 4090 */
   u_char local_prot;     /* RFC 4090 */
};
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Holder for RRO subobject info of type Label. Defined in section 4.4.1.3
 * in RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_rro_info_label
{
   u_int32_t label;
   u_char global_label;
};

/*----------------------------------------------------------------------------
 * Unified holder for RRO Object Info.
 *--------------------------------------------------------------------------*/
struct rsvp_rro_info
{
   rsvp_rro_info_type_t type;
   union
   {
      struct rsvp_rro_info_ipv4_addr rro_info_ipv4_addr;
#ifdef HAVE_IPV6
      struct rsvp_rro_info_ipv6_addr rro_info_ipv6_addr;
#endif /* HAVE_IPV6 */
      struct rsvp_rro_info_label     rro_info_label;
   } u;
#define rro_info_ipv4_addr_t  u.rro_info_ipv4_addr
#ifdef HAVE_IPV6
#define rro_info_ipv6_addr_t  u.rro_info_ipv6_addr
#endif /* HAVE_IPV6 */
#define rro_info_label_t      u.rro_info_label
};


/*--------------------------------------------------------------------------
 * Holder that carries the list of RRO nodes.
 *------------------------------------------------------------------------*/
struct rsvp_rro_info_list
{
   u_char num;
   struct rsvp_rro_info rro_info [RSVP_RRO_HOP_MAX];
};

/*--------------------------------------------------------------------------
 * Filter Specification Info Types. Defined in Section 4.6.3 in RFC 3209.
 *------------------------------------------------------------------------*/
typedef enum 
{
   FLT_SPEC_INFO_LSP_TUN_IPV4 = 1,
#ifdef HAVE_IPV6
   FLT_SPEC_INFO_LSP_TUN_IPV6 = 2
#endif /* HAVE_IPV6 */
} rsvp_flt_spec_info_type_t;

/*----------------------------------------------------------------------------
 * Holder for filter spec info of type lsp tunnel ipv4. This is identical to
 * that of SENDER_TEMPLATE. Defined in Section 4.6.3.1 in RFC 3209
 *--------------------------------------------------------------------------*/
struct rsvp_flt_spec_info_lsp_tun_ipv4
{
   struct in_addr sender_addr;
   u_int16_t      lsp_id;   
}; 

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Holder for filter spec info of type lsp tunnel ipv6. This is identical to
 * that of SENDER_TEMPLATE. Defined in Section 4.6.3.2 in RFC 3209
 *--------------------------------------------------------------------------*/
struct rsvp_flt_spec_info_lsp_tun_ipv6
{
   struct in6_addr sender_addr;
   u_int16_t	   resv;
   u_int16_t       lsp_id;
};
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Unified holder for rsvp_filter_spec_info
 *--------------------------------------------------------------------------*/
struct rsvp_flt_spec_info
{
   rsvp_flt_spec_info_type_t type;
   union 
   {
      struct rsvp_flt_spec_info_lsp_tun_ipv4 flt_spec_info_lsp_tun_ipv4;
#ifdef HAVE_IPV6
      struct rsvp_flt_spec_info_lsp_tun_ipv6 flt_spec_info_lsp_tun_ipv6;
#endif /* HAVE_IPV6 */
   } u; 

#define flt_spec_info_lsp_tun_ipv4_t u.flt_spec_info_lsp_tun_ipv4
#ifdef HAVE_IPV6
#define flt_spec_info_lsp_tun_ipv6_t u.flt_spec_info_lsp_tun_ipv6 
#endif /* HAVE_IPV6 */
};

/*----------------------------------------------------------------------------
 * Session Attribute Object types defined in Section 4.7 in RFC 3209.
 *--------------------------------------------------------------------------*/
typedef enum
{
   SA_INFO_LSP_TUN    = 1,
   SA_INFO_LSP_TUN_RA = 2
} rsvp_sa_info_type_t;

/*----------------------------------------------------------------------------
 * Session Attribute Object Info without Resource Affinities. Defined in 
 * Section 4.7.1 in RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_sa_info_lsp_tun
{
   u_char setup_prio; /* Set up priority of the LSP. */
   u_char hold_prio;  /* Hold Priority of the LSP. */
   u_char local_prot; /* Local Protection Desired.(From flag info in object) */
   u_char label_record; /* Label Recording Desired (do).*/
   u_char bandwidth_prot; /* RFC 4090 FRR - Bandwidth Protection Desired.*/
   u_char node_prot; /* Node protection Desired.*/
   u_char se_style; /* SE - This flag indicates that SE Style reservation req*/
   char sess_name[128]; /* The Name of the Session. */    
};

/*----------------------------------------------------------------------------
 * Session Attribute Object Info with Resource Affinities. Defined in 
 * Section 4.7.2 in RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_sa_info_lsp_tun_ra
{
   u_int32_t exclude_any;
   u_int32_t include_any;
   u_int32_t include_all;
   u_char setup_prio;
   u_char hold_prio;
   u_char local_prot;
   u_char label_record;
   u_char bandwidth_prot; /* RFC 4090 FRR Param.*/
   u_char node_prot;      /* RFC 4090 FRR Param.*/
   u_char se_style;
   char sess_name[128];
};

/*---------------------------------------------------------------------------
 * Unified Holder of Session Attribute Info.
 *-------------------------------------------------------------------------*/
struct rsvp_sa_info
{
   rsvp_sa_info_type_t type;
   union
   {
      struct rsvp_sa_info_lsp_tun 	sa_info_lsp_tun;
      struct rsvp_sa_info_lsp_tun_ra	sa_info_lsp_tun_ra;
   } u;
 
#define sa_info_lsp_tun_t	u.sa_info_lsp_tun
#define sa_info_lsp_tun_ra_t	u.sa_info_lsp_tun_ra
};

/*----------------------------------------------------------------------------
 * RSVP HOP Object info types. (From RFC 2205)
 *---------------------------------------------------------------------------*/
typedef enum
{
   HOP_INFO_IPV4 = 1,
#ifdef HAVE_IPV6
   HOP_INFO_IPV6 = 2
#endif /* HAVE_IPV6 */
} rsvp_hop_info_type_t;

/*----------------------------------------------------------------------------
 * RSVP HOP IPv4 Info Holder.
 *--------------------------------------------------------------------------*/
struct rsvp_hop_info_ipv4
{
   struct in_addr addr;
   u_int32_t lih;
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * RSVP HOP IPV6 Info Holder.
 *--------------------------------------------------------------------------*/
struct rsvp_hop_info_ipv6
{
   struct in6_addr addr;
   u_int32_t lih;
};
#endif /* HAVE_IPV6. */

/*---------------------------------------------------------------------------
 * Unified Holder of RSVP HOP Info.
 *-------------------------------------------------------------------------*/
struct rsvp_hop_info
{
   rsvp_hop_info_type_t type;
   union
   {
      struct rsvp_hop_info_ipv4 hop_info_ipv4;
#ifdef HAVE_IPV6
      struct rsvp_hop_info_ipv6 hop_info_ipv6;
#endif /* HAVE_IPV6 */
   } u;

#define hop_info_ipv4_t		u.hop_info_ipv4
#ifdef HAVE_IPV6
#define hop_info_ipv6_t		u.hop_info_ipv6
#endif /* HAVE_IPV6. */
};

/*---------------------------------------------------------------------------
 * This enumerated structure specifies the RSVP Style Types.
 *-------------------------------------------------------------------------*/
typedef enum
{
   RSVP_STYLE_TYPE_WF = 1,
   RSVP_STYLE_TYPE_FF = 2,
   RSVP_STYLE_TYPE_SE = 3
} rsvp_style_type_t; 

/*---------------------------------------------------------------------------
 * Holder for RSVP Style Info.
 *-------------------------------------------------------------------------*/
struct rsvp_style_info 
{
   rsvp_style_type_t type;
};

/*----------------------------------------------------------------------------
 * RSVP Timevalues info (From RFC 2205)
 *--------------------------------------------------------------------------*/
struct rsvp_timeval_info
{
   u_int32_t refresh_period;
};

/*---------------------------------------------------------------------------
 * The types of RSVP Scope Info.
 *-------------------------------------------------------------------------*/
typedef enum
{
   SCOPE_INFO_IPV4 = 1,
   SCOPE_INFO_IPV6 = 2
} rsvp_scope_info_type_t;

/*---------------------------------------------------------------------------
 * RSVP Scope info holder.
 *-------------------------------------------------------------------------*/
struct rsvp_scope_info
{
   rsvp_scope_info_type_t type;
   struct list *addr_list;
};

/*-------------------------------------------------------------------------
 * Type of Resv Confirm.
 *-----------------------------------------------------------------------*/
typedef enum
{
   RESV_CONF_INFO_IPV4 = 1,
   RESV_CONF_INFO_IPV6 = 2
} rsvp_resv_conf_type_t;

/*--------------------------------------------------------------------------
 * RSVP Resv Conf Info Holder.
 *------------------------------------------------------------------------*/
struct rsvp_resv_conf_info
{
   rsvp_resv_conf_type_t type;
   
   union 
   {
      struct in_addr  resv_conf_ipv4_addr;
#ifdef HAVE_IPV6
      struct in6_addr resv_conf_ipv6_addr;
#endif /* HAVE_IPV6 */
   } u;

#define resv_conf_ipv4_addr_t  u.resv_conf_ipv4_addr
#ifdef HAVE_IPV6
#define resv_conf_ipv6_addr_t  u.resv_conf_ipv6+addr
#endif
};
 
/*---------------------------------------------------------------------------
 * Structure that holds the FRR Info received from Path Message (RFC 4090).
 *-------------------------------------------------------------------------*/
struct rsvp_frr_info
{
   u_char    setup_prio;
   u_char    hold_prio;
   u_char    hop_limit;
   u_char    backup_type;
   u_int32_t bandwidth;
   u_int32_t include_any;
   u_int32_t exclude_any;
   u_int32_t include_all;
};

/*---------------------------------------------------------------------------
 * This enum defines the types of rsvp detour info types TF 4209.
 *-------------------------------------------------------------------------*/
typedef enum 
{
   RSVP_DETOUR_INFO_IPV4 = 1,
   RSVP_DETOUR_INFO_IPV6 = 2
} rsvp_detour_info_type_t;

/*--------------------------------------------------------------------------
 * This struct holds a IPV4 PLR_ID-Avoid_Node_ID Pair. Section 4.2 in RFC 4209
 *------------------------------------------------------------------------*/
struct rsvp_detour_element_info_ipv4 
{
   struct in_addr plr_id;        /* The PLR ID .*/
   struct in_addr avoid_node_id; /* The Avoid Node ID.*/  
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * This structure holds a IPV6 PLR_ID/Avoid_Node_ID pair. Section 4.2 in 
 * RFC 3209.
 *--------------------------------------------------------------------------*/
struct rsvp_detour_element_info_ipv6
{
   struct in6_addr plr_id;	 /* The PLR ID>*/
   struct in6_addr avoid_node_id;/* The Avoid Node ID.*/
};
#endif /* HAVE_IPV6*/

/*----------------------------------------------------------------------------
 * The unified RSVP Detour Info (RFC 4209).
 *--------------------------------------------------------------------------*/
struct rsvp_detour_info
{
   rsvp_detour_info_type_t type; /* Type of the detour info.*/
   u_int32_t               num;  /* Number of PLR/Avoid_node Pairs.*/
   struct list             *detour_element_list; /* List of all elements.*/   
};

/*--------------------------------------------------------------------------
 * Enumerated Types for ERROR_SPEC Object Types.
 *------------------------------------------------------------------------*/
typedef enum
{
   ERR_SPEC_INFO_IPV4 = 1,
#ifdef HAVE_IPV6 
   ERR_SPEC_INFO_IPV6 = 2
#endif /* HAVE_IPV6 */
} rsvp_err_spec_info_type_t;

/*---------------------------------------------------------------------------
 * Holder for IPV4 RSVP Error Spec Onject.
 *-------------------------------------------------------------------------*/
struct rsvp_err_spec_ipv4_info 
{
   struct in_addr     addr;
   u_char 	      in_place_flag;
   u_char	      not_guilty_flag;
   u_char	      err_code;
   u_int16_t	      err_value;	
};

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Holder for IPV6 RSVP Error Spec. Object
 *--------------------------------------------------------------------------*/
struct rsvp_err_spec_ipv6_info
{
   struct in_addr    addr;
   u_char	     in_place_flag;
   u_char            no_guilty_flag;
   u_char            err_code;
   u_int16_t         err_value;
};
#endif
/*---------------------------------------------------------------------------
 * Unified Holder for RSVP Error Spec Object.
 *--------------------------------------------------------------------------*/
struct rsvp_err_spec_info
{
   rsvp_err_spec_info_type_t type;
   
   union 
   {
      struct rsvp_err_spec_ipv4_info err_spec_ipv4_info;
#ifdef HAVE_IPV6
      struct rsvp_err_spec_ipv6_info err_spec_ipv6_info;
#endif /* HAVE_IPV6 */
   } u;

#define err_spec_ipv4_info_t	u.err_spec_ipv4_info
#ifdef HAVE_IPV6
#define err_spec_ipv6_info_t	u.err_spec_ipv6_info
#endif /* HAVE_IPV6 */
};


/*----------------------------------------------------------------------------
 * Function Prototypes exported by this module.
 *--------------------------------------------------------------------------*/
int rsvp_obj_encode_hdr (struct stream *, 
                         u_int16_t, 
                         u_char, 
                         u_char);

int rsvp_obj_encode_integrity (struct stream *,
                               struct rsvp_integrity_info *);

int rsvp_obj_encode_session (struct stream *,
                             struct rsvp_session_info *);

int rsvp_obj_encode_st (struct stream *,
                        struct rsvp_st_info *);

int rsvp_obj_encode_lro (struct stream *,
                         struct rsvp_lro_info *);

int rsvp_obj_encode_label (struct stream *,
                           struct rsvp_label_info *);

int rsvp_obj_encode_ero_subobj_hdr (struct stream *,
                                    u_char,
                                    u_char,
                                    u_char);

int rsvp_obj_encode_ero_subobj_ipv4_prefix (struct stream *,
                                            u_char,
                                            struct in_addr *,
                                            u_char);
#ifdef HAVE_IPV6
int rsvp_obj_encode_ero_subobj_ipv6_prefix (struct stream *,
                                            u_char,
                                            struct in6_addr *,
                                            u_char);
#endif
int rsvp_obj_encode_style_gen (struct stream *,
                               u_char,
                               u_int32_t);
int rsvp_obj_encode_frr (struct stream *,
                         u_char *,
                         u_char *,
                         u_char *,
                         u_char *,
                         u_int32_t *,
                         u_int32_t *,
                         u_int32_t *,
                         u_int32_t *); 
/*----------------------------------------------------------------------------
 * The Object Decode Function Prototypes exported by this module.
 *--------------------------------------------------------------------------*/
int rsvp_obj_decode_hdr (struct stream *,
                         u_int16_t *,
                         u_char *,
                         u_char *);

int rsvp_obj_decode_integrity (struct stream *,
                               u_char,
                               u_int16_t,
                               struct rsvp_integrity_info *);

int rsvp_obj_decode_session  (struct stream *,
                              u_char,
                              u_int16_t,
                              struct rsvp_session_info *);

int rsvp_obj_decode_st (struct stream *,
                        u_char,
                        u_int16_t,
                        struct rsvp_st_info *);

int rsvp_obj_decode_lro (struct stream *,
                         u_char,
                         u_int16_t,
                         struct rsvp_lro_info *);

int rsvp_obj_decode_label (struct stream *,
                           u_char,
                           u_int16_t,
                           struct rsvp_label_info *);

int rsvp_obj_decode_ero (struct stream *,
                         u_char,
                         u_int16_t,
                         struct rsvp_ero_info_list *);

int rsvp_obj_decode_rro (struct stream *,
                         u_char,
                         u_int16_t,
                         struct rsvp_rro_info_list *);

int rsvp_obj_decode_flt_spec (struct stream *,
                              u_char,
                              u_int16_t,
                              struct rsvp_flt_spec_info *);

int rsvp_obj_decode_sa (struct stream *,
                        u_char,
                        u_int16_t,
                        struct rsvp_sa_info *);

int rsvp_obj_decode_hop (struct stream *,
                         u_char,
                         u_int16_t,
                         struct rsvp_hop_info *);

int rsvp_obj_decode_timeval (struct stream *,
                             u_char,
                             u_int16_t,
                             struct rsvp_timeval_info *);

int rsvp_obj_decode_err_spec (struct stream *,
                              u_char,
                              u_int16_t,
                              struct rsvp_err_spec_info *);

int rsvp_obj_decode_style (struct stream *,
                           u_char,
                           u_int16_t,
                           struct rsvp_style_info *);

int rsvp_obj_decode_frr (struct stream *,
                         u_char,
                         u_int16_t,
                         struct rsvp_frr_info *);

int rsvp_obj_decode_detour (struct stream *s,
                            u_char,
                            u_int16_t,
                            struct rsvp_detour_info *);
