/* Zebra daemon server routine.
 * Copyright (C) 1997, 98, 99 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
 * Boston, MA 02111-1307, USA.  
 */

#include <zebra.h>
#include "prefix.h"
#include "command.h"
#include "if.h"
#include "thread.h"
#include "stream.h"
#include "memory.h"
#include "table.h"
#include "network.h"
#include "sockunion.h"
#include "log.h"
#include "zclient.h"
#include "privs.h"
#include "network.h"
#include "buffer.h"
#include "zebra/rib.h"

#include "fpm/fpm.h"
#include "zebra/zebra_fpm.h"
#include "zebra/zebra_fpm_private.h"

#include "fpm/fpm_socket.h"
/***************************************************************************/
#define FPM_ROUTE_DEBUG
/***************************************************************************/
enum fpm_event { FPM_SERV, FPM_READ, FPM_WRITE };
/***************************************************************************/
struct fpm_server *fpm_server;
//extern struct thread_master *master;
/***************************************************************************/
static void fpm_event (enum fpm_event event, int sock, struct fpm_server *fpm);
static void fpm_client_close (struct fpm_server *fpm);
/***************************************************************************/
/***************************************************************************/
static int fpm_delayed_close(struct thread *thread)
{
  struct fpm_server *fpm = THREAD_ARG(thread);
  fpm->t_suicide = NULL;
  fpm_client_close(fpm);
  return 0;
}

static int fpm_flush_data(struct thread *thread)
{
  struct fpm_server *fpm = THREAD_ARG(thread);

  fpm->t_write = NULL;
  if (fpm->t_suicide)
  {
      fpm_client_close(fpm);
      return -1;
  }
  switch (buffer_flush_available(fpm->wb, fpm->sock))
  {
    case BUFFER_ERROR:
      printf("%s: buffer_flush_available failed on zserv client fd %d\n, ""closing", __func__, fpm->sock);
      fpm_client_close(fpm);
      break;
    case BUFFER_PENDING:
      fpm->t_write = thread_add_write(fpm_server->master, fpm_flush_data, fpm, fpm->sock);
      break;
    case BUFFER_EMPTY:
      break;
  }
  return 0;
}

static int fpm_server_send_message(struct fpm_server *fpm)
{
  if (fpm->t_suicide)
    return -1;
  switch (buffer_write(fpm->wb, fpm->sock, STREAM_DATA(fpm->obuf),stream_get_endp(fpm->obuf)))
  {
    case BUFFER_ERROR:
      printf("%s: buffer_write failed to zserv client fd %d\n, closing",__func__, fpm->sock);
      fpm->t_suicide = thread_add_event(fpm_server->master, fpm_delayed_close, fpm, 0);
      return -1;
    case BUFFER_EMPTY:
      THREAD_OFF(fpm->t_write);
      break;
    case BUFFER_PENDING:
      THREAD_WRITE_ON(fpm_server->master, fpm->t_write,fpm_flush_data, fpm, fpm->sock);
      break;
  }
  return 0;
}

