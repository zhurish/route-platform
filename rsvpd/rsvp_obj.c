/* The RSVP Object/Ctypes Libray Module. It provides utilities for 
** Encode/Decode RSVP Objects/Ctypes. 
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

#include <zebra.h>

#include "memory.h"
#include "linklist.h"
#include "prefix.h"
#include "lsp.h"
#include "stream.h"
#include "vty.h"
#include "avl.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_if.h"
#include "rsvpd/rsvp_obj.h"
#include "rsvpd/rsvp_packet.h"
#include "rsvpd/rsvp_state.h"

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_hdr
 * Input    : s = Pointer to struct stream on which to encode the header.
 *          : len = Value for "Length" field in the object header.
 *          : class_num = Value for "Class-Num" field in object header
 *          : ctype = Value for "C-Type" field in object header.
 * Output   : Returns number of bytes written.
 * Synopsis : This function encodes a object header on stream with parameters 
 *            from the input arguments.
 * Callers  : (TBD)..
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_hdr (struct stream *s,
                     u_int16_t len,
                     u_char class_num,
                     u_char ctype)
{
   int write = 0;

   /* Sanity Check. */
   if (!s)
      return write;

   write += stream_putw (s, len);
   write += stream_putc (s, class_num);
   write += stream_putc (s, ctype);

   return write;
}

int
rsvp_obj_encode_integrity (struct stream 	      *s,
                           struct rsvp_integrity_info *integrity_info)
{
   int write = 0;

   return write;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_session_lsp_tun_ipv4
 * Input    : s		       = Pointer to struct stream on which to encode.
 *          : session_info_ipv4= Pointer2 struct rsvp_session_info_lsp_tun_ipv4
 * Output   : Returns number of bytes encoded on the stream.
 * Synopsis : This function encodes a session object of type LSP_TUNNEL_IPV4
 *            (RFC 3209) on the current pointer (s->putp) of the input stream
 *            from the rest of the input arguments.
 * Callers  : (TBD)
 *-------------------------------------------------------------------------*/
static int
rsvp_obj_encode_session_lsp_tun_ipv4 (struct stream *s,
                                      struct rsvp_session_info_lsp_tun_ipv4 
                                      *session_info_ipv4)
{
   int write = 0;
   /* Sanity Check. */
   if (!s || !session_info_ipv4)
      return write;
   /* Encode the header first. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 12, RSVP_CLASS_SESSION,
                                  RSVP_CLASS_SESSION_CTYPE_LSP_TUN_IPV4);
   /* Insert the IPV4 Tunnel End Point Address. */
   write += stream_put_in_addr (s, &session_info_ipv4->addr);
   /* Insert the reserved field of all zeros.*/
   write += stream_putw (s, 0);
   write += stream_putw (s, session_info_ipv4->tun_id);
   write += stream_putl (s, session_info_ipv4->ext_tun_id);

   return write;
}

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_session_lsp_tun_ipv6 
 * Input    : s    = Pointer to struct of type stream 
 *          : addr = Pointer to struct of type in6_addr
 *          : tun_id = Value of tunnel id
 *          : ext_tun_id = Value of extended tunnel id.
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes a rsvp session object of type lsp tunnel
 *            ipv6 on the stream.
 * Callers  ; (TBD) 
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_encode_session_lsp_tun_ipv6 (struct stream *s,
                                      struct rsvp_session_info_lsp_tun_ipv6
                                      *session_info_ipv6)
{
   int write = 0;

   /* Sanity Check. */
   if (!s || !session_info_ipv6)
      return write;
   /* Encode the object header first.*/
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 36, RSVP_CLASS_SESSION,
                                 RSVP_CLASS_SESSION_CTYPE_LSP_TUN_IPV6);

   /* TBD. */
}
#endif

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_session 
 * Input    : s 	   = Pointer to object of type struct stream.
 *          : session_info = Pointer to object of type struct rsvp_session_info
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes a SESSION object onto the stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_session (struct stream *s,
                         struct rsvp_session_info *session_info)
{
   int write = 0;
   
   /* Sanity Check on input arguments.*/
   if (!s || !session_info)
      return write;

   /* Encode based on info type.*/
   switch (session_info->type)
   {
      case SESSION_INFO_LSP_TUN_IPV4:
         write += rsvp_obj_encode_session_lsp_tun_ipv4 (s, 
                              &session_info->session_info_lsp_tun_ipv4_t);
         break;

#ifdef HAVE_IPV6
      case SESSION_INFO_LSP_TUN_IPV6:	
         write += rsvp_obj_encode_session_lsp_tun_ipv6 (s,
                              &session_info->session_info_lsp_tun_ipv6_t);
         break;
#endif /* HAVE_IPV6 */
      default:
         write = -1;
   }
   
   return write;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_obj_encode_st_lsp_tun_ipv4
 * Input    : s = Pointer to struct stream.
 *          : st_info_ipv4 = Pointer to struct rsvp_st_info_lsp_tun_ipv4
 * Output   : Returns number of bytes encoded.
 * Synopsis : This function encodes a RSVP Sender Template object of type
 *            LSP_TUNNEL_IPV4
 * Callers  : (TBD)..
 *-------------------------------------------------------------------------*/
static int
rsvp_obj_encode_st_lsp_tun_ipv4 (struct stream *s,
                                 struct rsvp_st_info_lsp_tun_ipv4 
                                 *st_info_ipv4)                                 
{
   int write = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !st_info_ipv4)
      return write;

   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 8, RSVP_CLASS_ST,
                                 RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV4);
   /* Write the IPV4 Tunnel Sender Address. */
   write += stream_put_in_addr (s, &st_info_ipv4->sender_addr);
   /* Write the reserved field.*/
   write += stream_putw (s, 0);
   /* Write the LSP ID. */
   write += stream_putw (s, st_info_ipv4->lsp_id);

   return write;
}

#ifdef HAVE_IPV6
/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_st_lsp_tun_ipv6
 * Input    : s 	   = Pointer to object of type struct stream.
 *          : st_info_ipv6 = Pointer to struct rsvp_st_info_lsp_tun_ipv6 
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes a Sender Template of type LSP_TUNNEL_IPV6
 *            on the stream from the input parameters.
 * Callers  : (TBD)..
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_encode_st_lsp_tun_ipv6 (struct stream *s,
                                 struct rsvp_st_info_lsp_tun_ipv6 
                                 *st_info_ipv6)
{
   int write = 0;

   return write;
}
#endif /* HAVE_IPV6 */

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_st
 * Input    : s 	= Pointer to object of type struct stream.
 *          : st_info   = Pointer to object of type struct rsvp_st_info.
 * Output   : Returns the number of bytes encoded. If encoding fails returns 
 *            -1.
 * Synopsis : This function encodes a SENDER_TEMPLATE object on the input 
 *            stream.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
int
rsvp_obj_encode_st (struct stream 	*s,
                    struct rsvp_st_info *st_info)
{
   int write = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !st_info)
      return write;

   /* Encode based on info*/
   switch (st_info->type)
   {
      case ST_INFO_LSP_TUN_IPV4:
         write += rsvp_obj_encode_st_lsp_tun_ipv4 (s, 
                                             &st_info->st_info_lsp_tun_ipv4_t);
         break;

#ifdef HAVE_IPV6
      case ST_INFO_LSP_TUN_IPV6:
         write += rsvp_obj_encode_st_lsp_tun_ipv6 (s,
                                             &st_info->st_info_lsp_tun_ipv6_t);
         break;
#endif /* HAVE_IPV6 */
      default :
         write = -1;
   }
   
   return write;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_lro_gen
 * Input    : s        = Pointer to type struct stream.
 *          : lro_info_gen = Pointer to object of type struct rsvp_lro_info_gen.
 * Output   : The number of bytes encoded on the stream.
 * Synopsis : This function encodes a generic lro object on the stream.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
static int
rsvp_obj_encode_lro_gen (struct stream 	          *s,
                         struct rsvp_lro_info_gen *lro_info_gen)
{
   int write = 0;

   /* Sanity Check. */
   if (!s || !lro_info_gen)
      return write;

   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 4, RSVP_CLASS_LRO,
                                 RSVP_CLASS_LRO_CTYPE_GEN);
   /* Insert the reserved field */
   write += stream_putw (s, 0); 
   write += stream_putw (s, lro_info_gen->l3pid);

   return write;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_lro_atm
 * Input    : s        = Pointer to object of type struct stream.
 *          : lro_info = Pointer to object of type struct rsvp_lro_info.
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes  a LRO object of type ATM on the stream 
 *            from the input parameters.
 * Callers  : (TBD)
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_encode_lro_atm (struct stream 	      	  *s,
                         struct rsvp_lro_info_atm *lro_info_atm)
{
   int write = 0;
   
   /* Sanity Check. */
   if (!s || !lro_info_atm)
      return write;
   /* Encode the Common RSVP Header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 12, RSVP_CLASS_LRO,
                                 RSVP_CLASS_LRO_CTYPE_ATM);
   /* Insert the L3PID Fie;d. */
   write += stream_putw (s, lro_info_atm->l3pid);
   /* If merge capability is set. */
   if (lro_info_atm->m)
      /* Set the M-Bit as the highest order bit in min_vpi. See section 4.2.2
         in RFC 3209 for more info */
      lro_info_atm->min_vpi |= 0x8000;
      
   write += stream_putw (s, lro_info_atm->min_vpi);
   write += stream_putw (s, lro_info_atm->min_vci);
   write += stream_putw (s, lro_info_atm->max_vpi);
   write += stream_putw (s, lro_info_atm->max_vci);

   return write;      
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_lro_fr
 * Input    : s 	  = Pointer to object of type struct stream
 *          : lro_info_fr = Pointer to object of type struct rsvp_lro_info_fr. 
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function writes the LRO object header of type FR.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_encode_lro_fr (struct stream 		*s,
                        struct rsvp_lro_info_fr *lro_info_fr)
{
   int write = 0;

   /* Sanity Check. */
   if (!s || !lro_info_fr)
      return write;

   /* Insert the Common Object Header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 12, RSVP_CLASS_LRO,
                                 RSVP_CLASS_LRO_CTYPE_FR);
   /* Insert the L3PID Field. */ 
   write += stream_putw (s, lro_info_fr->l3pid);
   /* Map the DLI field on min DLCI. */
   if (lro_info_fr->dlci_len == 2)
      lro_info_fr->dlci_min |= (0x0001 << 24);
   /* Insert the Min DLCI Fields. */
   write += stream_putw (s, lro_info_fr->dlci_min);
   write += stream_putw (s, lro_info_fr->dlci_max);

