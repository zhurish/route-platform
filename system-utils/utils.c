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