/* Close zebra client. */
static void fpm_client_close (struct fpm_server *fpm)
{
  /* Close file descriptor. */
  if (fpm->sock)
  {
      close (fpm->sock);
      fpm->sock = -1;
  }
  /* Free stream buffers. */
  if (fpm->ibuf)
    stream_free (fpm->ibuf);
  if (fpm->obuf)
    stream_free (fpm->obuf);
  if (fpm->wb)
    buffer_free(fpm->wb);
  /* Release threads. */
  if (fpm->t_read)
    thread_cancel (fpm->t_read);
  if (fpm->t_write)
    thread_cancel (fpm->t_write);
  if (fpm->t_suicide)
    thread_cancel (fpm->t_suicide);
  /* Free client structure. */
  XFREE (0, fpm);
  fpm = NULL;
}
#ifdef HAVE_NETLINK
static void fpm_netlink_parse_rtattr (struct rtattr **tb, int max, struct rtattr *rta, int len)
{
  while (RTA_OK (rta, len))
    {
      if (rta->rta_type <= max)
        tb[rta->rta_type] = rta;
      rta = RTA_NEXT (rta, len);
    }
}
#endif /*HAVE_NETLINK*/
static void fpm_netlink_route_entry (int cmd, struct fpm_route_table *table)
{
	char ipstr[128];
	//inet_ntop(PF_INET6,&rcv_udp_addr.sin6_addr,ip,sizeof(ip))
	if( table->family == AF_INET)
		printf("inte:");
	else if( table->family == AF_INET6)
		printf("inet6:");
	else
		printf("unknow:");	
	if(table->dest)
	{
		inet_ntop(table->family,(table->dest),ipstr,sizeof(ipstr));
		//if( table->family == AF_INET)
		//	printf("%s",inet_ntoa(*(table->dest)));
		printf("%s/%d",ipstr,table->masklen);
	}
	else
		printf(" ");
	
	if(table->gate)
	{
		inet_ntop(table->family,(table->dest),ipstr,sizeof(ipstr));
		printf("gw %s",ipstr);
		//if( table->family == AF_INET)
		//	printf(" gw %s ",inet_ntoa(*(table->gate)));
		//if( table->family == AF_INET6)
		//	printf("gw %s",inet6_ntoa(*(table->gate)));
	}
	else
		printf(" ");
	if(table->src)
	{
		inet_ntop(table->family,(table->dest),ipstr,sizeof(ipstr));
		printf("src %s",ipstr);
		//if( table->family == AF_INET)
		//	printf("src %s ",inet_ntoa(*(table->src)));
		//if( table->family == AF_INET6)
		//	printf("src %s ",inet6_ntoa(*(table->src)));
	}
	else
		printf(" ");
	printf("0x%x %d metric %d 0x%x if %d \n",table->protocol,table->table,table->metric,table->flags,table->ifindex);
	return 0;
}
#ifdef HAVE_NETLINK
static int fpm_netlink_routing_table (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len = 0;
  struct rtmsg *rtm = NULL;
  struct rtattr *tb[RTA_MAX + 1];
  char anyaddr[16] = { 0 };
	struct fpm_route_table table_entry;
  rtm = NLMSG_DATA (h);
/*
  if (h->nlmsg_type != RTM_NEWROUTE)
    return 0;
  if (rtm->rtm_type != RTN_UNICAST)
    return 0;
*/
  len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct rtmsg));
  if (len < 0)
  {
  	printf("%s:len < 0\n",__func__);
    return -1;
  }
  memset (tb, 0, sizeof tb);
  
  fpm_netlink_parse_rtattr (tb, RTA_MAX, RTM_RTA (rtm), len);
/*
  if (rtm->rtm_flags & RTM_F_CLONED)
    return 0;
  if (rtm->rtm_protocol == RTPROT_REDIRECT)
    return 0;
  if (rtm->rtm_protocol == RTPROT_KERNEL)
    return 0;
*/
  if (rtm->rtm_src_len != 0)
  {
   	printf("%s:rtm->rtm_src_len != 0 :%d\n",__func__,rtm->rtm_src_len); 
    return 0;
	}
	memset (&table_entry, 0, sizeof table_entry);
  table_entry.family = rtm->rtm_family;
  table_entry.table = rtm->rtm_table;
  table_entry.masklen = rtm->rtm_dst_len;
  	
  /* Route which inserted by Zebra. */
  if (rtm->rtm_protocol == RTPROT_ZEBRA)
    table_entry.flags |= ZEBRA_FLAG_SELFROUTE;

  if (tb[RTA_OIF])
    table_entry.ifindex = *(int *) RTA_DATA (tb[RTA_OIF]);
  if (tb[RTA_DST])
    table_entry.dest = RTA_DATA (tb[RTA_DST]);
  else
    table_entry.dest = anyaddr;
  if (tb[RTA_PREFSRC])
    table_entry.src = RTA_DATA (tb[RTA_PREFSRC]);
  if (tb[RTA_GATEWAY])
    table_entry.gate = RTA_DATA (tb[RTA_GATEWAY]);
    
  if (tb[RTA_METRICS])
    {
      struct rtattr *mxrta[RTAX_MAX+1];
      memset (mxrta, 0, sizeof mxrta);
      fpm_netlink_parse_rtattr (mxrta, RTAX_MAX, RTA_DATA(tb[RTA_METRICS]),RTA_PAYLOAD(tb[RTA_METRICS]));
      if (mxrta[RTAX_MTU])
        table_entry.mtu = *(u_int32_t *) RTA_DATA(mxrta[RTAX_MTU]);
    }
  if (rtm->rtm_family == AF_INET)
    {
    	table_entry.family = AF_INET;
      if (!tb[RTA_MULTIPATH])
      {
        fpm_netlink_route_entry (h->nlmsg_type, &table_entry);
      }
      else
        {
          /* This is a multipath route */
          struct rtnexthop *rtnh = (struct rtnexthop *) RTA_DATA (tb[RTA_MULTIPATH]);
          len = RTA_PAYLOAD (tb[RTA_MULTIPATH]);

          for (;;)
            {
              if (len < (int) sizeof (*rtnh) || rtnh->rtnh_len > len)
                break;
              table_entry.ifindex = rtnh->rtnh_ifindex;
              table_entry.gate = NULL;
              if (rtnh->rtnh_len > sizeof (*rtnh))
                {
                  memset (tb, 0, sizeof (tb));
                  fpm_netlink_parse_rtattr (tb, RTA_MAX, RTNH_DATA (rtnh),
                                        rtnh->rtnh_len - sizeof (*rtnh));
                  if (tb[RTA_GATEWAY])
                    table_entry.gate = RTA_DATA (tb[RTA_GATEWAY]);
                }
              if (table_entry.gate)
                {
                  fpm_netlink_route_entry (h->nlmsg_type, &table_entry);
                }
              else
               fpm_netlink_route_entry (h->nlmsg_type, &table_entry);

              len -= NLMSG_ALIGN(rtnh->rtnh_len);
              rtnh = RTNH_NEXT(rtnh);
            }
          //if (rib->nexthop_num == 0)
            //XFREE (MTYPE_RIB, rib);
          //else
            //rib_add_ipv4_multipath (&p, rib, SAFI_UNICAST);
        }
    }
