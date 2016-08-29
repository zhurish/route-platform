#ifndef _FPM_SOCKET_H
#define _FPM_SOCKET_H

/* nsm server structure. */
struct fpm_server
{
  /* The thread master we schedule ourselves on */	
  struct thread_master *master;	
  /* Client file descriptor. */
  int sock;
  int fpm_port;
  /* Input/output buffer to the client. */
  struct stream *ibuf;
  struct stream *obuf;

  /* Buffer of data waiting to be written to client. */
  struct buffer *wb;

  /* Threads for read/write. */
  struct thread *t_read;
  struct thread *t_write;

  /* Thread for delayed close. */
  struct thread *t_suicide;
};
struct fpm_route_table
{
	unsigned int family;
	struct in_addr *dest;
	struct in_addr *gate;
	unsigned int masklen;		
	struct in_addr *src;
	unsigned int protocol;	
	unsigned int table;			
	unsigned int metric;	
	unsigned int flags;	
	unsigned int ifindex;	
	unsigned int mtu;			
};

extern struct fpm_server *fpm_server;
extern void fpm_server_init ( struct thread_master *m, int port);

#endif /* _FPM_SOCKET_H */