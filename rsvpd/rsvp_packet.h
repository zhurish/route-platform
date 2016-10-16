/* Header for RSVP Packet Processing Module
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
 * MACRO Definitions for this module.
 *------------------------------------------------------------------------*/
#define RSVP_MSG_HDR_LEN		8 
/*--------------------------------------------------------------------------
 * The RSVP/RSVP-TE Message Types
 *------------------------------------------------------------------------*/
#define RSVP_MSG_TYPE_PATH 		1
#define RSVP_MSG_TYPE_RESV 		2
#define RSVP_MSG_TYPE_PATH_ERR		3
#define RSVP_MSG_TYPE_RESV_ERR		4
#define RSVP_MSG_TYPE_PATH_TEAR		5
#define RSVP_MSG_TYPE_RESV_TEAR		6
#define RSVP_MSG_TYPE_RESV_CONF		7
#define RSVP_MSG_TYPE_HELLO		8 /* RFC 3209 Extensions. */
#define RSVP_MSG_TYPE_BUNDLE		9 /* RFC 2961 Extensions. */


/*---------------------------------------------------------------------------
 * RSVP Common Message Header.
 *               0           1           2           3
 *          +----------+-----------+-----------+-----------+
 *          |Vers|Flags| Msg Type  |    RSVP Checksum      |    
 *          +----------+-----------+-----------+-----------+
 *          |Send_TTL  | (Reserved)|    RSVP Length        |
 *          +----------+-----------+-----------+-----------+
 *
 * NOTE : The following struct is not a __attribute__((packed)) structure, 
 *        Rather contains only info for a standard RSVP header fields. This
 *        is used for direct analysis and so we make it a packed structure. 
 *--------------------------------------------------------------------------*/
struct rsvp_msg_hdr
{
   u_char 		vers	:4 ; /* Protocol version number: always  1 */
   u_char 		flags	:4 ; /* 0x01-0x08; Reserved */
   u_char	 	msg_type   ; /* The RSVP Message Type */
   u_int16_t		checksum   ; /* The complete RSVP msg Checksum */
   u_char		send_ttl   ; /* The IP TTL value on the msg sent */
   u_char		resv       ; /* Not used */
   u_int16_t		length     ; /* Total length in bytes incl. common 
                                        header and variable length objects 
                                        that follow */
} __attribute__ ((packed));

/*---------------------------------------------------------------------------
 * Generic RSVP-TE Packet. This was needed because the interpretation of
 * path, resv packets are need to be handled differently at network level.
 * For example, when a packet is sent from ingress, IP header is to be created
 * which is created and then the rsvp_read thread is invoked. In that case
 * ip header will already be included in stream. In case of transit hops ,
 * IP header will not be touched at all and same behaviour. But for Resv
 * Messages are sent hop ny hop so IP header will be included bu rsvp_write. 
 *--------------------------------------------------------------------------*/
struct rsvp_packet
{
   struct rsvp_packet	*next;      /* Next Packet in the chain.*/
   u_char 		cflags;
#define RSVP_PACKET_FLAG_RA		(1 << 0) /* Router Alert is needed. */
   /* IP Hdr Information.*/ 
   struct in_addr	ip_src;   /* Src Addr in IP Header of RSVP Packet.*/
   struct in_addr 	ip_dst;   /* Dst Addr in IP Header of RSVP Packet.*/
   u_char		ip_ttl;	  /* TTL in IP Header of RSVP Packet.*/

   struct stream 	*s;	  /* Pointer to the rsvp data stream.*/ 
   struct in_addr	nhop;	  /* Next hop where to send the packet*/
   u_int16_t		length;	  /* Length of the RSVP Message  */
};

/*---------------------------------------------------------------------------
 * Queue of RSVP Packets.
 *-------------------------------------------------------------------------*/
struct rsvp_packet_fifo
{
   unsigned long 	count;
   struct rsvp_packet	*head;
   struct rsvp_packet	*tail;
};

