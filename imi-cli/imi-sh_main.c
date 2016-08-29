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

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "thread.h"
#include "prefix.h"
#include "filter.h"
#include "log.h"
#include "vty.h"
#include "privs.h"
#include "sigevent.h"
#include "zclient.h"


#ifdef IMISH_IMI_MODULE

#include "imi-cli/imi-sh.h"
#include "imi-cli/imi-sh_log.h"
#include "imi-cli/imi-sh_kernel.h"
 
//#include "imi-cli/vtysh_user.h"

/* VTY shell program name. */
static char *progname = NULL;
/* Configuration file name and directory. */
static char config_default[] = SYSCONFDIR IMISH_DEFAULT_CONFIG;
static char *config_file = NULL;
/* RIP VTY bind address.  RIP VTY connection port. */
static char *vty_addr = NULL;
static int vty_port = IMISH_IMI_MODULE;
static const char *pid_file = PATH_IMISH_PID;
/* Master of threads. */
struct thread_master *master = NULL;

/* ripd privileges */
zebra_capabilities_t _caps_p [] = 
{
  ZCAP_NET_RAW,
  ZCAP_BIND
};

struct zebra_privs_t imid_privs =
{
#if defined(QUAGGA_USER)
  .user = QUAGGA_USER,
#endif
#if defined QUAGGA_GROUP
  .group = QUAGGA_GROUP,
#endif
#ifdef VTY_GROUP
  .vty_group = VTY_GROUP,
#endif
  .caps_p = _caps_p,
  .cap_num_p = 2,
  .cap_num_i = 0
};


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
usage (int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {    
      printf ("Usage : %s [OPTION...]\n\
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
Report bugs to %s\n", progname, ZEBRA_BUG_ADDRESS);
    }

  exit (status);
}
/* SIGHUP handler. */
static void 
sighup (void)
{
  zlog_info ("SIGHUP received");
	imish_kernel_shell_cmd_exit();
	imish_log_server_exit();
	imish_module_config_exit ();
	closezlog (zlog_default);
	imish_module_exit ();
	thread_master_free (master);
  exit (0);	
}

/* SIGINT handler. */
static void
sigint (void)
{
  zlog_notice ("Terminating on signal");

	imish_kernel_shell_cmd_exit();
	imish_log_server_exit();
	imish_module_config_exit ();
	closezlog (zlog_default);
	imish_module_exit ();
	thread_master_free (master);
  exit (0);
}

/* SIGUSR1 handler. */
static void
sigusr1 (void)
{
  zlog_notice ("SIGUSR1 signal");	
	imish_kernel_shell_cmd_exit();
	imish_log_server_exit();
	imish_module_config_exit ();
	closezlog (zlog_default);
	imish_module_exit ();
	thread_master_free (master);
  exit (0);
}

static struct quagga_signal_t imid_signals[] =
{
  { 
    .signal = SIGHUP,
    .handler = &sighup,
  },
  { 
    .signal = SIGUSR1,
    .handler = &sigusr1,
  },
  {
    .signal = SIGINT,
    .handler = &sigint,
  },
  {
    .signal = SIGTERM,
    .handler = &sigint,
  },
}; 

/* VTY shell main routine. */
int
main (int argc, char **argv, char **env)
{
	char *p = NULL;
  int daemon_mode = 0;
  struct thread thread;
  umask (0027);
  /* Preserve name of myself. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);
  	
  zlog_default = openzlog (progname, ZLOG_IMISH,
			   LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);
			   
  /* Option handling. */
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
          /* Deal with atoi() returning 0 on failure, and ripd not
             listening on rip port... */
          if (strcmp(optarg, "0") == 0) 
            {
              vty_port = 0;
              break;
            } 
          vty_port = atoi (optarg);
          if (vty_port <= 0 || vty_port > 0xffff)
            vty_port =IMISH_IMI_MODULE;
	  			break;
			case 'u':
	  		imid_privs.user = optarg;
	  		break;
			case 'g':
	  		imid_privs.group = optarg;
	  		break;
			case 'v':
	  		print_version (progname);
	  		exit (0);
	  		break;
			case 'h':
	  		usage ( 0);
	  		break;
			default:
	  		usage ( 1);
	  		break;
			}
    }

  master = thread_master_create ();
  /* Library initialization. */
  zprivs_init (&imid_privs);
  signal_init (master, array_size(imid_signals), imid_signals);

  /* Make vty structure and register commands. */
  imish_module_init ();
  imish_module_init_cmd ();

  imish_module_config_init ();
  
  imish_log_server_init();
  imish_kernel_shell_cmd_init();

  /* Change to the daemon program. */
  if (daemon_mode && daemon (0, 0) < 0)
    {
      zlog_err("RIPd daemon failed: %s", strerror(errno));
      exit (1);
    }

  /* Do not connect until we have passed authentication. */
  if (imish_module_connect_all (NULL) <= 0)
    {
      fprintf(stderr, "Exiting: failed to connect to any daemons.\n");
      exit(1);
    }
    
  /* Read vtysh configuration file before connecting to daemons. */
  if(config_file)    
		imish_read_config (config_file);  
	else
		imish_read_config (config_default);
  /* Pid file create. */
  pid_output (pid_file);

  /* Create VTY's socket */
  vty_serv_sock (vty_addr, vty_port, IMISH_UNIX_PATH);
    /* Print banner. */
  zlog_notice ("IMISH %s starting: vty@%d", QUAGGA_VERSION, vty_port);
  
  /* Execute each thread. */
  while (thread_fetch (master, &thread))
    thread_call (&thread);
 
  /* Rest in peace. */
  exit (0);
}
#endif /* IMISH_IMI_MODULE */
