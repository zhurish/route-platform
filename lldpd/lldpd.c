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


struct lldpd *lldpd_config;


int lldp_interface_enable(struct interface *ifp)
{
	int ret = 0;
	struct lldp_interface *lifp;
	if(ifp == NULL)
		return CMD_WARNING;
	lifp =  (struct lldp_interface *)ifp->info;
	if(lifp == NULL)
		return CMD_WARNING;

	ret = lldp_interface_socket_init(ifp);

	if(ret <= 0)
	{
		lifp->mode = LLDP_DISABLE;
		zlog_debug("Can't enable lldp on interface %s (create socket error)",ifp->name);
		return CMD_WARNING;
	}
	lifp->ibuf = stream_new (LLDP_PACKET_MAX_SIZE);
	lifp->obuf = stream_new (LLDP_PACKET_MAX_SIZE);
	//lifp->outbuf = XMALLOC (MTYPE_STREAM_DATA, LLDP_PACKET_MAX_SIZE);
	LLDP_DEBUG_LOG("%s:%s\n",__func__,ifp->name);
	if(lifp->mode & LLDP_WRITE_MODE)
		lifp->t_time = thread_add_timer(master, lldp_timer, ifp, lifp->lldp_timer);
	//lldp_send_debug(ifp);
	return CMD_SUCCESS;
}
/*
int lldp_interface_enable_update(struct interface *ifp)
{

}
*/
int lldp_interface_enable_update(struct interface *ifp)
{
	struct lldp_interface *lifp;
	if(ifp == NULL)
		return -1;
	lifp =  (struct lldp_interface *)ifp->info;
	if(lifp == NULL)
		return -1;
	if(lifp->sock <= 0)
		return CMD_SUCCESS;
	if(lifp->mode == LLDP_DISABLE)
	{
		if(lifp->t_read)
			thread_cancel(lifp->t_read);
		if(lifp->t_write)
			thread_cancel(lifp->t_write);
		if(lifp->t_time)
			thread_cancel(lifp->t_time);
		if(lifp->ibuf)
			stream_free(lifp->ibuf);
		if(lifp->obuf)
			stream_free(lifp->ibuf);
		//if(lifp->outbuf)
		//	XFREE(MTYPE_STREAM_DATA, lifp->outbuf);
	}
	if(lifp->mode & LLDP_READ_MODE)
	{
		if(lifp->t_read)
			thread_cancel(lifp->t_read);
		lifp->t_read = thread_add_read(master, lldp_read, ifp, lifp->sock);
	}
	if(lifp->mode & LLDP_WRITE_MODE)
	{
		if(lifp->t_write)
			thread_cancel(lifp->t_write);
		//参数变更立即更新数据
		lifp->t_write = thread_add_write(master, lldp_write, ifp, lifp->sock);

		if(lifp->t_time)
			thread_cancel(lifp->t_time);
		lifp->t_time = thread_add_timer(master, lldp_timer, ifp, lifp->lldp_timer);
	}
	return CMD_SUCCESS;
}

int lldp_timer(struct thread *thread)
{
	struct interface *ifp;
	struct lldp_interface *lifp;
	ifp = THREAD_ARG (thread);
	//sock = THREAD_FD (thread);
	lifp =  (struct lldp_interface *)ifp->info;
	if(lifp)
	{
		if(lifp->mode != LLDP_DISABLE)
		{
			lldp_make_lldp_pdu(ifp);

			lifp->t_time = thread_add_timer(master, lldp_timer, ifp, lifp->lldp_timer);

			//LLDP_DEBUG_LOG("aaaaaaaaaaaaa:%s size=%d\n",ifp->name,(int)stream_get_endp(lifp->obuf));

			LLDP_DEBUG_LOG("%s:%s\n",__func__,ifp->name);
			//lldp_write_debug(ifp);
			//lldp_write_packet (lifp->sock, ifp, lifp->obuf);
			lifp->t_write = thread_add_write (master, lldp_write, ifp, lifp->sock);
		}
	}
	return CMD_SUCCESS;
}



int lldp_config_init(void)
{
	lldpd_config = XMALLOC (MTYPE_TMP, sizeof(struct lldpd));
	//lldpd_config->system_name = strdup("route-platform");

	//lldpd_config->system_desc = strdup("aaaaaaaaa.bbbbbbbbbbb.ccccccccc");
	//lldpd_config->mgt_address = strdup("192.168.1.1");
#ifdef LLDP_DEBUG_TEST
	extern int aaa_test();
	aaa_test();
#endif
	return 0;
}
