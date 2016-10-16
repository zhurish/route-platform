/* The RSVP-TE Daemon Main Header 
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

/*-------------------------------------------------------------------------
 * RSVP-TE Daemon specific constants 
 *-----------------------------------------------------------------------*/
//#define zMPLS_RSVPD_VERSION  "0.95 build 01"

/* Default configuration settings for RSVPD. */
#define RSVPD_VTY_PORT			2619
//#define RSVPD_VTYSH_PATH		"/tmp/.rsvpd"
#define RSVPD_DEFAULT_CONFIG		"rsvpd.conf"

/* MACROS related to data structures. */
#define RSVP_IP_PATH_NAME_LEN		20

/*--------------------------------------------------------------------------
 * Typedef RSVPD specific types
 *-------------------------------------------------------------------------*/
typedef u_int16_t rsvp_size_t;


/*---------------------------------------------------------------------------
 * The Central Holder of RSVP-TE Framework. This is as per Zebra framework
 *-------------------------------------------------------------------------*/
struct rsvp_master
{
   /* The RSVP-TE Stack Holder. */
   struct rsvp *rsvp;

   /* RSVP-TE Thread Master. */
   struct thread_master *master;

   /* RSVP-TE Start Time. */
   time_t start_time;
};   

/*--------------------------------------------------------------------------
 * Generic container for tracking RSVP Packet statistics 
 *------------------------------------------------------------------------*/
struct rsvp_statistics
{
   u_int32_t path_in;
   u_int32_t path_out;

   u_int32_t resv_in;
   u_int32_t resv_out;

   u_int32_t path_err_in;
   u_int32_t path_err_out;

   u_int32_t resv_err_in;
   u_int32_t resv_err_out;

   u_int32_t path_tear_in;
   u_int32_t path_tear_out;

   u_int32_t resv_tear_in;
   u_int32_t resv_tear_out;

   u_int32_t hello_in;
   u_int32_t hello_out;

   u_int32_t resv_conf_in;
   u_int32_t resv_conf_out;
};

/*----------------------------------------------------------------------------
 * The central object for holding the RSVP-TE Stack. The RSVP-TE implementation
 * follows the basic RSVP architecture from RFC 2205, but doesn't implement
 * it completely. This stack implements RSVP-TE as per RFC 3209. 
 *--------------------------------------------------------------------------*/
struct rsvp
{
   u_char init_flag; /* This flag is set to 0 when this structure is created
                        and set to 1 when all sub-module initializations are
                        complete. It has variuos uses.*/
   /* RSVP-TE Router ID */
   struct prefix router_id; /* Configured autimatically from Loopback 
                               Interface. */

   /*--------------------------------------------------
    * Following are RSVP-TE Global Configs. After stack
    * initialize will contain default values. Can be 
    * configured with new values from CLI.
    *------------------------------------------------*/
   u_int32_t cflags;
#define RSVP_CFLAG_ROUTER_ID_LOOPBACK		(1 << 0)
#define RSVP_CFLAG_ROUTER_ID_ADDR		(1 << 1)

   /* Various Timer Values */
   int v_reopt_interval;   /* Interval for reoptimizing an existing LSP. */
   int v_retry_interval;   /* Interval for retrying a failed LSP. */
   int v_retry_count_max;  /* The maximum number of times that a failed LSP
                              can be retried. */

   /* Interface List obtained from Zebra */
   struct list *iflist;

   /*---------------------------------------------------------------- 
    * Following four objects are maintained by rsvp_softstate module 
    *--------------------------------------------------------------*/
   void *psb_ipv4_obj;       /* Global hook for PSBs of IPV4 Sessions */
#ifdef HAVE_IPV6
   void *psb_ipv6_obj;	     /* Global hook for RSBs of IPV6 Sessions.*/    
#endif /* HAVE_IPV6 */
   void *psb_hash_if_as_key; /* Hash List of PSBs with ifindex as key. */
   void *rsb_hash_if_as_key; /* Hash List of RSBs with ifindex as key. */

