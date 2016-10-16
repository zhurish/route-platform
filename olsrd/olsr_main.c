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

#include "olsrd/olsrd.h"
#include "olsrd/olsrd_version.h"
#include "olsrd/olsr_zebra.h"
#include "olsrd/olsr_interface.h"
#include "olsrd/olsr_vty.h"
#include "olsrd/olsr_debug.h"
#include "olsrd/olsr_neigh.h"
#include "olsrd/olsr_route.h"


/*
 * GLOBALS
 */


/* Configuration filename and directory. */
char config_default[] = SYSCONFDIR OLSR_DEFAULT_CONFIG;

/* Process ID saved for use by init system */
const char *pid_file = PATH_OLSRD_PID;

/* Master of threads. */
struct thread_master *master;

/* OLSRDd options. */
struct option longopts[] = 
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "pid_file",    required_argument, NULL, 'i'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_addr",    required_argument, NULL, 'A'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "user",        required_argument, NULL, 'u'},
  { "group",       required_argument, NULL, 'g'},
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};



/* Help information display. */
static void
usage (char *progname, int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {    
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages OLSR routing protocol.\n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-i, --pid_file     Set process identifier file name\n\
-A, --vty_addr     Set vty's bind address\n\
-P, --vty_port     Set vty's port number\n\
-u, --user         User to run as\n\
-g, --group        Group to run as\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
\n\
Please report OLSR related bugs to %s and Quagga related bugs to %s.\n", progname, 
	      OLSRD_BUG_ADDRESS, ZEBRA_BUG_ADDRESS);
    }
  exit (status);
}


void olsr_print_version()
{
  printf("%s version %s\n", OLSRD_PROGNAME, OLSRD_VERSION);
  printf("%s\n", OLSRD_COPYRIGHT);
}


/* OLSRd privileges */
zebra_capabilities_t _caps_p [] = 
{
/* 2016��7��3�� 15:34:05 zhurish: �޸�OLSR·��Э��ʹ�ó����û�Ȩ�޶��� */
  ZCAP_NET_RAW,
  ZCAP_BIND,
//  ZCAP_BROADCAST,
  ZCAP_NET_ADMIN,
  /*	
  ZCAP_RAW,
  ZCAP_BIND,
  ZCAP_BROADCAST,
  ZCAP_ADMIN,
  */
/* 2016��7��3�� 15:34:05  zhurish: �޸�OLSR·��Э��ʹ�ó����û�Ȩ�޶��� */
};

struct zebra_privs_t olsrd_privs =
{
#if defined(QUAGGA_USER) && defined(QUAGGA_GROUP)
  .user = QUAGGA_USER,
  .group = QUAGGA_GROUP,
#endif
#if defined(VTY_GROUP)
  .vty_group = VTY_GROUP,
#endif
  .caps_p = _caps_p,
  .cap_num_p = sizeof(_caps_p)/sizeof(_caps_p[0]),
  .cap_num_i = 0
};


/* SIGHUP handler. */
void 
sighup (int sig)
{
  zlog (NULL, LOG_INFO, "SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  zlog (NULL, LOG_INFO, "Terminating on signal");

  olsr_terminate ();
  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  zlog_rotate (NULL);
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
void
olsr_signal_init ()
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
  char *p;
  char *vty_addr = NULL;
  int vty_port = OLSR_VTY_PORT;
  int daemon_mode = 0;
  char *config_file = NULL;
  char *progname;
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

  zlog_default = openzlog (OLSRD_PROGNAME, ZLOG_OLSR,
			   LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);
  
  
  /* OLSR master init. */
  olsr_master_init();

  while (1) 
    {
      int opt;

      opt = getopt_long (argc, argv, "df:i:hA:P:u:g:v", longopts, 0);
    
      if (opt == EOF)
	break;

      switch (opt) 
	{
	case 0:
	  break;
	case 'd':
	  daemon_mode = 1;
	  break;
	case 'f':
	  config_file = optarg;
	  break;
	case 'A':
	  vty_addr = optarg;
	  break;
        case 'i':
          pid_file = optarg;
          break;
	case 'P':
	  /* Deal with atoi() returning 0 on failure */
          if (strcmp(optarg, "0") == 0) 
            {
              vty_port = 0;
              break;
            } 
          vty_port = atoi (optarg);
          vty_port = (vty_port ? vty_port : OLSR_VTY_PORT);
  	  break;
	case 'u':
	  olsrd_privs.user = optarg;
	  break;
	case 'g':
	  olsrd_privs.group = optarg;
	  break;
	case 'v':
	  olsr_print_version();
	  exit (0);
	  break;
	case 'h':
	  usage (progname, 0);
	  break;
	default:
	  usage (progname, 1);
	  break;
	}
    }



  /* 
   * Initializations. 
   */
  master = olm->master;
  
  olsr_signal_init();
  

  /* 
   * Library inits 
   */
  zprivs_init (&olsrd_privs);

  cmd_init(1);
  vty_init(master);
  memory_init();
#if (OEM_PACKAGE_VERSION > OEM_BASE_VERSION(1,0,0))
  vrf_init();
#else
  //if_init ();
#endif
  access_list_init ();
  prefix_list_init ();


  /*
   * OLSR inits.
   */
  olsr_if_init();


  olsr_vty_init();
  olsr_debug_init ();
  olsr_zebra_init();
  
  olm->olsr = list_new();

/* 2016��7��3�� 15:35:07 zhurish: �����°�quagga��ʹ�õĺ��� */
  //sort_node();
/* 2016��7��3�� 15:35:07  zhurish: �����°�quagga��ʹ�õĺ��� */

  /* Read configuration */
  vty_read_config (config_file, config_default);
  
  /* Change to the daemon program. */
  if (daemon_mode)
    daemon(0, 0);


  /* Create PID file */
  pid_output(pid_file);
  
  /* Create VTY socket. */
  vty_serv_sock (vty_addr, vty_port, OLSR_VTYSH_PATH);

  /* Print banner. */
  zlog_notice ("OLSRd %s starting: vty@%d", OLSRD_VERSION, vty_port);
  
  /* Fetch next active thread. */
  while (thread_fetch (master, &thread))
    thread_call (&thread);
  
  /* Not reached. */
  exit (0);
}
