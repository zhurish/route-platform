 /************************************************************************************

	(C) COPYRIGHT 2005-2007 by Beijing Jointbest System Technology Co. Ctd.
						All rights reserved.

	This software is confidential and proprietary to Beijing Jointbest System Technology Co. Ctd.
	No part of this software may be reproduced, stored, transmitted, disclosed or used in any 
	form or by any means other than as expressly provided by the written license agreement
	between Beijing Jointbest System Technology Co. Ctd. and its licensee.

	JOINTBEST MAKES NO OTHER WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT
	LIMITATION WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
	WITH REGARD TO THIS SOFTWARE OR ANY RELATED MATERIALS.

	IN NO EVENT SHALL JOINTBEST BE LIABLE FOR ANY INDIRECT, SPECIAL, OR CONSEQUENTIAL
	DAMAGES IN CONNECTION WITH OR ARISING OUT OF THE USE OF, OR INABILITY TO USE,
	THIS SOFTWARE, WHETHER BASED ON BREACH OF CONTRACT, TORT (INCLUDING 
	NEGLIGENCE), PRODUCT LIABILITY, OR OTHERWISE, AND WHETHER OR NOT IT HAS BEEN
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

					IMPORTANT LIMITATION(S) ON USE

	The use of this software is limited to the Use set 	forth in the written License Agreement 
	between Jointbest and its Licensee. Among other things, the Use of this software may be limited
	to a particular type of Designated Equipment. Before any installation, use or transfer of this 
	software, please consult the written License Agreement or contact Jointbest at the location set 
	forth below in order to confirm that you are engaging in a permissible Use of the software.

	Beijing Jointbest System Technology Co. Ctd.
	1th, 10th Floor, A Building, Changyin Plaza,
	Yongding Road, Haidian District, Beijing, China

	Tel:	+86 10 58896099
	Fax:	+86 10 58895234
	Web: www.jointbest.com

************************************************************************************/

/************************************************************************************

	Module:	Eigrp

	Name:	EigrpSysPort_Snmp.c

	Desc:	

	Rev:	
	
***********************************************************************************/

#include	"./include/EigrpPreDefine.h"

#ifdef INCLUDE_EIGRP_SNMP

#if		(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS || EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#define	ROUTER_STACK
#include	"vxworks.h"
#include	"memLib.h"
#include	"msgqlib.h"
#include	"netinet/in.h"
#include	"timers.h"
#include	"time.h"
#include	"semLib.h"
#include	"assert.h"
#include	"socket.h"
#include	"ioLib.h"
#include	"m2Lib.h"
#ifndef _EIGRP_PLAT_MODULE
#include	"routeEnhLib.h"
#endif//_EIGRP_PLAT_MODULE
#include <sysLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include	"./include/EigrpSysPort.h"
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL || EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
#include	"./include/EigrpSysPort.h"
#include	"../PIL/common/include/util.h"
#include	"../PIL/common/include/memory.h"
#include	"../PIL/common/include/ListAndCallback.h"
#include	"../PIL/common/include/RbX.h"
#include	"../PIL/common/include/Timer.h"
#include	"../PIL/common/include/Queue.h"
#include	"../PIL/common/include/circ.h"
#include	"../PIL/IfM/include/IfM.h"
#include	"../PIL/common/include/MacFdb.h"
#include	"../PIL/common/include/process.h"
#include	"../PIL/common/include/system.h"
#include	"..\PIL\common\include\packet.h"

#include	"../IpV4/Socket/include/socket.h"
#include	"../IpV4/ipnet/include/ip.h"
#include	"../IpV4/include/errno.h"
#include	"../IpV4/socket/include/socketCommon.h"
#include	"../IpV4/in/include/Ip4In.h"

#include	"../rt/include/Route.h"
#include	"../rt/include/RouteCore.h"
#include	"../rt/include/RouteMain.h"
#elif(EIGRP_OS_TYPE == EIGRP_OS_LINUX )
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "nsm_router.h"
#include "nsm_table.h"
#include "./include/EigrpSysPort.h"
#include <netinet/in.h>
#endif

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
#include "pal.h"
#include "cli.h"
#include "host.h"	
#include "lib.h"

#include "vty.h"
#include "linklist.h"
#include "prefix.h"
#include "rib.h"
#include "table.h"
#include "linklist.h"
#include "hash.h"
#include "filter.h"
#include "show.h"

#include "if.h"
#include "nexthop.h"

#include "log.h"
#include "table.h"

#include "nsm_client.h"
#include "nsm_message.h"

#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_debug.h"
#include "nsm_igmp.h"
#include "nsm_router.h"
#include "thread.h"
#include "snprintf.h"
#endif// EIGRP_PLAT_ZEBOS
#endif

#include	"./include/EigrpCmd.h"
#include	"./include/EigrpUtil.h"
#include	"./include/EigrpDual.h"
#include	"./include/Eigrpd.h"
#include	"./include/EigrpIntf.h"
#include	"./include/EigrpIp.h"
#include	"./include/EigrpMain.h"
#include	"./include/EigrpPacket.h"

extern	Eigrp_pt	gpEigrp;

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
extern	struct lib_globals *nzg;
extern	int	errorFd;
extern	int	errorProto;
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)

#include"./include/EigrpDef.h"
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef HAVE_SNMP
#endif//HAVE_SNMP

#include "./include/EigrpSysPortRouterSnmp.h"

/* OSPF-MIB instances. */
oid eigrp_oid [] = { EIGRPMIB };
oid eigrpd_oid [] = { EIGRPDOID };

static u_char *EigrpSysPortAsTable ();

struct variable eigrp_variables[] =
{
  /* OSPF general variables */
  {EIGRP_AS_NO,              INTEGER, RONLY, EigrpSysPortAsTable,
   2, {1, 1}},
  {EIGRP_AS_SUBNET,             STRING, RONLY, EigrpSysPortAsTable,
   2, {1, 2}},
  {EIGRP_AS_ENABLENET,             STRING, RONLY, EigrpSysPortAsTable,
   2, {1, 2}},
  {EIGRP_AS_AUTO_FOUND_NEIGHBORS,             STRING, RONLY, EigrpSysPortAsTable,
   2, {1, 2}},
  {EIGRP_AS_AUTO_FOUND_NEIGHBORS,             STRING, RONLY, EigrpSysPortAsTable,
   2, {1, 2}}
};


/* ospfGeneralGroup { ospf 1 } */
static u_char *
EigrpSysPortAsTable(struct variable *v, oid *name, size_t *length,
		  int exact, size_t *var_len, WriteMethod **write_method)
{
	printf("v->magic:%d, name:%d, length:%d, exact:%d, var_len:%d", v->magic, name, *length, 
			exact, *var_len);
  return NULL;
}

/* Register EIGRP-MIB. */
void
EigrpPortRouterSnmpInit(struct lib_globals *zg)
{
  smux_init (zg, eigrpd_oid, sizeof (eigrpd_oid) / sizeof (oid));
  REGISTER_MIB (zg, "mibII/eigrp", eigrp_variables, variable, eigrp_oid);
    
  smux_start (zg);
}

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif /* HAVE_SNMP */
#endif
#endif //INCLUDE_EIGRP_SNMP
