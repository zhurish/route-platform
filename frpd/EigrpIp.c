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

	Module:	EigrpIp

	Name:	EigrpIp.c

	Desc:	

	Rev:	
	
***********************************************************************************/
//#include "../Config.h"

#include	"./include/EigrpPreDefine.h"
#include	"./include/EigrpSysPort.h"
#include	"./include/EigrpCmd.h"
#include	"./include/EigrpUtil.h"
#include	"./include/EigrpDual.h"
#include	"./include/Eigrpd.h"
#include	"./include/EigrpIntf.h"
#include	"./include/EigrpIp.h"
#include	"./include/EigrpMain.h"
#include	"./include/EigrpPacket.h"
#ifdef _EIGRP_PLAT_MODULE
#include	"./include/EigrpZebra.h"
#endif// _EIGRP_PLAT_MODULE

extern	Eigrp_pt	gpEigrp;

#ifdef _DC_
extern	S32 DcUtilSetUaiHelloInterval(S8 *ifName, U32 interval);
extern	S32 DcUtilSetUaiHoldTime(S8 *ifName, U32 holdTime);
extern	void	*DcPortGetFirstUaiByCircName(S8 *ifName);
extern	U8 DcPortUaiUpdateCurSei(struct DcUai_ *);
#endif
/************************************************************************************

	Name:	EigrpIpVmetricBuild

	Desc:	This function is to duplicate a set of eigrp vmetric data.
		
	Para: 	vmetric	- pointer to the given vector metric
	
	Ret:		pointer to the new eigrp vmetric data which is equal to the given vector metric
************************************************************************************/

EigrpVmetric_st *EigrpIpVmetricBuild(EigrpVmetric_st *vmetric)
{
	EigrpVmetric_st *vbuild;

	EIGRP_FUNC_ENTER(EigrpIpVmetricBuild);
	if(!vmetric){
		EIGRP_FUNC_LEAVE(EigrpIpVmetricBuild);
		return NULL;
	}
	
	vbuild = EigrpPortMemMalloc(sizeof(EigrpVmetric_st));
	if(!vbuild){	
		EIGRP_FUNC_LEAVE(EigrpIpVmetricBuild);
		return NULL;
	}
	EigrpUtilMemZero((void *) vbuild, sizeof(EigrpVmetric_st));
	*vbuild = *vmetric;
	EIGRP_FUNC_LEAVE(EigrpIpVmetricBuild);
	
	return vbuild;
}

/************************************************************************************

	Name:	EigrpIpVmetricDelete

	Desc:	This function is to free an eigrp vmetric data structure.
		
	Para: 	vmectric		- pointer to the eigrp vmetric data structure to be freed
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpVmetricDelete(EigrpVmetric_st *vmetric)
{
	EIGRP_FUNC_ENTER(EigrpIpVmetricDelete);
	if(vmetric){
		EigrpPortMemFree(vmetric);
		vmetric = (EigrpVmetric_st *) 0;
	}
	EIGRP_FUNC_LEAVE(EigrpIpVmetricDelete);
	
	return;
}
    
/************************************************************************************

	Name:	EigrpIpGetNetsPerAddress

	Desc:	This function is to get the number of networks in process pdb which covers the incoming
			ipaddr.
		
	Para: 	netLst	- pointer to the network address list
			ipaddr	- the incoming ip address
	
	Ret:		the number of networks in process pdb which covers the incoming ipaddr.
************************************************************************************/

S32	EigrpIpGetNetsPerAddress(EigrpNetEntry_st *netLst, U32 ipaddr)
{
	EigrpNetEntry_st *network_addr;
	U32 mask, address;
	S32 found = 0;

	EIGRP_FUNC_ENTER(EigrpIpGetNetsPerAddress);
	EIGRP_SOCK_LIST(netLst, network_addr){
		address	= network_addr->address ;
		mask	= network_addr->mask;
		if((ipaddr & mask) == (address & mask)){
			found++;
		}
	}
	EIGRP_SOCK_LIST_END(netLst, network_addr);
	EIGRP_FUNC_LEAVE(EigrpIpGetNetsPerAddress);
	
	return found;
}

/************************************************************************************

	Name:	EigrpIpGetNetsPerIdb

	Desc:	This function is to get the number of networks in process pdb which covers all the 
			ipaddress of the given interface.
		
	Para: 	netLst	- pointer to the network address list
			pEigrpIntf	- pointer to the given interface
			
	Ret:		the number of networks in process pdb which covers all the ip address of  the 
			given interface.
************************************************************************************/

S32	EigrpIpGetNetsPerIdb(EigrpNetEntry_st *netLst, EigrpIntf_pt pEigrpIntf)
{
	EigrpIntfAddr_pt	pAddr;
	S32	retVal;

	EIGRP_FUNC_ENTER(EigrpIpGetNetsPerIdb);
	for(pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			retVal	= EigrpIpGetNetsPerAddress(netLst, pAddr->ipDstAddr);
		}else{
			retVal	= EigrpIpGetNetsPerAddress(netLst, pAddr->ipAddr);
		}
		if(retVal){
			EigrpPortMemFree(pAddr);
			EIGRP_FUNC_LEAVE(EigrpIpGetNetsPerIdb);
			return retVal;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpGetNetsPerIdb);

	return 0;
}

/************************************************************************************

	Name:	EigrpIpNetworkEntryDelete

	Desc:	This function is to delet an old network entry from the entry list.
		
	Para: 	list		- pointer to the network entry list 
			address	- the ip address of the network entry to be deleted
			mask	- the ip mask of the network entry to be deleted
	
	Ret:		TRUE	for success to delete
			FALSE	for nothing is found
************************************************************************************/

S32	EigrpIpNetworkEntryDelete(EigrpNetEntry_st *list, U32 address, U32 mask)
{
	EigrpNetEntry_st	*entry_temp;
	EigrpNetEntry_st	*del_entry;

	EIGRP_FUNC_ENTER(EigrpIpNetworkEntryDelete);
	/* Search the entry list to see whether there is a same entry in it */
	EIGRP_SOCK_LIST(list, del_entry){
		if((del_entry->address & del_entry->mask) == (address & mask)){
			break;
		}
	}EIGRP_SOCK_LIST_END(list, del_entry);

	if(!del_entry){
		EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryDelete);
		return FALSE;
	}

	/* Delete the entry for the list and free the memory */
	entry_temp = del_entry;

	del_entry->back->next	= del_entry->next;
	del_entry->next->back	= del_entry->back;
	if(entry_temp){
		EigrpPortMemFree((S8 *)entry_temp); 
	}
	EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryDelete);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpNetworkListCleanup

	Desc:	This function is to delete all the entries in the network entry list.
		
	Para: 	list	- pointer to the network entry list
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpNetworkListCleanup(EigrpNetEntry_st *list)
{
	EigrpNetEntry_st *entry_temp ;
	EigrpNetEntry_st *del_entry ;
	
	EIGRP_FUNC_ENTER(EigrpIpNetworkListCleanup);
	EIGRP_SOCK_LIST(list, del_entry){
		entry_temp	= del_entry;
		del_entry->back->next		= del_entry->next;
		del_entry->next->back		= del_entry->back;
		del_entry		= del_entry->back;
	
		if(entry_temp){
			EigrpPortMemFree((S8 *)entry_temp); 
		}
	}EIGRP_SOCK_LIST_END(list, del_entry);
	EIGRP_FUNC_LEAVE(EigrpIpNetworkListCleanup);

	return;
}

/************************************************************************************

	Name:	EigrpIpNetworkEntryBuild

	Desc:	This function is to create a new network entry.
		
	Para: 	addr		- the ip address of the new network entry
			mask	- the ip mask of the new network entry
	
	Ret:		pointer to the new network entry
************************************************************************************/

EigrpNetEntry_st	*EigrpIpNetworkEntryBuild(U32 addr,   U32 mask)
{
	U32	len = sizeof(EigrpNetEntry_st);
	struct EigrpNetEntry_ *new_entry;

	EIGRP_FUNC_ENTER(EigrpIpNetworkEntryBuild);
	new_entry = (EigrpNetEntry_st *) EigrpPortMemMalloc(len);
	if(!new_entry){	
		EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryBuild);
		return NULL;
	}
	new_entry->address	= addr;
	new_entry->mask		= mask;

	EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryBuild);
	return new_entry;
}

/************************************************************************************

	Name:	EigrpIpNetworkEntrySearch

	Desc:	This function is to search an network entry with the given ip address and mask.
		
	Para: 	elist		- pointer to the network entry list
			address	- the given ip address
			mask	- the given ip mask
	
	Ret:		pointer to the exact network entry we needed
************************************************************************************/

EigrpNetEntry_st	*EigrpIpNetworkEntrySearch(EigrpNetEntry_st *elist, U32 address, U32 mask)
{
	EigrpNetEntry_st *new_entry = (EigrpNetEntry_st *)0;
	
	EIGRP_FUNC_ENTER(EigrpIpNetworkEntrySearch);
	/* Search the entry list to see whether there is a same entry in it */
	EIGRP_SOCK_LIST(elist, new_entry){
		if((new_entry->address & new_entry->mask)== (address & new_entry->mask)){
			break;
		}
	}EIGRP_SOCK_LIST_END(elist, new_entry);
	EIGRP_FUNC_LEAVE(EigrpIpNetworkEntrySearch);

	return new_entry;
}

/************************************************************************************

	Name:	EigrpIpNetworkEntryInsert

	Desc:	This function is to insert a prepared network entry.
		
	Para: 	list		- pointer to the network entry list
			address	- the given ip address
			mask	- the given ip mask
	
	Ret:		TRUE	for success
			FALSE	for fail
************************************************************************************/

S32	EigrpIpNetworkEntryInsert(EigrpNetEntry_st *list,   U32 addr, U32 mask)
{
	EigrpNetEntry_st *new_entry;
	EigrpNetEntry_st *entry_temp;
#ifdef _EIGRP_PLAT_MODULE
	EigrpIntf_pt pEigrpIntf = NULL;
	//检测这个地址是否在接口链表里面，如果这个接口不在
	//接口链表里面，就不让这个接口启动FRP路由
	pEigrpIntf = EigrpIntfFindByNetAndMask(addr,  mask);
	if(pEigrpIntf == NULL)
	{
		return FALSE ;
	}
#endif/* _EIGRP_PLAT_MODULE */

	EIGRP_FUNC_ENTER(EigrpIpNetworkEntryInsert);

	if(EigrpIpNetworkEntrySearch(list, addr, mask)){
		EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryInsert);
		return FALSE ;
	}

	new_entry	= EigrpIpNetworkEntryBuild(addr, mask);

	if(!new_entry){	
		EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryInsert);
		return FALSE ;
	}
	entry_temp	= list->back ;

	list->back		= new_entry;
	new_entry->next	= list;
	new_entry->back	= entry_temp;
	entry_temp->next	= new_entry;

	EIGRP_FUNC_LEAVE(EigrpIpNetworkEntryInsert);
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpRedisEntryInsert

	Desc:	This function is to malloc a new redistribute entry and  insert it into the 
			redistribute list or just change the params of the existing entry.
		
	Para: 	redisLst		- doublepointer to the redistribute list 
			rInfo		- pointer to the redistribute command line param
	
	Ret:		pointer to the new redistribute entry
************************************************************************************/

EigrpRedisEntry_st	*EigrpIpRedisEntryInsert(EigrpRedisEntry_st **redisLst, EigrpRedis_st*rinfo)
{
	EigrpVmetric_st	vmetric;
	EigrpRedisEntry_st	*redis;
	U16		redisindex	= 0;
	U32		changes		= 0;
	EigrpPdb_st	*pdb;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpIpRedisEntryInsert);
	pdb = (EigrpPdb_st *)((S8 *)redisLst-EIGRP_MEMBER_OFFSET(EigrpPdb_st, redisLst));

	/* search for old redistribute config */
	for(redis = *redisLst; redis; redis = redis->next){
		/* In this pdb, all recv_proto is EIGRP_ROUTE_EIGRP, and all recv_asystem is pdb->process.
		  * Don't need compare */
		if((redis->proto == rinfo->srcProto) && (redis->process == rinfo->srcProc)){
			break;
		}

		/* Pickup the largest index */
		if(redisindex < redis->index){
			redisindex = redis->index;
		}
	}

	if(!redis)	{
		/* can not find a old one, create it */
		redis = (EigrpRedisEntry_st *)EigrpPortMemMalloc(sizeof(EigrpRedisEntry_st));
		if(!redis) {               
			EIGRP_FUNC_LEAVE(EigrpIpRedisEntryInsert);
			return NULL;
		}	
		EigrpUtilMemZero((void *)redis, sizeof(EigrpRedisEntry_st));

		redis->proto		= rinfo->srcProto;
		redis->process	= rinfo->srcProc;
		redis->rcvProto	= EIGRP_ROUTE_EIGRP;
		redis->rcvProc	= pdb->process;

		redisindex++;
		redis->index	= redisindex;
		redis->next	= *redisLst;
		*redisLst	= redis;
		changes++;
	}

	/* Do we config metric in redistribute command ? */
	if(BIT_TEST(rinfo->flag, EIGRP_REDISTRIBUTE_FLAG_METRIC)){
		/* Yes, metric is config, update it */
		vmetric.bandwidth	= rinfo->vmetric[0];
		vmetric.delay		= rinfo->vmetric[1];
		vmetric.reliability	= rinfo->vmetric[2];
		vmetric.load		= rinfo->vmetric[3];
		vmetric.mtu		= rinfo->vmetric[4];
		vmetric.hopcount	= 0;
		EigrpIpVmetricDelete(redis->vmetric);
		redis->vmetric = EigrpIpVmetricBuild(&vmetric);
		BIT_SET(redis->flag, EIGRP_REDISTRIBUTE_FLAG_METRIC);
		changes++;
	}else{   /* no, metric is no config. do we have old one ? */
		if(redis->vmetric){
			/* Yes, we have a old one , clear it */
			EigrpIpVmetricDelete(redis->vmetric);
			BIT_RESET(redis->flag, EIGRP_REDISTRIBUTE_FLAG_METRIC);
			redis->vmetric = NULL;
			changes++;
		}
	}

	/* Do we config a route-map in redistribute command ? */
	if(BIT_TEST(rinfo->flag, EIGRP_REDISTRIBUTE_FLAG_RTMAP)){   
		/* Yes, we config route-map, update it */
		EigrpPortMemCpy(redis->rtMapName, rinfo->rtMapName, EIGRP_MAX_ROUTEMAPNAME_LEN);
		redis->rtMapName[EIGRP_MAX_ROUTEMAPNAME_LEN]	= '\0';
		BIT_SET(redis->flag, EIGRP_REDISTRIBUTE_FLAG_RTMAP);
		redis->rtMap = EigrpPortGetRouteMapByName(redis->rtMapName);
		changes++;
	}else{   /* No , we have not config route-map, do we have a old route-map? */
		if(redis->rtMapName[0]){
			/* Yes, we have a old route-map config, clear it */
			redis->rtMapName[0]	= '\0';
			redis->rtMap		= NULL;
			BIT_RESET(redis->flag, EIGRP_REDISTRIBUTE_FLAG_RTMAP);
			changes ++;
		}
	}

	if(changes){
		_EIGRP_DEBUG("EigrpIpRedisEntryInsert SUCCESS\n");
		EIGRP_FUNC_LEAVE(EigrpIpRedisEntryInsert);
		return	redis;
	}else{
		_EIGRP_DEBUG("EigrpIpRedisEntryInsert FAILURE\n");
		EIGRP_FUNC_LEAVE(EigrpIpRedisEntryInsert);
		return	(EigrpRedisEntry_st*) 0;
	}
}

/************************************************************************************

	Name:	EigrpIpRedisEntryDelete

	Desc:	This function is to delete a redis entry from the redis list, given the redis index.
		
	Para: 	redisLst		- doublepointer to the redis list 
			del_redisindex		- the given redis index
		
	Ret:		NONE
************************************************************************************/

void	EigrpIpRedisEntryDelete(EigrpRedisEntry_st **redisLst, U16 del_redisindex)
{
	EigrpRedisEntry_st	*redis;

	EIGRP_FUNC_ENTER(EigrpIpRedisEntryDelete);
	redis = *redisLst;
	while(redis){
		if(redis->index == del_redisindex){
			*redisLst = redis->next;
			EigrpIpVmetricDelete(redis->vmetric);
			EigrpPortMemFree(redis);
			redis = (EigrpRedisEntry_st*) 0;
			break;
		}
		redisLst	= &redis->next;
		redis	= redis->next;
	}
	EIGRP_FUNC_LEAVE(EigrpIpRedisEntryDelete);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpRedisEntrySearch

	Desc:	This function is to find a redistribute entry from the redistribute list by the given 
			index.
		
	Para: 	redisLst		- pointer to the redistribute list
			redisindex	- the given index
	
	Ret:		pointer to the exact redistribute entry with the given index
************************************************************************************/

EigrpRedisEntry_st	*EigrpIpRedisEntrySearch(EigrpRedisEntry_st *redisLst, U16 redisindex)
{
	EigrpRedisEntry_st	*redis;

	EIGRP_FUNC_ENTER(EigrpIpRedisEntrySearch);
	for(redis = redisLst; redis; redis = redis->next)
		if(redis->index == redisindex){
			EIGRP_FUNC_LEAVE(EigrpIpRedisEntrySearch);
			return redis;
		}
	EIGRP_FUNC_LEAVE(EigrpIpRedisEntrySearch);
	
	return(EigrpRedisEntry_st*) 0;
}

/************************************************************************************

	Name:	EigrpIpRedisEntrySearchWithProto

	Desc:	This function is to find a redistribute entry from the redistribute list by the given
			protocol.
		
	Para: 	redisLst		- pointer to the redistribute list
			proto		- the given protocol
			asystem		- asystem number
			
	Ret:		pointer to the exact redistribute entry we need		
************************************************************************************/

EigrpRedisEntry_st	*EigrpIpRedisEntrySearchWithProto(EigrpRedisEntry_st *redisLst, U32 proto, U32 asystem)
{
	EigrpRedisEntry_st	*redis;

	EIGRP_FUNC_ENTER(EigrpIpRedisEntrySearchWithProto);
	
	for(redis = redisLst; redis; redis = redis->next)
		if((redis->proto == proto)  && (redis->process == asystem)){
			EIGRP_FUNC_LEAVE(EigrpIpRedisEntrySearchWithProto);
			return redis;
		}
	EIGRP_FUNC_LEAVE(EigrpIpRedisEntrySearchWithProto);
		
	return(EigrpRedisEntry_st*) 0;
}

/************************************************************************************

	Name:	EigrpIpRedisListCleanup

	Desc:	This function is to cleanup the whole redistribute list.
		
	Para: 	redisLst		- doublepointer to the redistribute list
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpRedisListCleanup(EigrpRedisEntry_st **redisLst)
{
	EigrpRedisEntry_st	*redis;

	EIGRP_FUNC_ENTER(EigrpIpRedisListCleanup);
	redis = *redisLst;
	while(redis){
		*redisLst = redis->next;
		EigrpIpVmetricDelete(redis->vmetric);
		EigrpPortMemFree(redis);
		redis = *redisLst;
	}
	EIGRP_FUNC_LEAVE(EigrpIpRedisListCleanup);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpSetRouterId

	Desc:	This function is to set the router ID if it's not yet set.  The vagaries of initialization
			order require us to call this at each configuration path.
		
	Para: 	ddb		- pointer to the dual descriptor block 
		
	Ret:		NONE		
************************************************************************************/
#ifndef CETC50_API
extern U32 ipAddrOfZhiKongNet;
#endif/* CETC50_API */
#ifdef _EIGRP_PLAT_MODULE
static U32	zebraEigrpIntfGetAddress(U32 ip, U32 mask)
{
	EigrpIntf_pt pTem = NULL;
	
	if((!gpEigrp) ||(! gpEigrp->intfLst) )
		return FAILURE;

	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next)
	{
		if(pTem)
		{
			if( (pTem->ipAddr != 0)&&(pTem->ifindex != 0) )
			{
				if( (pTem->ipAddr & pTem->ipMask)==(ip & mask) )
					return pTem->ipAddr;
			}
		}
	}
	return FAILURE;
}
void	EigrpIpSetRouterId(EigrpDualDdb_st *ddb, U32 ip, U32 mask)
{
	int value = 0;
	if(ddb->routerId != EIGRP_IPADDR_ZERO){    
		return;
	}
	value = zebraEigrpIntfGetAddress( ip, mask);
	if(value != FAILURE)
		ddb->routerId = value;
	else
		ddb->routerId = ip;
	return;
}
#else//_EIGRP_PLAT_MODULE
void	EigrpIpSetRouterId(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpIpSetRouterId);
	if(ddb->routerId != EIGRP_IPADDR_ZERO){    
		EIGRP_FUNC_LEAVE(EigrpIpSetRouterId);
		return;
	}
#ifndef CETC50_API
	ddb->routerId = EigrpPortGetTimeSec();
#else//CETC50_API
	ddb->routerId = ipAddrOfZhiKongNet;
#endif//CETC50_API
	EIGRP_FUNC_LEAVE(EigrpIpSetRouterId);
	return;
}
#endif//_EIGRP_PLAT_MODULE
#ifdef _EIGRP_PLAT_MODULE
void	zebraEigrpIpSetRouterId(U32 as, U32 routeid)
{
	EigrpPdb_st		*pdb;	
	EigrpDualDdb_st *ddb;
	EIGRP_FUNC_ENTER(zebraEigrpIpSetRouterId);	
	pdb	= EigrpIpFindPdb(as);
	if(pdb)
	{
		ddb = pdb->ddb;
		if(routeid != 0)
			ddb->routerId = routeid;
		else
			ddb->routerId = EigrpPortGetTimeSec();
	}
	EIGRP_FUNC_LEAVE(zebraEigrpIpSetRouterId);
	return;
}
#endif/* _EIGRP_PLAT_MODULE */
/************************************************************************************

	Name:	EigrpIpLaunchPdbJob

	Desc:	This function is to add a pdb into the process job.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpLaunchPdbJob(EigrpPdb_st *pdb)
{
	EIGRP_FUNC_ENTER(EigrpIpLaunchPdbJob);
	if(!pdb || pdb->eventJob){ 
		EIGRP_FUNC_LEAVE(EigrpIpLaunchPdbJob);
		return;
	}

	pdb->eventJob = EigrpUtilSchedAdd((S32 (*)(void *))EigrpIpProcessWorkq, (void *)pdb);

	EIGRP_TRC(DEBUG_EIGRP_EVENT, "EIGRP-EVENT: Create event job:%d\n", (U32)pdb->eventJob);
	EIGRP_FUNC_LEAVE(EigrpIpLaunchPdbJob);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueEvent

	Desc:	This function is to insert an eigrp event into the eigrp event process queue.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp event
			type		- type of the event
	
	Ret:		TRUE	for success
			FALSE	for fail
************************************************************************************/

S32	EigrpIpEnqueueEvent(EigrpPdb_st *pdb, EigrpWork_st *work, U32 type)
{
	U32 msgbuf[4], err;
	S32 ret;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueEvent);
	work->type	= type;
	msgbuf[0]	= (U32) work;

	/* When VOS_Que_AsyWrite return an error, we should free the work. */
	err = EigrpUtilQueWriteElem(pdb->workQueId, msgbuf);
	if(err == 0){
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asywrite:%d, work-type:%d\n", 
					msgbuf[0], work->type);
		ret = TRUE;
	}else{
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asywrite Fail:%d, work-type:%d\n",
					msgbuf[0], work->type);
		EigrpUtilChunkFree(pdb->ddb->workQueChunk, work);
		ret = FALSE;
	}
	EigrpIpLaunchPdbJob(pdb);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueEvent);
	
	return ret;
}

/************************************************************************************

	Name:	EigrpIpConnAddressActivated

	Desc:	This function is to walk through the pdb->netLst,check out if the ifap is in the 
			network list, return TRUE if in it, else return FALSE.

			The caller should make sure that pdb && ifap are not null-pointer.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			address	- ip address of ifap
			mask	- ip mask of ifap
			
	Ret:		TRUE 	for the ifap is in network list
			FALSE	for it is not so
************************************************************************************/
/* 确认这个地址是否已经启动路由 */
S32	EigrpIpConnAddressActivated(EigrpPdb_st *pdb, U32 address, U32 mask)
{
	EIGRP_FUNC_ENTER(EigrpIpConnAddressActivated);
	if(EigrpIpNetworkEntrySearch(&pdb->netLst, address, mask)){
		EIGRP_FUNC_LEAVE(EigrpIpConnAddressActivated);
		//_EIGRP_DEBUG("%s:TRUE:%x %x\n",__func__,address, mask);
		return TRUE;
	}
	//_EIGRP_DEBUG("%s:FALSE :%x %x\n",__func__,address, mask);
	EIGRP_FUNC_LEAVE(EigrpIpConnAddressActivated);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpIpGetActiveAddrCount

	Desc:	This function is to make clear that how many ip addresses is covered by the networks 
 			of the given pdb.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pCirc	- pointer to the physical interface
	
	Ret:		the number of ipaddess which is covered by the networks of the given pdb
************************************************************************************/
/* 确认这个接口有多少个地址启动路由 */
U32	EigrpIpGetActiveAddrCount(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf)
{
#if 0//#ifdef _EIGRP_PLAT_MODULE
	S32	retVal;
	_EIGRP_DEBUG("EigrpIpGetActiveAddrCount(%s)\n", pEigrpIntf->name);
	retVal = EigrpIpConnAddressActivated(pdb, pEigrpIntf->ipAddr, pEigrpIntf->ipMask);
	if(retVal == TRUE)
	{
		return 1;
		_EIGRP_DEBUG("EigrpIpGetActiveAddrCount find one\n");
	}
	//if(pEigrpIntf)
	//	if(pEigrpIntf->active == TRUE)
	//		return 1;
	return 0;	
#else//_EIGRP_PLAT_MODULE
	EigrpIntfAddr_pt	pAddr;
	U32 act_count;
	S32	retVal;

	EIGRP_FUNC_ENTER(EigrpIpGetActiveAddrCount);

	_EIGRP_DEBUG("EigrpIpGetActiveAddrCount(%s)\n", pEigrpIntf->name);

	act_count = 0;
	/* changed a mistake to support multiple ipaddress on one interface */
	pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc);
	if(pAddr){
		for(; pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				if(pAddr->ipAddr == EigrpPortGetRouterId()){/*主用户口地址*/
					retVal	= EigrpIpConnAddressActivated(pdb, pAddr->ipAddr, pAddr->ipMask);
				}else{//无编号
					retVal	= EigrpIpConnAddressActivated(pdb, pAddr->ipDstAddr, pAddr->ipMask);
				}
			}else{//有编号
				retVal	= EigrpIpConnAddressActivated(pdb, pAddr->ipAddr, pAddr->ipMask);
			}

			if(retVal){
				act_count ++;
			}
		}
	}else{
	/*zhenxl_20121126 此时NSM模块的接口表中此接口的地址信息有可能还未更新好*/
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			if(pEigrpIntf->ipAddr == EigrpPortGetRouterId()){
				retVal	= EigrpIpConnAddressActivated(pdb, pEigrpIntf->ipAddr, pEigrpIntf->ipMask);
			}else{
				retVal	= EigrpIpConnAddressActivated(pdb, pEigrpIntf->remoteIpAddr, pEigrpIntf->ipMask);
			}
		}else{
			retVal	= EigrpIpConnAddressActivated(pdb, pEigrpIntf->ipAddr, pEigrpIntf->ipMask);
		}

		if(retVal){
			act_count ++;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpGetActiveAddrCount);

	return act_count;
#endif//_EIGRP_PLAT_MODULE	
}

/************************************************************************************

	Name:	EigrpIpFindPdb

	Desc:	This function is to find a pdb, given its as number.
		
	Para: 	process	- asystem number
	
	Ret:		pointer to the found pdb
************************************************************************************/

EigrpPdb_st *EigrpIpFindPdb(U32 process)
{
	EigrpPdb_st *pdb;

	EIGRP_FUNC_ENTER(EigrpIpFindPdb);
	for(pdb = (EigrpPdb_st*)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->process == process){
			EIGRP_FUNC_LEAVE(EigrpIpFindPdb);
			return pdb;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpFindPdb);
	
	return(EigrpPdb_st*) 0;
}

/************************************************************************************

	Name:	EigrpIpGetOffset

	Desc:	This function is to get the offset if it exists.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			network	- pointer to the applied network entry
			iidb		- pointer to the applied interface
			direction	- when the offset is applied, receiving or sending
	
	Ret:		the offset value or 0 if fail
************************************************************************************/

U32	EigrpIpGetOffset(EigrpDualDdb_st *ddb, EigrpNetEntry_st *network, EigrpIdb_st *iidb, S8 direction)
{
	U32	acl, os_type;

	EIGRP_FUNC_ENTER(EigrpIpGetOffset);
	EIGRP_ASSERT((U32)iidb);

	if((U32)(acl = iidb->offset[ (U8 ) direction ].acl)){
		os_type	= EIGRP_OFFSET_LIST_INTF;
	}else if((U32)(acl = ddb->pdb->offset[ (U8 ) direction ].acl)){
		os_type	= EIGRP_OFFSET_LIST_GLOBAL;
	}else{
		EIGRP_FUNC_LEAVE(EigrpIpGetOffset);
		return 0;
	}

	if(EigrpPortCheckAclPermission(acl , network->address, network->mask) != SUCCESS){
		EIGRP_FUNC_LEAVE(EigrpIpGetOffset);
		return 0;
	}else{
		if(os_type == EIGRP_OFFSET_LIST_INTF){
			EIGRP_FUNC_LEAVE(EigrpIpGetOffset);
			return iidb->offset[ (U8 ) direction ].offset;
		}else if(os_type == EIGRP_OFFSET_LIST_GLOBAL){
			EIGRP_FUNC_LEAVE(EigrpIpGetOffset);
			return ddb->pdb->offset[ (U8 ) direction ].offset;
		}
		EIGRP_ASSERT((U32)0);
		EIGRP_FUNC_LEAVE(EigrpIpGetOffset);
		return 0;
	}
}