/*---------------------------------------------------------------------------
 * Info for IP Header of RSVP Messages.
 *--------------------------------------------------------------------------*/
struct rsvp_ip_hdr_info
{
  struct in_addr ip_src; /* Src Address of IP Header.*/
  struct in_addr ip_dst; /* Dst Address of IP Header.*/
  u_char         ip_ttl;      /* TTL in IP Header.*/
};

/*----------------------------------------------------------------------------
 * This structure carries info extracted from a RSVP Path Message.
 *--------------------------------------------------------------------------*/
struct rsvp_path_msg_info
{
   u_char			  rsvp_msg_ttl;	
   u_int32_t		          cflags;
#define PATH_MSG_CFLAG_INTEGRITY	(1 << 0)
#define PATH_MSG_CFLAG_SESSION	  	(1 << 1)
#define PATH_MSG_CFLAG_HOP	  	(1 << 2)
#define PATH_MSG_CFLAG_TIMEVAL	  	(1 << 3)
#define PATH_MSG_CFLAG_ERO	  	(1 << 4)
#define PATH_MSG_CFLAG_LRO	  	(1 << 5)
#define PATH_MSG_CFLAG_SA	  	(1 << 6)
#define PATH_MSG_CFLAG_POLICY	  	(1 << 7)
#define PATH_MSG_CFLAG_ST	  	(1 << 8)
#define PATH_MSG_CFLAG_SENDER_TSPEC	(1 << 9)
#define PATH_MSG_CFLAG_ADSPEC		(1 << 10)
#define PATH_MSG_CFLAG_RRO		(1 << 11)  

   struct rsvp_integrity_info	  integrity_info;/* Info INTEGRITY Object.*/
   struct rsvp_session_info   	  session_info;/* Info from SESSION Object.*/
   struct rsvp_hop_info	      	  hop_info;    /* Info from HOP Object.*/
   struct rsvp_timeval_info   	  timeval_info;/* Info from TIMEVAL Object.*/
   struct rsvp_ero_info_list	  ero_info_list;/* Info from ERO Object. */
   struct rsvp_lro_info       	  lro_info;    /* Info from LRO Object.*/
   struct rsvp_sa_info        	  sa_info;     /* Info from SESSION_ATTRIBUTE*/
   struct rsvp_policy_info    	  policy_info; /* Info from POLICY_DATA .*/
   struct rsvp_st_info        	  st_info;     /* Info from SENDER_TEMPLATE.*/
   struct rsvp_sender_tspec_info  sender_tspec_info; /* SENDER_TSPEC Info.*/
   struct rsvp_adspec_info	  adspec_info; /* Info from ADSPEC Object.*/
   struct rsvp_rro_info_list      rro_info_list; /* RRO Info List.*/
};

/*--------------------------------------------------------------------------
 * This structure carries info extracted from RSVP Resv Message by rsvp_packet
 * layer OR sent to packet layer.
 *------------------------------------------------------------------------*/
struct rsvp_resv_msg_info 
{
   u_int32_t 		      cflags;
#define RESV_MSG_CFLAG_INTEGRITY	(1 << 0)
#define RESV_MSG_CFLAG_SESSION		(1 << 1)
#define RESV_MSG_CFLAG_HOP		(1 << 2)
#define RESV_MSG_CFLAG_TIMEVAL		(1 << 3)
#define RESV_MSG_CFLAG_RESV_CONF	(1 << 4)
#define RESV_MSG_CFLAG_SCOPE		(1 << 5)
#define RESV_MSG_CFLAG_POLICY		(1 << 6)
#define RESV_MSG_CFLAG_STYLE		(1 << 7)  
 
