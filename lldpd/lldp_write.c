/*
 * lldp_write.c
 *
 *  Created on: Oct 25, 2016
 *      Author: zhurish
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
#include "sockopt.h"
#include "zclient.h"

#include "lldpd.h"
#include "lldp_db.h"
#include "lldp_interface.h"
#include "lldp_packet.h"
#include "lldp-socket.h"

int lldp_write(struct thread *thread)
{
	int sock;
	struct interface *ifp;
	struct lldp_interface *lifp;
	ifp = THREAD_ARG (thread);
	sock = THREAD_FD (thread);
	lifp = ifp->info;
	return lldp_write_packet (sock, ifp, lifp->obuf);
}



