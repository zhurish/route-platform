/*
 * OLSR Rout(e)ing protocol
 *
 * Copyright (C) 2005        Tudor Golubenco
 *                           Polytechnics University of Bucharest 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public Licenseas published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
 * more details.

 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <zebra.h>

#include "thread.h"
#include "memory.h"
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "sockunion.h"
#include "stream.h"
#include "log.h"
#include "sockopt.h"
/* 2016年7月3日 15:32:09 zhurish: 修改头文件名，原来是md5-gnu.h */
#include "md5.h"
/* 2016年7月3日 15:32:09  zhurish: 修改头文件名，原来是md5-gnu.h */

#include "olsrd/olsrd.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_linkset.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_mpr.h"
#include "olsrd/olsr_dup.h"
#include "olsrd/olsr_time.h"
#include "olsrd/olsr_vty.h"
#include "olsrd/olsr_debug.h"
#include "olsrd/olsr_route.h"


/*
  RFC 3632: 18.3

  The Vtime in the message header (see section 3.3.2), and the Htime in
  the HELLO message (see section 6.1) are the fields which hold
  information about the above values in mantissa and exponent format
  (rounded up).  In other words:
  
  value = C*(1+a/16)*2^b [in seconds]
  
  where a is the integer represented by the four highest bits of the
  field and b the integer represented by the four lowest bits of the
  field.
*/
float olsr_uchar_to_float (struct olsr *olsr, u_char c)
{
  int a = c >> 4;
  int b = c & 0xff;
  
  return olsr->C * (1.0 + (float)a / 16.0) * (float)(1 << b);
}

/*
   Given one of the above holding times, a way of computing the
   mantissa/exponent representation of a number T (of seconds) is the
   following:

     -    find the largest integer 'b' such that: T/C >= 2^b

     -    compute the expression 16*(T/(C*(2^b))-1), which may not be a
          integer, and round it up.  This results in the value for 'a'

     -    if 'a' is equal to 16: increment 'b' by one, and set 'a' to 0

     -    now, 'a' and 'b' should be integers between 0 and 15, and the
          field will be a byte holding the value a*16+b
 */
u_char olsr_float_to_uchar (struct olsr *olsr, float T)
{
  int  a, b;
  float exp;

  b = 0;

  while (T / olsr->C >= 1 << b)
    ++ b;
  b --;

  exp = 16.0 * (T / (olsr->C * (1 << b)) - 1.0);
  a = (int)(exp + 0.5);		/* round. */

  if (a >= 16)
    {
      b ++;
      a -= 16;
    }

  return ((u_char)a << 4) | (u_char)b;
}

/*
 * Utility function that advances in an output stream. Should be in stream.c
 */
void
stream_put_forward (struct stream *s, int size)
{
/* 2016年7月3日 15:32:39 zhurish: 修改数据流结束点设置功能 */
  //s->putp += size;
  stream_forward_endp(s, size); 
/* 2016年7月3日 15:32:39  zhurish: 修改数据流结束点设置功能 */
}

struct duplicate* olsr_dup_set_search (struct olsr *olsr, struct in_addr addr,
				       u_int16_t msn)
{
  return NULL;
}


/* Gets an olsr header from the input stream. */
struct olsr_header * olsr_get_header (struct olsr *olsr, struct stream *stream)
{
  struct olsr_header *oh = XCALLOC (MTYPE_OLSR_HEADER, sizeof(struct olsr_header));
  
  oh->mtype = stream_getc (stream);
  oh->vtime = olsr_uchar_to_float (olsr, stream_getc (stream));
  oh->m_size = stream_getw (stream);

  stream_get (&oh->oaddr, stream,  4);

  oh->ttl = stream_getc (stream);
  oh->hops = stream_getc (stream);
  oh->msn = stream_getw (stream);
  
  return oh;
}

/* Gets an olsr hello header from the input stream. */
struct olsr_hello_header *
olsr_get_hello_header (struct olsr *olsr, struct stream *stream)
{
  struct olsr_hello_header *ohh = XCALLOC (MTYPE_OLSR_HELLO_HEADER, 
					   sizeof(struct olsr_hello_header));

  stream_forward (stream, 2);	/* Reserved. */
  ohh->htime = olsr_uchar_to_float (olsr, stream_getc (stream));
  ohh->will = stream_getc (stream);

  return ohh;
}

/* Gets an olsr link header from the input stream. */
struct olsr_link_header *
olsr_get_link_header (struct stream *stream)
{
  u_char link_code;
  struct olsr_link_header *olh = XCALLOC (MTYPE_OLSR_LINK_HEADER, 
					  sizeof(struct olsr_link_header));

  /* Get and decode Link Code */
  link_code = stream_getc (stream);
  olh->lt = link_code & 0x03;
  olh->nt = (link_code >> 2) & 0x03;
  
  stream_forward (stream, 1);	/* Reserved. */
  
  olh->lm_size = stream_getw (stream);

  return olh;
}


void olsr_process_hello_msg (struct olsr *olsr, struct olsr_interface *oi,
			     struct ip *iph,
			     struct olsr_header *oh, struct stream *stream)
{
  struct olsr_hello_header *ohh;
  u_int16_t m_offset, lm_offset;
  struct olsr_link *ol;
  struct in_addr neigh_addr;
  struct olsr_neigh *sym_neigh;
  struct olsr_mid *mid;

