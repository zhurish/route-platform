/* RSVP-TE Packet Processing Procedures and Routines.
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
#include "thread.h"
#include "vty.h"
#include "prefix.h"
#include "avl.h"
#include "lsp.h"
#include "stream.h"
#include "log.h"
#include "if.h"
#include "checksum.h"

#include "rsvpd/rsvpd.h"
#include "rsvpd/rsvp_if.h"
#include "rsvpd/rsvp_obj.h"
#include "rsvpd/rsvp_packet.h"


/*---------------------------------------------------------------------------
 * RSVP Packet Module: This module receives the RSVP Packets when notified
 * by network (rsvp_network) and sends RSVP packets to network. The routines
 * decode/encode the RSVP Messages in the formats defined in RFC spec. On 
 * receiving path it does all error/sanity checks required by specific 
 * messages, decodes the objects and notify the message info to Softstate
 * module (rsvp_softstate). In sedning path, it receives the message info
 * from Softstate Module, encodes in the specific formats and sends to the 
 * network. 
 *------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Function : rsvp_packet_output_forward 
 * Input    : s = Pointer to object of type struct stream.
 *          : size = integer value by which to forward the packet.
 * Output   : None
 * Synopsis : This is a utility function that is used to forward s->putp by 
 *            value of size.
 * Callers  : (TBD)
 *---------------------------------------------------------------------------*/
