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

	Module:	EigrpIntf

	Name:	EigrpIntf.c

	Desc:	

	Rev:	
	
***********************************************************************************/
//#include	"../config.h"
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

/************************************************************************************

	Name:	EigrpIntfClean

	Desc:	This function is to clean up the eigrp interface list.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpIntfClean()
{
	EigrpIntf_pt	pTem, pNext;

	EIGRP_FUNC_ENTER(EigrpIntfClean);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pNext){
		pNext = pTem->next;
		EigrpPortMemFree(pTem);
	}

	gpEigrp->intfLst	= NULL;
	EIGRP_FUNC_LEAVE(EigrpIntfClean);

	return;
}

/************************************************************************************

	Name:	EigrpIntfInit

	Desc:	This function is to init the eigrp interface list and get the existing interface list
			from the system.
		
	Para: 	NONE	
	
	Ret:		NONE
************************************************************************************/

void	EigrpIntfInit()
{
	EIGRP_FUNC_ENTER(EigrpIntfInit);
	EIGRP_ASSERT(!gpEigrp->intfLst);
	EigrpPortGetAllSysIntf();
	EIGRP_FUNC_LEAVE(EigrpIntfInit);

	return;
}

/************************************************************************************

	Name:	EigrpIntfFindByName

	Desc:	This function is to try to find the interface data in the eigrp interface list by the
			given interface name and return a point which points to it.
 
 			If nothing is found, a zero point will be returned.
		
	Para: 	name	- pointer to the eigrp interface name
	
	Ret:		pointer to the eigrp interface
************************************************************************************/

EigrpIntf_pt	EigrpIntfFindByName(char *name)
{
	EigrpIntf_pt	pTem;

	EIGRP_FUNC_ENTER(EigrpIntfFindByName);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(!EigrpPortStrCmp(pTem->name, name)){
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByName);
	
	return pTem;
}

#ifndef _EIGRP_PLAT_MODULE
void *EigrpIntfFindByName_ex(char *name)
{
	EigrpIntf_pt	pIf;

	pIf = EigrpIntfFindByName(name);
	if(pIf){
		return	pIf->sysCirc;
	}

	return	NULL;
}
#endif// _EIGRP_PLAT_MODULE
/************************************************************************************

	Name:	EigrpIntfFindByIndex

	Desc:	This function is to try to find the interface data in the eigrp interface list by the
			given index and return a pointer which points to it.
 
 			If nothing is found, a zero point will be returned.
		
	Para: 	index	- the interface index
	
	Ret:		pointer to the eigrp interface
************************************************************************************/

EigrpIntf_pt	EigrpIntfFindByIndex(U32 index)
{
	EigrpIntf_pt	pTem;

	EIGRP_FUNC_ENTER(EigrpIntfFindByIndex);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(pTem->ifindex == index){
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByIndex);

	return pTem;
}

/************************************************************************************

	Name:	EigrpIntfFindByAddr

	Desc:	This function is to try to find the interface data in the eigrp interface list by the 
			given ip address and return a point which points to it.
		
	Para: 	ipAddr		- the given ip address
	
	Ret:		pointer to the eigrp interface
************************************************************************************/

EigrpIntf_pt	EigrpIntfFindByAddr(U32 ipAddr)
{
	EigrpIntf_pt	pTem;

	EIGRP_FUNC_ENTER(EigrpIntfFindByAddr);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(pTem->ipAddr == ipAddr){
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByAddr);

	return pTem;
}

/*寻找eigrp接口列表中信道地址与ipnet相同的，有返回接口，否则NULL*/
EigrpIntf_pt	EigrpIntfFindByDestNet(U32 ipNet)
{
	EigrpIntf_pt	pTem;

	EIGRP_FUNC_ENTER(EigrpIntfFindByAddr);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		/*主用户口地址不处理*/
		if(pTem->ipAddr == EigrpPortGetRouterId())
			continue;
#ifdef _EIGRP_PLAT_MODULE
		/*无编号信道地址不处理*/
		if(BIT_TEST(pTem->active, EIGRP_INTF_FLAG_UNNUMBERED))
			continue;
#else		
		/*无编号信道地址不处理*/
		if(0 == EigrpPortMemCmp(pTem->name, "uai", 3))
			continue;
#endif
		/*有编号信道*/
		if((pTem->ipAddr & pTem->ipMask) == ipNet){/*信道接口网段与拓扑update目的网段相同*/
				break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByAddr);

	return pTem;
}