   return write;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_lro
 * Input    : s 	= Pointer to object of type struct stream.
 *          : lro_info  = Pointer to object of type struct rsvp_lro_info.
 * Output   : Returns the number of bytes encoded on the stream. In case of 
 *          : error returns -1.
 * Synopsis : This function encodes a LRO Object on the stream.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_lro (struct stream        *s,
                     struct rsvp_lro_info *lro_info)
{
   int write = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !lro_info)
      return write;

   /* Encode based on info type.*/
   switch (lro_info->type)
   {
      case LRO_INFO_TYPE_GEN:
         write += rsvp_obj_encode_lro_gen (s, &lro_info->lro_info_gen_t);
         break;

      case LRO_INFO_TYPE_ATM:
         write += rsvp_obj_encode_lro_atm (s, &lro_info->lro_info_atm_t);
         break;
 
      case LRO_INFO_TYPE_FR:
         write += rsvp_obj_encode_lro_fr (s, &lro_info->lro_info_fr_t);
         break;

      default:
         return -1;
   }

   return write;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_label
 * Input    : s = Pointer to object of type struct stream .
 *          : label = u_int32_t value of the label to be encoded.
 * Output   : Returns the number of bytes to be encoded.
 * Synopsis : This function encodes a label object of the stream.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_label (struct stream 	      *s,
   	               struct rsvp_label_info *label_info)
{
   int write = 0;

   /* Sanity Check. */
   if (!s || !label_info)
      return write;
   /* Insert the common object header, */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 4, RSVP_CLASS_LABEL,
                                 RSVP_CLASS_LABEL_CTYPE_GEN); 
   /* Insert the label value. */
   write += stream_putl (s, label_info->label);