  ohh = olsr_get_hello_header (olsr, stream);

  if (IS_DEBUG_OLSR_PACKET (oh->mtype))
    zlog_debug ("HELLO MSG: htime: %f willigness: %s",
		ohh->htime, olsr_will_str (ohh->will));

  /* If the address of the interface that send this hello message is
     not the same as the originator, then we can add a MID tuple as the
     HELLO messages must never be transmitied.
     (FIXME: Not in RFC, is it okay?)
  */
  if (memcmp (&iph->ip_src, &oh->oaddr, 4) != 0)
    {
      mid = olsr_mid_lookup (olsr, &iph->ip_src, &oh->oaddr);
      if (mid == NULL)
	olsr_mid_add (olsr, oh->vtime, &iph->ip_src, &oh->oaddr);
    }

  /* Find if this is form a SYM neighbor. */
  sym_neigh = olsr_neigh_lookup_addr (olsr, &oh->oaddr);

  /* Search in linkset */
  ol = olsr_linkset_search (oi, &iph->ip_src);

  if (ol == NULL)
    ol = olsr_linkset_add (olsr, oi, iph, oh, ohh);

  olsr_linkset_update_asym (ol, oh);

  /* Update willingness. */
  ol->neigh->will = ohh->will;


  /* Process links */  
  m_offset = stream_get_getp (stream) - 16;
  
  while (stream_get_getp (stream) - m_offset < oh->m_size)
    {
      struct olsr_link_header *olh;
      lm_offset = stream_get_getp (stream);

      olh = olsr_get_link_header (stream);

      if (IS_DEBUG_OLSR_PACKET (oh->mtype))      
	zlog_debug ("LINK:  lt: %s nt: %s lm_size: %d",
		    olsr_lt_str (olh->lt), olsr_nt_str (olh->nt), olh->lm_size);

      /* Read neigbor addresses. */
      while (stream_get_getp (stream) - lm_offset < olh->lm_size)
	{
	  stream_get (&neigh_addr, stream, 4);

	  if (IS_DEBUG_OLSR_PACKET (oh->mtype))
	    zlog_debug ("addr: %s", inet_ntoa (neigh_addr));

	  /* RFC 3626 8.2.1.
	     Upon receiving a HELLO message from a symmetric neighbor, a node
	     SHOULD update its 2-hop Neighbor Set.
	  */
	  if (sym_neigh && (olh->nt == OLSR_NEIGH_SYM || olh->nt == OLSR_NEIGH_MPR))
	    olsr_hop2_update (olsr, sym_neigh, &neigh_addr, oh->vtime);
	  
	  if (sym_neigh && olh->nt == OLSR_NEIGH_NOT)
	    olsr_hop2_remove (olsr, sym_neigh, &neigh_addr);

	  /* RFC 3626 8.4.1
	     Upon receiving a HELLO message, if a node finds one of its own
	     interface addresses in the list with a Neighbor Type equal to
	     MPR_NEIGH, information from the HELLO message must be recorded in the
	     MPR Selector Set.
	  */
	  if (olh->nt == OLSR_NEIGH_MPR && 
	      olsr_if_lookup_by_addr (olsr, &neigh_addr))
	    {
	      if (sym_neigh == NULL)
		zlog_warn ("%s is not symetric neighbor but it selected me as MPR.",
			   inet_ntoa (oh->oaddr));
	      else
		olsr_is_mprs_set (sym_neigh, oh->vtime);
	    }

	  /* If the neighbor address referes to this interface, update
	   * link status. 
	   */
	  if (memcmp (&neigh_addr, &OLSR_IF_ADDR (oi), 4) == 0)
	    olsr_linkset_update_sym_time (olsr, ol, olh, oh);
	}

      XFREE (MTYPE_OLSR_LINK_HEADER, olh);
    }

  olsr_linkset_update_time (ol);
}


void 
olsr_process_mid_msg (struct olsr *olsr, struct olsr_header *oh,
		      struct ip* iph, struct stream *stream)
{
  u_int16_t m_offset;
  struct olsr_mid *mid;
  struct in_addr addr;
  

  /* RFC 3626 5.4.
     If the sender interface (NB: not originator) of this message
     is not in the symmetric 1-hop neighborhood of this node, the
     message MUST be discarded.
  */
  olsr_mid_get_main_addr (olsr, &iph->ip_src, &addr);
  if (! olsr_neigh_lookup_addr (olsr, &addr))
    {
      if (IS_DEBUG_OLSR_PACKET (oh->mtype))
	zlog_debug ("Ignored because the sender is not my neighbor");
      return;
    }


  m_offset = stream_get_getp (stream) - OLSR_MSG_HDR_SIZE;
  while (stream_get_getp (stream) - m_offset < oh->m_size)
    {
      stream_get (&addr, stream, 4);

      if (IS_DEBUG_OLSR_PACKET (oh->mtype))
  	zlog_debug ("addr: %s", inet_ntoa (addr));
      
      mid = olsr_mid_lookup (olsr, &addr, &oh->oaddr);

      if (mid == NULL)
	olsr_mid_add (olsr, oh->vtime, &addr, &oh->oaddr);
      else
	olsr_mid_update_time (mid, oh->vtime);
    }
}