   /*---------------------------------------------------------------
    * Holder for all IP Explicit Paths Configured from CLI. As paths
    * are identifies by "name", so I would organize it in a ternary 
    * search trie with name as key. Generally the number of unique 
    * paths configured in an ingress LSR would be not very huge, but
    * still we need to optimize because a path will be searched/
    * inserted/deleted from CLI and so has to be efficient.
    *
    * We also keep the paths in a list that makes life easier for
    * dumping all ip explicit path configs into config file.The list
    * is used only for writing configs. 
    *--------------------------------------------------------------*/
   void *ip_path_obj;
   void *ip_path_list;
   /*---------------------------------------------------------------
    * Holder for all TE-Tunnel Interfaces Structure (RFC 2702). Any 
    * tunnel interface configured from CLI would be maintained in this
    * hook by this module. 
    *-------------------------------------------------------------*/
   void *tunnel_if_obj;    /* Tunnel Interfaces in AVL Tree. */

   /*---------------------------------------------------------------
    * Containers for MPLS LSPs. This hook is maintained by mpls_lsp
    * module. It is organized as Hash Tree (Merkle Tree). I choose
    * it over AVL tree to reduce memory space. Because as a transit
    * LSR we might get banged with a huge number of LSP going though
    * it. 
    *-------------------------------------------------------------*/
   void *lsp_obj;            /* Global Object Handle for all mpls lsps. */
   void *lsp_hash_if_as_key; /* Hash list of lsps with if_index as key. */

   /*--------------------------------------------------------------
    * Network I/O Facilities for RSVP-TE daemon
    *------------------------------------------------------------*/
   int sock;              /* socket handle for reading/writing rsvp packets. */
   struct thread *t_read; /* thread for reading packets from socket. */
   struct thread *t_write;/* thread for writing packets from socket. */ 
   struct list *rsvp_if_write_q;/* Interfaces queued for reading and writing.*/

   char *name; /* Name of the Daemon. */

   /*----------------------------------------------------------------- 
    * Hook for RSVP-TE Label Manager. This daemon maintains a 
    * separate label-space which is maintained by this guy.
    *----------------------------------------------------------------*/
   void *labelmgr;

   /*----------------------------------------------------------------- 
    * Hook for Routing Table Maintained by RSVP-TE. At present an
    * exclusive route_table is kept for reference. This table is 
    * built by zclient after receiving routes from ZebraD. This enables
    * faster processing of routing decisions for path/resv messages
    * by avoiding the inter process communication required otherwise
    * for doing the same.  
    *----------------------------------------------------------------*/
   void *route_table;

   /*----------------------------------------------------------------- 
    * Hook to zclient for all communiction with zebra. All communication
    * wit ZebraD is maintained by this guy.
    *----------------------------------------------------------------*/
   void *zclient; 

   /*----------------------------------------------------------------- 
    * All RSVP-TE packets to/fro this daemon is kept by this container.
    *----------------------------------------------------------------*/
   struct rsvp_statistics statistics;
   
};

typedef enum
{
   RSVP_IP_HOP_STRICT = 0,
   RSVP_IP_HOP_LOOSE  = 1
} rsvp_ip_hop_type;

/*--------------------------------------------------------------------------
 * This data strcture stores info about a IP hop for RSVP LSP.
 *-------------------------------------------------------------------------*/
struct rsvp_ip_hop
{
   rsvp_ip_hop_type type;
   struct prefix hop;
};

/*---------------------------------------------------------------------------
 * Data Structure that holds all the IP Paths configured from CLI for a 
 * Tunnel interface. It is always organized as a linked list (as paths are
 * expected to be less tha LSPs/Tunnels) and is hooked into 
 * struct rsvp->path_obj. The rsvp_ip_paths are searchable  by name.
 *-------------------------------------------------------------------------*/
struct rsvp_ip_path
{
   char name[RSVP_IP_PATH_NAME_LEN + 1];    /* Name of the path. */

   int num_hops; /* Hop Limit for a path. */
   struct list *ip_hop_list; /* Linked List of rsvp_ip_hops. */
   /* The number of Tunnels that is referencing to this path.
      this parameter is used to restrict deletion of the path 
      from the CLI when this path is being used by Tunnels. */
   u_int32_t ref_cnt;
                         
};