#ifdef HAVE_IPV6
  if (rtm->rtm_family == AF_INET6)
    {
    	table_entry.family = AF_INET6;
    	fpm_netlink_route_entry (h->nlmsg_type, &table_entry);
    }
#endif /* HAVE_IPV6 */
  return 0;
}
#ifdef FPM_ROUTE_DEBUG
static int fpm_msg_debug(struct stream *ibuf)
{
  int i;
  unsigned char *p;
  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[1];
  }req;
  fpm_msg_hdr_t hdr;
  stream_set_getp(ibuf, 0);
  /* Fetch header values OK */
  hdr.version = stream_getc (ibuf);//FPM_PROTO_VERSION;
  hdr.msg_type = stream_getc (ibuf);//FPM_MSG_TYPE_NETLINK;
  hdr.msg_len = stream_getw (ibuf);
  
  netlink_routing_table (NULL, (struct nlmsghdr *)(ibuf->data+4), 0);
  //command = stream_getw (fpm->ibuf);
  req.n.nlmsg_len = ntohl(stream_getl (ibuf));//NLMSG_LENGTH (sizeof (struct rtmsg));OK
  req.n.nlmsg_type = ntohs(stream_getw (ibuf));//ri->nlmsg_type;�������� RTM_NEWROUTE : RTM_DELROUTE
  req.n.nlmsg_flags = ntohs(stream_getw (ibuf));//NLM_F_CREATE | NLM_F_REQUEST; OK
  req.n.nlmsg_seq = ntohl(stream_getl (ibuf));
  req.n.nlmsg_pid = ntohl(stream_getl (ibuf));
  
  req.r.rtm_family = stream_getc (ibuf);// OK
  req.r.rtm_dst_len = stream_getc (ibuf);// OK
  req.r.rtm_src_len = stream_getc (ibuf);// OK
  req.r.rtm_tos = stream_getc (ibuf);// OK
  req.r.rtm_table = stream_getc (ibuf);//ri->rtm_table; OK
  req.r.rtm_protocol = stream_getc (ibuf);// OK
  req.r.rtm_scope = stream_getc (ibuf);// OK
  req.r.rtm_type = stream_getc (ibuf);//RT_SCOPE_UNIVERSE; OK
  req.r.rtm_flags = stream_getl (ibuf);//RT_SCOPE_UNIVERSE; OK
  /*
  size_t len;
  struct rtattr *rta;

  len = RTA_LENGTH (alen);

  if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr *) (((char *) n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA (rta), data, alen);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  
  ipv4
0x01 0x01  0x00 0x40  0x3c 0x00 0x00 0x00  0x18 0x00  0x01 0x04  0x00 0x00 0x00 0x00 
0x00  0x00  0x00  0x00  0x02  0x18  0x00  0x00  0x00  0x0b  0x00  0x01  0x00 0x00 0x00 0x00 
route:
0x08 0x00  
0x01  
0x00 0xc0 0xc0 0xc0  
0x00 0x08 0x00 0x06 
0x00 0x00 0x00 0x00 
0x00 0x08 0x00 0x05 
0x00 
0xc0 0xa8 0x01 0x64 OK
0x08 0x00 0x04 0x00 0x02 0x00 0x00 0x00 

ipv6
0x01 0x01 0x00 0x44 0x40 0x00 0x00 0x00 0x18 0x00 0x01 0x04 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x0a 0x40 0x00 0x00 0x00 0x02 0x00 0x01 0x00 0x00 0x00 0x00 
route:
0x14 0x00 0x01 0x00 0xfe 0x80 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 
0x00 0x00 0x00 0x00 0x08 0x00 0x06 0x00 0x00 0x00 0x00 0x00 0x08 0x00 0x04 0x00 
0x02 0x00 0x00 0x00
  */
  printf("hdr.version:0x%x\n",hdr.version);  
  printf("hdr.msg_type:0x%x\n",hdr.msg_type);
  printf("hdr.msg_len:%d\n",hdr.msg_len);
  
  printf("req.n.nlmsg_len:%d\n",req.n.nlmsg_len);
  printf("req.n.nlmsg_type:0x%x\n",req.n.nlmsg_type);
  printf("req.n.nlmsg_flags:0x%x\n",req.n.nlmsg_flags);
  printf("req.n.nlmsg_seq:%d\n",req.n.nlmsg_seq);
  printf("req.n.nlmsg_pid:%d\n",req.n.nlmsg_pid);
  
  printf("req.r.rtm_family:0x%x\n",req.r.rtm_family);
  printf("req.r.rtm_dst_len:%d\n",req.r.rtm_dst_len);
  printf("req.r.rtm_src_len:%d\n",req.r.rtm_src_len);
  printf("req.r.rtm_tos:0x%x\n",req.r.rtm_tos);
  printf("req.r.rtm_table:0x%x\n",req.r.rtm_table);
  
  printf("req.r.rtm_protocol:0x%x\n",req.r.rtm_protocol);
  printf("req.r.rtm_scope:0x%x\n",req.r.rtm_scope);
  printf("req.r.rtm_type:0x%x\n",req.r.rtm_type);
  printf("req.r.rtm_flags:0x%x\n",req.r.rtm_flags);
  p =  (unsigned char *)ibuf->data;
  for(i = 0; i < hdr.msg_len; i++)
  {
    printf("0x%02x ",(unsigned int)p[i]);
    if((i+1)%16==0)
      printf("\n");
  }
  printf("\n");
}
#endif /* FPM_ROUTE_DEBUG */
#endif /*HAVE_NETLINK*/
/* Handler of zebra service request. */
int fpm_client_read (struct thread *thread)
{
  int sock;
  size_t already;
  uint16_t length;
  fpm_msg_hdr_t hdr;
  struct fpm_server *fpm;
  
  /* Get thread data.  Reset reading thread because I'm running. */
  sock = THREAD_FD (thread);
  fpm = THREAD_ARG (thread);
  fpm->t_read = NULL;

  if (fpm->t_suicide)
  {
      fpm_client_close(fpm);
      return -1;
  }
  /* Read length and command (if we don't have it already). */
  if ((already = stream_get_endp(fpm->ibuf)) < FPM_MSG_ALIGNTO)
  {
    ssize_t nbyte;
    if (((nbyte = stream_read_try (fpm->ibuf, sock, FPM_MSG_ALIGNTO-already)) == 0) ||(nbyte == -1))
		{
	  	fpm_client_close (fpm);
	  	return -1;
		}
    if (nbyte != (ssize_t)(FPM_MSG_ALIGNTO-already))
		{
	  	fpm_event (FPM_READ, sock, fpm);
	  	return 0;
		}
    already = FPM_MSG_ALIGNTO;
  }
  /* Reset to read from the beginning of the incoming packet. */
  stream_set_getp(fpm->ibuf, 0);
  /* Fetch header values */
  hdr.version = stream_getc (fpm->ibuf);//FPM_PROTO_VERSION;
  hdr.msg_type = stream_getc (fpm->ibuf);//FPM_MSG_TYPE_NETLINK;
  length = hdr.msg_len = stream_getw (fpm->ibuf);
  
  //if(!fpm_msg_ok (fpm->ibuf->data, stream_get_endp(fpm->ibuf)))
  if (hdr.version != FPM_PROTO_VERSION || hdr.msg_type != FPM_MSG_TYPE_NETLINK)
  {
      printf("%s: socket %d version mismatch, version %d, msg type %d\n",
               __func__, sock, hdr.version, hdr.msg_type);
      fpm_client_close (fpm);
      return -1;
  }
  if (length < FPM_MSG_ALIGNTO) 
  {
      printf("%s: socket %d message length %u is less than header size %d\n",
	        __func__, sock, length, FPM_MSG_ALIGNTO);
      fpm_client_close (fpm);
      return -1;
  }
  if (length > STREAM_SIZE(fpm->ibuf))
  {
      printf("%s: socket %d message length %u exceeds buffer size %lu\n",
	        __func__, sock, length, (u_long)STREAM_SIZE(fpm->ibuf));
      fpm_client_close (fpm);
      return -1;
  }
  /* Read rest of data. */
  if (already < length)
  {
      ssize_t nbyte;
      if (((nbyte = stream_read_try (fpm->ibuf, sock, length-already)) == 0) ||(nbyte == -1))
			{
	  		fpm_client_close (fpm);
	  		return -1;
			}
      if (nbyte != (ssize_t)(length-already))
      {
	  		/* Try again later. */
	  		fpm_event (FPM_READ, sock, fpm);
	  		return 0;
			}
  }
#ifdef HAVE_NETLINK  
	fpm_netlink_routing_table(NULL, (struct nlmsghdr *)(fpm->ibuf->data+4));  
#ifdef FPM_ROUTE_DEBUG
//  fpm_msg_debug(fpm->ibuf);
#endif /* FPM_ROUTE_DEBUG */
#endif /*HAVE_NETLINK*/
  if (fpm->t_suicide)
  {
      fpm_client_close(fpm);
      return -1;
  }
  stream_reset (fpm->ibuf);
  fpm_event (FPM_READ, sock, fpm);
  return 0;
}


