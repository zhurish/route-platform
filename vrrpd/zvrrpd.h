#ifndef __ZEBRA_VRRPD_H__
#define __ZEBRA_VRRPD_H__
#ifdef __cplusplus
extern "C" {
#endif

#define ZVRRPD_ON_LINUX		1
#define ZVRRPD_ON_VXWORKS	2
#define ZVRRPD_OS_TYPE		ZVRRPD_ON_LINUX

//#define ZVRRP_DEBUG

#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
#define VRRP_DEFAULT_CONFIG	"vrrpd.conf"
#define VRRP_VTY_PORT	2615
#endif

#define ZVRRPD_ON_ROUTTING	1


#ifdef ZVRRPD_ON_ROUTTING
#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
#include <zebra.h>
#include <lib/version.h>
#include "command.h"
#include "prefix.h"
#include "table.h"
#include "stream.h"
#include "memory.h"
#include "routemap.h"
#include "zclient.h"
#include "log.h"
#include "getopt.h"
#include "thread.h"
#include "vty.h"
#include "privs.h"
#include "sigevent.h"
#endif
#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_VXWORKS)
#include <zebra.h>
#include <versionq.h>
#include "command.h"
#include "prefix.h"
#include "table.h"
#include "streamq.h"
#include "memoryq.h"
#include "routemap.h"
#include "zclient.h"
#include "log.h"
#include "threadq.h"
#include "vty.h"
#include "sigevent.h"

#include "net/if_ll.h"
#endif
#else// ZVRRPD_ON_ROUTTING

#include <copyright_wrs.h>
#include <vxWorks.h>
#include <version.h>
#include <ctype.h>
#include <errno.h>
#include <errnoLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <utime.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <selectLib.h>

#include <ioSlib.h>
#include <ioLib.h>
#include <taskLib.h>
#include <tickLib.h>
#include <sysLib.h>

#include <end.h>			
#include <endLib.h>
#include <etherLib.h>
#include <muxLib.h>
#include <m2lib.h>

#include <semlib.h>
#include <memLib.h>

#include <fioLib.h>
#include <dirent.h>
#include <dosFsLib.h>
#include <symLib.h>

#include <stdarg.h>

#include <selectlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h> 
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/tcp.h>
#include <netinet/in_var.h>

#include <net/mbuf.h>
#include <net/if.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_var.h>
#include <net/route.h>
#include "net/if_ll.h"

#include <arpa/inet.h>
#include <private/symLibP.h>
#include <private/muxLibP.h>

#endif//ZVRRPD_ON_ROUTTING


#ifndef OK
#define OK (0)
#endif

#ifndef ERROR
#define ERROR (-1)
#endif
#ifndef ETH_ALEN
#define ETH_ALEN (6)
#endif

#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_LINUX)
typedef unsigned long ulong_t;
#endif

/* protocol constants */
#define INADDR_VRRP_GROUP 0xe0000012    /* multicast addr - rfc2338.5.2.2 */

#define IPPROTO_VRRP     112     /* IP protocol number -- rfc2338.5.2.4*/
#define VRRP_VERSION     2       /* current version -- rfc2338.5.3.1 */
#define VRRP_IP_TTL      255     /* in and out pkt ttl -- rfc2338.5.2.3 */

#define VRRP_PKT_ADVERT  1       /* packet type -- rfc2338.5.3.2 */

#define VRRP_PRIO_OWNER  255     /* priority of the ip owner -- rfc2338.5.3.4 */
#define VRRP_PRIO_DFL    100     /* default priority -- rfc2338.5.3.4 */
#define VRRP_PRIO_STOP   0       /* priority to stop -- rfc2338.5.3.4 */

#define VRRP_AUTH_NONE   0       /* no authentification -- rfc2338.5.3.6 */
#define VRRP_AUTH_PASS   1       /* password authentification -- rfc2338.5.3.6 */
#define VRRP_AUTH_AH     2       /* AH(IPSec) authentification - rfc2338.5.3.6 */

#define VRRP_ADVER_DFL   1       /* advert. interval (in sec) -- rfc2338.5.3.7 */
#define VRRP_PREEMPT_DFL 1       /* rfc2338.6.1.2.Preempt_Mode */


#define VRRP_IF_TRACK_MAX     8
#define VRRP_PRI_TRACK        10

#define VRRP_AUTH_LEN      8

#define VRRP_IS_BAD_VID(id)       ((id)<1 || (id)>255)    /* rfc2338.6.1.vrid */
#define VRRP_IS_BAD_PRIORITY(p)   ((p)<1 || (p)>254)      /* rfc2338.6.1.prio */
#define VRRP_IS_BAD_ADVERT_INT(d) ((d)<1 || (d)>255)
#define VRRP_IS_BAD_AUTH_TYPE(d)  ((d)>2)
#define VRRP_IS_BAD_DELAY(t)      ((t)>255)

/* use the 'tcp sequence number arithmetic' to handle the wraparound.
** VRRP_TIMER_SUB: <0 if t1 precedes t2, =0 if t1 equals t2, >0 if t1 follows t2
*/
#define VRRP_TIMER_SET( val, delta )  (val) = (delta)
#define VRRP_TIMER_EXPIRED( val )     ((0 != val) && ((--val) ==0) )
#define VRRP_TIMER_CLR( val )         (val) = 0
#define VRRP_TIMER_IS_RUNNING( val )  (val)
#define VRRP_TIMER_HZ                 1

#define VRRP_TIMER_SKEW( srv ) ((256-(srv)->priority)*VRRP_TIMER_HZ/256) 

#define VRRP_MIN( a , b )    ( (a) < (b) ? (a) : (b) )
#define VRRP_MAX( a , b )    ( (a) > (b) ? (a) : (b) )


#define VRRP_VSRV_SIZE_MAX    255
#define VRRP_VIP_NUMS_MAX     8

enum {
    zvrrp_opcode_add_vrs = 1,
    zvrrp_opcode_del_vrs,
    zvrrp_opcode_enable_vrs,
    zvrrp_opcode_disable_vrs,
    zvrrp_opcode_enable_ping,
    zvrrp_opcode_disable_ping,    
    zvrrp_opcode_add_ip,
    zvrrp_opcode_del_ip,
    zvrrp_opcode_set_pri,	
    zvrrp_opcode_unset_pri,  
    zvrrp_opcode_set_interval,
    zvrrp_opcode_unset_interval,
    zvrrp_opcode_set_interval_msec,
    zvrrp_opcode_unset_interval_msec,
    zvrrp_opcode_set_learn_master,
    zvrrp_opcode_unset_learn_master,  
    zvrrp_opcode_set_preempt,
    zvrrp_opcode_unset_preempt,  
    zvrrp_opcode_set_preempt_delay,
    zvrrp_opcode_unset_preempt_delay,   
    zvrrp_opcode_add_interface,
    zvrrp_opcode_del_interface,
    zvrrp_opcode_set_track,
    zvrrp_opcode_unset_track, 
    zvrrp_opcode_show,  
};

typedef struct {
    int vrid;
    int cmd;
    int ifindex;
    char ifname[32];
    int value;
    long ipaddress;
    void * param1;
    void * param2;
    void * param3;
    int respone;
} zvrrp_opcode;

#define VRRP_CFGERR_IFWRONG        1
#define VRRP_CFGERR_SUBNETDIFF     2
#define VRRP_CFGERR_MAXVSRV        3
#define VRRP_CFGERR_MAXVIP         4
#define VRRP_CFGERR_VSRVNOTEXIST   5
#define VRRP_CFGERR_VIPNOTEXIST    6


#include "zvrrp_if.h"

typedef struct {
    uint32_t    addr;         /* the ip address */
} vip_addr;

struct zvrrp_master;

typedef struct {    /* parameters per virtual router -- rfc2338.6.1.2 */    
    int    no;         /* number ????????????е??±? */
    int    used;       /* 0: δ???1: ????? */
    int    enable;
	int    vrid;         /* virtual id. from 1(!) to 255 */
    int    priority;     /* priority value */
    int    oldpriority;  /* old priority value */    
    int    runpriority;  /* run priority value */
    int    nowner;       /* ????IP?????????IP??????? */
    int    naddr;        /* number of virtual ip addresses */
    //每个VRRP组最多有8个虚拟IP地址
    vip_addr 	vaddr[VRRP_VIP_NUMS_MAX];     /* point on the ip address array */
    int    		adver_int;    /* delay between advertisements(in sec) */
    int    		preempt;    /* true if a higher prio preempt a lower one */
    int    		delay;      /* preempt delay */
    int    		state;      /* internal state (init/backup/master) */
    uint32_t    adver_timer;
    //每个VRRP组拥有的物理接口（目前只有一个接口在一个VRRP组上）
    vrrp_if		vif;
    char        vhwaddr[6]; /* ????MAC??? */
    int         adminState; /* ?????? 1: up??2: down */
    
    int			ms_learn;/* ?? master ????? */
    ulong_t		ms_router_id;/* master ?? router id */
    int    		ms_priority;/* master ?? priority  */
    uint32_t    ms_advt_timer;/* master ?? advertisements time */
    uint32_t    ms_down_timer;/* master ?? down time */
    
    ulong_t     upTime;     /* ??INIT?????rfc1907:sysUpTime????? */
    int         niftrack;   /* ?????????? */
    int         iftrack[VRRP_IF_TRACK_MAX]; /* ?????????????? */
    int         pritrack[VRRP_IF_TRACK_MAX]; /* ??????down???????????? */

    ulong_t     staBecomeMaster;     /* ??????Master????? */
    ulong_t     staAdverRcvd;        /* ?????VRRP????? */
    ulong_t     staAdverIntErrors;   /* ??????????????????????? */
    ulong_t     staAuthFailures;     /* ???????????? */
    ulong_t     staIpTtlErrors;      /* IP TTL??????255??????? */
    ulong_t     staPriZeroPktsRcvd;  /* ???????????0??????? */
    ulong_t     staPriZeroPktsSent;  /* ???????????0??????? */
    ulong_t     staInvTypePktsRcvd;  /* ????????????????? */
    ulong_t     staAddrListErrors;   /* ????????б????????????? */
    ulong_t     staInvAuthType;      /* ?????δ?????????????? */
    ulong_t     staAuthTypeMismatch; /* ????????????????????????? */
    ulong_t     staPktsLenErrors;    /* ????????????????? */

    struct zvrrp_master *zvrrp_master;
} vrrp_rt;


struct zvrrp_master
{
	int init;
	int task;
	void *SemMutexId;    
	int	ping_enable;/* ???ping 1: enable??0: disable */
	void *master;
#ifdef ZVRRPD_ON_ROUTTING	
	struct zclient *zclient;
	struct vty *vty;
#endif// ZVRRPD_ON_ROUTTING	
	zvrrp_opcode *opcode;
	int vrid;
	vrrp_rt gVrrp_vsrv[VRRP_VSRV_SIZE_MAX+1];
};

extern struct zvrrp_master *gVrrpMatser;
extern struct zvrrp_master *zvrrp_master_lookup(void);
extern vrrp_rt * zvrrp_vsrv_lookup( int vrid );
extern int zvrrp_handle_on_state(vrrp_rt *pVsrv, const char *buff);
extern int vrrp_mach_ipaddr( vrrp_rt *vsrv, uint32_t vipaddr );

extern int zvrrp_cmd_config(void *m);

extern int zvrrp_main(void *vrrp);
extern int zvrrp_master_init(int pri, void *m);
extern int zvrrp_master_uninit(void);
extern int zvrrp_vsrv_show(int vrid);


#define CHECK_VALID(p, retVal)\
		if(!(p))\
		{\
		   return retVal;\
		}

enum {
    zvrrpOperState_initialize = 1,
    zvrrpOperState_backup = 2,
    zvrrpOperState_master = 3,
    zvrrpOperAdminState_up = 1,
    zvrrpOperAdminState_down = 2,	
};

#ifdef ZVRRP_DEBUG
#define ZVRRP_DEBUG_LOG(msg,args...)	\
		{	\
			printf("## DEBUG: (%s:%d) %s --> ",__FILE__,__LINE__,__FUNCTION__);\
			printf(msg,##args);\
			printf("\r\n");\
		}

#define zvrrp_debug(msg,args...)	\
		{	\
			printf(msg,##args);\
		}
#define zvrrp_show(vsrv,msg,args...)	\
		{	\
			printf(msg,##args);\
			printf("\r\n");\
		}
#else
#define ZVRRP_DEBUG_LOG(msg,args...)

#define zvrrp_debug(msg,args...)	\
		{	\
			zlog_debug(msg,##args);\
		}
#define zvrrp_show(vsrv,msg,args...)	\
		{	\
			struct vty *vty = (struct vty *)vsrv->zvrrp_master->vty; \
			vty_out(vty, msg,##args);\
			vty_out(vty,"%s",VTY_NEWLINE);\
		}
#endif

#ifdef __cplusplus
}
#endif
#endif /*__ZEBRA_VRRPD_H__*/