/************************************************************************************

	Name:	EigrpIpBitsInMask

	Desc:	This function is to get the bit length of the given mask.
		
	Para: 	mask	- the given mask
	
	Ret:		the bit length of the given mask
************************************************************************************/

U32	EigrpIpBitsInMask(U32 mask)
{
	U32	thebits = 0;

	EIGRP_FUNC_ENTER(EigrpIpBitsInMask);
	while(mask & 0x80000000){
		thebits++;
		mask <<= 1;
	}
	EIGRP_FUNC_LEAVE(EigrpIpBitsInMask);
	
	return thebits;
}

/************************************************************************************

	Name:	EigrpIpBytesInMask

	Desc:	This function is to get the bit length of the given mask, in counts of 8.
		
	Para: 	mask	- the given mask
	
	Ret:		the bit length of the given mask, in counts of 8.
************************************************************************************/

U32	EigrpIpBytesInMask(U32 mask)
{
	return ((EigrpIpBitsInMask(mask) + 7) / 8 );
}

/************************************************************************************

	Name:	EigrpIpPrintNetwork

	Desc:	This function is to print the network address and its mask into string.
		
	Para: 	addr		- pointer to the structure which indicate the network address
	
	Ret:		pointer to the string
************************************************************************************/

S8 *EigrpIpPrintNetwork(EigrpNetEntry_st *addr)
{
	static S8 buffer[ 128 ];

	EIGRP_FUNC_ENTER(EigrpIpPrintNetwork);
	sprintf_s(buffer, sizeof(buffer), "%u.%u.%u.%u/%u", (addr->address & 0xff000000) >> 24,
					(addr->address & 0xff0000) >> 16, (addr->address & 0xff00) >> 8, addr->address & 0xff,
					EigrpIpBitsInMask(addr->mask));
	EIGRP_FUNC_LEAVE(EigrpIpPrintNetwork);

	return buffer;
}

/************************************************************************************

	Name:	EigrpIpSummaryRevise

	Desc:	This function is to attempt to update a summary metric.  Returns instructions as to
			whether updating the summary in the topo table is necessary, and if so whether the 
			metric should be found by searching the topo table.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			sum		- pointer to the summary entry
			added	- the sign by which we judge whether to add a new route
			metric	- the new metric
	
	Ret:		EIGRP_SUM_UPDATE_METRIC	for updating the summary with the new metric
			EIGRP_SUM_FIND_METRIC		for finding the new metric and updating the summary metric
			EIGRP_SUM_NO_CHANGE		for  do not need to update the summary 
************************************************************************************/

U32 EigrpIpSummaryRevise(EigrpDualDdb_st *ddb, EigrpSum_st *sum, S32	added,   U32 metric)
{
	U32 result;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpIpSummaryRevise);
	EigrpDualLogAll(ddb, EIGRP_DUALEV_SUMREV, &sum->address, &sum->mask);
	EigrpDualLogAll(ddb, EIGRP_DUALEV_SUMREV2, &added, &metric);

	if(added){
		/* Added the route. */
		/* Route is being added.  If the new route has a better metric than the old, update the
		  * summary with the new metric.  If the new route has a worse metric than the old, update
		  * the summary by searching for the route with the best metric(since this worse route
		  * might have been the best one just a moment ago).  If the metrics are equal, we're safe 
		  * and do nothing. */
		if(metric < sum->minMetric){
			/* We're the best */
			result = EIGRP_SUM_UPDATE_METRIC;
			sum->minMetric = metric;
		}else if(metric > sum->minMetric){
			/* Something else was better */
			result = EIGRP_SUM_FIND_METRIC;
		}else{
			/* Metric is the same */
			result = EIGRP_SUM_NO_CHANGE;
		}
	}else{	/* Deleted the route */
		/* Deleting the route.  If the metric of the route being deleted is worse than the best 
		  * metric, we're all set.  Otherwise(the metrics are equal)search for a new metric. */
		if((metric > sum->minMetric)&&(metric!= 0xffffffff)){   
			result = EIGRP_SUM_NO_CHANGE;
		}else{
			result = EIGRP_SUM_FIND_METRIC;
		}
	}
	EigrpDualLogAll(ddb, EIGRP_DUALEV_SUMREV3, &sum->minMetric, &result);
	EIGRP_FUNC_LEAVE(EigrpIpSummaryRevise);
	
	return result;
}

/************************************************************************************

	Name:	EigrpIpEnqueueSummaryEntryEvent

	Desc:	This function is to add or delete summary address from topology table.

			The supplied metric is used if adding the summary.  If no metric is supplied(a NULL
			pointer) an added summary will be assigned the	appropriate minima.
 
 			add is FALSE if the summary is being deleted, TRUE if it is being added or updated.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			summary	- pointer to summary network entry
			metric	- pointer to the summary metric
			add		- the sign by which we judge adding or deleting
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueSummaryEntryEvent(EigrpPdb_st *pdb, EigrpSum_st *summary,
														EigrpVmetric_st *metric, S32 add)
{
	EigrpDualDdb_st *ddb;
	EigrpWork_st *work;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpIpEnqueueSummaryEntryEvent);
	if(!pdb->ddb){     
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueSummaryEntryEvent);
		return;
	}

	ddb		= pdb->ddb;
	work		= (EigrpWork_st *) EigrpUtilChunkMalloc(ddb->workQueChunk);
	if(work == NULL) {    
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueSummaryEntryEvent);
		return;
	}

	EigrpUtilMemZero((void *) work, sizeof(EigrpWork_st));
	work->c.sum.dest.address	= summary->address;
	work->c.sum.dest.mask	= summary->mask;
	work->c.sum.add			= add;
	if(metric){
		work->c.sum.vmetric = EigrpPortMemMalloc(sizeof(EigrpVmetric_st));
		if(work->c.sum.vmetric == NULL){
			EigrpUtilChunkFree(ddb->workQueChunk, work);
			EIGRP_FUNC_LEAVE(EigrpIpEnqueueSummaryEntryEvent);
			return;
		}
		*work->c.sum.vmetric = *metric;
	}else{
		work->c.sum.vmetric = NULL;
	}
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_SUMMARY);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueSummaryEntryEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpSummaryDepend

	Desc:	This function is to process summary based on a more specific route coming or going.
 
 			added is TRUE if the route itself is going up, or FALSE if it is going down.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			address	- the ip address of the (added or deleted) route
			mask	- the ip mask of the (added or deleted) route
			vecmetric	- the vector metric of the (add or deleted) route
			metric		- the composite metric of (add or deleted) route
			added		- the sign of adding or deleting
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpSummaryDepend(EigrpPdb_st *pdb, U32 address, U32 mask,
										EigrpVmetric_st *vecmetric, U32 metric, S32 added)
{
	EigrpSum_st	*sum;
	EigrpDualDdb_st	*ddb;
	U32 result;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpIpSummaryDepend);
	ddb = pdb->ddb;

	/* Update all summaries which depend upon this route. */
	for(sum = (EigrpSum_st*)pdb->sumQue->head; sum; sum = sum->next){

		/* The summary itself changed. Only deal with non-summary route change. */
		if((sum->address == (address & mask)) && (sum->mask == mask)){
			continue;
		}
		if(sum->address != (address & sum->mask)){
			continue;
		}
		result = EigrpIpSummaryRevise(ddb, sum, added, metric);
		switch(result){
			case EIGRP_SUM_UPDATE_METRIC:
				EigrpIpEnqueueSummaryEntryEvent(pdb, sum, vecmetric, TRUE);
				_EIGRP_DEBUG("EigrpIpSummaryDepend: update metric\n");
				/* EigrpIpEnqueueSummaryEntryEvent(pdb, sum, vecmetric, added); */
				break;

			case EIGRP_SUM_FIND_METRIC:
				if(!sum->beingRecal){
					EigrpDualLogAll(ddb, EIGRP_DUALEV_SUMDEP, NULL, NULL);
					EigrpIpEnqueueSummaryEntryEvent(pdb, sum, NULL, TRUE);
					_EIGRP_DEBUG("EigrpIpSummaryDepend: find metric\n");
					/* EigrpIpEnqueueSummaryEntryEvent(pdb, sum, NULL, added); */
					sum->beingRecal = TRUE;
				}
				break;

			case EIGRP_SUM_NO_CHANGE:
				break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpSummaryDepend);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpRtUpdate

	Desc:	This function is to add routes into eigrp single process table(ddb->pdb->rtTbl)
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to network entry
			drdb		- pointer to routing entry
			promote	- pointer to the sign by which we judge whether to tell EIGRP to import
					   the route change
	
	Ret:		TRUE	for success
			FALSE	for fail
************************************************************************************/

S32	EigrpIpRtUpdate(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb, S32 *promote)
{
	EigrpRtNode_pt	rt;
	EigrpPdb_st		*pdb = ddb->pdb;
	EigrpDualRdb_st	*rdb;
	EigrpRouteInfo_st	*eigrpInfo;
	S32 		redistribute_exist, connected_exist;
	_EIGRP_DEBUG("EigrpIpRtUpdate ENTER\n");

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpIpRtUpdate);
	*promote = redistribute_exist = connected_exist = FALSE;

	/* we should never installed summary route in our route table. AND we should not install a
	  * redistribute route in route table. */
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	if(drdb->origin == EIGRP_ORG_SUMMARY || drdb->origin == EIGRP_ORG_REDISTRIBUTED){
		EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
		return FALSE;
	}
#endif//(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	/* In the case of we have a redistributed route,
	  * 1. if it remains usable(active), we should always trust and use it as our successor.
	  * 2. if the new drdb is a eigrp extern route and it metric large than the redistributed
	  * route, we should not add it to our route-table because it is possible a loop route; in
	  * other case, we can add it to our route-table.
	  *
	  * In the case of we have a connected origined drdb, and we learn another drdb in the same
	  * dndb, we should prompt it when the connected route is existing in our topology table. */

	for(rdb = dndb->rdb; rdb; rdb = rdb->next){
		if(rdb->origin == EIGRP_ORG_SUMMARY){
			EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
			return FALSE;
		}

		if(rdb->origin == EIGRP_ORG_CONNECTED){
			/* return FALSE; */
			connected_exist = TRUE;
		}
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)

		if(rdb->origin == EIGRP_ORG_REDISTRIBUTED
			|| rdb->origin == EIGRP_ORG_RSTATIC
			|| rdb->origin == EIGRP_ORG_RCONNECT){
			if(rdb->metric <= drdb->metric){
				EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
				return FALSE;
			}
				redistribute_exist = TRUE;
		}
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
			if(rdb->origin == EIGRP_ORG_REDISTRIBUTED
				|| rdb->origin == EIGRP_ORG_RSTATIC
				|| rdb->origin == EIGRP_ORG_RCONNECT){
				redistribute_exist = TRUE;

			}

#endif//(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	}

	rt	= EigrpUtilRtNodeFindExact(pdb->rtTbl, dndb->dest.address, dndb->dest.mask);
	if(!rt){
		rt	= EigrpUtilRtNodeAdd(pdb->rtTbl, dndb->dest.address, dndb->dest.mask);
	}

	if(!rt){	
		EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
		return FALSE;
	}
	EIGRP_ASSERT((U32)rt);
	eigrpInfo = (EigrpRouteInfo_st*)rt->extData;

	if(eigrpInfo){
		/* If we get a non-null rt here, it means we have a same destination route already, but
		  * maybe different nexthop and metric !*/

		/* Not a eigrp route, probably a redistribute route here */
		if(eigrpInfo->type != EIGRP_ROUTE_EIGRP){                /* should not occur */
			EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
			return FALSE;
		}

		if(drdb->nextHop != eigrpInfo->nexthop.s_addr){    
			if(eigrpInfo->metric < drdb->metric){
				EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
				return FALSE;
			}

			if(eigrpInfo->metric == drdb->metric){
				/* currently, we don't add equal metric route into route table */
				EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
				return FALSE;
			}
		}

		if(redistribute_exist ||connected_exist){
			*promote = FALSE;
		}else{
			*promote = TRUE;
		}
		/* We can only add a single route node here so if we have a node with smaller metric here,
		  * just revise its back pointer to lower drdb */
		EIGRP_ASSERT((U32)eigrpInfo);
		EIGRP_ASSERT((U32)eigrpInfo->metric >= drdb->metric);

		/* Delete old larger metirc route */
		EigrpPortCoreRtDel(dndb->dest.address, dndb->dest.mask, eigrpInfo->nexthop.s_addr, eigrpInfo->metric);

		eigrpInfo->nexthop.s_addr	= drdb->nextHop;
		eigrpInfo->process		= ddb->pdb->process;
		eigrpInfo->metricDetail	= drdb->vecMetric;
		eigrpInfo->metric			= drdb->metric;
		if((eigrpInfo->isExternal = drdb->flagExt)){
			eigrpInfo->distance = ddb->pdb->distance2;
		}else{
			eigrpInfo->distance = ddb->pdb->distance;
		}
		
		/* Update zebra table */

#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
		/* Update zebra table */
		if(BIT_TEST(drdb->iidb->idb->flags, EIGRP_INTF_FLAG_POINT2POINT) && drdb->origin == EIGRP_ORG_CONNECTED){
			EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->iidb->idb->remoteIpAddr, drdb->metric,
								eigrpInfo->distance, eigrpInfo->process);
		}else if(drdb->origin != EIGRP_ORG_CONNECTED){
			EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->nextHop, drdb->metric,
								eigrpInfo->distance, eigrpInfo->process) ;
		}
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		if(drdb->origin != EIGRP_ORG_REDISTRIBUTED){
			if(BIT_TEST(drdb->iidb->idb->flags, EIGRP_INTF_FLAG_POINT2POINT) && drdb->origin == EIGRP_ORG_CONNECTED){
				EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->iidb->idb->remoteIpAddr, drdb->metric, 
									eigrpInfo->distance, eigrpInfo->process);
			}else if(drdb->origin != EIGRP_ORG_CONNECTED){
				EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->nextHop, drdb->metric,
									eigrpInfo->distance, eigrpInfo->process);
			}
		}
#endif//(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
		EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
		return TRUE;
	}

	/* Here we have no previous route node, add a new route node into this pdb with eigrp_info */
	eigrpInfo = (EigrpRouteInfo_st*)EigrpPortMemMalloc(sizeof(EigrpRouteInfo_st));
	if(!eigrpInfo){
		EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
		return FALSE;
	}
	EigrpPortMemSet((U8 *)eigrpInfo, 0, sizeof(EigrpRouteInfo_st));

	/* fill out the eigrp_info */
	if((eigrpInfo->isExternal = drdb->flagExt)){
		eigrpInfo->distance = ddb->pdb->distance2;
	}else{
		eigrpInfo->distance = ddb->pdb->distance;
	}

	eigrpInfo->process	= ddb->pdb->process;
	eigrpInfo->type		= EIGRP_ROUTE_EIGRP;
	eigrpInfo->metric		= drdb->metric;
	eigrpInfo->metricDetail	= drdb->vecMetric;
	eigrpInfo->nexthop.s_addr	= drdb->nextHop;

	rt->extData	= (void *)eigrpInfo;

	if(redistribute_exist ||connected_exist){
		*promote = FALSE;
	}else {   
		*promote = TRUE;
	}
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	/*add to zebra table */
			if(BIT_TEST(drdb->iidb->idb->flags, EIGRP_INTF_FLAG_POINT2POINT) && drdb->origin == EIGRP_ORG_CONNECTED){
				EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->iidb->idb->remoteIpAddr, drdb->metric, 
									eigrpInfo->distance, eigrpInfo->process);
			}else if(drdb->origin != EIGRP_ORG_CONNECTED){
				EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->nextHop, drdb->metric,
									eigrpInfo->distance, eigrpInfo->process);
			}
#elif	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	if(drdb->origin != EIGRP_ORG_REDISTRIBUTED){
			if(BIT_TEST(drdb->iidb->idb->flags, EIGRP_INTF_FLAG_POINT2POINT) && drdb->origin == EIGRP_ORG_CONNECTED){
				EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->iidb->idb->remoteIpAddr, drdb->metric,
									eigrpInfo->distance, eigrpInfo->process);
			}else if(drdb->origin != EIGRP_ORG_CONNECTED){
				EigrpPortCoreRtAdd(dndb->dest.address, dndb->dest.mask, drdb->nextHop, drdb->metric, 
									eigrpInfo->distance, eigrpInfo->process);
			}
	}
#endif//(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	dndb->succNum = 1;

	/* Check if this route affects a summary. */
	if(drdb->origin != EIGRP_ORG_SUMMARY){
		EigrpIpSummaryDepend(pdb, dndb->dest.address, dndb->dest.mask,
									&drdb->vecMetric, drdb->metric, TRUE);
	}
	EIGRP_FUNC_LEAVE(EigrpIpRtUpdate);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpAddrMatch

	Desc:	This function is to check if the given two ip address are same.
		
	Para: 	addr1	- pointer to one ip address
			addr2	- pointer to the other ip address
	
	Ret:		TRUE	for they are equal
			FALSE	for it is not so
************************************************************************************/

S32	EigrpIpAddrMatch(U32 *addr1, U32 *addr2)
{
	return (*addr1 == *addr2);
}

/************************************************************************************

	Name:	EigrpIpPrintAddr

	Desc:	This function is to print an given ip address into string.
		
	Para: 	addr		- pointer to the given ip address
	
	Ret:		pointer to the string
************************************************************************************/

S8 *EigrpIpPrintAddr(U32 *addr)
{
	static S8 buffer[64];

	EIGRP_FUNC_ENTER(EigrpIpPrintAddr);
	sprintf_s(buffer, sizeof(buffer), "%u.%u.%u.%u", (*addr & 0xff000000) >> 24,
					(*addr & 0xff0000) >> 16, (*addr & 0xff00) >> 8, *addr & 0xff);
	EIGRP_FUNC_LEAVE(EigrpIpPrintAddr);
	return(buffer);
}

/************************************************************************************

	Name:	EigrpIpNetMatch

	Desc:	This function is to check if the given two network belong to the same subnet.
		
	Para: 	net1		- pointer to one network entry
			net2		- pointer to the other network entry
	
	Ret:		TRUE	for the given two network belong to the same subnet
			FALSE	for it is not
************************************************************************************/

S32	EigrpIpNetMatch(EigrpNetEntry_st *net1, EigrpNetEntry_st *net2)
{
	_EIGRP_DEBUG("EigrpIpNetMatch(net1:%s, %s;  net2:%s, %s) ENTER\n",
					EigrpUtilIp2Str(net1->address),
					EigrpUtilIp2Str(net1->mask),
					EigrpUtilIp2Str(net2->address),
					EigrpUtilIp2Str(net2->mask));

	if(((net1->address & net1->mask) == (net2->address & net2->mask)) &&  
			(net1->mask == net2->mask)){
		return	TRUE;
	}

	return	FALSE;
}

/************************************************************************************

	Name:	EigrpIpAddrCopy

	Desc:	This function is to copy an IP address from source to dest. First byte specifies length
 			of address. Length returned is(length of address + 1).
		
	Para: 	source	- pointer to the source ip address
			dest		- pointer to the destination ip address
	
	Ret:		the size of address plus 1
************************************************************************************/

U32	EigrpIpAddrCopy(U32 *source, S8 *dest)
{
	EIGRP_FUNC_ENTER(EigrpIpAddrCopy);
	*dest	= sizeof(U32);
	dest++;
	*(U32*)dest	= *source;

	EIGRP_FUNC_LEAVE(EigrpIpAddrCopy);
	
	return(sizeof(U32) + 1);
}

/************************************************************************************

	Name:	EigrpIpLocalAddr

	Desc:	This function is to see if supplied address is one of the system's addresses.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			address	- pointer to the supplied address
	
	Ret:		TRUE	for the suppllied address is one of the system's address
			FALSE	for it is not so
************************************************************************************/

S32	EigrpIpLocalAddr(EigrpDualDdb_st *ddb, U32 *address, EigrpIntf_pt pEigrpIntf)
{
	EigrpIdb_st	*iidb;
	EigrpIntf_pt	pEigrpIntfTem;
	EigrpIntfAddr_pt	pAddr;

	EIGRP_FUNC_ENTER(EigrpIpLocalAddr);
	for(iidb = (EigrpIdb_st *)(ddb->iidbQue->head); iidb; iidb = iidb->next){
		pEigrpIntfTem = iidb->idb;
		for(pAddr = EigrpPortGetFirstAddr(pEigrpIntfTem->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
			if(EigrpIpAddrMatch(&(pAddr->ipAddr), address)){
				EigrpPortMemFree(pAddr);
				EIGRP_FUNC_LEAVE(EigrpIpLocalAddr);
				return TRUE;
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpLocalAddr);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpIpRtDelete

	Desc:	This function is to delete an entry from the routing table only if it belongs to us.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- pointer to the entry which is to be deleted
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpRtDelete(EigrpDualDdb_st *ddb, EigrpDualNewRt_st*route)
{
	EigrpRtNode_pt	rt;
	EigrpRouteInfo_st	*eigrpInfo;
	_EIGRP_DEBUG("EigrpIpRtDelete ENTER\n");

	EIGRP_FUNC_ENTER(EigrpIpRtDelete);
	if(!route){
		EIGRP_ASSERT((U32)0);
		EIGRP_FUNC_LEAVE(EigrpIpRtDelete);
		return;
	}

	/* we have never installed summary route in our route table. If drdb's origin is 
	  * EIGRP_ORG_REDISTRIBUTED, we could return now. */
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	if(route->origin == EIGRP_ORG_SUMMARY   || route->origin == EIGRP_ORG_REDISTRIBUTED){
		EIGRP_FUNC_LEAVE(EigrpIpRtDelete);
		return;
	}
#endif//(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_A)
	rt = EigrpUtilRtNodeFindExact(ddb->pdb->rtTbl, route->dest.address, route->dest.mask);
	if(!rt){
		EIGRP_FUNC_LEAVE(EigrpIpRtDelete);
		return;
	}

	eigrpInfo = (EigrpRouteInfo_st *)rt->extData;

	EIGRP_ASSERT((U32)eigrpInfo);

	if(route->nextHop == eigrpInfo->nexthop.s_addr){
		/* It was me who added this route. So I must delete it from pdb->rtTbl and zebra table as
		  * well. */
		EigrpPortCoreRtDel(route->dest.address, route->dest.mask, route->nextHop, eigrpInfo->metric);

		EigrpPortMemFree(eigrpInfo);
		rt->extData	= NULL;
		EigrpUtilRtNodeDel(ddb->pdb->rtTbl, rt);

		EigrpIpSummaryDepend(ddb->pdb, route->dest.address, route->dest.mask,
									&route->vecMetric, route->metric, FALSE);
	}
	EIGRP_FUNC_LEAVE(EigrpIpRtDelete);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpPeerBucket

	Desc:	This function is to return slot based on IP address.
		
	Para: 	address		- pointer to the given ip address
	
	Ret:		the slot based on the ip address
************************************************************************************/

S32	EigrpIpPeerBucket(U32 *address)
{
	return(EigrpUtilNetHash(*address));
}

/************************************************************************************

	Name:	EigrpIpNdbBucket

	Desc:	This function is to return slot based on given address.
		
	Para: 	address		- pointer to the given address
	
	Ret:		the slot based on the given address
************************************************************************************/

S32	EigrpIpNdbBucket(EigrpNetEntry_st *address)
{
	return(EigrpUtilNetHash(address->address));
}

/************************************************************************************

	Name:	EigrpIpHeaderPtr

	Desc:	This function is to return pointer to beginning of eigrp fixed length header.
		
	Para: 	pak		- pointer to the ip packet
				
	Ret:		pointer to the eigrp packet header
************************************************************************************/

EigrpPktHdr_st *EigrpIpHeaderPtr(void *pak)
{
	struct EigrpIpHdr_ *ip;
	EigrpPktHdr_st *eigrph;
	S32 iplen;

	EIGRP_FUNC_ENTER(EigrpIpHeaderPtr);
	ip = (struct EigrpIpHdr_ *)pak;

	iplen		= (S32) ip->hdrLen * 4;
	eigrph	= (EigrpPktHdr_st*)((U8 *) ip + iplen);
	EIGRP_FUNC_LEAVE(EigrpIpHeaderPtr);

	return(eigrph);
}

/************************************************************************************

	Name:	EigrpIpEnqueueConnRtchangeEvent

	Desc:	This function is to enqueue a directly connected route change event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the interface of directly connected route
			address		- ip address
			mask		- ip mask
			sense		- the sign by which we judge adding or deleting
			config		- manual configuration if it is TRUE
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueConnRtchangeEvent(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf,
												U32 address, U32 mask,
												S32 sense, S32 config)
{
	EigrpWork_st *work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueConnRtchangeEvent);

	_EIGRP_DEBUG("EigrpIpEnqueueConnRtchangeEvent ENTER\n");

	if(!pdb->ddb){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueConnRtchangeEvent);
		return;
	}

	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueConnRtchangeEvent);
		return;
	}

	work->c.con.ifp		= pEigrpIntf;
	work->c.con.dest.address	= address & mask;
	work->c.con.dest.mask	= mask;
	work->c.con.up		= sense;
	work->c.con.config	= config;
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_CONNSTATE);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueConnRtchangeEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpIsExterior

	Desc:	
		
	Para: 
	
	Ret:		
************************************************************************************/

S32	EigrpIpIsExterior(U32 addr, U32 mask)
{
	return FALSE;
}

/************************************************************************************

	Name:	EigrpIpAutoSummaryNeeded

	Desc:	An auto-summary is needed for the net/mask on this interface if the
			interface is speaking EIGRP and none of its addresses fall under
			the net/mask.

			Returns TRUE if a summary is needed.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given interface
			sum_addr	- summary ip address
			sum_mask	- summary ip mask
				
	Ret:		TURE	for the summary is needed
			FALSe	for it is not so
************************************************************************************/

S32	EigrpIpAutoSummaryNeeded(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf, U32 sum_addr, U32 sum_mask)
{
	U32 if_address;
#if	(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_11)
	EigrpSum_st	*summary;
	EigrpSumIdb_st	*sum_intf;
#endif//(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_11)

	EIGRP_FUNC_ENTER(EigrpIpAutoSummaryNeeded);
	if(!pEigrpIntf || !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
		/* the interface had gone or down.added by  2002.11.22 */
		EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
		return  FALSE;
	}
	/* No summary needed if the interface is passive. */
	if(!EigrpIpGetNetsPerIdb(&pdb->netLst, pEigrpIntf)){
		EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
		return  FALSE;
	}
	/* No summary needed if the address is not in our process. */
	if(!EigrpIpGetNetsPerAddress(&pdb->netLst, sum_addr)){
		EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
		return  FALSE;
	}

	if_address = pEigrpIntf->ipAddr;
	if(if_address == (U32)0){
		EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
		return  FALSE;
	}

	/* No summary if main address is not covered by summary. */
	if((if_address & sum_mask) != sum_addr){
		EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
		return  FALSE;
	}

#if	(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_11)
	for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = summary->next){
		for(sum_intf = (EigrpSumIdb_st*)summary->idbQue->head; sum_intf; sum_intf = sum_intf->next){
			if(!BIT_TEST(sum_intf->flag, EIGRP_SUM_TYPE_CFG)){
				continue;
			}

			if(sum_intf->idb == pEigrpIntf){
				EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
				return FALSE;
			}
		}
	}
#endif//(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_11)
	EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryNeeded);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpFindSumIdb

	Desc:	This function is to find a given idb for a summary address.
		
	Para: 	summary		- pointer to the summary network
			pEigrpIntf	- pointer to the given idb
	
	Ret:		pointer to the found summary interface, or NULL if nothing is found
************************************************************************************/

EigrpSumIdb_st *EigrpIpFindSumIdb(EigrpSum_st *summary, EigrpIntf_pt pEigrpIntf)
{
	EigrpSumIdb_st *sum_idb = (EigrpSumIdb_st *)0;

	EIGRP_FUNC_ENTER(EigrpIpFindSumIdb);
	for(sum_idb = (EigrpSumIdb_st*)summary->idbQue->head; sum_idb; sum_idb = sum_idb->next){
		if(sum_idb->idb ==  pEigrpIntf){
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpFindSumIdb);
	
	return(sum_idb);
}

/************************************************************************************

	Name:	EigrpIpDeleteSummaryInterface

	Desc:	This function is to delete a single summary entry.  Triggers the appropriate DUAL
			response.
	 
 			It may delete the entire summary entry.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			summary	- pointer to the summary network
			sum_idb	- pointer to the summary interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpDeleteSummaryInterface(EigrpPdb_st *pdb, EigrpSum_st *summary, EigrpSumIdb_st *sum_idb)
{
	EIGRP_FUNC_ENTER(EigrpIpDeleteSummaryInterface);
	EigrpTakeNbrsDown(pdb->ddb, sum_idb->idb, FALSE, "summary deleted");

	EigrpUtilQue2Unqueue(summary->idbQue, (EigrpQueElem_st *)sum_idb);

	EigrpPortMemFree(sum_idb);

	if(summary->idbQue->count == 0){
		EigrpIpEnqueueSummaryEntryEvent(pdb, summary, NULL, FALSE);
		_EIGRP_DEBUG("EigrpIpDeleteSummaryInterface: update metric\n");
		EigrpUtilQue2Unqueue(pdb->sumQue, (EigrpQueElem_st *)summary);
		EigrpPortMemFree(summary);
	}
	EIGRP_FUNC_LEAVE(EigrpIpDeleteSummaryInterface);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpConfigureSummaryEntry

	Desc:	This function is to add/delete summary address from pdb. In this function, we 
			deliberately take neighbors down over every interface the summary is configured.  This 
			allows an easy way to, in the adding case to delete the more specific destinations for
			the summary in the neighboring topology tables, and in the deleting case to announce
			more specific destination to neighbors.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			address	- the summary ip address to be added/deleted
			mask	- the ip mask of the address
			pEigrpIntf	- pointer summary interface
			sense		- the sign by which we judge adding or deleting
			summary_type	- type of summary address(manual summary or auto summary)
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpConfigureSummaryEntry(EigrpPdb_st *pdb, U32 address, U32 mask, EigrpIntf_pt pEigrpIntf, S32 sense, U8 summary_type)
{
	EigrpSum_st	*summary;
	EigrpSumIdb_st	*sum_intf;
	
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpIpConfigureSummaryEntry);
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"\nIP-EIGRP: configure_summary: %s %s/%d %s\n", pEigrpIntf->name, 
						EigrpUtilIp2Str(address), EigrpIpBitsInMask(mask), sense ? "on" : "off");
	sum_intf = NULL;

	/* Normalize address. */
	address = address & mask;

	/* Find summary, if any. */
	for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = summary->next){
		if(summary->address == address && summary->mask == mask){
			sum_intf = EigrpIpFindSumIdb(summary, pEigrpIntf);
			break;
		}
	}

	/* Adding a summary address. */
	if(sense){
		if(!summary){
			summary = EigrpPortMemMalloc(sizeof(EigrpSum_st));
			if(!summary){
				EIGRP_FUNC_LEAVE(EigrpIpConfigureSummaryEntry);
				return;
			}
			EigrpUtilMemZero((void *)summary, sizeof(EigrpSum_st));
			summary->idbQue		= EigrpUtilQue2Init();
			summary->address	= address;
			summary->mask		= mask;
			summary->minMetric	= EIGRP_METRIC_INACCESS;
			EigrpUtilQue2Enqueue(pdb->sumQue, (EigrpQueElem_st *)summary);
		}
		if(!sum_intf){
			sum_intf = EigrpPortMemMalloc(sizeof(EigrpSumIdb_st));
			if(!sum_intf){
				EIGRP_FUNC_LEAVE(EigrpIpConfigureSummaryEntry);
				return;
			}
			EigrpUtilMemZero((void *)sum_intf, sizeof(EigrpSumIdb_st));
			sum_intf->idb = pEigrpIntf;
			EigrpUtilQue2Enqueue(summary->idbQue, (EigrpQueElem_st *)sum_intf);
			if(summary->idbQue->count == 1){
				EigrpIpEnqueueSummaryEntryEvent(pdb, summary, NULL, TRUE);
				_EIGRP_DEBUG("EigrpIpConfigureSummaryEntry: update metric\n");
			}
			EigrpTakeNbrsDown(pdb->ddb, pEigrpIntf, FALSE, "summary added");
		}
		BIT_SET(sum_intf->flag, summary_type);
	}else{
		/* Deleting a summary address. */
		if(summary && sum_intf){
			BIT_RESET(sum_intf->flag, summary_type);
			if(!sum_intf->flag){
				EigrpIpDeleteSummaryInterface(pdb, summary, sum_intf);
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpConfigureSummaryEntry);
	
	return;
}
   
/************************************************************************************

	Name:	EigrpIpBuildAutoSummaries

	Desc:	This function is to updates the set of autosummaries on an interface to match the 
			current configuration using brute force.  Does nothing if autosummaries are disabled.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpBuildAutoSummaries(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf)
{
	EigrpSum_st	*summary, *next_sum;
	EigrpSumIdb_st	*sum_idb, *idbNext;
	EigrpIdb_st		*iidb, *iidb2;
	EigrpDualDdb_st	*ddb;
	U32 net,net0, mask,mask0;
	S32 unique = TRUE;
	
	EIGRP_FUNC_ENTER(EigrpIpBuildAutoSummaries);
	iidb	= NULL;	
	if(pdb->sumAuto != TRUE){
		EIGRP_FUNC_LEAVE(EigrpIpBuildAutoSummaries);
		return;
	}
	if(!pEigrpIntf){	
		EIGRP_FUNC_LEAVE(EigrpIpBuildAutoSummaries);
		return;
	}
	if((U32)(ddb = pdb->ddb)){
		iidb = EigrpFindIidb(ddb, pEigrpIntf);
	}

	/* First walk all summaries looking for autosummary entries that are no longer valid. */
	for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = next_sum){
		next_sum = summary->next;	/* Entry may get deleted! */
		sum_idb = EigrpIpFindSumIdb(summary, pEigrpIntf);

		/* Autosummary there */
		if(sum_idb && (!pEigrpIntf ||!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE) ||  !iidb)){	
			/* the interface had gone or down, so we delete the interface no matter of adding by 
			  * auto or config */
			/* it will be restored when the interface up again.added by  2002.11.22 */
			sum_idb->flag = 0;
			EigrpIpDeleteSummaryInterface(pdb, summary, sum_idb);
		}else if(sum_idb && BIT_TEST(sum_idb->flag, EIGRP_SUM_TYPE_AUTO)){
			/* Autosummary there */
			if(!EigrpIpAutoSummaryNeeded(pdb, pEigrpIntf, summary->address,  summary->mask)){
				BIT_RESET(sum_idb->flag, EIGRP_SUM_TYPE_AUTO);
				if(!sum_idb->flag){
					EigrpIpDeleteSummaryInterface(pdb, summary, sum_idb);
				}
			}
		}
	}

	if(!ddb||!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpBuildAutoSummaries);
		return;
	}

	/* the follwing is added by  2002.11.23. */
	/* to handle to interface delete/down case */
	net0 = pEigrpIntf->ipAddr;
	if (net0 == 0){
		EIGRP_FUNC_LEAVE(EigrpIpBuildAutoSummaries);
		return;
	}

	mask0	= EigrpPortSock2Ip(EIGRP_INET_MASK_NATURAL_BYTE(net0));
	net0		= net0 & mask0;
	for(iidb2 = (EigrpIdb_st *)pdb->ddb->iidbQue->head; iidb2; iidb2=iidb2->next){  
		if(iidb2->idb==pEigrpIntf){
			continue;
		}

		net = iidb2->idb->ipAddr;
		if(net == 0){
			continue;
		}

		mask	= EigrpPortSock2Ip(EIGRP_INET_MASK_NATURAL_BYTE(net));
		net		= net & mask;
		if(net==net0){
			unique = FALSE;
		}
	}

	if(unique){
		for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = next_sum){
			next_sum = summary->next;	
			if(summary->address!=net0){
				continue;
			}
			for(sum_idb = (EigrpSumIdb_st*)summary->idbQue->head; sum_idb; sum_idb = idbNext){
				idbNext	= sum_idb->next;		
				
				if(!pEigrpIntf||!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE) || !iidb){
					sum_idb->flag = 0;
					EigrpIpDeleteSummaryInterface(pdb, summary, sum_idb);
				}else if(!EigrpIpAutoSummaryNeeded(pdb, sum_idb->idb, net0, mask0)){
					BIT_RESET(sum_idb->flag, EIGRP_SUM_TYPE_AUTO);
					if(!sum_idb->flag){
						EigrpIpDeleteSummaryInterface(pdb, summary, sum_idb);
					}
				}
			}
		}
	}
	/* end delete unnecessary summary */

	for(iidb2 = (EigrpIdb_st *)pdb->ddb->iidbQue->head; iidb2; iidb2=iidb2->next){
		if(iidb2->idb != pEigrpIntf){	
			continue;
		}

		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE) && 
					EigrpIpAutoSummaryNeeded(pdb, iidb2->idb, net0, mask0)){  
			EigrpIpConfigureSummaryEntry(pdb, net0, mask0, iidb2->idb, TRUE, EIGRP_SUM_TYPE_AUTO);
		}

	}  		

	EIGRP_FUNC_LEAVE(EigrpIpBuildAutoSummaries);

	return;
 }