   struct rsvp_integrity_info integrity_info;/* Info from INTEGRITY Object.*/
   struct rsvp_session_info   session_info;  /* Info from SESSION Object.*/
   struct rsvp_hop_info	      hop_info;      /* Info from HOP Object.*/
   struct rsvp_timeval_info   timeval_info;  /* Info from TIMEVAL Object.*/
   struct rsvp_resv_conf_info resv_conf_info;/* Info from RESV CONF Object.*/
   struct rsvp_scope_info     scope_info;    /* Info from SCOPE Object.*/
   struct rsvp_policy_info    policy_info;   /* Info from POLICY_DATA.*/
   struct rsvp_style_info     style_info;    /* Info from STYLE Object.*/
   struct rsvp_rro_info_list  rro_info_list; /* Info from RRO Object.*/ 
};

/*----------------------------------------------------------------------------
 * This structure carries info extracted from a RSVP Path Error Message.
 *--------------------------------------------------------------------------*/
struct rsvp_path_err_msg_info
{
   u_int32_t			 cflags;
#define PATH_ERR_MSG_CFLAG_INTEGRITY	(1 << 0)
#define PATH_ERR_MSG_CFLAG_SESSION	(1 << 1)
#define PATH_ERR_MSG_CFLAG_ERR_SPEC	(1 << 2)
#define PATH_ERR_MSG_CFLAG_POLICY	(1 << 3)
#define PATH_ERR_MSG_CFLAG_ST		(1 << 4)
#define PATH_ERR_MSG_CFLAG_SENDER_TSPEC (1 << 5)
#define PATH_ERR_MSG_CFLAG_ADSPEC	(1 << 6)
 
   struct rsvp_integrity_info	 integrity_info;
   struct rsvp_session_info	 session_info;
   struct rsvp_err_spec_info	 err_spec_info;
   struct rsvp_policy_info	 policy_info;
   struct rsvp_st_info		 st_info;
   struct rsvp_sender_tspec_info sender_tspec_info;
   struct rsvp_adspec_info	 adspec_info; 
};

/*---------------------------------------------------------------------------
 * This structure carries info extracted from a RSVP Resv Error Message.
 *-------------------------------------------------------------------------*/
struct rsvp_resv_err_msg_info
{
   u_int32_t 			cflags;
#define RESV_ERR_MSG_CFLAG_INTEGRITY		(1 << 0)
#define RESV_ERR_MSG_CFLAG_SESSION		(1 << 1)
#define RESV_ERR_MSG_CFLAG_HOP			(1 << 2)
#define RESV_ERR_MSG_CFLAG_ERR_SPEC		(1 << 3)
#define RESV_ERR_MSG_CFLAG_SCOPE		(1 << 4)
#define RESV_ERR_MSG_CFLAG_POLICY		(1 << 5)
#define RESV_ERR_MSG_CFLAG_STYLE		(1 << 6)

   struct rsvp_integrity_info	integrity_info;
   struct rsvp_session_info	session_info;
   struct rsvp_hop_info		hop_info;
   struct rsvp_err_spec_info	err_spec_info;
   struct rsvp_scope_info	scope_info;
   struct rsvp_policy_info	policy_info;
   struct rsvp_style_info	style_info;
};

/*----------------------------------------------------------------------------
 * This structure carries info extracted from a RSVP Path Tear Message.
 *--------------------------------------------------------------------------*/
struct rsvp_path_tear_msg_info
{
   u_char			 rsvp_msg_ttl;  /*Send_TTL from RSVP Msg Hdr.*/
   u_int32_t 			 cflags;
#define PATH_TEAR_MSG_CFLAG_INTEGRITY		(1 << 0)
#define PATH_TEAR_MSG_CFLAG_SESSION		(1 << 1)
#define PATH_TEAR_MSG_CFLAG_HOP			(1 << 2)
#define PATH_TEAR_MSG_CFLAG_ST			(1 << 3)
#define PATH_TEAR_MSG_CFLAG_SENDER_TSPEC	(1 << 4)
#define PATH_TEAR_MSG_CFLAG_ADSPEC		(1 << 5)