/* Accept code of zebra server socket. */
static int fpm_accept (struct thread *thread)
{
  int sock;
  int accept_sock;  
  socklen_t len;
  struct sockaddr_in client;
  struct fpm_server *fpm;
  fpm = THREAD_ARG (thread);
  accept_sock = THREAD_FD (thread);
  /* Reregister myself. */
  fpm_event (FPM_SERV, accept_sock, fpm);
  len = sizeof (struct sockaddr_in);
  sock = accept (accept_sock, (struct sockaddr *) &client, &len);
  if (sock < 0)
  {
      printf ("Can't accept zebra socket: %s\n", safe_strerror (errno));
      return -1;
  }
  /* Make client socket non-blocking.  */
  set_nonblocking(sock);
  fpm->sock = sock;
  fpm_event (FPM_READ, sock, fpm);
  return 0;
}

static void fpm_serv_socket_init (struct fpm_server *fpm)
{
  int ret;
  int accept_sock;
  struct sockaddr_in addr;
  accept_sock = socket (AF_INET, SOCK_STREAM, 0);
  if (accept_sock < 0) 
  {
      zlog_warn ("Can't create zserv stream socket: %s", safe_strerror (errno));
      zlog_warn ("zebra can't provice full functionality due to above error");
      return;
  }
  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (fpm->fpm_port);
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
  addr.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
  addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  sockopt_reuseaddr (accept_sock);
  sockopt_reuseport (accept_sock);

  ret  = bind (accept_sock, (struct sockaddr *)&addr, sizeof (struct sockaddr_in));
  if (ret < 0)
  {
      zlog_warn ("Can't bind to stream socket: %s", safe_strerror (errno));
      zlog_warn ("zebra can't provice full functionality due to above error");
      close (accept_sock);      /* Avoid sd leak. */
      return;
  }
  ret = listen (accept_sock, 1);
  if (ret < 0)
  {
      zlog_warn ("Can't listen to stream socket: %s", safe_strerror (errno));
      zlog_warn ("zebra can't provice full functionality due to above error");
      close (accept_sock);	/* Avoid sd leak. */
      return;
  }
  fpm_event (FPM_SERV, accept_sock, fpm);
}

static void fpm_event (enum fpm_event event, int sock, struct fpm_server *fpm)
{
  switch (event)
  {
    case FPM_SERV:
      thread_add_read (fpm_server->master, fpm_accept, fpm, sock);
      break;
    case FPM_READ:
      fpm->t_read = thread_add_read (fpm_server->master, fpm_client_read, fpm, sock);
      break;
    case FPM_WRITE:
      /**/
      break;
  }
}

/* Make zebra server socket, wiping any existing one (see bug #403). */
void fpm_server_init ( struct thread_master *m, int port)
{
  fpm_server = XCALLOC (0, sizeof (struct fpm_server));
  /* Make client input/output buffer. */
  fpm_server->ibuf = stream_new (ZEBRA_MAX_PACKET_SIZ);
  fpm_server->obuf = stream_new (ZEBRA_MAX_PACKET_SIZ);
  fpm_server->wb = buffer_new(0);
  /* Make new read thread. */
  fpm_server->fpm_port = port;
  fpm_server->master = m;
  fpm_serv_socket_init (fpm_server);
}
