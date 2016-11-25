#ifndef	_EIGRP_PRE_DEFINE_H_
#define	_EIGRP_PRE_DEFINE_H_
#ifdef __cplusplus 
extern "C"{
#endif

/* ����ϵͳƽ̨���� */
#define	EIGRP_OS_VXWORKS		1
#define	EIGRP_OS_PIL			2/* window ����ƽ̨ */
#define EIGRP_OS_LINUX			3
#define	EIGRP_OS_TYPE			EIGRP_OS_LINUX

#define	EIGRP_LINUX_USE_VM	TRUE

/* ·��ƽ̨���� */
#define	EIGRP_PLAT_BSD		1/*û��ƽ̨*/
#define	EIGRP_PLAT_ROUTER		2/* zebra��Quagga�� ������ zebos ƽ̨*/
#define	EIGRP_PLAT_PIL			3/* window ����ƽ̨ */
#define	EIGRP_PLAT_TYPE		EIGRP_PLAT_ROUTER	

#define	EIGRP_SUMMARY_RULE_VER_12		1
#define	EIGRP_SUMMARY_RULE_VER_11		2
#define	EIGRP_SUMMARY_RULE_TYPE		EIGRP_SUMMARY_RULE_VER_12

#define	EIGRP_ROUTE_REDIS_RULE_A	1
#define	EIGRP_ROUTE_REDIS_RULE_B	2
#define	EIGRP_ROUTE_REDIS_RULE_TYPE	EIGRP_ROUTE_REDIS_RULE_B

#define	EIGRP_PRODUCT_TYPE_COMMERCIAL	1
#define	EIGRP_PRODUCT_TYPE_YEZONG		2
#define	EIGRP_PRODUCT_TYPE_DIYUWANG	3
#define	EIGRP_PRODUCT_TYPE		EIGRP_PRODUCT_TYPE_YEZONG

/*add_zhenxl_20100810*/
/* SNMP ֧�� */
#define INCLUDE_EIGRP_SNMP
#undef INCLUDE_EIGRP_SNMP

/*���������ŵ�*/
/*#define INCLUDE_SATELLITE_RESTRICT*/


/*************************************************************************/
/*************************************************************************/
/* ��չƽ̨���� �Ż����� */
#define _EIGRP_PLAT_MODULE	


/* ƽ̨���ޱ��֧�� */
#define _EIGRP_UNNUMBERED_SUPPORT	
#undef _EIGRP_UNNUMBERED_SUPPORT

/* (Ӣ�𲩹�˾��Ʒ�����豸�ģ�VLAN֧��) */
#define _EIGRP_VLAN_SUPPORT
#undef _EIGRP_VLAN_SUPPORT
/* 
 * Ӣ�𲩹�˾��Ʒ�����豸�ģ�DC��������֧�֣�Ϊ�ޱ�Žӿڷ���
 * Ӧ���Ǻ�_EIGRP_UNNUMBERED_SUPPORT�Լ�_EIGRP_VLAN_SUPPORT ���ʹ��
 */
#define _DC_ 
#undef _DC_


/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#ifdef _EIGRP_PLAT_MODULE

//�Ż���ʱ����ع���
#define _EIGRP_PLAT_MODULE_TIMER

#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)//֧��·��ƽ̨
//#define EIGRP_PLAT_ZEBOS
#define EIGRP_PLAT_ZEBRA//֧��zebraƽ̨

#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
#endif //(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
#if (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#define FRP_DEFAULT_CONFIG	"frpd.conf"
#define FRP_VTY_PORT	2616
#endif
//#define EIGRP_REDISTRIBUTE_METRIC 
#define _EIGRP_PLAT_MODULE_DEBUG

#define _EIGRP_DEBUG	printf
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifndef EIGRP_PLAT_ZEBRA
//#define	ROUTER_STACK
	#include "vxworks.h"
	#include "stdio.h"
	#include "stdlib.h"
	#include "string.h"
	#include "memLib.h"
	#include "msgqlib.h"
	#include "netinet/in.h"
	#include "netinet/ip.h"
	#include "netinet/in_var.h"
	#include "netinet/in_systm.h"
	#include "netinet/ip_icmp.h"
	#include "netinet/if_ether.h"
	#include "timers.h"
	#include "time.h"
	#include "semLib.h"
	#include "assert.h"
	#include "socket.h"
	#include "ioLib.h"
	#include "m2Lib.h"
	#include "errno.h"
	#include "errnoLib.h"
	
	#include "end.h"			
	#include "endLib.h"
	#include "etherLib.h"
	#include "etherMultiLib.h"		
	#include "muxLib.h"
	#include "arpLib.h"
	#include "etherLib.h"
	
	#include "net/mbuf.h"
	#include "net/route.h"
	#include "net/unixLib.h"
	#include "net/protosw.h"
	#include "net/systm.h"
	#include "net/if.h"

	#include "sys/socket.h"
	#include "sys/ioctl.h"
	#include "sys/times.h"
	
	#include "logLib.h"
	#include "sysLib.h"
	#include "arpa/inet.h"

#ifndef	_BYTE_
#define	_BYTE_
#endif
#endif// EIGRP_PLAT_ZEBRA
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#ifdef EIGRP_PLAT_ZEBRA //·��ƽ̨֧�ֵ�ͷ�ļ�
	#include "../../../Quagga/include/config.h"
	#include "../../../Quagga/include/memoryq.h"
	#include "../../../Quagga/include/buffer.h"
	#include "../../../Quagga/include/log.h"
	#include "../../../Quagga/include/network.h"
	#include "../../../Quagga/include/threadq.h"
	#include "../../../Quagga/include/linklist.h"
	#include "../../../Quagga/include/str.h"
	#include "../../../Quagga/include/streamq.h"
	#include "../../../Quagga/include/sockoptq.h"
	#include "../../../Quagga/include/plist.h"
	#include "../../../Quagga/include/prefix.h"
	#include "../../../Quagga/include/ifq.h"
	#include "../../../Quagga/include/linklist.h"
	#include "../../../Quagga/include/zclient.h"
	#include "../../../Quagga/include/zebra/connected.h"
#endif//EIGRP_PLAT_ZEBRA
/*************************************************************************/
/*************************************************************************/
#elif (EIGRP_OS_TYPE == EIGRP_OS_PIL)
	#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
		#include	<rwos.h>
		#include	"rtm_constants.h"
		#include	"rtm_enums.h"
		#include	"rtm_ip_includes.h"
		#include	"rtm_explib.h"
	#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	#include	"../PIL/common/include/PilSysPort.h"
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
	#include	"../PIL/common/include/packet.h"
	#include	"../PIL/win32/include/win32Port.h"

	#include	"../IpV4/Socket/include/socket.h"
	#include	"../IpV4/ipnet/include/ip.h"
	#include	"../IpV4/include/errno.h"
	#include	"../IpV4/socket/include/socketCommon.h"
	#include	"../IpV4/in/include/Ip4In.h"
	#include	"../IpV4/include/Ipv4SysPort.h"

	#include	"../rt/include/Route.h"
	#include	"../rt/include/RouteCore.h"
	#include	"../rt/include/RouteMain.h"
#endif //(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
/*************************************************************************/
/*************************************************************************/
#elif (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#ifndef EIGRP_PLAT_ZEBRA
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <errno.h>
	#include <sys/ipc.h>
	#include <sys/time.h>
	#include <semaphore.h>
	#include <signal.h>
	#include <time.h>
	#include <assert.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif// EIGRP_PLAT_ZEBRA
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

#ifdef EIGRP_PLAT_ZEBRA//·��֧�ֵ�ͷ�ļ�
	#include <zebra.h>
	#include "lib/version.h"
	#include "thread.h"
	#include "time.h"
	#include "hash.h"
	#include "stream.h"
	#include "if.h"
	#include "zclient.h"
	#include "linklist.h"
	#include "vty.h"
	#include "command.h"
	#include "sockunion.h"
	#include "prefix.h"
	#include "memory.h"
	#include "table.h"
	#include "buffer.h"
	#include "str.h"
	#include "log.h"
	#include "getopt.h"
	#include "privs.h"
	#include "sigevent.h"
	#include "filter.h"
	#include <math.h>
#endif//EIGRP_PLAT_ZEBRA

#endif //(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
/*************************************************************************/
/*************************************************************************/

#ifdef	_DC_
#include	"../DC/include/Dc.h"
#include	"../DC/include/Dc.h"
#include	"../DC/include/DcCmd.h"
#include	"../DC/include/DcConfig.h"
#include	"../DC/include/DcMain.h"
#include	"../DC/include/DcPacket.h"
#include	"../DC/include/DcUtil.h"
#endif
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifndef CMSG_FIRSTHDR/* HAVE_BRSUCCESSEN_CMSG_FIRSTHDR */
#define	CMSG_FIRSTHDR(mhdr)	((struct cmsghdr *)(mhdr)->msg_control)
#endif
/* 
 * RFC 3542 defines several macros for using struct cmsghdr.
 * Here, we define those that are not present
 */

/*
 * Internal defines, for use only in this file.
 * These are likely wrong on other than ILP32 machines, so warn.
 */
#ifndef _CMSG_DATA_ALIGN
#define _CMSG_DATA_ALIGN(n)           (((n) + 3) & ~3)
#endif /* _CMSG_DATA_ALIGN */

#ifndef _CMSG_HDR_ALIGN
#define _CMSG_HDR_ALIGN(n)            (((n) + 3) & ~3)
#endif /* _CMSG_HDR_ALIGN */

/*
 * CMSG_SPACE and CMSG_LEN are required in RFC3542, but were new in that
 * version.
 */
#ifndef CMSG_SPACE
#define CMSG_SPACE(l)       (_CMSG_DATA_ALIGN(sizeof(struct cmsghdr)) + _CMSG_HDR_ALIGN(l))
#warning "assuming 4-byte alignment for CMSG_SPACE"
#endif  /* CMSG_SPACE */


#ifndef CMSG_LEN
#define CMSG_LEN(l)         (_CMSG_DATA_ALIGN(sizeof(struct cmsghdr)) + (l))
#warning "assuming 4-byte alignment for CMSG_LEN"
#endif /* CMSG_LEN */


/*  The definition of struct in_pktinfo is missing in old version of
    GLIBC 2.1 (Redhat 6.1).  */
#if defined (GNU_LINUX) && defined (HAVE_STRUCT_IN_PKTINFO)
struct in_pktinfo
{
  int ipi_ifindex;
  struct in_addr ipi_spec_dst;
  struct in_addr ipi_addr;
};
#endif


/* 
 * if we have  struct sockaddr_dl ,shuld defined HAVE_NET_IF_DL_H
 * ��Ҫ�����ڻ�ȡ�������ݰ��Ľӿڲ��� 
 */
#ifndef HAVE_NET_IF_DL_H
#define HAVE_NET_IF_DL_H
#endif//HAVE_NET_IF_DL_H

#ifdef HAVE_NET_IF_DL_H
#ifndef SOPT_SIZE_CMSG_RECVIF_IPV4
#define SOPT_SIZE_CMSG_RECVIF_IPV4()	(sizeof (struct sockaddr_dl))
#endif //SOPT_SIZE_CMSG_RECVIF_IPV4
#endif //HAVE_NET_IF_DL_H
#endif//#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/			
#endif//_EIGRP_PLAT_MODULE

#ifdef __cplusplus
}
#endif

#endif