/************************************************************************************

	Name:	EigrpIpAdjustConnected

	Desc:	This function is to adjust all directly connected routes for an interface.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given interface of directly connected routes
			sense		- the sign by which we judge adding or deleting
			config		- manual configuration if it is TRUE
				
	Ret:		NONE
************************************************************************************/

void	EigrpIpAdjustConnected(EigrpPdb_st *pdb,  EigrpIntf_pt pEigrpIntf, S32 sense, S32 config)
{
	U32 address, mask;

	EIGRP_FUNC_ENTER(EigrpIpAdjustConnected);
	address	= pEigrpIntf->ipAddr;
	address	&= pEigrpIntf->ipMask;
	mask	= pEigrpIntf->ipMask;
	
	if(EigrpIpGetNetsPerAddress(&pdb->netLst, address)){
		EigrpIpEnqueueConnRtchangeEvent(pdb, pEigrpIntf, address, mask, sense, config);
	}
	EIGRP_FUNC_LEAVE(EigrpIpAdjustConnected);

	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueIfdownEvent

	Desc:	This function is to be called to take down an IIDB.
  			
 			This routine is safe to call from other threads;  it enqueues work queue entries.
		
	Para: 	pdb			- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer the interface whose state has changed
			isdelete		- the sign by which we judge whether to delete to interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfdownEvent(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf, S32 isdelete)
{
	EigrpWork_st	*work;
	EigrpIdb_st	*iidb;
	EigrpDualDdb_st	*ddb;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfdownEvent);
	EigrpIpAdjustConnected(pdb, pEigrpIntf, FALSE, isdelete);

	ddb	= pdb->ddb;
	iidb	= EigrpFindIidb(ddb, pEigrpIntf);
	if(!iidb){  
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfdownEvent);
		return;
	}

	/* Take the interface down. */
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(ddb->workQueChunk);
	if(work == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfdownEvent);
		return;
	}

	iidb->goingDown	= isdelete;
	work->c.ifd.ifp	= pEigrpIntf;
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IF_DOWN_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfdownEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpSetupMulticast

	Desc:	This function is to turn multicast processing on or off on the interface as necessary.
		
	Para: 	ddb		- pointer to the dual descriptor block
			pEigrpIntf	- pointer to the interface
			sense		- if TRUE	, turn the multicast processing on
						   if FALSE, turn the multicast processing off
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpSetupMulticast(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf, S32 sense)
{
	EigrpPdb_st	*pdb;
	EigrpDualDdb_st	*other_ddb;
	EigrpIdb_st	*iidb, *other_iidb;
	U32	other_processes;

	EIGRP_FUNC_ENTER(EigrpIpSetupMulticast);
	iidb = EigrpFindIidb(ddb, pEigrpIntf);
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpSetupMulticast);
		return;
	}

	/* Bail if the interface is already in the right state. */
	if(sense == iidb->mcastEnabled){
		EIGRP_FUNC_LEAVE(EigrpIpSetupMulticast);
		return;
	}

	/* If we're enabling EIGRP on this interface and this is the first  EIGRP process on the
	  * interface, enable the multicast.  If we're disabling EIGRP and we're the last EIGRP 
	  * process on the interface, disable the multicast. */
	other_processes = 0;
	for(pdb = (EigrpPdb_st*)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		other_ddb = pdb->ddb;
		if(!other_ddb){
			continue;
		}
		if(other_ddb == ddb){       		/* Skip our process */
			continue;
		}
		for(other_iidb = (EigrpIdb_st*) other_ddb->iidbQue->head; other_iidb; other_iidb = other_iidb->next){
			if(other_iidb->idb == pEigrpIntf){
				other_processes++;
			}
		}
	}

	if(!other_processes){
		if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST)){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: multicast not supported on interface %s\n",
						pEigrpIntf->name);
			EIGRP_FUNC_LEAVE(EigrpIpSetupMulticast);
			return;
		}

#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifdef _DC_
		EigrpPortMcastOptionWithLayer2(gpEigrp->socket, pEigrpIntf, sense);
#else//_DC_
		EigrpPortMcastOption(gpEigrp->socket, pEigrpIntf, sense);
#endif//_DC_
#elif((EIGRP_OS_TYPE == EIGRP_OS_PIL) ||(EIGRP_OS_TYPE == EIGRP_OS_LINUX))
		EigrpPortMcastOption(gpEigrp->socket, pEigrpIntf, sense);
#endif

		iidb->mcastEnabled = sense;
	}
	EIGRP_FUNC_LEAVE(EigrpIpSetupMulticast);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpSetMtu

	Desc:	This function is set the mtu of an iidb.
		
	Para: 	iidb		- pointer to the given iidb
		
	Ret:		NONE
************************************************************************************/

void	EigrpIpSetMtu (EigrpIdb_st *iidb)
{
	EigrpIntf_pt pEigrpIntf;
	U32	mtu;

	EIGRP_FUNC_ENTER(EigrpIpSetMtu);
	if(iidb){
		pEigrpIntf = iidb->idb;

		/* iidb->maxPktSize : NOT include IP header, but include EIGRP header
		  * idb->ifa_mtu = mtu - 64 : NOT include IP header. so it is not necessary to sub 60 again.
		  *
		  * but when idb->ifa_mtu<108, iidb->maxPktSize should be larger to carry at least one 
		  * TLV. */

#define MIN_PACKET_SIZE    108
		mtu	= pEigrpIntf->mtu;

		if(mtu >= MIN_PACKET_SIZE){
			iidb->maxPktSize = mtu;
		}else{
			iidb->maxPktSize = MAX(mtu * 2, MIN_PACKET_SIZE);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpSetMtu);

	return;
}

/************************************************************************************

	Name:	EigrpIpOnoffIdb

	Desc:	This function is to turn an interface on or off for EIGRP.  This may cause work queue 
			entries to be enqueued, since we can't necessarily do it all on this thread.
  
 			If "passive" is true, the IIDB is considered a passive interface (significant only when
 			"sense" is TRUE).
	
			Tinkers with the multicast addresses appropriately.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the given interface to be turned on/off
			passive		- the sign by which we judge whether the interface is passive or active
			
	Ret:		NONE
************************************************************************************/

void	EigrpIpOnoffIdb(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf, S32 sense, S32 passive)
{
	EigrpIdb_st *iidb;

	EIGRP_VALID_CHECK_VOID();
	_EIGRP_DEBUG("EigrpIpOnoffIdb(%s, %d, %d) ENTER\n",
						pEigrpIntf->name,
						sense,
						passive);

	EIGRP_FUNC_ENTER(EigrpIpOnoffIdb);
	iidb = EigrpFindIidb(ddb, pEigrpIntf);
	if(sense){
		/* activate it */
		if(iidb){
			/* It's already there */
			iidb->goingDown	= FALSE; /* Stop any enqueued deletion */
			iidb->useMcast	= BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST) ? TRUE : FALSE;
		}else{
			EigrpAllocateIidb(ddb, pEigrpIntf, passive);
		}

		/* only enable multicast for the on case.  the off case is taken care of when we process 
		  * the interface down event */
		EigrpIpSetupMulticast(ddb, pEigrpIntf, sense);
	}else{
		EigrpIpEnqueueIfdownEvent(ddb->pdb, pEigrpIntf, TRUE);
	}
	EIGRP_FUNC_LEAVE(EigrpIpOnoffIdb);

	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueConnAddressEvent

	Desc:	This function is to enqueue a redist event on the work queue. 
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			address	- the ip address of the directly connected route
			mask	- the ip mask of the directly connected route
			event	- type of the event ,EIGRP_ROUTE_UP or EIGRP_ROUTE_DOWN
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueConnAddressEvent(EigrpPdb_st *pdb, U32 address, U32 mask, S32 event)
{
	EigrpWork_st *work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueConnAddressEvent);

	_EIGRP_DEBUG("EigrpIpEnqueueConnAddressEvent ENTER\n");

	if(!pdb->ddb){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueConnAddressEvent);
		return;
	}

	/* Queue redistribution request and process later. */
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueConnAddressEvent);
		return;
	}

	work->c.red.dest.address	= address;
	work->c.red.dest.mask		= mask;

	work->c.red.event = event;
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_REDIST_CONNECT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueConnAddressEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpRouteOnPdbOrNdb

	Desc:	if this route is in pdb topology, retrun TRUE,  otherwise return FALSE.
			(1). this route is eigrp route and asnum = pdb->asnum;
			(2). this route is not eigrp route, but it redistributed by this pdb.
		
	Para: 	rtNode		- pointer to the given route
			pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		TRUE	for the route is in pdb topology
			FALSE	for the route is not is pdb topology
************************************************************************************/

S32	EigrpIpRouteOnPdbOrNdb(void *rtNode, EigrpPdb_st *pdb)
{
	EigrpRouteInfo_st	*info;
	EigrpNetEntry_st	dest;

	EIGRP_FUNC_ENTER(EigrpIpRouteOnPdbOrNdb);

	_EIGRP_DEBUG("EigrpIpRouteOnPdbOrNdb(%s, %s) ENTER\n",
					EigrpUtilIp2Str(EigrpPortCoreRtGetDest(rtNode)),
					EigrpUtilIp2Str(EigrpPortCoreRtGetMask(rtNode)));

	if(!rtNode || !pdb){
		EIGRP_FUNC_LEAVE(EigrpIpRouteOnPdbOrNdb);
		return FALSE;
	}

	info = (EigrpRouteInfo_st *)EigrpPortCoreRtGetEigrpExt(rtNode);

	if(info && (info->type == EIGRP_ROUTE_EIGRP) && ((U32)info->process == pdb->process)){
		EIGRP_FUNC_LEAVE(EigrpIpRouteOnPdbOrNdb);
		return TRUE;
	}

	dest.address	= EigrpPortCoreRtGetDest(rtNode);
	dest.mask	= EigrpPortCoreRtGetMask(rtNode);
	if(EigrpDualNdbLookup(pdb->ddb, &dest)){
		EIGRP_FUNC_LEAVE(EigrpIpRouteOnPdbOrNdb);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpIpRouteOnPdbOrNdb);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpIpRedistConnState

	Desc:	This function is to cause a directly connected route to be redistributed or not when the 
			"network" configuration changes.
 
 			Note that the PDB reflects the addition or deletion of a network before we are called,
 			so EigrpIpRouteOnPdbOrNdb will correctly reflect the new state of the network command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the interface of the directly connected route
			address		- the ip address of directly connected route
			mask		- the ip mask of directly connnected route
			sense		- if the command is "network",it is true, if the command is "no network",
						   it is false
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpRedistConnState(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf, U32 address, U32 mask,  S32 sense)
{
	void		*pRtNode;
	S32		pdbonndb, retVal;
	
 	EIGRP_FUNC_ENTER(EigrpIpRedistConnState);

	_EIGRP_DEBUG("EigrpIpRedistConnState(%s, %s, %s, %d) ENTER\n",
					pEigrpIntf->name,
					EigrpUtilIp2Str(address),
					EigrpUtilIp2Str(mask),
					sense);

	pRtNode	= EigrpPortCoreRtNodeFindExact(address, mask);
	if(!pRtNode){
		EIGRP_FUNC_LEAVE(EigrpIpRedistConnState);
		return;
	}
	retVal	= EigrpPortCoreRtNodeHasConnect(pRtNode);//确认链表是否有直连路由
	if(retVal == FALSE){
		EIGRP_FUNC_LEAVE(EigrpIpRedistConnState);
		return;
	}

	/* Redistribute in the opposite sense of the setting of the network command(but if the 
	  * interface is down, don't redistribute the route if we would otherwise). */
	pdbonndb = EigrpIpRouteOnPdbOrNdb(pRtNode, pdb);

	if(pdbonndb && !sense){
		/* We own the route */
		EigrpIpEnqueueConnAddressEvent(pdb, address, mask, EIGRP_ROUTE_DOWN);
	}

	if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE) && !pdbonndb && sense){
		/* Don't own;  is it up? */
		_EIGRP_DEBUG("EigrpIpRedistConnState: %x %x\n",address, mask);
		EigrpIpEnqueueConnAddressEvent(pdb, address, mask, EIGRP_ROUTE_UP);//把直连路由扩散出去
	}
	EIGRP_FUNC_LEAVE(EigrpIpRedistConnState);

	return;
}

/************************************************************************************

	Name:	EigrpIpConnSummaryDepend

	Desc:	This function is to check the  summary route dependencies as a directly connected route 
			comes/goes.
		
	Para: 	ddb			- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the directly connected route interface
			address		- the ip address of the directly connected route interface
			mask		- the ip mask of the directly connected route interface
			sense		- it is true for a directly connected route comes, it is false for a 
						  directly route goes
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpConnSummaryDepend(EigrpDualDdb_st *ddb,  EigrpIntf_pt pEigrpIntf, U32 address,   U32 mask, S32 sense)
{
	U32 metric;
	S32 up;

	EIGRP_FUNC_ENTER(EigrpIpConnSummaryDepend);
	up = (BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE) && sense);
	/* Check if interface address was part of summary. */
	metric = EigrpDualComputeMetric(EigrpDualIgrpMetricToEigrp(EIGRP_SCALED_BANDWIDTH(1000 * pEigrpIntf->bandwidth)),
									1, (U32)(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST) ? EIGRP_B_DELAY : EIGRP_S_DELAY),
									255, ddb->k1, ddb->k2, ddb->k3, ddb->k4, ddb->k5 );

	EigrpIpSummaryDepend(ddb->pdb, address, mask, NULL, metric, up);
	EIGRP_FUNC_LEAVE(EigrpIpConnSummaryDepend);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueRedisEntryEvent

	Desc:	This function is to insert an eigrp redistribute event into the system event processing
			queue.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			redisindex	- the index of redistribute route
			sense		- it is true for redistributing route, it is false for stopping 
						  redistributing 
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueRedisEntryEvent(EigrpPdb_st *pdb, U16 redisindex, S32 sense)
{
	EigrpWork_st	*work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueRedisEntryEvent);
	if(!pdb->ddb){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueRedisEntryEvent);
		return;
	}

	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(work == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueRedisEntryEvent);
		return;
	}

	work->c.rea.ulindex	= redisindex;
	work->c.rea.sense		= sense;
	(void)EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_REDISTALL);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueRedisEntryEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessNetMetricCommand

	Desc:	This function is to process metric value to net.
		
	Para: 	noFlag	- the flag of negative command
			ipNet		- dst network
			metric	- the metric value to dst network
	
	Ret:		NONE
************************************************************************************/
void	EigrpIpProcessNetMetricCommand(S32 noFlag, U32 ipNet, void *vMetric)
{
	EigrpNetMetric_pt	pNetMetric;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessNetMetricCommand);
	if(!ipNet){	
		EIGRP_FUNC_LEAVE(EigrpIpProcessNetMetricCommand);
		return;
	}
	if(noFlag == FALSE){
		pNetMetric = EigrpUtilDllSearch((EigrpDll_pt *)&gpEigrp->pNetMetric, (void *)&ipNet, EigrpUtilCmpNet);
		if(!pNetMetric){
			pNetMetric = EigrpPortMemMalloc(sizeof(EigrpNetMetric_st));
			EigrpPortMemSet(pNetMetric, 0, sizeof(EigrpNetMetric_st));
			pNetMetric->ipNet = ipNet;
			pNetMetric->vecmetric = vMetric;
			EigrpUtilDllInsert((EigrpDll_pt)pNetMetric, (EigrpDll_pt *)NULL, (EigrpDll_pt *)&gpEigrp->pNetMetric);
		}else{
			pNetMetric->vecmetric = vMetric;
		}
	}else{
		pNetMetric = EigrpUtilDllSearch((EigrpDll_pt *)&gpEigrp->pNetMetric, (void *)&ipNet, EigrpUtilCmpNet);
		if(pNetMetric){
			EigrpUtilDllRemove((EigrpDll_pt)pNetMetric, (EigrpDll_pt *)&gpEigrp->pNetMetric);
			if(pNetMetric->vecmetric){
				EigrpPortMemFree(pNetMetric->vecmetric);
			}
			EigrpPortMemFree(pNetMetric);
		}
	}