/*-------------------------------------------------------------------------
 * This data structure is to be only used by struct rsvp_tunnel_if as its
 * path option. This is filled from CLI as a member of rsvp_tunnel_if;
 *-----------------------------------------------------------------------*/
struct rsvp_tunnel_if_path_option
{
   /* This flag will be set if it contains valid information. */
   int valid_flag;
   /* Type of this path option. */
   int type;
#define RSVP_TUNNEL_IF_PATH_OPTION_EXPLICIT 0
#define RSVP_TUNNEL_IF_PATH_OPTION_DYNAMIC  1
   /* This is set if the path option is made standby from its parent 
      rsvp_tunnel_if. */
   int standby_flag;
   /* Pointer to the path object.If explicit this path will be always
     set. For dynamic path this is set only when CSPF is set */
   struct rsvp_ip_path *path;
   /* Handle for corresponding "Ingress" LSP. Ingress LSP is always created
      when the path is set active. This is only manipulated by rsvp_lsp
      module and is opaque to this module.*/
   void *rsvp_lsp_handle;
   /* Back pointer to the parent Interface Tunnel. */
   struct rsvp_tunnel_if *rsvp_tunnel_if;
};

/*--------------------------------------------------------------------------
 * This data structure holds all info on a configured TE-Tunnel in this LSR.
 * It provides a virtual interface abstraction to CLI. The tunnel interface
 * is NOT a LSP, rather it's the traffic trunk as defined in RFC 2702. A 
 * tunnel can have one or more LSPs and will point to the mpls_lsp objects.
 * At present we don't support load balancing of the Tunnel over multiple 
 * LSPs. So only one LSP would be active. This data structure is organized
 * as Hash List. The implementation assumes 1000 MAX Ingress Tunnels 
 * organized in a AVL Tree with Tunnel interface number as key. So max depth 
 * would be 10.
 *-----------------------------------------------------------------------*/
struct rsvp_tunnel_if
{
   /* Tunnel Interface Number . */
   u_int32_t num;
   /* Tunnel Status Flags. */
   u_int32_t sflags;
#define RSVP_TUNNEL_IF_SFLAG_ADMIN_UP	(1 << 0) /* When admin UP. */
                                                 /* Default CSPF is ON. */
#define RSVP_TUNNEL_IF_SFLAG_ACTIVE     (1 << 6) /* The LSP is UP.*/
   /* Tunnel Config Flags. */
   u_int32_t cflags;
#define RSVP_TUNNEL_IF_CFLAG_RSVP	(1 << 0) /* TE enabled. */
#define RSVP_TUNNEL_IF_CFLAG_DST        (1 << 1) /* destination is set. */
#define RSVP_TUNNEL_IF_CFLAG_CSPF       (1 << 2) /* CSPF is enabled. */
#define RSVP_TUNNEL_IF_CFLAG_COMPLETE   (1 << 3) /* Tunnel config is complete
                                                    and is ready for signal */
   struct prefix destination;
   /* List of RSVP LSPs for this Tunnel. Max 6 can be configured */
   int num_paths;
   /* Path option number among 6 which is in use at present.
      if none actibe it is 0.*/
   int active_path;
   struct rsvp_tunnel_if_path_option path_option[6]; 
};

/*---------------------------------------------------------------------------
 * This structure is the hoom into rsvp->rsvp_if_obj. This object contains 
 * the AVL Tree for storing the Tunnels globally and its traverser.
 *--------------------------------------------------------------------------*/
struct rsvp_tunnel_if_obj
{
   struct avl_table *rsvp_tunnel_if_avl_table;
   struct avl_traverser rsvp_tunnel_if_avl_table_traverser;
};

/*----------------------------------------------------------------------------
 * rsvp_tunnel_if related MACROS.
 *--------------------------------------------------------------------------*/
#define RSVP_TUNNEL_IF_SET_RSVP(X) \
 do\
 {\
    SET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_RSVP);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_UNSET_RSVP(X) \
