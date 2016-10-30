/*
 * lldp_read.c
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



int lldp_read(struct thread *thread)
{
	int sock;
	struct stream *ibuf;
	struct interface *ifp;
	struct interface *get_ifp;
	struct lldp_interface *lifp;
	/* first of all get interface pointer. */
	ifp = THREAD_ARG (thread);
	sock = THREAD_FD (thread);
	lifp = ifp->info;
	/* prepare for next packet. */
	lifp->t_read = thread_add_read (master, lldp_recv_packet, ifp, sock);
	stream_reset(lifp->ibuf);
	if (!(ibuf = lldp_recv_packet (lifp->sock, &get_ifp, lifp->ibuf)))
	    return -1;

	if (get_ifp == NULL)
		return -1;

	lifp = get_ifp->info;
	if(lifp)
	{
		//if(lifp->mode & LLDP_READ_MODE)
		//	thread_add_event (master, lldp_read_pa, ifp, 0);
	}
	return CMD_SUCCESS;
}