void 
olsr_process_tc_msg (struct olsr *olsr, struct olsr_header *oh,
		     struct ip *iph, struct stream *stream)
{
  u_int16_t m_offset;
  struct olsr_top *top;
  struct in_addr addr;
  u_int16_t ansn;

  /* RFC 3626 9.5.
     If the sender interface (NB: not originator) of this message
     is not in the symmetric 1-hop neighborhood of this node, the
     message MUST be discarded.
  */
  olsr_mid_get_main_addr (olsr, &iph->ip_src, &addr);
  if (! olsr_neigh_lookup_addr (olsr, &addr))
    {
      if (IS_DEBUG_OLSR_PACKET (oh->mtype))
	zlog_debug ("Ignored because the sender is not my neighbor");
      return;
    }


  m_offset = stream_get_getp (stream) - OLSR_MSG_HDR_SIZE;

  ansn = stream_getw (stream);

  if (IS_DEBUG_OLSR_PACKET (oh->mtype))
    zlog_debug ("ansn: %d", ansn);

  stream_forward (stream, 2);	                 /* Reserverd. */

  /*
    2    If there exist some tuple in the topology set where:

               T_last_addr == originator address AND

               T_seq       >  ANSN,

          then further processing of this TC message MUST NOT be
          performed and the message MUST be silently discarded (case:
          message received out of order).
   */
  if (olsr_top_exists_newer (olsr, &oh->oaddr, ansn))
    return;

  /*
    3    All tuples in the topology set where:

               T_last_addr == originator address AND

               T_seq       <  ANSN

          MUST be removed from the topology set.
   */
  olsr_top_cleanup_older (olsr, &oh->oaddr, ansn);


  while (stream_get_getp (stream) - m_offset < oh->m_size)
    {
      stream_get (&addr, stream, 4);

      if (IS_DEBUG_OLSR_PACKET (oh->mtype))
  	zlog_debug ("addr: %s", inet_ntoa (addr));

      top = olsr_top_lookup (olsr, &addr, &oh->oaddr);

      if (top == NULL)
	olsr_top_add (olsr, oh->vtime, ansn, &addr, &oh->oaddr);
      else
	olsr_top_update_time (top, oh->vtime);
    }
}

int 
olsr_read_packet (struct olsr *olsr, struct stream *ibuf,
		  struct olsr_interface *oi,
		  struct ip *iph, u_int16_t plen)
{
  u_int16_t p_len, psn;
  u_int16_t p_offset, next_msg_off, msg_off;
  struct olsr_header *oh;
  struct olsr_dup *dup;

  p_len = stream_getw (ibuf);
  psn = stream_getw (ibuf);

  if (p_len != plen)
    zlog_warn ("Packet length from olsr header and the length from the UDP header are"
	       "not the same");
  
  if (plen < OLSR_PACKET_MINSIZE)
    {
      zlog_warn ("packet size %d is smaller than minimum size %d",
		 plen, OLSR_PACKET_MINSIZE);
      return plen;
    }
  
  /* Get current position in stream. */
  p_offset = stream_get_getp (ibuf) - 4;
  
  /* For each message. */
  while (stream_get_getp (ibuf) - p_offset < plen)
    {
      msg_off = stream_get_getp (ibuf);
      oh = olsr_get_header (olsr, ibuf);

      if (IS_DEBUG_OLSR_PACKET (oh->mtype))
	{
	  /* Dump ip and udp headers. */
	  zlog_debug (" ");
	  zlog_debug ("------------------------------------------------");
	  zlog_debug ("ip_src: %s", inet_ntoa (iph->ip_src));
	  zlog_debug ("ip_dst: %s", inet_ntoa (iph->ip_dst));  

	  zlog_debug ("mtype: %s vtime: %f  m_size: %d" ,
		      OLSR_MSG_TYPE (oh->mtype), oh->vtime, oh->m_size);
	  zlog_debug ("orig: %s ttl: %d hops: %d msn %d", inet_ntoa (oh->oaddr),
		      oh->ttl, oh->hops, oh->msn);
	}
      

      next_msg_off = msg_off + oh->m_size;

      /* Processing conditions. */
      if (oh->ttl == 0)
	{
	  /* Skip message. */
	  stream_set_getp (ibuf, next_msg_off);
	  continue;
	}

      switch (oh->mtype)
	{
	case OLSR_HELLO_MSG:
	  olsr_process_hello_msg (olsr, oi, iph, oh, ibuf);
	  
	  /* RFC 3626 8.2.1
	     Notice, that a HELLO message
	     MUST neither be forwarded nor be recorded in the duplicate set.
	  */
	  break;

	case OLSR_MID_MSG:
	  
	  dup = olsr_dup_lookup (olsr, &oh->oaddr, oh->msn);

	  if (dup == NULL)
	    olsr_process_mid_msg (olsr, oh, iph, ibuf);

	  /* RFC 3626 5.3.
	     MID messages are broadcast and retransmitted by the MPRs in order to
	     diffuse the messages in the entire network.  The "default forwarding
	     algorithm" (described in section 3.4) MUST be used for forwarding of
	     MID messages.
	  */
	  if (olsr_default_forwarding (olsr, dup, oi, oh, &iph->ip_src))
	    olsr_forward (olsr, oh, ibuf, msg_off);

	  break;

	case OLSR_TC_MSG:

	  dup = olsr_dup_lookup (olsr, &oh->oaddr, oh->msn);

	  if (dup == NULL)
	    olsr_process_tc_msg (olsr, oh, iph, ibuf);

	  /* RFC 3626 9.4
	     TC messages are broadcast and retransmitted by the MPRs in order to
	     diffuse the messages in the entire network.  TC messages MUST be
	     forwarded according to the "default forwarding algorithm"
	   */
	  if (olsr_default_forwarding (olsr, dup, oi, oh, &iph->ip_src))
	    olsr_forward (olsr, oh, ibuf, msg_off);

	  break;
  
	default:
	  /* Unknown type, don't know how to process it but I might forward it. */
	  dup = olsr_dup_lookup (olsr, &oh->oaddr, oh->msn);

	  if (olsr_default_forwarding (olsr, dup, oi, oh, &iph->ip_src))
	    olsr_forward (olsr, oh, ibuf, msg_off);
	  break;
	}

      stream_set_getp (ibuf, next_msg_off);

      /* TODO: free msg. */
    }

