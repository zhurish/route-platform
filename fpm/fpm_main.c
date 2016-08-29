#include <zebra.h>
#include <lib/version.h>
#include "getopt.h"
#include "thread.h"
#include "prefix.h"
#include "linklist.h"
#include "if.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "filter.h"
#include "plist.h"
#include "stream.h"
#include "log.h"
#include "memory.h"
#include "privs.h"
#include "sigevent.h"

#include "fpm/fpm.h"
#include "fpm/fpm_socket.h"



/*
 * GLOBALS
 */

/* Master of threads. */
extern struct thread_master *master;




/* SIGHUP handler. */
void 
sighup (int sig)
{
  printf ("SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  printf ("Terminating on signal");
  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  printf ("SIGUSR1 on signal");
}


#define RETSIGTYPE void
/* Signal wrapper. */
RETSIGTYPE *
signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);

  if (ret < 0) 
    return (SIG_ERR);
  else
    return (osig.sa_handler);
}


/* Initialization of signal handles. */
void fpm_signal_init ()
{
  signal_set (SIGHUP, sighup);
  signal_set (SIGINT, sigint);
  signal_set (SIGTERM, sigint);
  signal_set (SIGPIPE, SIG_IGN);
#ifdef SIGTSTP
  signal_set (SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
  signal_set (SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
  signal_set (SIGTTOU, SIG_IGN);
#endif
  signal_set (SIGUSR1, sigusr1);
}


int main(int argc, char **argv, char **envp)
{
  char *progname;
  char *p;
  struct thread thread;

  /* Set umask before anything for security */
  umask (0027);

  /* Get program name. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Check if invoked by root. */
  if (geteuid() != 0)
    {
      errno = EPERM;
      perror(progname);
      exit(1);
    }

  master = thread_master_create ();
  
  fpm_signal_init();
  fpm_server_init (master, 2620);

  /* Fetch next active thread. */
  while (thread_fetch (master, &thread))
    thread_call (&thread);
  
  /* Not reached. */
  exit (0);
}
