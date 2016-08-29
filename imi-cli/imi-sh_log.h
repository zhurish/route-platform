#ifndef _IMISH_LOG_H
#define _IMISH_LOG_H

#ifdef IMISH_IMI_MODULE

struct imishlog
{
  int sock;
  struct stream *ibuf;
  struct thread *t_read;
  u_char proto; 
};

extern int imish_log_server_init(void);
extern int imish_log_server_exit(void);

#endif /* IMISH_IMI_MODULE */

#endif/* _IMISH_LOG_H */