  return 0;
}



struct stream *
olsr_recv_packet (int fd, struct interface **ifp, struct ip *iph)
{
  int ret;
  u_int16_t ip_len;
  struct stream *ibuf;
  unsigned int ifindex = 0;
  struct iovec iov;
  /* Header and data both require alignment. */
  char buff [CMSG_SPACE(SOPT_SIZE_CMSG_IFINDEX_IPV4())];
  struct msghdr msgh;

  memset (&msgh, 0, sizeof (struct msghdr));
  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;
  msgh.msg_control = (caddr_t) buff;
  msgh.msg_controllen = sizeof (buff);
  
  /* Read IP header first. */
  ret = recvfrom (fd, (void *)iph, 20, MSG_PEEK, NULL, 0);
  
  if (ret != 20)
    {
      zlog_warn ("olsr_recv_packet packet smaller than ip header");
      /* XXX: We peeked, and thus perhaps should discard this packet. */
      return NULL;
    }
  
  sockopt_iphdrincl_swab_systoh (iph);

  ip_len = iph->ip_len;
  
#if !defined(GNU_LINUX) && (OpenBSD < 200311)
  /*
   * Kernel network code touches incoming IP header parameters,
   * before protocol specific processing.
   *
   *   1) Convert byteorder to host representation.
   *      --> ip_len, ip_id, ip_off
   *
   *   2) Adjust ip_len to strip IP header size!
   *      --> If user process receives entire IP packet via RAW
   *          socket, it must consider adding IP header size to
   *          the "ip_len" field of "ip" structure.
   *
   * For more details, see <netinet/ip_input.c>.
   */
  ip_len= ip_len + (iph.ip_hl << 2);
#endif
  
  ibuf = stream_new (ip_len);
  iov.iov_base = STREAM_DATA (ibuf);
  iov.iov_len = ip_len;
  ret = recvmsg (fd, &msgh, 0);
  
  ifindex = getsockopt_ifindex (AF_INET, &msgh);
  
  *ifp = if_lookup_by_index (ifindex);

  if (ret != ip_len)
    {
      zlog_warn ("olsr_recv_packet short read. "
		 "ip_len %d bytes read %d", ip_len, ret);
      stream_free (ibuf);
      return NULL;
    }
  
  return ibuf;
}



/* Read incoming packet. */
int 
olsr_read (struct thread *thread)
{
  struct olsr *olsr;
  struct interface *ifp;
  struct olsr_interface *oi;
  struct ip iph;
  struct stream *ibuf;
  u_int16_t port;
  u_int16_t len;
  int ret;
  

  /* Fetch packet and reschedule thread. */
  olsr = THREAD_ARG (thread);
  olsr->t_read = NULL;

  /* read OLSR packet. */
  ibuf = olsr_recv_packet (olsr->fd, &ifp, &iph);
  if (ibuf == NULL)
    return -1;

  if (ifp == NULL)
    /* Handle cases where the platform does not support retrieving the ifindex,
       and also platforms (such as Solaris 8) that claim to support ifindex
       retrieval but do not. */
    ifp = if_lookup_address (iph.ip_dst);

  
  if (ifp == NULL)
    {
      stream_free (ibuf);
      return 0;
    }

  olsr->t_read = thread_add_read (master, olsr_read, olsr, olsr->fd);

  /* Self-originated packet should be discarded silently. */
  if (olsr_if_check_address (iph.ip_src))
    {
      stream_free (ibuf);
      return 0;
    }


  oi = olsr_oi_lookup_by_ifp (olsr, ifp);
  if (oi == NULL)
    {
      /* OLSR is not enabled on this interface. */
      stream_free (ibuf);
      return 0;
    }
  

  /* Skip IP header. */
  stream_forward (ibuf, 20);
  
  /* Read UDP dest. port. and packet len. */
  stream_forward (ibuf, 2);  
  port = stream_getw (ibuf);
  len = stream_getw (ibuf) - 8;
  stream_forward (ibuf, 2);	/* skip checksum. */

  if (port != olsr->port)
    return 0;
    

  if ((ret = olsr_read_packet (olsr, ibuf, oi, &iph, len)) != 0)
    return ret;
  
  return 0;
}