   return write;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_ero_subobj_hdr
 * Input    : s = Pointer to object of type struct stream.
 *          : loose = loose but info
 *          : type = Type of the ERO Subobject.
 *          : length = Length of the ERO Subobject.
 * Output   : It returns the number of bytes written .
 * Synopsis : This function inserts the ERO Subobject header on the stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_ero_subobj_hdr (struct stream *s,
                                u_char loose,
                                u_char type,
                                u_char length)
{
   int write = 0;
   
   /* Sanity Check. */
   if (!s)
      return write;
   
   /* If loose is set then map it in the highest bit in type fields. */
   if (loose)
      type |= 0x8f;
   /* Insert the Type field. */
   write += stream_putc (s, type);
   /* Insert the Length Field.*/
   write += stream_putc (s, length);
   
   return write;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_ero_subobj_ipv4_prefix
 * Input    : s = Pointer to object of type struct stream.
 *          : loose = The loose bit.
 *          : addr = Pointer to struct in_addr
 *          : prefix_length = The length of the prefix.
 * Output   : Returns the number of bytes written on the stream.
 * Synopsis : This function encodes a ERO subobject of type IPV4 Prefix.
 * Callers  : (TBD)
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_ero_subobj_ipv4_prefix (struct stream *s,
                                        u_char loose,
                                        struct in_addr *addr,
                                        u_char prefix_len)
{
   int write = 0;

   /* Sanity Check. */
   if (!s)
      return write;
   /* Insert the common sub object header. */
   write += rsvp_obj_encode_ero_subobj_hdr (s, loose, 
                                            RSVP_CLASS_ERO_SUBOBJ_IPV4_PREFIX,
                                            RSVP_ERO_SUBOBJ_HDR_LEN + 6);
   write += stream_put_in_addr (s, addr);
   write += stream_putc (s, prefix_len);
   write += stream_putc (s, 0);

   return write;
}

#ifdef HAVE_IPV6
int
rsvp_obj_encode_ero_subobj_ipv6_prefix (struct stream *s,
                                        u_char loose,
                                        struct in6_addr *addr,
                                        u_char prefix_length)
{
   int write = 0;

   return write;
}
#endif /* HAVE_IPV6 */ 

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_ero_subobj_as 
 * Input    : s = Pointer to object of type struct stream.
 *          : loose = The loose bit.
 *          : as_num = The AS Number.
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes a ERO subobject of type AS Number.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_ero_subobj_as (struct stream *s,
                               u_char loose,
                               u_int16_t as_num)
{
   int write = 0;
   
   /* Sanity Check. */
   if (!s)
      return write;
   /* Insert the subobject header. */
   write += rsvp_obj_encode_ero_subobj_hdr (s, loose, RSVP_CLASS_ERO_SUBOBJ_AS,
                                            RSVP_ERO_SUBOBJ_HDR_LEN + 2);
   /* Insert the AS Number. */
   write += stream_putw (s, as_num);

   return write;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_rro_subobj_hdr 
 * Input    : s = Pointer to object of type struct stream.
 *          : type = The Type field.
 *          : length = The Length Field.
 * Output   : Returns the number of bytes written
 * Synopsis : This function encodes a RRO sub object header on the stream.
 * Callers  : (TBD)..
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_rro_subobj_hdr (struct stream *s,
                                u_char type,
                                u_char length)
{
   int write = 0;
 
   /* Sanity Check. */
   if (!s)
      return write;

   write += stream_putc (s, type);
   write += stream_putc (s, length);

   return write;
}  

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_rro_subobj_ipv4_addr
 * Input    : s = Pointer to struct stream
 *          : addr = Pointer to struct in_addr (IPV4 address field)
 *          : prefix_length = The Prefix Length Field.
 * Output   : Returns number of bytes encoded 
 * Synopsis : This function encodes a RRO subobject of type IPV4 Address.
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_rro_subobj_ipv4_addr (struct stream *s,
                                      struct in_addr *addr,
                                      u_char prefix_length,
                                      u_char flags)
{
   int write = 0;

   /* Sanity Check. */
   if (!s)
      return write;

   /* Insert the Common Header for RRO Subobjects. */
   write += rsvp_obj_encode_rro_subobj_hdr (s, 
                                            RSVP_CLASS_RRO_SUBOBJ_IPV4_ADDR,
                                            RSVP_RRO_SUBOBJ_HDR_LEN + 6);
   /* Insert the IPV4 Address.*/
   write += stream_put_in_addr (s, addr);
   /* Insert prefix length. */
   write += stream_putc (s, prefix_length);
   /* Insert the flags field. */
   write += stream_putc (s, flags);

   return write;
}

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_rro_subobj_ipv6_addr
 * Input    : s = Pointer to object iof type struct stream.
 *          : addr = Pointer to IPV6 address in struct in6_addr
 *          : prefix_length = Prefix Length field.
 *          : flags = The flags field.
 * Output   : Returns the number of bytes written.
 * Synopsis : This function encodes a RRO subobject of type IPV6 Address.
 * Callers  : (TBD)..
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_rro_subobj_ipv6_addr (struct stream *s,
                                      struct in6_addr *addr,
                                      u_char prefix_length,
                                      u_char flags)
{
   int write = 0;

   return write;
}
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_rro_subobject_label
 * Input    : s = Pointer to object of type struct stream.
 *          : flags = The flags field.
 *          : obj_info = Pointer to struct rsvp_
 *--------------------------------------------------------------------------*/
// (TBD)

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_flt_spec_lsp_tun_ipv4
 * Input    : s = Pointer to struct stream.
 *          : addr = struct in_addr  of the initaitor of the Tunnel
 *          : lsp_id = The LSP ID of the Tunnel.
 * Output   : Returns the number of bytes encoded on the stream.
 * Synopsis : This function encodes a Filter Spec object on the stream.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
int
rsvp_obj_encode_flt_spec_lsp_tun_ipv4 (struct stream *s,
                                       struct in_addr *addr,
                                       u_int16_t lsp_id)
{ 
   int write = 0;
 
   /* Sanity Check.*/
   if (!s)
      return write;

   /* Insert the Header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 8, RSVP_CLASS_FLT_SPEC,
                                 RSVP_CLASS_FLT_SPEC_CTYPE_LSP_TUN_IPV4);
   write += stream_put_in_addr (s, addr);
   write += stream_putw (s, 0);
   write += stream_putw (s, lsp_id);

   return write;
}

#ifdef HAVE_IPV6
/*--------------------------------------------------------------------------
 * Function : rsvp_obj_encode_flt_spec_lsp_tun_ipv6
 * Input    : s = Pointer to struct stream.
 *          : addr = Pointer to struct in_addr.
 *          : lsp_id = The LSP ID of the Tunnel.
 * Output   ; Returns the number of bytes encoded in the stream.
 * Synopsis : This function encodes a FILTER SPEC of type LSP_TUN_IPV6
 * Callers  ; (TBD).
 *------------------------------------------------------------------------*/
int
rsvp_obj_encode_flt_spec_lsp_tun_ipv6 (struct stream *s,
                                       struct in_addr *addr,
                                       u_int16_t lsp_id)
{
   int write = 0;

   /* Sanity Check. */
   if (!s)
      return;
 
   // TBD
   return write;
}
#endif            

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_sa_tun
 * Input    : s = Pointer to object struct stream.
 *          : setup_prio = The Value of Setup Priority.
 *          : hold_prio = The value of Hold Priority.
 *          : local_prot = Flag that Local Protection Desired.
 *          : label_record = Label Recording Desired when doing a Route Record.
 *          : set_style = Flag that says SE style is desired.
 * Output   : Returns the number of bytes  encoded in the SESSION ATTRIBUTE
 *            Object.
 * Synopsis : This function encodes a Session Attribute Object of type
 *            LSP TUNNEL into the stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_sa_tun (struct stream *s,
                        u_char setup_prio,
                        u_char hold_prio,
                        u_char local_prot,
                        u_char label_record,
                        u_char se_style,
                        char *sess_name)
{
   int write = 0;
   u_char flags = 0x00;
   u_char len = 0;
   

   /* Sanity Check. */
   if (!s)
      return write;
   /* Calculate the Length. */
   len = 4 + len/4 + (4 - len %4);
   /* Encode the Header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + len, RSVP_CLASS_SA,
                                 RSVP_CLASS_SA_CTYPE_LSP_TUN); 
   write += stream_putc (s, setup_prio);
   write += stream_putc (s, hold_prio);
   /* Calculate the Flags. */
   if (local_prot)
      flags |= 0x01;
   if (label_record)
      flags |= 0x02;
   if (se_style)
      flags |= 0x04;
   /* Insert the flags. */
   write += stream_putc (s, flags); 
   /* Insert the name len) */
   write += stream_putc (s, strlen (sess_name));
   /* Insert the name */
   stream_put (s, (void *)sess_name, (size_t)strlen (sess_name));
   write += strlen (sess_name);
   /* Check how many padding is required. */
   len = 4 - strlen(sess_name)%4;
   /* Insert the padding. */
   while (len)
   {
      write += stream_putc (s, '\0');
      len--;   
   }
  
   return write; 
}

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_sa_tun_ra
 * Input    : s = Pointer to object of type struct stream.
 *          : excl_any = The 32-bit map for Exclude Any
 *          : incl_any = The 32-bit map for Include Any
 *          : incl_all = The 32-bit map for Include All.
 *          : setup_prio = The setup priority.
 *          : hold_prio = The hold priority.
 *          : label_prot = The flag for Label Protection.
 *          : label_record = The flag for Label Record Desired.
 *          : se-style = The SE style desired.
 *          : sess_name = The name of the session.
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes a Session Attribute of type LSP_TUNNEL
 *            with Resource Affinities.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_sa_tun_ra (struct stream *s,
                           u_int32_t excl_any,
                           u_int32_t incl_any,
                           u_int32_t incl_all,
                           u_char setup_prio,
                           u_char hold_prio,
                           u_char local_prot,
                           u_char label_record,
                           u_char se_style,
                           char *sess_name)
{
   int write = 0;
   int name_len = strlen (sess_name);
   int pad_len  = 4 - (name_len % 4); 
   u_char flags = 0x00;

   /* Sanity Check. */
   if (!s)
      return write;

   /* Encode the Header on the Packet. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + name_len + pad_len,
                                 RSVP_CLASS_SA, RSVP_CLASS_SA_CTYPE_LSP_TUN_RA);
   /* Encode the affinity values. */
   write += stream_putl (s, excl_any);
   write += stream_putl (s, incl_any);
   write += stream_putl (s, incl_all);
   /* Encode Priorities. */
   write += stream_putc (s, setup_prio);
   write += stream_putc (s, hold_prio);
   /* Encode Flags. */
   if (local_prot)
      flags |= 0x01;
   if (label_record)
      flags |= 0x02;
   if (se_style)
      flags |= 0x04;
   write += stream_putc (s, flags);
   /* Encode the Session Name. */
   stream_put (s, sess_name, name_len);
   write += name_len;
   /* Insert padding bits. */
   while (pad_len)
   {
      stream_putc (s, '\0');
      pad_len--;
   }

   return write;
}
  
/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_hop_ipv4
 * Input    : s = Pointer to object of type struct stream.
 *          : addr = struct in_addr of the phop or nhop.
 *          : lih  = Logical Interface Handle.
 * Output   : Returns the number of bytes encoded in the stream.
 * Synopsis : This function encodes a RSVP_HOP object of type IPV4.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_hop_ipv4 (struct stream *s,
                          struct in_addr *addr,
                          u_int32_t lih)
{
   int write = 0;
   
   /* Sanity Check. */
   if (!s)
      return write;
   /* Insert the Object Header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 8, RSVP_CLASS_HOP,
                                 RSVP_CLASS_HOP_CTYPE_IPV4);
   /* Encode the address. */
   write += stream_put_in_addr (s, addr);
   /* Encode the Logical Interface Handle. */
   write += stream_putl (s, lih);
   
   return write;
}

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_hop_ipv6 
 * Input    : s = Pointer to object of type struct stream.
 *          : addr = Pointer to IPV6 Addr of the HOP.
 *          : lih = Logical Interface Handle.
 * Output   : Returns the number of bytes encoded on the stream.
 * Synopsis : This function encodes a IPV6 address on a stream.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_hop_ipv6 (struct stream *s,
                          struct in6_addr *addr,
                          u_int32_t lih)
{
   int write = 0;

   /* Sanity Check. */
   if (!s)
      return write;

   return write;
}
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_timeval
 * Input    : s = Pointer to object of type struct stream.
 *          : timeval = The Refresh Time Period in milliseconds.
 * Output   : Returns the number of bytes encoded on the header.
 * Synopsis : This function encodes a time values info on the stream.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_timeval (struct stream *s,
                         u_int32_t timeval)
{
   int write = 0;

   /* Sanity Check. */
   if (!s)
      return write;
   /* Encode the header on the stream. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 4, RSVP_CLASS_TIMEVAL,
                                 RSVP_CLASS_TIMEVAL_CTYPE_GEN);
   write += stream_putl (s, timeval);

   return write;    
}

/*---------------------------------------------------------------------------
 * Function : rsvp_encode_obj_err_spec_ipv4
 * Input    : s = Pointer to object of type struct stream.
 *          : node_addr = IPV4 Address in struct in_addr.
 *          : in_place_flag = Flag 
 *          : not_guilty_flag = flag
 *          : err_code = The Error Code.
 *          : err_value = The value of the error.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function encodes a ERROR_SPEC Object on the stream.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_encode_obj_err_spec_ipv4 (struct stream *s,
                               struct in_addr *node_addr,
                               u_char in_place_flag,
                               u_char not_guilty_flag,
                               u_char err_code,
                               u_int16_t err_val)
{
   int write = 0;
   u_char flags = 0x00;
   /* Sanity Check. */
   if (!s || !node_addr)
      return write;

   /* Insert the Common RSVP Object Header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 8, RSVP_CLASS_ERR_SPEC,
                                 RSVP_CLASS_ERR_SPEC_CTYPE_IPV4);
   /* Insert the IPV4 Node Address. */
   write += stream_put_in_addr (s, node_addr);
   /* Insert the flags. */
   if (in_place_flag)
      flags |= 0x01;
   if (not_guilty_flag)
      flags |= 0x02;
   /* Write the flags on the stream.*/
   write += stream_putc (s,flags);
   /* Insert the Error Code. */
   write += stream_putc (s, err_code);
   write += stream_putw (s, err_val);

   return write;
}  

#ifdef HAVE_IPV6
/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_err_spec_ipv6
 * Input    : s = Pointer to object of type struct stream.
 *          : node_addr = Pointer to object of type struct in6_addr.
 *          : in_place_flag = Flag
 *          : not_guilty_flag = Flag.
 *          : err_code = The Error Code.
 *          : err_val = The value of the error.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function encodes ERROR_SPEC of type IPV6.
 * Callers  : (TBD).
 *-------------------------------------------------------------------------*/
int
rsvp_obj_encode_err_spec_ipv6 (struct stream *s,
                               struct in6_addr *node_addr,
                               u_char in_place_flag,
                               u_char not_guilty_flag,
                               u_int16_t err_code,
                               u_char err_val)
{
   int write = 0;

   /* Sanity Check. */
   if (!s || !node_addr)
      return write;
   /* Insert the common RSVP objects header. */
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 20, RSVP_CLASS_ERR_SPEC,
                                 RSVP_CLASS_ERR_SPEC_CTYPE_IPV6);
   return write;
}
#endif /* HAVE_IPV6 */

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_encode_style
 * Input    : s = Pointer to object of type struct stream.
 *          : type = The Filyer type.
 * Output   : Ut returns the Number of bytes encoded .
 * Synopsis : This function encodes a RSVP Style Object on the input stream.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_style (struct stream *s,
                       u_char type)
{
   int write = 0;
   u_int32_t value = 0;
   /* Sanity Check.*/
   if (!s)
      return write;
   /* Insert the common RSVP objects header.*/
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 4, RSVP_CLASS_STYLE,
                                 RSVP_CLASS_STYLE_CTYPE_GEN);
   switch (type)
   {
      case RSVP_STYLE_TYPE_WF:
         value |= 0x00000011;
         break;
    
      case RSVP_STYLE_TYPE_FF:
         value |= 0x0000000A;
         break;
   
      case RSVP_STYLE_TYPE_SE:
         value |= 0x00000012;
         break;
   }
   write += stream_putl (s, value);
   
   return write;
}; 

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_frr
 * Input    : s           = Pointer to object of type struct stream.
 *          : setup_prio  = Setup Priority in pointer to u_char 
 *          : hold_prio   = Hold Priority in pointer to u_char
 *          : hop_limit   = Hop Limit in pointer to u_char
 *          : backup_type = The type of backup in pointer to u_char
 *          : bandwidth   = Bandwidth in pointer u_int32_t
 *          : include_any = The include any colors as pointers to u_int32_t.
 *          : exclude_all = The exclude_all colors as pointer to u_int32_t.
 *          : include_all = The include_all colors as pointer to u_int32_t.
 * Output   : Returns number of bytes encoded on the stream.
 * Synopsis : This function encodes a FRR object (RFC 4090) into the stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_encode_frr (struct  stream *s,
                     u_char         *setup_prio,
                     u_char         *hold_prio,
                     u_char  	    *hop_limit,
                     u_char  	    *backup_type,
                     u_int32_t      *bandwidth,
                     u_int32_t      *include_any,
                     u_int32_t      *exclude_any,
                     u_int32_t      *include_all)
{
   int write = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !setup_prio || !hold_prio || !hop_limit || !backup_type ||
       !bandwidth || !include_any || !exclude_any || !include_all)
      return write;

   /* Encode the Common RSVP Object Header.*/
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + 20, RSVP_CLASS_FRR,
                                 RSVP_CLASS_FRR_CTYPE_GEN);
   /* Encode the Setup Prio field.*/
   write += stream_putc (s, *setup_prio);
   /* Encode the Hold Prio field.*/
   write += stream_putc (s, *hold_prio);
   /* Encode the Hop-limit field.*/
   write += stream_putc (s, *hop_limit);
   /* Encode the Flags field.*/
   write += stream_putc (s, *backup_type);
   /* Enocode the Bandwidth field.*/
   write += stream_putl (s, *bandwidth);
   /* Encode the Include-any field.*/
   write += stream_putl (s, *include_any);
   /* Encode the Exclude-any field.*/
   write += stream_putl (s, *exclude_any);
   /* Encode the Include-all field.*/
   write += stream_putl (s, *include_all);