   struct rsvp_integrity_info    integrity_info; /*Info from INTEGRITY object.*/
   struct rsvp_session_info      session_info;   /* Info from SESSION Object.*/
   struct rsvp_hop_info	         hop_info;       /* Info from RSVP_HOP Object.*/
   struct rsvp_st_info	         st_info;        /* Info from SENDER_TEMPLATE.*/
   struct rsvp_sender_tspec_info sender_tspec_info; /* Info from SENDER_TSPEC*/
   struct rsvp_adspec_info	 adspec_info;    /* Info from ADSPEC Object.*/ 
};

/*---------------------------------------------------------------------------
 * This structure carries info extracted from a RSVP Resv Tear Message.
 *-------------------------------------------------------------------------*/
struct rsvp_resv_tear_msg_info
{
   u_int32_t			cflags;
#define RESV_TEAR_MSG_CFLAG_INTEGRITY		(1 << 0)
#define RESV_TEAR_MSG_CFLAG_SESSION		(1 << 1)
#define RESV_TEAR_MSG_CFLAG_HOP			(1 << 2)
#define RESV_TEAR_MSG_CFLAG_SCOPE		(1 << 3)
#define RESV_TEAR_MSG_CFLAG_STYLE		(1 << 4)

   struct rsvp_integrity_info	integrity_info;
   struct rsvp_session_info	session_info;
   struct rsvp_hop_info		hop_info;
   struct rsvp_style_info	style_info;   
};

/*----------------------------------------------------------------------------
 * This structure carries info extracted from Resv Conf Message.
 *--------------------------------------------------------------------------*/
struct rsvp_resv_conf_msg_info
{
   u_char 	rsvp_msg_ttl; /* We need to preserve the TTL of Message Header*/
   u_int32_t    cflags;
#define RESV_CONF_MSG_CFLAG_INTEGRITY		(1 << 0)
#define RESV_CONF_MSG_CFLAG_SESSION		(1 << 1)
#define RESV_CONF_MSG_CFLAG_ERR_SPEC		(1 << 2)
#define RESV_CONF_MSG_CFLAG_RESV_CONF		(1 << 3)
#define RESV_CONF_MSG_CFLAG_STYLE		(1 << 4)

   struct rsvp_integrity_info integrity_info;
   struct rsvp_session_info   session_info;
   struct rsvp_err_spec_info  err_spec_info;
   struct rsvp_resv_conf_info resv_conf_info;
   struct rsvp_style_info     style_info;
};

/*---------------------------------------------------------------------------
 * This structure carries the info in RSVP Hello Message. Used for REQ and
 * ACK Message.
 *-------------------------------------------------------------------------*/
struct rsvp_hello_msg_info
{
   u_char			cflags;
#define HELLO_REQ_MSG_CFLAG_INTEGRITY		(1 << 0)
   struct rsvp_integrity_info   integrity_info;
   u_int32_t    		src_inst;
   u_int32_t			dst_inst;
};


/*----------------------------------------------------------------------------
 * Function Prototypes 
 *--------------------------------------------------------------------------*/
void rsvp_packet_output_forward (struct stream *, 
                                 size_t);
struct rsvp_packet *rsvp_packet_new (size_t);
void rsvp_packet_free (struct rsvp_packet *);

/* RSVP FIFO Utilities.*/
struct rsvp_packet *rsvp_packet_fifo_head (struct rsvp_packet_fifo *);
struct rsvp_packet_fifo *rsvp_packet_fifo_new (void);
void rsvp_packet_fifo_push (struct rsvp_packet_fifo *, 
                            struct rsvp_packet *);
struct rsvp_packet *rsvp_packet_fifo_pop (struct rsvp_packet_fifo *);
void rsvp_packet_fifo_flush (struct rsvp_packet_fifo *);
void rsvp_packet_fifo_free (struct rsvp_packet_fifo *);


void rsvp_packet_add_to_if   (struct rsvp_if *, 
                              struct rsvp_packet *);
void rsvp_packet_del_from_if (struct rsvp_if *);

struct stream *rsvp_stream_copy (struct stream *, 
                                 struct stream *);

void rsvp_packet_decode_msg_hdr (struct stream *,
                                 struct rsvp_msg_hdr *);
