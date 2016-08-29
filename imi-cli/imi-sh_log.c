/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
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
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include <zebra.h>
#include "linklist.h"
#include "thread.h"
#include "buffer.h"
#include "stream.h"
#include "sockunion.h"
#include "str.h"
#include "log.h"
#include "vty.h"
#include "memory.h"
#include "vector.h"
#include "privs.h"
#include "network.h"


#ifdef IMISH_IMI_MODULE
#include "imi-cli/imi-sh.h"
#include "imi-cli/imi-sh_log.h"

/************************************************************************************/
extern vector vtyvec;
static struct imishlog *imish_log = NULL;
static int imish_log_socket_init(struct imishlog *imish_log, int port);
/************************************************************************************/
static int imish_log_out (struct vty *vty, const char *format, int len)
{
  if (write(vty->fd, format, len) < 0)
    {
      if (ERRNO_IO_RETRY(errno))
				return -1;
      vty->monitor = 0; /* disable monitoring to avoid infinite recursion */
      zlog_warn("%s: write failed to vty client fd %d, closing: %s",
				__func__, vty->fd, safe_strerror(errno));
      buffer_reset(vty->obuf);
      vty->status = VTY_CLOSE;
      shutdown(vty->fd, SHUT_RDWR);
      return -1;
    }
  return 0;
}
static int imish_log_format (const char *format,...)
{
  unsigned int i;
  struct vty *vty;
  va_list args;
  int len = 0;
  char buf[2048];
  if (!vtyvec)
    return 0;
    
  memset(buf, 0, sizeof(buf));  
  va_start (args, format);
  len = vsnprintf (buf, sizeof(buf), format, args);
  va_end (args);
      
  for (i = 0; i < vector_active (vtyvec); i++)
    if ((vty = vector_slot (vtyvec, i)) != NULL)
      if (vty->monitor)
		{
	  imish_log_out(vty, buf, len);
		}
	return 0;	
}
/************************************************************************************/
/************************************************************************************/
static int imish_log_server_read_thread(struct thread *thread)
{
#if 0

#else
	int lenth = -1;
	int sock;
	char buf[2048];
	struct imishlog *imish_log;
  sock = THREAD_FD (thread);
  imish_log = THREAD_ARG (thread);
  memset(buf, 0, sizeof(buf));
  lenth = read(sock, buf, sizeof(buf));
  if(lenth <= 0)
  {
      //imish_log->t_read = thread_add_read (master, imish_log_server_read_thread, imish_log, sock);
      if( (errno == EINTR)|| (errno == EAGAIN))
    	  return -1; /* signal received - process it */
      if(errno == EPIPE)  
      {
        close(sock);
        imish_log->sock = 0;
        imish_log_socket_init(imish_log, IMISH_IMI_MODULE);
        return -1;
      }
      return -1;
  }
  imish_log_format(buf);
  imish_log->t_read = thread_add_read (master, imish_log_server_read_thread, imish_log, sock);	
  return 0;
#endif   
}
/************************************************************************************/
/************************************************************************************/
static int imish_log_socket_init(struct imishlog *imish_log, int port)
{
  int sock = -1,ret = -1;
  struct sockaddr_in addr;
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
  {
    zlog_err(" IMISH log socket init(%s)",safe_strerror (errno));
    return -1;
  }  
  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
  addr.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  
  ret  = bind (sock, (struct sockaddr *)&addr, sizeof (struct sockaddr_in));
  if (ret < 0)
  {
  	zlog_err(" IMISH log socket bind port:%d(%s)",port,safe_strerror (errno));
    close (sock); 
    return -1;
  } 
  if(setsockopt_so_recvbuf (sock, 8192) < 0)
  {
    close (sock);
    zlog_err(" IMISH log socket bind port:%d(%s)",port,safe_strerror (errno));
    return -1;
  }
  if(setsockopt_so_sendbuf (sock, 8192) < 0)
  {
    close (sock);
    zlog_err(" IMISH log socket setsockopt_so_sendbuf:(%s)",safe_strerror (errno));
    return -1;
  }
  if (set_nonblocking(sock) < 0)
  {
    close (sock);
    zlog_err(" IMISH log socket set_nonblocking(%s)",safe_strerror (errno));
    return -1;
  }  
  imish_log->sock = sock;
  imish_log->t_read = thread_add_read (master, imish_log_server_read_thread, imish_log, sock);
  return sock;		
}
/************************************************************************************/
/************************************************************************************/
int imish_log_server_init(void)
{
  imish_log = XCALLOC (MTYPE_ZLOG, sizeof (struct imishlog));
  memset(imish_log, 0, sizeof(struct imishlog));
  imish_log_socket_init(imish_log, IMISH_IMI_MODULE);
  return 0;
} 
/************************************************************************************/
/************************************************************************************/
int imish_log_server_exit(void)
{
  if(imish_log->t_read)
    thread_cancel(imish_log->t_read);
  if(imish_log->sock > 0)
    close(imish_log->sock);
  if(imish_log) 
    XFREE(MTYPE_ZLOG, imish_log);
  imish_log = NULL;
  return 0;
} 

#endif /* IMISH_IMI_MODULE */