/*
 * Create a new interface link.
 */
struct olsr_oi_link *
olsr_oi_link_new ()
{
  return XCALLOC (MTYPE_OLSR_OI_LINK, sizeof (struct olsr_oi_link));
}


/*
 * Add a new oi_link to the list sorted in such a way that
 * elements with the same lt and gt are one after another.
 */
void
olsr_oll_add (struct list *list, struct olsr_oi_link *new)
{
  struct listnode *node;
  struct olsr_oi_link *oll;

  for (node = listhead (list); node; nextnode (node))
    {
      oll = getdata (node);

      if (oll->lt == new->lt && oll->nt == new->nt)
	{
	  listnode_add_after (list, node, new);
	  return;
	}
    }

  /* If no similar node exists, add to tail. */
  listnode_add (list, new);
}

/*
 * Build the links list for this interface. 
 */
struct list *
olsr_build_oi_links (struct olsr_interface *oi)
{
  struct list *list;
  struct listnode *node;



  list = list_new ();

  for (node = listhead (oi->linkset); node; nextnode (node))
    {
      struct olsr_oi_link *new = olsr_oi_link_new ();
      struct olsr_link *ol = getdata (node);

      /* RFC 3626 6.2.
	 
      1    The Link Type set according to the following:

      1.1  if L_SYM_time >= current time (not expired)
      
      Link Type = SYM_LINK
      */
      if (!ol->sym_expired)
	new->lt = OLSR_LINK_SYM;
      /*
	1.2  Otherwise, if L_ASYM_time >= current time (not expired)
	AND
	L_SYM_time  <  current time (expired)

	Link Type = ASYM_LINK
      */
      else
	if (!ol->asym_expired)
	  new->lt = OLSR_LINK_ASYM;
      /*
	1.3  Otherwise, if L_ASYM_time < current time (expired) AND
	L_SYM_time  < current time (expired)
	Link Type = LOST_LINK 
      */
	else 
	  new->lt = OLSR_LINK_LOST;


      /*
	2    The Neighbor Type is set according to the following:

	2.1  If the main address, corresponding to
	L_neighbor_iface_addr, is included in the MPR set:

	Neighbor Type = MPR_NEIGH
      */
      if (ol->neigh->is_mpr)
	new->nt = OLSR_NEIGH_MPR;
      /*
	2.2  Otherwise, if the main address, corresponding to
	L_neighbor_iface_addr, is included in the neighbor set:

	2.2.1
	if N_status == SYM

	Neighbor Type = SYM_NEIGH
	2.2.2
	Otherwise, if N_status == NOT_SYM
	Neighbor Type = NOT_NEIGH	 
      */
      else
	if (ol->neigh->status == OLSR_NEIGH_SYM)
	  new->nt = OLSR_NEIGH_SYM;
	else
	  if (ol->neigh->status == OLSR_NEIGH_NOT)
	    new->nt = OLSR_NEIGH_NOT;

      memcpy (&new->neigh_addr, &ol->neigh_addr, sizeof (struct in_addr));

      olsr_oll_add (list, new);
      
    }

  /*
    For each tuple in the Neighbor Set, for which no
    L_neighbor_iface_addr from an associated link tuple has been
    advertised by the previous algorithm,  N_neighbor_main_addr is
    advertised with:

    - Link Type = UNSPEC_LINK,

    - Neighbor Type set as described in step 2 above
  */
  for (node = listhead (oi->olsr->neighset); node; nextnode (node))
    {
      struct olsr_neigh *on = getdata (node);
      struct listnode *lnode;
      int found = FALSE;

      for (lnode = listhead (on->assoc_links); lnode; nextnode (lnode))
	{
	  struct olsr_link *ol = getdata (lnode);

	  if (ol->oi == oi)
	    {
	      found = TRUE;
	      break;
	    }
	}

      if (! found)
	{
	  struct olsr_oi_link *new = olsr_oi_link_new ();

	  new->lt = OLSR_LINK_UNSPEC;
	  
	  if (on->is_mpr)
	    new->nt = OLSR_NEIGH_MPR;
	  else
	    if (on->status == OLSR_NEIGH_SYM)
	      new->nt = OLSR_NEIGH_SYM;
	    else
	      if (on->status == OLSR_NEIGH_NOT)
		new->nt = OLSR_NEIGH_NOT;

	  

	  memcpy (&new->neigh_addr, &on->main_addr, sizeof (struct in_addr));
      
	  olsr_oll_add (list, new);

	}
    }

  return list;
}

/*
 * Create new output message.
 */
struct olsr_msg *
olsr_msg_new (struct in_addr *dst, long bufsize)
{
  struct olsr_msg *msg = XCALLOC (MTYPE_OLSR_MSG, sizeof (struct olsr_msg));

  memcpy (&msg->dst, dst, 4);
  msg->obuf = stream_new (bufsize);

  return msg;
}

struct olsr_msg *
olsr_msg_clone (struct olsr_msg *msg)
{
  struct olsr_msg *new = XCALLOC (MTYPE_OLSR_MSG, sizeof (struct olsr_msg));

  memcpy (&new->dst, &msg->dst, 4);

  new->obuf = stream_new (STREAM_SIZE (msg->obuf));
  stream_put (new->obuf, STREAM_DATA (msg->obuf), STREAM_SIZE (msg->obuf));

  return new;
}
/*
 * Delete message.
 */