/*	
	EigrpCmdApiRedistType(1, asNumCur, "static", 0, 0);
	EigrpCmdApiRedistType(0, asNumCur, "static", 0, 0);
*/
	EIGRP_FUNC_LEAVE(EigrpIpProcessNetMetricCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessRedisCommand

	Desc:	This function is to process redistribue add/delete command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			noFlag	- the flag of negative command
			rinfo		- pointer to the redistribute information
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessRedisCommand(S32 noFlag, EigrpPdb_st *pdb, EigrpRedis_st *rinfo)
{
	U32 proto, process;
	EigrpRedisEntry_st	*redis;

	EIGRP_FUNC_ENTER(EigrpIpProcessRedisCommand);
	if(!rinfo){	
		EIGRP_FUNC_LEAVE(EigrpIpProcessRedisCommand);
		return;
	}
	proto	= rinfo->srcProto;
	process	= rinfo->srcProc;
	if(noFlag == FALSE){
		redis = EigrpIpRedisEntryInsert(&pdb->redisLst, rinfo);
		if(redis){
			EigrpIpEnqueueRedisEntryEvent(pdb, redis->index, TRUE);
		}
	}else{
		redis = EigrpIpRedisEntrySearchWithProto(pdb->redisLst, (U16)proto, process);
		if(redis){
			EigrpIpEnqueueRedisEntryEvent(pdb, redis->index, FALSE);
		}
	}

	if(rinfo){
		EigrpPortMemFree(rinfo);
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessRedisCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessDefaultRouteCommand

	Desc:	This function is to process all eigrp default route commands.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			isin		- the flag of Accepting default routing information 
			noFlag	- the flag of negative command
				
	Ret:		NONE
************************************************************************************/

 void	EigrpIpProcessDefaultRouteCommand(S32 noFlag, EigrpPdb_st *pdb, S32 isin)
{
	S32 sense, changed = FALSE;

	EIGRP_FUNC_ENTER(EigrpIpProcessDefaultRouteCommand);
	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}

	if(isin == TRUE){
		if(pdb->rtInDef != sense){
			pdb->rtInDef = sense;
			changed = TRUE;
		}
	}else{
		if(pdb->rtOutDef != sense){
			pdb->rtOutDef = sense;
			changed = TRUE;
		}
	}

	if(changed){
		EigrpTakeAllNbrsDown(pdb->ddb, "default-information origin changed");
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessDefaultRouteCommand);
	
	return;
}
  
/************************************************************************************

	Name:	EigrpIpActivateConnAddress

	Desc:	Handle a single address on an interface when a network command is added or deleted.

			We tinker with the redistributed connecteds, the topology table, and summaries.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given interface
			address		- the ip address in the "network" command
			mask		- ip mask
			sense		- if it is true ,the Eigrp network will be started up; if it is false the
						  the eigrp network will be turned down
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpActivateConnAddress_int(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf, U32 address, U32 mask,  S32 sense, S8 *file, U32 line)
{
	EigrpDualDdb_st *ddb;

	EIGRP_FUNC_ENTER(EigrpIpActivateConnAddress);

	_EIGRP_DEBUG("EigrpIpActivateConnAddress(%s, %s, %s, %d)\n",
					pEigrpIntf->name,
					EigrpUtilIp2Str(address),
					EigrpUtilIp2Str(mask),
					sense);

	ddb = pdb->ddb;
	address &= mask; 

	EigrpIpRedistConnState(pdb, pEigrpIntf, address, mask, sense);
	EigrpIpEnqueueConnRtchangeEvent(pdb, pEigrpIntf, address, mask, sense, TRUE);

	EIGRP_FUNC_LEAVE(EigrpIpActivateConnAddress);
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessNetworkCommand

	Desc:	Process "network" command for an EIGRP routing process. This function will obtain the
			idbs associated with the network number configured.	Each idb will be stored in an
			EigrpIdb_st data structure that resides	in the ddb.
 
			The network is a,b,c class network.
	
			We will always be called for every network, even when a "no router eigrp" command is
			entered(each network is turned off first).  When we are called, the change has already 
			been made to the pdb(the network has been added or deleted).

			sense = TRUE : network
			sense = FALSE : no network
		
	Para:	pdb		- pointer to the EIGRP process descriptor block
			noFlag	- the flag of negative command
			net		- the network address configured
			mask	- the network mask
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessNetworkCommand(S32 noFlag, EigrpPdb_st *pdb, U32 net, U32 mask)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st	*iidb;
	EigrpIntf_pt	pEigrpIntf;
	EigrpIntfAddr_pt	pAddr;
	U32	address, mask2, act_count;
	S32	passive_intf, sense;
	void *pNode;

	EIGRP_FUNC_ENTER(EigrpIpProcessNetworkCommand);
#ifdef _EIGRP_PLAT_MODULE
	//检测这个地址是否在接口链表里面，如果这个接口不在
	//接口链表里面，就不让这个接口启动FRP路由
	if(EigrpIntfFindByNetAndMask(net,  mask) == NULL)
	{
		EIGRP_FUNC_LEAVE(EigrpIpProcessNetworkCommand);
		return ;
	}
#endif/* _EIGRP_PLAT_MODULE */
	act_count		= 0;
	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}
	/* get work request parameter */
	net		= net & mask;                      /* just in case added by  2002.09.26 */

	if(sense == TRUE){
		if(!EigrpIpNetworkEntryInsert(&pdb->netLst, net, mask)){
			EIGRP_FUNC_LEAVE(EigrpIpProcessNetworkCommand);
			_EIGRP_DEBUG("EigrpIpNetworkEntryInsert:FALSE\n");
			return;
		}
		_EIGRP_DEBUG("EigrpIpNetworkEntryInsert:TRUE %x %x\n",net, mask);
	}else{
		if(!EigrpIpNetworkEntryDelete(&pdb->netLst, net, mask)){
			EIGRP_FUNC_LEAVE(EigrpIpProcessNetworkCommand);
			return;
		}
	}
	ddb = pdb->ddb;
#ifdef _EIGRP_PLAT_MODULE		
	EigrpIpSetRouterId(ddb, net, mask );	/* Just in case */
#else
	EigrpIpSetRouterId(ddb);	/* Just in case */
#endif

	/*for each interface */
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		
#if 0/*(EIGRP_OS_TYPE == EIGRP_OS_LINUX)*/
		if(((pEigrpIntf->ipAddr & pEigrpIntf->ipMask) == 0x0a124E00) || 
				((pEigrpIntf->ipAddr & pEigrpIntf->ipMask) == 0x004E120a)){
			pEigrpIntf->bandwidth	= 512;
		}
#endif
		if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
			continue;
		}	
		act_count = EigrpIpGetActiveAddrCount(pdb, pEigrpIntf);
		
		_EIGRP_DEBUG("EigrpIpGetActiveAddrCount:%d \n",act_count);
		
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK)){
			act_count = 0;				/* relocated by  2002.11.29 */
		}
		for(pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				if(pAddr->ipAddr == EigrpPortGetRouterId()){/*无编号模式*/
					address = pAddr->ipAddr;
				}else{
					/*tigerwh*/
					/*address	= pAddr->ipDstAddr;*/
					address	= pAddr->ipAddr;
				}
			}else{
				address	= pAddr->ipAddr;
			}
			/* if interface is pointtopoint, the OS should set the mask to 0xffffffff */
			mask2	= pAddr->ipMask;
			/* if local interface's ipaddress match network <A.B.C.D/M> */
			if((address & mask) == (net & mask)){
				/*add_zhenxl_20120524 可在TOPO表中显示P2P接口的网段*/
/*				if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
					address = pAddr->ipDstAddr;
				}*/
				_EIGRP_DEBUG("EigrpIpProcessNetworkCommand 1\n");
				EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask2, sense);
				/* break; */
			}
		}
		passive_intf	= EigrpIsIntfPassive(pdb, pEigrpIntf); /* is this interface passive for current ddb? */
		iidb			= EigrpFindIidb(ddb, pEigrpIntf);

		if(sense && act_count){
			EigrpIpOnoffIdb(ddb, pEigrpIntf, TRUE, passive_intf);
			/* We need enable the configuration on this interface */
			EigrpConfigIntfApply(pEigrpIntf->name);
		}
		/* Update any autosummaries for this interface. */
		EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);
		/* If we're needing to delete an IIDB, do it now. */
		if(!sense && iidb && !act_count){
			EigrpIpOnoffIdb(ddb, pEigrpIntf, FALSE, passive_intf);
			/* EigrpIpBuildAutoSummaries is invoked in ifdown event */
		}
	}
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
			continue;
		}

		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK)){
			continue;        
		}

		address = pEigrpIntf->ipAddr;

		if(address == (U32)0){
			continue;
		}

		if((address & mask) != (net & mask)){
			EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);
		}
	}

	pNode	= EigrpPortCoreRtNodeFindExact(net, mask);//查找目的地址和掩码一样的路由链表
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD || EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	if(pNode && (EigrpPortCoreRtNodeHasStatic(pNode) == TRUE)){//确认链表是否有静态路由
		EigrpPortRedisProcNetworkAddStatic(pNode, sense);
	}
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	if(pNode){
		_EIGRP_DEBUG("EigrpIpProcessNetworkCommand 2\n");
		EigrpPortRedisProcNetworkAddStatic(pNode, sense);
	}
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	
	EIGRP_FUNC_LEAVE(EigrpIpProcessNetworkCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessNeighborCommand

	Desc:	This function is to process neighbor add/delete command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			noFlag	- the flag of negative command
			ipAddr	- neighbor ip address
	
	Ret:		
************************************************************************************/

void	EigrpIpProcessNeighborCommand(S32 noFlag, EigrpPdb_st *pdb, U32 ipAddr)
{
	EigrpNeiEntry_st	*neighbor, **pneighbor;
	S32	sense;

	EIGRP_FUNC_ENTER(EigrpIpProcessNeighborCommand);
	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}
	
	/* See if we're in the right state.  If so, bail. */
	if(sense){
		neighbor = pdb->nbrLst;
		while(neighbor){
			if(neighbor->address == ipAddr){
				break;
			}
			neighbor = neighbor->next;
		}
		if(!neighbor){
			neighbor = EigrpPortMemMalloc(sizeof(EigrpNeiEntry_st));
			if(!neighbor){	 
				EIGRP_FUNC_LEAVE(EigrpIpProcessNeighborCommand);
				return;
			}
			EigrpUtilMemZero((void *) neighbor, sizeof(EigrpNeiEntry_st));	 
			neighbor->address		= ipAddr;
			neighbor->next		= pdb->nbrLst;
			pdb->nbrLst	= neighbor;
		}
	}else{
		pneighbor = &pdb->nbrLst;
		while(*pneighbor){   
			if((*pneighbor)->address == ipAddr){
				neighbor		= *pneighbor;
				*pneighbor	= (*pneighbor)->next;
				EigrpPortMemFree(neighbor);
				break;
			}
			pneighbor = &(*pneighbor)->next;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessNeighborCommand);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueZapIfEvent

	Desc:	This function is to enqueue an interface zap event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given interface
	
	Ret:		TRUE	for success
			FALSE	for fail
************************************************************************************/

S32	EigrpIpEnqueueZapIfEvent(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf)
{
	EigrpWork_st *work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueZapIfEvent);
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueZapIfEvent);
		return FALSE;
	}
	work->c.ifc.ifp = pEigrpIntf;

	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_ZAP_IF_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueZapIfEvent);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpProcessZapIfEvent

	Desc:	This function is to really process an interface zap event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessZapIfEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpIntf_pt pEigrpIntf, pTem;

	EIGRP_FUNC_ENTER(EigrpIpProcessZapIfEvent);
	pEigrpIntf = work->c.ifc.ifp;
	if(!pEigrpIntf)	{
		EIGRP_FUNC_LEAVE(EigrpIpProcessZapIfEvent);
		return;
	}

	pTem	= EigrpIntfFindByName(pEigrpIntf->name);
	if(pTem == pEigrpIntf){
		EigrpIntfDel(pTem);
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessZapIfEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpIpAddressChange

	Desc:	This function is to be called when an IP address is added or deleted from an interface.

			The IDB will always reflect the state of the change; if added, the address will already
			be present, and vice versa.  The one exception to this is with unnumbered interfaces;
			the callback for deletion is done before the unnumbered pointer is NULLed out.
		
	Para: 	pEigrpInf		- pointer to the interface
			ipAddr		- the IP address added or deleted
			ipMask		- the ip adrdress mask
			adding		- the sign of adding or deleting
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpIpAddressChange_int(EigrpIntf_pt pEigrpIntf, U32 ipAddr, U32 ipMask, S32 adding, S8 *file, U32 line)
{
	EigrpDualDdb_st	*ddb;
	S32		passive_intf, retVal, act_count = 0/*zhenxl_20121126*/;
	U32		address, mask;
	EigrpPdb_st	*pdb;
	EigrpIntfAddr_pt	pAddr;
	EigrpIntf_pt	pIntf;
	
	EIGRP_FUNC_ENTER(EigrpIpIpAddressChange);

	_EIGRP_DEBUG("EigrpIpIpAddressChange(%s, %s, %s, %d) \n",
					pEigrpIntf->name,
					EigrpUtilIp2Str(ipAddr),
					EigrpUtilIp2Str(ipMask),
					adding);

	/* Inform all EIGRP speakers that are configured on this interface there is an address change. */
	if((EigrpPdb_st *)gpEigrp->protoQue->head){
		pdb = (EigrpPdb_st *)gpEigrp->protoQue->head;

		if(ipAddr){
			if(!EigrpIpConnAddressActivated(pdb, ipAddr, ipMask)){
				EIGRP_FUNC_LEAVE(EigrpIpIpAddressChange);
				_EIGRP_DEBUG("EigrpIpIpAddressChange 1\n");
				return;
			}
			address	= ipAddr;
			mask	= ipMask;
		}
		if(adding){
			if((act_count = EigrpIpGetActiveAddrCount(pdb, pEigrpIntf)) == 0){
				EIGRP_FUNC_LEAVE(EigrpIpIpAddressChange);
				_EIGRP_DEBUG("EigrpIpIpAddressChange 2\n");
				return;
			}
		}else{
			for(pIntf = gpEigrp->intfLst; pIntf; pIntf = pIntf->next){
				if(pIntf->sysCirc != pEigrpIntf->sysCirc){
					continue;
				}
				retVal	= EigrpIpConnAddressActivated(pdb, pIntf->ipAddr, pIntf->ipMask);
				if(retVal){
					act_count ++;
				}
			}
		}
		

		ddb = pdb->ddb;
		if(!ddb){
			EIGRP_FUNC_LEAVE(EigrpIpIpAddressChange);
			return;
		}
#ifdef _EIGRP_PLAT_MODULE		
		//接口IP地址变动不影响路由ID
		//EigrpIpSetRouterId(ddb, ipAddr, ipMask );	/* Just in case */
#else
		//EigrpIpSetRouterId(ddb);	/* Just in case */
#endif
		passive_intf = EigrpIsIntfPassive(pdb, pEigrpIntf);

		/* 根据删除或者新增作不同处理 */
		if(adding){
			if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK)){
				EigrpIpOnoffIdb(ddb, pEigrpIntf, TRUE, passive_intf);
				EigrpConfigIntfApply(pEigrpIntf->name);
			}
			if(ipAddr){ /* added address */
				EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, TRUE);
			}else{ /*if up*/
				for(pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
					if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
						address	= pAddr->ipDstAddr;
					}else{
						address	= pAddr->ipAddr;
					}
					mask	= pAddr->ipMask;
					if(EigrpIpConnAddressActivated(pdb, address, mask)){ 
						EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, TRUE);
					}
				}
			}
			EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);

		}else{
			if(ipAddr){
				EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, FALSE);
				act_count --;
				EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);
			}else{ /*if down*/
				if(pEigrpIntf->sysCirc){
					for(pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
						if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
							address	= pAddr->ipDstAddr;
						}else{
							address	= pAddr->ipAddr;
						}
						mask	= pAddr->ipMask;
						if(EigrpIpConnAddressActivated(pdb, address, mask)){ 
							EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, FALSE);
						}
					}
				}else{/**/
					if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
						address	= pEigrpIntf->remoteIpAddr;
					}else{
						address	= pEigrpIntf->ipAddr;
					}
					mask	= pEigrpIntf->ipMask;
					if(EigrpIpConnAddressActivated(pdb, address, mask)){ 
						EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, FALSE);
					}
				}
				act_count = 0;
			}
			/* They're all gone now. */
			if(act_count <= 0 && !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK)){	
				EigrpIpOnoffIdb(ddb, pEigrpIntf, FALSE, passive_intf);
			}
		}
		/* Clear all neighbors on the interface. */
		EigrpTakeNbrsDown(ddb, pEigrpIntf, FALSE, "address changed");
	}
	EIGRP_FUNC_LEAVE(EigrpIpIpAddressChange);
#ifdef _EIGRP_PLAT_MODULE
	pEigrpIntf->ipAddr = ipAddr;
	pEigrpIntf->ipMask = ipMask;
//	pEigrpIntf->remoteIpAddr = 0;
#endif /* _EIGRP_PLAT_MODULE  */
	return;
}
/************************************************************************************

	Name:	EigrpIpEnqueueIfAddEvent

	Desc:	This function is to enqueue eigrp interface add event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the eigrp interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfAddEvent(EigrpPdb_st *pdb, EigrpIntf_st *pEigrpIntf)
{
	EigrpWork_st *work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfAddEvent);
	if(!pdb->ddb) {      
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddEvent);
		return;
	}

	/* Queue redistribution request and process later. */
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work) { 
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddEvent);
		return;
	}

	work->c.ifc.ifp = pEigrpIntf;

	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IFC_ADD_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueIfDeleteEvent

	Desc:	This function is to enqueue eigrp interface delete event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			index	- interface index of the event 
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfDeleteEvent(EigrpPdb_st *pdb, S32 index)
{
	EigrpWork_st *work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfDeleteEvent);
	if(!pdb->ddb){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfDeleteEvent);
		return;
	}

	/* Queue redistribution request and process later. */
	work	= (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfDeleteEvent);
		return;
	}

	work->c.ifc.index	= index;

	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IFC_DELETE_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfDeleteEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueIfUpEvent

	Desc:	This function is to enqueue eigrp interface going-up event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
 			ifpnew	- pointer to the interface parameter of the event 
 	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfUpEvent(EigrpPdb_st *pdb, void *ifpnew)
{
	EigrpWork_st *work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfUpEvent);
	if(!pdb->ddb){ 
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfUpEvent);
		return;
	}

	/* Queue redistribution request and process later. */
	work = (EigrpWork_st *)EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){           
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfUpEvent);
		return;
	}

	work->c.ifc.ifp	= ifpnew;
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IFC_UP_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfUpEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueIfDownEvent

	Desc:	This function is to enqueue eigrp interface going-down event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			index	- interface index of the event 
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfDownEvent(EigrpPdb_st *pdb, S32 index)
{
	EigrpWork_st	*work;

	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfDownEvent);
	if(!pdb->ddb){ 
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfDownEvent);
		return;
	}

	/* Queue redistribution request and process later. */
	work	= (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){ 
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfDownEvent);
		return;
	}

	work->c.ifc.index	= index;
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IFC_DOWN_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfDownEvent);
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueIfAddressAddEvent

	Desc:	This function is to enqueue eigrp interface ip-address-add event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			index	- interface index of the event 
			ipAddr	- interface ip address of the event
			ipMask	- interface ip mask of the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfAddressAddEvent(EigrpPdb_st *pdb, S32 index, U32 ipAddr, U32 ipMask, U8 resetIdb)
{
	EigrpWork_st	*work;
	
	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfAddressAddEvent);
	if(!pdb->ddb){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddressAddEvent);
		return;
	}

	/* Queue redistribution request and process later. */
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){ 
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddressAddEvent);
		return;
	}

	work->c.ifc.ipAddr		= ipAddr;
	work->c.ifc.ipMask		= ipMask;
	work->c.ifc.index		= index;
	work->c.ifc.sense		= resetIdb;
	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IFC_ADDRESS_ADD_EVENT);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddressAddEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueIfAddressDeleteEvent

	Desc:	This function is to enqueue eigrp interface ip-address-delete event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			index	- interface index of the event 
			ipAddr	- interface ip address of the event
			ipMask	- interface ip mask of the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpEnqueueIfAddressDeleteEvent(EigrpPdb_st *pdb, S32 index, U32 ipAddr, U32 ipMask)
{
	EigrpWork_st *work;

	_EIGRP_DEBUG("dbg>>EigrpIpEnqueueIfAddressDeleteEvent(%d, %s, %s)\n", 
									index,
								EigrpUtilIp2Str(ipAddr),
								EigrpUtilIp2Str(ipMask));

	EIGRP_FUNC_ENTER(EigrpIpEnqueueIfAddressDeleteEvent);

	if(!pdb->ddb){            
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddressDeleteEvent);
		return;
	}

	/* Queue redistribution request and process later.  */
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work) {           
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddressDeleteEvent);
		return;
	}

	work->c.ifc.ipAddr		= ipAddr;
	work->c.ifc.ipMask		= ipMask;
	work->c.ifc.index		= index;

	EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_IFC_ADDRESS_DELETE_EVENT);
	
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueIfAddressDeleteEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfAddEvent

	Desc:	This function is to process eigrp interface-adding event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp interface-adding event
		
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfAddEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpIntf_st *pEigrpIntf, *pEigrpIntfOld;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfAddEvent);

	pEigrpIntf = work->c.ifc.ifp;
	if(pEigrpIntf == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddEvent);
		return;
	}

	pEigrpIntfOld = EigrpIntfFindByIndex(pEigrpIntf->ifindex);
#ifndef _EIGRP_PLAT_MODULE	
	if(pEigrpIntfOld && (pEigrpIntfOld != pEigrpIntf)){
		EigrpIpIpAddressChange(pEigrpIntfOld, 0, 0, FALSE);
		EigrpIpEnqueueZapIfEvent(pdb, pEigrpIntfOld);
	}
#else /* _EIGRP_PLAT_MODULE */
	if(pEigrpIntfOld){//接口已经存在，不允许添加接口
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddEvent);
		return;
	}
#endif/* _EIGRP_PLAT_MODULE */
	EigrpIntfAdd(pEigrpIntf);
	if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
#ifndef _EIGRP_PLAT_MODULE
		EigrpIpIpAddressChange(pEigrpIntf, 0, 0, TRUE);
#endif/* _EIGRP_PLAT_MODULE */
	}

	EigrpConfigIntfApply(pEigrpIntf->name);
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfDeleteEvent

	Desc:	This function is to process eigrp interface-deleting event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp interface-deleting event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfDeleteEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpIntf_st	*pEigrpIntf;
	S32	index;    
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfDeleteEvent);
	index = work->c.ifc.index;

	pEigrpIntf = EigrpIntfFindByIndex(index);
	if(pEigrpIntf == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfDeleteEvent);
		return;
	}
#ifndef _EIGRP_PLAT_MODULE
	EigrpIpIpAddressChange(pEigrpIntf, 0, 0, FALSE);
#else/* _EIGRP_PLAT_MODULE */
	//删除接口IP地址
	EigrpIpIpAddressChange(pEigrpIntf, pEigrpIntf->ipAddr, pEigrpIntf->ipMask, FALSE);
#endif/* _EIGRP_PLAT_MODULE */
	EigrpPortUnSetUnnumberedIf(pEigrpIntf->sysCirc);	/* tigerwh 120524 */

	EigrpIpEnqueueZapIfEvent(pdb, pEigrpIntf);
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfDeleteEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfUpEvent

	Desc:	This function is to process eigrp interface going-up event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp interface going-up event
	
	Ret:		NONE
************************************************************************************/

 void	EigrpIpProcessIfUpEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpIntf_pt	pEigrpIntf, pEigrpIntfTem;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfUpEvent);

	pEigrpIntfTem = work->c.ifc.ifp;

	pEigrpIntf = EigrpIntfFindByIndex(pEigrpIntfTem->ifindex);
	if(pEigrpIntf == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfUpEvent);
		return;
	}

	EigrpPortStrCpy(pEigrpIntf->name, pEigrpIntfTem->name);
	pEigrpIntf->metric		= pEigrpIntfTem->metric;
	pEigrpIntf->mtu		= pEigrpIntfTem->mtu;
	pEigrpIntf->bandwidth	= pEigrpIntfTem->bandwidth;
	pEigrpIntf->delay		= pEigrpIntfTem->delay;
	pEigrpIntf->flags		= pEigrpIntfTem->flags;/*zhenxl_20120906 用于接口的有/无编号模式切换*/

	EigrpPortMemFree(pEigrpIntfTem);
	BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
#ifndef _EIGRP_PLAT_MODULE
	EigrpIpIpAddressChange(pEigrpIntf, 0, 0, TRUE);
#else/* _EIGRP_PLAT_MODULE */
	//EigrpIpIpAddressChange(pEigrpIntf, pEigrpIntf->ipAddr, pEigrpIntf->ipMask, TRUE);
#endif/* _EIGRP_PLAT_MODULE */
	EigrpConfigIntfApply(pEigrpIntf->name);
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfUpEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfDownEvent

	Desc:	This function is to process eigrp interface going-down event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp interface going-down event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfDownEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpIntf_pt	pEigrpIntf;
	S32	index;    
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfDownEvent);
	index = work->c.ifc.index;

	pEigrpIntf = EigrpIntfFindByIndex(index);
	if(pEigrpIntf == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfDownEvent);
		return;
	}

	BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
#ifndef _EIGRP_PLAT_MODULE
	EigrpIpIpAddressChange(pEigrpIntf, 0, 0, FALSE);
#else/* _EIGRP_PLAT_MODULE */
	//EigrpIpIpAddressChange(pEigrpIntf, pEigrpIntf->ipAddr, pEigrpIntf->ipMask, TRUE);
#endif/* _EIGRP_PLAT_MODULE */
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfDownEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfAddressAddEvent

	Desc:	This function is to process eigrp interface ip-address-adding event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp interface ip-address-adding event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfAddressAddEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpIntf_pt	pEigrpIntf;
	S32	index;
	U32	ipAddr, ipMask;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfAddressAddEvent);

	_EIGRP_DEBUG("EigrpIpProcessIfAddressAddEvent(%d, %s, %s, %d) ENTER\n",
					work->c.ifc.index,
					EigrpUtilIp2Str(work->c.ifc.ipAddr),
					EigrpUtilIp2Str(work->c.ifc.ipMask),
					work->c.ifc.sense);

	ipAddr	= work->c.ifc.ipAddr;
	ipMask	= work->c.ifc.ipMask;
	index	= work->c.ifc.index;

	pEigrpIntf = EigrpIntfFindByIndex(index);
	if(!pEigrpIntf){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddressAddEvent);
		return;
	}
	

	if(!EIGRP_IP_INVALID_ADDR(ipAddr) && BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)
		&& work->c.ifc.sense){
#ifdef _EIGRP_PLAT_MODULE
		if( (ipAddr != 0)&&(ipMask != 0) )
#endif/* _EIGRP_PLAT_MODULE */
		EigrpIpIpAddressChange(pEigrpIntf, ipAddr, ipMask, TRUE);
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddressAddEvent);
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfAddressDeleteEvent

	Desc:	This function is to process eigrp interface ip-address-deleting event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the eigrp interface ip-address-deleting event
	
	Ret:		
************************************************************************************/

void	EigrpIpProcessIfAddressDeleteEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	 EigrpIntf_pt	 pEigrpIntf;
	S32	index;    

	{
		_EIGRP_DEBUG("dbg>>EigrpIpProcessIfAddressDeleteEvent(%d, %s, %s)\n", 
								work->c.ifc.index,
								EigrpUtilIp2Str(work->c.ifc.ipAddr),
								EigrpUtilIp2Str(work->c.ifc.ipMask));
	}

	EIGRP_FUNC_ENTER(EigrpIpProcessIfAddressDeleteEvent);
	index	= work->c.ifc.index;

	pEigrpIntf = EigrpIntfFindByIndex(index);
	if(!pEigrpIntf){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddressDeleteEvent);
		return;
	}
#ifdef _EIGRP_PLAT_MODULE
	//传入参数和接口参数不匹配，不能删除
	if( (pEigrpIntf->ipAddr != work->c.ifc.ipAddr)||(pEigrpIntf->ipMask != work->c.ifc.ipMask) )
		return;