int rsvp_packet_verify_checksum (struct rsvp_msg_hdr *);
int rsvp_packet_verify_msg_hdr (struct rsvp_packet *, 
                                struct rsvp_if *,
                                struct rsvp_msg_hdr *); 

/* Specific Message Handler Routines. */
void rsvp_packet_path_receive (struct rsvp_msg_hdr *, 
                               struct rsvp_packet *,
                               struct rsvp_if *); 

void rsvp_packet_resv_receive (struct rsvp_msg_hdr *, 
                               struct rsvp_packet *,
                               struct rsvp_if *);
                               
void rsvp_packet_path_err_receive (struct rsvp_msg_hdr *, 
                                   struct rsvp_packet *,
                                   struct rsvp_if *);
                                   
void rsvp_packet_resv_err_receive (struct rsvp_msg_hdr *, 
                                   struct rsvp_packet *,
                                   struct rsvp_if *); 
                                   
void rsvp_packet_path_tear_receive (struct rsvp_msg_hdr *,
                                    struct rsvp_packet *,
                                    struct rsvp_if *);
                                   
void rsvp_packet_resv_tear_receive (struct rsvp_msg_hdr *,
                                    struct rsvp_packet *,
                                    struct rsvp_if *);
                                    
void rsvp_packet_resv_conf_receive (struct rsvp_msg_hdr *,
                                    struct rsvp_packet *,
                                    struct rsvp_if *);
                                   
void rsvp_packet_hello_receive (struct rsvp_msg_hdr *,
                                struct rsvp_packet *,
                                struct rsvp_if *);
                               
void rsvp_packet_bundle_receive (struct rsvp_msg_hdr *,
                                 struct rsvp_packet *,
                                 struct rsvp_if *);
                                 

 
struct rsvp_packet *rsvp_packet_receive (int , struct interface **);

int rsvp_read  (struct thread *);
int rsvp_write (struct thread *);

/*----------------------------------------------------------------------------
 * Prototypes exported by rsvp_packet module to its user for sending and 
 * receiving all types of RSVP Messages.
 *--------------------------------------------------------------------------*/
void rsvp_packet_send_path (struct rsvp_ip_hdr_info *,
                            struct rsvp_path_msg_info *,
                            struct in_addr *,
                            struct rsvp_if *);

void rsvp_packet_send_resv (struct rsvp_ip_hdr_info *,
                            struct rsvp_resv_msg_info *,
                            struct in_addr *,
                            struct rsvp_if *);

void rsvp_packet_send_path_err (struct rsvp_ip_hdr_info *,
                                struct rsvp_path_err_msg_info *,
                                struct in_addr *,
                                struct rsvp_if *);

void rsvp_packet_send_resv_err (struct rsvp_ip_hdr_info *,
                                struct rsvp_resv_err_msg_info *,
                                struct in_addr *,
                                struct rsvp_if *);

void rsvp_packet_send_path_tear (struct rsvp_ip_hdr_info *,
                                 struct rsvp_path_tear_msg_info *,
                                 struct in_addr *,
                                 struct rsvp_if *);

void rsvp_packet_send_resv_tear (struct rsvp_ip_hdr_info *,
                                 struct rsvp_resv_tear_msg_info *,
                                 struct in_addr *,
                                 struct rsvp_if *);

void rsvp_packet_send_resv_conf (struct rsvp_ip_hdr_info *,
                                 struct rsvp_resv_conf_msg_info *,
                                 struct in_addr *,
                                 struct rsvp_if *);

void rsvp_packet_send_hello_req (struct rsvp_ip_hdr_info *,
                                 struct rsvp_hello_msg_info *,
                                 struct in_addr *,
                                 struct rsvp_if *);
 
void rsvp_packet_send_hello_ack (struct rsvp_ip_hdr_info *,
                                 struct rsvp_hello_msg_info *,
                                 struct in_addr *,
                                 struct rsvp_if *); 