void 
olsr_msg_free (struct olsr_msg *msg)
{
  stream_free (msg->obuf);
  XFREE (MTYPE_OLSR_FIFO, msg);
}

struct olsr_fifo*
olsr_fifo_new ()
{
  struct olsr_fifo *new;
  new = XCALLOC (MTYPE_OLSR_FIFO, sizeof (struct olsr_fifo));
  return new;
}

/*
 * Add new message to fifo.
 */
void
olsr_msg_fifo_push (struct olsr_fifo *fifo, struct olsr_msg *msg)
{
  if (fifo == NULL)
    return;

  if (fifo->tail)
    fifo->tail->next = msg;
  else
    fifo->head = msg;
  
  fifo->tail = msg;
  
  fifo->count++;
}

/*
 * Delete first message from fifo.
 */
struct olsr_msg *
olsr_fifo_pop (struct olsr_fifo *fifo)
{
  struct olsr_msg *msg;
  
  msg = fifo->head; 

  if (msg)
    { 
      fifo->head = msg->next;

      if (fifo->head == NULL)
	fifo->tail = NULL;
    }

  fifo->count--;

  return msg; 
}



void
olsr_fifo_clean (struct olsr_fifo *fifo)
{
  struct olsr_msg *msg;
  struct olsr_msg *next;

  for (msg = fifo->head; msg; msg = next)
    {
      next = msg->next;
      olsr_msg_free (msg);
    }
  fifo->head = fifo->tail = NULL;
  fifo->count = 0;
}

void
olsr_fifo_free (struct olsr_fifo *fifo)
{
  olsr_fifo_clean (fifo);
  XFREE (MTYPE_OLSR_FIFO, fifo);
}


void 
olsr_put_hello_header (struct stream *buf, struct olsr_interface *oi, 
		       u_int16_t *msize_off)
{
  stream_putc (buf, OLSR_HELLO_MSG);              /* Mtype. */
  stream_putc (buf, olsr_float_to_uchar (oi->olsr, 
					 oi->olsr->neighb_hold_time) );  /* Vtime. */
  
  /* Remember position and reserve space for message size. */
  (*msize_off) = stream_get_putp (buf);
  stream_putw (buf, 0);

  stream_put (buf, &oi->olsr->main_addr, 4);       /* Originator addr. */

  stream_putc (buf, 1);		                  /* TTL. */
  stream_putc (buf, 0);		                  /* Hop count. */
  stream_putw (buf, oi->olsr->msn ++);            /* Message sequence number. */

  stream_putw (buf, 0);	                          /* Reserverd by RFC. */
  stream_putc (buf, olsr_float_to_uchar(oi->olsr,
					OLSR_IF_HELLO (oi))) ;  /* Htime */
  stream_putc (buf, oi->olsr->will);              /* Willingness */
  
}


void 
olsr_build_and_send_hello (struct olsr_interface *oi, struct list *links)
{
  struct listnode *node;
  struct olsr_msg *msg;
  struct stream *buf;
  u_int16_t msize_off;
  u_char old_lt, old_nt;
  u_int16_t lmsize_off = 0;

  msg = olsr_msg_new (&OLSR_IF_BROADCAST (oi), oi->ifp->mtu - 4);
  buf = msg->obuf;

  olsr_put_hello_header (buf, oi, &msize_off);
  
  /* Copy links to output buffer. */
  old_lt = OLSR_LINK_IMPOSIBLE;
  old_nt = OLSR_NEIGH_IMPOSSIBLE;
  for (node = listhead (links); node; nextnode (node))
    {
      struct olsr_oi_link *ool = getdata (node);

      /* Check if we have enough free space for at least one link,
       * and if not, push this message to fifo and start a new one.
       */
      if ((ool->lt == old_lt && ool->nt == old_nt && STREAM_REMAIN (buf) < 4) ||
	  (((ool->lt != old_lt) || (ool->nt != old_nt)) && STREAM_REMAIN (buf) < 8))
	{
	  /* Fill last link message size. */
	  if (node != listhead (links))
	    stream_putw_at (buf, lmsize_off,
			    stream_get_putp(buf) - lmsize_off + 2);

	  /* Fill message size. */
	  stream_putw_at (buf, msize_off,
			  stream_get_putp(buf) - msize_off + 2);
	
	  /* Push it. */
	  olsr_msg_fifo_push (oi->fifo, msg);

	  /* Create new message. */
	  msg = olsr_msg_new (&OLSR_IF_BROADCAST (oi), oi->ifp->mtu - 4);
	  buf = msg->obuf;
	  olsr_put_hello_header (buf, oi, &msize_off);

	  old_lt = OLSR_LINK_IMPOSIBLE;
	  old_nt = OLSR_NEIGH_IMPOSSIBLE;
	}

      /* If lt or nt are new, put link message header. */
      if (ool->lt != old_lt || ool->nt != old_nt)
	{
	  /* Fill last link message size. */
	  if (node != listhead (links))
	    stream_putw_at (buf, lmsize_off,
			    stream_get_putp(buf) - lmsize_off + 2);
	  
	  old_lt = ool->lt;
	  old_nt = ool->nt;

	  stream_putc (buf, ool->nt << 2 | ool->lt);       /* Link Code. */
	  stream_putc (buf, 0);                      /* Reserved by RFC. */
	  
	  /* Remember position and reserve space for link message size. */
	  lmsize_off = stream_get_putp (buf);
	  stream_putw (buf, 0);	                           /* Reserverd. */
	}

      /* Put Neigbor Interface Address. */
      stream_put (buf, &ool->neigh_addr, 4);

      /* Free ool structure. */
      XFREE (MTYPE_OLSR_OI_LINK, ool);
    }

  /* Fill last link message size. */
  if (node != listhead (links))
    stream_putw_at (buf, lmsize_off,
		    stream_get_putp(buf) - lmsize_off + 2);

  /* Fill message size. */
  stream_putw_at (buf, msize_off,
		  stream_get_putp(buf) - msize_off + 2);
	
  /* Push it. */
  olsr_msg_fifo_push (oi->fifo, msg);

  /* Free list. */
  list_delete_all_node (links);
  list_free (links);
}