#endif/* _EIGRP_PLAT_MODULE */
	if((!EIGRP_IP_INVALID_ADDR(work->c.ifc.ipAddr)) && BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){

		EigrpIpIpAddressChange(pEigrpIntf, work->c.ifc.ipAddr, work->c.ifc.ipMask, FALSE);
	}
	_EIGRP_DEBUG("dbg>>EigrpIpProcessIfAddressDeleteEvent pEigrpIntf->ipAddr=%s\n", EigrpUtilIp2Str(pEigrpIntf->ipAddr));

	/*zhenxl_20120902 删除EIGRP接口表中的地址.*/
	if(work->c.ifc.ipAddr == pEigrpIntf->ipAddr){
		pEigrpIntf->ipAddr = 0;
		pEigrpIntf->ipMask = 0;
		pEigrpIntf->remoteIpAddr = 0;
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfAddressDeleteEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessReloadIptableEvent

	Desc:	This function is to re-populate the ip routing table with eigrp routes from the topology
			table.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessReloadIptableEvent(EigrpPdb_st *pdb)
{
	EigrpDualDdb_st *ddb;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessReloadIptableEvent);
	ddb = pdb->ddb;
	if(ddb == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpProcessReloadIptableEvent);
		return;
	}

	EigrpDualReloadProtoTable(ddb);
	EIGRP_FUNC_LEAVE(EigrpIpProcessReloadIptableEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpEnqueueReloadIptableEvent

	Desc:	This function is to enqueue reload iptable event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			idb		- pointer to the interface
	
	Ret:		
************************************************************************************/

void	EigrpIpEnqueueReloadIptableEvent(EigrpPdb_st *pdb, void *idb)
{
	EigrpWork_st *work;
	
	EIGRP_FUNC_ENTER(EigrpIpEnqueueReloadIptableEvent);
	if(!pdb->ddb){
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueReloadIptableEvent);
		return;
	}

	if(idb){
	/* This is just an interface down event.  We will get redist callbacks, or backup route 
	  * callbacks, or lost route callbacks for such an event.  No need to attempt to reload the 
	  * entire topology table. */ 
		EIGRP_FUNC_LEAVE(EigrpIpEnqueueReloadIptableEvent);
		return;
	}
	/* Queue table reload request and process later. */
	work = (EigrpWork_st *) EigrpUtilChunkMalloc(pdb->ddb->workQueChunk);
	if(!work){
		return;
	}
	(void)EigrpIpEnqueueEvent(pdb, work, EIGRP_WORK_TABLERELOAD);
	EIGRP_FUNC_LEAVE(EigrpIpEnqueueReloadIptableEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessExteriorChange

	Desc:	If a route's interior/exterior disposition has changed, this function will attempt to 
			mirror that change in the topology table.  Also, notify our neighbors of this event.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessExteriorChange(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualDdb_st	*ddb;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	EigrpNetEntry_st	dest;
	EigrpExtData_st	*extdata;
	U32		addr, mask;
	S32		iptable_entry_exterior, rdb_is_exterior, changed;
	S8		*flag_ptr;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessExteriorChange);
	addr			= work->c.d.dest.address;
	mask		= work->c.d.dest.mask;
	ddb			= pdb->ddb;
	dest.address	= addr;
	dest.mask	= mask;
	dndb		= EigrpDualNdbLookup(ddb, &dest);

	if(dndb == NULL) {           
		EIGRP_FUNC_LEAVE(EigrpIpProcessExteriorChange);
		return;
	}

	iptable_entry_exterior = EigrpIpIsExterior(addr, mask);
	/* IP routing table has indicated a change in exterior/interior state for this route. Update 
	  * the drdbs of our feasible successors to reflect the change. */
	extdata	= NULL;
	changed	= FALSE;
	for(drdb = dndb->rdb; drdb; drdb = drdb->next){
		if(drdb->succMetric >= dndb->oldMetric){
			continue;
		}
		if(drdb->flagExt){
			extdata = drdb->extData;
			if(extdata == NULL){
				continue;
			}
			flag_ptr = &extdata->flag;
		}else{
			flag_ptr = &drdb->opaqueFlag;
		}
		rdb_is_exterior = (*flag_ptr & EIGRP_INFO_CD);
		if(iptable_entry_exterior == rdb_is_exterior){
			continue;
		}
		if(iptable_entry_exterior){
			*flag_ptr |= EIGRP_INFO_CD;
		}else{
			*flag_ptr &= ~EIGRP_INFO_CD;
		}
		changed = TRUE;
	}
	if(changed){
		EigrpDualSendUpdate(ddb, dndb, "Change in CD bit");
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessExteriorChange);

	return;
}

/************************************************************************************

	Name:	EigrpIpGetProtId

	Desc:	This function is to map an route type into an IP-EIGRP external protocol id.
		
	Para: 	proto	- IP-EIGRP internal protocol id
	
	Ret:		IP-EIGRP external protocol id
************************************************************************************/

S32	EigrpIpGetProtId(U32 proto)
{
	enum IPEIGRP_PROT_ID prot_id = 0;
	
	EIGRP_FUNC_ENTER(EigrpIpGetProtId);
	switch(proto){
		case EIGRP_ROUTE_CONNECT :
			prot_id = EIGRP_PROTO_CONNECTED;
			break;
			
		case EIGRP_ROUTE_STATIC:
			prot_id = EIGRP_PROTO_STATIC;
			break;
			
		case EIGRP_ROUTE_BGP:
			prot_id = EIGRP_PROTO_BGP;
			break;
			
		case EIGRP_ROUTE_RIP:
			prot_id = EIGRP_PROTO_RIP;
			break;
			
		case EIGRP_ROUTE_IGRP:
			prot_id = EIGRP_PROTO_IGRP;
			break;
			
		case EIGRP_ROUTE_OSPF:
			prot_id = EIGRP_PROTO_OSPF;
			break;
			
		case EIGRP_ROUTE_EIGRP:
			prot_id = EIGRP_PROTO_EIGRP;
			break;
			
		default:
			prot_id = EIGRP_PROTO_NULL;
			break;
	}
	EIGRP_FUNC_LEAVE(EigrpIpGetProtId);

	return(prot_id);
}

/************************************************************************************

	Name:	EigrpIpBuildExternalInfo

	Desc:	This function is to build external data for external destinations. Get information from
			either the routing table entry or policy configuration.
 
			Returns a pointer to the external info block, or NULL if we're out of
			memory.

		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ddb		- pointer to the dual descriptor block 
			dist		- pointer to external EIGRP process
			rt		- pointer to external route
	
	Ret:		pointer to the external data ,or NULL for fail
************************************************************************************/

EigrpExtData_st *EigrpIpBuildExternalInfo(EigrpPdb_st *pdb, EigrpDualDdb_st *ddb,     EigrpPdb_st *dist, EigrpDualNewRt_st *rt)
{
	EigrpExtData_st	*ext_info;
	U32		tag;
	
	EIGRP_FUNC_ENTER(EigrpIpBuildExternalInfo);
	ext_info = (EigrpExtData_st *) EigrpUtilChunkMalloc(ddb->extDataChunk);
	if(ext_info){
		/* Check and see if route-map has a "set tag" value. */
		tag = 0; 

		ext_info->routerId = HTONL(ddb->routerId);
		if(dist){
			ext_info->asystem	= HTONL(dist->process);
			ext_info->tag		= tag;
		}else{
			ext_info->asystem	= HTONL(pdb->process);
			ext_info->tag		= 0;
		}
		ext_info->metric	= HTONL(rt->metric);
		ext_info->protocol	= EigrpIpGetProtId(rt->rtType);

		/* Set candidate default bit if routing table says so.  This is defined as a series of
		  * flags, but since only one bit(CD) is currently supported, we simply equate it. */
		if(rt->flagExt){
			ext_info->flag = EIGRP_INFO_CD;
		}else{
			ext_info->flag = EIGRP_INFO_EXT;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpBuildExternalInfo);

	return(ext_info);
}

/************************************************************************************

	Name:	EigrpIpPickupRedistMetric

	Desc:	This function is to choose what metric to use for this redistributed route.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			rt		- pointer to the redistributed route
			metric	- pointer to the composite metric
			vecmetric	- pointer to the vector metric
	
	Ret:		TRUE	for success
			FALSE	for fail
************************************************************************************/

S32	EigrpIpPickupRedistMetric(EigrpPdb_st *pdb, EigrpDualNewRt_st *rt, U32 *metric, EigrpVmetric_st *vecmetric)
{
	EigrpRedisEntry_st	*redis;
	EigrpVmetric_st	*vMetricDef, *rtmetric;
	EigrpIntf_pt	pEigrpIntf;
	EigrpDualDdb_st	*ddb;
	S32		ret;
	
	EIGRP_FUNC_ENTER(EigrpIpPickupRedistMetric);
	if(!rt){
		EIGRP_FUNC_LEAVE(EigrpIpPickupRedistMetric);
		_EIGRP_DEBUG("%s:!rt\n",__func__);
		return FALSE;
	}

	if(rt->rtType == EIGRP_ROUTE_EIGRP){
		redis = EigrpIpRedisEntrySearchWithProto(pdb->redisLst, rt->rtType, rt->asystem);
	}else{
		redis = EigrpIpRedisEntrySearchWithProto(pdb->redisLst, rt->rtType, 0);
	} 

	if(!redis && ((rt->rtType != EIGRP_ROUTE_SYSTEM) && (rt->rtType != EIGRP_ROUTE_CONNECT)) && EigrpIpBitsInMask(rt->dest.mask)!= 0){ 
		EIGRP_FUNC_LEAVE(EigrpIpPickupRedistMetric);
		_EIGRP_DEBUG("%s: rt->rtType = %d rt->dest.mask = %x \n",__func__,rt->rtType,rt->dest.mask);
		return FALSE;
	}

	EigrpUtilMemZero((void *) vecmetric, sizeof(EigrpVmetric_st));

	_EIGRP_DEBUG("EigrpIpPickupRedistMetric: rtType=%d\n", rt->rtType);

	switch(rt->rtType){
		case EIGRP_ROUTE_CONNECT :
		case EIGRP_ROUTE_SYSTEM :
			/* case RTPROTO_STATIC :*/
			/* This stuff is used for routes, and redistributed connected routes.  It supports the
			  * useof the metric stored in the next hop interface if a default metric is not 
			  * configured. */
			if(rt->idb){                           
				pEigrpIntf = rt->idb->idb;
			}else{
				pEigrpIntf = rt->intf;
			}

			if(rt->origin == EIGRP_ORG_RSTATIC || rt->origin == EIGRP_ORG_RCONNECT){
				pEigrpIntf	= (EigrpIntf_pt)EigrpPortGetGateIntf(rt->nextHop);
			}
			if(!pEigrpIntf){ 
				EIGRP_FUNC_LEAVE(EigrpIpPickupRedistMetric);
				_EIGRP_DEBUG("%s: can not find pEigrpIntf \n",__func__);
				return FALSE;
			}

			{
				_EIGRP_DEBUG("EigrpIpPickupRedistMetric:    interface infomation\n");
				_EIGRP_DEBUG("\t name=%s\n", pEigrpIntf->name);
				_EIGRP_DEBUG("\t metric=%ld\n", pEigrpIntf->metric);
				_EIGRP_DEBUG("\t mtu=%ld\n", pEigrpIntf->mtu);
				_EIGRP_DEBUG("\t bandwidth=%ld\n", pEigrpIntf->bandwidth);
				_EIGRP_DEBUG("\t delay=%ld\n", pEigrpIntf->delay);
				_EIGRP_DEBUG("\t ipAddr=%x\n", pEigrpIntf->ipAddr);
				_EIGRP_DEBUG("\t ipmask=%x\n", pEigrpIntf->ipMask);
			}	

			vecmetric->delay	= BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST) ? EIGRP_B_DELAY : EIGRP_S_DELAY;
			vecmetric->bandwidth	= EigrpDualIgrpMetricToEigrp(EIGRP_SCALED_BANDWIDTH(1000 * pEigrpIntf->bandwidth)); 
			vecmetric->reliability	= 255;
			vecmetric->load		= 1;
			vecmetric->mtu		= pEigrpIntf->mtu;
			vecmetric->hopcount	= 0;
			if(rt->origin == EIGRP_ORG_RSTATIC || rt->origin == EIGRP_ORG_RCONNECT){
				if(pEigrpIntf){
					EigrpPortMemFree(pEigrpIntf);
				}
			}
			break;

		case EIGRP_ROUTE_IGRP :
			/* This is a hack.  Redistribution is based on the index set in the ndb. However, we
			  * may still have a different process which wrote this ndb. We must determine if it
			  * is igrp1 or eigrp regardless of what the index in the ndb is set to in order to
			  * avoid confusing the metrics. */

			/* IGRP use structure igrp_metric_i store metric in rt_date, EIGRP use structure
			  * EigrpVmetric_st store metric in rt_date. */

			break;
		case EIGRP_ROUTE_EIGRP :
			{
				/* redistribute eigrp route */
				rtmetric = (EigrpVmetric_st *)(rt->data);
				if(!rtmetric){
					break;
				}
				vecmetric->delay	= rtmetric->delay;
				vecmetric->bandwidth	= rtmetric->bandwidth;
				vecmetric->reliability	= rtmetric->reliability;
				vecmetric->load		= rtmetric->load;
				vecmetric->mtu		= rtmetric->mtu;
				vecmetric->hopcount	= rtmetric->hopcount;
			}
			break;

		default :
			if(!redis ||!(vMetricDef = redis->vmetric)){
				vMetricDef = &pdb->vMetricDef;
			}
			/* 当def_vmetric已经是不可达时,不可以再乘256得到vecmetric(会导致溢出). 此时应该把
			  * vecmetric直接赋成不可达*/
			if((vMetricDef->delay == EIGRP_METRIC_INACCESS)
			    || (vMetricDef->bandwidth == EIGRP_METRIC_INACCESS)){
				vecmetric->delay		= EIGRP_METRIC_INACCESS;
				vecmetric->bandwidth	= EIGRP_METRIC_INACCESS;
			}else{
				vecmetric->delay		= EigrpDualIgrpMetricToEigrp(vMetricDef->delay);

				/* the "redis ... metric" and "default-metric" command use kbps, but
				  * EIGRP_SCALED_BANDWIDTH use bps . */
				/*old code:
				  * vecmetric->bandwidth = EigrpDualIgrpMetricToEigrp
				  * (EIGRP_SCALED_BANDWIDTH(vMetricDef->bandwidth));*/
				vecmetric->bandwidth = EigrpDualIgrpMetricToEigrp(EIGRP_SCALED_BANDWIDTH(1000 *vMetricDef->bandwidth));
			}
			vecmetric->reliability	= vMetricDef->reliability;
			vecmetric->load		= vMetricDef->load;
			vecmetric->mtu		= vMetricDef->mtu;
			vecmetric->hopcount	= 0;

			if(redis /*&& redis->rtMap*/){
				ret = EigrpPortRouteMapJudge(redis->rtMap, rt->dest.address, rt->dest.mask, (void *)rt);

				if(ret == TRUE){
					if(rt->metric){
						*metric = rt->metric;
						EIGRP_FUNC_LEAVE(EigrpIpPickupRedistMetric);
						return TRUE;
					}else{
						break;
					}
				}else if(ret == FALSE){
					_EIGRP_DEBUG("%s: can not find pEigrpIntf \n",__func__);
					EIGRP_FUNC_LEAVE(EigrpIpPickupRedistMetric);
					return FALSE;
				}
				/* RMAP_NOMATCH   fall down*/
			}

		/* If no route-map or route-map touch nothing */
	}

	{
		_EIGRP_DEBUG("EigrpIpPickupRedistMetric:\n");
		_EIGRP_DEBUG("\tbandwidth=%-10ld", vecmetric->bandwidth);
		_EIGRP_DEBUG("\tload=%-10ld", vecmetric->load);
		_EIGRP_DEBUG("\tdelay=%-10ld", vecmetric->delay);
		_EIGRP_DEBUG("\treliability=%-10ld\n", vecmetric->reliability);
	}

	ddb		= pdb->ddb;
	*metric	= EigrpDualComputeMetric(vecmetric->bandwidth,
									vecmetric->load,
									vecmetric->delay,
									vecmetric->reliability,
									ddb->k1, ddb->k2, ddb->k3, ddb->k4, ddb->k5 );
	EIGRP_FUNC_LEAVE(EigrpIpPickupRedistMetric);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpRedistributeRoutesAllCleanup

	Desc:	This function is to cleanup the redistribute routes.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpRedistributeRoutesAllCleanup(EigrpPdb_st *pdb)
{
	EigrpDualNdb_st	*dndb, *nNext;
	EigrpDualRdb_st	*drdb, *rNext;
	EigrpDualNewRt_st	newroute;
	S32		i;
	
	EIGRP_FUNC_ENTER(EigrpIpRedistributeRoutesAllCleanup);
	/* this function revised by  because the dndb, drdb may be freed, we retrieve next pointer
	  * beforehand */
	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = pdb->ddb->topo[ i ]; dndb; dndb = nNext){	
			nNext = dndb->next;
			for(drdb = dndb->rdb; drdb; drdb = rNext){
				rNext = drdb->next;
				if(drdb->origin == EIGRP_ORG_REDISTRIBUTED || drdb->origin == EIGRP_ORG_RSTATIC){
					/* DOWN this redistributed route */
					newroute.nextHop		= drdb->nextHop;
					newroute.dest		= drdb->dndb->dest;
					newroute.origin		= drdb->origin;
					newroute.flagExt		= drdb->flagExt;
					newroute.rtType		= drdb->rtType;
					newroute.idb		= drdb->iidb;
					newroute.infoSrc	= drdb->infoSrc;
					newroute.vecMetric	= drdb->vecMetric;

					EigrpIpRtchange(pdb, &newroute, EIGRP_ROUTE_DOWN);
				}
			}
		}
	}

	/* This func must be called after EigrpDualSendUpdate. */
	EigrpDualFinishUpdateSend(pdb->ddb);
	EIGRP_FUNC_LEAVE(EigrpIpRedistributeRoutesAllCleanup);

	return;
}

/************************************************************************************

	Name:	EigrpIpRtchange

	Desc:	This function is to inform DUAL about redistributed route. This inserts/deletes the
 			route from the topology table.
 
 			This function is also called when an EIGRP route gets overwritten by another process
 			because of administrative distance.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			newroute	- pointer to the redistributed route
			event	- the type of event ,EIGRP_ROUTE_UP or EIGRP_ROUTE_MODIF
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpRtchange_int(EigrpPdb_st *pdb, EigrpDualNewRt_st *newroute, S32 event, S8 *file, U32 line)
{
	EigrpDualDdb_st	*ddb;
	EigrpPdb_st		*src_pdb;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	EigrpNetEntry_st	dest;
	U32	the_dual_event_type;
	S32	reachable,  pdbonndb;
	EIGRP_FUNC_ENTER(EigrpIpRtchange);
	
	_EIGRP_DEBUG(" %s :event = %d\n",__func__,event);

	reachable	= (event == EIGRP_ROUTE_UP || event == EIGRP_ROUTE_MODIF);

	if(!newroute){            
		EIGRP_FUNC_LEAVE(EigrpIpRtchange);
		return;
	}
	src_pdb = NULL;
	if(newroute->asystem){
		src_pdb = (EigrpPdb_st *)EigrpPortMemMalloc(sizeof(EigrpPdb_st));
		if(!src_pdb){	 
			EIGRP_FUNC_LEAVE(EigrpIpRtchange);
			return;
		}
		src_pdb->process	= newroute->asystem;
	}
	
	/* Inform DUAL that route has changed. */
	ddb = pdb->ddb;

	pdbonndb 	= FALSE;
	dest.address	= newroute->dest.address;
	dest.mask	= newroute->dest.mask;
	if(EigrpDualNdbLookup(pdb->ddb, &dest)){
		_EIGRP_DEBUG(" %s :EigrpDualNdbLookup(pdb->ddb, &dest) TRUE\n",__func__);
		pdbonndb =  TRUE;
	}
	
	/* Set up metric for DUAL call. */
	if(reachable){
		_EIGRP_DEBUG(" %s :Set up metric for DUAL call\n",__func__);
		if(!newroute->metric){
			if(!EigrpIpPickupRedistMetric(pdb, newroute, &newroute->metric,  &newroute->vecMetric)){
				EigrpPortMemFree(src_pdb);	
				EIGRP_FUNC_LEAVE(EigrpIpRtchange);
				_EIGRP_DEBUG(" %s :EigrpIpPickupRedistMetric\n",__func__);
				return;
			}
		}
		the_dual_event_type = (event == EIGRP_ROUTE_UP) ? EIGRP_DUAL_RTUP : EIGRP_DUAL_RTCHANGE;
	}else{
		newroute->metric			= EIGRP_METRIC_INACCESS;
		newroute->vecMetric.delay	= EIGRP_METRIC_INACCESS;
		the_dual_event_type				= EIGRP_DUAL_RTDOWN;
	}
	newroute->flagIgnore = FALSE;

	dndb = EigrpDualNdbLookup(ddb, &newroute->dest);
	
	if(reachable && (newroute->origin == EIGRP_ORG_REDISTRIBUTED ||
		newroute->origin == EIGRP_ORG_RSTATIC)){
		_EIGRP_DEBUG(" %s :EigrpIpBuildExternalInfo\n",__func__);
			newroute->data =  EigrpIpBuildExternalInfo(pdb, ddb, src_pdb, newroute);
			newroute->originRouter = ddb->routerId;/*zhenxl_20130116*/
	}else if(pdbonndb && dndb){
		_EIGRP_DEBUG(" %s :else if(pdbonndb && dndb)\n",__func__);
		newroute->data =  EigrpIpBuildExternalInfo(pdb, ddb, src_pdb, newroute);
	}else if(newroute->origin == EIGRP_ORG_CONNECTED){

	}else{
	
		_EIGRP_DEBUG(" %s :reachable %d dndb %s\n",__func__,reachable,dndb? "FULL":"NULL");
		EigrpPortMemFree(src_pdb);	
		EIGRP_FUNC_LEAVE(EigrpIpRtchange);
		return;
	}

	/* If we are telling DUAL to delete a redistributed route, and it is a feasible succesor, then
	  * update our possible summary dependencies. */
	if(the_dual_event_type == EIGRP_DUAL_RTDOWN){

		drdb = EigrpDualRouteLookup(ddb, &newroute->dest, newroute->idb,  &newroute->nextHop);
		if(drdb && drdb->origin != EIGRP_ORG_SUMMARY){
			EigrpIpSummaryDepend(pdb, newroute->dest.address, newroute->dest.mask,
										NULL, drdb->metric, FALSE);
		}
	}

	/* In function EigrpDualRecvUpdate, we maybe call rt_open when we update rtTbl, however, in
	  * this case, we process a redistributed route, it has called rt_open before enter this
	  * function, so we should do that. */
	EigrpDualRtchange(ddb, newroute, the_dual_event_type);

	/* If DUAL installed a redistributed route as a feasible successor, then update our possible
	  * summary dependencies. */
	if(the_dual_event_type != EIGRP_DUAL_RTDOWN){
		drdb = EigrpDualRouteLookup(ddb, &newroute->dest, newroute->idb,
									&newroute->nextHop);
		if(drdb && drdb->origin != EIGRP_ORG_SUMMARY){ 
			EigrpIpSummaryDepend(pdb, newroute->dest.address,
										newroute->dest.mask,
										&newroute->vecMetric, drdb->metric,
										TRUE);
		}
	}
	EigrpPortMemFree(src_pdb);	
	EIGRP_FUNC_LEAVE(EigrpIpRtchange);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpRedistributeRoutesCleanup

	Desc:	This function is to clean up all eigrp redistributed routes.
		
	Para: 	asystem		- autonomous system number
			type			- the type of the redistributed route
	
	Ret:		0 	for success
			1	for fail
************************************************************************************/

S32	EigrpIpRedistributeRoutesCleanup(U16 asystem, S32 type, U32 srcProc)
{
	EigrpPdb_st		*pdb;
	EigrpDualNdb_st	*dndb, *nNext;
	EigrpDualRdb_st	*drdb, *rNext;
	EigrpDualNewRt_st	newroute;
	S32		i;
	/* this function revised by  because the dndb, drdb may be freed or not, we retrieve next 
	  * pointer beforehand */
	EIGRP_FUNC_ENTER(EigrpIpRedistributeRoutesCleanup);

	pdb = EigrpIpFindPdb(asystem);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpRedistributeRoutesCleanup);
		return 1;
	}

	/* Walk through all route in topology table */
	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		if(!pdb->ddb->topo[ i ]){
			continue;
		}
		for(dndb = pdb->ddb->topo[ i ]; dndb; dndb = nNext){	
			nNext = dndb->next;
			for(drdb = dndb->rdb; drdb; drdb = rNext){
				rNext = drdb->next;
				if((drdb->origin == EIGRP_ORG_REDISTRIBUTED || drdb->origin == EIGRP_ORG_RSTATIC) 
						&& (drdb->rtType == type)){
					EIGRP_ASSERT((U32)(drdb->dndb = dndb));
					EigrpPortMemSet((U8 *)&newroute, 0, sizeof(EigrpDualNewRt_st));
					newroute.dest		= dndb->dest; /* union = union */
					newroute.nextHop	= drdb->nextHop; /* union = union */
					newroute.flagExt	= drdb->flagExt;
					newroute.idb		= drdb->iidb;
					newroute.infoSrc	= drdb->infoSrc; /* union = union */
					newroute.metric	= drdb->metric;
					newroute.vecMetric	= drdb->vecMetric; /*struct = struct */
					newroute.rtType	= type;
					newroute.origin	= drdb->origin;
					newroute.asystem	= srcProc;

					EigrpIpRtchange(pdb, &newroute, EIGRP_ROUTE_DOWN);
				}
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpRedistributeRoutesCleanup);

	return 0;
}

/************************************************************************************

	Name:	EigrpIpProcessRedisEntryEvent

	Desc:	This function is to  process the redistribute command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessRedisEntryEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpRedisEntry_st	*redis;
	U16		redisindex;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessRedisEntryEvent);
	redisindex	= work->c.rea.ulindex;
	redis		= EigrpIpRedisEntrySearch(pdb->redisLst, redisindex);
	if(!redis){
		EIGRP_FUNC_LEAVE(EigrpIpProcessRedisEntryEvent);
		return;
	}

	/* For each network created by routing process dist, add to topology table. */
	if(work->c.rea.sense){
		EigrpPortRedisAddProto(pdb, redis->proto, redis->process);
		EigrpPortRedisTrigger(redis->proto);
	}else{
		/* deleting redistribute routes */
		EigrpPortRedisDelProto(pdb, redis->proto, redis->process);
		EigrpIpRedistributeRoutesCleanup(redis->rcvProc, redis->proto, redis->process);
		EigrpIpRedisEntryDelete(&pdb->redisLst, redisindex) ;        /* added by */
		EigrpPortRedisUnTrigger(redis->proto);
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessRedisEntryEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessConnAddressEvent

	Desc:	This function is to process each element on the redistribution queue. As each element
			is inserted in the topology table, it is then unqueued.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessConnAddressEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualNewRt_st	newroute;
	EigrpIntf_pt		pEigrpIntf;
	S32		event, retVal;
	U32		address, mask;
	void		*pRtNode;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessConnAddressEvent);

	_EIGRP_DEBUG("EigrpIpProcessConnAddressEvent ENTER\n");

	address	= work->c.red.dest.address;
	mask	= work->c.red.dest.mask;
	event	= work->c.red.event;

	pRtNode	= EigrpPortCoreRtNodeFindExact(address, mask);//根据目的地址和掩码查找路由链表节点
	if(!pRtNode){
		EIGRP_FUNC_LEAVE(EigrpIpProcessConnAddressEvent);
		return;
	}
	retVal	= EigrpPortCoreRtNodeHasConnect(pRtNode);//确认链表是否有直连路由
	if(retVal == FALSE){
		EIGRP_FUNC_LEAVE(EigrpIpProcessConnAddressEvent);
		return;
	}

	pEigrpIntf = EigrpIntfFindByIndex(EigrpPortCoreRtGetConnectIntfId(pRtNode));
	EIGRP_ASSERT((U32)pEigrpIntf);

	{
		_EIGRP_DEBUG("EigrpIpProcessConnAddressEvent:    interface infomation\n");
		_EIGRP_DEBUG("\t name=%s\n", pEigrpIntf->name);
		_EIGRP_DEBUG("\t metric=%ld\n", pEigrpIntf->metric);
		_EIGRP_DEBUG("\t mtu=%ld\n", pEigrpIntf->mtu);
		_EIGRP_DEBUG("\t bandwidth=%ld\n", pEigrpIntf->bandwidth);
		_EIGRP_DEBUG("\t delay=%ld\n", pEigrpIntf->delay);
		_EIGRP_DEBUG("\t ipAddr=%x\n", pEigrpIntf->ipAddr);
		_EIGRP_DEBUG("\t ipmask=%x\n", pEigrpIntf->ipMask);
	}	

	EigrpUtilMemZero((void *)&newroute, sizeof(newroute));
	newroute.dest.address	= work->c.red.dest.address;
	newroute.dest.mask	= work->c.red.dest.mask;
	newroute.rtType		= EIGRP_ROUTE_CONNECT;
	newroute.origin		= EIGRP_ORG_CONNECTED;
	newroute.flagExt		= FALSE;

	newroute.intf		= pEigrpIntf;
	newroute.idb		= EigrpFindIidb(pdb->ddb, pEigrpIntf);
	/* Route has been, or is in the process of being deleted. Process EIGRP_ROUTE_DOWN event with 
	  * use of temporary db on stack if needed. temp_ndb->pdbindex = 0 down below does not get used
	  * in EigrpIpRtchange(). Route may still exist, but it might be time to stop redistributing 
	  * this particular route.  For example, we are redistributing RIP, but RIP is no longer 
	  * configured over network X.  (Ie. someone typed 'no network X'). */

	EigrpIpRtchange(pdb, &newroute, event);
	EIGRP_FUNC_LEAVE(EigrpIpProcessConnAddressEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessBackup

	Desc:	This function is to process each element on the backup queue.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessBackup(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualDdb_st	*ddb;
	EigrpNetEntry_st	dest;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessBackup);
	ddb = pdb->ddb;

	dest.address	= work->c.d.dest.address;
	dest.mask	= work->c.d.dest.mask;

	/* Ask DUAL if route is still active and validate if available. */
	EigrpDualValidateRoute(ddb, &dest);
	EIGRP_FUNC_LEAVE(EigrpIpProcessBackup);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessLost

	Desc:	This function is to process each element on the lost route queue.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessLost(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualDdb_st	*ddb;
	EigrpNetEntry_st	dest;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessLost);
	/* Inform DUAL that route has changed. */
	ddb = pdb->ddb;

	dest.address	= work->c.d.dest.address;
	dest.mask	= work->c.d.dest.mask;
	EigrpDualLostRoute(ddb, &dest);
	EIGRP_FUNC_LEAVE(EigrpIpProcessLost);

	return;
}

/************************************************************************************

	Name:	EigrpIpGetSummaryMetric

	Desc:	This function is to examine all more specific routes in topology table for summary. Get
 			minimum metric.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			summary	- pointer to the summary route
			route	- pointer to the route which metric could be changed
	
	Ret:		TRUE 	for a more specific route was found.
			FALSE	for nothing is found
************************************************************************************/