   return write;
};
   
/*--------------------------------------------------------------------------
 * Function : rsvp_obj_encode_detour_ipv4 
 * Input    : s = Pointer to the object of type struct stream.
 *          : detour_info_ipv4 = Pointer to struct rsvp_detour_info_ipv4.
 * Output   : Returns the number of bytes encoded on the stream.
 * Synopsis : This function encodes a DETOUR Object of type IPV4 into 
 *            the stream as per encoding in Section 4.2 in RFC 4090.
 * Callers  : (TBD).
 *-----------------------------------------------------------------------*/
int
rsvp_obj_encode_detour_ipv4 (struct stream *s,
                             struct rsvp_detour_info *detour_info)
{
   int write = 0;
   int num;
   struct listnode *node;
   struct rsvp_detour_element_info_ipv4  *element_info;

   /* Sanity Check on input arguments.*/
   if (!s || !detour_info)
      return write;
   /* Get the number of elements in detour info.*/
   num = detour_info->num;
   
   /* Encode the Common RSVP Object Header.*/
   write += rsvp_obj_encode_hdr (s, RSVP_OBJ_HDR_LEN + (num*4), 
                                 RSVP_CLASS_DETOUR, 
                                 RSVP_CLASS_DETOUR_CTYPE_IPV4);
   /* Write all the PLR_ID/Avoid_Node Pairs  one by one.*/
   for (node = listhead (detour_info->detour_element_list);
        node && num; nextnode (node), num--)
   {
      element_info = (struct rsvp_detour_element_info_ipv4 *) getdata (node);
      
      write += stream_put_in_addr (s, &element_info->plr_id);
      write += stream_put_in_addr (s, &element_info->avoid_node_id);
   }  
   
   return write;
}

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_encode_detour_ipv6
 * Input    : s     	  = Pointer to object of type struct stream.
 *          : detour_info = Pointer to object of type struct rsvp_detour_info.
 * Output   : Returns the number of bytes encoded.
 * Synopsis : This function encodes a DETOUR Object on a stream.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_encode_detour_ipv6 (struct stream 	     *s,
                             struct rsvp_detour_info *detour_info)
{
   int write = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !detour_info)
      return write;

   return write;
}
#endif /* HAVE_IPV6 */                          
 
/*---------------------------------------------------------------------------
 * Objects decoder routines
 *-------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_decode_hdr 
 * Input    : s = Pointer to object of type struct stream.
 *          : length = Pointer to u_int16_t to fill the Length of the object.
 *          : class_num = Pointer to u_char to fill the Class of the object.
 *          ; ctype = Pointer to u_char to fill the C-Type of the object.
 * Output   : Returns number of bytes decoded.
 * Synopsis : This function decodes a object header and fills the 
 *            corresponding input arguments.
 * Callers  : (TBD)
 *-------------------------------------------------------------------------*/
