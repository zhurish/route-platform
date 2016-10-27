/*
 * This is an implementation of draft-katz-yeung-ospf-traffic-06.txt
 * Copyright (C) 2001 KDD R&D Laboratories, Inc.
 * http://www.kddlabs.co.jp/
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
/*****************************************************************************
* On unnumbered point-to-point networks and on virtual links this field should
* be set to 0.0.0.0
* file		: ospf_dcn.c
* declare	: 实现DCN自通扩展OSPF的Type 10 LSA功能实现
* auther	: 在ZKX设计下移植到高版本的quagga过程中改写相关功能及函数名称等
* zhurish (zhurish@163.com)
*****************************************************************************/
#include <zebra.h>
#include "linklist.h"
#include "prefix.h"
#include "if.h"
#include "table.h"
#include "memory.h"
#include "command.h"
#include "vty.h"
#include "stream.h"
#include "log.h"
#include "thread.h"
#include "hash.h"
#include "sockunion.h"		/* for inet_aton() */
#include "privs.h"

#ifdef HAVE_OSPFD_DCN

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_ism.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_dump.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_zebra.h"

#include "ospfd/ospf_dcn.h"

#define DCN_MAC_TABLE
#ifdef DCN_MAC_TABLE
static int ospf_dcn_mac_init(const char *name);
static int ospf_dcn_mac_exit(void);
static int show_ospf_dcn_mac_table(struct vty *);
#endif

#ifndef MTYPE_OSPF_DCN_IF
#define MTYPE_OSPF_DCN_IF	MTYPE_OSPF_FIFO/* MTYPE_OSPF_FIFO */
#endif /* MTYPE_OSPF_DCN_IF */
/*******************************************************************************************/
enum sched_opcode { REORIGINATE_PER_AREA, REFRESH_THIS_LSA, FLUSH_THIS_LSA };
/*******************************************************************************************/
static struct ospf_dcn OspfDcn;
static char CPE_DEVICEMODEL[MAX_MODEL_LENGTH+1] = "GT-MSAP-PTAP240-SC320";
/*******************************************************************************************/
static void ospf_dcn_link_build_lsa_tlv (struct stream *s, struct dcn_link *lp);

static struct ospf_lsa * ospf_dcn_link_lsa_new (struct ospf_area *area, struct dcn_link *lp);

static void ospf_dcn_link_lsa_schedule (struct dcn_link *lp, enum sched_opcode opcode);

static struct ospf_lsa * ospf_dcn_link_lsa_install (struct ospf_area *area, struct dcn_link *lp, int flood);
/*******************************************************************************************/
enum sched_event { 
	DELETE_LOCAL_LSA 	= 0, 	/*删除本地LSA*/
	DELETE_ALL_LSA 		= 1,	/*删除所有LSA*/
	DELETE_OTHER_LSA 	= 2,	/*删除非本地LSA*/
	
	UPDATE_LINK_LSA 	= 6, /*刷新本地LSA*/
	FLOOD_LINK_LSA		= 7, /*泛洪本地LSA*/
	INSTALL_LINK_LSA	= 8, /*安装本地LSA*/
	UPDATE_LINK_LSA_REQ	= 9, /*发送LSR请求*/
};
static int ospf_dcn_link_foreach_area (enum sched_event event,  void (*func)(void *, void *));
/*******************************************************************************************/
int ospf_dcn_debug = 0;
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: dcn_link_is_loopback
* declare	: check interface it's loopbak 
* input	: 		loopbak return 1;else return 0
* output	: 
* return	: 
* other	:unused
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:30:5
*****************************************************************************/
static int dcn_link_is_loopback (struct interface *ifp)
{
	  //return (ifp->flags & (IFF_LOOPBACK|IFF_NOXMIT|IFF_VIRTUAL));
	//if_is_loopback()
	if(strcmp(ifp->name,"lo")==0)
		return 1;
	return 0;
}
/*****************************************************************************
* function	: ospf_if_lookup_by_ifp
* declare	: 根据 interface 查找 ospf_interface 接口
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:55
*****************************************************************************/
static struct ospf_interface * ospf_if_lookup_by_ifp (struct ospf *ospf,struct interface *ifp)
{
  struct listnode *node;
  struct ospf_interface *oi;

  for (ALL_LIST_ELEMENTS_RO (ospf->oiflist, node, oi))
    if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
      {
    	if (ifp && oi->ifp->ifindex == ifp->ifindex)
    		return oi;
      }

  return NULL;
}
/*****************************************************************************
* function	: set_dcn_router_addr
* declare	: setting router id for DCN module 
* input	: 		router id format IP
* output	: 
* return	: 
* other	: 设置router id，未使用
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:30:5
*****************************************************************************/
#ifdef OSPF_DCN_ROUTE_ID
static void set_dcn_router_addr (struct in_addr ipv4)
{
  OspfDcn.router_addr.header.type   = htons (DCN_TLV_ROUTER_ADDR);
  OspfDcn.router_addr.header.length = htons (sizeof (ipv4));
  OspfDcn.router_addr.value = ipv4;
  return;
}
#endif /*OSPF_DCN_ROUTE_ID*/
/*****************************************************************************
* function	: set_dcn_link_manul
* declare	: 设置厂家标识
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:30:43
*****************************************************************************/
static void set_dcn_link_manul (struct dcn_link *lp)
{
  lp->lv_manul.header.type   = htons (DCN_LINK_SUBTLV_MANUL);
  lp->lv_manul.header.length = htons (sizeof (lp->lv_manul.value));
  memset(lp->lv_manul.value, 0 ,sizeof(lp->lv_manul.value));
  strcpy((char *)lp->lv_manul.value, CPE_MANULFACTORY);
  return;
}
/*****************************************************************************
* function	: set_dcn_link_devmodel
* declare	: 设置设备型号
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:30:54
*****************************************************************************/
static void set_dcn_link_devmodel ( struct dcn_link *lp)
{
  lp->lv_devmodel.header.type   = htons (DCN_LINK_SUBTLV_DEVMODEL);
  lp->lv_devmodel.header.length = htons (sizeof (lp->lv_devmodel.value));
  memset(lp->lv_devmodel.value, 0 ,sizeof(lp->lv_devmodel.value));
  strcpy((char *)lp->lv_devmodel.value, CPE_DEVICEMODEL);
  return;
}
/*****************************************************************************
* function: set_dcn_link_mac
* declare	: 设置设备桥MAC
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:31:2
*****************************************************************************/
static void set_dcn_link_mac (struct dcn_link *lp, u_char* mac)
{
  lp->lv_mac.header.type   = htons (DCN_LINK_SUBTLV_MAC);
  lp->lv_mac.header.length = htons (sizeof (lp->lv_mac.mac.value));
  memset(&lp->lv_mac.mac, 0 ,sizeof(lp->lv_mac.mac));
  memcpy(&lp->lv_mac.mac.value, mac , MAX_MAC_LENGTH);
  GT_TLV_DEBUG("%s: lenth=%d,NeId(%02d:%02d:%02d:%02d:%02d:%02d)\n",__func__,
    sizeof (lp->lv_mac.mac.value), 
    lp->lv_mac.mac.value[0],
    lp->lv_mac.mac.value[1],
    lp->lv_mac.mac.value[2],
    lp->lv_mac.mac.value[3],
    lp->lv_mac.mac.value[4],
    lp->lv_mac.mac.value[5]);  
  return;
}
/*****************************************************************************
* function	: set_dcn_link_neid
* declare	: 设置NBR的ID
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:31:22
*****************************************************************************/
static void set_dcn_link_neid (struct dcn_link *lp, struct in_addr *neid)
{
  lp->lv_neid.header.type   = htons (DCN_LINK_SUBTLV_NEID);
  lp->lv_neid.header.length = htons (sizeof (lp->lv_neid.value));
  lp->lv_neid.value->s_addr = neid->s_addr;
  GT_TLV_DEBUG("%s: lenth=%d,NeId(%s)\n",__func__,sizeof (lp->lv_neid.value), inet_ntoa(*neid));
  return;
}
/*****************************************************************************
* function	: set_dcn_link_ipaddr
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:31:34
*****************************************************************************/
static void set_dcn_link_ipaddr (struct dcn_link *lp, struct in_addr *ipaddr)
{
  lp->lv_ipaddr.header.type   = htons (DCN_LINK_SUBTLV_IPADDR);
  lp->lv_ipaddr.header.length = htons (sizeof (lp->lv_ipaddr.value));
  lp->lv_ipaddr.value->s_addr = ipaddr->s_addr; 
  GT_TLV_DEBUG("%s: lenth=%d,ipaddr(%s)\n",__func__,sizeof (lp->lv_ipaddr.value), inet_ntoa(*ipaddr));   
  return;
}
#ifdef HAVE_IPV6
/*****************************************************************************
* function	: set_dcn_link_ipaddrv6
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:31:41
*****************************************************************************/
static void set_dcn_link_ipaddrv6 (struct dcn_link *lp, struct in_addr ipaddr)
{
  /* Note that TLV-length field is the size of array. */
  return;
}
#endif /* HAVE_IPV6 */
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_mac_address
* declare	: 获取接口MAC地址
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:31:50
*****************************************************************************/
static int ospf_dcn_link_mac_address(struct interface   *ifp, u_char *mac)
{
    struct   ifreq   ifreq; 
    int   sock;
	if(ifp == NULL)
	{
		zlog_debug("%s: %s address 0", __func__,ifp->name);
		return 0;
	}
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
    { 
    	zlog_debug("socket error: %s\n", strerror(errno));
        perror( "socket");
        GT_DEBUG("%s:%s\n",__func__,ifp->name);
        return   2; 
    } 
    strcpy(ifreq.ifr_name,ifp->name); 
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
    	zlog_debug("ioctl error: %s\n", strerror(errno));
        perror( "ioctl "); 
        GT_DEBUG("%s:%s\n",__func__,ifp->name);
        return   3; 
    } 
    memcpy((void*)mac,(void*)ifreq.ifr_hwaddr.sa_data, 6);
    close(sock);
    return 0;
}
/*****************************************************************************
* function	: ospf_dcn_link_address
* declare	: 获取OSPF接口的IP地址
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:2
*****************************************************************************/
static u_int32_t ospf_dcn_link_address (struct ospf_interface *oi)
{
  if (!oi)
    return 0;
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return 0;
  return  (oi->address->u.prefix4.s_addr);
}
/*****************************************************************************
* function	: ospf_dcn_link_ip_address
* declare	: 获取接口的IP地址
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:2
*****************************************************************************/
#if 0
static unsigned long ospf_dcn_link_ip_address(struct interface   *ifp)
{
	struct ospf *ospf;
	struct listnode *node;
	struct ospf_interface *oi;
	ospf = ospf_lookup ();
	if(ospf == NULL || ospf->oiflist == NULL || ifp == NULL)
	{
		zlog_debug("%s: %s address 0", __func__,ifp->name);
		return 0;
	}
	/* Check each Interface. */
	for (ALL_LIST_ELEMENTS_RO (ospf->oiflist, node, oi))
	{
		  if (ifp->ifindex == oi->ifp->ifindex)
		  {
			  zlog_debug("==================: %s",IF_NAME (oi));
			  return ntohl (oi->address->u.prefix4.s_addr);
		  }
	}
	zlog_debug("%s: %s address 1", __func__,ifp->name);
	return 0;
}
#else
static unsigned long ospf_dcn_link_ip_address(struct interface   *ifp)
{
	struct ifreq  ifreq; 
	int      sock = -1;
	struct sockaddr_in *tmp = NULL;
	if(ifp == NULL)
	{
		zlog_debug("%s: %s address 0", __func__,ifp->name);
		return 0;
	}
	if((sock=socket(AF_INET, SOCK_DGRAM,0)) <0) 
	{ 
		perror( "socket "); 
		GT_DEBUG("%s:%s\n",__func__,ifp->name);
		return   0; 
	} 
	strcpy(ifreq.ifr_name,ifp->name); 
	if(ioctl(sock,SIOCGIFADDR,&ifreq) <0) 
	{ 
		perror( "ioctl"); 
		GT_DEBUG("%s:%s\n",__func__,ifp->name);
		return   0; 
	}
	tmp = (struct sockaddr_in *)&(ifreq.ifr_addr);
	close(sock);
	return tmp->sin_addr.s_addr;
}
#endif	
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_initialize
* declare	: initialize dcn interface table
* input	: 
* output	: 
* return	: 
* other	: 初始化接口的DCN LINK数据，并设置已经初始化完毕标识
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:11
*****************************************************************************/
static int ospf_dcn_link_initialize (struct dcn_link *lp)
{
	int ret = 0;
    struct interface   *ifp = lp->ifp;
    struct in_addr      ipaddr,neid;
    u_char macAddr[MAX_MAC_LENGTH] = {0};
    
    if(ifp == NULL)
    	return -1;
    /*回环接口，不需要，返回*/
    if(if_is_loopback(ifp))
    	return -1;
    
    set_dcn_link_manul (lp);
    set_dcn_link_devmodel (lp);
    /*获取接口MAC地址失败，返回*/
    ret = ospf_dcn_link_mac_address(lp->ifp, macAddr);
    if(ret != 0)
    	return -1;
    set_dcn_link_mac (lp,macAddr);
    /*获取接口IP地址失败，返回*/
    ipaddr.s_addr = ospf_dcn_link_ip_address(lp->ifp);
    if(ipaddr.s_addr == 0)
    	return -1;

	lp->ip_address.s_addr = ipaddr.s_addr;
	
	set_dcn_link_ipaddr (lp, &ipaddr);
	GT_DEBUG("initialize interface on dcn link: %s %s\n", ifp->name, inet_ntoa(ipaddr));

	neid.s_addr = ipaddr.s_addr;
	neid.s_addr &= 0xffffff00; /*Fisrt bit set 0*/
	set_dcn_link_neid (lp, &neid);
	/*初始化完毕，设置已经初始化标识*/
    lp->isInitialed = 1; /*Set Initial flag.*/
    
    return 0;
}
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_lookup_by_ifp
* declare	: lookup dcn link interface by zebra interface
* input	: 
* output	: 
* return	: 
* other	: 根据接口获取DCN LINK数据
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:25
*****************************************************************************/
static struct dcn_link *ospf_dcn_link_lookup_by_ifp (struct interface *ifp)
{
  struct listnode *node, *nnode;
  struct dcn_link *lp;
  if(ifp == NULL)
	  return NULL;
  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
    //if (lp && lp->ifp && lp->ifp->ifindex == ifp->ifindex)
	  if (lp && lp->ifp && lp->ifp == ifp)
      return lp;
  if(IS_OSPF_DCN_EVENT_DEBUG)
	  zlog_warn ("can not lookup dcn link by interface:%s", ifp->name);
  return NULL;
}
/*****************************************************************************
* function	: ospf_dcn_link_lookup_by_instance
* declare	: lookup dcn link interface by instance in lsa data
* input	: 
* output	: 
* return	: 
* other	: 根据LSA的instance获取DCN LINK数据
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:29
*****************************************************************************/
static struct dcn_link *ospf_dcn_link_lookup_by_instance (struct ospf_lsa *lsa)
{
  struct listnode *node;
  struct dcn_link *lp;
  unsigned int key = 0;
  if(lsa == NULL || lsa->data == NULL)
	  return NULL;
  key = GET_OPAQUE_ID (ntohl (lsa->data->id.s_addr));
  for (ALL_LIST_ELEMENTS_RO (OspfDcn.iflist, node, lp))
    if (lp && lp->instance == key)
      return lp;
  if(IS_OSPF_DCN_EVENT_DEBUG)
	  zlog_warn ("can not lookup dcn link by instance:0x%x", key);
  return NULL;
}
/*****************************************************************************
* function	: ospf_dcn_link_get_instance
* declare	: setting dcn link lsa instance and return it
* input	: 
* output	: 
* return	: 
* other	: 设置并返回LSA的instance
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:34
*****************************************************************************/
static u_int32_t ospf_dcn_link_get_instance (void)
{
#if 0	
  static u_int32_t seqno = 0;
  if (seqno < MAX_LEGAL_DCN_INSTANCE_NUM )
    seqno += 1;
  else
    seqno  = 1; /* Avoid zero. */
  seqno = 0x00ffee00;
  return seqno;
#endif 
  return OSPF_LEGAL_DCN_INSTANCE;
}
/*******************************************************************************************/
/*******************************************************************************************/
/*------------------------------------------------------------------------*
 * Followings are callback functions against generic Opaque-LSAs handling.
 *------------------------------------------------------------------------*/