void 
olsr_put_msg_header (u_char type, u_char ttl, u_char hops, struct stream *buf, 
		     struct olsr_interface *oi, u_int16_t *msize_off)
{
  stream_putc (buf, type);              /* Mtype. */
  stream_putc (buf, olsr_float_to_uchar (oi->olsr, 
					 oi->olsr->neighb_hold_time) );  /* Vtime. */
  
  /* Remember position and reserve space for message size. */
  (*msize_off) = stream_get_putp (buf);
  stream_putw (buf, 0);
  
  stream_put (buf, &oi->olsr->main_addr, 4);       /* Originator addr. */
  
  stream_putc (buf, ttl);	                  /* TTL. */
  stream_putc (buf, hops);	                  /* Hop count. */
  stream_putw (buf, oi->olsr->msn ++);            /* Message sequence number. */
}


void 
olsr_build_and_send_mid (struct olsr_interface *oi)
{
  struct olsr_msg *msg;
  struct stream *buf;
  struct olsr_interface *oif;
  struct listnode *node;
  u_int16_t msize_off;

  msg = olsr_msg_new (&OLSR_IF_BROADCAST (oi), oi->ifp->mtu - 4);
  buf = msg->obuf;

  olsr_put_msg_header (OLSR_MID_MSG, 255, 0, buf, oi, &msize_off);


  LIST_LOOP (oi->olsr->oiflist, oif, node)
    if (memcmp (&OLSR_IF_ADDR (oif), &oi->olsr->main_addr, 4) != 0)
      {
	if (STREAM_REMAIN (buf) < 4)
	  {
	    /* Fill message size. */
	    stream_putw_at (buf, msize_off,
			    stream_get_putp(buf) - msize_off + 2);

	    /* Push it. */
	    olsr_msg_fifo_push (oi->fifo, msg);
	    
	    /* Create new message. */
	    msg = olsr_msg_new (&OLSR_IF_BROADCAST (oi), oi->ifp->mtu - 4);
	    buf = msg->obuf;
	    olsr_put_msg_header (OLSR_MID_MSG, 255, 0,  buf, oi, &msize_off);
	  }
	
	/* Put Neigbor Interface Address. */
	stream_put (buf, &OLSR_IF_ADDR (oif), 4);
      }

  /* Fill message size. */
  stream_putw_at (buf, msize_off,
		  stream_get_putp(buf) - msize_off + 2);
  
  /* Push it. */
  olsr_msg_fifo_push (oi->fifo, msg);
}


void 
olsr_build_and_send_tc (struct olsr_interface *oi)
{
  struct olsr_msg *msg;
  struct stream *buf;
  struct olsr_neigh *on;
  struct listnode *node;
  u_int16_t msize_off;

  msg = olsr_msg_new (&OLSR_IF_BROADCAST (oi), oi->ifp->mtu - 4);
  buf = msg->obuf;

  olsr_put_msg_header (OLSR_TC_MSG, 255, 0, buf, oi, &msize_off);
  stream_putw (buf, oi->olsr->ansn);
  stream_putw (buf, 0);		/* Reserverd. */

  LIST_LOOP (oi->olsr->advset, on, node)
    {
      if (STREAM_REMAIN (buf) < 4)
	  {
	    /* Fill message size. */
	    stream_putw_at (buf, msize_off,
			    stream_get_putp(buf) - msize_off + 2);

	    /* Push it. */
	    olsr_msg_fifo_push (oi->fifo, msg);
	    
	    /* Create new message. */
	    msg = olsr_msg_new (&OLSR_IF_BROADCAST (oi), oi->ifp->mtu - 4);
	    buf = msg->obuf;
	    olsr_put_msg_header (OLSR_TC_MSG, 255, 0, buf, oi, &msize_off);
	    stream_putw (buf, oi->olsr->ansn);
	    stream_forward (buf, 2);
	  }
	
	/* Put Neigbor Interface Address. */
      stream_put (buf, &on->main_addr, 4);
    }

  /* Fill message size. */
  stream_putw_at (buf, msize_off,
		  stream_get_putp(buf) - msize_off + 2);
  
  /* Push it. */
  olsr_msg_fifo_push (oi->fifo, msg);
}

void
olsr_forward (struct olsr *olsr, struct olsr_header *oh, 
	      struct stream *stream, u_int16_t msg_off)
{
  struct olsr_msg *msg;
  struct olsr_interface *oi;
  struct listnode *node;
  int nothing = 0;