S32	EigrpIpGetSummaryMetric(EigrpDualDdb_st *ddb, EigrpSum_st *summary, EigrpDualNewRt_st *route)
{
	EigrpDualNdb_st	*dndb, *save_dndb;
	S32 index;
	U32 ipaddr, metric;
	
	EIGRP_FUNC_ENTER(EigrpIpGetSummaryMetric);

	EigrpDualLogAll(ddb, EIGRP_DUALEV_GETSUM, &summary->address, &summary->mask);
	summary->beingRecal = FALSE;

	/* Find minimum metric by traversing all more specific routes for this summary address. */
	dndb		= NULL;
	save_dndb	= NULL;
	index		= 0;

	/* A min metric of infinity means there are no specific routes for the summary in the topology
	  * table. */
	summary->minMetric = EIGRP_METRIC_INACCESS;

	/* Scan topology table. */
	while((dndb = EigrpDualTableWalk(ddb, dndb, &index))){
		if(!dndb->succNum){
			continue;
		}

		/* Don't consider the metric of other summaries when choosing a metric for this summary. */
		if(dndb->rdb && (dndb->rdb->origin == EIGRP_ORG_SUMMARY)){
			continue;
		}
		/* Ignore the summary itself, regardless of who owns it. */
		if(dndb->dest.address == summary->address &&
			dndb->dest.mask == summary->mask){
			continue;
		}

		ipaddr = dndb->dest.address & summary->mask;
		/*revise to compare mask by   2002.09.24*/
		if((ipaddr == summary->address) && (summary->mask < dndb->dest.mask)){    
			if(dndb->metric == EIGRP_METRIC_INACCESS){
				/* dndb must be in the process of being deleted.  Ignore. */
				continue;
			}
			metric = EigrpDualComputeMetric(dndb->vecMetric.bandwidth,
			                  dndb->vecMetric.load,
			                  dndb->vecMetric.delay,
			                  dndb->vecMetric.reliability,
			                  ddb->k1, ddb->k2, ddb->k3, ddb->k4,
			                  ddb->k5 );
			if(metric < summary->minMetric){
				summary->minMetric	= metric;
				save_dndb	= dndb;
			}
			EIGRP_TRC(DEBUG_EIGRP_EVENT,"IP-EIGRP: get_summary_metric: %s/%d\n",
					        EigrpUtilIp2Str(summary->address), EigrpIpBitsInMask(summary->mask));
		}
	}

	EigrpDualLogAll(ddb, EIGRP_DUALEV_GETSUM2, &summary->minMetric, NULL);

	if(!save_dndb){
		EIGRP_FUNC_LEAVE(EigrpIpGetSummaryMetric);
		return FALSE;
	}

	/* Save vector metric in ddb for caller. */
	route->vecMetric	= save_dndb->vecMetric;
	route->metric	= summary->minMetric;
	EIGRP_FUNC_LEAVE(EigrpIpGetSummaryMetric);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpProcessIfdownEvent

	Desc:	This function is to process an ifdown event.  Tear down all neighbors, and delete the
			IIDB if so directed.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the ifdown event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfdownEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st	*iidb;
	void		*idb;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfdownEvent);

	ddb	= pdb->ddb;
	idb	= work->c.ifd.ifp;
	if(ddb && idb){
		iidb = EigrpFindIidb(ddb, idb);
		if(iidb){
			/* Tear down all peers. */
			EigrpTakeNbrsDown(ddb, idb, TRUE, "interface down");

			/* If we're deleting the IIDB, turn off multicast receive and clean up any
			  * autosummaries. */
			if(iidb->goingDown == TRUE){
				EigrpIpSetupMulticast(ddb, idb, FALSE);
				EigrpDeallocateIidb(ddb, iidb);
				EigrpIpBuildAutoSummaries(pdb, idb);
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfdownEvent);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessSummaryEntryEvent

	Desc:	This function is to process a summary from the work queue.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessSummaryEntryEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualDdb_st	*ddb;
	EigrpDualNdb_st	*dndb;
	EigrpVmetric_st	*metric;
	EigrpSum_st		*summary;
	EigrpDualNewRt_st	route;
	EigrpDualRdb_st	*drdb, *drdbTmp;
	S32		add;
	U32		event, address, mask;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessSummaryEntryEvent);
	EigrpUtilMemZero((void *) &route, sizeof(route));

	address	= work->c.sum.dest.address;
	mask	= work->c.sum.dest.mask;

	route.dest.address	= address;
	route.dest.mask	= mask;
	ddb				= pdb->ddb;
	add				= work->c.sum.add;
	route.idb			= ddb->nullIidb;
	metric			= work->c.sum.vmetric;

	_EIGRP_DEBUG("EigrpIpProcessSummaryEntryEvent:route  for %x metric=%ld\n", address, metric);

	EigrpDualLogAll(ddb, EIGRP_DUALEV_PROCSUM, &address, &mask);
	EigrpDualLogAll(ddb, EIGRP_DUALEV_PROCSUM2, &add, &metric);

	/* Deleting an entry that is not present. */
	dndb = EigrpDualNdbLookup(ddb, &route.dest);
	if(!add && !dndb){
		if(metric){
		    EigrpPortMemFree(metric);
		}
		EIGRP_FUNC_LEAVE(EigrpIpProcessSummaryEntryEvent);
		return;
	}

	event = (add) ? EIGRP_DUAL_RTUP : EIGRP_DUAL_RTDOWN;
	route.origin	= EIGRP_ORG_SUMMARY;
	route.rtType	= EIGRP_ROUTE_EIGRP ;  

	/* Use metric in workq entry, if caller supplied them.  Otherwise, scan topology table to find
	  * minimum metric. */
	if(metric == NULL){
		/* Find summary, if it exists. */
		for(summary = (EigrpSum_st*)pdb->sumQue->head; summary;
			        summary = summary->next){
			if(summary->address == address && summary->mask == mask){
				break;
			}
		}

		if(!summary || !EigrpIpGetSummaryMetric(ddb, summary, &route)){
			/* Don't delete an entry that doesn't exist or is not our summary. */
			if(!dndb){
				EIGRP_FUNC_LEAVE(EigrpIpProcessSummaryEntryEvent);
				return;
			}
			if(dndb->rdb){
				for(drdb = dndb->rdb; drdb; drdb = drdb->next){
					if(drdb->origin == EIGRP_ORG_SUMMARY){
						break; /* We find a summary drdb */
					}
				}
				if(!drdb){
					EIGRP_FUNC_LEAVE(EigrpIpProcessSummaryEntryEvent);
					return; /* Not find summary drdb */
				}
			}

			/* No more specific routes exist, or the summary is being deleted. If summary is in
			  * topology table, we must get rid of it. */
			route.metric			= EIGRP_METRIC_INACCESS;
			route.vecMetric.delay	= EIGRP_METRIC_INACCESS;
			event					= EIGRP_DUAL_RTDOWN;
		}
	}else{
		route.vecMetric = *metric;
		EigrpPortMemFree(metric);
		route.metric = EigrpDualComputeMetric(route.vecMetric.bandwidth,
												route.vecMetric.load,
												route.vecMetric.delay,
												route.vecMetric.reliability,
												ddb->k1, ddb->k2,
												ddb->k3, ddb->k4, ddb->k5 );
	}

	/* Inform DUAL about summary route. */
	route.opaqueFlag	= 0;
	route.flagExt		= FALSE;
	if(EigrpIpIsExterior(address, mask)){
		route.opaqueFlag = EIGRP_INFO_CD;
	}
	EigrpDualRtchange(ddb, &route, event);
	
#if(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_12)
	if(dndb && dndb->rdb){
		for(drdb = dndb->rdb; drdb;){
			drdbTmp = drdb->next;
			if(drdb->origin == EIGRP_ORG_EIGRP){
				EigrpDualRdbClear(ddb, &dndb, drdb);/*此函中会将drdb从链表中删除*/
			}
			drdb	= drdbTmp;
		}
	}

#endif
	EIGRP_FUNC_LEAVE(EigrpIpProcessSummaryEntryEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessPassiveIntfCommand

	Desc:	This function is to process eigrp interface passive command.
 
 			If we are switching states, we need to tear everything down and then
			rebuild it in the other state.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given eigrp interface
			noFlag		- the flag of negative command
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessPassiveIntfCommand(S32 noFlag, EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st		*iidb;
	S32	sense;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessPassiveIntfCommand);

	ddb = pdb->ddb;
	if(!ddb) {
		EIGRP_FUNC_LEAVE(EigrpIpProcessPassiveIntfCommand);
		return;
	}

	iidb = EigrpFindIidb(ddb, pEigrpIntf);

	/* If there's no IIDB, it means that we haven't configured anything on this interface yet. 
	  * We'll wait, and when the appropriate commands are issued, the passive interface will be
	  * configured. */
	if(!iidb){
		EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: Interface not enabled\n"); 
		EIGRP_FUNC_LEAVE(EigrpIpProcessPassiveIntfCommand);
		return;
	}

	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}
	
	/* See if we're in the right state.  If so, bail. */
	if(iidb->passive == sense){
		if(iidb->passive == TRUE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: Interface already in passive mode\n"); 
		}else{
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: Interface not in passive mode\n"); 
		}
		EIGRP_FUNC_LEAVE(EigrpIpProcessPassiveIntfCommand);
		return;
	}

	/* SUCCESS, we're switching states.  Go do it. */
	EigrpPassiveInterface(ddb, iidb, sense);

	/* Update any autosummaries for this interface. */
	EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);
	EIGRP_FUNC_LEAVE(EigrpIpProcessPassiveIntfCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessInvisibleIntfCommand

	Desc:	This function is to process eigrp interface passive command.
 
 			If we are switching states, we need to tear everything down and then
			rebuild it in the other state.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			pEigrpIntf	- pointer to the given eigrp interface
			noFlag		- the flag of negative command
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessInvisibleIntfCommand(S32 noFlag, EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st		*iidb;
	S32	sense;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessInvisibleIntfCommand);

	ddb = pdb->ddb;
	if(!ddb) {
		EIGRP_FUNC_LEAVE(EigrpIpProcessInvisibleIntfCommand);
		return;
	}

	iidb = EigrpFindIidb(ddb, pEigrpIntf);

	/* If there's no IIDB, it means that we haven't configured anything on this interface yet. 
	  * We'll wait, and when the appropriate commands are issued, the passive interface will be
	  * configured. */
	if(!iidb){
		EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: Interface not enabled\n"); 
		EIGRP_FUNC_LEAVE(EigrpIpProcessInvisibleIntfCommand);
		return;
	}

	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}
	
	/* See if we're in the right state.  If so, bail. */
	if(iidb->invisible == sense){
		if(iidb->invisible == TRUE){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: Interface already in invisible mode\n"); 
		}else{
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "EIGRP: Interface not in invisible mode\n"); 
		}
		EIGRP_FUNC_LEAVE(EigrpIpProcessInvisibleIntfCommand);
		return;
	}

	/* SUCCESS, we're switching states.  Go do it. */
	EigrpInvisibleInterface(ddb, iidb, sense);

	EIGRP_FUNC_LEAVE(EigrpIpProcessInvisibleIntfCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessConnChangeEvent

	Desc:	This function is to process an interface or connected route change event. If an address
			was supplied, only change that particular address for  the interface.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			work	- pointer to the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessConnChangeEvent(EigrpPdb_st *pdb, EigrpWork_st *work)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st		*iidb;
	EigrpIntf_pt 		pEigrpIntf;

	EigrpDualNewRt_st	route;
	U32	event, address, mask;
	S32	adding, config, retVal;
	void	*pRtNode;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessConnChangeEvent);

	pEigrpIntf	= work->c.con.ifp;
	address		= work->c.con.dest.address;
	mask		= work->c.con.dest.mask;
	adding		= work->c.con.up;
	config		= work->c.con.config;

	EigrpUtilMemZero((void *) & route, sizeof(route));

	{
		_EIGRP_DEBUG("EigrpIpProcessConnChangeEvent:    interface infomation\n");
		_EIGRP_DEBUG("\t name=%s\n", pEigrpIntf->name);
		_EIGRP_DEBUG("\t metric=%ld\n", pEigrpIntf->metric);
		_EIGRP_DEBUG("\t mtu=%ld\n", pEigrpIntf->mtu);
		_EIGRP_DEBUG("\t bandwidth=%ld\n", pEigrpIntf->bandwidth);
		_EIGRP_DEBUG("\t delay=%ld\n", pEigrpIntf->delay);
		_EIGRP_DEBUG("\t ipAddr=%x\n", pEigrpIntf->ipAddr);
		_EIGRP_DEBUG("\t ipmask=%x\n", pEigrpIntf->ipMask);
	}
	

	/* Setup protocol specific parameters for call. */
	ddb	= pdb->ddb;
	iidb	= EigrpFindIidb(ddb, pEigrpIntf);
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessConnChangeEvent);
		return;
	}

	/* If this change is due to configuration, rebuild autosummaries. */
	if((pdb->sumAuto == TRUE) && config && !BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK)){
		EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);
	}

	route.idb				= iidb;
	route.vecMetric.mtu	= pEigrpIntf->mtu;
	route.dest.address		= address;
	route.dest.mask		= mask;
	route.opaqueFlag		= 0;

	if(adding){
		/*
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST)){
			route.vecMetric.delay = EIGRP_B_DELAY;
		}else{
			route.vecMetric.delay = EIGRP_S_DELAY;
		}
		*/
		/*hanbing_modify 120704 start*/
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST)){
			if(pEigrpIntf->delay > 0){
				route.vecMetric.delay = pEigrpIntf->delay;
				_EIGRP_DEBUG("connect interface route delay=interface delay=%ld\n", pEigrpIntf->delay);
			}else{
				route.vecMetric.delay = EIGRP_B_DELAY;
				_EIGRP_DEBUG("connect interface route delay=interface default delay=%ld\n", EIGRP_B_DELAY);
			}
		}else{
			if(pEigrpIntf->delay > 0){
				route.vecMetric.delay = pEigrpIntf->delay;
				_EIGRP_DEBUG("connect interface route delay=interface delay=%ld    2\n", pEigrpIntf->delay);
			}else{
				route.vecMetric.delay = EIGRP_S_DELAY;
				_EIGRP_DEBUG("connect interface route delay=interface default delay=%ld    2\n", EIGRP_B_DELAY);
	
			}
		}

		route.vecMetric.bandwidth	= EigrpDualIgrpMetricToEigrp(EIGRP_SCALED_BANDWIDTH(1000 * pEigrpIntf->bandwidth)); 
		route.vecMetric.reliability	= 255;
		route.vecMetric.load		= 1;
		route.vecMetric.hopcount	= 0;
		route.metric				= EigrpDualComputeMetric(route.vecMetric.bandwidth,
															route.vecMetric.load, route.vecMetric.delay,
															route.vecMetric.reliability,
															ddb->k1, ddb->k2, ddb->k3, ddb->k4, ddb->k5 );

		event = EIGRP_DUAL_RTUP;

		pRtNode = EigrpPortCoreRtNodeFindExact(address & mask, mask);//根据目的地址和掩码查找路由链表
		if(!pRtNode) {          
			EIGRP_FUNC_LEAVE(EigrpIpProcessConnChangeEvent);
			return;
		}

		retVal	= EigrpPortCoreRtNodeHasConnect(pRtNode);//确认链表是否有直连路由
		if(retVal == FALSE){
			EIGRP_FUNC_LEAVE(EigrpIpProcessConnChangeEvent);
			return;
		}
		
		if(EigrpIpIsExterior(route.dest.address, route.dest.mask)){
			route.opaqueFlag = EIGRP_INFO_CD;
		}
	}else{
		route.succMetric		= EIGRP_METRIC_INACCESS;
		route.metric			= EIGRP_METRIC_INACCESS;
		route.vecMetric.delay	= EIGRP_METRIC_INACCESS;
		event				= EIGRP_DUAL_RTDOWN;
	}

	EigrpDualConnRtchange(ddb, &route, event);

	EigrpIpConnSummaryDepend(pdb->ddb, pEigrpIntf, address, mask, adding);
	EIGRP_FUNC_LEAVE(EigrpIpProcessConnChangeEvent);

	return;
}


/************************************************************************************

	Name:	EigrpIpProcessDistanceCommand

	Desc:	This function is to process eigrp distance command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			inter		- internal distance
			exter	- external disteance
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessDistanceCommand(EigrpPdb_st *pdb, U32 inter, U32 exter)
{
	EigrpRtNode_pt	rn;
	EigrpRouteInfo_st	*eigrpInfo;
	S32		int_need = FALSE, ext_need = FALSE;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessDistanceCommand);
	if(pdb->distance != inter){
		int_need	= TRUE;
		pdb->distance	= inter;
	}

	if(pdb->distance2 != exter){
		ext_need	= TRUE;
		pdb->distance2	= exter;
	}

	if(!int_need && !ext_need){
		EIGRP_FUNC_LEAVE(EigrpIpProcessDistanceCommand);
		return;
	}

	/* Delete & reInsert all Eigrp routes */
	for(rn = EigrpUtilRtNodeGetFirst(pdb->rtTbl); rn; rn = EigrpUtilRtNodeGetNext(pdb->rtTbl, rn)){
		eigrpInfo	= (EigrpRouteInfo_st *)rn->extData;
		if(eigrpInfo){
			if(eigrpInfo->type != EIGRP_ROUTE_EIGRP){
				continue;
			}

			if(eigrpInfo->isExternal){
				if(!ext_need){     
					continue;
				}
				eigrpInfo->distance = pdb->distance2;
			}else{
				if(!int_need){    
					continue;
				}
				eigrpInfo->distance = pdb->distance;
			}

			EigrpPortCoreRtDel(rn->dest, rn->mask, eigrpInfo->nexthop.s_addr, eigrpInfo->metric);
			EigrpPortCoreRtAdd(rn->dest, rn->mask, eigrpInfo->nexthop.s_addr, eigrpInfo->metric,
								eigrpInfo->distance, eigrpInfo->process);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessDistanceCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessDefaultmetricCommand

	Desc:	This function is process eigrp default metric command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			noFlag	- the flag of negative command
			vmetric	- pointer to vector metric
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessDefaultmetricCommand(S32 noFlag, EigrpPdb_st *pdb, EigrpVmetric_st	*vmetric)
{
	EigrpRedisEntry_st	*redis;
	S32	sense;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessDefaultmetricCommand);
	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}
	
	pdb->metricFlagDef	= sense;
	if(vmetric){
		if(EigrpPortMemCmp((U8 *)vmetric, (U8 *)&pdb->vMetricDef, sizeof(EigrpVmetric_st)) == 0){ 
			EigrpPortMemFree(vmetric);
			EIGRP_FUNC_LEAVE(EigrpIpProcessDefaultmetricCommand);
			return;   /*the same as old one */
		}	
		pdb->vMetricDef	= *vmetric;
		EigrpPortMemFree(vmetric);
 	}
	
	for(redis = pdb->redisLst; redis; redis = redis->next){
		/* update redistributed route here, when user issue default-metric command */
		switch(redis->proto){
			case EIGRP_ROUTE_IGRP:
			default :
				if(!redis->vmetric){
					EigrpPortRedisTrigger(redis->proto);
				}
		} 
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessDefaultmetricCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessMetricweightCommand

	Desc:	This function is process metric weight command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessMetricweightCommand(EigrpPdb_st *pdb)
{
	EigrpDualDdb_st	*ddb;
	EigrpRedisEntry_st	*redis;
	EigrpIntfAddr_pt	pAddr;
	EigrpIntf_pt		pEigrpIntf;
	U32		address,mask;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessMetricweightCommand);
	ddb	= pdb->ddb;
	ddb->k1	= pdb->k1;
	ddb->k2 = pdb->k2;
	ddb->k3 = pdb->k3;
	ddb->k4 = pdb->k4;
	ddb->k5 = pdb->k5;

	EigrpTakeAllNbrsDown(ddb, "kvaules changed");

	for(redis = pdb->redisLst; redis; redis = redis->next){
		EigrpPortRedisDelProto(pdb, redis->proto, redis->process);
		EigrpIpRedistributeRoutesCleanup(redis->rcvProc, redis->proto, redis->proto);
		EigrpPortRedisAddProto(pdb, redis->proto, redis->process);
		EigrpPortRedisTrigger(redis->proto);
	}

	/* for each interface */
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){        
			continue;
		}

		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK)){
			continue;
		}

		for(pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				address	= pAddr->ipDstAddr;
			}else{
				address	= pAddr->ipAddr;
			}
			mask	= pAddr->ipMask;

			if(EigrpIpConnAddressActivated(pdb,  address, mask)){
				EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, FALSE);
				EigrpIpActivateConnAddress(pdb, pEigrpIntf, address, mask, TRUE);
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessMetricweightCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetHelloIntervalCommand

	Desc:	This function is to process intface hello interval command in eigrp task.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			interval	- the hello interval
	
	Ret:		NONE
************************************************************************************/
void	EigrpIpProcessIfSetHelloIntervalCommand(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName, U32 interval)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetHelloIntervalCommand);

	if(!pdb) {
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetHelloIntervalCommand);
		return;
	}

	pEigrpIntf = EigrpIntfFindByName(ifName);

	/*无线信道始终由pal开始*/
	if(SUCCESS== EigrpIpProcessIfCheckUnnumberd(ifName)){
		pEigrpIntf = NULL;
	}
	
	if(pEigrpIntf)
	{
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
#ifdef _EIGRP_VLAN_SUPPORT	
	else{/*vlan接口有可能是FRP接口*/
		if(!EigrpPortMemCmp("vlan", ifName, 4)){
			if(interval % 5 != 0){
				printf("Hello-interval MUST be times of 5 when set on vlan interface.\n");
				return;
			}
			EigrpPortVlanHelloInterval(noFlag, ifName, interval);
			return;
		}
	}
#endif//_EIGRP_VLAN_SUPPORT	
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetHelloIntervalCommand);
		return;
	}

	/* When user "no" hello-interval, we should set hello-inverval to default value insead of zero. */
	if(noFlag == FALSE){
		iidb->helloInterval = interval *EIGRP_MSEC_PER_SEC;
	}else{            /* Get default value. The paramter is not used now */
		iidb->helloInterval = EIGRP_DEF_HELLOTIME;
	}
#ifdef _DC_
	DcUtilSetUaiHelloInterval(iidb->idb->name, interval);
#endif//_DC_	
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetHelloIntervalCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetHoldtimeCommand

	Desc:	This function is to process intface hello holdtime command in eigrp task.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			time		- hello holdtime
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetHoldtimeCommand(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName, U32 time)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetHoldtimeCommand);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetHoldtimeCommand);
		return;
	}

	pEigrpIntf = EigrpIntfFindByName(ifName);

	/*无线信道始终由pal开始*/
	if(SUCCESS== EigrpIpProcessIfCheckUnnumberd(ifName)){
		pEigrpIntf = NULL;
	}
	
	if(pEigrpIntf)
	{
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
#ifdef _EIGRP_VLAN_SUPPORT	
	else
	{/*vlan接口有可能是FRP接口*/
		if(!EigrpPortMemCmp("vlan", ifName, 4)){
			EigrpPortVlanHoldTime(noFlag, ifName, time);
			return;
		}
	}
#endif//_EIGRP_VLAN_SUPPORT	
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetHoldtimeCommand);
		return;
	}

	/* When user "no" hold-time, we should set hello-inverval to default value insead of zero. */
	if(noFlag == FALSE){
		iidb->holdTime	= time *EIGRP_MSEC_PER_SEC;
	}else{
		/* Get default value. The paramter is not used now */
		iidb->holdTime	= EIGRP_DEF_HOLDTIME;
	}
#ifdef _DC_	
	DcUtilSetUaiHoldTime(iidb->idb->name, iidb->holdTime);
#endif//_DC_	
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetHoldtimeCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetSplitHorizonCommand

	Desc:	This function is process eigrp interface split horizon command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			noFlag	- the flag of negative command
	
	Ret:		
************************************************************************************/

 void	EigrpIpProcessIfSetSplitHorizonCommand(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName)
{
	 EigrpIntf_pt	 pEigrpIntf;
	 EigrpIdb_st	 *iidb;
	 S32	mode;
	 
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetSplitHorizonCommand);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetSplitHorizonCommand);
		return;
	 }
	iidb	= NULL;	
	 pEigrpIntf = EigrpIntfFindByName(ifName);
	 if(pEigrpIntf){
		 iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	 
	 if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetSplitHorizonCommand);
		return;
	}

	if(noFlag == TRUE){
		mode	= FALSE;
	}else{
		mode	= TRUE;
	}

	/* Set the value. */
	if(iidb->splitHorizon != mode){
		iidb->splitHorizon = mode;
		EigrpTakeNbrsDown(pdb->ddb, pEigrpIntf, FALSE, "split horizon changed");
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetSplitHorizonCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetBandwidthPercentCommand

	Desc:	This function is to process eigrp interface bandwidth percent command.
			
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			percent	- the bandwidth percent
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetBandwidthPercentCommand(EigrpPdb_st *pdb, U8 *ifName, U32 percent)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetBandwidthPercentCommand);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidthPercentCommand);
		return;
	}

	pEigrpIntf = EigrpIntfFindByName(ifName);
	if(pEigrpIntf){
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidthPercentCommand);
		return;
	}

	/* Set the value. */
	if(percent == 0){
		percent = EIGRP_DEF_BANDWIDTH_PERCENT;
	}
	iidb->bwPercent = percent;
	EigrpSetPacingIntervals(pdb->ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidthPercentCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetBandwidthCommand

	Desc:	This function is to process eigrp interface bandwidth percent command.
			
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			percent	- the bandwidth percent
	
	Ret:		NONE
************************************************************************************/
/* 检测这个接口是否为无线信道，或者是无编号 */
S32 EigrpIpProcessIfCheckUnnumberd(char *ifName)
{
#ifdef _EIGRP_VLAN_SUPPORT
	char ifVid[16];
	int   vid;
	/*接口名称为vlan*/
	if(!EigrpPortMemCmp(ifName, "vlan", 4)){
		EigrpPortMemCpy(ifVid, ifName+4, 12);
		vid = EigrpPortAtoi(ifVid);
		if(vid >= 30){/*无线信道*/
			return SUCCESS;
		}else {/*有线信道暂不考虑*/
			return FAILURE;
		}
	}
#endif//_EIGRP_VLAN_SUPPORT	
	return FAILURE;
}
#ifdef _EIGRP_UNNUMBERED_SUPPORT
/*cwf 20121225 for set uuai band and delay*/
void	EigrpIpProcessIfSetUUaiBandwidthCommand(U32 asNum, U8 *ifName)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	EigrpIdb_st		*pIidb;
	U32	net, mask;
#ifdef _DC_	
	DcUai_pt	pUai;
#endif//_DC_	
	S32	retVal;
	void *ifp=NULL;
	EigrpPdb_st *pdb;
	int cnt;

	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetUUaiBandwidthCommand);

	pdb	= EigrpIpFindPdb(asNum);
	if(!pdb){
		_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: pdb error\n");
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#ifdef _DC_	
	pUai = DcUtilGetUaiByName(ifName);
	if(!pUai){
		_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: find pUai for %s error\n", ifName);
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#endif//_DC_	
#if 0  /*changed 20130105---cetc--f*/
	retVal = DcPortUaiUpdateCurSei(pUai);
	if(retVal != TRUE){
		_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: pSei best error\n");
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#endif	
#ifdef _DC_	
	pEigrpIntf = EigrpIntfFindByName(pUai->name);
	if(!pEigrpIntf){
		//_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: find pEigrpIntf for %s error\n", pUai->name);
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#else//_DC_	
	pEigrpIntf = EigrpIntfFindByName(ifName);
	if(!pEigrpIntf){
		//_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: find pEigrpIntf for %s error\n", pUai->name);
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#endif//_DC_	
	iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	if(!iidb){
		_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: find iidb for %s error\n", pEigrpIntf->name);
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#ifdef _DC_	
	retVal = EigrpPortSetIfBandwidthAndDelay_2(pEigrpIntf->sysCirc, pUai->sei[pUai->seiCur].pSei->pCircSys);
	if(retVal != SUCCESS){
		_EIGRP_DEBUG("EigrpIpProcessIfSetUUaiBandwidthCommand: band and delay for %s-->%s error\n", pUai->sei[pUai->seiCur].pSei->name, pEigrpIntf->name);
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);
		return;
	}
#endif// _DC_	
	EigrpSysIntfDown_ex(pEigrpIntf->sysCirc);
	EigrpSysIntfUp_ex(pEigrpIntf->sysCirc);
	for(pIidb = (EigrpIdb_st*) pdb->ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
		EigrpTakeNbrsDown(pdb->ddb, pIidb->idb, TRUE, "bandwidth changed");
	}

	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetUUaiBandwidthCommand);

	return;
}
#endif//_EIGRP_UNNUMBERED_SUPPORT

void	EigrpIpProcessIfSetBandwidthCommand(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName, U32 bw)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	EigrpIdb_st		*pIidb;
	U32	net, mask;
#ifdef _DC_	
	DcUai_pt	pUai;
#endif//_DC_	
	S32	retVal;
	void *ifp=NULL;
	int cnt;

	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetBandwidthCommand);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidthCommand);
		return;
	}

	pEigrpIntf = EigrpIntfFindByName(ifName);		

	/*无线信道始终由pal开始*/
	if(SUCCESS== EigrpIpProcessIfCheckUnnumberd(ifName)){
		_EIGRP_DEBUG("EigrpIpProcessIfCheckUnnumberd:%s is unnumbered\n ", ifName);
		pEigrpIntf = NULL;
	}
	
	if(pEigrpIntf){
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(iidb){
		EigrpSysIntfDown_ex(pEigrpIntf->sysCirc);
		
		EigrpSysIntfUp_ex(pEigrpIntf->sysCirc);
		for(pIidb = (EigrpIdb_st*) pdb->ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
			EigrpTakeNbrsDown(pdb->ddb, pIidb->idb, TRUE, "bandwidth changed");
		}
	}else { 

		//pal_kernel_if_list(1);
		//ifp = pal_kernel_if_lookup(ifName);
		if(ifp==NULL){
			_EIGRP_DEBUG("EigrpIpProcessIfSetBandwidthCommand: %s find in kernel error\n", ifName);
			return;
		}
		EigrpPortChangeIfBandwidth(ifp, bw);
#ifdef _EIGRP_VLAN_SUPPORT			
		EigrpPortVlanBandwidth(noFlag, ifName, bw);
#endif// _EIGRP_VLAN_SUPPORT			
#ifdef _DC_		
		for(pUai = DcPortGetFirstUaiByCircName(ifName); pUai; pUai = DcPortGetNextUaiByCircName(pUai, ifName)){
			retVal = DcPortUaiUpdateCurSei(pUai);
			if(retVal != TRUE){
				continue;
			}

			pEigrpIntf = EigrpIntfFindByName(pUai->name);
			if(!pEigrpIntf){
				continue;
			}
			iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
			if(!iidb){
				continue;
			}

			retVal = EigrpPortSetIfBandwidth_2(pEigrpIntf->sysCirc, pUai->sei[pUai->seiCur].pSei->pCircSys);
			if(retVal != SUCCESS){
				continue;
			}
			EigrpSysIntfDown_ex(pEigrpIntf->sysCirc);
			EigrpSysIntfUp_ex(pEigrpIntf->sysCirc);
			for(pIidb = (EigrpIdb_st*) pdb->ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
				EigrpTakeNbrsDown(pdb->ddb, pIidb->idb, TRUE, "bandwidth changed");
			}
		}
#endif//_DC_	
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidthCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetDelayCommand

	Desc:	This function is to process eigrp interface bandwidth percent command.
			
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			percent	- the bandwidth percent
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetDelayCommand(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName, U32 delay)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st *iidb = NULL;
	EigrpIdb_st 	*pIidb;
	U32 net, mask;
#ifdef _DC_	
	DcUai_pt	pUai;
#endif//_DC_
	S32 retVal;
	void* ifp=NULL;
	int cnt;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetDelayCommand);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetDelayCommand);
		return;
	}
	
	pEigrpIntf = EigrpIntfFindByName(ifName);

	/*无线信道始终由pal开始*/
	if(SUCCESS== EigrpIpProcessIfCheckUnnumberd(ifName)){
		printf("EigrpIpProcessIfCheckUnnumberd:%s is unnumbered\n ", ifName);
		pEigrpIntf = NULL;
	}
	
	if(pEigrpIntf){
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(iidb){		
		EigrpSysIntfDown_ex(pEigrpIntf->sysCirc);
		EigrpSysIntfUp_ex(pEigrpIntf->sysCirc);
		for(pIidb = (EigrpIdb_st*) pdb->ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
			EigrpTakeNbrsDown(pdb->ddb, pIidb->idb, TRUE, "delay changed");
		}
	}
	else 
	{
		//pal_kernel_if_list(1);
		//ifp = pal_kernel_if_lookup(ifName);
		if(ifp==NULL){
			_EIGRP_DEBUG("EigrpIpProcessIfSetBandwidthCommand: %s find in kernel error\n", ifName);
			return;
		}

		EigrpPortChangeIfDelay(ifp,  delay);
#ifdef _EIGRP_VLAN_SUPPORT			
		EigrpPortVlanBandwidth(noFlag, ifName, delay);
#endif// _EIGRP_VLAN_SUPPORT			
#ifdef _DC_		
		for(pUai = DcPortGetFirstUaiByCircName(ifName); pUai; pUai = DcPortGetNextUaiByCircName(pUai, ifName)){
			retVal = DcPortUaiUpdateCurSei(pUai);
			if(retVal != TRUE){
				continue;
			}
				
				pEigrpIntf = EigrpIntfFindByName(pUai->name);
				if(!pEigrpIntf){
					continue;
				}
				iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
				if(!iidb){
					continue;
				}

				retVal = EigrpPortSetIfDelay_2(pEigrpIntf->sysCirc, pUai->sei[pUai->seiCur].pSei->pCircSys);
				if(retVal != SUCCESS){
					continue;
				}
				EigrpSysIntfDown_ex(pEigrpIntf->sysCirc);
				EigrpSysIntfUp_ex(pEigrpIntf->sysCirc);
				for(pIidb = (EigrpIdb_st*) pdb->ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
						EigrpTakeNbrsDown(pdb->ddb, pIidb->idb, TRUE, "bandwidth changed");
				}
		}
#endif// _DC_			
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetDelayCommand);
	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetBandwidth

	Desc:	This function is to process eigrp interface bandwidth percent command.
			
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			percent	- the bandwidth percent
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetBandwidth(EigrpPdb_st *pdb, U8 *ifName, U32 bw)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetBandwidth);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidth);
		return;
	}
				
	pEigrpIntf = EigrpIntfFindByName(ifName);
	if(pEigrpIntf){
		pEigrpIntf->bandwidth	= bw;
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidth);
		return;
	}
			
	EigrpTakeNbrsDown(pdb->ddb, pEigrpIntf, TRUE, "bandwidth changed");
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidth);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetBandwidth

	Desc:	This function is to process eigrp interface bandwidth percent command.
			
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			percent	- the bandwidth percent
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetDelay(EigrpPdb_st *pdb, U8 *ifName, U32 delay)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetBandwidth);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidth);
		return;
	}
				
	pEigrpIntf = EigrpIntfFindByName(ifName);
	if(pEigrpIntf){
		pEigrpIntf->delay	= delay;
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidth);
		return;
	}
			
	EigrpTakeNbrsDown(pdb->ddb, pEigrpIntf, TRUE, "delay changed");
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetBandwidth);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSummaryAddressCommand

	Desc:	This function is to process eigrp summary command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			noFlag	- the flag of negative command
			address	- summary ip address
			mask	- summary ip mask
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSummaryAddressCommand(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName, U32 address, U32 mask)
{
	EigrpIdb_st	*iidb; 
	EigrpDualDdb_st	*ddb;
	EigrpIntf_pt	pEigrpIntf;
	S32 sense;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSummaryAddressCommand);
	ddb = pdb->ddb;
	if(!ddb){  
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSummaryAddressCommand);
		return;
	}

	pEigrpIntf		= EigrpIntfFindByName(ifName);
	if(!pEigrpIntf){ 
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSummaryAddressCommand);
		return;
	}
	
	if(noFlag == TRUE){
		sense	= FALSE;
	}else{
		sense	= TRUE;
	}
	
	EigrpIpConfigureSummaryEntry(pdb, address, mask, pEigrpIntf, sense, EIGRP_SUM_TYPE_CFG);
	
	if (pdb->sumAuto == TRUE) {
		if (sense == TRUE) {
			EigrpIpBuildAutoSummaries(pdb, pEigrpIntf);	
		} else { /* sense == FALSE */
		   	for	(iidb = (EigrpIdb_st*) ddb->iidbQue->head; iidb; iidb = iidb->next){
				if (iidb->idb != pEigrpIntf){
					EigrpIpBuildAutoSummaries(pdb, iidb->idb);
				}
			} 		    
		}
	}	

	if(ifName){
		EigrpTakeNbrsDown(ddb, pEigrpIntf, FALSE, "summary configured");
	}else{
		EigrpTakeAllNbrsDown(ddb, "summary configured");
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSummaryAddressCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfUaiP2mp

	Desc:	This function is to process eigrp summary command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			noFlag	- the flag of negative command
			address	- summary ip address
			mask	- summary ip mask
	
	Ret:		NONE
************************************************************************************/

#ifdef _EIGRP_UNNUMBERED_SUPPORT
void	EigrpIpProcessIfUaiP2mp(S32 noFlag, U8 *seiName, U32 vlan_id)
{
	EigrpIntf_pt	pEigrpIntf;
	S8	name[128];

	EIGRP_FUNC_ENTER(EigrpIpProcessIfUaiP2mp);

#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	if(seiName && seiName[0]){
		pEigrpIntf		= EigrpIntfFindByName(seiName);
	}
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	if(vlan_id != 0){
#if 0	/* tigerwh 120603 */
		sprintf(name, "vlan %d", vlan_id);
#else
		sprintf(name, "vlan%d", vlan_id);
#endif
		pEigrpIntf		= EigrpIntfFindByName(name);
	}
#endif
	if(pEigrpIntf){ 
		_EIGRP_DEBUG("This port is already STATIC-IP port or UAI port! \n");
		
		/*return;*/
	}else{
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
		EigrpPortSndDcUaiP2mp(noFlag, seiName);
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef _EIGRP_VLAN_SUPPORT		
		EigrpPortSndDcUaiP2mp_vlanid(noFlag, vlan_id);
#endif//_EIGRP_VLAN_SUPPORT		
#endif

	}
/*
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	EigrpPortSndDcUaiP2mp(noFlag, seiName);
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	EigrpPortSndDcUaiP2mp_vlanid(noFlag, vlan_id);
#endif
*/
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfUaiP2mp);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfUaiP2mp

	Desc:	This function is to process eigrp summary command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			noFlag	- the flag of negative command
			address	- summary ip address
			mask	- summary ip mask
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfUaiP2mp_Nei(S32 noFlag, U32 ipaddr, U32 vlan_id)
{
	EigrpIntf_pt	pEigrpIntf;
	S8	name[128];
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfUaiP2mp_Nei);
#ifdef _DC_		
	EigrpPortSndDcUaiP2mp_Nei(noFlag, ipaddr, vlan_id);