int
rsvp_obj_decode_hdr (struct stream *s,
                     u_int16_t     *length,
                     u_char        *class_num,
                     u_char        *ctype)
{
   struct rsvp_obj_hdr *hdr;
   /* Sanity Check. */
   if (!s || !length || !class_num || !ctype)
      return 0;

   hdr = (struct rsvp_obj_hdr *)STREAM_PNT (s);
   *length = hdr->length;
   *class_num = hdr->class_num;
   *ctype = hdr->ctype;  
   /* Forward the stream. */
   stream_forward (s, RSVP_OBJ_HDR_LEN);
 
   return RSVP_OBJ_HDR_LEN;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_decode_integrity
 * Input    : s = Pointer to object of type struct stream.
 *          : ctype = Ctype of the integrity object.
 *          : size = Size of the integrity object.
 *          : integrity_info = Pointer to struct rsvp_integrity_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes an integrity object from input stream and 
 *            fills integrity info witb relevant info.
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_integrity (struct stream 	      *s,
                           u_char		      ctype,
                           u_int16_t		      size,
                           struct rsvp_integrity_info *integrity_info)
{
   int read = 0;

   return read;
}
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_session_lsp_tun_ipv4
 * Input    : s = Pointer to struct stream from where to decode 
 *          : session_info = Pointer to struct rsvp_session_info
 *          : size = Size of the object. 
 * Output   : Returns number of bytes decoded.
 * Synopsis : This internal function decodes session info from the stream and 
 *            fills the relevant info into input arg session_info
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_session_lsp_tun_ipv4 (struct stream 	  	*s,
                                      u_int16_t			size,
                                      struct rsvp_session_info 	*session_info)
{
   int read = 0;
   /* Sanity Check. */
   if (!s || !session_info)
      return read ;

   session_info->type = SESSION_INFO_LSP_TUN_IPV4;
   session_info->session_info_lsp_tun_ipv4_t.addr.s_addr = stream_get_ipv4 (s);
   read += 4;
   session_info->session_info_lsp_tun_ipv4_t.tun_id = stream_getw (s);
   read += 2;
   session_info->session_info_lsp_tun_ipv4_t.ext_tun_id = stream_getl (s);
   read += 4;

  /* Check in case we have read longer. If yes then trigger error*/
  if (read > size)
     return -1;
  /* Check if the object is padded, if yes, then forward the stream by
     padded bytes.*/
  else if (read < size)
     stream_forward (s, size - read);
    
   return size;
}

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_session_lsp_tun_ipv6
 * Input    : s = Pointer to the struct stream.
 *          : session_info = Pointer to object of type struct rsvp_session_info
 *          : size = The length of the object.
 * Output   : Returns number of bytes decoded.
 * Synopsis : This internal function decodes a object of type session and ctype
 *            lsp_tunnel_ipv6 and fills up the input struct rsvp_session_info.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_session_lsp_tun_ipv6 (struct stream 		*s,
                                      u_int16_t			size,
                                      struct rsvp_session_info 	*session_info)
{
   int read = 0;

   return read;
}
#endif /* HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_session
 * Input    : s = Pointer to the struct stream.
 *          : ctype = The C-Type of the object.
 *          : size = The length of the object.
 *          : session_info = Pointer to object of type struct rsvp_session_info
 * Output   : Returns number of bytes decoded.
 * Synopsis : This internal function decodes a object of type session and ctype
 *          :  and fills up the input struct rsvp_session_info.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_decode_session (struct stream		  *s,
                         u_char 		  ctype,
                         u_int16_t		  size,
                         struct rsvp_session_info *session_info)
{
   int read = 0;
   
   switch (ctype)
   {
      case RSVP_CLASS_SESSION_CTYPE_LSP_TUN_IPV4:
         read = rsvp_obj_decode_session_lsp_tun_ipv4 (s, size, session_info);
         break ;
#ifdef HAVE_IPV6
      case RSVP_CLASS_SESSION_CTYPE_LSP_TUN_IPV6:
         read = rsvp_obj_decode_session_lsp_tun_ipv6 (s, size, session_info);
         break;
#endif /* HAVE_IPV6 */
      /* If c-type doesn't match then error.*/ 
      default :
         read = -1; 
   }
   return read;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_st_lsp_tun_ipv4
 * Input    : s = Pointer to struct stream.
 *          : size = The size of the object to decode.
 *          : st_info = Pointer to struct rsvp_st_info
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes SENDER_TEMPLATE object of type LSP_TUN_IPV4
 *            and fills the st_info.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_st_lsp_tun_ipv4 (struct stream       *s,
                                 u_int16_t           size,
                                 struct rsvp_st_info *st_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s)
      return read;
  /* Set the type of LSP Info. */
   st_info->type = ST_INFO_LSP_TUN_IPV4;
  /* Get the sender's address. */
   st_info->st_info_lsp_tun_ipv4_t.sender_addr.s_addr  = stream_get_ipv4 (s);
   read += 4;
  /* Check if the reserved field is zero. */
   if (stream_getw (s))
      return read;
   read += 2;
   /* Get the LSP ID. */
   st_info->st_info_lsp_tun_ipv4_t.lsp_id = stream_getw (s);
   read += 2;

   /* Check that if we have read more then the size.*/
   if (read > size)
      return -1;
   /* Check we the object is padded.*/
   else if (read < size)
      stream_forward (s, size - read);
   
   return size; 
}

#ifdef HAVE_IPV6
/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_st_lsp_tun_ipv6
 * Input    : s = Pointer to object of type struct stream.
 *          : size = The size of the object to decode.
 *          : st_info = Pointer to object of type struct rsvp_st_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes SENDER_TEMPLATE obj. of type LSP_TUN_IPV6.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_st_lsp_tun_ipv6 (struct stream       *s,
                                 u_int16_t           size,
                                 struct rsvp_st_info *st_info)
{
   int read = 0;

   return read;

}
#endif

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_st
 * Input    : s = Pointer to object of type struct stream.
 *          : ctype = The C-Type of the sender template object.
 *          : size = The size of the object to be decoded.
 *          : st_info = Pointer to object of type struct rsvp_st_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes SENDER_TEMPLATE obj
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_decode_st (struct stream 	*s,
                    u_char 	  	ctype,
                    u_int16_t	  	size,
                    struct rsvp_st_info *st_info)
{
   int read = 0;
   
   switch (ctype)
   {
      case RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV4:
         read = rsvp_obj_decode_st_lsp_tun_ipv4 (s, size, st_info);
         break;
#ifdef HAVE_IPV6 
      case RSVP_CLASS_ST_CTYPE_LSP_TUN_IPV6:
         read = rsvp_obj_decode_st_lsp_tun_ipv6 (s, size, st_info);
         break; 
#endif /*HAVE_IPV6 */ 
   
      default:
         read = -1;
   }
   return read;
}
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_lro_gen
 * Input    : s = Pointer to object of struct stream.
 *          : size = Size of the object to be decoded.
 *          : lro_info = Pointer to object of type struct rsvp_lro_info.
 * Output   ; Returns the number of bytes decoded from the object.
 * Synopsis : This function decodes a LRO Object withput Label Range from the
 *            stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_lro_gen (struct stream        *s,
                         u_int16_t	      size,
                         struct rsvp_lro_info *lro_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || lro_info)
      return read;
   /* Set the type as generic. */
   lro_info->type = LRO_INFO_TYPE_GEN;
   /* Skip the Reserved Field. */
   stream_forward (s, 2);
   read += 2;
   /* Get the L3PID. */
   lro_info->lro_info_gen_t.l3pid = stream_getw (s);
   read += 2;
   /* Check if the decoded bytes are more than the size.*/
   if (read > size)
     return -1;
   /* Check if the object is padded.*/
   else if(read < size)
      stream_forward (s, size - read);

   return size;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_lro_atm
 * Input    : s = Pointer to object of type struct stream.
 *          : size = The size of the object.
 *          : lro_info = Pointer to object of type struct rsvp_lro_info
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a LRO object with ATM Range.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_lro_atm (struct stream *s,
                         u_int16_t     size,
                         struct rsvp_lro_info *lro_info)
{
   int read = 0;
   u_int16_t value = 0;

   /* Sanity Check. */
   if (!s || !lro_info)
      return read;
   /* Set the Type of LRO Info */
   lro_info->type = LRO_INFO_TYPE_ATM;
   /* Skipe the reserved field. */
   stream_forward (s, 2);
   read += 2;
   /* Extract the L3PID Info. */
   lro_info->lro_info_atm_t.l3pid = stream_getw (s);
   /* Extract the Merge Capability of the ATM LSR. + Min. VPI */
   value = stream_getw (s);
   read += 2;
   /* Extract the M Bit. */ 
   if (value & 0xF000)  
      lro_info->lro_info_atm_t.m = 1;
   /* Extract the Min VPI. */
   lro_info->lro_info_atm_t.min_vpi = value & 0x0FFF;
   /* Extract the Min. VCI Info. */
   lro_info->lro_info_atm_t.min_vci = stream_getw (s);
   read += 2;
   /* Extract the Max VPI. */
   lro_info->lro_info_atm_t.max_vpi = stream_getw (s) & 0x0FFF;
   read +=2;
   lro_info->lro_info_atm_t.max_vci = stream_getw (s);
   read +=2;

   /* Check if we have read longer then the size.*/
   if ( read > size)
      return -1;
   /* Check if some bytes are padded for word alignment.*/
   else if (read < size)
      stream_forward (s, size - read);

   return size;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_lro_fr
 * Input    : s = Pointer to object of type struct stream.
 *          : size = The size of the object.
 *          : lro_info = Pointer to object of type struct rsvp_lro_info.
 * Output   : Returns number of bytes decoded from the stream.
 * Synopsis : This function decodes LRO object with FR Range from the input
 *            stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int 
rsvp_obj_decode_lro_fr (struct stream        *s,
                        u_int16_t            size,
                        struct rsvp_lro_info *lro_info)
{
   int read = 0;
   u_int32_t word; 
   /* Sanity Check. */
   if (!s || !lro_info)
      return read;

   /* Set the LRO Info Type. */
   lro_info->type = LRO_INFO_TYPE_FR;
   /* Skip the Rserved field. */
   stream_forward (s, 2);
   read += 2;
   /* Get the L3PID Value. */
   lro_info->lro_info_fr_t.l3pid = stream_getw(s);
   /* Get the byte containing DLCI Length + Min DLCI */
   word = stream_getl (s);
   read += 4;
   /* Get the DLCI Len. */
   if (word & 0x00800000)
   {
      lro_info->lro_info_fr_t.dlci_len = 10;
      /* Extract the Min. DLCI Value. */
      lro_info->lro_info_fr_t.dlci_min = word & 0x000003FF;
      /* Extract the Max, DLCI Value. */
      lro_info->lro_info_fr_t.dlci_max = stream_getl (s) & 0x000003FF;
      read += 4;
   }
   else
   { 
      lro_info->lro_info_fr_t.dlci_len = 23;
      /* Extract the Min. DLCI Value. */ 
      lro_info->lro_info_fr_t.dlci_min = word & 0x007FFFFF;
      /* Extract the Max. DLCI Value. */
      lro_info->lro_info_fr_t.dlci_max = stream_getl (s) & 0x007FFFFF;
      read += 4;
   }

   /* Check if we have read more than the size.*/
   if (read > size)
      return -1;
   /* Check if we have some bytes padded in the object.*/
   else if (read < size)
      stream_forward (s, size - read);
      
   return size;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_lro
 * Input    : s = Pointer to object of type struct stream.
 *          : size = The size of the object.
 *          : lro_info = Pointer to object of type struct rsvp_lro_info.
 * Output   : Returns number of bytes decoded from the stream.
 * Synopsis : This function decodes LRO object with FR Range from the input
 *            stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_decode_lro (struct stream        *s,
                     u_char	          ctype,
                     u_int16_t	          size,
                     struct rsvp_lro_info *lro_info)
{
   int read = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !lro_info)
      return -1;
   
   switch (ctype)
   {
      case RSVP_CLASS_LRO_CTYPE_GEN:
         read = rsvp_obj_decode_lro_gen (s, size, lro_info);
         break;

      case RSVP_CLASS_LRO_CTYPE_ATM:
         read = rsvp_obj_decode_lro_atm (s, size, lro_info);
         break;
   
      case RSVP_CLASS_LRO_CTYPE_FR :
         read = rsvp_obj_decode_lro_fr (s, size, lro_info);
         break;
      /* If Ctypes is invalid.*/
      default :
         read = -1;
   }
   
   return read;
} 
                    
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_label_gen
 * Input    : s = Pointer to object of type struct stream.
 *          : size = Size of the object to be decoded.
 *          : label_info = Pointer to u_int32_t to return the label.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This internal function decodes a label object and returns the 
 *            label.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_label_gen (struct stream          *s,
                           u_int16_t 	          size,
                           struct rsvp_label_info *label_info)
{
   int read = 0;
  
   /* Sanity Check. */
   if (!s || !label_info)
      return read;

   /* Extract the Label Info. */
   label_info->label = stream_getl (s);
   read += 4;

   /* Check in case we have read longer then size.*/
   if (read > size)
      return -1;
   /* We don't expect padding in label objects, but still make check*/
   else if (read < size)
      stream_forward (s, size - read);

   return size;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_label
 * Input    : s = Pointer to object of type struct stream.
 *          : ctype = The C-Type of the label. 
 *          : size = Size of the object to be decoded.
 *          : label_info = Pointer to u_int32_t to return the label.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a label object and returns the label.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_label (struct stream          *s,
                       u_char		      ctype,
                       u_int16_t	      size,
                       struct rsvp_label_info *label_info)		
{
   int read = 0;

   /* Sanity Check for input arguments.*/
   if (!s || !label_info)
      return -1;

   /* Although we have only one label C-type still we use switch case so that
      in future same paradigm can be extended with new label C-types.*/
   switch (ctype)
   {
      case RSVP_CLASS_LABEL_CTYPE_GEN:
         read = rsvp_obj_decode_label_gen (s, size, label_info);
         break;
     
      default:
         read = -1;
   }
   return read;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_ero_subobj_hdr
 * Input    : s = Pointer to object of type struct stream.
 *          : loose = Pointer to u_char to return the loose flag.
 *          : type = Pointer to u_char to hold the type of subobject.
 *          : length = Pointer to u_char to hold the length of subobject.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes the subobject header of ERO Object.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_ero_subobj_hdr (struct stream *s,
                                u_char *loose,
                                u_char *type,
                                u_char *length)
{
   struct rsvp_obj_ero_subobj_hdr *hdr;
   /*Sanity Check. */
   if (!s || !loose || !type || !length)
      return 0;
   hdr = (struct rsvp_obj_ero_subobj_hdr *)STREAM_PNT (s);
   /* Check the L flag. */
   if (hdr->l)
      *loose = 1;
   else
      *loose = 0;
   *type = hdr->type;
   *length = hdr->length;
   stream_forward (s, RSVP_ERO_SUBOBJ_HDR_LEN); 

   return RSVP_ERO_SUBOBJ_HDR_LEN; 
}        

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_ero_subobj_ipv4_prefix
 * Input    : s = Pointer to the object of type struct stream.
 *          : size = Size of the ERO Object of type IPV4_Prefix.
 *          : ero_info = Pointer to the object of type struct rsvp_ero_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a ERO Subobject of type IPV4 PREFIX and
 *            fill the relevant info in struct rsvp_ero_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_ero_subobj_ipv4_prefix (struct stream        *s,
                                        u_int16_t            size,
                                        struct rsvp_ero_info *ero_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !ero_info)
      return read;
   /* Set the ero info type. */
   ero_info->type = ERO_INFO_TYPE_IPV4_PREFIX;
   /* Get the IPV4 Prefix */
   ero_info->ero_info_ipv4_prefix_t.addr.s_addr = stream_get_ipv4(s);
   read += 4;
   /* Get the Prefix Length. */
   ero_info->ero_info_ipv4_prefix_t.prefix_len = stream_getc (s);
   read++;
   /* Skip the Reserved byte. */
   stream_forward (s, 1);
   read++;

   /* Check if we have read longer then its size.*/
   if (read > size)
      return -1;
   /* Check if anything is padded in between. However we don't expect that.*/
   else if (read < size)
      stream_forward (s, size - read);
   
   return size;
} 

#ifdef HAVE_IPV6
/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_ero_subobj_ipv6_prefix
 * Input    : s = Pointer to object of type struct stream.
 *          : size = Size of the object to be decoded.
 *          : ero_info = Pointer to object of type struct rsvp_ero_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a ERO subobject of type IPV6_PREFIX
 *            and fills the relevant info in ero_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_ero_subobj_ipv6_prefix (struct stream 	      *s,
                                        u_int16_t     	      size,
                                        struct rsvp_ero_info *ero_info)
{
   int read = 0;

   return read;
}
#endif

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_ero_subobj_as
 * Input    : s = Pointer to object of type struct stream.
 *          : size = Size of the object.
 *          : ero_info = Pointer to struct ero_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decoded a ERO subobject of type AS and fills the
 *            relevant info in ero_info.
 * Callers  : None.
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_ero_subobj_as (struct stream 	     *s,
                               u_int16_t 	     size,		
                               struct rsvp_ero_info *ero_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !ero_info)
      return read;
   /* Set the ERO Info Type. */
   ero_info->type = ERO_INFO_TYPE_AS;
   /* Extract the AS Number. */
   ero_info->ero_info_as_t.as_num = stream_getw (s);
   read += 2;

   /* Check if we have read longer then the size.*/
   if (read > size)
      return -1;
   /* Check if we have some padding in the object.*/
   else if (read < size)
      stream_forward (s, size - read);

   return size;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_ero
 * Input    : s = Pointer to object of type struct stream.
 *          : size = Size of the object.
 *          : ero_info_list = Pointer to struct rsvp_ero_info_list.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decoded a ERO subobject and fills the
 *            relevant info in ero_info.
 * Callers  : None.
 *---------------------------------------------------------------------------*/
int 
rsvp_obj_decode_ero (struct stream        	*s,
                     u_char              	ctype,
                     u_int16_t	          	size,
                     struct rsvp_ero_info_list *ero_info_list)
{
   int read = 0;
   u_char l = 0;
   u_char subobj_type;
   u_char subobj_len;
   struct rsvp_ero_info *ero_info;

   /* Sanity check for input arguments.*/
   if (!s || !ero_info)
      return -1;

   /* Check ctype.*/
   if (ctype != RSVP_CLASS_ERO_CTYPE_GEN)
      return -1;
   ero_info = ero_info_list->ero_info;

   /* Decode all the ERO Subobjects.*/
   while (size)
   {
      /* Decode the ERO Subobject Header First.*/
      read = rsvp_obj_decode_ero_subobj_hdr (s, &l, &subobj_type, &subobj_len);
      size -= read;
      /* Set the loose bit.*/
      ero_info->l = l;

      switch (subobj_type)
      {
         case RSVP_CLASS_ERO_SUBOBJ_IPV4_PREFIX:
            read = rsvp_obj_decode_ero_subobj_ipv4_prefix (s, subobj_len,
                                                           ero_info);
            size -= read;
            break;
#ifdef HAVE_IPV6
	 case RSVP_CLASS_ERO_SUBOBJ_IPV6_PREFIX:
	    read = rsvp_obj_decode_ero_subobj_ipv6_prefix (s, subobj_len,
                                                           ero_info);
            size -= read;
            break;
#endif /* HAVE_IPV6 */
	 case RSVP_CLASS_ERO_SUBOBJ_AS:
	    read = rsvp_obj_decode_ero_subobj_as (s, subobj_len, ero_info);
            size -= read;
            break;

         default :
            return -1;
      } /* End of switch. */   
      ero_info_list->num++;
      ero_info++;
   }/* End of while.*/
   return read;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_rro_subobj_hdr
 * Input    : s = Pointer to object of type struct stream.
 *          : type = Pointer to u_char to return the type.
 *          : length = Pointer to u_char to return the length.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes the RRO subobject header.
 * Callers  ; (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_rro_subobj_hdr (struct stream *s,
                                u_char *type,
                                u_char *length)
{
   struct rsvp_obj_rro_subobj_hdr *hdr;
   int read = 0;

   /* Sanity Check. */
   if (!s)
      return read;

   /* Get the Map of the Header. */
   hdr = (struct rsvp_obj_rro_subobj_hdr *) STREAM_PNT (s);
   *type = hdr->type;
   *length = hdr->length;
   stream_forward (s, RSVP_ERO_SUBOBJ_HDR_LEN);
   read += RSVP_ERO_SUBOBJ_HDR_LEN;

   return read;
}         

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_rro_subobj_ipv4_addr
 * Input    : s        = Pointer to object of type struct stream.
 *          : size     = The size of the subobject as per subobj header.
 *          : rro_info =  Pointer to struct rsvp_rro_info
 * Output   : Retirns the number of bytes decoded.
 * Synopsis : This function decodes a RRO Subobject of type IPV4 Addrss and 
 *            fills the info in rro_info.
 * Callers  : (TBD)
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_rro_subobj_ipv4_addr (struct stream        *s,
                                      u_int16_t            size,
                                      struct rsvp_rro_info *rro_info)
{
   int read = 0;
   u_char flags;
   /* Sanity Check. */
   if (!s || !rro_info)
      return read;
   /* Set the Type in the info. */
   rro_info->type = RRO_INFO_IPV4_ADDR;
   /* Get the IPV4 Address. */
   rro_info->rro_info_ipv4_addr_t.addr.s_addr = stream_get_ipv4(s); 
   read += 4;
   /* Get the Prefix Length. */
   rro_info->rro_info_ipv4_addr_t.prefix_len = stream_getc (s);
   read++;
   /* Get the flags field. */
   flags = stream_getc (s);
   read++;
   /* Check the flags. */
   if (flags & 0x01)
      rro_info->rro_info_ipv4_addr_t.local_prot_avail = 1;
   if (flags & 0x02)
      rro_info->rro_info_ipv4_addr_t.local_prot_inuse = 1;

   /* Check if we have overshot the size.*/
   if (read > size)
      return -1;
   /* Else check if we have received the object with padding.*/
   else if (read < size)
      stream_forward (s, size - read);

   return read;
}

#ifdef HAVE_IPV6
/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_rro_subobj_ipv6_addr 
 * Input    : s        = Pointer to object of type struct stream.
 *          : size     = Size of the sub-object.
 *          : rro_infp = Pointer to the object of type struct rro_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function returns the number of bytes decoded.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_rro_subobj_ipv6_addr (struct stream 	    *s,
                                      u_int16_t	    size,
                                      struct rsvp_rro_info *rro_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !rro_info)
      return read;

   /* Set the RRO Info Type. */
   rro_info->type = RRO_INFO_IPV6_ADDR;
  
   // (TBD)
   return read;
}

#endif /*HAVE_IPV6 */

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_rro_subobj_label 
 * Input    : s        = Pointer to object of type struct stream.
 *          : size     = The size of the subobject as per header.
 *          : rro_info = Pointer to the object of type struct rsvp_rro_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a RRO object of type LABEL.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_rro_subobj_label (struct stream 	*s,
                                  u_int16_t		size,
                                  struct rsvp_rro_info *rro_info)
{
   int read = 0;
   u_char flags;
   /* Sanity Check. */
   if (!s || !rro_info)
      return read;

   rro_info->type = RRO_INFO_LABEL;
   /* Extract the flags. */
   flags = stream_getc (s);
   read ++;
   /* Check if its a global label. */
   if (flags & 0x01)
      rro_info->rro_info_label_t.global_label = 1;
   /* Check that the C-type of the label is 1 .*/
   if (!(stream_getc (s) & RSVP_CLASS_LABEL_CTYPE_GEN))
      return 0;
   read++;
   /* Get the Label Value. */
   rro_info->rro_info_label_t.label = stream_getl (s);
   read += 4;

   /* Check if we have overshot the size.*/
   if (read > size)
      return -1;
   /* Check if we have received padding in the header.*/
   else if (read < size)
      stream_forward (s, size - read);

   return read;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_rro
 * Input    : s 	= Pointer to object of type struct stream.
 *          : ctype	= The C-Type of the ERO Object.
 *          : size 	= The size of the object from the object header.
 *          : rro_info_list= Pointer to struct rsvp_rro_info_list 
 * Output   : Returns the number of bytes decodes for the object. If faliled
 *          : return -1.
 * Synopsis : This function decodes the RRO Object from the stream and fills
 *            the relevant info into struct rsvp_rro_obj.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int 
rsvp_obj_decode_rro (struct stream 		*s,
                     u_char			ctype,
                     u_int16_t		        size,
                     struct rsvp_rro_info_list	*rro_info_list)
{
   int read = 0;
   u_char subobj_type;
   u_char subobj_len;
   struct rsvp_rro_info *rro_info;
   

   /* Sanity Check on input arguments.*/
   if (!s || !rro_info_list)
      return read;

   rro_info = rro_info_list->rro_info;

   /* There is only one C-type for RRO so check if Ctype is correct.*/
   if (ctype != RSVP_CLASS_RRO_CTYPE_GEN)
      return -1;
   /* Decode the RRO Subobject Header.*/
   read = rsvp_obj_decode_rro_subobj_hdr (s, &subobj_type, &subobj_len); 
 
   /* Check if header has error then we return from here.*/
   if (read <= 0)
      return -1;   

   while (size)
   {   
      /* Process based on the RRO Object Type. (TBD)*/
      switch (subobj_type)
      {
         case RSVP_CLASS_RRO_SUBOBJ_IPV4_ADDR:
            read += rsvp_obj_decode_rro_subobj_ipv4_addr (s, size, rro_info);
            break;

#ifdef HAVE_IPV6
         case RSVP_CLASS_RRO_SUBOBJ_IPV6_ADDR:
            read += rsvp_obj_decode_rro_subobj_ipv6_addr (s, size, rro_info);
            break;
#endif /*HAVE_IPV6 */

         case RSVP_CLASS_RRO_SUBOBJ_LABEL:
            read += rsvp_obj_decode_rro_subobj_label (s, size, rro_info);
            break;
         default:
            return -1;
      }

      size -= read;
      rro_info++;
      rro_info_list->num++;
   } 
   
   return read; 
}
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_flt_spec_lsp_tun_ipv4
 * Input    : s 	    = Pointer to object of type struct stream.
 *          : size          = The size of bthe object to be decoded.
 *          : flt_spec_info = Pointer to object of type struct 
 *                            rsvp_flt_spec_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodeds a Filter Spec Object of type LSP_TUN_IPV4.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_flt_spec_lsp_tun_ipv4(struct stream 		*s,
                                      u_int16_t			size,
                                      struct rsvp_flt_spec_info *flt_spec_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !flt_spec_info)
     return read;
   /* Set the Filter Spec Info Type. */
   flt_spec_info->type = FLT_SPEC_INFO_LSP_TUN_IPV4;
   /* Get the the sender address. */
   flt_spec_info->flt_spec_info_lsp_tun_ipv4_t.sender_addr.s_addr = 
                                                        stream_getl(s);
   read += 4;
   /* Check that reserved field is zero. */
   if (stream_getw (s))
      return 0;
   read += 2;

   flt_spec_info->flt_spec_info_lsp_tun_ipv4_t.lsp_id = stream_getw (s);
   read += 2;
   /* If we have read more then the size.*/
   if (read > size)
      return -1;
   /* If we have received padding in this object then skip the padding info.*/
   else if (read < size)
      stream_forward (s, size - read);

   return size;
}

#ifdef HAVE_IPV6
/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_flt_spec_lsp_tun_ipv6
 * Input    : s 	    = Pointer to object of type struct stream.
 *          : size          = Size of the object
 *          : flt_spec_info = Pointer to struct rsvp_flt_spec_info
 * Output   : Returns the Number of bytes decoded.
 * Synopsis : This function decodes Filter Spec Object of type LSP_TUN_IPV4.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_flt_spec_lsp_tun_ipv6 (struct stream 		 *s,
                                       u_int16_t		 size,
                                       struct rsvp_flt_spec_info *flt_spec_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !flt_spec_info)
      return read;

   /*Set the Filter Spec info as LSP_TUN_IPV6. */
   flt_spec_info = FLT_SPEC_INFO_LSP_TUN_IPV6;
   // (TBD).

   return read;
}
#endif

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_flt_spec
 * Input    : s		    = Pointer to the object of type struct stream.
 *          : ctype         = The Ctype of the object received from its header.
 *          : size	    = The length of the object.
 *          : flt_spec_info = Pointer to struct rsvp_flt_spec_info.
 * Output   : Returns the number of bytes decoded from the stream.
 * Synopsis : This function decodes FILTER_SPEC object from a stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_decode_flt_spec (struct stream 	    *s,
                          u_char		    ctype,
                          u_int16_t		    size,
                          struct rsvp_flt_spec_info *flt_spec_info)
{
   int read = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !flt_spec_info)
      return read;

   /* Process based on C-type.*/
   switch (ctype)
   {
      case RSVP_CLASS_FLT_SPEC_CTYPE_LSP_TUN_IPV4:
         read += rsvp_obj_decode_flt_spec_lsp_tun_ipv4(s, size, flt_spec_info);
         break;
#ifdef HAVE_IPV6
      case RSVP_CLASS_FLT_SPEC_CTYPE_LSP_TUN_IPV6:
         read += rsvp_obj_decode_flt_spec_lsp_tun_ipv6(s, size, flt_spec_info);
         break;
#endif
      default:
         return -1;
   }

   return read;
}

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_sa_lsp_tun
 * Input    : s       = Pointer to object of type struct stream.
 *          : size    = The size of the object
 *          : sa_info = Pointer to object of type struct rsvp_sa_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes SESSION_ATTRIBUTE object of type 
 *            LSP_TUNNEL.
 * Callers  ; (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_sa_lsp_tun (struct stream 	*s,
                            u_int16_t		size,
                            struct rsvp_sa_info *sa_info)
{
   int read = 0;
   u_char byte;

   /* Sanity Check. */
   if (!s || !sa_info)
      return read;

   /* Set the proper info type. */
   sa_info->type = SA_INFO_LSP_TUN;
   /* Get the set up prioruty info. */
   sa_info->sa_info_lsp_tun_t.setup_prio = stream_getc (s);
   read++;
   /* Get the Hold Priority Info. */
   sa_info->sa_info_lsp_tun_t.hold_prio = stream_getc (s);
   read++;
   /* Get the flags field.*/
   byte = stream_getc (s);
   read++;
   /* Check if local protection desired. */
   if (byte & 0x01)
      sa_info->sa_info_lsp_tun_t.local_prot = 1;
   /* Check if Label Recording is desired. */
   if (byte & 0x02)
      sa_info->sa_info_lsp_tun_t.label_record = 1;
   /* Check if se_style desired. */
   if (byte & 0x04)
      sa_info->sa_info_lsp_tun_t.se_style = 1;
   /* Get the name length field. */
   byte = stream_getc (s);
   read++;
   /* Get the Null Terminated String. */
   stream_get (sa_info->sa_info_lsp_tun_t.sess_name, s, (size_t)(byte + 1));
   read += byte;

   /* Check we have overshot the size while decoding.*/
   if (read > size)
      return -1;
   /* Check if we have received padding in the object.*/
   else if ( read < size)
      stream_forward (s, size - read);
  
   return size;
} 

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_sa_lsp_tun_ra
 * Input    : s       = Pointer to object of type struct stream.
 *          : size    = The size of the object.
 *          : sa_info = Pointer to object of type struct rsvp_sa_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes SESSION_ATTRIBUTE object of type 
 *            LSP_TUNNEL_RA
 * Callers  ; (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_sa_lsp_tun_ra (struct stream 	   *s,
                               u_int16_t      	    size,
                               struct rsvp_sa_info *sa_info)
{
   int read = 0;
   u_char byte;

   /* Sanity Check. */
   if (!s || !sa_info)
      return read;

   /* Set the proper info type. */
   sa_info->type = SA_INFO_LSP_TUN_RA;
   /* Get the Exclude-Any Info. */
   sa_info->sa_info_lsp_tun_ra_t.exclude_any = stream_getl (s);
   read += 4;
   /* Get the Include-Any. */
   sa_info->sa_info_lsp_tun_ra_t.include_any = stream_getl (s);
   read += 4;
   /* Get the include-all info. */
   sa_info->sa_info_lsp_tun_ra_t.include_all = stream_getl (s);
   read += 4;
   /* Get the set up prioruty info. */
   sa_info->sa_info_lsp_tun_ra_t.setup_prio = stream_getc (s);
   read++;
   /* Get the Hold Priority Info. */
   sa_info->sa_info_lsp_tun_ra_t.hold_prio = stream_getc (s);
   read++;
   /* Get the flags field.*/
   byte = stream_getc (s);
   read++;
   /* Check if local protection desired. */
   if (byte & 0x01)
      sa_info->sa_info_lsp_tun_ra_t.local_prot = 1;
   /* Check if Label Recording is desired. */
   if (byte & 0x02)
      sa_info->sa_info_lsp_tun_ra_t.label_record = 1;
   /* Check if se_style desired. */
   if (byte & 0x04)
      sa_info->sa_info_lsp_tun_ra_t.se_style = 1;
   /* Get the name length field. */
   byte = stream_getc (s);
   read++;
   /* Get the Null Terminated String. */
   stream_get (sa_info->sa_info_lsp_tun_ra_t.sess_name, s, (size_t)(byte + 1));
   read += byte;

   /* Check if we have overshot than the eize of the object.*/
   if (read > size)
      return -1;
   /* Check if we have received padding in the object.*/
   else if ( read < size)
      stream_forward (s, size - read);
  
   return size;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_sa
 * Input    : s 	= Pointer to object of type struct stream.
 *          : ctype 	= The C-type of the SESSION_ATTRIBUTE Object.
 *          : size	= The size of the object from its header.
 *          : sa_info	= Pointer to object of type struct rsvp_sa_info
 * Output   : Returns the number of bytes read from the stream. If error 
 *            return1 -1.
 * Synopsis : This function decodes an object of type SESSION_ATTRBUTE from
 *            the sream and fills the decoded info into sa_info.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_decode_sa (struct stream 	*s,
                  u_char		ctype,
 		  u_int16_t		size,
                  struct rsvp_sa_info	*sa_info)
{
   int read = 0;
   
   /* Sanity Check on input arguments.*/
   if (!s || !sa_info)
      return read;

   /* Process as per the Ctype.*/
   switch (ctype)
   {
      case RSVP_CLASS_SA_CTYPE_LSP_TUN:
         read += rsvp_obj_decode_sa_lsp_tun (s, size, sa_info);
         break;

      case RSVP_CLASS_SA_CTYPE_LSP_TUN_RA:
         read += rsvp_obj_decode_sa_lsp_tun_ra (s, size, sa_info);
         break;

      default :
         return -1;
   }
   
   return read;
}

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_hop_ipv4
 * Input    : s        = Pointer to object of type struct stream.
 *          : size     = Size of the object.
 *          : hop_info = Pointer to object of type struct rsvp_hop_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a RSVP_HOP Object of type IPV4 and fills
 *            the hop_info.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_hop_ipv4 (struct stream 	*s,
                          u_int16_t             size,
                          struct rsvp_hop_info  *hop_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !hop_info)
      return read;

   /* Set the Proper Hop Info.*/
   hop_info->type = HOP_INFO_IPV4;
   /* Extract the IPV4 Address Field. */
   hop_info->hop_info_ipv4_t.addr.s_addr = stream_get_ipv4 (s);
   read += 4;
   /* Extract the logical interface handle info. */
   hop_info->hop_info_ipv4_t.lih = stream_getl (s);
   read += 4;

   /* Check if we have overshot the size specified.*/
   if (read > size)
      return -1;
   /* Check if we have received padding from the object.*/
   else if (read < size)
      stream_forward (s, size - read);

   return size;
} 

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_hop_ipv6 
 * Input    : s	       = Pointer to object of type struct stream.
 *          : size     = Size of the object.
 *          : hop_info = Pointer to object of type struct rsvp_hop_info.
 * Output   : Returns number of bytes decoded.
 * Synopsis : This function decodes a RSVP_HOP object of type IPV6.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_hop_ipv6 (struct stream 	*s,
                          u_int16_t		size,
                          struct rsvp_hop_info 	*hop_info)
{
   int read = 0;

   /* Sanity Check. */
   if (!s || !hop_info)
      return read;

   return read;
}
#endif /* HAVE_IPV6 */ 

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_hop
 * Input    : s		= Pointer to object of type struct stream.
 *          : ctype 	= The C-Type of the HOP Object.
 *          : size	= The size of the object (decoder from its header).
 *          : hop_info	= Pointer to object of type struct rsvp_hop_info.
 * Output   : It returns the number of bytes decoded from the object. If error
 *          : in decoding it returns -1.
 * Synopsis : This function decodes a RSVP HOP object from the input stream
 *          : and fills the extracted info in to hop_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_hop (struct stream 	  *s,
                     u_char		  ctype,
                     u_int16_t		  size,
                     struct rsvp_hop_info *hop_info)
{
   int read = 0;

   switch (ctype)
   {
      case RSVP_CLASS_HOP_CTYPE_IPV4:
         read = rsvp_obj_decode_hop_ipv4 (s, size, hop_info);
         break;
#ifdef HAVE_IPV6
      case RSVP_CLASS_HOP_CTYPE_IPV6:
         read = rsvp_obj_decode_hop_ipv6 (s, size, hop_info);
         break;
#endif /* HAVE_IPV6 */

      default : 
         read = -1;
   }

   return read;      
}

/*-----------------------------------------------------------------------------
 * Function ; rsvp_obj_decode_timeval
 * Input    : s = Pointer to object of type struct stream.
 *          : timeval_info = Pointer to struct rsvp_timeval_info.
 * Output   : Returns number of bytes decoded.
 * Synopsis :
 * Callers  :
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_timeval (struct stream            *s,
                         u_char		          ctype,
                         u_int16_t		  size,
                         struct rsvp_timeval_info *timeval_info)
{
   int read = 0;
  
   /* We still use thsi small switch case block so that in future if we 
      develop any new CTYPES for TIME_VALUES this can be extended with another
      case. */
   switch (ctype)
   { 
      case RSVP_CLASS_TIMEVAL_CTYPE_GEN :

         timeval_info->refresh_period = stream_getl (s);
         read += 4;
         break;
     
      default :
         read = -1;
   }

   return read;   
}	

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_err_spec_ipv4
 * Input    : s			= Pointer to object of type struct stream.
 *	    : size		= The size of the object (from its header).
 *          : err_spec_info	= Pointer to struct rsvp_err_spec_info.
 * Output   : Returns the number of bytes decoded from the stream. If error in
 *          : decode then it returns -1.
 * Synopsis : This function decodes an object of type ERROR_SPEC and fills
 *          : the extracted info into err_spec_info.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
static int
rsvp_obj_decode_err_spec_ipv4 (struct stream		 *s,
                               u_int16_t		 size,
                               struct rsvp_err_spec_info *err_spec_info)
{
   int read = 0;
   u_char flags;
   struct rsvp_err_spec_ipv4_info *err_spec_ipv4_info;

   /* Sanity Check. */
   if (!s || !err_spec_info)
      return read;

   err_spec_info->type = ERR_SPEC_INFO_IPV4;
   err_spec_ipv4_info = &err_spec_info->err_spec_ipv4_info_t;
   /* Extract the Node's IPV4 Address. */
   err_spec_ipv4_info->addr.s_addr = stream_get_ipv4 (s);
   read += 4;
   /* Extract the Flags field. */
   flags = stream_getc (s);
   read++;
   /* Check for InPlace Flag.*/
   if (flags & 0x01)
      err_spec_ipv4_info->in_place_flag = 1;
   /* Check for NotGuilty Flag. */
   if (flags & 0x02)
      err_spec_ipv4_info->not_guilty_flag = 1;
   /* Extract the Error Code. */
   err_spec_ipv4_info->err_code = stream_getc (s);
   read++;
   /* Extract the Error Value. */
   err_spec_ipv4_info->err_value = stream_getw (s);
   read += 2;

   /* Check if we have overshot in decoding.*/
   if ( read > size)
      return -1;
   /* Check if we have received padding from the object.*/
   else if ( read < size)
      stream_forward (s, size - read);

   return size;
}

#ifdef HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_err_spec_ipv6
 * Input    : s 		= Pointer to the object of type struct stream.
 *          : size 		= The size of the object to be decoded.
 *          : err_spec_info	= Pointer to struct rsvp_err_spec_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes ERROR_SPEC Object of type IPV6.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_err_spec_ipv6 (struct stream 		 *s,
                               u_int16_t		 size,
                               struct rsvp_err_spec_info *err_spec_info)
{
   int read = 0;
   u_char flags;

   /* Sanity Check on input arguments.*/
   if (!s || !err_spec_info)
      return -1;

   // (TBD);
   return read;
}
#endif /* HAVE_IPV6. */

/*---------------------------------------------------------------------------
 * Function : rsvp_obj_decode_err_spec
 * Input    : s			= Pointer to object of type struct stream.
 * 	    : ctype		= C-Type of the ERROR_SPEC Object.
 *          : size 		= The object size as received from the header.
 *          : err_spec_info	= Pointer to struct rsvp_err_spec_info.
 * Output   : Returns the number of bytes decoded from the stream. On error
 *          : returns -1.
 * Synopsis : This function decodes object of type ERROR_SPEC and fills 
 *          : the extracted info into err_spec_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_err_spec (struct stream 	    *s,
                          u_char		    ctype,
                          u_int16_t		    size,
                          struct rsvp_err_spec_info *err_spec_info)
{
   int read = 0;

   /* Sanity Check on Input arguments.*/
   if (!s || !err_spec_info)
      return -1;
   /* Process based on C-Type.*/
   switch (ctype)
   {
      case RSVP_CLASS_ERR_SPEC_CTYPE_IPV4:
         read += rsvp_obj_decode_err_spec_ipv4 (s, size, err_spec_info);
         break;

#ifdef HAVE_IPV6
      case RSVP_CLASS_ERR_SPEC_CTYPE_IPV6:			
	 read += rsvp_obj_decode_err_spec_ipv6 (s, size, err_spec_info);
	 break;

#endif /* HAVE_IPV6 */
      default:
         return -1;
   }
 
   return read;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_style 
 * Input    : s 	 = Pointer to object of type struct stream.
 *          : ctype 	 = C-Type of the style object.
 *          : size 	 = The size of the object.
 *          : style_info = Pointer to object of type rsvp_style_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a RSVP Style Object.
 * Callers  : None.
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_style (struct stream 	      *s,
		       u_char		      ctype,
		       u_int16_t	      size,	
                       struct rsvp_style_info *style_info)
{
   int read = 0;
   u_int32_t value;

   /* Sanity Check.*/
   if (!s || !style_info)
      return read;
   /* Get the 32-bit value in style object */
   value = stream_getl (s);   
   read += 4;
   /* get the 5 LSBs.*/
   value &= 0x0000001F;

   return read;
};

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_frr
 * Input    : s         = Pointer to object of type struct stream.
 *          : ctype 	= C-Type of the FRR Object.
 *          : size	= Size of the object.
 *          : frr_info  = Pointer to object of type struct rsvp_frr_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a FRR Object (RFC 4090) from stream s and 
 *            fills the relevant parameters in frr_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
int
rsvp_obj_decode_frr (struct stream 	  *s,
		     u_char		  ctype,
		     u_int16_t		  size,
                     struct rsvp_frr_info *frr_info)
{
   int read = 0;
 
   /* Sanity Check.*/
   if (!s || !frr_info)
      return read;
  
   /* Decode the Setup Prio Field.*/
   frr_info->setup_prio = stream_getc (s);
   read++;
   /* Decode the Hold Prio Field.*/
   frr_info->hold_prio = stream_getc (s);
   read++;
   /* Decode the hop limit field.*/
   frr_info->hop_limit = stream_getc (s);
   read++;
   /* Decode the flags field.*/
   frr_info->backup_type = stream_getc (s);
   read++;
   /* Decode the bandwidth field.*/
   frr_info->bandwidth = stream_getl (s);
   read += 4;
   /* Decode the Include-any field.*/
   frr_info->include_any = stream_getl (s);
   read += 4;
   /* Decode Include-all Field.*/
   frr_info->include_all = stream_getl (s);
   read += 4;

   /* Check if we have overshot the size.*/
   if ( read > size)
      return -1;
   /* Check if we have received padding from the object.*/
   else if ( read < size)
      stream_forward (s, size - read);

   return size;
};

/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_detour_ipv4
 * Input    : s		  = Pointer to object of type struct stream.
 *          : size 	  = The Size of the object.
 *          : detour_info = Pointer to struct rsvp_detour_info.
 * Output   : Returns the number of bytes decoded.
 * Synopsis : This function decodes a DETOUR Object from the stream and fills
 *            up the detour_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_detour_ipv4 (struct stream 	     *s,
                             u_int16_t               size,
                             struct rsvp_detour_info *detour_info)
{
   int read = 0;
   int num;
   struct rsvp_detour_element_info_ipv4 *element_info;

   /* Sanity Check on input arguments.*/
   if (!s || !detour_info)
      return read;
   /* Calculate the number of PLR ID-Avoid_Node_ID Pair from size.*/
   num = detour_info->num = size/8;
   /* Set the Type as IPV4.*/ 
   detour_info->type = RSVP_DETOUR_INFO_IPV4;
   /* Create a list for storing the PLR_ID-Avoid_Node_ID Pair.*/
   if ((detour_info->detour_element_list = list_new ()) == NULL)
      return read;
 
   /* Extract all the PLR_ID/Avoid_Node_ID Pairs */
   while (num)
   {
      element_info = XCALLOC (MTYPE_RSVP_DETOUR_ELEMENT_INFO_IPV4, 
                              sizeof (struct rsvp_detour_element_info_ipv4));
      element_info->plr_id.s_addr = stream_get_ipv4 (s); 
      element_info->avoid_node_id.s_addr = stream_get_ipv4 (s);
      read += 8;
      listnode_add (detour_info->detour_element_list, element_info);
      num--;
   }
   return read;  
}

#ifdef  HAVE_IPV6
/*----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_detour_ipv6
 * Input    : s           = Pointer to struct stream.
 *          : size	  = The size of the object.
 *          : detour_info = Pointer to struct rsvp_detour_info.
 * Output   : It returns the number of bytes decoded.
 * Synopsis : This function decodes DETOUR Object of type IPV6 and fills 
 *            the relevant info in detour_info.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
static int
rsvp_obj_decode_detour_ipv6 (struct stream 	     *s,
                             u_int16_t		     size,
                             struct rsvp_detour_info *detour_info)
{
   int read = 0;

   /* Sanity Check on input arguments.*/
   if (!s || !detour_info)
      return read;

   return read;
}
#endif /* HAVE_IPV6 */

/*-----------------------------------------------------------------------------
 * Function : rsvp_obj_decode_detour 
 * Input    : s 	  = Pointer to object of type struct stream.
 *          : ctype	  = C-Type of the DETOUR object.
 *	    : size	  = The Size of the object as received from header.
 *          : detour_info = Pointer to struct rsvp_detour_info.
 * Output   : Returns the number of bytes decoded from the stream. If error
 *          : in decoding it returns -1.
 * Synopsis : This function decodes DETOUR Object from stream.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
int
rsvp_obj_decode_detour (struct stream 		*s,
                        u_char			ctype,
                        u_int16_t		size,
                        struct rsvp_detour_info	*detour_info)
{
   int read;
   
   /* Sanity Check on input arguments.*/
   if (!s || !detour_info)
      return -1;

   /* Process based on the C-Type.*/
   switch (ctype)
   {
      case RSVP_CLASS_DETOUR_CTYPE_IPV4:
         read += rsvp_obj_decode_detour_ipv4 (s, size, detour_info);
         break;
#ifdef HAVE_IPV6 
      case RSVP_CLASS_DETOUR_CTYPE_IPV6:
         read += rsvp_obj_decode_detour_ipv6 (s, size, detour_info);
         break;
#endif /* HAVE_IPV6 */
      default :
         return -1;
   }
   return read;
}