U8 EigrpIntfAddrLocal(U32 ipAddr)
{
	EigrpIntf_pt	pTem;

	EIGRP_FUNC_ENTER(EigrpIntfFindByAddr);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(pTem->ipAddr == ipAddr){
			return	TRUE;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByAddr);

	return FALSE;
}

/************************************************************************************

	Name:	EigrpIntfFindByPeer

	Desc:	This function is to try to find an interface in the eigrp interface list which is on 
			the same segment of the given ip address from the interface's view.
	
	Para: 	ipAddr	- the given ip address
	
	Ret:		pointer to the found interface
************************************************************************************/

EigrpIntf_pt	EigrpIntfFindByPeer(U32 ipAddr)
{
	EigrpIntf_pt	pTem, pFound;

	pFound	= NULL;
	EIGRP_FUNC_ENTER(EigrpIntfFindByPeer);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(BIT_TEST(pTem->flags, EIGRP_INTF_FLAG_POINT2POINT)){
				if(EigrpPortCircIsUnnumbered(pTem->sysCirc) && !pTem->remoteIpAddr){/* tigerwh 120517 */
					if(!pFound){
						pFound	= pTem;	/* tigerwh 120525 */
					}
					/* Dont break here , to find better interface whose remote ip address matches the given ipaddr */
				}else if(pTem->remoteIpAddr == ipAddr){
					pFound	= pTem;
					break;
				}

				continue;
		}else if(!BIT_TEST(pTem->flags, EIGRP_INTF_FLAG_POINT2POINT)
				&& (pTem->ipAddr) && (pTem->ipMask)
				&& (pTem->ipAddr & pTem->ipMask) == (ipAddr & pTem->ipMask)){
			pFound	= pTem;
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByPeer);
		
	return pTem;
}

EigrpIntf_pt	EigrpIntfFindByNetAndMask(U32 net, U32 ipMask)
{
	EigrpIntf_pt	pTem;
	
	EIGRP_FUNC_ENTER(EigrpIntfFindByNetAndMask);

	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(BIT_TEST(pTem->flags, EIGRP_INTF_FLAG_POINT2POINT) && 
					((pTem->remoteIpAddr & ipMask) ==(net & ipMask))){
			break;
		}else if(!BIT_TEST(pTem->flags, EIGRP_INTF_FLAG_POINT2POINT) 
				&& (pTem->ipAddr) && (pTem->ipMask)&&
					(pTem->ipAddr & pTem->ipMask) == (net & pTem->ipMask)){
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByNetAndMask);
		
	return pTem;
}
#ifdef _EIGRP_PLAT_MODULE
//根据接口地址和掩码设置激活这接口还是禁制这个接口
int EigrpIntfActiveByNetAndMask(U32 net, U32 ipMask, int type)
{
	EigrpIntf_pt	pTem;
	
	EIGRP_FUNC_ENTER(EigrpIntfFindByNetAndMask);

	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(BIT_TEST(pTem->flags, EIGRP_INTF_FLAG_POINT2POINT) && 
					((pTem->remoteIpAddr & ipMask) ==(net & ipMask))){
			if(type == TRUE)
				BIT_SET(pTem->active, EIGRP_INTF_FLAG_ACTIVE);
			else
				BIT_RESET(pTem->active, EIGRP_INTF_FLAG_ACTIVE);
			break;
		}else if(!BIT_TEST(pTem->flags, EIGRP_INTF_FLAG_POINT2POINT) 
				&& (pTem->ipAddr) && (pTem->ipMask)&&
					(pTem->ipAddr & pTem->ipMask) == (net & pTem->ipMask)){
			if(type == TRUE)
				BIT_SET(pTem->active, EIGRP_INTF_FLAG_ACTIVE);
			else
				BIT_RESET(pTem->active, EIGRP_INTF_FLAG_ACTIVE);
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIntfFindByNetAndMask);
		
	return 0;
}
#endif
/************************************************************************************

	Name:	EigrpIntfDel

	Desc:	This function is remove an interface from the eigrp interface list.

			The caller should make sure the given interface is on the eigrp interface list.
		
	Para: 	pIntf		- pointer to the interface which is to be deletd
	
	Ret:		NONE
************************************************************************************/