#endif//_DC_
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfUaiP2mp_Nei);
	
	return;
}
#endif /* _EIGRP_UNNUMBERED_SUPPORT */

/************************************************************************************

	Name:	EigrpIpProcessIfSetAuthenticationKey

	Desc:	This function is to process eigrp interface auth key command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			auth		- pointer to the interface authentication information
	
	Ret:		NONE
************************************************************************************/

 void	EigrpIpProcessIfSetAuthenticationKey(EigrpPdb_st *pdb, U8 *ifName, U8 *keyStr)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetAuthenticationKey);
	pEigrpIntf	= EigrpIntfFindByName(ifName);
	if(pEigrpIntf){ 
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	
	if(!iidb){
		if(keyStr){
			EigrpPortMemFree(keyStr);
		}
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKey);
		return;
	}

	if(keyStr){
		/* attemp to enable auth */
		if(iidb->authSet == TRUE && !EigrpPortStrCmp(iidb->authInfo.authData, keyStr)){
			EigrpPortMemFree(keyStr);
			EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKey);
			return;   /*same config */
		}

		iidb->authSet = TRUE;
		EigrpPortStrCpy(iidb->authInfo.authData, keyStr);
		EigrpPortMemFree((void*)keyStr);
	}else{
		/* attemp to disable */
		if(iidb->authSet == FALSE){
			EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKey);
			return;  /* disabled alread */
		}
		iidb->authSet = FALSE;
		EigrpUtilMemZero((void *) & iidb->authInfo, sizeof(EigrpAuthInfo_st));
	}

	EigrpTakeNbrsDown(pdb->ddb, iidb->idb, TRUE, "auth key-chain changed");
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKey);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetAuthenticationMode

	Desc:	This function is to process eigrp set auth mode command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			noFlag	- the flag of negative command
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetAuthenticationMode(S32 noFlag, EigrpPdb_st *pdb, U8 *ifName)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb;
	S32	mode;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetAuthenticationMode);
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationMode);
		return;
	}

	iidb	= NULL;
	pEigrpIntf = EigrpIntfFindByName(ifName);
	if(pEigrpIntf){
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationMode);
		return;
	}

	if(noFlag == TRUE){
		mode	= FALSE;
	}else{
		mode	= TRUE;
	}
	
	/* Set the value. */
	if(mode != iidb->authMode){
		iidb->authMode = mode;
		EigrpTakeNbrsDown(pdb->ddb, iidb->idb, TRUE, "auth mode changed");
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationMode);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessIfSetAuthenticationKeyidCommand

	Desc:	This function is process eigrp interface key id command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			ifName	- pointer to the string which indicate the interface name
			keyId	- the interface authentication key id
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessIfSetAuthenticationKeyidCommand(EigrpPdb_st *pdb, U8 *ifName, U32 keyId)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpProcessIfSetAuthenticationKeyidCommand);

	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKeyidCommand);
		return;
	}

	pEigrpIntf = EigrpIntfFindByName(ifName);
	if(pEigrpIntf){
		iidb = EigrpGetIidb(pdb->ddb, pEigrpIntf);
	}
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKeyidCommand);
		return;
	}

	/* store the key-id value into the iidb */
	iidb->authInfo.keyId = keyId;

	EigrpTakeNbrsDown(pdb->ddb, iidb->idb, TRUE, "auth key-id changed");
	EIGRP_FUNC_LEAVE(EigrpIpProcessIfSetAuthenticationKeyidCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpShouldAcceptPeer

	Desc:	Don't process hello from neighbor if not on the same subnet as received interface. 
 			Return an indication of whether or not the peer was accepted.  
			FALSE: means peer was not accepted. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			source	- pointer the source ip address where the hello packet comes from
			iidb		- pointer the interface on which the hello packet is received
		
	Ret:		TRUE	for this packet should be received
			FALSe	for this packet should not be received
************************************************************************************/
S32	EigrpIpShouldAcceptPeer(EigrpDualDdb_st *ddb, U32 *source,  EigrpIdb_st *iidb)
{
	EigrpIntf_pt	pEigrpIntf;
	U32	src;
	
	EIGRP_FUNC_ENTER(EigrpIpShouldAcceptPeer);
	pEigrpIntf = iidb->idb;
	src = *source;
	EIGRP_ASSERT((U32)pEigrpIntf);
	EIGRP_ASSERT((U32)src);

	if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
		if(EigrpPortCircIsUnnumbered(pEigrpIntf->sysCirc)){
			EIGRP_FUNC_LEAVE(EigrpIpShouldAcceptPeer);
			return TRUE;
		}else if(pEigrpIntf->remoteIpAddr == src){
			EIGRP_FUNC_LEAVE(EigrpIpShouldAcceptPeer);
			return TRUE;
		}
	}else if((src & pEigrpIntf->ipMask) == (pEigrpIntf->ipAddr & pEigrpIntf->ipMask)){
		EIGRP_FUNC_LEAVE(EigrpIpShouldAcceptPeer);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpIpShouldAcceptPeer);

	return FALSE;
}
#if	(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_12)
/************************************************************************************

	Name:	EigrpIpIsCfgSummary

	Desc:	This function is to check if the given summary entry is config summary or auto summary 
		
	Para: 	summary	    - pointer to the given summary 
	
	Ret:		true if it is config summary
			false if the is auto summary
************************************************************************************/
S32	EigrpIpIsCfgSummary(EigrpSum_st *summary)
{
	EigrpSumIdb_st	*sumIdb;

	EIGRP_FUNC_ENTER(EigrpIpIsCfgSummary);
	for(sumIdb = (EigrpSumIdb_st*)summary->idbQue->head; sumIdb; sumIdb = sumIdb->next){
		if(BIT_TEST(sumIdb->flag, EIGRP_SUM_TYPE_CFG)){
			break;
		}
	}

	if(!sumIdb){
		EIGRP_FUNC_LEAVE(EigrpIpIsCfgSummary);
		return FALSE;
	}

		EIGRP_FUNC_LEAVE(EigrpIpIsCfgSummary);
	return TRUE;
}
#endif//(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_12)

S32	EigrpIpAutoSummaryAdvertise(EigrpPdb_st *pdb, EigrpSum_st *summary)
{
	EigrpSumIdb_st	*sumIdb;
	EigrpSum_st		*pSum;

	EIGRP_FUNC_ENTER(EigrpIpIsCfgSummary);
	for(pSum = (EigrpSum_st*)pdb->sumQue; pSum; pSum = pSum->next){
		if((pSum->mask < summary->mask) && 
			(summary->address & pSum->mask) == (pSum->address & pSum->mask)){
			for(sumIdb = (EigrpSumIdb_st*)pSum->idbQue->head; sumIdb; sumIdb = sumIdb->next){
				if(BIT_TEST(sumIdb->flag, EIGRP_SUM_TYPE_CFG)){
					break;
				}
			}
			if(pSum){
				EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryAdvertise);
				return FALSE;
			}

		}
	}

	EIGRP_FUNC_LEAVE(EigrpIpAutoSummaryAdvertise);	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpIpAdvertiseRequest

	Desc:	This function is called by DUAL to ask if a destination should be advertised out the
			supplied interface. IP-EIGRP will check filter and summary(aggregation) data structures
			to determine if the destination is eligible for interface.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destination network entry
			pEigrpIntf	- pointer to the supplied interface
	
	Ret:		type of the route advertising
************************************************************************************/

U32 EigrpIpAdvertiseRequest(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpIntf_pt pEigrpIntf)
{
	EigrpPdb_st		*pdb;
	EigrpSum_st		*summary;
	EigrpDualRdb_st	*drdb;
	EigrpIdb_st		*iidb;
	EigrpSumIdb_st	*sumIdb;
	U32		advertise, addr, mask;
	EIGRP_FUNC_ENTER(EigrpIpAdvertiseRequest);

	pdb		= ddb->pdb;
	addr		= dndb->dest.address;
	mask	= dndb->dest.mask;

	/* Check outbound interface filters. */
	/* distribute-list filter */
	drdb = dndb->rdb;
	if(!drdb){
		EIGRP_FUNC_LEAVE(EigrpIpAdvertiseRequest);
		return EIGRP_ROUTE_TYPE_SUPPRESS;
	}

	iidb = EigrpFindIidb(ddb, pEigrpIntf);
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpIpAdvertiseRequest);
		return EIGRP_ROUTE_TYPE_SUPPRESS;
	}

	if(EIGRP_IN_NET0(dndb->dest.address) && dndb->dest.address != 0){
		EIGRP_FUNC_LEAVE(EigrpIpAdvertiseRequest);
		return EIGRP_ROUTE_TYPE_SUPPRESS;
	}

	/* Deal with default route only with 'default-information' switch */
	if(dndb->dest.mask == 0){
		if(pdb->rtOutDef == FALSE){
			EIGRP_FUNC_LEAVE(EigrpIpAdvertiseRequest);
			return EIGRP_ROUTE_TYPE_SUPPRESS;
		}
	}

	if(EigrpPortPermitOutgoing(dndb->dest.address & dndb->dest.mask, dndb->dest.mask, iidb) != TRUE){
		EIGRP_FUNC_LEAVE(EigrpIpAdvertiseRequest);
		return EIGRP_ROUTE_TYPE_SUPPRESS;
	}

	/* Check summary addresses. */
	advertise = EIGRP_ROUTE_TYPE_ADVERTISE;
	for(summary = (EigrpSum_st*)pdb->sumQue->head; summary;
					summary = summary->next){
		/* This is a summary address destination.  Advertise it on all interfaces if there are no
		  * component subnets to the summary; otherwise, advertise it on the specified interfaces
		  * and poison it on the others(since we may have advertised it previously). */
		if(summary->address == addr && summary->mask == mask){
			if(summary->minMetric == EIGRP_METRIC_INACCESS){ /* Unused */
				advertise = EIGRP_ROUTE_TYPE_ADVERTISE;
			}else{
#if	(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_11)
				sumIdb	= EigrpIpFindSumIdb(summary, pEigrpIntf);
				if(sumIdb){
					advertise = EIGRP_ROUTE_TYPE_ADVERTISE;
					continue;	
				}else{
					advertise = EIGRP_ROUTE_TYPE_POISON;
				}
#endif

#if	(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_12)
				sumIdb	= EigrpIpFindSumIdb(summary, pEigrpIntf);
				
				if(!sumIdb){
					if(EigrpIpIsCfgSummary(summary) == TRUE){
						advertise	= EIGRP_ROUTE_TYPE_SUPPRESS;
					}else{
						if(EigrpIpAutoSummaryAdvertise(pdb, summary)){
							advertise	= EIGRP_ROUTE_TYPE_SUPPRESS;
						}else{
							advertise = EIGRP_ROUTE_TYPE_ADVERTISE;
						}
					}
					continue;	
				}else{
					if(BIT_TEST(sumIdb->flag, EIGRP_SUM_TYPE_CFG)){
						advertise = EIGRP_ROUTE_TYPE_ADVERTISE;
					}else{
						advertise = EIGRP_ROUTE_TYPE_POISON;
					}
				}
#endif
			}
			break;      
		}
		
		/* This address is a more specific address of a configured summary. Don't advertise it if 
		  * we are advertising the summary out this interface. Keep looping, there might be a more
		  * specific summary but blow out of here as soon as a summary is found for thisaddress. */
		if((summary->address == (addr & summary->mask)) && (mask > summary->mask)){ 
			sumIdb	= EigrpIpFindSumIdb(summary, pEigrpIntf);
			if(sumIdb){
				advertise = EIGRP_ROUTE_TYPE_SUPPRESS;
				break;
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpAdvertiseRequest);

	return(advertise);
}

/************************************************************************************

	Name:	EigrpIpShowSummaryEntry

	Desc:	This function is to show summarization information for a particular summary table 
			entry.
		
	Para: 	  
	
	Ret:		
************************************************************************************/

void	EigrpIpShowSummaryEntry(EigrpSum_st *summary, S32 *banner, U8 summary_type, S8 *banner_msg)
{
	EigrpSumIdb_st *sum_idb, *sum_next;
	S32 count;

	EIGRP_FUNC_ENTER(EigrpIpShowSummaryEntry);
	count = 0;

	for(sum_idb = (EigrpSumIdb_st*)summary->idbQue->head; sum_idb; sum_idb = sum_next){
		/* mem_lock(sum_idb); */
		sum_next = sum_idb->next;
		if(!BIT_TEST(sum_idb->flag, summary_type)) {
			continue;
		}

		/* Found one! */
		if(!*banner){
			*banner = TRUE;
			EIGRP_TRC(DEBUG_EIGRP_OTHER, banner_msg);
		}

		if(count){
			if((count % 3) == 0){		
				EIGRP_TRC(DEBUG_EIGRP_OTHER, "\r\n     "); 
			}else {     			
				EIGRP_TRC(DEBUG_EIGRP_OTHER, ", "); 
			}
		}else{
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "\r\n    %s/%d for",
						EigrpUtilIp2Str(summary->address),  EigrpIpBitsInMask(summary->mask)); 
		}

		count++;
		EIGRP_TRC(DEBUG_EIGRP_OTHER, " %s", (S8 *)sum_idb->idb->name); 
	}

	if(count && summary->minMetric != EIGRP_METRIC_INACCESS){
		EIGRP_TRC(DEBUG_EIGRP_OTHER, "\r\n      Summarizing with metric %u", summary->minMetric); 
	}
	EIGRP_FUNC_LEAVE(EigrpIpShowSummaryEntry);

	return;
}

