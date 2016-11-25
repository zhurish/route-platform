/*
 * utils.c
 *
 *  Created on: Oct 16, 2016
 *      Author: zhurish
 */
#include <zebra.h>

#include "if.h"
#include "vty.h"
#include "sockunion.h"
#include "prefix.h"
#include "command.h"
#include "memory.h"
#include "log.h"
#include "zclient.h"
#include "thread.h"
#include "privs.h"
#include "sigevent.h"

#include "system-utils/utils.h"


int super_system(const char *cmd)
{
	int ret = 0;
	errno = 0;
	//if ( vpn_privs.change (ZPRIVS_RAISE) )
	//	zlog_err ("%s: could not raise privs, %s",__func__,safe_strerror (errno) );

	ret = system(cmd);
	if(ret == -1 || ret == 127)
	{
		zlog_err ("%s: execute cmd: %s(%s)",__func__,cmd,safe_strerror (errno) );
		return CMD_WARNING;
	}
	//if ( vpn_privs.change (ZPRIVS_LOWER) )
	//	zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
	UTILS_DEBUG_LOG ("%s %s",cmd, safe_strerror (errno) );
	return ret;
}



/* Debug node. */
struct cmd_node debug_node =
{
  DEBUG_NODE,
  "",				/* Debug node has no interface. */
  1
};
static int config_write_debug (struct vty *vty)
{
  int write = 0;
#ifdef HAVE_UTILS_SNTP
  extern int sntpc_debug_config(struct vty *vty);
  sntpc_debug_config(vty);
#endif /*HAVE_UTILS_SNTP*/
  return write;
}

void utils_debug_init (void)
{
  install_node (&debug_node, config_write_debug);
}