do\
{\
   UNSET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_RSVP);\
}\
while (0)

#define RSVP_TUNNEL_IF_IS_RSVP_SET(X) \
   CHECK_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_RSVP)


#define RSVP_TUNNEL_IF_SET_CSPF(X) \
 do\
 {\
     SET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_CSPF);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_UNSET_CSPF(X) \
do\
{\
   UNSET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_CSPF);\
}\
while (0)

#define RSVP_TUNNEL_IF_IS_CSPF_SET(X) \
   CHECK_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_CSPF)

#define RSVP_TUNNEL_IF_SET_DST(X) \
do\
{\
   SET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_DST);\
}\
while (0)

#define RSVP_TUNNEL_IF_UNSET_DST(X) \
do\
{\
   UNSET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_DST);\
}\
while (0)
   
#define RSVP_TUNNEL_IF_IS_DST_SET(X) \
   CHECK_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_DST)

#define RSVP_TUNNEL_IF_SET_COMPLETE(X) \
 do\
 {\
     SET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_COMPLETE);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_SET_INCOMPLETE(X) \
 do\
 {\
     UNSET_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_COMPLETE);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_IS_COMPLETE(X) \
   (CHECK_FLAG (X->cflags, RSVP_TUNNEL_IF_CFLAG_COMPLETE))