/************************************************************************************

	Name:	EigrpIpShowAutoSummary

	Desc:	This function is to show IP-EIGRP auto summary information.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpShowAutoSummary(EigrpPdb_st *pdb)
{
	EigrpSum_st *summary;
	S32 banner;
	
	EIGRP_FUNC_ENTER(EigrpIpShowAutoSummary);
	banner = FALSE;
	for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = summary->next){
		EigrpIpShowSummaryEntry(summary, &banner, EIGRP_SUM_TYPE_AUTO, "\r\n  Automatic address summarization:");
	}
	EIGRP_FUNC_LEAVE(EigrpIpShowAutoSummary);

	return;
}

/************************************************************************************

	Name:	EigrpIpShowSummary

	Desc:	This function is to show IP-EIGRP summaries in "show ip protocols" display.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpShowSummary(EigrpPdb_st *pdb)
{
	EigrpSum_st *summary;
	S32 banner;
	
	EIGRP_FUNC_ENTER(EigrpIpShowSummary);
	if(pdb->sumAuto == TRUE){           
		EigrpIpShowAutoSummary(pdb);
	}

	banner = FALSE;
	for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = summary->next){
		EigrpIpShowSummaryEntry(summary, &banner, EIGRP_SUM_TYPE_CFG, "\r\n  Address Summarization:");
	}
	EIGRP_FUNC_LEAVE(EigrpIpShowSummary);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessAutoSummaryCommand

	Desc:	This function is to process eigrp auto summary command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			sense	- auto summary or manual summary
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessAutoSummaryCommand(EigrpPdb_st *pdb, S32 sense)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st	*iidb;
	EigrpSum_st	*summary, *next_sum;
	EigrpSumIdb_st	*sum_idb, *next_sum_idb;

	EIGRP_FUNC_ENTER(EigrpIpProcessAutoSummaryCommand);
	ddb = pdb->ddb;
	if(!ddb) {
		EIGRP_FUNC_LEAVE(EigrpIpProcessAutoSummaryCommand);
		return;
	}

	if(pdb->sumAuto == sense) {
		EIGRP_FUNC_LEAVE(EigrpIpProcessAutoSummaryCommand);
		return;  /* there is no change */
	}

	pdb->sumAuto = sense;

	if(pdb->sumAuto == TRUE){
		/* Turning on autosummaries.  Build them for each interface. */
		for(iidb = (EigrpIdb_st*) ddb->iidbQue->head; iidb; iidb = iidb->next){
			EigrpIpBuildAutoSummaries(pdb, iidb->idb);
		}
	}else{
		/* Turning off autosummaries.  Delete them all. */
		for(summary = (EigrpSum_st*)pdb->sumQue->head; summary; summary = next_sum){
			next_sum = summary->next;
			for(sum_idb = (EigrpSumIdb_st*)summary->idbQue->head; sum_idb;
					sum_idb = next_sum_idb){
				next_sum_idb = sum_idb->next;
				if(BIT_TEST(sum_idb->flag, EIGRP_SUM_TYPE_AUTO)){
					EigrpIpDeleteSummaryInterface(pdb, summary, sum_idb);
				}
			}
		}
	}
	EigrpTakeAllNbrsDown(ddb, "summary configured");
	EIGRP_FUNC_LEAVE(EigrpIpProcessAutoSummaryCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpFindDdb

	Desc:	This function is to find the DDB, given the as number.
		
	Para: 	process		- autonomous system number
	
	Ret:		pointer to the found DDB ,or NULL if nothing is found
************************************************************************************/

EigrpDualDdb_st *EigrpIpFindDdb(U32 process)
{
	EigrpPdb_st *pdb;
	
	EIGRP_FUNC_ENTER(EigrpIpFindDdb);

	for(pdb = (EigrpPdb_st*)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->process == process){
			EIGRP_FUNC_LEAVE(EigrpIpFindDdb);
			return pdb->ddb;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpFindDdb);
	return(EigrpDualDdb_st *)0;
}

/************************************************************************************

	Name:	EigrpIpExteriorBit

	Desc:	This function is to check the exterior bit in a drdb, which may be in one of 2
			different places, depending upon the route type.
		
	Para: 	drdb		- pointer to the given route entry
	
	Ret:		the exterior bit of the drdb
************************************************************************************/

S8 EigrpIpExteriorBit(EigrpDualRdb_st *drdb)
{
	EigrpExtData_st *ext_info;
	
	EIGRP_FUNC_ENTER(EigrpIpExteriorBit);
	if(drdb->extData){
		ext_info = drdb->extData;
		EIGRP_FUNC_LEAVE(EigrpIpExteriorBit);
		return(ext_info->flag & EIGRP_INFO_CD);
	}else{
		EIGRP_FUNC_LEAVE(EigrpIpExteriorBit);
		return(drdb->opaqueFlag);
	}
}

/************************************************************************************

	Name:	EigrpIpExteriorFlag

	Desc:	This function is to scan the feasible succesors, looking for an exterior bit which is
			set.	
		
	Para: 	dndb	- pointer to the destination network entry
	
	Ret:		the bit if found, or 0 if no flag values are set 
************************************************************************************/

S8 EigrpIpExteriorFlag(EigrpDualNdb_st *dndb)
{
	EigrpDualRdb_st *drdb;
	S32 successors;
	
	EIGRP_FUNC_ENTER(EigrpIpExteriorFlag);

	successors = dndb->succNum;

	for(drdb = dndb->rdb; drdb && (successors-- > 0); drdb = drdb->next){
		if(EigrpIpExteriorBit(drdb)){
			EIGRP_FUNC_LEAVE(EigrpIpExteriorFlag);
			return EIGRP_INFO_CD;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpExteriorFlag);

	return 0;
}

/************************************************************************************

	Name:	EigrpIpExteriorDiffer

	Desc:	This function is to compare exterior bit of the feasible successors.  
		
	Para: 	dndb	- pointer to the destination network entry
			flag		- pointer to the exterior bit of the feasible successors
	
	Ret:		TRUE 	for there	is disagreement between the feasible successors;
			FALSE 	for otherwise.
************************************************************************************/

S32	EigrpIpExteriorDiffer(EigrpDualNdb_st *dndb, S8 *flag)
{
	EigrpDualRdb_st	*drdb;
	S32	successors;
	S8	new_flag;
	
	EIGRP_FUNC_ENTER(EigrpIpExteriorDiffer);
	drdb			= dndb->rdb;
	successors	= dndb->succNum;
	if((drdb == NULL) || (successors == 0)){
		*flag = 0;
		EIGRP_FUNC_LEAVE(EigrpIpExteriorDiffer);
		return FALSE;
	}
	*flag = EigrpIpExteriorBit(drdb);

	if(successors-- < 2){
		EIGRP_FUNC_LEAVE(EigrpIpExteriorDiffer);
		return FALSE;
	}
	for(drdb = drdb->next; drdb && (successors-- > 0); drdb = drdb->next){
		new_flag = EigrpIpExteriorBit(drdb);
		if(*flag == new_flag) {
			continue;
		}
		if(!(*flag)){
			*flag = new_flag;
		}
		EIGRP_FUNC_LEAVE(EigrpIpExteriorDiffer);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpIpExteriorDiffer);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpIpExteriorCheck

	Desc:	This function is to see if the exterior bit has changed state, and also update the
			routing table if necessary.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destination network entry
			flag		- pointer to the exterior bit of the feasible successors
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpExteriorCheck(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, S8 *flag)
{
	EIGRP_FUNC_ENTER(EigrpIpExteriorCheck);
	if(!EigrpIpExteriorDiffer(dndb, flag)){
		EIGRP_FUNC_LEAVE(EigrpIpExteriorCheck);
		return;
	}
	EIGRP_FUNC_LEAVE(EigrpIpExteriorCheck);
	
	return;
}

/************************************************************************************

	Name:	EigrpIpCompareExternal

	Desc:	This function is to do a protocol-dependent check to compare EXTERIOR_TYPE data 
			structure
		
	Para: 	data1	- pointer to one external data
			data2	- pointer to the other external data
	
	Ret:		TRUE	for the two data are equal ,or the are both NULL
			FALSE	for otherwise
************************************************************************************/

S32	EigrpIpCompareExternal(void *data1, void *data2)
{
	EIGRP_FUNC_ENTER(EigrpIpCompareExternal);

	if(data1 == NULL && data2 == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpCompareExternal);
		return TRUE;
	}
	if(data1 == NULL || data2 == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpCompareExternal);
		return FALSE;
	}
	EIGRP_FUNC_LEAVE(EigrpIpCompareExternal);

	return (!EigrpPortMemCmp((U8 *)data1, (U8 *)data2, sizeof(EigrpExtData_st)));
}

/************************************************************************************

	Name:	EigrpIpPeerIsFlowReady

	Desc:	This function returns TRUE if the peer is flow-control ready, meaning that a new
			unicast packet for this peer would be eligible to be transmitted immediately (ignoring
			the pacing timer).  This is the case only if both the multicast and unicast queues are
			empty.  This will be the case only if the interface is completely quiescent, or if a 
			single multicast was transmitted and the peer in question has already acknowledged 
			it(other peers may not have).

			If this routine returns false, it is guaranteed that a transmit_done callback will
			happen at some point in the future.
		
	Para: 	peer		- pointer to the given peer
	
	Ret:		TRUE	for the peer is flow-control ready
			FALSe	for otherwise
************************************************************************************/

S32	EigrpIpPeerIsFlowReady(EigrpDualPeer_st *peer)
{
	return(!(peer->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head ||
				peer->iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head || peer->flagGoingDown));
}

/************************************************************************************

	Name:	EigrpIpTransportReady

	Desc:	This function is  to be called when the transport is ready.

			peer is non-null if a peer has finished with the packet, or null if an interface has
			finished with the packet.

			If the refCnt in the packet descriptor is zero, the packet is completely acknowledged.
			We don't actually care about any of this;  we just try to send packets if things are 
			flow-ready.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- the interface on which the packet will be sent
			pktDesc	- pointer to the packet descriptor
			peer		- pointer to the peer the packet will be sent to 
			others_pending	- the sign of some one else wants to send the packet
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpTransportReady(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc, EigrpDualPeer_st *peer, S32 others_pending)
{
	EIGRP_FUNC_ENTER(EigrpIpTransportReady);
	/* Bail if someone else wants to send;  we defer until it's quiet. */
	if(others_pending) {
		EIGRP_FUNC_LEAVE(EigrpIpTransportReady);
		return;
	}

	/* If we're sending peer probes and this peer is flow-ready, send one. */
	if(peer && peer->sndProbeSeq && EigrpIpPeerIsFlowReady(peer)){
		pktDesc = EigrpEnqueuePak(ddb, peer, iidb, TRUE);
		if(pktDesc){
			pktDesc->opcode		= EIGRP_OPC_PROBE;
			pktDesc->serNoStart	= peer->sndProbeSeq;
			pktDesc->serNoEnd	= peer->sndProbeSeq;
			peer->sndProbeSeq++;
			EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
		}
	}

	/* If we're sending interface probes and this interface is flow-ready, send one. */
	if(iidb->sndProbeSeq && EigrpIntfIsFlowReady(iidb)){
		pktDesc = EigrpEnqueuePak(ddb, NULL, iidb, TRUE);
		if(pktDesc){
			pktDesc->opcode		= EIGRP_OPC_PROBE;
			pktDesc->serNoStart	= iidb->sndProbeSeq;
			pktDesc->serNoEnd	= iidb->sndProbeSeq;
			iidb->sndProbeSeq++;
			EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIpTransportReady);

	return;
}

/************************************************************************************

	Name:	EigrpIpItemSize

	Desc:	This function returns the size of the packet item that would be necessary to carry the
			supplied route.  We return the worst-case value, since the item may change from
			internal to external between packetization and transmission.
		
	Para: 	dndb	- pointer to the destination network entry
	
	Ret:		size of the packet item
************************************************************************************/

U32	EigrpIpItemSize(EigrpDualNdb_st *dndb)
{
	U32 dest_size, itemSize;
	
	EIGRP_FUNC_ENTER(EigrpIpItemSize);
	dest_size = 1 + EigrpIpBytesInMask(dndb->dest.mask);
	itemSize = MAX(EIGRP_EXTERN_TYPE_SIZE, EIGRP_METRIC_TYPE_SIZE) + dest_size;
	EIGRP_FUNC_LEAVE(EigrpIpItemSize);
	
	return(itemSize);
}

/************************************************************************************

	Name:	EigrpIpPdmTimerExpiry

	Desc:	This function is to handle the expiration of the PDM timer.  For us, this means that
			it's time to run the ager.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			timer	- pointer to the PDM timer
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpPdmTimerExpiry(EigrpDualDdb_st *ddb, EigrpMgdTimer_st *timer)
{
	EigrpPdb_st *pdb;
	
	EIGRP_FUNC_ENTER(EigrpIpPdmTimerExpiry);
	EigrpUtilMgdTimerStop(timer);
	pdb = ddb->pdb;
	EIGRP_FUNC_LEAVE(EigrpIpPdmTimerExpiry);

	return;
}
  
/************************************************************************************

	Name:	EigrpIpDdbInit

	Desc:	This function is to initialize the ddb data structure.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		TRUE	for success
			FALSE	for otherwise
************************************************************************************/

S32	EigrpIpDdbInit(EigrpPdb_st *pdb)
{
	EigrpDualDdb_st *ddb;
	U32 ddbsize;
	
	EIGRP_FUNC_ENTER(EigrpIpDdbInit);
	ddbsize	= sizeof(EigrpDualDdb_st);
	ddb		= EigrpPortMemMalloc(ddbsize);
	if(ddb == NULL){
		EIGRP_FUNC_LEAVE(EigrpIpDdbInit);
		return FALSE;
	}

	EigrpUtilMemZero((void *) ddb, sizeof(EigrpDualDdb_st));

	if(!EigrpDualInitDdb(ddb, "IP-EIGRP Hello", sizeof(EigrpExtData_st), sizeof(EigrpWork_st))){
		EigrpPortMemFree(ddb);
		ddb = (EigrpDualDdb_st*) 0;
		EIGRP_FUNC_LEAVE(EigrpIpDdbInit);
		return FALSE;
	}

	ddb->pdb	= pdb;
	pdb->ddb	= ddb;

	ddb->name	= "IP-EIGRP";
	ddb->k1 = pdb->k1;
	ddb->k2 = pdb->k2;
	ddb->k3 = pdb->k3;
	ddb->k4 = pdb->k4;
	ddb->k5 = pdb->k5;
	ddb->maxHopCnt	= EIGRP_MAX_HOPS;
	ddb->asystem	= pdb->process;
	ddb->nullIidb		= EigrpCreateIidb(ddb, (void *) NULL);

	/*
	* Turn on split horizon by default
	*/
	ddb->activeTime		= EIGRP_DEF_ACTIVE_TIME;

	/* Set up the transfer vectors. */
	ddb->acceptPeer		= EigrpIpShouldAcceptPeer;
	ddb->itemAdd		= EigrpAddItem;
	ddb->addrCopy		= EigrpIpAddrCopy;
	ddb->addressMatch	= EigrpIpAddrMatch;
	ddb->advertiseRequest	= EigrpIpAdvertiseRequest;
	ddb->buildPkt			= EigrpBuildPacket;
	ddb->compareExt		= EigrpIpCompareExternal;
	ddb->exteriorCheck	= EigrpIpExteriorCheck;
	ddb->exteriorFlag		= EigrpIpExteriorFlag;
	ddb->headerPtr		= EigrpIpHeaderPtr;
	ddb->hellohook		= NULL;
	ddb->iidbCleanUp		= NULL;
	ddb->iidbShowDetail	= NULL;
	ddb->intfChg			= NULL;
	ddb->itemSize		= EigrpIpItemSize;
	ddb->localAddress		= EigrpIpLocalAddr;
	ddb->mmetricFudge	= EigrpIpGetOffset;
	ddb->ndbBucket		= EigrpIpNdbBucket;
	ddb->networkMatch	= EigrpIpNetMatch;
	ddb->pdmTimerExpiry = EigrpIpPdmTimerExpiry;
	ddb->peerBucket		= EigrpIpPeerBucket;
	ddb->peerShowHook	= NULL;
	ddb->printAddress		= EigrpIpPrintAddr;
	ddb->printNet			= EigrpIpPrintNetwork;
	ddb->rtDelete			= EigrpIpRtDelete;
	ddb->rtModify			= EigrpIpRtDelete;
	ddb->rtUpdate		= EigrpIpRtUpdate;
	ddb->sndPkt			= EigrpPortSendPacket;
	ddb->tansReady		= EigrpIpTransportReady;
	ddb->setMtu			= EigrpIpSetMtu;

	/* Other random gorp. */
	//屏蔽路由ID初始化
	//EigrpIpSetRouterId(ddb);
	ddb->routerId = EIGRP_IPADDR_ZERO;
	//
	ddb->flagLogEvent		= FALSE;
	ddb->pktOverHead	= EIGRP_IP_HDR_LEN;
	ddb->routes			= 0;

	/* Fire up the active timer. */
	/* Here replaced by threadAtciveTimer*/
	/* EigrpUtilMgdTimerStart(&ddb->activeTimer, EIGRP_ACTIVE_SCAN_FREQUENCY);*/
	/* Enqueue and we're off. */
	EigrpUtilQue2Enqueue(gpEigrp->ddbQue, (EigrpQueElem_st *)ddb);	
	EIGRP_FUNC_LEAVE(EigrpIpDdbInit);
	
	return TRUE;
}
/************************************************************************************

	Name:	EigrpIpStartAclTimer

	Desc:	This function is to (re)start the ACL change timer.  If the timer is already running,
			it is restarted.  This means that as long as the user enters at least one access list
			change every 10 seconds, the neighbors won't be cleared until they're all done.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			idb		- 
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpStartAclTimer(EigrpPdb_st *pdb, void *idb)
{
	EigrpDualDdb_st	*ddb;
	EigrpIdb_st	*iidb = NULL;
	
	EIGRP_FUNC_ENTER(EigrpIpStartAclTimer);
	ddb = pdb->ddb;
	if(!ddb) {
		EIGRP_FUNC_LEAVE(EigrpIpStartAclTimer);
		return;
	}

	if(idb){
		iidb = EigrpFindIidb(ddb, idb);
		if(!iidb){
			EIGRP_FUNC_LEAVE(EigrpIpStartAclTimer);
			return;
		}

		EigrpUtilMgdTimerStart(&iidb->aclChgTimer, EIGRP_ACL_CHANGE_DELAY);
	}else{
		EigrpUtilMgdTimerStart(&ddb->aclChgTimer, EIGRP_ACL_CHANGE_DELAY);
	}
	EIGRP_FUNC_LEAVE(EigrpIpStartAclTimer);

	return;
}

/************************************************************************************

	Name:	EigrpIpHopcountChanged

	Desc:	This function is to handle change in configured max hopcount.

			Update the DDB field.  Blast all of the adjacencies;  a brute force way of ridding
			ourselves of now-unreachable destinations.  It happens so seldom that we need not get
			fancy.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			hopcount	- the max hopcount
	
	Ret:		
************************************************************************************/

void	EigrpIpHopcountChanged(EigrpPdb_st *pdb, U8 hopcount)
{
	EigrpDualDdb_st *ddb;
	
	EIGRP_FUNC_ENTER(EigrpIpHopcountChanged);
	ddb = pdb->ddb;
	if(!ddb){
		EIGRP_FUNC_LEAVE(EigrpIpHopcountChanged);
		return;
	}

	ddb->maxHopCnt = hopcount;
	EigrpTakeAllNbrsDown(ddb, "Max hopcount changed");
	EIGRP_FUNC_LEAVE(EigrpIpHopcountChanged);

	return;
}

/************************************************************************************

	Name:	EigrpIpPdbInit

	Desc:	This function is to initialize the pdb data structure.
		
	Para: 	process		- the autonomous system number
	
	Ret:		pointer to the new pdb data structure
************************************************************************************/

EigrpPdb_st *EigrpIpPdbInit(U32 process)
{
	EigrpPdb_st *pdb;

	EIGRP_FUNC_ENTER(EigrpIpPdbInit);

	pdb = (EigrpPdb_st *)EigrpPortMemMalloc(sizeof(EigrpPdb_st));
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpIpPdbInit);	 	
		return NULL;
	}
	EigrpUtilMemZero((void *)pdb, sizeof(EigrpPdb_st));

	pdb->process	= process;
	(void)EigrpPortStrCpy(pdb->name, "EIGRP");
	(void)EigrpPortStrCpy(pdb->pname, "IP-EIGRP Router");
	pdb->sumAuto	= EIGRP_DEF_AUTOSUM;
	pdb->metricFlagDef	= FALSE; /* default metric is default */
	pdb->query	= EigrpIpEnqueueReloadIptableEvent;
	pdb->aclChanged		= EigrpIpStartAclTimer;
	pdb->hopCntChange	= EigrpIpHopcountChanged;
	pdb->cdOut	= TRUE;
	pdb->cdIn	= TRUE;
	pdb->rtInDef 						= EIGRP_DEF_DEFROUTE_IN;
	pdb->rtOutDef 					= EIGRP_DEF_DEFROUTE_OUT;
	pdb->bcastTime = pdb->bcastTimeDef	= 0;
	pdb->invalidTime = pdb->invalidTimeDef		= 0;
	pdb->holdTime = pdb->holdTimeDef			= 0;
	pdb->flushTime = pdb->flushTimeDef			= 0;
	pdb->multiPath = pdb->multiPathDef			= EIGRP_DEF_MAX_ROUTES;
	pdb->distance = pdb->distDef			= EIGRP_DEF_DISTANCE_INTERNAL;
	pdb->distance2 = pdb->dist2Def		= EIGRP_DEF_DISTANCE_EXTERNAL;
	pdb->variance = EIGRP_DEF_VARIANCE;

	pdb->k1 = EIGRP_K1_DEFAULT;
	pdb->k2 = EIGRP_K2_DEFAULT;
	pdb->k3 = EIGRP_K3_DEFAULT;
	pdb->k4 = EIGRP_K4_DEFAULT;
	pdb->k5 = EIGRP_K5_DEFAULT;
	pdb->maxHop = pdb->maxHopDef = EIGRP_MAX_HOPS;

	pdb->netLst.next	= &pdb->netLst;
	pdb->netLst.back	= &pdb->netLst;

	/* Init the Default Metric	 */
	pdb->vMetricDef.bandwidth	= 20000;
	pdb->vMetricDef.delay			= 1;
	pdb->vMetricDef.reliability		= 255;
	pdb->vMetricDef.load			= 1;
	pdb->vMetricDef.mtu			= 1500;

	/* Initialize random stuff. */
	pdb->nbrLst			= NULL;

	/* EIGRP_TIMER_STOP(pdb->sleep_timer); */
	if(!EigrpIpDdbInit(pdb)){
		EigrpPortMemFree(pdb);
		pdb = (EigrpPdb_st*) 0;
		EIGRP_FUNC_LEAVE(EigrpIpPdbInit);
		return pdb;
	}

	/* create pdb work queue */
	if(EigrpUtilQueCreate(EIGRP_PDB_WORK_QUE_LEN, &(pdb->workQueId))){
		EigrpPortMemFree(pdb);
		pdb = (EigrpPdb_st*) 0;
		EIGRP_FUNC_LEAVE(EigrpIpPdbInit);
		return pdb;
	}

	/* create pdb command queue */
	if(EigrpUtilQueCreate(EIGRP_PDB_CMD_QUE_LEN, &(pdb->cmdQueId))){
		EigrpPortMemFree(pdb);
		pdb = (EigrpPdb_st*) 0;
		EIGRP_FUNC_LEAVE(EigrpIpPdbInit);
		return pdb;
	}

	pdb->rtTbl = EigrpUtilRtTreeCreate();

	pdb->sumQue	= EigrpUtilQue2Init();

	EigrpUtilQue2Enqueue(gpEigrp->protoQue, (EigrpQueElem_st *)pdb);
	EIGRP_FUNC_LEAVE(EigrpIpPdbInit);

	return pdb;
}

/************************************************************************************

	Name:	EigrpIpSummarylistCleanup

	Desc:	This function is to clean up a given summary list.
		
	Para: 	sum_list		- pointer to the given summary list
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpSummarylistCleanup(EigrpQue_st *sum_list)
{
	EigrpSum_st	*summary;
	EigrpSumIdb_st	*sum_idb;
	
	EIGRP_FUNC_ENTER(EigrpIpSummarylistCleanup);
	while((summary = (EigrpSum_st *) EigrpUtilQue2Dequeue(sum_list))){
		while(summary->idbQue->head){
			sum_idb = (EigrpSumIdb_st *)EigrpUtilQue2Dequeue(summary->idbQue);

			EigrpPortMemFree(sum_idb);
		}
		EigrpPortMemFree(summary);
	}
	EIGRP_FUNC_LEAVE(EigrpIpSummarylistCleanup);

	return;
}

/************************************************************************************

	Name:	EigrpIpIfapDown

	Desc:	This function is to simulate an ifap down event to clear the topology table.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpIfapDown(EigrpPdb_st *pdb)
{
	EigrpIdb_st	*iidb;
	EigrpDualDdb_st	*ddb;
	EigrpIntf_pt	pEigrpIntf;
	EigrpIntfAddr_pt	pAddr;
	
	EIGRP_FUNC_ENTER(EigrpIpIfapDown);
	ddb = pdb->ddb;
	if(!ddb){
		EIGRP_FUNC_LEAVE(EigrpIpIfapDown);
		return;
	}

	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
			/* If it is already inactive, we dont process it. */
			continue;
		}
		/* 1.remove routes */
		for(pAddr = EigrpPortGetFirstAddr(pEigrpIntf->sysCirc); pAddr; pAddr = EigrpPortGetNextAddr(pAddr)){
			if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				if(EigrpIpConnAddressActivated(pdb, pAddr->ipDstAddr, pAddr->ipMask)){
					EigrpIpEnqueueConnRtchangeEvent(pdb, pEigrpIntf, (pAddr->ipAddr & pAddr->ipMask), pAddr->ipMask,  FALSE, TRUE);
				}
			}else{
				if(EigrpIpConnAddressActivated(pdb, pAddr->ipAddr, pAddr->ipMask)){
					EigrpIpEnqueueConnRtchangeEvent(pdb, pEigrpIntf, (pAddr->ipAddr & pAddr->ipMask), pAddr->ipMask,  FALSE, TRUE);
				}
			}
		}

		/* 2.remove interface */
		iidb = EigrpFindIidb(ddb, pEigrpIntf);
		if(iidb){	/* They're all gone now. */
			EigrpIpOnoffIdb(ddb, pEigrpIntf, FALSE, FALSE);
		}

		/* 3.remove neighbor */
		EigrpTakeNbrsDown(ddb, pEigrpIntf, TRUE, "address changed");
	}
	EIGRP_FUNC_LEAVE(EigrpIpIfapDown);

	return;
}

/************************************************************************************

	Name:	EigrpIpCleanup

	Desc:	This function is to do the cleanup after an EIGRP router has been shutdown. Close
			socket and release pdb storage.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpCleanup(EigrpPdb_st *pdb)
{
	EigrpDualDdb_st	*ddb;
	EigrpWork_st	*work;
	EigrpNeiEntry_st	*neighbor;
	U32		msgnum, msgbuf[4], i;
	
	EIGRP_FUNC_ENTER(EigrpIpCleanup);
	
	ddb = pdb->ddb;

	/* cleanup the pdb redistribute list */
	EigrpIpRedisListCleanup(&pdb->redisLst);
	
	while(pdb->nbrLst){   
		neighbor = pdb->nbrLst;
		pdb->nbrLst = neighbor->next;
		EigrpPortMemFree(neighbor);
	}

	/* cleanup the redistribute routes. */
	EigrpIpRedistributeRoutesAllCleanup(pdb);

	/* we simulate ifap down to clear topology table */
	EigrpIpIfapDown(pdb);

	/* delete the pdb event job */
	/* we should deal with the work by myself and delete the job */
	if(pdb->eventJob){
		EigrpIpWorkqFlush(pdb);
		EigrpUtilSchedCancel(pdb->eventJob);
		pdb->eventJob = 0;
	}

	/* cleanup the pdb cmdQ */
	(void)EigrpUtilQueGetMsgNum(pdb->cmdQueId, &msgnum);
	for(i = 0; i < msgnum; i++){
		(void)EigrpUtilQueFetchElem(pdb->cmdQueId, msgbuf);
		work = (EigrpWork_st*) msgbuf[0];
		EigrpUtilChunkFree(ddb->workQueChunk, work);
	}

	(void)EigrpUtilQueDelete(pdb->cmdQueId);

	/* cleanup the pdb workQ */
	/* I think this will be empty because of the ipeigrp_prcoess_workq(); */
	(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
	for(i = 0; i < msgnum; i++){
		(void)EigrpUtilQueFetchElem(pdb->workQueId, msgbuf);
		work = (EigrpWork_st*) msgbuf[0];
		EigrpUtilChunkFree(ddb->workQueChunk, work);
	}

	/* delete_watched_queue(&pdb->workQueId); */
	(void)EigrpUtilQueDelete(pdb->workQueId);

	EigrpIpNetworkListCleanup(&pdb->netLst);
	EigrpFreeDdb(ddb);
	EigrpIpSummarylistCleanup(pdb->sumQue);

	/* cleanup the pdb */
	EigrpUtilQue2Unqueue(gpEigrp->protoQue, (EigrpQueElem_st *)pdb);

	EigrpUtilRtTreeDestroy(pdb->rtTbl);
	EigrpPortMemFree(pdb);
	pdb	=(EigrpPdb_st*) 0;
	EIGRP_FUNC_LEAVE(EigrpIpCleanup);

	return;
}

/************************************************************************************

	Name:	EigrpIpProcessMaxActiveTimeCommand

	Desc:	This function is process eigrp max active time command.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			minutes	- the eigrp max active time
			sense	- 
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpProcessMaxActiveTimeCommand(EigrpPdb_st *pdb, U32 minutes, S32 sense)
{
	EIGRP_FUNC_ENTER(EigrpIpProcessMaxActiveTimeCommand);
	
	if((pdb == NULL) || (pdb->ddb == NULL)) {        /* Let's make sure */
		EIGRP_FUNC_LEAVE(EigrpIpProcessMaxActiveTimeCommand);
		return;
	}

	if(sense){
		pdb->ddb->activeTime = minutes * EIGRP_MSEC_PER_MIN;
	}else{
		pdb->ddb->activeTime = EIGRP_DEF_ACTIVE_TIME;
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessMaxActiveTimeCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpResetCommand

	Desc:	This function is to process eigrp reset command.

			The eigrp process will be terminated and then restarted.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpResetCommand(EigrpPdb_st *pdb)
{
	U32 process;
	
	EIGRP_FUNC_ENTER(EigrpIpResetCommand);
	/* Make sure ! */
	if((pdb == NULL) || (pdb->process == 0)){
		EIGRP_FUNC_LEAVE(EigrpIpResetCommand);
		return;
	}

	/* save as-number */
	process = pdb->process;

	/* close the process, all config and all data will lost! */
	EigrpIpCleanup(pdb);

	/* Open it again! */
	EigrpIpPdbInit(process);
	EIGRP_FUNC_LEAVE(EigrpIpResetCommand);

	return;
}

/************************************************************************************

	Name:	EigrpIpInetMask

	Desc:	This function is to generate the natural net mask, given the ip address.
		
	Para: 	x	- pointer to the given ip address
	
	Ret:		the natural net mask of the given ip address
************************************************************************************/

U8 EigrpIpInetMask(U32  x)
{
	U32	*y;
	EIGRP_FUNC_ENTER(EigrpIpInetMask);
	x	= x >> 24;
	y	= &x;
	
	if((*y) <= 191){
		if((*y) < 127){
			if((*y) <= 63){
				if((*y) > 0){       
					/* 1-63 */
					EIGRP_FUNC_LEAVE(EigrpIpInetMask);
					return EIGRP_INET_MASK_CLASSA;
				}else{	/* 0 */
					EIGRP_FUNC_LEAVE(EigrpIpInetMask);
					return EIGRP_INET_MASK_DEFAULT;
				}
			}else{	/* 64-126 */
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_CLASSA;
			}
		}else{
			if((*y) > 127){       	/* 128-191 */
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_CLASSB;
			}else{		/* 127 */
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_CLASSA;
			}
		}
	}else{
		if((*y) <= 223){
			if((*y) <= 207){       /* 192-207 */
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_CLASSC;
			}else{		/* 208-223 */
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_CLASSC;
			}
		}else{
			if((*y) <= 239 ){       	/* 224-239*/
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_CLASSD;
			}else{		/* 240-255 */
				EIGRP_FUNC_LEAVE(EigrpIpInetMask);
				return EIGRP_INET_MASK_INVALID;
			}
		}
	}
}

/************************************************************************************

	Name:	EigrpIpInetFlags

	Desc:	
		
	Para: 
	
	Ret:		
************************************************************************************/

U8 EigrpIpInetFlags(U32 x)
{
	U8 *y;
	EIGRP_FUNC_ENTER(EigrpIpInetFlags);
	x	= x >> 24;
	y	= (U8 *)(&x);

	if((*y) <= 191){
		if((*y) < 127){
			if((*y) <= 63){
				if((*y) > 0){        /* 1-63 */
					EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
					return EIGRP_INET_CLASSF_NETWORK;
				}else{	/* 0 */
					EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
					return EIGRP_INET_CLASSF_NETWORK | EIGRP_INET_CLASSF_DEFAULT;
				}
			}else{	/* 64-126 */
#ifdef	EIGRP_INET_CLASS_A_SHARP
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_EXPERIMENTAL;
#else	/* EIGRP_INET_CLASS_A_SHARP */
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_NETWORK;
#endif	/* EIGRP_INET_CLASS_A_SHARP */

			}
		}else{
			if((*y) > 127){       	/* 128-191*/
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_NETWORK;
			}else{		/* 127 */
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_LOOPBACK;
			}
		}
	}else{
		if((*y) <= 223){
			if((*y) <= 207){        /* 192-207 */
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_NETWORK;
			}else{		/* 208-223	*/
#ifdef	EIGRP_INET_CLASS_C_SHARP
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_NETWORK;
#else	/* EIGRP_INET_CLASS_C_SHARP */
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_NETWORK;
#endif	/* EIGRP_INET_CLASS_C_SHARP */
			}
		}else{
			if((*y) <= 239 ){       	/* 224-239*/
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_MULTICAST;
			}else{		/* 240-255 */
#ifdef	EIGRP_INET_CLASS_E
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_NETWORK;
#else	/* EIGRP_INET_CLASS_E */
				EIGRP_FUNC_LEAVE(EigrpIpInetFlags);
				return EIGRP_INET_CLASSF_RESERVED;
#endif	/* EIGRP_INET_CLASS_E */
			}
		}
	}
}

/************************************************************************************

	Name:	EigrpIpProcessWorkq

	Desc:	This function is to process the work queue, and process the managed timers.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		0
************************************************************************************/

S32	EigrpIpProcessWorkq (EigrpPdb_st *pdb)
{
	EigrpWork_st *work;
	U32 workq_limiter = EIGRP_WORK_QUE_LIMIT;
	U32 msgnum, msgbuf[4];
	
	EIGRP_FUNC_ENTER(EigrpIpProcessWorkq);

	/* process event queue, max 20 events each time */
	(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asyread workQ num :%d\n", msgnum);
	while(msgnum){
		workq_limiter--;
		if(!workq_limiter){
			break;
		}

		(void)EigrpUtilQueFetchElem(pdb->workQueId, msgbuf);
		work = (EigrpWork_st*) msgbuf[0];
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asyread:%d, work-type:%d\n",
					msgbuf[0], work->type);
		switch(work->type){
			case EIGRP_WORK_IFC_ADD_EVENT:
				EigrpIpProcessIfAddEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_DELETE_EVENT:
				EigrpIpProcessIfDeleteEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_UP_EVENT:
				EigrpIpProcessIfUpEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_DOWN_EVENT:
				EigrpIpProcessIfDownEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_ADDRESS_ADD_EVENT:
				EigrpIpProcessIfAddressAddEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_ADDRESS_DELETE_EVENT:
				EigrpIpProcessIfAddressDeleteEvent(pdb, work);
				break;
				
			case EIGRP_WORK_ZAP_IF_EVENT:
				EigrpIpProcessZapIfEvent(pdb, work);
				break;
				
			case EIGRP_WORK_REDIST_CONNECT:
				EigrpIpProcessConnAddressEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IF_DOWN_EVENT:
				EigrpIpProcessIfdownEvent(pdb, work);
				break;
				
			case EIGRP_WORK_LOST:
				EigrpIpProcessLost(pdb, work);
				break;
				
			case EIGRP_WORK_BACKUP:
				EigrpIpProcessBackup(pdb, work);
				break;
				
			case EIGRP_WORK_CONNSTATE:
				EigrpIpProcessConnChangeEvent(pdb, work);
				break;
				
			case EIGRP_WORK_REDISTALL:
				EigrpIpProcessRedisEntryEvent(pdb, work);
				break;
				
			case EIGRP_WORK_REDIS_DELETE:
			/* eigrp_process_redis_delete_event(pdb, work); */
				break;
			
			case EIGRP_WORK_SUMMARY:
				EigrpIpProcessSummaryEntryEvent(pdb, work);
				break;
				
			case EIGRP_WORK_TABLERELOAD:
				EigrpIpProcessReloadIptableEvent(pdb);
				break;
				
			case EIGRP_WORK_INTEXTSTATE:
			/* EigrpIpProcessExteriorChange(pdb, work); */
				break;
			
			case EIGRP_WORK_SENDSAP:       	/* Not used in IP */
				break;			/* do nothing */

			default:
				break;
		}
		EigrpUtilChunkFree(pdb->ddb->workQueChunk, work);
		EigrpDualFinishUpdateSend(pdb->ddb);

		(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asyread workQ num :%d\n", msgnum);
	}

	/* process command queue, just one command each time. and only when event queue is empty we
	  * process command queue. */

	/* process EigrpMgdTimer_st tree */
	/* job complete and clear job. */
	pdb->eventJob = NULL;

	/*never reach here */
	(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
	if(msgnum){
		EigrpIpLaunchPdbJob(pdb);
	}
	EIGRP_FUNC_LEAVE(EigrpIpProcessWorkq);

	return 0;
}

/************************************************************************************

	Name:	EigrpIpWorkqFlush

	Desc:	This function is to flush the work queue.
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		NONE
************************************************************************************/

void	EigrpIpWorkqFlush(EigrpPdb_st *pdb)
{
	EigrpWork_st *work;
	U32 msgnum, msgbuf[4];
	
	EIGRP_FUNC_ENTER(EigrpIpWorkqFlush);

	(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asyread Message Num :%d\n", msgnum);
	while(msgnum){
		(void)EigrpUtilQueFetchElem(pdb->workQueId, msgbuf);
		work = (EigrpWork_st*) msgbuf[0];
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asyread: 0x%X, work-type:%d\n",msgbuf[0], work->type);
		switch(work->type){
			case EIGRP_WORK_IFC_ADD_EVENT:
				EigrpIpProcessIfAddEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_DELETE_EVENT:
				EigrpIpProcessIfDeleteEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_UP_EVENT:
				EigrpIpProcessIfUpEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_DOWN_EVENT:
				EigrpIpProcessIfDownEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_ADDRESS_ADD_EVENT:
				EigrpIpProcessIfAddressAddEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IFC_ADDRESS_DELETE_EVENT:
				EigrpIpProcessIfAddressDeleteEvent(pdb, work);
				break;
				
			case EIGRP_WORK_ZAP_IF_EVENT:
				EigrpIpProcessZapIfEvent(pdb, work);
				break;
		
			case EIGRP_WORK_REDIST_CONNECT:
				EigrpIpProcessConnAddressEvent(pdb, work);
				break;
				
			case EIGRP_WORK_IF_DOWN_EVENT:
				EigrpIpProcessIfdownEvent(pdb, work);
				break;
				
			case EIGRP_WORK_LOST:
				EigrpIpProcessLost(pdb, work);
				break;
				
			case EIGRP_WORK_BACKUP:
				EigrpIpProcessBackup(pdb, work);
				break;
				
			case EIGRP_WORK_CONNSTATE:
				EigrpIpProcessConnChangeEvent(pdb, work);
				break;
				
			case EIGRP_WORK_REDISTALL:
				EigrpIpProcessRedisEntryEvent(pdb, work);
				break;
				
			case EIGRP_WORK_REDIS_DELETE:
				/* eigrp_process_redis_delete_event(pdb, work); */
				break;
				
			case EIGRP_WORK_SUMMARY:
				EigrpIpProcessSummaryEntryEvent(pdb, work);
				break;
				
			case EIGRP_WORK_TABLERELOAD:
				EigrpIpProcessReloadIptableEvent(pdb);
				break;
				
			case EIGRP_WORK_INTEXTSTATE:
				/* EigrpIpProcessExteriorChange(pdb, work); */
				break;
				
			case EIGRP_WORK_SENDSAP:       	/* Not used in IP */
				break;			/* do nothing */
		}
		EigrpUtilChunkFree(pdb->ddb->workQueChunk, work);
		EigrpDualFinishUpdateSend(pdb->ddb);

		(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Asyread Message Num :%d\n", msgnum);
	}
	EIGRP_FUNC_LEAVE(EigrpIpWorkqFlush);
	return;
}