  msg = olsr_msg_new ((struct in_addr*)&nothing, oh->m_size);

  stream_put (msg->obuf, STREAM_DATA (stream) + msg_off, oh->m_size);

  /*  RFC 3626 3.4.1
      If, and only if, according to step 4, the message must be
      retransmitted then:
      6    The TTL of the message is reduced by one.
  */
  stream_putc_at (msg->obuf, 8, oh->ttl - 1);
  
  /*
    7    The hop-count of the message is increased by one
  */
  stream_putc_at (msg->obuf, 9, oh->hops + 1);
  
  /*
    8    The message is broadcast on all interfaces
  */
  LIST_LOOP (olsr->oiflist, oi, node)
    {
      memcpy (&msg->dst, &OLSR_IF_BROADCAST (oi), 4);
      olsr_msg_fifo_push (oi->fifo, msg);

      msg = olsr_msg_clone (msg);

      /* Schedule write to socket. */
      THREAD_WRITE_ON (olm->master, oi->t_write, olsr_write, oi,
		       oi->sock);
    }
}


/* 
 * Write output packet.
 */
int 
olsr_write (struct thread *thread)
{
  struct olsr_interface *oi;
  struct stream *buf;
  struct olsr_msg *msg;
  struct sockaddr_in addr;
  
  oi = THREAD_ARG (thread);
  oi->t_write = NULL;
  
  if (!oi->fifo || oi->fifo->count <= 0)
    return 0;
  
  buf = stream_new (oi->ifp->mtu);
  
  /* Reserve space for packet length. Address is 0. */
  stream_put_forward (buf, 2);
  stream_putw (buf, oi->psn ++);         /* Packet Sequence Number. */
  
  
  /* Take message from queue and copy to buffer. */
  msg = olsr_fifo_pop (oi->fifo);

  assert (msg);
  
  stream_put (buf, STREAM_DATA (msg->obuf), stream_get_putp (msg->obuf));

  
  /* Try to get more messages in this packet if possible..  */
  while (oi->fifo->count > 0 && 
	 OLSR_MSG_SIZE (OLSR_FIFO_HEAD (oi->fifo)) >= STREAM_REMAIN (buf) &&
	 memcmp (&oi->fifo->head->dst, &msg->dst, 4) == 0)
    {
      olsr_msg_free (msg);
      msg = olsr_fifo_pop (oi->fifo);

      assert (msg);
  
      stream_put (buf, STREAM_DATA (msg->obuf), stream_get_putp (msg->obuf));
    }


  /* Fill in packet length. */
  stream_putw_at (buf, 0, stream_get_putp (buf));

  /* Send packet. */
  addr.sin_family = AF_INET;
  memcpy (&addr.sin_addr, &msg->dst, 4);
  addr.sin_port = htons (oi->olsr->port);

  if (sendto (oi->sock, STREAM_DATA (buf), stream_get_putp (buf), 0,
	      (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) < 0) 
    {
      zlog_err ("olsr_write: sendto: %s", safe_strerror (errno));
      exit (1);
    }

  olsr_msg_free (msg);
  stream_free (buf);

  /* If queue not empty, reschedule. */
  if (oi->fifo->count > 0)
    THREAD_WRITE_ON (olm->master, oi->t_write, olsr_write, oi, oi->sock);

  return 0;
}

int 
olsr_hello_timer(struct thread *thread)
{
  struct olsr_interface *oi;
  struct list *list;

  oi = THREAD_ARG (thread);
  oi->t_hello = NULL;

  /* Rearm timer. */
  OLSR_TIMER_ON (oi->t_hello, olsr_hello_timer, oi, OLSR_IF_HELLO (oi));

  list = olsr_build_oi_links (oi);

  /* Build hello message (s) and push them to output fifo. */
  olsr_build_and_send_hello (oi, list);
  
  /* Schedule write to socket. */
  THREAD_WRITE_ON (olm->master, oi->t_write, olsr_write, oi, oi->sock);
  
  return 0;
}


int 
olsr_mid_timer (struct thread *thread)
{
  struct olsr_interface *oi;

  oi = THREAD_ARG (thread);
  oi->t_mid = NULL;

  /* Build mid message(s) and push them to output fifo. */
  olsr_build_and_send_mid (oi);

  /* Schedule write to socket. */
  THREAD_WRITE_ON (olm->master, oi->t_write, olsr_write, oi, oi->sock);
  
  /* Rearm timer. */
  OLSR_TIMER_ON (oi->t_mid, olsr_mid_timer, oi, OLSR_IF_MID (oi));

  return 0;
}

int 
olsr_tc_timer (struct thread *thread)
{
  struct olsr_interface *oi;

  oi = THREAD_ARG (thread);
  oi->t_tc = NULL;

  /* Build tc message(s) and push them to output fifo. */
  olsr_build_and_send_tc (oi);

  /* Schedule write to socket. */
  THREAD_WRITE_ON (olm->master, oi->t_write, olsr_write, oi, oi->sock);
  
  /* Rearm timer. */
  OLSR_TIMER_ON (oi->t_tc, olsr_tc_timer, oi, OLSR_IF_TC(oi));

  return 0;
}
  