void	EigrpIntfDel(EigrpIntf_pt pIntf)
{
	EigrpIntf_pt	pTem;

	EIGRP_FUNC_ENTER(EigrpIntfDel);
	pTem	= NULL;	
	if(gpEigrp->intfLst == pIntf){
		gpEigrp->intfLst = pIntf->next;
	}else{
		for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
			if(pTem->next == pIntf){
				break;
			}
		}
		EIGRP_ASSERT((U32)pTem);
		if(!pTem){	
#ifdef _EIGRP_PLAT_MODULE
			if(BIT_TEST(pTem->active, EIGRP_INTF_FLAG_ACTIVE))
			{
				EIGRP_TRC(DEBUG_EIGRP_OTHER,"EIGRP: Can't delete this interface of:%s\n",pTem->name);
				return;
			}
#endif //_EIGRP_PLAT_MODULE
			EigrpPortMemFree(pIntf);
			EIGRP_FUNC_LEAVE(EigrpIntfDel);
			return;
		}
		pTem->next= pIntf->next;
	}

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS	
	EigrpPortDelZebraIfp(pIntf->sysCirc);//在本地链表删除一个接口
	pIntf->sysCirc	= NULL;
#endif//EIGRP_PLAT_ZEBOS	
#ifdef EIGRP_PLAT_ZEBRA	
	zebraEigrpIntfDel(pIntf);//在本地链表删除一个接口
	pIntf->sysCirc	= NULL;
#endif//EIGRP_PLAT_ZEBRA	
#endif
	EigrpPortMemFree(pIntf);
	EIGRP_FUNC_LEAVE(EigrpIntfDel);
	
	return;
}

/************************************************************************************

	Name:	EigrpIntfAdd

	Desc:	This function is to insert a new eigrp interface into the eigrp interface list.
		
	Para: 	pIntf		- pointer to the eigrp interface which is to be added
	
	Ret:		NONE
************************************************************************************/

void	EigrpIntfAdd(EigrpIntf_pt pIntf)
{
	EigrpIntf_pt pTem;

	EIGRP_FUNC_ENTER(EigrpIntfAdd);
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		if(pTem == pIntf){
			EIGRP_FUNC_LEAVE(EigrpIntfAdd);
			return;	/* this interface has already exist in this list. */
		}
	}
	pIntf->next	= gpEigrp->intfLst;
	gpEigrp->intfLst = pIntf;
	EIGRP_FUNC_LEAVE(EigrpIntfAdd);
	
	return;
}

/************************************************************************************

	Name:	EigrpSysIntfAdd_ex

	Desc:	This function is to collect the infomation of a given system interface, create a new
			eigrp interface and insert it into the eigrp interface list.
		
	Para: 	sysIntf	- pointer to the given system interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpSysIntfAdd_ex(void *sysIntf)		
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpPdb_st	*pdb;

	if(sysIntf == NULL){
		return;
	}
  
	pEigrpIntf = EigrpPortMemMalloc(sizeof(EigrpIntf_st));
	if(!pEigrpIntf){	
		return;
	}
	EigrpPortCopyIntfInfo(pEigrpIntf, sysIntf);
#ifdef _EIGRP_PLAT_MODULE
	if(EigrpIntfFindByIndex(pEigrpIntf->ifindex))
	{
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	if(!gpEigrp->protoQue->head){
		EigrpIntfAdd(pEigrpIntf);
	}else{
		for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
			EigrpIpEnqueueIfAddEvent(pdb, pEigrpIntf);
		}
	}
	return;
}

/************************************************************************************

	Name:	EigrpSysIntfDel_ex

	Desc:	This function is to delete an interface from the eigrp interface list for the physical 
			media removing.
		
	Para: 	pIntf		- pointer to the interface which is to be deleted
	
	Ret:		NONE
************************************************************************************/