void rsvp_packet_output_forward (struct stream *s,
                            size_t size)
{
   //s->putp += size;
   stream_forward_endp(s, size);
   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_packet_new 
 * Input    : size = Integer value that indicated the size of a packet.
 * Output   : Returns pointer to struct rsvp_packet if created successfully,
 *            else returns NULL.
 * Synopsis : This is a utility function that creates a new rsvp_packet.
 * Callers  : (TBD)..
 *--------------------------------------------------------------------------*/
struct rsvp_packet *
rsvp_packet_new (size_t size)
{
   struct rsvp_packet *rsvp_packet;

   rsvp_packet = XCALLOC (MTYPE_RSVP_PACKET, sizeof (struct rsvp_packet));
   rsvp_packet->s = stream_new (size);

   return rsvp_packet;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_free
 * Input    : rsvp_packet = Pointer to object of type struct rsvp_packet
 * Output   : None
 * Synopsis : This is a utility function that frees a rsvp_packet.
 * Callers  : (TBD).
 *------------------------------------------------------------------------*/
void
rsvp_packet_free (struct rsvp_packet *rsvp_packet)
{
   /* If stream exists free the stream.*/
   if (rsvp_packet->s)
      stream_free (rsvp_packet->s);

   /* Free the rsvp_packet now. */
   XFREE (MTYPE_RSVP_PACKET, rsvp_packet);

   rsvp_packet = NULL;
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_fifo_new
 * Input    : None
 * Output   : Returns pointer to struct rsvp_packet_fifo if successful, else
 *            returns NULL
 * Synopsis : This function allocates a FIFO of rsvp_packets.
 * Callers  : (TBD)..
 *------------------------------------------------------------------------*/
struct rsvp_packet_fifo *
rsvp_packet_fifo_new (void)
{
   struct rsvp_packet_fifo *rsvp_packet_fifo;

   rsvp_packet_fifo = XCALLOC (MTYPE_RSVP_PACKET_FIFO, 
                               sizeof (struct rsvp_packet_fifo));
   return rsvp_packet_fifo;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_fifo_push 
 * Input    : rsvp_packet_fifo = Pointer to object of type struct
 *          :                    rsvp_packet_fifo 
 *          : rsvp_packet      = Pointer to object of type struct rsvp_packet.
 * Output   : None
 * Synopsis ; This utility function adds a new rsvp_packet into fifo.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
void
rsvp_packet_fifo_push (struct rsvp_packet_fifo *rsvp_packet_fifo,
                       struct rsvp_packet *rsvp_packet)
{
   if (rsvp_packet_fifo->tail)
      rsvp_packet_fifo->tail->next = rsvp_packet;
   else
      rsvp_packet_fifo->head = rsvp_packet;

   rsvp_packet_fifo->tail = rsvp_packet;
   
   rsvp_packet_fifo->count++;
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_fifo_pop
 * Input    : rsvp_packet_fifo = Pointer to object of type strucr
 *          :                    rsvp_packet_fifo.
 * Output   : Pointer to object of type struct rsvp_packet
 * Synopsis : This utility function deletes a packet from FIFO.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/
struct rsvp_packet *
rsvp_packet_fifo_pop (struct rsvp_packet_fifo *rsvp_packet_fifo)
{
   struct rsvp_packet *rsvp_packet;

   rsvp_packet = rsvp_packet_fifo->head;

   if (rsvp_packet)
   {
      rsvp_packet_fifo->head = rsvp_packet->next;

      if (rsvp_packet_fifo->head == NULL)
         rsvp_packet_fifo->tail = NULL;

      rsvp_packet_fifo->count--;
   }
   return rsvp_packet;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_packet_fifo_head 
 * Input    : rsvp_packet_fifo = Pointer to object of type struct 
 *          :                    rsvp_packet_fifo.
 * Output   : Pointer to the rsvp_packet
 * Synopsis : This utility gunctions returns the rsvp_packet at the head of 
 *            a rsvp_packet_fifo.
 * Callers  : (TBD).
 *---------------------------------------------------------------------------*/
struct rsvp_packet *
rsvp_packet_fifo_head (struct rsvp_packet_fifo *rsvp_packet_fifo)
{
   return rsvp_packet_fifo->head;
}
/*----------------------------------------------------------------------------
 * Function : rsvp_packet_fifo_flush
 * Input    : rsvp_packet_fifo = Pointer to struct rsvp_packet_fifo.
 * Output   : None
 * Synopsis : This utility function deletes all rsvp_packets from a 
 *            rsvp_packet_fifo.
 * Callers  : TBD.
 *--------------------------------------------------------------------------*/
void
rsvp_packet_fifo_flush (struct rsvp_packet_fifo *rsvp_packet_fifo)
{
   struct rsvp_packet *rsvp_packet;
   struct rsvp_packet *next;

   for (rsvp_packet = rsvp_packet_fifo->head; rsvp_packet; rsvp_packet = next)
   {
      next = rsvp_packet->next;
      rsvp_packet_free (rsvp_packet);
   }

   rsvp_packet_fifo->head = rsvp_packet_fifo->tail = NULL;
   rsvp_packet_fifo->count = 0;

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_packet_fifo_free 
 * Input    : rsvp_packet_fifo = Pointer to object of type 
 *          :                    struct rsvp_packet_fifo.
 * Output   : None
 * Synopsis : This utility function frees a rsvp_packet_fifo.
 * Callers  : TBD
 *--------------------------------------------------------------------------*/
void
rsvp_packet_fifo_free (struct rsvp_packet_fifo *rsvp_packet_fifo)
{
   rsvp_packet_fifo_flush (rsvp_packet_fifo);
   
   XFREE (MTYPE_RSVP_PACKET_FIFO, rsvp_packet_fifo);

   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_packet_add_to_if
 * Input    : rsvp_if 	  = Pointer to object of type struct rsvp_if
 *          : rsvp_packet = Pointer to object of struct rsvp_packet.
 * Output   : None
 * Synopsis : This function enqueues a rsvp_packet on a rsvp interface.
 * Callers  : (TBD)
 *---------------------------------------------------------------------------*/
void
rsvp_packet_add_to_if (struct rsvp_if *rsvp_if,
                       struct rsvp_packet *rsvp_packet)
{
   rsvp_packet_fifo_push (rsvp_if->obuf, rsvp_packet);
  
   return;
}

/*----------------------------------------------------------------------------
 * Function : rsvp_packet_del_from_if
 * Input    : rsvp_if = Pointer to object of type struct rsvp_if
 * Output   : None
 * Synopsis : This function deletes a rsvp_packet from a rsvp_if
 * Callers  : (TBD)
 *--------------------------------------------------------------------------*/
void
rsvp_packet_del_from_if (struct rsvp_if *rsvp_if)
{
   struct rsvp_packet *rsvp_packet;

   rsvp_packet = rsvp_packet_fifo_pop (rsvp_if->obuf);

   if (rsvp_packet)
      rsvp_packet_free (rsvp_packet);
   
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_stream_copy
 * Input    : new = Pointer to object of type struct stream.
 *          : s   = Pointer to object of type struct stream.
 * Output   : Returns the updated new stream.
 * Synopsis : This utility function copies a existing stream to new one.
 * Callers  : (TBD)
 *-------------------------------------------------------------------------*/
struct stream *
rsvp_stream_copy (struct stream *new, 
                  struct stream *s)
{
   new->endp = s->endp;
   //new->putp = s->putp;
   new->getp = s->getp;

   memcpy (new->data, s->data, stream_get_endp (s));

   return new;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_decode_msg_hdr
 * Input    : s   = Pointer to the object of type struct stream.
 *          : hdr = Pointer to the object of type struct rsvp_msg_hdr.
 * Output   : None
 * Synopsis : This utility fuctiion extracts rsvp_msg_hdr info from the input
 *            stream. This function copied the entire rsvp_msg_hdr field by
 *            field into the struct rsvp_msg_hdr
 * Callers  : 
 *--------------------------------------------------------------------------*/
void
rsvp_packet_decode_msg_hdr (struct stream *s,
                            struct rsvp_msg_hdr *hdr) 
{
   u_char byte;
   /* Sanity Check. */
   if (!s || !hdr)
      return;
   /* Get the first byte from stream. */
   byte = stream_getc (s);
   /* Insert the version value from this byte.*/
   hdr->vers = byte >> 4;
   /* Insert the flags value from this byte.*/
   hdr->flags = byte & 0x0F;
   stream_forward (s, 1);
   /* Get Msg Type value*/
   hdr->msg_type = stream_getc (s);
   stream_forward (s, 1);
   /* Get the Msg Checksum .*/
   hdr->checksum = stream_getw (s);
   stream_forward (s, 2);
   /* Get the Send_TTL. */
   hdr->send_ttl = stream_getc (s);
   stream_forward (s, 1);
   /* Get the Reserved field.*/
   hdr->resv = stream_getc (s);
   stream_forward (s, 1);
   /* Get the Msg Length. */
   hdr->length = stream_getw (s);

   return;
}

/*--------------------------------------------------------------------------
 * Function : rsvp_packet_verify_checksum
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr.
 * Output   : Returns 1 if checksum is correct , else returns 0.
 * Synopsis : This utility calculates the checksum of a RSVP Header and 
 *            compares with that in the header.
 * Callers  : rsvp_packet_verify_msg_hdr in rsvp_packet.c
 *-------------------------------------------------------------------------*/
int
rsvp_packet_verify_checksum (struct rsvp_msg_hdr *hdr)
{
   u_int32_t ret;
   u_int32_t sum;

   /* Preserve the checksum and clear the field from heder. */
   sum = hdr->checksum;
   memset (&hdr->checksum, 0, sizeof (u_int16_t));

   /*Calculate Checksum. */
   ret = in_cksum (hdr, ntohs (hdr->length));

   /* Cross verify with the checksum that was obtained in packet.*/
   if (ret != sum) 
   {
      zlog_info ("[rsvp_packet_verify_checksum] : checksum mismatch, my %X,\
                  his %X", ret, sum);
      return 0;
   }

   return 1;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_verify_msg_hdr
 * Input    : rsvp_packet = Pointer to object of type struct rsvp_packet
 *          : rsvp_if     = Pointer to object of type struct rsvp_if
 *          : rsvp_msg_hdr= Pointer to the RSVP Message Header.
 * Output   : Returns 0 if verification successful, else returns -1.
 * Synopsis : This utility veifies errors if any in RSVP Standard Message 
 *            header.
 * Callers  : rsvp_read.
 *--------------------------------------------------------------------------*/
int
rsvp_packet_verify_msg_hdr (struct rsvp_packet  *rsvp_packet,
                            struct rsvp_if      *rsvp_if,
                            struct rsvp_msg_hdr *rsvp_msg_hdr)
{
  struct interface *ifp = rsvp_if->ifp;
  /* Check the RSVP version . */
  if (rsvp_msg_hdr->vers != RSVP_PROTO_VERSION)
  {
     zlog_warn ("[rsvp_packet_verify_msg_hdr] interface %s : RSVP version \
                 number mismatch.", ifp->name);
     return -1;
  }

  /* Check that we have minimum length.*/
  if (rsvp_msg_hdr->length <= RSVP_MSG_HDR_LEN)
  {
     zlog_warn ("[rsvp_packet_verify_msg_hdr] interface %s : RSVP Msg Length \
                 below minimum. ", ifp->name);
     return -1;
  }

  /* Verify the header checksum. */
  if (!rsvp_packet_verify_checksum (rsvp_msg_hdr))
  {
     zlog_warn ("[rsvp_packet_verify_msg_hdr] interface %s : RSVP Msg Cheksum \
                 Error, ", ifp->name);
     return -1;
  }  
  return 0; 
} 

/*-------------------------------------------------------------------------
 * Specific Type of Message Processing Routines in receive path
 *------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
 * Function : rsvp_packet_path_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Path Message received from a
 *            rsvp_if. It decodes the Path message and extracts all the msg 
 *            info. Then the function checks the sanity of the message info 
 *            and if all looks OK then further forwards the message info to 
 *            Softstate Module.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_path_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                          struct rsvp_packet  *rsvp_packet,
                          struct rsvp_if      *rsvp_if)
{
   struct rsvp_path_msg_info  path_msg_info;
   struct rsvp_ip_hdr_info    ip_hdr_info;
   size_t                     size;
   struct stream              *s;

   /* Sanity Check on Input Arguments.*/
   if (!rsvp_msg_hdr || !rsvp_packet || !rsvp_if)
      return;
   /* Get the size of the message - header.*/
   size = rsvp_msg_hdr->length - RSVP_MSG_HDR_LEN;
   s = rsvp_packet->s;
   /* Initialize the path_msg_info.*/
   bzero (&path_msg_info, sizeof (struct rsvp_path_msg_info));
   bzero (&ip_hdr_info, sizeof (struct rsvp_ip_hdr_info));

   /* Extract the IP Header Info.*/
   memcpy (&ip_hdr_info.ip_src, &rsvp_packet->ip_src, sizeof (struct in_addr));
   memcpy (&ip_hdr_info.ip_dst, &rsvp_packet->ip_dst, sizeof (struct in_addr));
   ip_hdr_info.ip_ttl = rsvp_packet->ip_ttl;

   /* Get the TTL from the RSVP Message Header.*/
   path_msg_info.rsvp_msg_ttl = rsvp_msg_hdr->send_ttl;

   /* Decode all the objects in the path message one by one in this loop*/
   while (size)
   { 
      u_int16_t obj_len;
      u_char    obj_class;
      u_char    obj_ctype;
      u_int32_t	read;

      /* If size is lsess than minimub object length then return.*/
      if (size <= RSVP_OBJ_LEN_MIN)
         return;
      /* Decode the object header.*/
      read = rsvp_obj_decode_hdr (s, &obj_len, &obj_class, &obj_ctype);

      /* Check that the object length is multiple of 4*/
      if (obj_len % 4)
      {
         /* Handle the error.*/
         return;
      } 
      size -= read;
      
      /* decode the object as per its class and ctype.*/      
      switch (obj_class)
      {
         case RSVP_CLASS_INTEGRITY:
            read = rsvp_obj_decode_integrity (s, obj_ctype, obj_len,
                                              &path_msg_info.integrity_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_INTEGRITY);
            size -= obj_len;
            break;
  
         case RSVP_CLASS_SESSION:
            read = rsvp_obj_decode_session (s, obj_ctype, obj_len, 
                                            &path_msg_info.session_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_SESSION);
            size -= obj_len;
            break;            

         case RSVP_CLASS_HOP:
            read = rsvp_obj_decode_hop (s, obj_ctype, obj_len,
                                        &path_msg_info.hop_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_HOP);
            size -= obj_len;
            break;

         case RSVP_CLASS_TIMEVAL:
            read = rsvp_obj_decode_timeval (s, obj_ctype, obj_len,
                                            &path_msg_info.timeval_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_TIMEVAL);
            size -= obj_len;
            break;

         case RSVP_CLASS_ERO:
            read = rsvp_obj_decode_hop (s, obj_ctype, obj_len,
                                        &path_msg_info.hop_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_HOP);
            size -= obj_len;            
            break;

         case RSVP_CLASS_LRO:
            read = rsvp_obj_decode_lro (s, obj_ctype, obj_len,
                                        &path_msg_info.lro_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_LRO);
            size -= obj_len;
            break;

         case RSVP_CLASS_SA        	:
            read = rsvp_obj_decode_sa (s, obj_ctype, obj_len,
                                       &path_msg_info.sa_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_SA);
            break;         

         case RSVP_CLASS_POLICY_DATA    :
            break;

         case RSVP_CLASS_ST:
            read = rsvp_obj_decode_st (s, obj_ctype, obj_len,
                                       &path_msg_info.st_info);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_ST);
            size -= obj_len;
            break;             

         case RSVP_CLASS_SENDER_TSPEC: 
            break;
     
         case RSVP_CLASS_ADSPEC		:
            break;

         case RSVP_CLASS_RRO		:
            read = rsvp_obj_decode_rro (s, obj_ctype, obj_len,
                                        &path_msg_info.rro_info_list);
            SET_FLAG (path_msg_info.cflags, PATH_MSG_CFLAG_RRO);
            size -= obj_len;
            break;

         default :
            return;
      }                   
   }// End of while ..

   /* Now send the path message info to the state module for processing.*/
   rsvp_state_recv_path (&ip_hdr_info, &path_msg_info, rsvp_if);
                
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_resv_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Resv Message received from a
 *            rsvp_if and extracts all its info.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_resv_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                          struct rsvp_packet *rsvp_packet,
                          struct rsvp_if *rsvp_if)
{
   struct rsvp_resv_msg_info  resv_msg_info;
   struct rsvp_ip_hdr_info    ip_hdr_info;	
   size_t                     size;
   struct stream              *s;

   /* Sanity Check on Input Arguments.*/
   if (!rsvp_msg_hdr || !rsvp_packet || !rsvp_if)
      return;
   /* Get the size of the message - header.*/
   size = rsvp_msg_hdr->length - RSVP_MSG_HDR_LEN;
   s = rsvp_packet->s;
   /* Initialize the path_msg_info.*/
   bzero (&resv_msg_info, sizeof (struct rsvp_resv_msg_info));
   bzero (&ip_hdr_info, sizeof (struct rsvp_ip_hdr_info));
   /* Fill up the ip_hdr_info from rsvp_packet.*/   
   memcpy (&ip_hdr_info.ip_src, &rsvp_packet->ip_src, sizeof (struct in_addr));
   memcpy (&ip_hdr_info.ip_dst, &rsvp_packet->ip_dst, sizeof (struct in_addr));
   ip_hdr_info.ip_ttl = rsvp_packet->ip_ttl;

   while (size)
   {
      u_int16_t obj_len;
      u_char    obj_class;
      u_char    obj_ctype;
      u_int32_t read;

      /* If size is lsess than minimub object length then return.*/
      if (size <= RSVP_OBJ_LEN_MIN)
         return;
      /* Decode the object header.*/
      read = rsvp_obj_decode_hdr (s, &obj_len, &obj_class, &obj_ctype);

      size -= read;

      /* Process individula objects.*/
      switch (obj_ctype)
      {
         case RSVP_CLASS_INTEGRITY:
            read = rsvp_obj_decode_integrity (s, obj_ctype, obj_len,
                                              &resv_msg_info.integrity_info);
            SET_FLAG (resv_msg_info.cflags, RESV_MSG_CFLAG_INTEGRITY);
            size -= read;
            break; 

         case RSVP_CLASS_SESSION:
            read = rsvp_obj_decode_session (s, obj_ctype, obj_len,
                                            &resv_msg_info.session_info);
            SET_FLAG (resv_msg_info.cflags, RESV_MSG_CFLAG_SESSION);
            size -= read;
            break;
 
         case RSVP_CLASS_HOP:
            read = rsvp_obj_decode_hop (s, obj_ctype, obj_len, 
                                        &resv_msg_info.hop_info);
            SET_FLAG (resv_msg_info.cflags, RESV_MSG_CFLAG_HOP); 
            size -= read;
            break;

         case RSVP_CLASS_TIMEVAL:
            read = rsvp_obj_decode_timeval (s, obj_ctype, obj_len,
                                            &resv_msg_info.timeval_info);
            SET_FLAG (resv_msg_info.cflags, RESV_MSG_CFLAG_HOP);
            size -= read;
            break;

         case RSVP_CLASS_RESV_CONF:

         case RSVP_CLASS_SCOPE:

         case RSVP_CLASS_POLICY_DATA:

         case RSVP_CLASS_STYLE:
	break;
      }

   }// End of while
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_path_err_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Path Err Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_path_err_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                              struct rsvp_packet  *rsvp_packet,
                              struct rsvp_if      *rsvp_if)
{
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_path_tear_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Path Tear Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void
rsvp_packet_resv_err_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                              struct rsvp_packet  *rsvp_packet,
                              struct rsvp_if      *rsvp_if)
{
   return;
}
/*---------------------------------------------------------------------------
 * Function : rsvp_packet_path_tear_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Path Tear Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_path_tear_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                               struct rsvp_packet *rsvp_packet,
                               struct rsvp_if *rsvp_if)
{
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_resv_tear_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Resv Tear Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_resv_tear_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                               struct rsvp_packet *rsvp_packet,
                               struct rsvp_if *rsvp_if)
{
   return;
} 
/*---------------------------------------------------------------------------
 * Function : rsvp_packet_resv_conf_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 * Synopsis : This function processes a RSVP Resv Conf Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_resv_conf_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                               struct rsvp_packet *rsvp_packet,
                               struct rsvp_if *rsvp_if)
{
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_hello_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 *          : size         = Size of the Path message.
 * Synopsis : This function processes a RSVP Hello Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_hello_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                           struct rsvp_packet *rsvp_packet,
                           struct rsvp_if *rsvp_if)
{
   return;
} 

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_bundle_receive
 * Input    : rsvp_msg_hdr = Pointer to object of type struct rsvp_msg_hdr
 *          : rsvp_packet  = Pointer to object of type struct rsvp_packet.
 *          : rsvp_if      = Pointer to object of type struct rsvp_if.
 *          : size         = Size of the Path message.
 * Synopsis : This function processes a RSVP Bundle Message received from a
 *            rsvp_if.
 * Callers  : rsvp_read in rsvp_packet.c
 *-------------------------------------------------------------------------*/
void 
rsvp_packet_bundle_receive (struct rsvp_msg_hdr *rsvp_msg_hdr,
                            struct rsvp_packet *rsvp_packet,
                            struct rsvp_if *rsvp_if)
{
   return;
} 



/*----------------------------------------------------------------------------
 * Following all are APIs provided by this module to its user (e.g rsvp_state)
 * for sending all types of RSVP Packets.
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_path
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : path_msg_info = Pointer to struct rsvp_path_msg_info that carries
 *          :                 the info to be sent on the RSVP Path Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Path Message. From the input arguments it builds 
 *          : the IP Header and the Path Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/  

void
rsvp_packet_send_path (struct rsvp_ip_hdr_info   *ip_hdr_info,
                       struct rsvp_path_msg_info *path_msg_info,
                       struct in_addr		 *nhop,
                       struct rsvp_if 		 *rsvp_if_out)
{

   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_resv
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : resv_msg_info = Pointer to struct rsvp_resv_msg_info that carries
 *          :                 the info to be sent on the RSVP Resv Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Resv Message. From the input arguments it builds 
 *          : the IP Header and the Resv Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/  

void
rsvp_packet_send_resv (struct rsvp_ip_hdr_info 	 *ip_hdr_info,
                       struct rsvp_resv_msg_info *resv_msg_info,
                       struct in_addr		 *nhop,
                       struct rsvp_if 		 *rsvp_if_out)
{
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_path_err
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : path_err_msg_info = Pointer to struct rsvp_path_err_msg_info that 
 *          :                     carries the info to be sent on the RSVP Path 
 *          :                     Error Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Path ErrMessage. From the input arguments it builds 
 *          : the IP Header and the Path Err Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/  

void
rsvp_packet_send_path_err (struct rsvp_ip_hdr_info	 *ip_hdr_info,
                           struct rsvp_path_err_msg_info *path_err_msg_info,
                           struct in_addr		 *nhop,
                           struct rsvp_if		 *rsvp_if_out)
{
 
   return;
}

/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_resv_err
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Resv Err Message.
 *          : resv_err_msg_info = Pointer to struct rsvp_resv_err_msg_info that 
 *          :                     carries the info to be sent on the RSVP Resv 
 *          :                     Err Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Resv Err Message. From the input arguments it builds 
 *          : the IP Header and the Resv Err Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/  

void
rsvp_packet_send_resv_err (struct rsvp_ip_hdr_info 	 *ip_hdr_info,
                           struct rsvp_resv_err_msg_info *resv_err_msg_info,
                           struct in_addr		 *nhop,
                           struct rsvp_if		 *rsvp_if_out)
{

   return;
}


/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_path_tear
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : path_tear_msg_info = Pointer to struct rsvp_path_tear_msg_info that 
 *          :                      carries the info to be sent on the RSVP Path 
 *          :                      Tear Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Path Tear Message. From the input arguments it builds 
 *          : the IP Header and the Path Tear Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/ 
void
rsvp_packet_send_path_tear (struct rsvp_ip_hdr_info        *ip_hdr_info,
                            struct rsvp_path_tear_msg_info *path_tear_msg_info,
                            struct in_addr		   *nhop,
                            struct rsvp_if		   *rsvp_if_out)
{

   return;
}


/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_resv_tear
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : resv_tear_msg_info = Pointer to struct rsvp_path_msg_info that 
 *          :                     carries the info to be sent on the RSVP Path 
 *          :                     Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Path Message. From the input arguments it builds 
 *          : the IP Header and the Path Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/ 
void
rsvp_packet_send_resv_tear (struct rsvp_ip_hdr_info 	   *ip_hdr_info,
                            struct rsvp_resv_tear_msg_info *resv_tear_msg_info,
                            struct in_addr		   *nhop,
                            struct rsvp_if		   *rsvp_if_out)
{

   return;
}


/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_resv_conf
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : path_err_msg_info = Pointer to struct rsvp_path_msg_info that 
 *          :                     carries the info to be sent on the RSVP Resv 
 *          :                     Conf Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Resv Conf Message. From the input arguments it builds 
 *          : the IP Header and the Resv Err Message as per spec. and sends it
 *          : as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/ 
void 
rsvp_packet_send_resv_conf (struct rsvp_ip_hdr_info	   *ip_hdr_info,
                            struct rsvp_resv_conf_msg_info *resv_conf_msg_info,
                            struct in_addr		   *nhop,
                            struct rsvp_if		   *rsvp_if_out)
{
   return;
}


/*---------------------------------------------------------------------------
 * Function : rsvp_packet_send_hello_req
 * Input    : ip_hdr        = Pointer to struct rsvp_ip_hdr containing the 
 *          :                 info to be inserted into the IP Header of the 
 *          :                 RSVP Message.
 *          : path_err_msg_info = Pointer to struct rsvp_path_msg_info that 
 *          :                     carries the info to be sent on the RSVP Hello 
 *          :                     Message.
 *          : nhop          = Pointer to struct in_addr that specifies the 
 *          :                 next hop the message should be sent to.
 *          : rsvp_if_out   = Pointer to struct rsvp_if via which the message
 *          :                 should be sent to the next hop.
 * Synopsis : This procedure is an interface provided by this module for 
 *          : sending RSVP Hello Req Message. From the input arguments it 
 *          : builds the IP Header and the Hello Message as per spec. and sends
 *          : it as rsvp_packet.
 * Callers  : (TBD).
 *--------------------------------------------------------------------------*/ 
void
rsvp_packet_send_hello_req (struct rsvp_ip_hdr_info	    *ip_hdr_info,
                            struct rsvp_hello_msg_info      *hello_msg_info,
                            struct in_addr		    *nhop,
                            struct rsvp_if		    *rsvp_if_out)
{

   return;
}	  

void
rsvp_packet_send_hello_ack (struct rsvp_ip_hdr_info	    *ip_hdr_info,
                            struct rsvp_hello_msg_info	    *hello_msg_info,
                            struct in_addr		    *nhop,
                            struct rsvp_if		    *rsvp_if_out)
{

   return;
} 


/*----------------------------------------------------------------------------
 * Function : rsvp_packet_receive
 * Input    : sock = The socket file descriptor
 *          : ifp  = Pointer to the pointer of interface to fill up.
 * Output   : Pointer to struct rsvp_packet
 * Synopsis : This function reads a RSVP Packet (including IP Header) from 
 *            the input socket. It creates a rsvp_packet and fills the IP 
 *            header information to it. Further it sets the rsvp_packet->s 
 *            to the rsvp message. It also extracts the incoming interface info
 *            and returns the info in the second argument.
 * Callers  : This is an intermediate function called from rsvp_read in 
 *            rsvp_packet.c
 *--------------------------------------------------------------------------*/
struct rsvp_packet *
rsvp_packet_receive (int sock, struct interface **ifp)
{
   int ret;
   struct ip iph;
   u_int16_t ip_len;
   struct rsvp_packet *rsvp_packet;
   unsigned int ifindex = 0;
   struct iovec iov;
   struct cmsghdr *cmsg;

#if defined (IP_PKTINFO)
   struct in_pktinfo *pktinfo;
#elif defined (IP_RECVIF)
   struct sockaddr_dl *pktinfo;
#else
   char *pktinfo;
#endif
   char buff [sizeof (*cmsg) + sizeof (*pktinfo)];
   struct msghdr msgh = {NULL, 0, &iov, 1, buff,
                         sizeof (*cmsg) + sizeof (*pktinfo), 0};

   /* Get the IP Header Information from the socket first to see 
      whether the packet is acceptable at IP level first. */
   ret = recvfrom (sock, (void *)&iph, sizeof (iph), MSG_PEEK, NULL, 0);

   if (ret != sizeof (iph))
   {
      zlog_warn ("[rsvp_packet_receive] Packet smaller than IP Header");
      return NULL;
   }

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(OpenBSD_IP_LEN)
   ip_len = iph.ip_len;
#else
   ip_len = ntohs (iph.ip_len);
#endif

#if !defined(GNU_LINUX) && !defined(OpenBSD_IP_LEN)
/* Kernel Network code touches incoming IP header parameters,
   before protocol specific processing.

   1) Convert byteorder to host represention.
    --->ip_len, ip_id, ip_off

   2) Adjust ip_len to strip IP header size!
    ---> If user process receive entire IP packet via RAW
         socket, it must consider adding IP header size to
         the "ip_len" field of "ip" structure.

   For more details, see <netinet/ip_input.c>
*/
   ip_len = ip_len + (iph.ip_hl << 2);   

#endif

   /* Allocate a new RSVP Packet. */
   rsvp_packet = rsvp_packet_new (ip_len);
   /* Now Get the RSVP Part of the message. */
   iov.iov_base = STREAM_DATA (rsvp_packet->s);
   iov.iov_len = ip_len;
   
   ret = recvmsg (sock, &msgh, 0);

   cmsg = CMSG_FIRSTHDR (&msgh);

   if (cmsg != NULL && cmsg->cmsg_level == IPPROTO_IP &&
#if defined (IP_PKTINFO)
      cmsg->cmsg_type == IP_PKTINFO
#elif defined (IP_RECVIF)
      cmsg->cmsg_type == IP_RECVIF
#else
      0
#endif
    )     
   {
#if defined (IP_PKTINFO)
      pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
      ifindex = pktinfo->ipi_ifindex;
#elif defined (IP_RECVIF)
      pktinfo = (struct sockaddr_dl *)CMSG_DATA(cmsg); 
      ifindex = pktinfo->sdl_index;
#else
      ifindex = 0;
#endif
   }

   *ifp = if_lookup_by_index (ifindex);

   if (ret != ip_len)
   {
      zlog_warn ("[rsvp_packet_receive] short read. "
                  "ip_len %d bytes read %d",ip_len, ret);
      rsvp_packet_free (rsvp_packet);
      return NULL;
   }
   
   /* Copy the IP Src,Dst addr and ttl into the rsvp_packet.*/
   memcpy (&rsvp_packet->ip_src, &iph.ip_src, sizeof (struct in_addr));
   memcpy (&rsvp_packet->ip_dst, &iph.ip_dst, sizeof (struct in_addr));
   rsvp_packet->ip_ttl = iph.ip_ttl;
   /* Forward the stream by IP Hdr so that it now pointer to the RSVP Msg.*/
   stream_forward (rsvp_packet->s, iph.ip_hl * 4);    
	
   return rsvp_packet;   
}

/*----------------------------------------------------------------------------
 * Function : rsvp_write
 * Input    : thread = Pointer to object of type struct thread.
 * Output   : Returns 0 if succesful, else returns -1
 * Synopsis : This function is the final write function to network. This 
 *            procedure writes a rsvp_packet enqueued on interfaces to 
 *            the respective next hop. Its a thread function and is invoked
 *            when the thread is executed.
 * Callers  : Thread function (TBB..)
 *---------------------------------------------------------------------------*/
int
rsvp_write (struct thread *thread)
{
   struct rsvp *rsvp = THREAD_ARG (thread);
   struct rsvp_if *rsvp_if_out;
   struct rsvp_packet *rsvp_packet;
   struct sockaddr_in sa_dst;
   struct ip iph;
   struct msghdr msg;
   struct iovec iov[2];
   int ret;
   struct listnode *node;
   int flags = 0;
   
   rsvp->t_write = NULL;

   /* Get the interface list that has packets send pending. */
   node = listhead (rsvp->rsvp_if_write_q);
   assert (node);
   rsvp_if_out = getdata (node);
   assert (rsvp_if_out);

   /* Get one packet from queue. */
   rsvp_packet = rsvp_packet_fifo_head (rsvp_if_out->obuf);
   assert (rsvp_packet);
   assert (rsvp_packet->length >= RSVP_MSG_HDR_LEN);

   /* Fill the destination socket from the next hop info in rsvp_packet.*/
   memset (&sa_dst, 0, sizeof (sa_dst));
   sa_dst.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
   sa_dst.sin_len  = sizeof (sa_dst);
#endif /* HAVE_SIN_LEN */
   sa_dst.sin_addr = rsvp_packet->nhop;
   sa_dst.sin_port = htons (0);

   /* We need create the IP header here from the ip_src, ip_dst and ip_ttl 
    * of the rsvp_packet.*/
   iph.ip_hl = sizeof (struct ip) >> 2;
   iph.ip_v  = IPVERSION;
   iph.ip_tos = IPTOS_PREC_INTERNETCONTROL;
#if defined(__NetBSD__) || defined(__FreeBSD__)
   iph.ip_len = iph.ip_hl*4 + rsvp_packet->length;
#else
   iph.ip_len = htons (iph.ip_hl*4 + rsvp_packet->length);
#endif
   iph.ip_id = 0;
   iph.ip_off = 0;
   iph.ip_ttl = RSVP_IP_TTL;
   iph.ip_p = IPPROTO_RSVP;
   iph.ip_sum = 0;
   memcpy (&iph.ip_src, &rsvp_packet->ip_src, sizeof (struct in_addr));
   memcpy (&iph.ip_src, &rsvp_packet->ip_dst, sizeof (struct in_addr));

   memset (&msg, 0, sizeof (msg));
   msg.msg_name = &sa_dst;
   msg.msg_namelen = sizeof (sa_dst);
   msg.msg_iov = iov;
   msg.msg_iovlen = 2;
   iov[0].iov_base = (char *)&iph;
   iov[0].iov_len  = iph.ip_hl*4;
   iov[1].iov_base = STREAM_DATA (rsvp_packet->s);
   iov[1].iov_len = rsvp_packet->length;
   
   /* Send the RSVP Message now. */ 
   ret = sendmsg (rsvp->sock, &msg, flags);

   if (ret < 0)
      zlog_warn ("*** sendmsg in rsvp_write failed with %s", strerror(errno));

   /* Put Debug Info Here. */

   /* Now delete the packet from queue in the interface. */
   rsvp_packet_del_from_if (rsvp_if_out);            
  
   /* If there is no pending packet on this interface. */
   if (rsvp_packet_fifo_head (rsvp_if_out->obuf) == NULL)
   {
      /* Set that the queue is empty on this interface. */
      rsvp_if_out->on_write_q = 0;
      /* Delete this interface from the interface list in 
         rsvp_>rsvp_if_write_q. */
      list_delete_node (rsvp->rsvp_if_write_q, node);
   }

   /* Check if packets still remain in queue, call write thread again. */
   if (!list_isempty (rsvp->rsvp_if_write_q))
      rsvp->t_write = thread_add_write (master, rsvp_write, rsvp, rsvp->sock);

   return 0;
}

/*-----------------------------------------------------------------------------
 * Function : rsvp_read
 * Input    : thread = Pointer to object of type struct thread.
 * Output   : Returns 0 if read successful, else retunns -1.
 * Synopsis : This is the central receive function for all RSVP Messages. It's
 *            the starting point of packet receive function. AS its a thread
 *            function it get's called when the rsvp->sock is ready with 
 *            incoming packets.
 * Callers  : thread_execute() in thread.c 
 *---------------------------------------------------------------------------*/
int
rsvp_read (struct thread *thread)
{
   int ret;
   struct rsvp_packet *rsvp_packet;
   struct rsvp *rsvp;
   struct rsvp_if *rsvp_if;
   struct rsvp_msg_hdr *rsvp_msg_hdr;
   u_int16_t length;
   struct interface *ifp;

   /* Get the rsvp structure from thread. */
   rsvp = THREAD_ARG (thread);
   rsvp->t_read = NULL;

   /* Prepare the thread for the next packet.*/
   rsvp->t_read = thread_add_read (master, rsvp_read, rsvp, rsvp->sock);

   /* Read RSVP Packet into rsvp_packet and incoming interface into &ifp */
   rsvp_packet = rsvp_packet_receive (rsvp->sock, &ifp);

   if (rsvp_packet == NULL)
      return -1;

   /* Get the rsvp_if from the interface pointer.*/
   rsvp_if = (struct rsvp_if *)ifp->info;

   /* Check if the interface is configured for RSVP. Else return */
   if (!CHECK_FLAG (rsvp_if->sflags, RSVP_IF_SFLAG_RSVP))
   {
      rsvp_packet_free (rsvp_packet);
      return -1;
   }
   /* Self Originated packet should be discarded silently.So we 
      don't say it as error */
   if (rsvp_if_lookup_by_local_addr (rsvp_packet->ip_src))
   {
      rsvp_packet_free (rsvp_packet);
      return 0;
   }

   /* Get the RSVP Message Header. */
   rsvp_msg_hdr = (struct rsvp_msg_hdr *)STREAM_PNT (rsvp_packet->s);
   /* Do Header Verification. */
   ret = rsvp_packet_verify_msg_hdr (rsvp_packet, rsvp_if, rsvp_msg_hdr);

   if (ret < 0)
   {
      rsvp_packet_free (rsvp_packet);
      return ret;
   }
   /* Forward by RSVP Header Size. */
   stream_forward (rsvp_packet->s, RSVP_MSG_HDR_LEN);

   /* Adjust size to message length. */
   length = ntohs (rsvp_msg_hdr->length) - RSVP_MSG_HDR_LEN;

   /* Read rest of the packet and call each specific routibe based on type
      of packet. The rsvp_packet is not freed here. It is frred inside 
      individual processing modules */
   switch (rsvp_msg_hdr->msg_type)
   {
      case RSVP_MSG_TYPE_PATH:
         rsvp_packet_path_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break;

      case RSVP_MSG_TYPE_RESV:
         rsvp_packet_resv_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break;
    
      case RSVP_MSG_TYPE_PATH_ERR:
         rsvp_packet_path_err_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break; 
 
      case RSVP_MSG_TYPE_RESV_ERR:
         rsvp_packet_resv_err_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break;

      case RSVP_MSG_TYPE_PATH_TEAR:
         rsvp_packet_path_tear_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break;

      case RSVP_MSG_TYPE_RESV_TEAR:
         rsvp_packet_resv_tear_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break;

      case RSVP_MSG_TYPE_RESV_CONF:
         rsvp_packet_resv_conf_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break; 

      case RSVP_MSG_TYPE_HELLO:
         rsvp_packet_hello_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break; 
     
      case RSVP_MSG_TYPE_BUNDLE:
         rsvp_packet_bundle_receive (rsvp_msg_hdr, rsvp_packet, rsvp_if);
         break;

      default :
         zlog (NULL, LOG_WARNING, "RSVP Message Type Illegal");
         rsvp_packet_free (rsvp_packet);
         break;
   }

   return 0;
}