/*****************************************************************************
* function	: ospf_dcn_link_new_hook
* declare	: create an interface function hook;in this function DCN module may be create an dcn link data 
* input	: 
* output	: 
* return	: 
* other	: 创建接口钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:43
*****************************************************************************/
static int ospf_dcn_link_new_hook (struct interface *ifp)
{
  struct dcn_link *lp = NULL;
  if(ifp == NULL)
	  return -1;
  
  if(if_is_loopback(ifp))
  	return -1;
  /*确认这个接口是否已经创建*/
  
  if (ospf_dcn_link_lookup_by_ifp (ifp) != NULL)
    {
      zlog_warn ("%s: %s already in use?", __func__,ifp->name);
      return -1;
    }
  
  lp = XCALLOC (MTYPE_OSPF_IF,sizeof (struct dcn_link));
  if (lp == NULL)
    {
      zlog_warn ("%s: XMALLOC: %s", __func__,safe_strerror (errno));
      return -1;
    }
  memset(lp, 0, sizeof(struct dcn_link));
  lp->area = NULL;
  lp->instance = ospf_dcn_link_get_instance ();
  lp->ifp = ifp;
  //lp->ifp->ifindex = ifp->ifindex;
  
  GT_DEBUG("%s:create an interface :%s\n",__func__,ifp->name);
  /*把接口加入DCN LINK数据链表*/
  listnode_add (OspfDcn.iflist, lp);
  ospf_dcn_link_initialize (lp);
  /* Schedule Opaque-LSA refresh. *//* XXX */
  return 0;
}
/*****************************************************************************
* function	: ospf_dcn_link_del_hook
* declare	: delete an interface function hook,in this function DCN module may be delete an dcn link data
* input	: 
* output	: 
* return	: 
* other	: 删除接口钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:50
*****************************************************************************/
static int ospf_dcn_link_del_hook (struct interface *ifp)
{
  struct dcn_link *lp;
  if(ifp == NULL)
	  return -1;
  if ((lp = ospf_dcn_link_lookup_by_ifp (ifp)) != NULL)
    {
      /* Dequeue listnode entry from the list. */
      listnode_delete (OspfDcn.iflist, lp);
      GT_DEBUG("%s:delete an interface :%s\n",__func__,ifp->name);
      /* Avoid misjudgement in the next lookup. */
      if (listcount (OspfDcn.iflist) == 0)
    	  OspfDcn.iflist->head = OspfDcn.iflist->tail = NULL;

      XFREE (MTYPE_OSPF_IF, lp);
    }
  /* Schedule Opaque-LSA refresh. *//* XXX */
  return 0;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_lookup
* declare	: 根据接口相关参数查找接口的本地LSA
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:50
*****************************************************************************/
static struct ospf_lsa *ospf_dcn_link_lsa_lookup(struct dcn_link *lp)
{
	struct in_addr lsa_id;
	struct ospf_lsa *lsa;
	if(lp == NULL || lp->area == NULL || lp->area->ospf == NULL)
	{
		GT_DEBUG("****************%s:lp =  %s \n",__func__,lp ? "lp":"NULL");
		GT_DEBUG("****************%s:area =  %s \n",__func__,lp->area ? "area":"NULL");
		GT_DEBUG("****************%s:ospf =  %s \n",__func__,lp->area->ospf ? "ospf":"NULL");
		return NULL;
	}
	lsa_id.s_addr = SET_OPAQUE_LSID (OPAQUE_TYPE_FLOODDCNINFO, lp->instance);
	lsa_id.s_addr = htonl (lsa_id.s_addr);
	lsa = ospf_lsa_lookup (lp->area, OSPF_OPAQUE_AREA_LSA, lsa_id, lp->area->ospf->router_id);
	if(lsa)
		return lsa;
	return NULL;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_local_flush
* declare	: 根据接口参数强制删除LSA,当LSA为空的时候表示删除本地LSA
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:50
*****************************************************************************/
static int ospf_dcn_link_lsa_local_flush(struct dcn_link *lp, struct ospf_lsa *lsa)
{
	int maxage_delay = 0;
	struct ospf_lsa *dcn_lsa = NULL;
	/*输入参数为空，表示删除本地LSA*/
	if(lsa == NULL)
		dcn_lsa = ospf_dcn_link_lsa_lookup(lp);
	else
		dcn_lsa = lsa;	
	
	if(dcn_lsa && lp && lp->ifp && lp->area && lp->area->ospf)
	{
		/* Unregister LSA from Refresh queue. */
		//ospf_refresher_unregister_lsa (lp->area->ospf, lsa);
		/* Flush AS-external-LSA through AS. */
		//修改MAX AGE延时时间，使得可以快速把LSA删除
		maxage_delay = lp->area->ospf->maxage_delay;
		lp->area->ospf->maxage_delay = 1;
		ospf_lsa_flush(lp->area->ospf, dcn_lsa);
		lp->area->ospf->maxage_delay = maxage_delay;
		lp->install_lsa = 0;
		//事件调度设置完毕后要恢复MAX AGE 时间
		GT_DEBUG("****************%s:if %s \n",__func__,lp->ifp->name);
		return 0;
	}
	return -1;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_flush
* declare	: 根据接口参数强制删除接口LSA,
* input	: 当type=1表示删除所有，当type=0表示删除本地，type=2表示删除非本地的
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:50
*****************************************************************************/
static int ospf_dcn_link_lsa_flush(struct dcn_link *lp, int type)
{
	struct prefix_ls lps;	
	struct route_table *table = NULL;
	struct route_node *rn = NULL;
	struct route_node *start = NULL;
	struct ospf_lsa *lsa = NULL;
	if(lp == NULL || lp->area == NULL || lp->area->lsdb == NULL || /*lp->ifp == NULL || */lp->ifp == NULL)
		return 0;
	/*删除本地LSA*/
	if(type == 0)
		return ospf_dcn_link_lsa_local_flush(lp, NULL);
	
	memset (&lps, 0, sizeof (struct prefix_ls));
	lps.family = 0;
	lps.prefixlen = 0;
	lps.id.s_addr = 0;
	lps.adv_router.s_addr = 0;

	/*获取LSDB节点*/
	table = AREA_LSDB (lp->area, OSPF_OPAQUE_AREA_LSA);
	if(table)
	{
		start = route_node_get (table, (struct prefix *) &lps);
		if (start)
		{
			route_lock_node (start);//在LSDB链表里遍历
			for (rn = start; rn; rn = route_next_until (rn, start))
			{
				if (!(lsa = rn->info))
					continue;
			        
				if(OPAQUE_TYPE_FLOODDCNINFO != GET_OPAQUE_TYPE (ntohl (lsa->data->id.s_addr)))
					continue;
				/*找到OSPF DCN的LSA*/
				if(type == 2)
				{
					/*删除接口内所有非本地LSA*/
					if(lp->area->ospf->router_id.s_addr != lsa->data->adv_router.s_addr)
						ospf_dcn_link_lsa_local_flush(lp, lsa);
				}
				/*删除接口内所有LSA*/
				else
					ospf_dcn_link_lsa_local_flush(lp, lsa);
			} 
			route_unlock_node (start);
		}
	}//
	GT_DEBUG("********************%s:if %s \n",__func__,lp->ifp->name);
	return 0;
}
/*****************************************************************************
* function	: ospf_dcn_link_ism_change_hook_handle
* declare	: when an ospf interface state is change,this hook well be call an handle DCN link LSA state 
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:55
*****************************************************************************/
static void ospf_dcn_link_ism_change_hook_handle (struct ospf_interface *oi, int old_state,  struct dcn_link *lp)
{
	  if(oi == NULL || oi->ifp == NULL || lp == NULL || lp->ifp == NULL)
		  return;
	  if(lp->area == NULL)
	  {
		  lp->area = oi->area;
		  //安装LSA
		  if(lp->isInitialed && lp->install_lsa == 0 && lp->area && lp->enable == 1 )
		  {
			  GT_DEBUG("%s: frist install LSA on: %s\n",__func__,oi->ifp->name);
			  ospf_dcn_link_lsa_install (lp->area, lp, 1);//install DCN LSA into ospf LSDB
			  lp->install_lsa = 1;
		  }
	  }
	  else if (lp->area != NULL && oi->area != NULL && lp->enable == 1) 
	  {/*
		  if(! IPV4_ADDR_SAME (&lp->area->area_id, &oi->area->area_id) )
		  {
			  //delete old LSA
			  GT_DEBUG("%s: %s change area id\n",__func__,IF_NAME (oi));
			  if(lp->install_lsa == 1)
			  {
				  ospf_dcn_link_lsa_local_flush(lp);
				  lp->install_lsa = 0;
			  }
			  lp->area = oi->area;
			  if(lp->area && lp->install_lsa == 0)
			  {
				  ospf_dcn_link_lsa_install (lp->area, lp, 1);//install DCN LSA into ospf LSDB
				  lp->install_lsa = 1;
			  }
		  }*/
	  }
	  else if(lp->area != NULL && oi->area == NULL && lp->enable == 1)	
	  {
		  //delete LSA
		  GT_DEBUG("%s: %s delete area id\n",__func__,IF_NAME (oi));
		  if(lp->install_lsa == 1)
		  {
			  ospf_dcn_link_lsa_flush(lp, DELETE_OTHER_LSA);/*删除接口内所有非本地LSA*/
			  //lp->install_lsa = 0;
		  }
		  lp->area = NULL;
	  }
	  GT_DEBUG("%s: chk if io ip address is change\n",__func__);
	  if( (ospf_dcn_link_address(oi) != lp->ip_address.s_addr )&&(lp->enable == 1) )// 
	  {
		  //1 -> 3
		  GT_DEBUG("%s: %s ip address change\n",__func__,IF_NAME (oi));
		  if(lp->install_lsa == 1)
		  {
			  ospf_dcn_link_lsa_flush(lp, DELETE_ALL_LSA);/*删除接口内所有LSA*/
			  lp->install_lsa = 0;
		  }
		  else //if(lp->isInitialed == 1 && lp->install_lsa == 0)
		  {
			  if(lp->isInitialed == 1)
			  {
				  ospf_dcn_link_lsa_install (lp->area, lp, 1);
				  lp->install_lsa = 1;
			  }
		  }
		  lp->ip_address.s_addr = ospf_dcn_link_address(oi);
	  }	
	  return;
}
/*****************************************************************************
* function	: ospf_dcn_link_ism_change_hook
* declare	: when an ospf interface state is change,this hook well be call an handle DCN link LSA state 
* input	: 
* output	: 
* return	: 
* other	: 接口状态变化钩子函数，在OSPF接口状态发生变化的时候会调用这个函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:32:55
*****************************************************************************/
void ospf_dcn_link_ism_change_hook (struct ospf_interface *oi, int old_state)
{
  struct dcn_link *lp = NULL;
  
  if(oi == NULL || oi->ifp == NULL /*|| lp->ifp->name == NULL*/)
	  return;
  
  GT_DEBUG("%s: -----------------------------------------:%s\n ",__func__,oi->ifp->name);
  if(if_is_loopback(oi->ifp))
  	return;  
  /*DCN功能没有启动，返回*/
  if (OspfDcn.status == disabled)
  {
	  GT_DEBUG ("%s: DCN is disabled now on %s.\n",__func__,oi->ifp->name);
      return ;
  }
  GT_DEBUG("%s: ==========================================:%s\n ",__func__,oi->ifp->name);
  
  if ((lp = ospf_dcn_link_lookup_by_ifp (oi->ifp)) == NULL)
    {
      zlog_warn ("%s: Cannot get %s dcn link ?", __func__,oi->ifp->name);
      return;
    }
  
  if (oi->area == NULL || oi->area->ospf == NULL)
    {
      zlog_warn ("%s: Cannot refer to OSPF from OI(%s)?",__func__,IF_NAME (oi));
      return;
    }
  GT_DEBUG("%s: +++++++++++++++++++++++++++++++++++++++++++:%s\n ",__func__,lp->ifp->name);
  //return ;
#ifdef notyet
  if ((lp->area != NULL
  &&   ! IPV4_ADDR_SAME (&lp->area->area_id, &oi->area->area_id))
  || (lp->area != NULL && oi->area == NULL))
    {
      /* How should we consider this case? */
      zlog_warn ("DCN: Area for OI(%s) has changed to [%s], flush previous LSAs", IF_NAME (oi), oi->area ? inet_ntoa (oi->area->area_id) : "N/A");
      ospf_dcn_link_lsa_schedule (lp, FLUSH_THIS_LSA);
    }
#endif
  
  GT_DEBUG("*************************************\n");
  GT_DEBUG("%s:if %s state: %d->%d\n",__func__,IF_NAME (oi),old_state,oi->state);
  GT_DEBUG("*************************************\n");	
  ospf_dcn_link_ism_change_hook_handle (oi,  old_state,  lp);
  switch (oi->state)
    {
  /*
  * 外部修改IP地址：
  * 当前状态为：state >= ISM_Waiting(3) 先直接转为ISM_Down（1）；执行default分支功能，删除LSA
  * IP地址变化后，在回调这个钩子函数，执行if(ospf_dcn_link_address(oi) != lp->ip_address.s_addr)分支
  */
    case ISM_PointToPoint:
    case ISM_DROther:
    case ISM_Backup:
    case ISM_DR:
    	//当dcn link参数发生变化更新LSA或重新安装
    	GT_DEBUG("%s: --------------- interface state change:%s ",__func__,lp->ifp->name);
    	GT_DEBUG("lp->install_lsa = %d,lp->enable=%d,lp->area=%s\n",lp->install_lsa,lp->enable,lp->area? "FULL":"NULL");
    	if(lp->area && lp->install_lsa == 0 && lp->enable == 1)
    	{/*当前接口使能LSA，并且接口已经初始化完毕，安装本地LSA*/
    		if(lp->isInitialed)
    		{
    			GT_DEBUG("%s: %s interface state change: REORIGINATE_PER_AREA \n",__func__,IF_NAME (oi));
    			//ospf_dcn_link_lsa_schedule (lp, REORIGINATE_PER_AREA);
    			ospf_dcn_link_lsa_install (lp->area, lp, 1);
    			lp->install_lsa = 1;
    		}
    	}
    	else if(lp->area && lp->install_lsa == 1 && lp->enable == 1)
    	{
    		/* 接口已经安装LSA，刷新LSA数据*/
    		GT_DEBUG("%s: %s interface state change: REFRESH_THIS_LSA\n",__func__,IF_NAME (oi));
    		ospf_dcn_link_lsa_schedule (lp, REFRESH_THIS_LSA);
    		//ospf_opaque_lsa_refresh_schedule: Invalid parameter?
    	}
    	//ospf_dcn_link_lsa_schedule (lp, REORIGINATE_PER_AREA);
    	break;
    default:
      {/* 当前接口已经安装LSA，现在接口状态发生变化，强行删除接口的所有LSA */
    	  if(lp->install_lsa == 1 && lp->area && lp->enable == 1 )
    	  {
    		  ospf_dcn_link_lsa_flush(lp, DELETE_OTHER_LSA);//删除所有非本地LSA
    		  //lp->install_lsa = 0;
    	  } 
      }
      break;
    }
  return;
}

/*****************************************************************************
* function	: ospf_dcn_link_nsm_change_hook
* declare	: when ospf nbr state is change,this hook well be call an handle DCN link LSA state 
* input	: 
* output	: 
* return	: 
* other	: 邻居状态变化钩子函数，邻居状态发生变化的时候会调用这个函数（网络断开链接的时候，或者对端关闭OSPF，这时需要删除邻居LSA），
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:3
*****************************************************************************/
static void ospf_dcn_link_nsm_change_hook (struct ospf_neighbor *nbr, int old_state)
{
	struct dcn_link *lp = NULL;
	if(nbr == NULL || nbr->oi == NULL || nbr->oi->ifp == NULL)
		  return;
	if (OspfDcn.status == disabled)
	{
		  GT_DEBUG ("%s: DCN is disabled now.\n",__func__);
	      return ;
	} 
	if(if_is_loopback(nbr->oi->ifp))
		return; 
    /* So far, nothing to do here. */	
	GT_DEBUG("*************************************\n");
	GT_DEBUG("%s:nbr state: %d->%d on %s\n",__func__,old_state,nbr->state, nbr->oi->ifp->name);//9->1
	GT_DEBUG("*************************************\n");
	
	switch (nbr->state)
	{
	case NSM_DependUpon:
	case NSM_Deleted:
	case NSM_Down:
	case NSM_Attempt:	
	case NSM_Init:
	case NSM_TwoWay:
	case NSM_ExStart:
		/*邻居状态发生变化后，要清除发生邻居状态变化的接口的相关LSA*/
		if(old_state >= NSM_Loading)
		{
			lp = ospf_dcn_link_lookup_by_ifp (nbr->oi->ifp);
			if (lp == NULL)
			{
				zlog_warn ("%s: Cannot get %s dcn link ?", __func__,nbr->oi->ifp->name);
				return;
			}
			ospf_dcn_link_lsa_flush(lp, DELETE_OTHER_LSA);//删除所有非本地LSA
		}
		break;

	case NSM_Exchange:
	case NSM_Loading:
	case NSM_Full:
		if(old_state <= NSM_Exchange)
		{
			lp = ospf_dcn_link_lookup_by_ifp (nbr->oi->ifp);
	    	if(lp && lp->area && lp->install_lsa == 0 && lp->enable == 1)
	    	{/*当前接口使能LSA，并且接口已经初始化完毕，安装本地LSA*/
	    		if(lp->isInitialed)
	    		{
	    			//ospf_dcn_link_lsa_schedule (lp, REORIGINATE_PER_AREA);
	    			ospf_dcn_link_lsa_install (lp->area, lp, 1);
	    			lp->install_lsa = 1;
	    		}
	    	}
		}
		break;
    default:
    	break;
	}
	return;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_originate_sublayer
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: 安装本地LSA并在区域内泛洪
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:12
*****************************************************************************/
static int ospf_dcn_link_lsa_originate_sublayer (struct ospf_area *area, struct dcn_link *lp)
{
	  struct ospf_lsa *new;
	  if(area == NULL || lp == NULL || lp->enable == 0)
			return -1; 
	  /* Create new Opaque-LSA/DCN instance. */
	  if ((new = ospf_dcn_link_lsa_new (area, lp)) == NULL)
	    {
	      zlog_warn ("%s: ospf_dcn_link_lsa_new() ?",__func__);
	      return -1;
	    }

	  /* Install this LSA into LSDB. */
	  if (ospf_lsa_install (area->ospf, NULL/*oi*/, new) == NULL)
	    {
	      zlog_warn ("%s: ospf_lsa_install() ?",__func__);
	      ospf_lsa_unlock (&new);
	      return -1;
	    }
	  lp->install_lsa = 1;
	  
	  GT_DEBUG("%s: install local lsa by interface :%s\n",__func__,lp->ifp->name);
	  /* Update new LSA origination count. */
	  area->ospf->lsa_originate_count++;
	  //lp->install_lsa = 1;
	  /* Flood new LSA through area. */
	  ospf_flood_through_area (area, NULL/*nbr*/, new);
	  
	  if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
	    {
	      char area_id[INET_ADDRSTRLEN];
	      strcpy (area_id, inet_ntoa (area->area_id));
	      zlog_debug ("LSA[Type%d:%s]: Originate Opaque-LSA/OSPF-DCN: Area(%s), Link(%s)", new->data->type, inet_ntoa (new->data->id), area_id, lp->ifp->name);
	      ospf_lsa_header_dump (new->data);
	    }
	  return 0;
}

/*****************************************************************************
* function	: ospf_dcn_link_lsa_originate_hook
* declare	: 创建OSPF DCN 的LSA并泛洪
* input	: 
* output	: 
* return	: 
* other	: 安装LSA钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:16
*****************************************************************************/
static int ospf_dcn_link_lsa_originate_hook (void *arg)
{
  struct ospf_area *area = (struct ospf_area *) arg;
  struct listnode *node, *nnode;
  struct dcn_link *lp;
  
  GT_DEBUG ("%s:  \n", __func__);
  if(area == NULL)
	  return -1; 
  
  if (OspfDcn.status == disabled)
    {
	  GT_DEBUG ("%s: DCN is disabled now.\n",__func__);
      return -1;
    }

  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
    {
	  GT_DEBUG ("%s by %s  \n", __func__,lp->ifp ? lp->ifp->name : "?");
	  //GT_DEBUG ("%s by %s  \n", __func__,lp->ifp ? lp->ifp->name : "?");
	  //GT_DEBUG ("%s by %s  \n", __func__,lp->ifp ? lp->ifp->name : "?");
	  //GT_DEBUG ("%s by %s  \n", __func__,lp->ifp ? lp->ifp->name : "?");
	  //GT_DEBUG ("%s by %s  \n", __func__,lp->ifp ? lp->ifp->name : "?");
      if (lp->area == NULL)
        continue;
      
      if (! IPV4_ADDR_SAME (&lp->area->area_id, &area->area_id))
      {
        continue;
      }
      if (lp->isInitialed == 0)
        {
    	  GT_DEBUG ("ospf_dcn_lsa_originate: Link(%s) lacks some mandated DCN parameters.\n", lp->ifp ? lp->ifp->name : "?");
          continue;
        }
      if (lp->enable == 1)
        {
          if(lp->install_lsa == 1)//刷新
            {
        	  GT_DEBUG("%s: REFRESH_THIS_LSA:%s!\n", __func__,lp->ifp ? lp->ifp->name : "?");
              ospf_dcn_link_lsa_schedule (lp, REFRESH_THIS_LSA);
            }
          else
          {
        	  GT_DEBUG("%s: originate:%s!\n", __func__,lp->ifp ? lp->ifp->name : "?");
        	  ospf_dcn_link_lsa_originate_sublayer (area, lp);
          }
          continue;
        }
      /* Ok, let's try to originate an LSA for this area and Link. */
      //if (ospf_dcn_link_lsa_originate_sublayer (area, lp) != 0)
      //  return -1;
    }
  return 0;
}

/*****************************************************************************
* function	: ospf_dcn_link_lsa_refresh_hook
* declare	: 刷新OSPF DCN 的LSA数据钩子函数
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:22
*****************************************************************************/
struct ospf_lsa * ospf_dcn_link_lsa_refresh_hook (struct ospf_lsa *lsa)
{
  //int lsa_seqnum = 0;
  struct dcn_link *lp;
  struct ospf_area *area = lsa->area;
  struct ospf_lsa *new = NULL;
  if(lsa == NULL || lsa->area == NULL || lsa->data == NULL)
	  return NULL; 
  if (OspfDcn.status == disabled)//清除DCN功能
    {
      /*
       * This LSA must have flushed before due to DCN status change.
       * It seems a slip among routers in the routing domain.
       */
	  GT_DEBUG ("%s: DCN is disabled now.\n",__func__);
      lsa->data->ls_age = htons (OSPF_LSA_MAXAGE); /* Flush it anyway. */
    }

  /* At first, resolve lsa/lp relationship. */
  if ((lp = ospf_dcn_link_lookup_by_instance (lsa)) == NULL)
    {
	  GT_DEBUG ("ospf_dcn_lsa_refresh: Invalid parameter?\n");
      lsa->data->ls_age = htons (OSPF_LSA_MAXAGE); /* Flush it anyway. */
    }
  if(lp->ifp == NULL || lp->area == NULL)
	  return NULL;
  /* If the lsa's age reached to MaxAge, start flushing procedure. */
  if (IS_LSA_MAXAGE (lsa))
    {
      //清除安装标识
      //lp->install_lsa = 0;
      GT_DEBUG("%s:force flush\n",__func__);		//zhurish eidt
      ospf_dcn_link_lsa_local_flush(lp, NULL);/*删除接口内本地LSA*/
      lp->install_lsa = 0;
      //ospf_opaque_lsa_flush_schedule (lsa);
      return NULL;
    }

  /* Create new Opaque-LSA/OSPF-DCN instance. */
  if ((new = ospf_dcn_link_lsa_new (area, lp)) == NULL)
    {
      zlog_warn ("%s: ospf_dcn_link_lsa_new() ?",__func__);
      return NULL;
    }
  new->data->ls_seqnum = lsa_seqnum_increment (lsa);

  /* Install this LSA into LSDB. */
  /* Given "lsa" will be freed in the next function. */
  if (ospf_lsa_install (area->ospf, NULL/*oi*/, new) == NULL)
    {
      zlog_warn ("%s: ospf_lsa_install() ?",__func__);
      ospf_lsa_unlock (&new);
      return NULL;
    }

  /* Flood updated LSA through area. */
  ospf_flood_through_area (area, NULL/*nbr*/, new);

  /* Debug logging. */
  if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
    {
      zlog_debug ("LSA[Type%d:%s]: Refresh Opaque-LSA/OSPF-DCN",
		 new->data->type, inet_ntoa (new->data->id));
      ospf_lsa_header_dump (new->data);
    }
  GT_DEBUG("%s:refresh by %s\n",__func__,lp->ifp->name);
  return new; 
}

/*****************************************************************************
* function	: ospf_dcn_link_config_write_router_hook
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: OSPF DCN功能配置写文件钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:27
*****************************************************************************/
static void ospf_dcn_link_config_write_router_hook (struct vty *vty)
{
  if(vty == NULL)
	return; 	
  if (OspfDcn.status == enabled)
    {
      vty_out (vty, " ospf dcn%s", VTY_NEWLINE);
#ifdef OSPF_DCN_ROUTE_ID      
      if(OspfDcn.router_addr.value.s_addr != 0)
    	  vty_out (vty, "  ospf dcn router-address %s%s",
               inet_ntoa (OspfDcn.router_addr.value), VTY_NEWLINE);
#endif /*OSPF_DCN_ROUTE_ID*/
    }
  return;
}
/*****************************************************************************
* function	: ospf_dcn_link_config_write_if_hook
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: OSPF DCN功能接口配置写文件钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:32
*****************************************************************************/
static void ospf_dcn_link_config_write_if_hook (struct vty *vty, struct interface *ifp)
{
  struct dcn_link *lp;	
  if(vty == NULL || ifp == NULL)
	return; 
  lp = ospf_dcn_link_lookup_by_ifp (ifp);
  if ((OspfDcn.status == enabled) && (! if_is_loopback (ifp)) &&  ( lp != NULL) )
    {
	  if(lp->enable)
		  vty_out (vty, " ip ospf link dcn%s", VTY_NEWLINE);
 	  //vty_out (vty, "%s%s",lp->enable?" ip ospf link dcn": "", VTY_NEWLINE);
    }
  return;
}
/*****************************************************************************
* function	: ospf_dcn_link_config_write_if_hook
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: OSPF DCN功能debug配置写文件钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:32
*****************************************************************************/
static void ospf_dcn_link_config_debug_hook (struct vty *vty)
{
	if(vty == NULL)
		return; 	
	if(IS_OSPF_DCN_EVENT_DEBUG)
		vty_out (vty, "debug ospf dcn event%s", VTY_NEWLINE);
	if(IS_OSPF_DCN_LSA_DEBUG)
		vty_out (vty, "debug ospf dcn lsa%s", VTY_NEWLINE);
	if(IS_OSPF_DCN_LSA_D_DEBUG)
		vty_out (vty, "debug ospf dcn lsa detail%s", VTY_NEWLINE);
	if(IS_OSPF_DCN_LINK_DEBUG)
		vty_out (vty, "debug ospf dcn link%s", VTY_NEWLINE);	
	return;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_schedule
* declare	: 根据操作功能码调度OSPF DCN的LSA数据操作
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:38
*****************************************************************************/
static void ospf_dcn_link_lsa_schedule (struct dcn_link *lp, enum sched_opcode opcode)
{
  struct ospf_lsa lsa;
  struct ospf_lsa *dcn_lsa;  
  struct lsa_header lsah;
  u_int32_t tmp;
  
  if(lp == NULL || lp->area == NULL)
	return; 
  //struct opaque_info_per_type *oipt;
  //GT_DEBUG ("ospf_dcn_link_lsa_schedule: opcode %d by %s\r\n",opcode,lp->ifp->name);

  memset (&lsa, 0, sizeof (lsa));
  memset (&lsah, 0, sizeof (lsah));

  lsa.area = lp->area;
  lsa.data = &lsah;
  lsah.type = OSPF_OPAQUE_AREA_LSA;
  tmp = SET_OPAQUE_LSID (OPAQUE_TYPE_FLOODDCNINFO, lp->instance);
  lsah.id.s_addr = htonl (tmp);
  
  dcn_lsa = &lsa;//ospf_dcn_link_lsa_lookup(lp);
  
  switch (opcode)
    {
    case REORIGINATE_PER_AREA:
    	GT_DEBUG("%s:ospf_opaque_lsa_reoriginate_schedule\n",__func__);		//zhurish eidt	
    	ospf_opaque_lsa_reoriginate_schedule ((void *) lp->area,
          OSPF_OPAQUE_AREA_LSA, OPAQUE_TYPE_FLOODDCNINFO);
      break;
    case REFRESH_THIS_LSA:
    	GT_DEBUG("%s:ospf_opaque_lsa_refresh_schedule\n",__func__);		//zhurish eidt	
    	//dcn_lsa = ospf_dcn_link_lsa_lookup(lp);
    	ospf_opaque_lsa_refresh_schedule (dcn_lsa);
      break;
    case FLUSH_THIS_LSA:
    	GT_DEBUG("%s:ospf_opaque_lsa_flush_schedule\n",__func__);		//zhurish eidt	
    	ospf_opaque_lsa_flush_schedule (dcn_lsa);
      break;
    default:
      zlog_warn ("ospf_dcn_link_lsa_schedule: Unknown opcode (%u)", opcode);
      break;
    }
  return;
}
/* Create new opaque-LSA. */
/*****************************************************************************
* function	: ospf_dcn_link_lsa_new
* declare	: 创建OSPF DCN的LSA数据
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:48
*****************************************************************************/
static struct ospf_lsa * ospf_dcn_link_lsa_new (struct ospf_area *area, struct dcn_link *lp)
{
  struct stream *s;
  struct lsa_header *lsah;
  struct ospf_lsa *new = NULL;
  struct in_addr lsa_id;
  int options;
  u_int16_t length;
  
  if(area == NULL || lp == NULL || lp->ifp == NULL)
	return NULL; 
  if(if_is_loopback(lp->ifp))
  	return NULL;
  if(strcmp(lp->ifp->name,"lo")==0)
  	return NULL;  
  /* Create a stream for LSA. */
  if ((s = stream_new (OSPF_MAX_LSA_SIZE)) == NULL)
    {
      zlog_warn ("%s: stream_new() ?",__func__);
      return NULL;
    }
  lsah = (struct lsa_header *) STREAM_DATA (s);

  options  = LSA_OPTIONS_GET (area);
  options |= LSA_OPTIONS_NSSA_GET (area);
  options |= OSPF_OPTION_O; /* Don't forget this :-) */
  
  lsa_id.s_addr = SET_OPAQUE_LSID (OPAQUE_TYPE_FLOODDCNINFO, lp->instance);
  lsa_id.s_addr = htonl (lsa_id.s_addr);

  if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
    zlog_debug ("LSA[Type%d:%s]: Create an Opaque-LSA/OSPF-DCN instance", OSPF_OPAQUE_AREA_LSA, inet_ntoa (lsa_id));

  /* Set opaque-LSA header fields. */
  lsa_header_set (s, options, OSPF_OPAQUE_AREA_LSA, lsa_id, area->ospf->router_id);

  /* Set opaque-LSA body fields. */
  ospf_dcn_link_build_lsa_tlv (s, lp);

  /* Set length. */
  length = stream_get_endp (s);
  lsah->length = htons (length);
  
  if (IS_DEBUG_OSPF (lsa, LSA_GENERATE))
    zlog_debug ("LSA: lsah->length %d", length);

  /* Now, create an OSPF LSA instance. */
  if ((new = ospf_lsa_new ()) == NULL)
    {
      zlog_warn ("%s: ospf_lsa_new() ?",__func__);
      stream_free (s);
      return NULL;
    }
  if ((new->data = ospf_lsa_data_new (length)) == NULL)
    {
      zlog_warn ("%s: ospf_lsa_data_new() ?",__func__);
      ospf_lsa_unlock (&new);
      stream_free (s);
      return NULL;
    }
  new->area = area;
  SET_FLAG (new->flags, OSPF_LSA_SELF);
  memcpy (new->data, lsah, length);
  stream_free (s);
  return new;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_install
* declare	: 向本地LSDB安装OSPF DCN 的 LSA（仅仅负责安装到本地LSDB）
* input	: 
* output	: 
* return	: 
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:55
*****************************************************************************/
static struct ospf_lsa * ospf_dcn_link_lsa_install (struct ospf_area *area, struct dcn_link *lp, int flood)
{
  struct ospf_lsa *new;
  if(area == NULL || lp == NULL || lp->enable == 0)
		return NULL; 
  /* Create new Opaque-LSA/DCN instance. */
  if ((new = ospf_dcn_link_lsa_new (area, lp)) == NULL)
    {
      zlog_warn ("%s: ospf_dcn_link_lsa_new() ?",__func__);
      return NULL;
    }

  /* Install this LSA into LSDB. */
  if (ospf_lsa_install (area->ospf, NULL/*oi*/, new) == NULL)
    {
      zlog_warn ("%s: ospf_lsa_install() ?",__func__);
      ospf_lsa_unlock (&new);
      return NULL;
    }
  GT_DEBUG("%s: install local lsa by interface :%s\n",__func__,lp->ifp->name);
  return new;
}
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_send_lsa_req
* declare	: DCN LINK接口发送LSA REQ报文
* input	: event 功能码（未使用）
* output	: 
* return	: 
* other	: 接口发送LSR报文，
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:55
*****************************************************************************/
/*******************************************************************************************/
static int ospf_dcn_link_send_lsa_req(struct dcn_link *lp, enum sched_event event)
{
	struct ospf_lsa *new = NULL;  
	struct lsa_header lsah;
	struct ospf_neighbor *nbr = NULL;
	struct ospf_interface *oi = NULL;
	struct route_node *rn = NULL;
	
	if(lp == NULL || lp->ifp == NULL || lp->area == NULL || lp->area->ospf == NULL)
		return; 
	//struct opaque_info_per_type *oipt;
	GT_DEBUG ("%s:  chk it's need to send LSR on %s\n",__func__,lp->ifp->name);

	oi = ospf_if_lookup_by_ifp (lp->area->ospf, lp->ifp);
	
	if(oi && oi->nbrs)
	{
		memset (&lsah, 0, sizeof (lsah));
		lsah.type = OSPF_OPAQUE_AREA_LSA;
		lsah.id.s_addr = SET_OPAQUE_LSID (OPAQUE_TYPE_FLOODDCNINFO, lp->instance);
		lsah.id.s_addr = htonl (lsah.id.s_addr);
		new = ospf_ls_request_new (&lsah); 
		
		for (rn = route_top (oi->nbrs); rn; rn = route_next (rn))
		{
			if ((nbr = rn->info) != NULL)
			{
				//route_unlock_node(rn);
				/*
				 * NSM_Exchange,NSM_Loading,NSM_Full
				 * 邻居状态大于NSM_Loading的 时候需要发送LSR报文
				 */
				if( (nbr->state >= NSM_Loading)&&(nbr->state < OSPF_NSM_STATE_MAX) )
				{
					ospf_ls_request_add (nbr, new);
					GT_DEBUG ("%s:sending LSR on %s\n",__func__,lp->ifp->name);
					//ospf_ls_req_event(nbr);
				}
			}
		}
	}
	return 0;
}
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_foreach_area
* declare	: 
* input	: event：操作功能码；func回调函数（未使用）
* output	: 
* return	: 
* other	: 遍历OSPF DCN LINK接口链表并根据参数event做相关操作
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:55
*****************************************************************************/
static int ospf_dcn_link_foreach_area (enum sched_event event,  void (*func)(void *, void *))
{
	struct listnode *node = NULL;
	struct listnode *nnode = NULL;
	struct dcn_link *lp = NULL;
	//struct ospf_neighbor *nbr = NULL;
	//struct ospf_interface *oi = NULL;
	
	for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
	{
		if(lp == NULL || lp->area == NULL)
			continue;
		if(event == UPDATE_LINK_LSA)/*刷新本地LSA*/
		{
			ospf_dcn_link_lsa_schedule(lp, REFRESH_THIS_LSA);
			//if(func)
			//	(* func)(lp, NULL);
			//continue;
		}
		else if(event == FLOOD_LINK_LSA)/*泛洪本地LSA*/
		{
		}
		else if(event == INSTALL_LINK_LSA)/*安装本地LSA*/
		{
		}
		else if(event == UPDATE_LINK_LSA_REQ)/*发送LSR请求*/
		{
			ospf_dcn_link_send_lsa_req(lp, UPDATE_LINK_LSA_REQ);
		}
		/*删除接口内所有本地LSA*/
		else if(event == DELETE_LOCAL_LSA)
			ospf_dcn_link_lsa_flush(lp, DELETE_LOCAL_LSA);
		/*删除接口内所有LSA*/
		else if(event == DELETE_ALL_LSA)
			ospf_dcn_link_lsa_flush(lp, DELETE_ALL_LSA);
		/*删除接口内所有非本地LSA*/
		else if(event == DELETE_OTHER_LSA)
			ospf_dcn_link_lsa_flush(lp, DELETE_OTHER_LSA);		
	}
	return 0;
}
/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_area_update
* declare	: 未使用
* input	: 
* output	: 
* return	: 
* other	: 遍历OSPF DCN LINK接口链表跟新接口区域号
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:55
*****************************************************************************/
static int ospf_dcn_link_area_update(void)
{
	  struct listnode *node, *nnode;
	  struct dcn_link *lp;
	  struct ospf_interface *oifp;
	  struct ospf *ospf = ospf_lookup ();
	  
	  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
	  {
		  oifp = ospf_if_lookup_by_ifp (ospf,lp->ifp);
		  if(oifp)
			  lp->area = oifp->area;
	  }
	  return 0;
}
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* 设地LSA数据操作函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:33:55
*****************************************************************************/
static void build_tlv_header (struct stream *s, struct dcn_tlv_header *tlvh)
{
  stream_put (s, tlvh, sizeof (struct dcn_tlv_header));
  return;
}

#ifdef OSPF_DCN_ROUTE_ID
static void build_router_tlv (struct stream *s)
{
  struct dcn_tlv_header *tlvh = &OspfDcn.router_addr.header;
  if (ntohs (tlvh->type) != 0)
    {
      build_tlv_header (s, tlvh);
      stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
    }
  return;
}
#endif /*OSPF_DCN_ROUTE_ID*/

static void build_link_subtlv_manu (struct stream *s, struct dcn_link *lp)
{
  struct dcn_tlv_header *tlvh = &lp->lv_manul.header;
  if (ntohs (tlvh->type) != 0)
  {
    build_tlv_header (s, tlvh);
    stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
    if(IS_OSPF_DCN_LINK_DEBUG)
    {
    	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
    	GT_DEBUG("	ManualFactory: %s\n",lp->lv_manul.value);
    }
  }
  return;
}

static void build_link_subtlv_devmodel (struct stream *s, struct dcn_link *lp)
{
  struct dcn_tlv_header *tlvh = &lp->lv_devmodel.header;
  if (ntohs (tlvh->type) != 0)
    {
      build_tlv_header (s, tlvh);
      stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
      if(IS_OSPF_DCN_LINK_DEBUG)
      {
      	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
      	GT_DEBUG("	DevcieModel : %s\n",lp->lv_devmodel.value);
      }
    }
  return;
}

static void build_link_subtlv_mac (struct stream *s, struct dcn_link *lp)
{
  char *mac;	
  struct dcn_tlv_header *tlvh = &lp->lv_mac.header;
  if (ntohs (tlvh->type) != 0)
    {
      build_tlv_header (s, tlvh);
      stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
      if(IS_OSPF_DCN_LINK_DEBUG)
      {
    	mac = lp->lv_mac.mac.value;  
      	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
      	GT_DEBUG("	Router-MAC : %02x-%02x-%02x-%02x-%02x-%02x\n",
      			mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
      }
    }
  return;
}

static void build_link_subtlv_neId (struct stream *s, struct dcn_link *lp)
{
  struct dcn_tlv_header *tlvh = &lp->lv_neid.header;
  if (ntohs (tlvh->type) != 0)
    {
      build_tlv_header (s, tlvh);
      stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
      if(IS_OSPF_DCN_LINK_DEBUG)
      { 
      	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
      	GT_DEBUG("	Router-NEID : %s\n",inet_ntoa(lp->lv_neid.value[0]));
      }
    }
  return;
}

static void build_link_subtlv_ipaddr (struct stream *s, struct dcn_link *lp)
{
  struct dcn_tlv_header *tlvh = &lp->lv_ipaddr.header;
  if (ntohs (tlvh->type) != 0)
    {
      build_tlv_header (s, tlvh);
      stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
      if(IS_OSPF_DCN_LINK_DEBUG)
      { 
      	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
      	GT_DEBUG("	Router-IP IPv4 : %s\n",inet_ntoa(lp->lv_ipaddr.value[0]));
      }
    }
  return;
}
#ifdef HAVE_IPV6
static void build_link_subtlv_ipaddrv6 (struct stream *s, struct dcn_link *lp)
{
  struct dcn_tlv_header *tlvh = &lp->lv_ipaddrv6.header;
  if (ntohs (tlvh->type) != 0)
    {
      build_tlv_header (s, tlvh);
      stream_put (s, tlvh+1, DCN_TLV_BODY_SIZE (tlvh));
      if(IS_OSPF_DCN_LINK_DEBUG)
      { 
      	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
      	GT_DEBUG("	Router-IP IPv6 : %s\n",inet6_ntoa(lp->lv_ipaddrv6.value[0]));
      }
    }
  return;
}
#endif /* HAVE_IPV6 */
/*****************************************************************************
* function	: ospf_dcn_link_build_lsa_tlv
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: 设地接口的DCN  LSA相关数据
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:7
*****************************************************************************/
static void ospf_dcn_link_build_lsa_tlv (struct stream *s, struct dcn_link *lp)
{
  if(s == NULL || lp == NULL)
	  return;
  if(IS_OSPF_DCN_LINK_DEBUG)
  {
  	//GT_DEBUG("build_link_subtlv_manu: lenth=%d",DCN_TLV_BODY_SIZE (tlvh));
  	GT_DEBUG("OSPF DCN LSA :  %s\n",__func__);
  }  
  build_link_subtlv_manu(s, lp);
  build_link_subtlv_devmodel(s, lp);
  build_link_subtlv_mac(s, lp);
  build_link_subtlv_neId(s, lp);
  build_link_subtlv_ipaddr(s, lp);
#ifdef HAVE_IPV6  
  build_link_subtlv_ipaddrv6(s, lp);
#endif /* HAVE_IPV6 */  
  return;
}
/*******************************************************************************************/
/*******************************************************************************************/
#define MAX_BUFFER_SIZE 1024
/*****************************************************************************
* function	: show_vty_subtlv_manualfactory
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: 显示设备厂家信息
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:19
*****************************************************************************/
static u_int16_t show_vty_subtlv_manualfactory (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  char tmp[MAX_BUFFER_SIZE] = {0};
  struct dcn_link_subtlv_manu *top = (struct dcn_link_subtlv_manu *) tlvh;
  if(top == NULL)
	  return 0;
  if(ntohs(top->header.length) > MAX_BUFFER_SIZE)
  {
    zlog_debug ("    ManualFactory length too long: %d more than size %d", top->header.length,MAX_BUFFER_SIZE);
  }
  strncpy(tmp, top->value, MAX_BUFFER_SIZE);
  if (vty != NULL)
    vty_out (vty, "  ManualFactory: %s lenth %d%s", tmp, DCN_TLV_SIZE (tlvh), VTY_NEWLINE);
  else
    zlog_debug ("    ManualFactory: %s", tmp);
  return DCN_TLV_SIZE (tlvh);
}
/*****************************************************************************
* function	: show_vty_subtlv_devmodel
* declare	: 
* input	: 
* output	: 
* return	: 
* other	: 显示设备型号信息
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:19
*****************************************************************************/
static u_int16_t show_vty_subtlv_devmodel (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  char tmp[MAX_BUFFER_SIZE] = {0};
  struct dcn_link_subtlv_devmodel *top = (struct dcn_link_subtlv_devmodel *) tlvh;
  if(top == NULL)
	  return 0;
  if(ntohs(top->header.length) > MAX_BUFFER_SIZE)
  {
    zlog_debug ("    ManualFactory length too long: %d more than size %d", top->header.length,MAX_BUFFER_SIZE);
  }

  strncpy(tmp, top->value, MAX_BUFFER_SIZE);
  
  if (vty != NULL)
    vty_out (vty, "  DevcieModel: %s lenth %d%s", tmp, DCN_TLV_SIZE (tlvh),VTY_NEWLINE);
  else
    zlog_debug ("    DevcieModel: %s", tmp);

  return DCN_TLV_SIZE (tlvh);
}

static u_int16_t show_vty_subtlv_mac (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  char tmp[MAX_BUFFER_SIZE] = {0};
  struct dcn_link_subtlv_mac *top = (struct dcn_link_subtlv_mac *) tlvh;
  if(top == NULL)
	  return 0;
  sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x", 
		  top->mac.value[0],top->mac.value[1],top->mac.value[2],
		  top->mac.value[3],top->mac.value[4],top->mac.value[5]);
  
  if (vty != NULL)
    vty_out (vty, "  Router-MAC: %s lenth %d%s", tmp, DCN_TLV_SIZE (tlvh), VTY_NEWLINE);
  else
    zlog_debug ("    Router-MAC: %s", tmp);

  return DCN_TLV_SIZE (tlvh);
}

static u_int16_t show_vty_subtlv_neId (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  struct dcn_link_subtlv_neid *top = (struct dcn_link_subtlv_neid*) tlvh;
  if(top == NULL)
	  return 0;
  if (vty != NULL)
    vty_out (vty, "  Router-NEID: %s lenth %d%s", inet_ntoa (top->value[0]), DCN_TLV_SIZE (tlvh), VTY_NEWLINE);
  else
    zlog_debug ("    Router-NEID: %s", inet_ntoa (top->value[0]));

  return DCN_TLV_SIZE (tlvh);
}
static u_int16_t show_vty_subtlv_ipaddr (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  struct dcn_link_subtlv_ipaddr *top = (struct dcn_link_subtlv_ipaddr*) tlvh;
  if(top == NULL)
	  return 0;
  if (vty != NULL)
    vty_out (vty, "  Router-IP: %s lenth %d%s", inet_ntoa (top->value[0]), DCN_TLV_SIZE (tlvh), VTY_NEWLINE);
  else
    zlog_debug ("    Router-IP: %s", inet_ntoa (top->value[0]));

  return DCN_TLV_SIZE (tlvh);
}

#ifdef OSPF_DCN_ROUTE_ID
static u_int16_t show_vty_router_addr (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  struct dcn_tlv_router_addr *top = (struct dcn_tlv_router_addr *) tlvh;
  if(top == NULL)
	  return 0;
  if (vty != NULL)
    vty_out (vty, "  Router-Address: %s%s", inet_ntoa (top->value), VTY_NEWLINE);
  else
    zlog_debug ("    Router-Address: %s", inet_ntoa (top->value));

  return DCN_TLV_SIZE (tlvh);
}
#endif /*OSPF_DCN_ROUTE_ID*/
/*****************************************************************************
* function	: show_vty_link_header
* declare	: 未使用
* input	: 
* output	: 
* return	: 
* other	: 显示LSA HEADER信息
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:19
*****************************************************************************/
static u_int16_t show_vty_link_header (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  struct dcn_tlv_link *top = (struct dcn_tlv_link *) tlvh;
  if(top == NULL)
	  return 0;
  if (vty != NULL)
    vty_out (vty, "  Link: %u octets of data%s", ntohs (top->header.length), VTY_NEWLINE);
  else
    zlog_debug ("    Link: %u octets of data", ntohs (top->header.length));

  return DCN_TLV_HDR_SIZE;	/* Here is special, not "TLV_SIZE". */
}

static u_int16_t show_vty_unknown_tlv (struct vty *vty, struct dcn_tlv_header *tlvh)
{
  if(tlvh == NULL)
	  return 0;	
  if (vty != NULL)
    vty_out (vty, "  Unknown TLV: [type(0x%x), length(0x%x)]%s", ntohs (tlvh->type), ntohs (tlvh->length), VTY_NEWLINE);
  else
    zlog_debug ("    Unknown TLV: [type(0x%x), length(0x%x)]", ntohs (tlvh->type), ntohs (tlvh->length));

  return DCN_TLV_SIZE (tlvh);
}
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_show_hook
* declare	: when exec cmd :show ip ospf database opaque-area ;this hook well be call to show DCN link infomation
* input	: 
* output	: 
* return	: 
* other	: 显示LSA信息钩子函数
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:28
*****************************************************************************/
static void ospf_dcn_link_show_hook (struct vty *vty, struct ospf_lsa *lsa)
{
  struct lsa_header *lsah = NULL;
  struct dcn_tlv_header *tlvh, *next;
  u_int16_t sum = 0, total = 0;
  if(lsa == NULL || vty == NULL)
	  return ;	
  lsah = (struct lsa_header *) lsa->data;
  total = ntohs (lsah->length) - OSPF_LSA_HEADER_SIZE;

  for (tlvh = DCN_TLV_HDR_TOP (lsah); sum < total;
			tlvh = (next ? next : DCN_TLV_HDR_NEXT (tlvh)))
    {
	    next = NULL;
        switch (ntohs (tlvh->type))
        {
        case DCN_LINK_SUBTLV_MANUL:
          sum += show_vty_subtlv_manualfactory(vty, tlvh);
          break;
        case DCN_LINK_SUBTLV_DEVMODEL:
          sum += show_vty_subtlv_devmodel(vty, tlvh);
          break;
        case DCN_LINK_SUBTLV_MAC:
          sum += show_vty_subtlv_mac (vty, tlvh);
          break;
        case DCN_LINK_SUBTLV_NEID:
          sum += show_vty_subtlv_neId (vty, tlvh);
          break;
        case DCN_LINK_SUBTLV_IPADDR:
          sum += show_vty_subtlv_ipaddr (vty, tlvh);
          break;
        default:
          sum += show_vty_unknown_tlv (vty, tlvh);
          break;
        }
    }
  return;
}
/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/
/*******************************************************************************************/
/*****************************************************************************
* function	: ospf_dcn_link_lsa_show_detail
* declare	: use for cmd :show ip ospf dcn ;this func well be call by 'ospf_dcn_link_lsa_show'
* input	: 
* output	: 
* return	: 
* other	: 显示LSA列表信息，用于命令行 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:33
*****************************************************************************/
static void ospf_dcn_link_lsa_show_detail (struct vty *vty, struct ospf_lsa *lsa)
{
  struct lsa_header *lsah = NULL;
  struct dcn_tlv_header *tlvh, *next;
  u_int16_t sum = 0, total = 0;

  char manualFactory[MAX_BUFFER_SIZE] = {0};
  char devModel[MAX_BUFFER_SIZE] = {0};
  char mac[MAX_MAC_LENGTH*3] = {0};
  char ip[MAX_IP_STRING_LENGTH] = {0};
  char id[MAX_IP_STRING_LENGTH] = {0};
  u_int32_t lsid = 0;	
  
  if(lsa == NULL || vty == NULL)
  {
	  return ;	
  }
  lsah = (struct lsa_header *) lsa->data;
  lsid = ntohl (lsah->id.s_addr);

  if(OPAQUE_TYPE_FLOODDCNINFO != GET_OPAQUE_TYPE (lsid))
  {
	  //vty_out (vty, " define %x get %x nget %x %s",OPAQUE_TYPE_FLOODDCNINFO, GET_OPAQUE_TYPE (lsid), lsid, VTY_NEWLINE);
	  return;
  }
  total = ntohs (lsah->length) - OSPF_LSA_HEADER_SIZE;

  for (tlvh = DCN_TLV_HDR_TOP (lsah); sum < total;
		tlvh = (next ? next : DCN_TLV_HDR_NEXT (tlvh)))
  {
    next = NULL;
    switch (ntohs (tlvh->type))
    {
    case DCN_LINK_SUBTLV_MANUL:
      {
        struct dcn_link_subtlv_manu *top = (struct dcn_link_subtlv_manu *) tlvh;
        strncpy(manualFactory, top->value, MAX_BUFFER_SIZE);
        sum += DCN_TLV_SIZE (tlvh);;
      }
      break;
    case DCN_LINK_SUBTLV_DEVMODEL:
      {
        struct dcn_link_subtlv_devmodel *top = (struct dcn_link_subtlv_devmodel *) tlvh;
        strncpy(devModel, top->value, MAX_BUFFER_SIZE);
        sum += DCN_TLV_SIZE (tlvh);;
      }
      break;
    case DCN_LINK_SUBTLV_MAC:
      {
        struct dcn_link_subtlv_mac *top = (struct dcn_link_subtlv_mac *) tlvh;
        sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
              top->mac.value[0],top->mac.value[1],top->mac.value[2],
              top->mac.value[3],top->mac.value[4],top->mac.value[5]);

        sum += DCN_TLV_SIZE (tlvh);;
      }
      break;
    case DCN_LINK_SUBTLV_NEID:
      {
        struct dcn_link_subtlv_neid *top = (struct dcn_link_subtlv_neid *) tlvh;
        sprintf(id, "%s", inet_ntoa (top->value[0]));
        sum += DCN_TLV_SIZE (tlvh);;
      }
      break;
    case DCN_LINK_SUBTLV_IPADDR:
      {
        struct dcn_link_subtlv_ipaddr *top = (struct dcn_link_subtlv_ipaddr *) tlvh;
        sprintf(ip, "%s", inet_ntoa (top->value[0]));
        sum += DCN_TLV_SIZE (tlvh);;
      }
      break;

    default:
      sum += DCN_TLV_SIZE (tlvh);;
      break;
    }
  }
  if(strlen(ip) && strlen(manualFactory))
  {
    vty_out (vty, "%-16s %-16s %-18s %-32s %-32s %s", 
      id, 
      ip,
      mac,
      devModel,
      manualFactory,
      VTY_NEWLINE);
  }
  return;
}
/*
 * @Purpose show DCN NE-INFO
 * @note added by zkx, July 15 2016
 */
static void ospf_dcn_lsa_prefix_set (struct vty *vty, struct prefix_ls *lp, struct in_addr *id,
		     struct in_addr *adv_router)
{
  memset (lp, 0, sizeof (struct prefix_ls));
  lp->family = 0;
  if (id == NULL)
    lp->prefixlen = 0;
  else if (adv_router == NULL)
    {
      lp->prefixlen = 32;
      lp->id = *id;
    }
  else
    {
      lp->prefixlen = 64;
      lp->id = *id;
      lp->adv_router = *adv_router;
    }
  return;
}
/*****************************************************************************
* function	: ospf_dcn_link_lsa_show
* declare	: use for cmd :show ip ospf dcn ;this func well be call by 'show_ip_ospf_dcn_link'
* input	: 
* output	: 
* return	: 
* other	: 遍历LSDB链表显示DCN的LSA信息
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:45
*****************************************************************************/
static void ospf_dcn_link_lsa_show (struct vty *vty, struct route_table *rt)
{
  struct prefix_ls lps;
  struct route_node *rn = NULL, *start = NULL;
  struct ospf_lsa *lsa = NULL;

  if(vty == NULL || rt == NULL)
  {
	  return;
  }
  ospf_dcn_lsa_prefix_set (vty, &lps, 0, 0);
  start = route_node_get (rt, (struct prefix *) &lps);
  if (start)
  {
    route_lock_node (start);
    for (rn = start; rn; rn = route_next_until (rn, start))
    {
      lsa = rn->info;
      if ( lsa != NULL )
      {
    	  //vty_out(vty,"=================================== %s",VTY_NEWLINE);
    	  //show_opaque_info_detail (vty, lsa);
    	  ospf_dcn_link_lsa_show_detail(vty, lsa);
    	  //vty_out(vty,"=================================== %s",VTY_NEWLINE);
      }
    } 
    route_unlock_node (start);
  }
}

/* Show dcn element information
   -- if id is NULL then show all LSAs. */
static const char *show_database_desc[] =
{
  "unknown",
  "Router Link States",
  "Net Link States",
  "Summary Link States",
  "ASBR-Summary Link States",
  "AS External Link States",
  "Group Membership LSA",
  "NSSA-external Link States",
  "Type-8 LSA",
  "Link-Local Opaque-LSA",
  "Area-Local Opaque-LSA",
  "AS-external Opaque-LSA",
};
   
/*****************************************************************************
* function	: show_ip_ospf_dcn_link
* declare	: use for cmd :show ip ospf dcn ;this func well be call 
* input	: 
* output	: 
* return	: 
* other	: 遍历area链表显示LSDB信息
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:54
*****************************************************************************/
static void show_ip_ospf_dcn_link (struct vty *vty, struct ospf *ospf)
{
  int type;
  struct listnode *node;
  struct ospf_area *area;
  if(vty == NULL || ospf == NULL)
	  return;
  type = OSPF_OPAQUE_AREA_LSA;
  vty_out (vty, "                %s %s%s",show_database_desc[type],VTY_NEWLINE, VTY_NEWLINE);
  for (ALL_LIST_ELEMENTS_RO (ospf->areas, node, area))
  {
    vty_out (vty, "%s                %s (Area %s)%s%s",
             VTY_NEWLINE, show_database_desc[type],
             ospf_area_desc_string (area), VTY_NEWLINE, VTY_NEWLINE);
    vty_out (vty, "%-16s %-16s %-18s %-32s %-32s %s", 
      "NEID", 
      "IP ADDRESS",
      "MAC ADDRESS",
      "DEVICETYPE",
      "COMPANY",
      VTY_NEWLINE);

    ospf_dcn_link_lsa_show (vty, AREA_LSDB (area, OSPF_OPAQUE_AREA_LSA));
  }
}
/*****************************************************************************
* function	: show_dcn_link_interface_sub
* declare	: use for cmd:show ip ospf dcn interface ;and to show dcn interface information
* input	: 
* output	: 
* return	: 
* other	: 显示DCN接口状态信息
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:34:59
*****************************************************************************/
static int show_dcn_link_interface_sub (struct vty *vty, struct interface *ifp)
{
  struct dcn_link *lp;
  int is_up;
  char mac[64];
  char ipstr[64];
  char neistrp[64];
  if(vty == NULL || ifp == NULL)
	  return -1;  
  if ((lp = ospf_dcn_link_lookup_by_ifp (ifp)) == NULL)
    {
      vty_out (vty, "interface:%s is not in dcn link table%s", ifp->name,VTY_NEWLINE);
      return CMD_WARNING;
    }
    
  vty_out (vty, "%s is %s%s", ifp->name, ((is_up = if_is_operative(ifp)) ? "up" : "down"), VTY_NEWLINE);
  vty_out (vty, "  ifindex %u, MTU %u bytes, BW %u Kbit %s%s",
  	   ifp->ifindex, ifp->mtu, ifp->bandwidth, if_flag_dump(ifp->flags),
	   VTY_NEWLINE);
	   
  if (lp->enable == 0)
    {
      vty_out (vty, "  OSPF DCN not enabled on this interface%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
  else if (!is_up)
    {
      vty_out (vty, "  OSPF DCN is enabled, but not running on this interface%s", VTY_NEWLINE);
      return CMD_WARNING;
    }
    vty_out (vty, "  DCN instance %x %s", lp->instance ,VTY_NEWLINE);
	if(lp->area)	
		vty_out (vty, "  DCN OSPF area: %s %s", inet_ntoa(lp->area->area_id) ,VTY_NEWLINE);
    vty_out (vty, "  DCN LOCAL LSA : %s %s", (lp->install_lsa ? "install":"uninstall") ,VTY_NEWLINE);
	
    vty_out (vty, "  local manul : %s %s", lp->lv_manul.value,VTY_NEWLINE);
    vty_out (vty, "  local devmodel : %s %s", lp->lv_devmodel.value,VTY_NEWLINE);
	
    memset(mac, 0, sizeof(mac));
    memset(ipstr, 0, sizeof(ipstr));
    memset(neistrp, 0, sizeof(neistrp));	
    sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",lp->lv_mac.mac.value[0],
		lp->lv_mac.mac.value[1],
		lp->lv_mac.mac.value[2],
		lp->lv_mac.mac.value[3],
		lp->lv_mac.mac.value[4],
		lp->lv_mac.mac.value[5]);
	
    sprintf(neistrp, "%s", inet_ntoa(lp->lv_neid.value[0]));
    sprintf(ipstr, "%s", inet_ntoa(lp->lv_ipaddr.value[0]));
	
    vty_out (vty, "  local mac : %s %s", mac,VTY_NEWLINE);
    vty_out (vty, "  local neid : %s %s", neistrp,VTY_NEWLINE);
    vty_out (vty, "  local ipaddr : %s %s", ipstr,VTY_NEWLINE);
    return CMD_SUCCESS;
}
/*******************************************************************************************/
/*******************************************************************************************/
/* show ip ospf dcn interface 命令，显示DCN接口信息 */
DEFUN (show_ip_ospf_dcn_interface,
		show_ip_ospf_dcn_interface_cmd,
       "show ip ospf dcn interface [INTERFACE]",
       SHOW_STR
       IP_STR
       "OSPF information\n"
       "DCN information\n"
       "Interface information\n"
       "Interface name\n")
{
  struct interface *ifp;
  struct listnode *node, *nnode;

  /* Show All Interfaces. */
  if (argc == 0)
    {
      for (ALL_LIST_ELEMENTS (iflist, node, nnode, ifp))
        show_dcn_link_interface_sub (vty, ifp);
    }
  else
    {
      if ((ifp = if_lookup_by_name (argv[0])) == NULL)
        vty_out (vty, "No such interface name%s", VTY_NEWLINE);
      else
        show_dcn_link_interface_sub (vty, ifp);
    }
  return CMD_SUCCESS;
}

/*
* DCN elment-info
* added by zkx.
* 2016 July 15.
*/
/* show ip ospf dcn 命令，显示DCN数据库信息简要信息 */
DEFUN (show_ip_ospf_dcn,
       show_ip_ospf_dcn_cmd,
       "show ip ospf dcn",
       SHOW_STR
       IP_STR
       "OSPF interface commands\n"
       "Dcn lsa information\n")
{
  struct ospf *ospf;

  ospf = ospf_lookup ();
  if (ospf == NULL)
    {
      vty_out (vty, " OSPF Routing Process not enabled%s", VTY_NEWLINE);
      return CMD_SUCCESS;
    }

  vty_out (vty, "%s       OSPF Router with ID (%s)%s%s", VTY_NEWLINE,
           inet_ntoa (ospf->router_id), VTY_NEWLINE, VTY_NEWLINE);

  /* Show all LSA. */
  if (argc == 0)
  {
	  show_ip_ospf_dcn_link (vty, ospf);
	  return CMD_SUCCESS;
  }
  return CMD_SUCCESS;
}

/* ospf dcn 命令，全局使能 DCN功能 */
DEFUN (ospf_dcn,
       ospf_dcn_cmd,
       "ospf dcn",
       "OSPF interface commands\n"
       "Enable the DCN functionality\n")
{
  int ret = 0;	
  struct listnode *node, *nnode;
  struct dcn_link *lp;

  if (OspfDcn.status == enabled)
    return CMD_SUCCESS;

  //if (IS_DEBUG_OSPF_EVENT)
  GT_DEBUG ("ospf dcn\n");

  OspfDcn.status = enabled;
  
  //ospf_dcn_link_area_update();
  /*
   * Following code is intended to handle two cases;
   *
   * 1) DCN was disabled at startup time, but now become enabled.
   * 2) DCN was once enabled then disabled, and now enabled again.
   */
  /* 遍历每一个接口，确认接口已经启动dcn 并且没有安装本地LSA，安装本地LSA并设置安装标志 */
  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
  {
	  ret = 0;
	  if(lp == NULL)
		  continue;
	  if(lp->isInitialed == 0)
		  ret = ospf_dcn_link_initialize (lp);
	  if(lp->isInitialed == 1 && lp->enable && lp->area)
	  {
		  if(lp->install_lsa == 0)
		  {
			  //lp->install_lsa = 1;
			  GT_DEBUG("%s:install local LSA by %s\n",__func__,lp->ifp->name);
			  //install DCN LSA into ospf LSDB  
			  //ospf_dcn_link_lsa_install (lp->area, lp, 0);
			  /*
			   * 安装完毕后需要泛洪出去,在执行no ospf dcn 后再执行ospf dcn，没有实现LSA泛洪（请求）
			   * 只有本地LSA，没有学习到对端LSA,ospf_dcn_link_lsa_refresh_hook函数有被调用，
			   * 但是没有学习到对端信息(对端学习到自己的LSA，但是自己没有学习到对端)
			   */
			  ospf_dcn_link_lsa_schedule (lp, REORIGINATE_PER_AREA);
			  //ospf_dcn_link_send_lsa_req(lp, UPDATE_LINK_LSA_REQ);
		  }
		  else
		  {
			  ospf_dcn_link_lsa_schedule (lp, FLUSH_THIS_LSA);
			  GT_DEBUG("%s:uninstall local LSA by %s\n",__func__,lp->ifp->name);
		  }
	  }
	  else
	  {
		  GT_DEBUG("%s: unstall local LSA by isInitialed=%d enable=%d AREA = %s on %s\n",
				  __func__,lp->isInitialed,lp->enable,lp->area ? "FULL":"NULL",lp->ifp->name);
	  }
  }
  ospf_dcn_link_foreach_area (UPDATE_LINK_LSA_REQ,  NULL);
  return CMD_SUCCESS;
}

/* no ospf dcn 命令，全局去使能 DCN功能 */
DEFUN (no_ospf_dcn,
       no_ospf_dcn_cmd,
       "no ospf dcn",
       NO_STR
       "OSPF interface commands\n"
       "Configure DCN parameters\n")
{
  struct listnode *node, *nnode;
  struct dcn_link *lp;
  
  GT_DEBUG ("no ospf dcn\n");
  
  if (OspfDcn.status == disabled)
    return CMD_SUCCESS;
  //if (IS_DEBUG_OSPF_EVENT)
  /* 遍历每一个接口，清除安装本地LSA标志位，若是该接口启动了ospf，则清除该接口的LSA */
  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
  {
	  /*去使能OSPF DCN功能，删除所有本地缓存的DCN LSA*/
	  if(lp->install_lsa && lp && lp->enable && lp->area)
	  {
		  ospf_dcn_link_lsa_local_flush(lp, NULL);
        //ospf_dcn_link_lsa_schedule (lp, FLUSH_THIS_LSA);
		  lp->install_lsa = 0;
	  }
  }
  /*去使能OSPF DCN功能，删除所有缓存的DCN LSA（非本地）*/
  ospf_dcn_link_foreach_area (DELETE_OTHER_LSA,  NULL);
  /*遍历缓存的LSA，清除所有存在缓存区的数据*/
  OspfDcn.status = disabled;
  return CMD_SUCCESS;
}

/* ip ospf link dcn 命令，在接口下使能 DCN功能 */
DEFUN (ip_ospf_dcn_link,
       ip_ospf_dcn_link_cmd,
       "ip ospf link dcn",
       "IP Information\n"
       "OSPF interface commands\n"
       "Configure ospf interface link type\n"
       "set Link type for DCN purpose\n")
{
	struct interface *ifp = (struct interface *) vty->index;
	struct dcn_link *lp = NULL;

	if ((lp = ospf_dcn_link_lookup_by_ifp (ifp)) == NULL)
	{
		vty_out (vty, "interface:%s is not in dcn link table%s", ifp->name,VTY_NEWLINE);
		return CMD_WARNING;
    }
	if(lp->enable == 0)
	{
		lp->enable = 1;
		GT_DEBUG ("ip ospf link dcn on %s\n",lp->ifp->name);
#ifdef DCN_MAC_TABLE
		//"enp0s25"
		if(!if_is_loopback(lp->ifp))
			ospf_dcn_mac_init(lp->ifp->name);
#endif
		//GT_DEBUG ("ip ospf link dcn:to init DCN LINK\n");
		if(lp->isInitialed == 0)
			ospf_dcn_link_initialize(lp);//初始化接口数据
		if(lp->isInitialed)
		{
			if (lp->install_lsa == 0)
			{
				if(lp->area != NULL)//安装LSA
				{
					//install DCN LSA into ospf LSDB  
					//lp->install_lsa = 1;
					zlog_debug("%s:ospf_dcn_lsa_install_local",__FUNCTION__);
					//ospf_dcn_link_lsa_install (lp->area, lp, 1);
					//REORIGINATE_PER_AREA, REFRESH_THIS_LSA, FLUSH_THIS_LSA, INSTALL_THIS_LSA;
					ospf_dcn_link_lsa_schedule (lp, REORIGINATE_PER_AREA);
					//需要根据当前接口邻居状态进行泛洪操作（发送LSU）
					GT_DEBUG ("ip ospf link dcn on %s (request LSA Update)\n",ifp->name);
					ospf_dcn_link_send_lsa_req(lp, UPDATE_LINK_LSA_REQ);
				}
			}
			//ospf_dcn_link_foreach_area (UPDATE_LINK_LSA_REQ,  NULL);
		}
	}
	/*
	 * 安装完毕后需要泛洪出去,在执行no ip ospf link dcn 后再执行ip ospf link dcn，没有实现LSA泛洪（请求）
	 * 只有本地LSA，没有学习到对端LSA,ospf_dcn_link_lsa_refresh_hook函数有被调用，
	 * 但是没有学习到对端信息(对端学习到自己的LSA，但是自己没有学习到对端)
	 */
	//GT_DEBUG ("ip ospf link dcn on %s\n",ifp->name);

	return CMD_SUCCESS;
}
/* no ip ospf link dcn 命令，在接口下去使能 DCN功能 */
DEFUN (no_ip_ospf_dcn_link,
       no_ip_ospf_dcn_link_cmd,
       "no ip ospf link dcn",
       NO_STR
       IP_STR
       "OSPF interface commands\n"
       "Configure ospf interface link type\n"
       "set Link type for DCN purpose\n")
{
  struct interface *ifp = (struct interface *) vty->index;
  struct dcn_link *lp = NULL;
  if(OspfDcn.status == disabled)
  {
      vty_out (vty, "dcn is not enable%s", VTY_NEWLINE);
      return CMD_WARNING;
  }
  if ((lp = ospf_dcn_link_lookup_by_ifp (ifp)) == NULL)
    {
	  vty_out (vty, "interface:%s is not in dcn link table%s", ifp->name,VTY_NEWLINE);
      return CMD_WARNING;
    }
  if(lp->enable)
  {
	 // int maxage_delay = 0;
    /* 接口已经启动DCN功能，清除LSA并清除安装标志 */
    //lp->enable = 0;
    //ospf_dcn_link_initialize (lp);
    GT_DEBUG("ospf_dcn_link_lsa_local_flush by cmd: no ip ospf dcn link\n");
    /*
     * 需要告诉对端，删除LSA
     * 同时要删除本地存储的所有从这个接口学习到的
     */
   /*禁用DCN功能，删除所有LSA*/
    ospf_dcn_link_lsa_flush(lp, DELETE_ALL_LSA); 
    lp->install_lsa = 0;
    lp->enable = 0;
#ifdef DCN_MAC_TABLE
  if(!if_is_loopback(lp->ifp))
	ospf_dcn_mac_exit();
#endif
  }
  GT_DEBUG ("no ip ospf link dcn on %s\n",ifp->name);
  return CMD_SUCCESS;
}
/*
 * service device-model NAME命令，用于设置设备型号名称
 * 改命令安装在service节点下
 */
DEFUN (ospf_dcn_device,
		ospf_dcn_device_cmd,
       "service device-model NAME",
       "Set up miscellaneous service\n"
       "device model information\n"
       "device name for OSPF DCN module\n")
{
  struct listnode *node, *nnode;
  struct dcn_link *lp;	
  if(argc != 1)
  {
	  vty_out (vty, "something of argv wrong!%s", VTY_NEWLINE);  
	  return CMD_WARNING;
  }
  if(argv[0] == NULL || (strlen(argv[0])>MAX_MODEL_LENGTH) )
  {
	vty_out (vty, "something of argv len wrong!%s", VTY_NEWLINE);  
	return CMD_WARNING;
  }
  memset(CPE_DEVICEMODEL, 0, MAX_MODEL_LENGTH+1);
  strcpy(CPE_DEVICEMODEL,argv[0]);
  vty_out (vty, "setting device model name:%s%s", CPE_DEVICEMODEL, VTY_NEWLINE); 
  /*遍历每一个接口，*/
  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
  {
	  if(lp->isInitialed)//接口已经初始化，重新设置设备类型字段
	  {
		  set_dcn_link_devmodel (lp);
		  if(lp->install_lsa == 1)//接口安装LSA
		  {
			  //删除旧的LSA，安装新的LSA并泛洪
		  }
	  }
  }
  return CMD_SUCCESS;
}

static struct cmd_node dcn_node =
{
 SERVICE_NODE,
  "",
  1
};

static int config_write_ospf_dcn_device (struct vty *vty)
{
  vty_out (vty, "service device-model %s%s", CPE_DEVICEMODEL, VTY_NEWLINE); 
  return 1;
}

/* ospf dcn router-address A.B.C.D，配置DCN路由ID */
#ifdef OSPF_DCN_ROUTE_ID  
DEFUN (ospf_dcn_router_addr,
       ospf_dcn_router_addr_cmd,
       "ospf dcn router-address A.B.C.D",
       "OSPF interface commands\n"
       "DCN specific commands\n"
       "Stable IP address of the advertising router\n"
       "DCN router address in IPv4 address format\n")
{
  struct dcn_tlv_router_addr *ra = &OspfDcn.router_addr;
  struct in_addr value;

  if (! inet_aton (argv[0], &value))
    {
      vty_out (vty, "Please specify Router-Addr by A.B.C.D%s", VTY_NEWLINE);
      return CMD_WARNING;
    }

  if (ntohs (ra->header.type) == 0
      || ntohl (ra->value.s_addr) != ntohl (value.s_addr))
    {
      struct listnode *node, *nnode;
      struct dcn_link *lp;
      int need_to_reoriginate = 0;

      set_dcn_router_addr (value);

      if (OspfDcn.status == disabled)
        goto out;

      for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
        {
          if (lp->area == NULL)
            continue;

          if ((lp->flags & DCNFLG_LSA_ENGAGED) == 0)
            {
              need_to_reoriginate = 1;
              break;
            }
        }
      
      for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
        {
          if (lp->area == NULL)
            continue;

          if (need_to_reoriginate)
            lp->flags |= DCNFLG_LSA_FORCED_REFRESH;
          else
            ospf_dcn_link_lsa_schedule (lp, REFRESH_THIS_LSA);
        }

      //if (need_to_reoriginate)
      //  ospf_dcn_foreach_area (
      //      ospf_dcn_link_lsa_schedule, REORIGINATE_PER_AREA);
    }
out:
  return CMD_SUCCESS;
}	
/* show ip ospf dcn router 命令，显示DCN router-id 信息 */ 
DEFUN (show_ip_ospf_dcn_router,
		show_ip_ospf_dcn_router_cmd,
       "show ip ospf dcn router",
       SHOW_STR
       IP_STR
       "OSPF information\n"
       "DCN information\n"
       "router id information\n")
{
  if (OspfDcn.status == enabled)
    {
      vty_out (vty, "--- DCN router parameters ---%s",
               VTY_NEWLINE);

      if (ntohs (OspfDcn.router_addr.header.type) != 0)
        show_vty_router_addr (vty, &OspfDcn.router_addr.header);
      else if (vty != NULL)
        vty_out (vty, "  N/A%s", VTY_NEWLINE);
    }
  return CMD_SUCCESS;
}
#endif /*OSPF_DCN_ROUTE_ID*/

#ifdef GT_DCN_DEBUG
DEFUN (show_ip_ospf_dcn_table,
       show_ip_ospf_dcn_table_cmd,
       "show ip ospf dcn-table",
       SHOW_STR
       IP_STR
       "OSPF interface commands\n"
       "Configure ospf interface link type\n"
       "set Link type for DCN purpose\n")
{
      struct listnode *node, *nnode;
      struct dcn_link *lp;
      vty_out (vty, "*********************************%s",VTY_NEWLINE);
      if (OspfDcn.status == disabled)
    	  vty_out (vty, "dcn is not enable%s", VTY_NEWLINE);
      else
    	  vty_out (vty, "dcn is enable%s", VTY_NEWLINE);
      /* 遍历每一个接口，清除安装本地LSA标志位，若是该接口启动了ospf，则清除该接口的LSA */
      for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
      {
    	  vty_out (vty, "*********************************%s",VTY_NEWLINE);
    	  vty_out (vty, "dcn link interface  : %s %s", lp->ifp->name,VTY_NEWLINE);
    	  vty_out (vty, "dcn link isInitialed: %d %s", lp->isInitialed,VTY_NEWLINE);
    	  vty_out (vty, "dcn link enable     :%d %s", lp->enable,VTY_NEWLINE);
    	  vty_out (vty, "dcn link install_lsa: %d %s", lp->install_lsa,VTY_NEWLINE);
    	  if(lp->area)
    		  vty_out (vty, "dcn link area   :FULL%s", VTY_NEWLINE);
    	  else
    		  vty_out (vty, "dcn link area   :NULL %s", VTY_NEWLINE);
    	  vty_out (vty, "init dcn link interface:%d %s", ospf_dcn_link_initialize(lp),VTY_NEWLINE); 
      }
      return CMD_SUCCESS;
}
#endif /*GT_DCN_DEBUG*/

#ifdef DCN_MAC_TABLE
DEFUN (show_ip_ospf_dcn_mac_table,
       show_ip_ospf_dcn_mac_table_cmd,
       "show ip ospf dcn-mac-table",
       SHOW_STR
       IP_STR
       "OSPF interface commands\n"
       "DNC mac table\n")
{
	show_ospf_dcn_mac_table(vty);
	return CMD_SUCCESS;
}
#endif /*DCN_MAC_TABLE*/

static void ospf_dcn_register_vty (void)
{ 
  install_node (&dcn_node, config_write_ospf_dcn_device);
  install_element (CONFIG_NODE, &ospf_dcn_device_cmd);
	  
  install_element (VIEW_NODE, &show_ip_ospf_dcn_interface_cmd);
  install_element (ENABLE_NODE, &show_ip_ospf_dcn_interface_cmd);

  install_element (VIEW_NODE, &show_ip_ospf_dcn_cmd);
  install_element (ENABLE_NODE, &show_ip_ospf_dcn_cmd);

  install_element (OSPF_NODE, &ospf_dcn_cmd);
  install_element (OSPF_NODE, &no_ospf_dcn_cmd);

#ifdef GT_DCN_DEBUG
  install_element (VIEW_NODE, &show_ip_ospf_dcn_table_cmd);
  install_element (ENABLE_NODE, &show_ip_ospf_dcn_table_cmd);
#endif	   
#ifdef OSPF_DCN_ROUTE_ID  
  install_element (VIEW_NODE, &show_ip_ospf_dcn_cmd);
  install_element (ENABLE_NODE, &show_ip_ospf_dcn_cmd);  
  install_element (OSPF_NODE, &ospf_dcn_router_addr_cmd);
#endif /*OSPF_DCN_ROUTE_ID*/ 
  
  install_element (INTERFACE_NODE, &ip_ospf_dcn_link_cmd);
  install_element (INTERFACE_NODE, &no_ip_ospf_dcn_link_cmd);

#ifdef DCN_MAC_TABLE
  install_element (VIEW_NODE, &show_ip_ospf_dcn_mac_table_cmd);
  install_element (ENABLE_NODE, &show_ip_ospf_dcn_mac_table_cmd);
#endif /*DCN_MAC_TABLE*/
  return;
}

/****************************************************************************/
/*
ÏÂÃæÊÇÊµÏÖDCN¹ŠÄÜÐèÒªÐÞžÄµÄOSPFÄ£¿éµÄŽúÂë
*/
/****************************************************************************/

#include <net/if_arp.h>


struct dcn_link_nbr nbrmac;
/*
 * 获取root用户权限，并执行命令
 */
extern struct zebra_privs_t ospfd_privs;

static int super_system(const char *cmd)
{
	int ret = 0;
	if ( ospfd_privs.change (ZPRIVS_RAISE) )
		zlog_err ("ospf_sock_init: could not raise privs, %s",safe_strerror (errno) );
	  
	ret = system(cmd);
	  
	if ( ospfd_privs.change (ZPRIVS_LOWER) )
		zlog_err ("ospf_sock_init: could not lower privs, %s",safe_strerror (errno) );
	
	return ret;
}

struct ospf_interface * ospf_if_dcn_enable (struct ospf_interface *oi)
{
  struct listnode *node;
  struct listnode *nnode;
  struct dcn_link *lp;
  if(oi == NULL || oi->ifp == NULL)
	  return NULL;
  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
  {
	  if (lp && lp->ifp && lp->ifp == oi->ifp)
	  {
		  if(lp->enable)
			  return oi;
	  }
  }
  if(IS_OSPF_DCN_EVENT_DEBUG)
	  zlog_warn ("can not lookup dcn link by interface:%s", oi->ifp->name);
  return NULL;
}
/*
 * 获取已经启动DCN功能的接口的名称（目前只支持一个接口启动DCN）
 */
static char * ospf_dcn_link_name (void)
{
  struct listnode *node = NULL;
  struct listnode *nnode = NULL;
  struct dcn_link *lp = NULL;
  for (ALL_LIST_ELEMENTS (OspfDcn.iflist, node, nnode, lp))
    if (lp && lp->ifp && lp->enable && lp->isInitialed)
      return lp->ifp->name;
  return NULL;
}

#ifdef DCN_MAC_TABLE

#include <linux/if_ether.h>
#include <linux/if_packet.h>


#define OSPF_DCN_NBR_MAC_MAX	(128)

struct dcn_nbr_mac
{
	int initialed;
	unsigned char mac[ETH_ALEN];
	struct in_addr nbrIp;
};
static unsigned char own_addr[ETH_ALEN];

struct dcn_nbr_mac dcn_mac[OSPF_DCN_NBR_MAC_MAX];
extern struct thread_master *master;
static struct thread *t_dcn_thread = NULL;
static int dcn_sock = 0;

static int ospf_dcn_mac_lookup(unsigned long ip, char *mac)
{
	int i = 0;
	for(i = 0; i < OSPF_DCN_NBR_MAC_MAX; i++)
	{
		if( dcn_mac[i].initialed != 0 && dcn_mac[i].nbrIp.s_addr != 0 )
		{
			if(dcn_mac[i].nbrIp.s_addr == ip && memcmp(dcn_mac[i].mac, mac, ETH_ALEN))
				return 1;
		}
	}
	return 0;
}
static int ospf_dcn_mac_table(int type, unsigned long ip, char *mac)
{
	int i = 0;
	int ret = 0;
	if(type)
	{
		ret = ospf_dcn_mac_lookup(ip, mac);
		if(ret)
			return 1;
		//mdf
		for(i = 0; i < OSPF_DCN_NBR_MAC_MAX; i++)
		{
			if( dcn_mac[i].initialed != 0 && dcn_mac[i].nbrIp.s_addr != 0 )
			{
				if(dcn_mac[i].nbrIp.s_addr == ip)
				{
					dcn_mac[i].nbrIp.s_addr = ip;
					dcn_mac[i].initialed = 1;
					memcpy(dcn_mac[i].mac, mac, ETH_ALEN);
					return 0;
				}
			}
		}
		//add
		for(i = 0; i < OSPF_DCN_NBR_MAC_MAX; i++)
		{
			if( dcn_mac[i].initialed == 0)
			{
				dcn_mac[i].nbrIp.s_addr = ip;
				dcn_mac[i].initialed = 1;
				memcpy(dcn_mac[i].mac, mac, ETH_ALEN);
				return 0;
			}
		}
	}
	else
	{
		//del
		for(i = 0; i < OSPF_DCN_NBR_MAC_MAX; i++)
		{
			if( dcn_mac[i].initialed != 0 && dcn_mac[i].nbrIp.s_addr != 0 )
			{
				if(dcn_mac[i].nbrIp.s_addr == ip )
				{
					dcn_mac[i].nbrIp.s_addr = 0;
					dcn_mac[i].initialed = 0;
					memset(dcn_mac[i].mac, 0, ETH_ALEN);
					return 0;
				}
			}
		}
	}
	return 1;
}

static int ospf_dcn_monitor_read(struct thread *thread)
{
	int sock = 0;
	char buf[1024];
	int len = 0;
	struct ethhdr *hdr = buf;
	struct ip *iph = (struct ip *)(buf + sizeof(struct ethhdr));
	/* first of all get interface pointer. */
	//ospf = THREAD_ARG (thread);
	sock = THREAD_FD (thread);
	/* prepare for next packet. */
	t_dcn_thread = thread_add_read (master, ospf_dcn_monitor_read, NULL, sock);
	len = read(sock, buf, sizeof(buf));
	if(len > 64)
	{
		//if(iph->ip_p != IPPROTO_OSPFIGP)
		//	return 0;
		zlog_debug("%s:%s--%02x:%02x:%02x:%02x:%02x:%02x",__func__, inet_ntoa(iph->ip_src),
				hdr->h_source[0],hdr->h_source[1],hdr->h_source[2],hdr->h_source[3],
				hdr->h_source[4],hdr->h_source[5]);

		if(memcmp(own_addr, hdr->h_source, ETH_ALEN) == 0)
			return 0;
		ospf_dcn_mac_table(1, iph->ip_src.s_addr, hdr->h_source);
	}
	return 0;
}
static int ospf_mac_socket_init(const char *ifname)
{
	int sock = 0;
	struct ifreq ifr;
	struct sockaddr_ll ll;

	if ( ospfd_privs.change (ZPRIVS_RAISE) )
		zlog_err ("ospf_sock_init: could not raise privs, %s",safe_strerror (errno) );

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if (sock < 0)
	{
	      int save_errno = errno;
	      if ( ospfd_privs.change (ZPRIVS_LOWER) )
	    	  zlog_err ("ospf_sock_init: could not lower privs, %s",safe_strerror (errno) );
	      zlog_err ("ospf_read_sock_init: socket: %s", safe_strerror (save_errno));
	}
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
	{
		if ( ospfd_privs.change (ZPRIVS_LOWER) )
			zlog_err ("%s: could not lower privs, %s",__func__,strerror (errno) );
		zlog_err( "%s: ioctl[SIOCGIFINDEX]: %s",__func__, strerror(errno));
		close(sock);
		return NULL;
	}

	memset(&ll, 0, sizeof(ll));
	ll.sll_family = PF_PACKET;
	ifr.ifr_ifindex = ifr.ifr_ifindex;//ifname2ifindex(ifname);

	ll.sll_ifindex = ifr.ifr_ifindex;
	ll.sll_protocol = htons(ETH_P_IP);
	if (bind(sock, (struct sockaddr *) &ll, sizeof(ll)) < 0)
	{
		if ( ospfd_privs.change (ZPRIVS_LOWER) )
			zlog_err ("ospf_sock_init: could not lower privs, %s",safe_strerror (errno) );
		zlog_err( "%s: bind[PF_PACKET]: %s",__func__, strerror(errno));
		close(sock);
		return NULL;
	}
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
	{
		if ( ospfd_privs.change (ZPRIVS_LOWER) )
			zlog_err ("ospf_sock_init: could not lower privs, %s",safe_strerror (errno) );
		zlog_err( "%s: ioctl[SIOCGIFHWADDR]: %s",__func__, strerror(errno));
		close(sock);
		return NULL;
	}
	memcpy(own_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	if (ospfd_privs.change (ZPRIVS_LOWER))
	{
	      zlog_err ("ospf_sock_init: could not lower privs, %s",safe_strerror (errno) );
	}
	t_dcn_thread = thread_add_read (master, ospf_dcn_monitor_read, NULL, sock);
	dcn_sock = sock;
	return sock;
}
static int ospf_dcn_mac_init(const char *name)
{
	memset(dcn_mac, 0, sizeof(dcn_mac));
	memset(own_addr, 0, sizeof(own_addr));
	ospf_mac_socket_init(name);
	return 0;
}
static int ospf_dcn_mac_exit(void)
{
	if(t_dcn_thread)
		thread_cancel(t_dcn_thread);
	if(dcn_sock)
		close(dcn_sock);
	memset(own_addr, 0, sizeof(own_addr));
	memset(dcn_mac, 0, sizeof(dcn_mac));
	return 0;
}

static int show_ospf_dcn_mac_table(struct vty *vty)
{
	int i = 0;
	char mac_str[64];
	vty_out (vty, "*********************************%s",VTY_NEWLINE);

	//vty_out (vty, " own:%02x:%02x:%02x:%02x:%02x:%02x%s",own_addr[0],own_addr[1],
	//		own_addr[2],own_addr[3],own_addr[4],own_addr[5],VTY_NEWLINE);
	vty_out (vty, "address			mac		 %s",VTY_NEWLINE);
	for(i = 0; i < OSPF_DCN_NBR_MAC_MAX; i++)
	{
		if(dcn_mac[i].initialed)
		{
			memset(mac_str, 0, sizeof(mac_str));
			sprintf(mac_str,"%02x:%02x:%02x:%02x:%02x:%02x",
						dcn_mac[i].mac[0],dcn_mac[i].mac[1],dcn_mac[i].mac[2],
						dcn_mac[i].mac[3],dcn_mac[i].mac[4],dcn_mac[i].mac[5]);
			vty_out (vty, "%s		%s		 %s", inet_ntoa(dcn_mac[i].nbrIp),mac_str,VTY_NEWLINE);
		}
	}
	vty_out (vty, "*********************************%s",VTY_NEWLINE);
	return CMD_SUCCESS;
}
#endif
/*
 * 用于获取获取连接邻居的MAC地址和邻居IP地址
 * 用于设置静态ARP表项，和设置直连路由
 * 函数在ospf_dcn.c定义
 */
int ospf_dcn_link_nbr_nbrmac(struct in_addr nbrIp, char *srcIfname, char *mac)
{
#ifdef DCN_MAC_TABLE
	int i = 0;
	char  cmd[256];
	int flag = 0;
	char mac_str[64];

	if(nbrIp.s_addr != nbrmac.nbrIp.s_addr)
	{
		/*邻居地址发生变化，删除ARP和直连路由信息*/
		if(nbrmac.nbrIp.s_addr != 0)
		{
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "arp -i %s -d %s",srcIfname? srcIfname:OSPF_DCN_LINK_IF_NAME, inet_ntoa(nbrmac.nbrIp));
			super_system(cmd);
			GT_DEBUG("%s:delete arp table(%s)\n",__func__,cmd);
			sprintf(cmd,"ip route del %s dev %s",inet_ntoa(nbrmac.nbrIp),srcIfname? srcIfname:OSPF_DCN_LINK_IF_NAME);
			super_system(cmd);
			GT_DEBUG("%s:delete connect route table(%s)\n",__func__,cmd);
		}
	}
	else
		return 1;

	for(i = 0; i < OSPF_DCN_NBR_MAC_MAX; i++)
	{
		if(dcn_mac[i].initialed && dcn_mac[i].nbrIp.s_addr == nbrIp.s_addr )
		{
			memset(mac_str, 0, sizeof(mac_str));
			sprintf(mac_str,"%02x:%02x:%02x:%02x:%02x:%02x",
					dcn_mac[i].mac[0],dcn_mac[i].mac[1],dcn_mac[i].mac[2],
					dcn_mac[i].mac[3],dcn_mac[i].mac[4],dcn_mac[i].mac[5]);
			//strcpy(mac, mac_str);
			strcpy(nbrmac.nbrMac, mac_str);
			flag = 1;
			break;
			//return 0;
		}
	}
	if(flag == 1)
	{	/*成功获取到对端MAC地址，添加直连路由*/
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"ip route add %s dev %s",inet_ntoa(nbrIp),srcIfname? srcIfname:OSPF_DCN_LINK_IF_NAME);
		super_system(cmd);
		GT_DEBUG("%s:get mac address setting connect route table(%s)\n",__func__,cmd);
		nbrmac.nbrIp.s_addr = nbrIp.s_addr;
		nbrmac.initialed = 1;
	}
	return flag;
#else
	FILE *fd;
	char *pStart = NULL;
	char *pEnd = NULL;
	char  cmd[256], buf[512];	
	int init = 0;	
	int count = 0;
	
	if(nbrIp.s_addr != nbrmac.nbrIp.s_addr)
	{
		/*邻居地址发生变化，删除ARP和直连路由信息*/
		if(nbrmac.nbrIp.s_addr != 0)
		{
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "arp -i %s -d %s",srcIfname? srcIfname:OSPF_DCN_LINK_IF_NAME, inet_ntoa(nbrmac.nbrIp));
			super_system(cmd);
			GT_DEBUG("%s:delete arp table(%s)\n",__func__,cmd);
			sprintf(cmd,"ip route del %s dev %s",inet_ntoa(nbrmac.nbrIp),srcIfname? srcIfname:OSPF_DCN_LINK_IF_NAME);
			super_system(cmd);
			GT_DEBUG("%s:delete connect route table(%s)\n",__func__,cmd);
		}
	}
	else
		return 1;
	
	memset(cmd, 0, sizeof(cmd));
	if(srcIfname == NULL)
	{
		sprintf(cmd,"arping -I %s -c 1 %s", OSPF_DCN_LINK_IF_NAME, inet_ntoa(nbrIp));
	}
	else
	{
		sprintf(cmd,"arping -I %s -c 1 %s", srcIfname, inet_ntoa(nbrIp));
	}	
	/* 可能一次获取不到，需要多次获取 */
	while(init == 0 && count <= 5)
	{
		fd = popen(cmd,"r");
		GT_DEBUG("%s:to get mac address by arping(%s) \n",__func__,cmd);
		if(fd != NULL)
		{
			memset(buf, 0, sizeof(buf));
			while(fgets(buf, 512, fd)!=NULL && init !=1)
			{
				pStart = strstr(buf,"[");
				pEnd = strstr(buf,"]");
				if(NULL != pStart && NULL != pEnd)
				{
					init = 1;
					/*成功获取到对端MAC地址，添加直连路由的ARP表*/
					memcpy(nbrmac.nbrMac, pStart+1, MIN(pEnd-pStart-1, OSPF_DCN_NBR_MAC_SIZE));
					sprintf(cmd, "arp -i %s -s %s %s", srcIfname? srcIfname: OSPF_DCN_LINK_IF_NAME, inet_ntoa(nbrIp), nbrmac.nbrMac);
					super_system(cmd);
					GT_DEBUG("%s:get mac address and setting arp table(%s)\n",__func__,cmd);
				}
			}
			pclose(fd);
		}
		count++;
	}
	if(init == 1)
	{	/*成功获取到对端MAC地址，添加直连路由*/
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"ip route add %s dev %s",inet_ntoa(nbrIp),srcIfname? srcIfname:OSPF_DCN_LINK_IF_NAME);
		super_system(cmd);
		GT_DEBUG("%s:get mac address setting connect route table(%s)\n",__func__,cmd);	
		nbrmac.nbrIp.s_addr = nbrIp.s_addr;
		nbrmac.initialed = 1;
	}
	return init;
#endif
}
/*根据IP地址信息查找接口*/
static struct interface * ospf_dcn_link_if_lookup_prefix (struct prefix *p)
{
	struct listnode *node;
	struct listnode *cnode;
	struct interface *ifp;
	struct connected *c;

	for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
	{
		for (ALL_LIST_ELEMENTS_RO (ifp->connected, cnode, c))
		{
			if (prefix_cmp(c->address, p) == 0)
			{
				return ifp;
			}
		}
	}
	return NULL;
}

static int ospf_zebra_route_dcn_handle(int type, struct prefix_ipv4 *p, char *ifname)
{
	char ipstr[64];	
	char cmd[1024];
	char ty [16];
    //route add -net 127.0.0.0 netmask 255.0.0.0 dev lo
    //route add -net 192.56.76.0 netmask 255.255.255.0 dev eth0
	char if_name[64];
	
	if(p == NULL)
		return -1; 
	/*目的地址和下一跳相同的路由不需要在操作*/
	if( (nbrmac.nbrIp.s_addr != 0) &&
		(nbrmac.nbrIp.s_addr == p->prefix.s_addr) &&
		(p->prefix.s_addr != 0) )
		return 0;
	
	/* 这里需要判断出目的地址和自己系统内相同网段的不需要操作
	 * 目的地址和自己同网段的属于直连路由，不需要操作
	 */
	if(ospf_dcn_link_if_lookup_prefix((struct prefix *)p))
	{
		GT_DEBUG("%s: destination address is same to local(%s)\n",__func__,inet_ntoa(p->prefix));	
		return 0;
	}
	
	if(ifname == NULL)
		sprintf(if_name,"%s",OSPF_DCN_LINK_IF_NAME);
	else
		sprintf(if_name,"%s",ifname);
	
	if(type==1)
		sprintf(ty,"%s","add");
	else
		sprintf(ty,"%s","del");
	/*
	 * 设置到网络10.0.0/24的路由经过网关193.233.7.65
	 * ip route add 10.0.0/24 via 193.233.7.65
	 * ip route add 192.168.10.0/24 via 192.168.5.100 dev eth0
	 * 修改到网络10.0.0/24的直接路由，使其经过设备dummy
	 * ip route chg 10.0.0/24 dev dummy
	 * ip route add 192.168.5.0/24 dev eth0
	 * 范例五：删除路由
	 * ip route del 192.168.10.0/24
	 * ip route del 192.168.5.0/24
	 * 
	 * 可能出现下一跳地址和nbr地址不一样，这就应该把下一跳地址替换为nbr地址
	 */
	if(type==1)
	{
		/*添加默认出口路由，到达邻居地址的路由从ifname接口出去*/
		/*
		sprintf(cmd,"ip route %s %s dev %s",ty,inet_ntoa(neigborIp),if_name);
		super_system(cmd);
		GT_DEBUG("%s:excute CMD:%s\n",__func__,cmd);
		*/
	}
 	inet_ntop (p->family, &p->prefix, ipstr, sizeof(ipstr));
 	if(type==1)
 	{
 		if(p->prefixlen != IPV4_MAX_PREFIXLEN)/*非32位掩码，网络路由*/
 		sprintf(cmd,"route %s -net %s/%d gw %s dev %s",ty,ipstr, p->prefixlen, 
			inet_ntoa(nbrmac.nbrIp), if_name);
 		else
 		{	/*32位掩码，主机路由*/
 			if(nbrmac.nbrIp.s_addr != p->prefix.s_addr)
 				sprintf(cmd,"route %s -host %s gw %s dev %s",ty,ipstr,inet_ntoa(nbrmac.nbrIp), if_name);
 			else
 				sprintf(cmd,"route %s -host %s dev %s",ty, ipstr, if_name);
 		}
 	}
 	else
 	{
 		if(p->prefixlen != IPV4_MAX_PREFIXLEN)
		sprintf(cmd,"route %s -net %s/%d gw %s dev %s",ty,ipstr, p->prefixlen, 
				inet_ntoa(nbrmac.nbrIp), if_name);
 		else 
 		{
 			if(nbrmac.nbrIp.s_addr != p->prefix.s_addr)
 				sprintf(cmd,"route %s -host %s gw %s dev %s",ty,ipstr,inet_ntoa(nbrmac.nbrIp), if_name);
 			else 
 				sprintf(cmd,"route %s -host %s dev %s",ty,ipstr, if_name);
 		}
 	}
	super_system(cmd);
	GT_DEBUG("%s:kernel route table handle(%s)\n",__func__,cmd);
}
/*****************************************************************************
* function	: ospf_zebra_auto_arp
* declare	: 
* input	: type: 1 add, 0 del; p: ipv4 address prefix; mac : mac address table ; ifname : out interfacce name
* output	: 
* return	: 0 OK, -1 ERROR
* other	: 
* auther	: zhurish (zhurish@163.com)
* create 	: 2016/8/17 14:35:36
*****************************************************************************/
/*
 * 用于OSPF学习到路由后添加静态ARP表，OSPF学习到的路由是直连路由：下一跳地址为0，只有下一跳出口
 * 目的地址，用于添加静态ARP表
 * 函数在ospf_dcn.c文件定义，
 */
int ospf_zebra_auto_arp(int type, struct prefix_ipv4 *p, char *mac, char *ifname)
{
	/*
	 * sudo arp -i eth0 -s 192.168.189.155 00:50:56:00:45:84 add
	 * sudo arp -i eth0 -d 192.168.189.155  del
	 * 
	 * 在设备eth0上，为地址10.0.0.3添加一个permanent ARP条目
	 * ip neigh add 192.168.189.155 lladdr 00:50:56:00:45:84 dev eth0 nud perm
	 *    noarp 网络邻居有效，不必检查, permanent这是一个noarp条目，只有系统管理员可以从邻接表中把它删除	
	 * ip neigh chg 192.168.189.155 dev eth0 nud reachable 把状态改为reachable(网络邻居有效并且可达)
	 * ip neigh del 192.168.189.155 dev eth0 删除一个邻接条目
	 */
	//FILE *fd = NULL;
	char cmd[1024];
	char ipstr[64];
	char macstr[64];
	char ty[5] = "s";
	char if_name[64];
	if(p == NULL /*|| strlen(neigborMac) < 6*/)
		return -1; 
	//return 0;
	if(ntohl(p->prefix.s_addr) == INADDR_LOOPBACK)
	{
		GT_DEBUG("%s: loopback ip address,do nothing\n",__func__);
	    return -1;
	}
	
	/* 先处理路由信息 */
    if(nbrmac.nbrIp.s_addr != 0)
    	ospf_zebra_route_dcn_handle(type, p, ifname);
    
    /* 不是主机地址返回，不需要添加静态ARP 表项 */
    if(p->prefixlen != IPV4_MAX_PREFIXLEN)
    {
    	GT_DEBUG("%s: network ip address do nothing\n",__func__);
    	return -1;
    }
	/* 这里需要判断出目的地址和自己系统内相同网段的不需要操作
	 * 防止把自己的IP地址和MAC写在ARP表上；例如192.168.1.197运行点对点OSPF，
	 * 通过这个检测防止把192.168.1.197的MAC写在ARP表
	 */
	if(ospf_dcn_link_if_lookup_prefix((struct prefix *)p))
	{
		GT_DEBUG("%s: destination address is same to local(%s)\n",__func__,inet_ntoa(p->prefix));	
		return 0;
	}    
	memset(ipstr, 0, sizeof(ipstr));
	memset(macstr, 0, sizeof(macstr));	
	memset(cmd, 0, sizeof(cmd));
	
	if(ifname == NULL)
		sprintf(if_name,"%s",OSPF_DCN_LINK_IF_NAME);
	else
		sprintf(if_name,"%s",ifname);
    if(mac)
		sprintf(macstr,"%s",mac);
    else
    	sprintf(macstr,"%s",nbrmac.nbrMac);
    
 	inet_ntop (p->family, &p->prefix, ipstr, sizeof(ipstr));
 	//snprintf (str, size, "%s/%d", buf, p->prefixlen);
	//sprintf(macstr, "%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	if(type == 1)
		sprintf (ty, "%s", "s");
	else if(type == 0)
		sprintf (ty, "%s", "d");
  
	if(type == 1)
		sprintf(cmd, "arp -i %s -%s %s %s", if_name, ty, ipstr, macstr);
	else
		sprintf(cmd, "arp -i %s -%s %s",if_name, ty, ipstr);/*delete arp table on eth0 */	
	
	super_system(cmd);
	GT_DEBUG("%s:add arp table for host route(%s) \n",__func__,cmd);
	return 0;
}
/****************************************************************************************/
/****************************************************************************************/
static void ospf_dcn_link_if_del_hook (void *val)
{
  XFREE (MTYPE_OSPF_IF, val);
  return;
}
//int (* new_lsa_hook)(struct ospf_lsa *lsa);
//int (* del_lsa_hook)(struct ospf_lsa *lsa);
/****************************************************************************************/
static int ospf_dcn_device_load(const char *filename)
{
	FILE *fp = NULL;
	if(filename ==  NULL)
		fp = fopen("/app/etc/device.conf","r");
	else
		fp = fopen(filename,"r");
	if(fp)
	{
		memset(CPE_DEVICEMODEL, 0, MAX_MODEL_LENGTH);
		fgets(CPE_DEVICEMODEL, MAX_MODEL_LENGTH, fp);
		fclose(fp);
	}
	return 0;
}
/****************************************************************************************/
int ospf_dcn_init (void)
{	
	int rc;
	rc = ospf_register_opaque_functab (
		  OSPF_OPAQUE_AREA_LSA,
		  OPAQUE_TYPE_FLOODDCNINFO,
		  ospf_dcn_link_new_hook,
		  ospf_dcn_link_del_hook,
		  ospf_dcn_link_ism_change_hook,
		  ospf_dcn_link_nsm_change_hook,
		  ospf_dcn_link_config_write_router_hook,
		  ospf_dcn_link_config_write_if_hook,
		  ospf_dcn_link_config_debug_hook,/* ospf_dcn_config_write_debug */
		  ospf_dcn_link_show_hook,
		  ospf_dcn_link_lsa_originate_hook,
		  ospf_dcn_link_lsa_refresh_hook,
		  NULL,/* ospf_dcn_new_lsa_hook */
		  NULL /* ospf_dcn_del_lsa_hook */);
	if (rc != 0)
	{
		zlog_warn ("ospf_dcn_init: Failed to register functions");
		return -1;
	}
	memset (&nbrmac, 0, sizeof (struct dcn_link_nbr));	
	memset (&OspfDcn, 0, sizeof (struct ospf_dcn));
	OspfDcn.status = disabled;
	OspfDcn.iflist = list_new ();
	OspfDcn.iflist->del = ospf_dcn_link_if_del_hook;
	ospf_dcn_register_vty ();
	ospf_dcn_device_load("/etc/app/device.conf");
	return 0;
}

void ospf_dcn_term (void)
{
	list_delete (OspfDcn.iflist);
	OspfDcn.iflist = NULL;
	OspfDcn.status = disabled;
	ospf_delete_opaque_functab (OSPF_OPAQUE_AREA_LSA,
			OPAQUE_TYPE_FLOODDCNINFO);
	memset (&nbrmac, 0, sizeof (struct dcn_link_nbr));	
	return;
}

#endif /* HAVE_OSPFD_DCN */