void	EigrpSysIntfDel_ex(void *pIntf)			
{
	EigrpIntf_st	eigrpIntf;

	if(pIntf == NULL){
		return;
	}

	EigrpPortCopyIntfInfo(&eigrpIntf, pIntf);	
#ifdef _EIGRP_PLAT_MODULE
	if(! EigrpIntfFindByIndex(eigrpIntf.ifindex))
	{
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	if((EigrpPdb_st *)gpEigrp->protoQue->head){
		EigrpIpEnqueueIfDeleteEvent((EigrpPdb_st *)gpEigrp->protoQue->head, eigrpIntf.ifindex);
	}else{
		/*TODO*/
#ifdef _EIGRP_PLAT_MODULE
		EigrpIntfDel(&eigrpIntf);
#endif// _EIGRP_PLAT_MODULE	
	}

	return; 
}

void	EigrpSysIntfDel2_ex(U32 ifIndex)			
{

	_EIGRP_DEBUG("dbg>>EigrpSysIntfDel2_ex: ifIndex=%d\n", ifIndex);
#ifdef _EIGRP_PLAT_MODULE
	if(! EigrpIntfFindByIndex(ifIndex))
	{
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	if((EigrpPdb_st *)gpEigrp->protoQue->head){
		EigrpIpEnqueueIfDeleteEvent((EigrpPdb_st *)gpEigrp->protoQue->head, ifIndex);
	}else{
#ifdef _EIGRP_PLAT_MODULE
		EigrpIntf_pt	pEigrpIntf;
		pEigrpIntf = EigrpIntfFindByIndex(ifIndex);
		if(pEigrpIntf)
			EigrpIntfDel(pEigrpIntf);
#endif// _EIGRP_PLAT_MODULE	
	}

	return; 
}

/************************************************************************************

	Name:	EigrpSysIntfUp_ex

	Desc:	This function is to process the interface-going-up event.
		
	Para: 	pIntf		- pointer to the interface where the event occur
	
	Ret:		NONE
************************************************************************************/

void	EigrpSysIntfUp_ex(void *pIntf)			
{
	EigrpIntf_st	eigrpIntf, *pEigrpIntfNew, *pEigrpIntf;

	if(pIntf == NULL){
		return;
	}

	EigrpPortCopyIntfInfo(&eigrpIntf, pIntf);	
#ifdef _EIGRP_PLAT_MODULE
	if(! EigrpIntfFindByIndex(eigrpIntf.ifindex))
	{
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	if((EigrpPdb_st *)gpEigrp->protoQue->head){
		pEigrpIntfNew = EigrpPortMemMalloc(sizeof(EigrpIntf_st));
		if(!pEigrpIntfNew){	
			return;
		}
		EigrpPortCopyIntfInfo(pEigrpIntfNew, pIntf);
		EigrpIpEnqueueIfUpEvent((EigrpPdb_st *)gpEigrp->protoQue->head, pEigrpIntfNew);
	}else{/*zhenxl_20120907*/
		EigrpPortCopyIntfInfo(&eigrpIntf, pIntf);
		pEigrpIntf = EigrpIntfFindByIndex(eigrpIntf.ifindex);
		if(pEigrpIntf == NULL){
			return;
		}
		EigrpPortStrCpy(pEigrpIntf->name, eigrpIntf.name);
		pEigrpIntf->flags = eigrpIntf.flags;
		pEigrpIntf->metric = eigrpIntf.metric;
		pEigrpIntf->mtu = eigrpIntf.mtu;
		pEigrpIntf->bandwidth = eigrpIntf.bandwidth;
		pEigrpIntf->delay = eigrpIntf.delay;
	}
	return;
 }

/************************************************************************************

	Name:	EigrpSysIntfDown_ex

	Desc:	This function is to process the interface-going-up event.
		
	Para: 	pIntf		- pointer to the interface where the event occur
	
	Ret:		NONE
************************************************************************************/

void	EigrpSysIntfDown_ex(void *pIntf)		
{
	EigrpIntf_st	eigrpIntf, *pEigrpIntf;

	if(pIntf == NULL){
		return;
	}

	EigrpPortCopyIntfInfo(&eigrpIntf, pIntf);	
#ifdef _EIGRP_PLAT_MODULE
	if(! EigrpIntfFindByIndex(eigrpIntf.ifindex))
	{
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	if((EigrpPdb_st *)gpEigrp->protoQue->head){
		EigrpIpEnqueueIfDownEvent((EigrpPdb_st *)gpEigrp->protoQue->head, eigrpIntf.ifindex);
	}
#ifdef _EIGRP_PLAT_MODULE
	else{
			pEigrpIntf = EigrpIntfFindByIndex(eigrpIntf.ifindex);
			if(pEigrpIntf == NULL){
				return;
			}
			EigrpPortStrCpy(pEigrpIntf->name, eigrpIntf.name);
			pEigrpIntf->flags = eigrpIntf.flags;
			pEigrpIntf->metric = eigrpIntf.metric;
			pEigrpIntf->mtu = eigrpIntf.mtu;
			pEigrpIntf->bandwidth = eigrpIntf.bandwidth;
			pEigrpIntf->delay = eigrpIntf.delay;
		}
#endif// _EIGRP_PLAT_MODULE	
	return;
}

void	EigrpSysIntfAddrAdd_ex(U32 index, U32 ipAddr, U32 ipMask, U8 resetIdb)
{
	EigrpPdb_st *pdb;

	if(EIGRP_IN_MULTICAST(ipAddr)){
		return;
	}
#ifdef _EIGRP_PLAT_MODULE
	if(!EigrpIntfFindByIndex(index))
	{
		return ;
	}

	if(!gpEigrp->protoQue->head)
	{
		EigrpIntf_pt	pEigrpIntf;
		pEigrpIntf = EigrpIntfFindByIndex(index);
		if(pEigrpIntf)
		{
			pEigrpIntf->ipAddr = 0;
			pEigrpIntf->ipMask = 0;
			//pEigrpIntf->remoteIpAddr = 0;
		}
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		EigrpIpEnqueueIfAddressAddEvent(pdb, index, ipAddr, ipMask, resetIdb);
	}
	return;
}

void	EigrpSysIntfAddrDel_ex(U32 index, U32 ipAddr, U32 ipMask)
{
	EigrpPdb_st *pdb;
	EigrpIntf_pt	pEigrpIntf;

	if(EIGRP_IN_MULTICAST(ipAddr)){
		return;
	}
#ifdef _EIGRP_PLAT_MODULE
	if(! EigrpIntfFindByIndex(index))
	{
		return ;
	}
#endif// _EIGRP_PLAT_MODULE	
	//gpEigrp->conf->pConfAs->asNum;
	if(gpEigrp->protoQue->head){
		for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
			EigrpIpEnqueueIfAddressDeleteEvent(pdb, index, ipAddr, ipMask);
		}
	}else{
		/*zhenxl_20120919 删除EIGRP接口表中的地址.*/
		pEigrpIntf = EigrpIntfFindByIndex(index);
		if(ipAddr == pEigrpIntf->ipAddr){
			pEigrpIntf->ipAddr = 0;
			pEigrpIntf->ipMask = 0;
			pEigrpIntf->remoteIpAddr = 0;
		}
	}

	return;
}
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
#ifdef _EIGRP_PLAT_MODULE
/**************************************************************************************************/
/**************************************************************************************************/
int zebraEigrpIpProcessIfStateChangeCommand(S32 noFlag, int asNum, U8 *ifName)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdb_st	*iidb = NULL;
	EigrpIdb_st		*pIidb;
	U32	net, mask;
	EigrpPdb_st *pdb;
	S32	retVal;
	int cnt;
	
	pdb	= EigrpIpFindPdb(asNum);
	if(!pdb){
		return FAILURE;
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
			EigrpTakeNbrsDown(pdb->ddb, pIidb->idb, TRUE, "interface state changed");
		}
	}
	return SUCCESS;
}
/**************************************************************************************************/
/**************************************************************************************************/
static int zebra_eigrp_if_handle(EigrpCmd_pt pCmd)
{
	EigrpPdb_st	*pdb;
	EigrpIntf_pt	pEigrpIntf;
	pEigrpIntf = EigrpIntfFindByIndex(pCmd->u32Para1);
	if(pCmd->noFlag == FALSE)
	{
		if(pEigrpIntf == NULL)
		{
			EigrpIntf_pt	pIntf;
			pIntf = EigrpPortMemMalloc(sizeof(EigrpIntf_st));
			if(!pIntf){	
				return FAILURE;
			}
			EigrpPortMemCpy(pIntf, pCmd->vsPara1, sizeof(EigrpIntf_st));

			if(!gpEigrp->protoQue->head)//没有启动EIGRP
				EigrpIntfAdd(pIntf);
			else
			{
				for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next)
					EigrpIpEnqueueIfAddEvent(pdb, pIntf);
			}
			if(pCmd->vsPara1)
				EigrpPortMemFree(pCmd->vsPara1);
		}
		else 
		{
			if(pCmd->vsPara1)
				EigrpPortMemFree(pCmd->vsPara1);
			_EIGRP_DEBUG("%s:This interface is here:%s\n",((EigrpIntf_pt)(pCmd->vsPara1))->name);
			return FAILURE;
		}
	}
	else
	{
		if(pEigrpIntf==NULL)
		{
			_EIGRP_DEBUG("%s:This interface of index is not here:%d\n",__func__,pCmd->u32Para1);
			return FAILURE;
		}

		if(BIT_TEST(pEigrpIntf->active, EIGRP_INTF_FLAG_ACTIVE))
		{
			EIGRP_TRC(DEBUG_EIGRP_OTHER,"EIGRP: Can't delete this interface of:%s\n",pEigrpIntf->name);
			return FAILURE;
		}

		if(!gpEigrp->protoQue->head)//没有启动EIGRP
			EigrpIntfDel(pEigrpIntf);
		else
		{
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next)
				EigrpIpEnqueueIfDeleteEvent(pdb, pCmd->u32Para1);
		}
		//if((EigrpPdb_st *)gpEigrp->protoQue->head)
		//	EigrpIpEnqueueIfDeleteEvent((EigrpPdb_st *)gpEigrp->protoQue->head, pCmd->u32Para1);
	}
	//_EIGRP_DEBUG("%s:\n",__func__);
	return SUCCESS;	
}
/**************************************************************************************************/
static int zebra_eigrp_if_address_handle(EigrpCmd_pt pCmd)
{
	EigrpPdb_st	*pdb;
	EigrpIntf_pt	pEigrpIntf;
	pEigrpIntf = EigrpIntfFindByIndex(pCmd->u32Para1);	
	if(pCmd->noFlag == FALSE)
	{
		if(pEigrpIntf!=NULL)
		{
			if(!gpEigrp->protoQue->head)//没有启动EIGRP
			{
				pEigrpIntf->ipAddr = pCmd->u32Para2;
				pEigrpIntf->ipMask = pCmd->u32Para3;
			}
			else
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next)
					EigrpIpEnqueueIfAddressAddEvent(pdb, pCmd->u32Para1, pCmd->u32Para2, pCmd->u32Para3, pCmd->u32Para4);
		}
		else
		{
			_EIGRP_DEBUG("%s:This interface of index is not here:%d\n",__func__,pCmd->u32Para1);
			return FAILURE;	
		}
	}
	else
	{
		if(pEigrpIntf==NULL)
		{
			_EIGRP_DEBUG("%s:This interface of index is not here:%d\n",__func__,pCmd->u32Para1);
			return FAILURE;
		}
		if(BIT_TEST(pEigrpIntf->active, EIGRP_INTF_FLAG_ACTIVE))
		{
			EIGRP_TRC(DEBUG_EIGRP_OTHER,"EIGRP: Can't delete this interface of:%s\n",pEigrpIntf->name);
			return FAILURE;
		}
		if(!gpEigrp->protoQue->head)//没有启动EIGRP
		{
			pEigrpIntf->ipAddr = pCmd->u32Para2;
			pEigrpIntf->ipMask = pCmd->u32Para3;
		}
		else		
		if(gpEigrp->protoQue->head){
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpIpEnqueueIfAddressDeleteEvent(pdb, pCmd->u32Para1, pCmd->u32Para2, pCmd->u32Para3);
			}
		}
	}
	return SUCCESS;		
}
/**************************************************************************************************/
/**************************************************************************************************/
static int zebra_eigrp_if_state_handle(EigrpCmd_pt pCmd)
{
	EigrpIntf_pt		pEigrpIntf;
	EigrpIntf_pt		Intf = (EigrpIntf_pt)pCmd->vsPara1;
	
	if(EigrpIntfFindByIndex(pCmd->u32Para1)==NULL)
	{
		_EIGRP_DEBUG("%s:This interface of index is not here:%d\n",__func__,pCmd->u32Para1);
		if(pCmd->vsPara1)
			EigrpPortMemFree(pCmd->vsPara1);
		return FAILURE;
	}
	if((EigrpPdb_st *)gpEigrp->protoQue->head){
		EigrpIntf_pt	pIntf;
		pIntf = EigrpPortMemMalloc(sizeof(EigrpIntf_st));
		if(!pIntf){	
			if(pCmd->vsPara1)
				EigrpPortMemFree(pCmd->vsPara1);
			return FAILURE;
		}
		EigrpPortMemCpy(pIntf, Intf, sizeof(EigrpIntf_st));
		
		if(pCmd->noFlag == FALSE)
			EigrpIpEnqueueIfUpEvent((EigrpPdb_st *)gpEigrp->protoQue->head, pIntf);
		else
			EigrpIpEnqueueIfDownEvent((EigrpPdb_st *)gpEigrp->protoQue->head, Intf->ifindex);
	}
	else
	{
		pEigrpIntf = EigrpIntfFindByIndex(Intf->ifindex);
		if(pEigrpIntf == NULL)
		{
			_EIGRP_DEBUG("%s:This interface of index is not here:%d\n",__func__,pCmd->u32Para1);
			if(pCmd->vsPara1)
				EigrpPortMemFree(pCmd->vsPara1);
			return FAILURE;
		}
		//update
		EigrpPortStrCpy(pEigrpIntf->name, Intf->name);
		pEigrpIntf->flags = Intf->flags;
		pEigrpIntf->metric = Intf->metric;
		pEigrpIntf->mtu = Intf->mtu;
		pEigrpIntf->bandwidth = Intf->bandwidth;
		pEigrpIntf->delay = Intf->delay;
	}
	zebraEigrpIpProcessIfStateChangeCommand(pCmd->noFlag, pCmd->asNum, pCmd->ifName);
	return SUCCESS;
}
/**************************************************************************************************/
static int zebra_eigrp_route_handle(EigrpCmd_pt pCmd)
{
	ZebraEigrpRt_pt route = (ZebraEigrpRt_pt)pCmd->vsPara1;
	if(pCmd->noFlag == FALSE)
	{
		EigrpPortRouteChange(pCmd->asNum, ZEBRA_IPV4_ROUTE_ADD, route->proto, route->target, route->mask, route->gateway, route->metric);
	}
	else
	{
		EigrpPortRouteChange(pCmd->asNum, ZEBRA_IPV4_ROUTE_DELETE, route->proto, route->target, route->mask, route->gateway, route->metric);
	}
	if(pCmd->vsPara1)
		EigrpPortMemFree(pCmd->vsPara1);
	return SUCCESS;		
}
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
int zebraEigrpCmdEvent(EigrpCmd_pt pCmd)
{
	U32	msgnum;
	S32	retVal = FAILURE;
	EigrpPdb_st	*pdb;
	pdb	= NULL;
	zebraEigrpCmdSetupResponse(NULL, FAILURE, TRUE);
	switch(pCmd->type){
		case ZEBRA_INTERFACE_ADD:
		case ZEBRA_INTERFACE_DELETE:
			retVal = zebra_eigrp_if_handle(pCmd);
			break;
		case ZEBRA_INTERFACE_ADDRESS_ADD:
		case ZEBRA_INTERFACE_ADDRESS_DELETE:
			retVal = zebra_eigrp_if_address_handle(pCmd);
			break;
		case ZEBRA_INTERFACE_UP:
		case ZEBRA_INTERFACE_DOWN:
			retVal = zebra_eigrp_if_state_handle(pCmd);
			break;
		case ZEBRA_IPV4_ROUTE_ADD:
		case ZEBRA_IPV4_ROUTE_DELETE:
			retVal = zebra_eigrp_route_handle(pCmd);
			break;
		case ZEBRA_IPV6_ROUTE_ADD:
		case ZEBRA_IPV6_ROUTE_DELETE:
		case ZEBRA_REDISTRIBUTE_ADD:
		case ZEBRA_REDISTRIBUTE_DELETE:
		case ZEBRA_REDISTRIBUTE_DEFAULT_ADD:
		case ZEBRA_REDISTRIBUTE_DEFAULT_DELETE:
		case ZEBRA_IPV4_NEXTHOP_LOOKUP:
		case ZEBRA_IPV6_NEXTHOP_LOOKUP:
		case ZEBRA_IPV4_IMPORT_LOOKUP:
		case ZEBRA_IPV6_IMPORT_LOOKUP:
		case ZEBRA_INTERFACE_RENAME:
		case ZEBRA_ROUTER_ID_ADD:
		case ZEBRA_ROUTER_ID_DELETE:
		case ZEBRA_ROUTER_ID_UPDATE:
			retVal = SUCCESS;
			break;
		default:
			break;
	}
	zebraEigrpCmdSetupResponse(pCmd, retVal, TRUE);
	EigrpPortMemFree(pCmd);
	if(pdb){
		(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
		if(msgnum){
			EigrpIpLaunchPdbJob(pdb);
		}
	}	
	//zebraEigrpCmdSetupResponse(pCmd, retVal);
	return retVal;
}
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
S32	zebraEigrpCmdInterface(S32 noFlag, U32 asNum, EigrpIntf_st *sysif)
{
	EigrpCmd_pt		pCmd;
	EigrpIntf_st	*eigrpIntf = (EigrpIntf_st	*)sysif;
	
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);
	
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		if(gpEigrp->EigrpSem)
			EigrpPortSemBGive(gpEigrp->EigrpSem);
		return FAILURE;
	}
	EigrpPortMemSet(pCmd, 0, sizeof(EigrpCmd_st));
	pCmd->type	= ZEBRA_INTERFACE_ADD;
	pCmd->asNum	= asNum;
	pCmd->noFlag = noFlag;
	EigrpPortStrCpy(pCmd->ifName, eigrpIntf->name);
	if(noFlag == FALSE){
		pCmd->vsPara1	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
		if(!pCmd->vsPara1){	
			EigrpPortMemFree(pCmd);
			if(gpEigrp->EigrpSem)
				EigrpPortSemBGive(gpEigrp->EigrpSem);
			return FAILURE;
		}
		EigrpPortMemCpy(pCmd->vsPara1, sysif, sizeof(EigrpIntf_st));
	}else{
		//pCmd->u32Para1 = eigrpIntf->ifindex;
		pCmd->vsPara1	= NULL;
	}
	pCmd->u32Para1 = eigrpIntf->ifindex;
	
	EigrpUtilQue2Enqueue(EigrpMaster->ZebraCmdQue, (EigrpQueElem_st *)pCmd);
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);	
	return zebraEigrpCmdResponse();
}
/**************************************************************************************************/
/**************************************************************************************************/
S32	zebraEigrpCmdInterfaceState(S32 noFlag, U32 asNum, EigrpIntf_st *sysif)
{
	EigrpCmd_pt		pCmd;
	EigrpIntf_st	*eigrpIntf = (EigrpIntf_st	*)sysif;
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		if(gpEigrp->EigrpSem)
			EigrpPortSemBGive(gpEigrp->EigrpSem);
		return FAILURE;
	}
	EigrpPortMemSet(pCmd, 0, sizeof(EigrpCmd_st));
	pCmd->type	= ZEBRA_INTERFACE_UP;
	pCmd->asNum	= asNum;
	pCmd->noFlag = noFlag;
	EigrpPortStrCpy(pCmd->ifName, eigrpIntf->name);

	pCmd->vsPara1	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
	if(!pCmd->vsPara1){	
		EigrpPortMemFree(pCmd);
		if(gpEigrp->EigrpSem)
			EigrpPortSemBGive(gpEigrp->EigrpSem);
		return FAILURE;
	}
	EigrpPortMemCpy(pCmd->vsPara1, sysif, sizeof(EigrpIntf_st));

	pCmd->u32Para1 = eigrpIntf->ifindex;
	EigrpUtilQue2Enqueue(EigrpMaster->ZebraCmdQue, (EigrpQueElem_st *)pCmd);
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);	
	return zebraEigrpCmdResponse();
}
/**************************************************************************************************/
/**************************************************************************************************/
S32	zebraEigrpCmdInterfaceAddress(S32 noFlag, U32 asNum, S32 index, U32 ipAddr, U32 ipMask, U8 resetIdb)
{
	EigrpCmd_pt		pCmd;

	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		if(gpEigrp->EigrpSem)
			EigrpPortSemBGive(gpEigrp->EigrpSem);
		return FAILURE;
	}
	EigrpPortMemSet(pCmd, 0, sizeof(EigrpCmd_st));
	pCmd->type	= ZEBRA_INTERFACE_ADDRESS_ADD;
	pCmd->asNum	= asNum;
	pCmd->noFlag = noFlag;

	//S32 index, U32 ipAddr, U32 ipMask, U8 resetIdb
	pCmd->u32Para1 = index;
	pCmd->u32Para2 = ipAddr;
	pCmd->u32Para3 = ipMask;
	pCmd->u32Para4 = resetIdb;
	EigrpUtilQue2Enqueue(EigrpMaster->ZebraCmdQue, (EigrpQueElem_st *)pCmd);
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);	
	return zebraEigrpCmdResponse();
}
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
S32	zebraEigrpCmdRoute(S32 noFlag, U32 asNum, int proto, long ip, long mask, long next, int metric)
{
	EigrpCmd_pt		pCmd;
	ZebraEigrpRt_pt route;// = (ZebraEigrpRt_pt)pCmd->vsPara1;
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		if(gpEigrp->EigrpSem)
			EigrpPortSemBGive(gpEigrp->EigrpSem);		
		return FAILURE;
	}
	EigrpPortMemSet(pCmd, 0, sizeof(EigrpCmd_st));
	pCmd->type	= ZEBRA_IPV4_ROUTE_ADD;
	pCmd->asNum	= asNum;
	pCmd->noFlag = noFlag;

	route	= EigrpPortMemMalloc(sizeof(ZebraEigrpRt_st));
	if(!route){	
		if(gpEigrp->EigrpSem)
			EigrpPortSemBGive(gpEigrp->EigrpSem);		
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}
	route->target	= ip;
	route->mask		= mask;
	route->gateway	= next;
	route->metric	= metric;
	route->proto	= proto;
	route->ifindex	= 0;	
	pCmd->vsPara1 = route;
	
	EigrpUtilQue2Enqueue(EigrpMaster->ZebraCmdQue, (EigrpQueElem_st *)pCmd);
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);	
	return zebraEigrpCmdResponse();
}
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
#endif //_EIGRP_PLAT_MODULE