#define RSVP_TUNNEL_IF_SET_ADMIN_UP(X) \
 do\
 {\
     SET_FLAG (X->sflags, RSVP_TUNNEL_IF_SFLAG_ADMIN_UP);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_SET_ADMIN_DOWN(X) \
 do\
 {\
     UNSET_FLAG (X->sflags, RSVP_TUNNEL_IF_SFLAG_ADMIN_UP);\
 }\
 while (0) 

#define RSVP_TUNNEL_IF_IS_ADMIN_UP(X) \
     (CHECK_FLAG (X->sflags, RSVP_TUNNEL_IF_SFLAG_ADMIN_UP))
 

#define RSVP_TUNNEL_IF_SET_ACTIVE(X) \
 do\
 {\
     SET_FLAG (X->sflags, RSVP_TUNNEL_IF_SFLAG_ACTIVE);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_SET_INACTIVE(X) \
 do\
 {\
     UNSET_FLAG (X->sflags, RSVP_TUNNEL_IF_SFLAG_ACTIVE);\
 }\
 while (0)

#define RSVP_TUNNEL_IF_IS_ACTIVE(X) \
     (CHECK_FLAG (X->sflags, RSVP_TUNNEL_IF_SFLAG_ACTIVE))
 
        

#define RSVP_PROTO_VERSION	1
#define RSVP_MSG_MAX_LEN	65535

#define LM_START_LABEL_RSVP	8017 /* Start Label for RSVP-TE Daemon.*/
/*----------------------------------------------------------------------------
 * System wide globals.
 *--------------------------------------------------------------------------*/
extern struct thread_master *master;
extern struct rsvp_master *rm;

/*---------------------------------------------------------------------------
 * RSVP-TE Global MACROS.
 *-------------------------------------------------------------------------*/
#define RSVP_REFRESH_INTERVAL_DEFAULT		30 /* Default 30 seconds. */
#define RSVP_REFRESH_MULTIPLE_DEFAULT		3
#define RSVP_HELLO_INTERVAL_DEFAULT		5  /* Default 5 seconds. */
#define RSVP_HELLO_PERSIST_DEFAULT		2
#define RSVP_HELLO_TOLERANCE_DEFAULT		3
#define RSVP_REOPT_INTERVAL_DEFAULT		20 /* 20 seconds. */
#define RSVP_RETRY_INTERVAL_DEFAULT		20 /* 20 seconds. */
#define RSVP_RETRY_COUNT_MAX_DEFAULT		20000 

/* IP Precedence to be used in RSVP Messages. */
#ifndef IPTOS_PREC_INTERNETCONTROL
#define IPTOS_PREC_INTERNET_CONTROL		0xC0
#endif /* IPTOS_PREC_INTERNETCONTROL.*/

/* RSVP TTL for RSVP Protocol. */
#define RSVP_IP_TTL				255 
/*---------------------------------------------------------------------------
 * Function Prototypes.
 *--------------------------------------------------------------------------*/
void rsvp_master_init (void);
void rsvp_init (void);
void rsvp_init_set_complete (void);
void *rsvp_tunnel_if_obj_init ();

struct rsvp *rsvp_create (char *);

struct rsvp_ip_hop *rsvp_ip_hop_create (int,
                                        struct prefix *);

/* rsvp_ip_path related APIs. */
struct rsvp_ip_path *rsvp_ip_path_create (char *);
struct rsvp_ip_path *rsvp_ip_path_search (char *);
int rsvp_ip_path_delete (char *);

int rsvp_ip_path_insert_hop (struct rsvp_ip_path *,
                             int,
                             struct prefix *);
int rsvp_ip_path_delete_hop (struct rsvp_ip_path *,
                             int,
                             struct prefix *); 

/* Interface Tunnel Related APIs exported by this module. */
struct rsvp_tunnel_if *rsvp_tunnel_if_create (u_int32_t);
struct rsvp_tunnel_if *rsvp_tunnel_if_search (u_int32_t);
struct rsvp_tunnel_if *rsvp_tunnel_if_traverse (int);
int rsvp_tunnel_if_delete (u_int32_t);
int rsvp_tunnel_if_is_config_complete (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_start (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_stop (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_switch_lsp (struct rsvp_tunnel_if *, 
                                int);
void rsvp_tunnel_if_admin_up (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_admin_down (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_set_rsvp (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_unset_rsvp (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_set_cspf (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_unset_cspf (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_set_dst (struct rsvp_tunnel_if *, 
                             struct prefix *);
void rsvp_tunnel_if_unset_dst (struct rsvp_tunnel_if *);
void rsvp_tunnel_if_set_path_option (struct rsvp_tunnel_if *, 
                                     int, 
                                     int, 
                                     int,
                                     struct rsvp_ip_path *);
void rsvp_tunnel_if_unset_path_option (struct rsvp_tunnel_if *, 
                                       int);
int rsvp_tunnel_if_path_option_is_valid (struct rsvp_tunnel_if *,
                                         int);
void rsvp_tunnel_if_path_option_select_active (struct rsvp_tunnel_if *);





#define stream_forward stream_forward_getp
#define route_node_delete route_unlock_node



/* List iteration macro. */
//zhurish
#define MPLS_STR "MPLS Information\n"
#define LDP_STR "LDP Information\n"
#define RSVP_STR "RSVP-TE Information\n"
#define RSVP_TUNNEL_STR "Tunnel Information\n"
#define RSVP_TE_STR "Traffic-Engg Information\n"   
#define IP_PATH_STR "IP Path Information\n" 
#define IP_EXPLICIT_PATH_STR "IP Explicit Path Information\n" 

#if defined(QUAGGA_NO_DEPRECATED_INTERFACES)
//#warning "Using deprecated libzebra interfaces"
#define LISTNODE_ADD(L,N) LISTNODE_ATTACH(L,N)
#define LISTNODE_DELETE(L,N) LISTNODE_DETACH(L,N)
#define nextnode(X) ((X) = (X)->next)
#define getdata(X) listgetdata(X)
#define LIST_LOOP(L,V,N) \
  for (ALL_LIST_ELEMENTS_RO (L,N,V))
#endif /* QUAGGA_NO_DEPRECATED_INTERFACES */

/*
#define LIST_LOOP(L,V,N) \
  for ( (N) = listhead(L), ((V) = NULL);\
  (N) != NULL && ((V) = listgetdata(N), 1); \
  (N) = listnextnode(N), ((V) = NULL) )
*/
/*
#define ALL_LIST_ELEMENTS_RO(list,node,data) \
  (node) = listhead(list), ((data) = NULL);\
  (node) != NULL && ((data) = listgetdata(node), 1); \
  (node) = listnextnode(node), ((data) = NULL)

#define LIST_LOOP(L,V,N) \
  for (ALL_LIST_ELEMENTS_RO (L,N,V))
*/
