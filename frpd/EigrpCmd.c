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

	Module:	EigrpCmd

	Name:	EigrpCmd.c

	Desc:	

	Rev:	
	
***********************************************************************************/

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

	Name:	EigrpConfigIntfApply

	Desc:	This function is to apply all the saved configurations on a given physical interface.
		
	Para: 	circName		- pointer to the physical interface name
	
	Ret:		NONE
************************************************************************************/

void	EigrpConfigIntfApply(U8 *circName)
{
	EigrpConfIntf_pt			pConfIntf;
	EigrpConfIntfAuthkey_pt	pAuthKey;
	EigrpConfIntfAuthid_pt		pAuthId;
	EigrpConfIntfAuthMode_pt	pAuthmode;
	EigrpConfIntfPassive_pt		pPassive;
	EigrpConfIntfBw_pt			pBandwidth;
	EigrpConfIntfDelay_pt			pDelay;
	EigrpConfIntfBandwidth_pt			pBw;
	EigrpConfIntfHello_pt		pHello;
	EigrpConfIntfHold_pt		pHold;
	EigrpConfIntfSplit_pt		pSplit;
	EigrpConfIntfSum_pt		pSummary;

	EigrpPdb_st	*pdb;
	EigrpIntf_pt	pEigrpIntf;
	S32	noFlag;
	U8	*keyStr;

	EIGRP_FUNC_ENTER(EigrpConfigIntfApply);
	if(!gpEigrp->conf->pConfIntf){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfApply);
		return;
	}

	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(EigrpPortStrCmp(pConfIntf->ifName, circName)){
			continue;
		}
		
		for(pAuthKey = pConfIntf->authkey; pAuthKey; pAuthKey = pAuthKey->next){
			pdb = EigrpIpFindPdb(pAuthKey->asNum);
			if(!pdb){
				continue;
			}
			keyStr	= EigrpPortMemMalloc(EIGRP_AUTH_LEN);
			if(!keyStr){	 
				EIGRP_FUNC_LEAVE(EigrpConfigIntfApply);
				return;
			}
			EigrpPortMemCpy(keyStr, pAuthKey->auth, EIGRP_AUTH_LEN);
			keyStr[EIGRP_AUTH_LEN - 1]	= '\0';
			EigrpIpProcessIfSetAuthenticationKey(pdb, circName, keyStr);
		}

		for(pPassive = pConfIntf->passive; pPassive; pPassive = pPassive->next){
			pdb = EigrpIpFindPdb(pPassive->asNum);
			if(!pdb){
				continue;
			}

			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}

			noFlag	= EIGRP_DEF_PASSIVE;
			
			EigrpIpProcessPassiveIntfCommand(noFlag, pdb, pEigrpIntf);
		}

		for(pAuthId = pConfIntf->authid; pAuthId; pAuthId = pAuthId->next){
			pdb = EigrpIpFindPdb(pAuthId->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}
		
			EigrpIpProcessIfSetAuthenticationKeyidCommand(pdb, circName, pAuthId->authId);
		}
			
		for(pAuthmode = pConfIntf->authmode; pAuthmode; pAuthmode = pAuthmode->next){
			pdb = EigrpIpFindPdb(pAuthmode->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			if(!pEigrpIntf){
				break;
			}
		
			noFlag = (pAuthmode->authMode == TRUE) ? FALSE : TRUE;
			EigrpIpProcessIfSetAuthenticationMode(noFlag, pdb, circName);
		}
				
		for(pBandwidth = pConfIntf->bandwidth; pBandwidth; pBandwidth = pBandwidth->next){
			pdb = EigrpIpFindPdb(pBandwidth->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}
		
			EigrpIpProcessIfSetBandwidthPercentCommand(pdb, circName, pBandwidth->bandwidth);
		}

		for(pBw = pConfIntf->bw; pBw; pBw = pBw->next){
			pdb = EigrpIpFindPdb(pBw->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				/*break; TODO*/
			}
		
			EigrpIpProcessIfSetBandwidth(pdb, circName, pBw->bandwidth);
		}
		
		for(pDelay = pConfIntf->delay; pDelay; pDelay = pDelay->next){
			pdb = EigrpIpFindPdb(pDelay->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}
		
			EigrpIpProcessIfSetDelay(pdb, circName, pDelay->delay);
		}

		for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
			pdb = EigrpIpFindPdb(pHello->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}
		
			EigrpIpProcessIfSetHelloIntervalCommand(FALSE, pdb, circName, pHello->hello);
		}

		for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
			pdb = EigrpIpFindPdb(pHold->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}
		
			EigrpIpProcessIfSetHoldtimeCommand(FALSE, pdb, circName, pHold->hold);
		}

		for(pSplit = pConfIntf->split; pSplit; pSplit = pSplit->next){
			pdb = EigrpIpFindPdb(pSplit->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}

			noFlag = (pSplit->split == TRUE) ? FALSE : TRUE;
			EigrpIpProcessIfSetSplitHorizonCommand(noFlag, pdb, circName);
		}

		for(pSummary = pConfIntf->summary; pSummary; pSummary = pSummary->next){
			pdb = EigrpIpFindPdb(pSummary->asNum);
			if(!pdb){
				continue;
			}
		
			pEigrpIntf = EigrpIntfFindByName(circName);
			 if(!pEigrpIntf){
				break;
			}

			EigrpIpProcessIfSummaryAddressCommand(FALSE, pdb, circName, pSummary->ipNet, pSummary->ipMask);
		}
	}
	
	EIGRP_FUNC_LEAVE(EigrpConfigIntfApply);
	
	return;
}

/************************************************************************************

	Name:	EigrpConfigIntfRapplyAll

	Desc:	This function is to apply all the saved configurations on a all possible interface.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpConfigIntfRapplyAll()
{
	EigrpConfIntf_pt	pConfIntf;

	EIGRP_FUNC_ENTER(EigrpConfigIntfRapplyAll);
	if(!gpEigrp->conf->pConfIntf){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfRapplyAll);
		return;
	}

	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		EigrpConfigIntfApply(pConfIntf->ifName);
	}
	
	EIGRP_FUNC_LEAVE(EigrpConfigIntfRapplyAll);
	
	return;
}

/************************************************************************************

	Name:	EigrpConfigRouter

	Desc:	This function is to create / remove an eigrp router or just enter the router config 
			mode. 
		
	Para: 	asNum		- autonomous system number
			noFlag		- the flag of negative command 
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigRouter(S32 noFlag, U32 asNum)
{
	EigrpConfAs_pt		pConfAs;
	EigrpConfAsNei_pt	pNei, pNeiNext;
	EigrpConfAsRedis_pt	pRedis, pRedisNext;

	EIGRP_FUNC_ENTER(EigrpConfigRouter);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}

	if(noFlag == FALSE){
		if(pConfAs){
			EIGRP_FUNC_LEAVE(EigrpConfigRouter);
			return FAILURE;
		}

		pConfAs	= EigrpPortMemMalloc(sizeof(EigrpConfAs_st));
		if(!pConfAs){	
			EIGRP_FUNC_LEAVE(EigrpConfigRouter);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs);
		
		pConfAs->asNum	= asNum;
		if(gpEigrp->conf->pConfAs){
			gpEigrp->conf->pConfAs->prev	= pConfAs;
		}
		
		pConfAs->next	= gpEigrp->conf->pConfAs;
		pConfAs->prev	= NULL;
		gpEigrp->conf->pConfAs	= pConfAs;
	}else{
		if(!pConfAs){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "This eigrp router has never been configured.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigRouter);
			return FAILURE;
		}

		if(!pConfAs->prev){
			gpEigrp->conf->pConfAs	= pConfAs->next;
		}
		
		if(pConfAs->next){
			pConfAs->next->prev	= pConfAs->prev;
		}
		if(pConfAs->prev){
			pConfAs->prev->next	= pConfAs->next;
		}

		/* release all members */
		if(pConfAs->defMetric){
			EigrpPortMemFree(pConfAs->defMetric);
		}
		if(pConfAs->distance){
			EigrpPortMemFree(pConfAs->distance);
		}
		if(pConfAs->metricWeight){
			EigrpPortMemFree(pConfAs->metricWeight);
		}
		for(pNei = pConfAs->nei; pNei; pNei = pNeiNext){
			pNeiNext	= pNei->next;
			EigrpPortMemFree(pNei);
		}
		for(pRedis = pConfAs->redis; pRedis; pRedis = pRedisNext){
			pRedisNext	= pRedis->next;
			EigrpPortMemFree(pRedis);
		}
		if(pConfAs->defRouteIn){
			EigrpPortMemFree(pConfAs->defRouteIn);
		}
		if(pConfAs->defRouteOut){
			EigrpPortMemFree(pConfAs->defRouteOut);
		}
		
		EigrpPortMemFree(pConfAs);
	}

	EIGRP_FUNC_LEAVE(EigrpConfigRouter);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigAutoSummary

	Desc:	This function is to enable / disable the auto summary of an EIGRP router. 
		
	Para: 	asNum		- autonomous system number
			noFlag		- the flag of negative command 
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigAutoSummary(S32 noFlag, U32 asNum)
{
	EigrpConfAs_pt	pConfAs;
	S32	autoSum;

	EIGRP_FUNC_ENTER(EigrpConfigAutoSummary);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}

	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigAutoSummary);
		return FAILURE;
	}

	if(noFlag == TRUE){
		autoSum	= FALSE;
	}else{
		autoSum	= TRUE;
	}
#ifdef _EIGRP_PLAT_MODULE
	if((pConfAs->autoSum) && (*pConfAs->autoSum == autoSum)) {
		EIGRP_FUNC_LEAVE(EigrpConfigAutoSummary);
		return SUCCESS;
	}
#else//_EIGRP_PLAT_MODULE
	if((pConfAs->autoSum && (*pConfAs->autoSum == autoSum)) ||
			(!pConfAs->autoSum && (autoSum == EIGRP_DEF_AUTOSUM))){
		EIGRP_FUNC_LEAVE(EigrpConfigAutoSummary);
		return FAILURE;
	}
#endif//_EIGRP_PLAT_MODULE


	if(!pConfAs->autoSum){
		pConfAs->autoSum	= EigrpPortMemMalloc(sizeof(S32));
		if(!pConfAs->autoSum){	
			EIGRP_FUNC_LEAVE(EigrpConfigAutoSummary);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs->autoSum);

		*pConfAs->autoSum	= autoSum;
	}else{
		EigrpPortMemFree(pConfAs->autoSum);
		pConfAs->autoSum	= NULL;
	}

	EIGRP_FUNC_LEAVE(EigrpConfigAutoSummary);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigDefMetric

	Desc:	This function is to set / unset the default metric of an EIGRP router. 
		
	Para: 	asNum		- autonomous system number
			noFlag		- the flag of negative command 
			vmetric		- pointer to the given vecmetroc metric
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigDefMetric(S32 noFlag, U32 asNum, EigrpVmetric_st *vmetric)
{
	EigrpConfAs_pt	pConfAs;

	EIGRP_FUNC_ENTER(EigrpConfigDefMetric);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}

	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigDefMetric);
		return FAILURE;
	}
	if(noFlag == FALSE){
		if((pConfAs->defMetric && pConfAs->defMetric->bandwidth == vmetric->bandwidth 
							&& pConfAs->defMetric->delay	== vmetric->delay
							&& pConfAs->defMetric->reliability == vmetric->reliability 
							&& pConfAs->defMetric->load	== vmetric->load
							&& pConfAs->defMetric->mtu	== vmetric->mtu) 
									){
			EIGRP_FUNC_LEAVE(EigrpConfigDefMetric);
			return FAILURE;
		}

		pConfAs->defMetric	= EigrpPortMemMalloc(sizeof(EigrpVmetric_st));
		if(!pConfAs->defMetric){	
			EIGRP_FUNC_LEAVE(EigrpConfigDefMetric);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs->defMetric);

		EigrpPortMemCpy((U8 *)pConfAs->defMetric, (U8 *)vmetric, sizeof(EigrpVmetric_st));
	}else{
		if(!pConfAs->defMetric){
			EIGRP_FUNC_LEAVE(EigrpConfigDefMetric);
			return FAILURE;
		}

		EigrpPortMemFree(pConfAs->defMetric);
		pConfAs->defMetric	= NULL;
	}

	EIGRP_FUNC_LEAVE(EigrpConfigDefMetric);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigDistance

	Desc:	This function is to set the interior and exterior distance of an EIGRP router.
		
	Para: 	asNum	- the autonomous system number
			inter		- interior distance 
			exter	- exterior distance
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigDistance(S32 noFlag, U32 asNum, U32 inter, U32 exter)
{
	EigrpConfAs_pt	pConfAs;

	EIGRP_FUNC_ENTER(EigrpConfigDistance);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}
	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigDistance);
		return FAILURE;
	}

	if((pConfAs->distance && pConfAs->distance->interDis == inter && pConfAs->distance->exterDis == exter) ||
			(!pConfAs->distance && noFlag == TRUE)){
		EIGRP_FUNC_LEAVE(EigrpConfigDistance);
		return FAILURE;
	}

	if(!pConfAs->distance){
		pConfAs->distance		= EigrpPortMemMalloc(sizeof(EigrpConfAsDistance_st));
		if(!pConfAs->distance	){	
			EIGRP_FUNC_LEAVE(EigrpConfigDistance);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs->distance);

		pConfAs->distance->interDis	= inter;
		pConfAs->distance->exterDis	= exter;
	}else{
		if(noFlag == TRUE){
			EigrpPortMemFree(pConfAs->distance);
			pConfAs->distance		= NULL;
		}else{
 			pConfAs->distance->interDis	= inter;
			pConfAs->distance->exterDis	= exter;
		}
	}

	EIGRP_FUNC_LEAVE(EigrpConfigDistance);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigMetricWeight

	Desc:	This function is to set the metric weight of an EIGRP router. 
		
	Para: 	asNum	- the autonomous system number
			k1,k2,k3,k4,k5	- the K value
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigMetricWeight(S32 noFlag, U32 asNum, U32 k1, U32 k2, U32 k3, U32 k4, U32 k5)
{
	EigrpConfAs_pt	pConfAs;

	EIGRP_FUNC_ENTER(EigrpConfigMetricWeight);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}
	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigMetricWeight);
		return FAILURE;
	}

	if((pConfAs->metricWeight && pConfAs->metricWeight->k1 == k1
							&& pConfAs->metricWeight->k2 == k2
							&& pConfAs->metricWeight->k3 == k3
							&& pConfAs->metricWeight->k4 == k4
							&& pConfAs->metricWeight->k5 == k5) ||
			(!pConfAs->metricWeight && noFlag == TRUE)){
		EIGRP_FUNC_LEAVE(EigrpConfigMetricWeight);							
		return FAILURE;
	}

	if(!pConfAs->metricWeight){
		pConfAs->metricWeight		= EigrpPortMemMalloc(sizeof(EigrpConfAsMetricWeight_st));
		if(!pConfAs->metricWeight	){	
			EIGRP_FUNC_LEAVE(EigrpConfigMetricWeight);							
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs->metricWeight);

		pConfAs->metricWeight->k1	= k1;
		pConfAs->metricWeight->k2	= k2;
		pConfAs->metricWeight->k3	= k3;
		pConfAs->metricWeight->k4	= k4;
		pConfAs->metricWeight->k5	= k5;
	}else{
		if(noFlag == TRUE){
			EigrpPortMemFree((void *)pConfAs->metricWeight);
			pConfAs->metricWeight		= NULL;
		}else{
			pConfAs->metricWeight->k1	= k1;
			pConfAs->metricWeight->k2	= k2;
			pConfAs->metricWeight->k3	= k3;
			pConfAs->metricWeight->k4	= k4;
			pConfAs->metricWeight->k5	= k5;
		}
	}

	EIGRP_FUNC_LEAVE(EigrpConfigMetricWeight);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigNetwork

	Desc:	This function is to add / remove the network of an EIGRP router.
		
	Para: 	asNum	- autonomous system number
			noFlag	- the flag of negative command
			ipAddr	- the ip address of the EIGRP router's network
			ipMask	- the ip mask of the EIGRP router's network
			
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
			
************************************************************************************/

S32	EigrpConfigNetwork(S32 noFlag, U32 asNum, U32 ipAddr, U32 ipMask)
{
	EigrpConfAs_pt	pConfAs;
	EigrpConfAsNet_pt	pNet;
	U32	ipNet;

	EIGRP_FUNC_ENTER(EigrpConfigNetwork);
#ifdef _EIGRP_PLAT_MODULE
	//检测这个地址是否在接口链表里面，如果这个接口不在
	//接口链表里面，就不让这个接口启动FRP路由
	if(EigrpIntfFindByNetAndMask(ipAddr,  ipMask) == NULL) {
			EIGRP_FUNC_LEAVE(EigrpConfigNetwork);
			return FAILURE;
	}
#endif /* _EIGRP_PLAT_MODULE */

	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}

	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigNetwork);
		return FAILURE;
	}

	ipNet	= ipAddr & ipMask;
	for(pNet = pConfAs->network; pNet; pNet = pNet->next){
		if(pNet->ipNet == ipNet && pNet->ipMask == ipMask){
			break;
		}
	}

	if(noFlag == TRUE){
		if(!pNet){
			EIGRP_FUNC_LEAVE(EigrpConfigNetwork);
			return FAILURE;
		}

		if(pConfAs->network == pNet){
			pConfAs->network = pNet->next;
		}
		if(pNet->next){
			pNet->next->prev	= pNet->prev;
		}

		if(pNet->prev){
			pNet->prev->next	= pNet->next;
		}

		EigrpPortMemFree((void *)pNet);
	}else{
		if(pNet){
			EIGRP_FUNC_LEAVE(EigrpConfigNetwork);
			return FAILURE;
		}

		pNet	= EigrpPortMemMalloc(sizeof(EigrpConfAsNet_st));
		if(!pNet){	
			EIGRP_FUNC_LEAVE(EigrpConfigNetwork);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pNet);
		pNet->ipNet	= ipNet;
		pNet->ipMask	= ipMask;

		if(pConfAs->network){
			pConfAs->network->prev	= pNet;
		}
		pNet->next	= pConfAs->network;
		pNet->prev	= NULL;

		pConfAs->network	= pNet;
	}
#ifdef _EIGRP_PLAT_MODULE
	//根据接口地址和掩码设置激活这接口还是禁制这个接口
	if(noFlag == TRUE)
		EIGRP_ROUTE_INACTIVE(ipAddr, ipMask);
	else//激活这个接口（设置这个地址启动路由标识）
		EIGRP_ROUTE_ACTIVE(ipAddr, ipMask);
#endif//_EIGRP_PLAT_MODULE
	EIGRP_FUNC_LEAVE(EigrpConfigNetwork);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfPassive

	Desc:	This function is to set / unset the given interface to passive mode if it is owned by 
			the given EIGRP router. 
		
	Para: 	ifName	- pointer to the string which indicate the interface name
			asNum	- the autonomous system number
			noFlag	- the flag of negative command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfPassive(S32 noFlag, U8 *ifName, U32 asNum)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfPassive_pt	pPassive;
	S32 passive;

	EIGRP_FUNC_ENTER(EigrpConfigIntfPassive);
	pPassive	= NULL;
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}

	if(pConfIntf){
		for(pPassive = pConfIntf->passive; pPassive; pPassive = pPassive->next){
			if(pPassive->asNum == asNum){
				break;
			}
		}
	}

	if(noFlag == FALSE){
		passive	= TRUE;
	}else{
		passive	= FALSE;
	}

	if((!pPassive && passive == EIGRP_DEF_PASSIVE) || (pPassive && passive != EIGRP_DEF_PASSIVE)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfPassive);
		return FAILURE;
	}

	if(!pPassive){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfPassive);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev	= NULL;
			pConfIntf->next	= gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pPassive	= EigrpPortMemMalloc(sizeof(EigrpConfIntfPassive_st));
		if(!pPassive){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfPassive);
			return FAILURE;
		}
		
		if(pConfIntf->passive){
			pConfIntf->passive->prev	= pPassive;
		}
		
		pPassive->prev = NULL;
		pPassive->next = pConfIntf->passive;
		
		pConfIntf->passive	= pPassive;

		pPassive->asNum	= asNum;
	}else{
			if(pConfIntf->passive ==  pPassive){
				pConfIntf->passive	= pPassive->next;
			}

			if(pPassive->prev){
				pPassive->prev->next	= pPassive->next;
			}

			if(pPassive->next){
				pPassive->next->prev	= pPassive->prev;
			}

			EigrpPortMemFree((void *)pPassive);
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfPassive);
	
	return SUCCESS;
}
	
/************************************************************************************

	Name:	EigrpConfigIntfInvisible

	Desc:	This function is to set / unset the given interface to invisible mode if it is owned by 
			the given EIGRP router. 
		
	Para: 	ifName	- pointer to the string which indicate the interface name
			asNum	- the autonomous system number
			noFlag	- the flag of negative command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfInvisible(S32 noFlag, U8 *ifName, U32 asNum)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfInvisible_pt	pInvisible;
	S32 invisible;

	EIGRP_FUNC_ENTER(EigrpConfigIntfInvisible);
	pInvisible	= NULL;
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}

	if(pConfIntf){
		for(pInvisible = pConfIntf->invisible; pInvisible; pInvisible = pInvisible->next){
			if(pInvisible->asNum == asNum){
				break;
			}
		}
	}

	if(noFlag == FALSE){
		invisible	= TRUE;
	}else{
		invisible	= FALSE;
	}

	if((!pInvisible && invisible == EIGRP_DEF_INVISIBLE) || (pInvisible && invisible != EIGRP_DEF_INVISIBLE)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfInvisible);
		return FAILURE;
	}

	if(!pInvisible){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfInvisible);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev	= NULL;
			pConfIntf->next	= gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pInvisible	= EigrpPortMemMalloc(sizeof(EigrpConfIntfInvisible_st));
		if(!pInvisible){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfInvisible);
			return FAILURE;
		}
		
		if(pConfIntf->invisible){
			pConfIntf->invisible->prev	= pInvisible;
		}
		
		pInvisible->prev = NULL;
		pInvisible->next = pConfIntf->invisible;
		
		pConfIntf->invisible	= pInvisible;

		pInvisible->asNum	= asNum;
	}else{
			if(pConfIntf->invisible ==  pInvisible){
				pConfIntf->invisible	= pInvisible->next;
			}

			if(pInvisible->prev){
				pInvisible->prev->next	= pInvisible->next;
			}

			if(pInvisible->next){
				pInvisible->next->prev	= pInvisible->prev;
			}

			EigrpPortMemFree((void *)pInvisible);
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfInvisible);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigNeighbor

	Desc:	This function is to add / remove a neighbor for the given EIGRP router. 
		
	Para: 	asNum	- autonomous system number
			noFlag	- the flag of negative command
			ipAddr	- the ip address of the neighbor
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigNeighbor(S32 noFlag, U32 asNum, U32 ipAddr)
{
	EigrpConfAs_pt	pConfAs;
	EigrpConfAsNei_pt	pNei;

	EIGRP_FUNC_ENTER(EigrpConfigNeighbor);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}
	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigNeighbor);
		return FAILURE;
	}

	for(pNei = pConfAs->nei; pNei; pNei = pNei->next){
		if(pNei->ipAddr == ipAddr){
			break;
		}
	}

	if(noFlag == TRUE){
		if(!pNei){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "This neighbor is not added.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigNeighbor);
			return FAILURE;
		}

		if(pConfAs->nei == pNei){
			pConfAs->nei = pNei->next;
		}
		if(pNei->next){
			pNei->next->prev	= pNei->prev;
		}

		if(pNei->prev){
			pNei->prev->next	= pNei->next;
		}

		EigrpPortMemFree((void *)pNei);
	}else{
		if(pNei){
			EIGRP_TRC(DEBUG_EIGRP_OTHER, "This neighbor has already been added.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigNeighbor);
			return FAILURE;
		}

		pNei	= EigrpPortMemMalloc(sizeof(EigrpConfAsNei_st));
		if(!pNei){	
			EIGRP_FUNC_LEAVE(EigrpConfigNeighbor);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pNei);
		pNei->ipAddr	= ipAddr;

		if(pConfAs->nei){
			pConfAs->nei->prev	= pNei;
		}
		pNei->next	= pConfAs->nei;
		pNei->prev	= NULL;

		pConfAs->nei	= pNei;
	}

	EIGRP_FUNC_LEAVE(EigrpConfigNeighbor);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfAuthKey

	Desc:	This function is to change the MD5 auth key for an given interface and a given eigrp
			router. 
		
	Para: 	asNum	- autonomous system number
			noFlag	- the flag of negative command
			auth		- pointer to the authentication information
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfAuthKey(S32 noFlag, U8 *ifName, U32 asNum, U8 *keyStr)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfAuthkey_pt	pAuthkey;

	EIGRP_FUNC_ENTER(EigrpConfigIntfAuthKey);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pAuthkey	= NULL;
	if(pConfIntf){
		for(pAuthkey = pConfIntf->authkey; pAuthkey; pAuthkey = pAuthkey->next){
			if(pAuthkey->asNum == asNum){
				break;
			}
		}
	}

	if((!pAuthkey && noFlag == TRUE) || (pAuthkey && noFlag == FALSE && !EigrpPortStrCmp(pAuthkey->auth, keyStr))){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthKey);
		return FAILURE;
	}

	if(!pAuthkey){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthKey);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev	= NULL;
			pConfIntf->next	= gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pAuthkey	= EigrpPortMemMalloc(sizeof(EigrpConfIntfAuthkey_st));
		if(!pAuthkey){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthKey);
			return FAILURE;
		}
		
		if(pConfIntf->authkey){
			pConfIntf->authkey->prev	= pAuthkey;
		}
		
		pAuthkey->prev = NULL;
		pAuthkey->next = pConfIntf->authkey;
		
		pConfIntf->authkey	= pAuthkey;

		pAuthkey->asNum	= asNum;
		EigrpPortStrCpy(pAuthkey->auth, keyStr);
	}else{
		if(noFlag == FALSE && EigrpPortStrCmp(pAuthkey->auth, keyStr)){
			EigrpPortStrCpy(pAuthkey->auth, keyStr);
		}else{
			if(pConfIntf->authkey ==  pAuthkey){
				pConfIntf->authkey	= pAuthkey->next;
			}

			if(pAuthkey->prev){
				pAuthkey->prev->next	= pAuthkey->next;
			}

			if(pAuthkey->next){
				pAuthkey->next->prev	= pAuthkey->prev;
			}

			EigrpPortMemFree((void *)pAuthkey);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthKey);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfAuthid

	Desc:	This function is to change the MD5 auth id for an given interface and a given eigrp
			router.
		
	Para: 	ifName	- pointer to the string which indicate the given interface name
			asNum	- the autonomous system number
			authId	- the MD5 auth id
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfAuthid(S32 noFlag, U8 *ifName, U32 asNum, U32 authId)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfAuthid_pt	pAuthId;

	EIGRP_FUNC_ENTER(EigrpConfigIntfAuthid);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pAuthId	= NULL;
	if(pConfIntf){
		for(pAuthId = pConfIntf->authid; pAuthId; pAuthId = pAuthId->next){
			if(pAuthId->asNum == asNum){
				break;
			}
		}
	}

	if((!pAuthId && noFlag == TRUE) || (noFlag == FALSE && pAuthId && pAuthId->authId == authId)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthid);
		return FAILURE;
	}

	if(!pAuthId){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthid);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev	= NULL;
			pConfIntf->next	= gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pAuthId	= EigrpPortMemMalloc(sizeof(EigrpConfIntfAuthid_st));
		if(!pAuthId){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthid);
			return FAILURE;
		}
		
		if(pConfIntf->authid){
			pConfIntf->authid->prev	= pAuthId;
		}
		
		pAuthId->prev = NULL;
		pAuthId->next = pConfIntf->authid;
		
		pConfIntf->authid	= pAuthId;

		pAuthId->asNum	= asNum;
		pAuthId->authId	= authId;
	}else{
		if(noFlag == TRUE){
			if(pConfIntf->authid == pAuthId){
				pConfIntf->authid	= pAuthId->next;
			}

			if(pAuthId->prev){
				pAuthId->prev->next	= pAuthId->next;
			}

			if(pAuthId->next){
				pAuthId->next->prev	= pAuthId->prev;
			}

			EigrpPortMemFree((void *)pAuthId);
		}else{
			pAuthId->authId = authId;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthid);
	
	return SUCCESS;
}
	
/************************************************************************************

	Name:	EigrpConfigIntfAuthMode

	Desc:	This function is to enable / disable MD5 authentication for an given interface and a 
			given eigrp router. 
		
	Para: 	ifName	- pointer to the string which indicate the given interface name
			asNum	- the autonomous system number
			noFlag	- the flag of negative command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfAuthMode(S32 noFlag, U8 *ifName, U32 asNum)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfAuthMode_pt	pAuthmode;
	S32 authMode;

	EIGRP_FUNC_ENTER(EigrpConfigIntfAuthMode);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pAuthmode = NULL;
	if(pConfIntf){
		for(pAuthmode = pConfIntf->authmode; pAuthmode; pAuthmode = pAuthmode->next){
			if(pAuthmode->asNum == asNum){
				break;
			}
		}
	}

	if(noFlag == FALSE){
		authMode	= TRUE;
	}else{
		authMode	= FALSE;
	}

	if((!pAuthmode && authMode == EIGRP_DEF_AUTHMODE) || (pAuthmode && pAuthmode->authMode == authMode)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthMode);
		return FAILURE;
	}

	if(!pAuthmode){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthMode);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pAuthmode = EigrpPortMemMalloc(sizeof(EigrpConfIntfAuthMode_st));
		if(!pAuthmode){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthMode);
			return FAILURE;
		}

		if(pConfIntf->authmode){
			pConfIntf->authmode->prev	= pAuthmode;
		}
		
		pAuthmode->prev = NULL;
		pAuthmode->next = pConfIntf->authmode;
		
		pConfIntf->authmode	= pAuthmode;

		pAuthmode->asNum	= asNum;
		pAuthmode->authMode	= authMode;
	}else{
		if(authMode == EIGRP_DEF_AUTHMODE){
			if(pConfIntf->authmode ==  pAuthmode){
				pConfIntf->authmode	= pAuthmode->next;
			}

			if(pAuthmode->prev){
				pAuthmode->prev->next = pAuthmode->next;
			}

			if(pAuthmode->next){
				pAuthmode->next->prev = pAuthmode->prev;
			}

			EigrpPortMemFree((void *)pAuthmode);
		}else{
			pConfIntf->authmode =  pAuthmode;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfAuthMode);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfBandwidth

	Desc:	This function is to change the bandwidth for an given interface and a given eigrp 
			router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			bandwidth	- the new bandwidth
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfBandwidth(S32 noFlag, U8 *ifName, U32 asNum, U32 bandwidth)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfBw_pt	pBandwidth;

	EIGRP_FUNC_ENTER(EigrpConfigIntfBandwidth);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pBandwidth = NULL;
	if(pConfIntf){
		for(pBandwidth = pConfIntf->bandwidth; pBandwidth; pBandwidth = pBandwidth->next){
			if(pBandwidth->asNum == asNum){
				break;
			}
		}
	}

	if((!pBandwidth && noFlag == TRUE) || (noFlag == FALSE && pBandwidth && pBandwidth->bandwidth == bandwidth)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfBandwidth);
		return FAILURE;
	}

	if(!pBandwidth){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfBandwidth);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pBandwidth = EigrpPortMemMalloc(sizeof(EigrpConfIntfBw_st));
		if(!pBandwidth){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfBandwidth);
			return FAILURE;
		}
		
		if(pConfIntf->bandwidth){
			pConfIntf->bandwidth->prev	= pBandwidth;
		}
		
		pBandwidth->prev = NULL;
		pBandwidth->next = pConfIntf->bandwidth;
		
		pConfIntf->bandwidth	= pBandwidth;

		pBandwidth->asNum	= asNum;
		pBandwidth->bandwidth= bandwidth;
	}else{
		if(noFlag == FALSE){
			pBandwidth->bandwidth	= bandwidth;
		}else{
			if(pConfIntf->bandwidth ==  pBandwidth){
				pConfIntf->bandwidth	= pBandwidth->next;
			}

			if(pBandwidth->prev){
				pBandwidth->prev->next = pBandwidth->next;
			}

			if(pBandwidth->next){
				pBandwidth->next->prev = pBandwidth->prev;
			}
			
			EigrpPortMemFree((void *)pBandwidth);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfBandwidth);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfBandwidth

	Desc:	This function is to change the bandwidth for an given interface and a given eigrp 
			router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			bandwidth	- the new bandwidth
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/
S32	EigrpConfigIntfBw(S32 noFlag, U8 *ifName, U32 asNum, U32 bandwidth)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfBandwidth_pt	pBandwidth;

	EIGRP_FUNC_ENTER(EigrpConfigIntfBw);
	
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pBandwidth = NULL;
	if(pConfIntf){
		for(pBandwidth = pConfIntf->bw; pBandwidth; pBandwidth = pBandwidth->next){
			if(pBandwidth->asNum == asNum){
				break;
			}
		}
	}

	/*?T??D?|ì¨¤¨o???¨?¨|pal?a¨o?*/
	if(SUCCESS== EigrpIpProcessIfCheckUnnumberd(ifName)){
		/*?T??D?|ì¨¤????¨a??é¨o?à?¨?2?¨oy?¨2stopFrp|ì¨¨2¨′?á??¨o?à?ê????????¨¨?¨|¨¨???ê?eigrpIntfList2??à?ê¨￠?*/
		if(!pBandwidth && noFlag == TRUE){
			EIGRP_FUNC_LEAVE(EigrpConfigIntfBw);
			return FAILURE;
		}
	}else {
		if((!pBandwidth && noFlag == TRUE) || (noFlag == FALSE && pBandwidth && pBandwidth->bandwidth == bandwidth)){
			
			EIGRP_FUNC_LEAVE(EigrpConfigIntfBw);
			return FAILURE;
		}
	}

	if(!pBandwidth){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfBw);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pBandwidth = EigrpPortMemMalloc(sizeof(EigrpConfIntfBandwidth_st));
		if(!pBandwidth){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfBw);
			return FAILURE;
		}
		
		if(pConfIntf->bw){
			pConfIntf->bw->prev	= pBandwidth;
		}
		
		pBandwidth->prev = NULL;
		pBandwidth->next = pConfIntf->bw;
		
		pConfIntf->bw	= pBandwidth;

		pBandwidth->asNum	= asNum;
		pBandwidth->bandwidth= bandwidth;
	}else{
		if(noFlag == FALSE){
			pBandwidth->bandwidth	= bandwidth;
		}else{
			if(pConfIntf->bw ==  pBandwidth){
				pConfIntf->bw	= pBandwidth->next;
			}

			if(pBandwidth->prev){
				pBandwidth->prev->next = pBandwidth->next;
			}

			if(pBandwidth->next){
				pBandwidth->next->prev = pBandwidth->prev;
			}
			
			EigrpPortMemFree((void *)pBandwidth);
		}
	}	
	EIGRP_FUNC_LEAVE(EigrpConfigIntfBw);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfBandwidth

	Desc:	This function is to change the bandwidth for an given interface and a given eigrp 
			router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			bandwidth	- the new bandwidth
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/
S32	EigrpConfigIntfDelay(S32 noFlag, U8 *ifName, U32 asNum, U32 delay)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfDelay_pt	pDelay;

	EIGRP_FUNC_ENTER(EigrpConfigIntfDelay);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pDelay = NULL;
	if(pConfIntf){
		for(pDelay = pConfIntf->delay; pDelay; pDelay = pDelay->next){
			if(pDelay->asNum == asNum){
				break;
			}
		}
	}

	/*?T??D?|ì¨¤¨o???¨?¨|pal?a¨o?*/
	if(SUCCESS== EigrpIpProcessIfCheckUnnumberd(ifName)){
		/*?T??D?|ì¨¤????¨a??é¨o?à?¨?2?¨oy?¨2stopFrp|ì¨¨2¨′?á??¨o?à?ê????????¨¨?¨|¨¨???ê?eigrpIntfList2??à?ê¨￠?*/
		if(!pDelay && noFlag == TRUE){			
			EIGRP_FUNC_LEAVE(EigrpConfigIntfDelay);
			return FAILURE;
		}
	}else {
		if((!pDelay && noFlag == TRUE) || (noFlag == FALSE && pDelay && pDelay->delay == delay)){			
			EIGRP_FUNC_LEAVE(EigrpConfigIntfDelay);
			return FAILURE;
		}
	}

	if(!pDelay){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfDelay);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pDelay = EigrpPortMemMalloc(sizeof(EigrpConfIntfDelay_st));
		if(!pDelay){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfDelay);
			return FAILURE;
		}
		
		if(pConfIntf->delay){
			pConfIntf->delay->prev	= pDelay;
		}
		
		pDelay->prev = NULL;
		pDelay->next = pConfIntf->delay;
		
		pConfIntf->delay	= pDelay;

		pDelay->asNum	= asNum;
		pDelay->delay= delay;
	}else{
		if(noFlag == FALSE){
			pDelay->delay	= delay;
		}else{
			if(pConfIntf->delay ==  pDelay){
				pConfIntf->delay	= pDelay->next;
			}

			if(pDelay->prev){
				pDelay->prev->next = pDelay->next;
			}

			if(pDelay->next){
				pDelay->next->prev = pDelay->prev;
			}
			
			EigrpPortMemFree((void *)pDelay);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfDelay);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfHello

	Desc:	This function is to change the hello interval for an given interface and a given eigrp
			router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			hello		- the new hello interval
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfHello(S32 noFlag, U8 *ifName, U32 asNum, U32 hello)
{
	EigrpConfIntf_pt		pConfIntf;
	EigrpConfIntfHello_pt	pHello;
	EigrpConfIntfHold_pt	pHold;

	EIGRP_FUNC_ENTER(EigrpConfigIntfHello);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pHello = NULL;
	if(pConfIntf){
		for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
			if(pHello->asNum == asNum){
				break;
			}
		}
	}

	pHold	= NULL;
	if(pConfIntf){
		for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
			if(pHold->asNum == asNum){
				break;
			}
		}
	}

#if 0
	if(noFlag == FALSE){
		if(!pHold && 3 * hello * EIGRP_MSEC_PER_SEC > EIGRP_DEF_HOLDTIME){
			printf("\nIllegal helloInterval, it is more than a third of holdtime.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
			return FAILURE;

		}

		if(pHold && 3 * hello > pHold->hold){
			printf("\nIllegal helloInterval, it is more than a third of holdtime.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
			return FAILURE;
		}
	}
#else
	if(noFlag == FALSE){
		if(!pHold && hello * EIGRP_MSEC_PER_SEC > EIGRP_DEF_HOLDTIME){
			_EIGRP_DEBUG("\nIllegal helloInterval, it is more than holdtime.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
			return FAILURE;

		}

		if(pHold && hello > pHold->hold){
			_EIGRP_DEBUG("\nIllegal helloInterval, it is more than holdtime.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
			return FAILURE;
		}
	}
#endif

	if((!pHello && noFlag == TRUE) || (noFlag == FALSE && pHello && pHello->hello == hello)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
		return FAILURE;
	}

	if(!pHello){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pHello = EigrpPortMemMalloc(sizeof(EigrpConfIntfHello_st));
		if(!pHello){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
			return FAILURE;
		}
		
		if(pConfIntf->hello){
			pConfIntf->hello->prev	= pHello;
		}
		
		pHello->prev = NULL;
		pHello->next = pConfIntf->hello;
		
		pConfIntf->hello	= pHello;

		pHello->asNum	= asNum;
		pHello->hello		= hello;
	}else{
		if(noFlag == FALSE){
			pHello->hello	= hello;
		}else{
			if(pConfIntf->hello ==  pHello){
				pConfIntf->hello	= pHello->next;
			}

			if(pHello->prev){
				pHello->prev->next = pHello->next;
			}

			if(pHello->next){
				pHello->next->prev = pHello->prev;
			}

			EigrpPortMemFree((void *)pHello);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfHello);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfHold

	Desc:	This function is to change the holding time for an given interface and a given eigrp 
			router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			hold		- the new holding time
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfHold(S32 noFlag, U8 *ifName, U32 asNum, U32 hold)
{
	EigrpConfIntf_pt		pConfIntf;
	EigrpConfIntfHold_pt	pHold;
	EigrpConfIntfHello_pt	pHello;

	EIGRP_FUNC_ENTER(EigrpConfigIntfHold);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pHold = NULL;
	if(pConfIntf){
		for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
			if(pHold->asNum == asNum){
				break;
			}
		}
	}
	
	pHello	= NULL;
	if(pConfIntf){
		for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
			if(pHello->asNum == asNum){
				break;
			}
		}
	}

#if 0
	if(noFlag == FALSE){
		if(!pHello && hold * EIGRP_MSEC_PER_SEC < 3 * EIGRP_DEF_HELLOTIME){
			printf("\nIllegal holdtime, it is less than treble hello-interval.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
			return FAILURE;

		}

		if(pHello && hold < 3 * pHello->hello){
			printf("\nIllegal holdtime, it is less than treble hello-interval.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
			return FAILURE;
		}
	}
#else
	if(noFlag == FALSE){
		if(!pHello && hold * EIGRP_MSEC_PER_SEC < EIGRP_DEF_HELLOTIME){
			_EIGRP_DEBUG("\nIllegal holdtime, it is less than hello-interval.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
			return FAILURE;

		}

		if(pHello && hold < pHello->hello){
			_EIGRP_DEBUG("\nIllegal holdtime, it is less than hello-interval.\n");
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
			return FAILURE;
		}
	}
#endif

	if((!pHold && noFlag == TRUE) || (noFlag == FALSE && pHold && pHold->hold == hold)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
		return FAILURE;
	}

	if(!pHold){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pHold = EigrpPortMemMalloc(sizeof(EigrpConfIntfHold_st));
		if(!pHold){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
			return FAILURE;
		}
		
		if(pConfIntf->hold){
			pConfIntf->hold->prev	= pHold;
		}
		
		pHold->prev = NULL;
		pHold->next = pConfIntf->hold;
		
		pConfIntf->hold	= pHold;

		pHold->asNum	= asNum;
		pHold->hold		= hold;
	}else{
		if(noFlag == FALSE){
			pHold->hold	= hold;
		}else{
			if(pConfIntf->hold ==  pHold){
				pConfIntf->hold	= pHold->next;
			}

			if(pHold->prev){
				pHold->prev->next = pHold->next;
			}

			if(pHold->next){
				pHold->next->prev = pHold->prev;
			}
			EigrpPortMemFree((void *)pHold);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfHold);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfSplit

	Desc:	This function is to enable /disable the horizon split on a given interface and a given 
			eigrp router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			noFlag	- the flag of negative command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfSplit(S32 noFlag, U8 *ifName, U32 asNum)
{
	EigrpConfIntf_pt		pConfIntf;
	EigrpConfIntfSplit_pt	pSplit;
	S32 split;

	EIGRP_FUNC_ENTER(EigrpConfigIntfSplit);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pSplit = NULL;
	if(pConfIntf){
		for(pSplit = pConfIntf->split; pSplit; pSplit = pSplit->next){
			if(pSplit->asNum == asNum){
				break;
			}
		}
	}

	if(noFlag == FALSE){
		split	= TRUE;
	}else{
		split	= FALSE;
	}
#ifdef _EIGRP_PLAT_MODULE
	if( (pSplit) && (pSplit->split == split)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfSplit);
		return SUCCESS;
	}
#else//_EIGRP_PLAT_MODULE
	if((!pSplit && split == EIGRP_DEF_SPLITHORIZON) || (pSplit && pSplit->split == split)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfSplit);
		return FAILURE;
	}
#endif//_EIGRP_PLAT_MODULE

	if(!pSplit){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfSplit);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pSplit = EigrpPortMemMalloc(sizeof(EigrpConfIntfSplit_st));
		if(!pSplit){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfSplit);
			return FAILURE;
		}
		
		if(pConfIntf->split){
			pConfIntf->split->prev	= pSplit;
		}
		
		pSplit->prev = NULL;
		pSplit->next = pConfIntf->split;
		
		pConfIntf->split	= pSplit;

		pSplit->asNum	= asNum;
		pSplit->split		= split;
	}else{
		if(split == EIGRP_DEF_SPLITHORIZON){
			if(pConfIntf->split ==  pSplit){
				pConfIntf->split	= pSplit->next;
			}

			if(pSplit->prev){
				pSplit->prev->next = pSplit->next;
			}

			if(pSplit->next){
				pSplit->next->prev = pSplit->prev;
			}

			EigrpPortMemFree((void *)pSplit);
		}else{
			pConfIntf->split =  pSplit;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfSplit);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfSummary

	Desc:	This function is to add /remove a manual summary address on a given interface and a
			given eigrp router. 
		
	Para: 	ifName	- pointer to the string which indicate the name of the given interface
			asNum	- the autonomous system number
			noFlag	- the flag of negative command
			ipAddr	- the summary ip address
			ipMask	- the ip mask of the summary ip address
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigIntfSummary(S32 noFlag, U8 *ifName, U32 asNum, U32 ipAddr, U32 ipMask)
{
	EigrpConfIntf_pt		pConfIntf;
	EigrpConfIntfSum_pt	pSummary;

	EIGRP_FUNC_ENTER(EigrpConfigIntfSummary);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, ifName)){
			break;
		}
	}
	
	pSummary = NULL;
	if(pConfIntf){
		for(pSummary = pConfIntf->summary; pSummary; pSummary = pSummary->next){
			if(pSummary->asNum == asNum 
				&& (pSummary->ipNet == (ipAddr & ipMask)) && pSummary->ipMask == ipMask){
				break;
			}
		}
	}

	if((!pSummary && noFlag == TRUE) || (pSummary && noFlag == FALSE)){
		EIGRP_FUNC_LEAVE(EigrpConfigIntfSummary);
		return FAILURE;
	}

	if(!pSummary){
		if(!pConfIntf){
			pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
			if(!pConfIntf	){	
				EIGRP_FUNC_LEAVE(EigrpConfigIntfSummary);
				return FAILURE;
			}

			if(gpEigrp->conf->pConfIntf){
				gpEigrp->conf->pConfIntf->prev	= pConfIntf;
			}

			pConfIntf->prev = NULL;
			pConfIntf->next = gpEigrp->conf->pConfIntf;

			gpEigrp->conf->pConfIntf	= pConfIntf;
			EigrpPortStrCpy(pConfIntf->ifName, ifName);
		}

		pSummary = EigrpPortMemMalloc(sizeof(EigrpConfIntfSum_st));
		if(!pSummary){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfSummary);
			return FAILURE;
		}
		
		if(pConfIntf->summary){
			pConfIntf->summary->prev	= pSummary;
		}
		
		pSummary->prev = NULL;
		pSummary->next = pConfIntf->summary;
		
		pConfIntf->summary	= pSummary;

		pSummary->asNum	= asNum;
		pSummary->ipNet		= ipAddr & ipMask;
		pSummary->ipMask	= ipMask;
	}else{
		if(pConfIntf->summary == pSummary){
			pConfIntf->summary	= pSummary->next;
		}

		if(pSummary->prev){
			pSummary->prev->next = pSummary->next;
		}

		if(pSummary->next){
			pSummary->next->prev = pSummary->prev;
		}

		EigrpPortMemFree((void *)pSummary);
	}
	EIGRP_FUNC_LEAVE(EigrpConfigIntfSummary);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigIntfUaiP2mp

	Desc:	
		
	Para: 	
	
	Ret:		
************************************************************************************/
S32	EigrpConfigIntfUaiP2mp(S32 noFlag, U8 *seiName)
{
	EigrpConfIntf_pt		pConfIntf;

	EIGRP_FUNC_ENTER(EigrpConfigIntfUaiP2mp);
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, seiName)){
			break;
		}
	}
	
	if(!pConfIntf){
		if(noFlag == TRUE){
			EIGRP_FUNC_LEAVE(EigrpConfigIntfUaiP2mp);
			return SUCCESS;
		}
		
		pConfIntf	= EigrpPortMemMalloc(sizeof(EigrpConfIntf_st));
		if(!pConfIntf){	
			EIGRP_FUNC_LEAVE(EigrpConfigIntfUaiP2mp);
			return FAILURE;
		}

		if(gpEigrp->conf->pConfIntf){
			gpEigrp->conf->pConfIntf->prev	= pConfIntf;
		}

		pConfIntf->prev = NULL;
		pConfIntf->next = gpEigrp->conf->pConfIntf;

		gpEigrp->conf->pConfIntf	= pConfIntf;
		EigrpPortStrCpy(pConfIntf->ifName, seiName);

		pConfIntf->IsSei	= TRUE;
	}else{
		if(noFlag == FALSE){
			pConfIntf->IsSei	= TRUE;
		}else{
			pConfIntf->IsSei	= FALSE;
		}
	}
	
	EIGRP_FUNC_LEAVE(EigrpConfigIntfUaiP2mp);
	
	return SUCCESS;
}


/************************************************************************************

	Name:	EigrpConfigAsRedis

	Desc:	This function is to add /remove a routing protocol redistribution on a given eigrp router. 
		
	Para: 	asNum	- the autonomous system number
			noFlag	- the flag of negative command
			protoName	- pointer to the string which indicate the name of the routing protocol
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigAsRedis(S32 noFlag, U32 asNum, struct EigrpRedis_ *redis_info, U32 againflag)
{
	EigrpConfAs_pt		pConfAs;
	EigrpConfAsRedis_pt	pRedis;
	EigrpPdb_st		*pdb;
	U8	*protoName;
	U32	srcProc;
	EIGRP_FUNC_ENTER(EigrpConfigAsRedis);

	protoName	= EigrpProto2str(redis_info->srcProto);
	srcProc		= redis_info->srcProc;
	if(!EigrpPortStrCmp(protoName, "unknown")){
		return FAILURE;
	}
	
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}
	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigAsRedis);
		return FAILURE;
	}

	for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
		if(!EigrpPortStrCmp(pRedis->protoName, protoName) && pRedis->srcProc == srcProc){
			break;
		}
	}

	if(noFlag == TRUE){
		if(!pRedis){
			EIGRP_FUNC_LEAVE(EigrpConfigAsRedis);
			return FAILURE;
		}

		if(pConfAs->redis == pRedis){
			pConfAs->redis = pRedis->next;
		}
		if(pRedis->next){
			pRedis->next->prev	= pRedis->prev;
		}

		if(pRedis->prev){
			pRedis->prev->next	= pRedis->next;
		}

		EigrpPortMemFree((void *)pRedis);
	}else{
		if(pRedis){
			if(againflag){
				if(pConfAs->redis == pRedis){
					pConfAs->redis = pRedis->next;
				}
				if(pRedis->next){
					pRedis->next->prev	= pRedis->prev;
				}

				if(pRedis->prev){
					pRedis->prev->next	= pRedis->next;
				}

				EigrpPortMemFree((void *)pRedis);
				pdb	= EigrpIpFindPdb(asNum);
				if(!pdb){
					return FAILURE;
				}
				EigrpIpProcessRedisCommand(TRUE, pdb, redis_info);
				EigrpPortSleep(500);//zhurish
				//taskDelay(500);
				
			}else{
				EIGRP_FUNC_LEAVE(EigrpConfigAsRedis);
				return FAILURE;
			}
		}
		
		pRedis	= EigrpPortMemMalloc(sizeof(EigrpConfAsRedis_st));
		if(!pRedis){	
			EIGRP_FUNC_LEAVE(EigrpConfigAsRedis);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pRedis);
		EigrpPortStrCpy(pRedis->protoName, protoName);
		pRedis->srcProc	= srcProc;
		
		if(pConfAs->redis){
			pConfAs->redis->prev	= pRedis;
		}
		pRedis->next	= pConfAs->redis;
		pRedis->prev	= NULL;

		pConfAs->redis	= pRedis;
	}
	EIGRP_FUNC_LEAVE(EigrpConfigAsRedis);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigAsDefRouteIn

	Desc:	This function is to Accept default routing information
		
	Para: 	asNum	- the autonomous system number
			noFlag	- the flag of negative command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigAsDefRouteIn(S32 noFlag, U32 asNum)
{
	EigrpConfAs_pt	pConfAs;
	S32 defRouteIn;

	EIGRP_FUNC_ENTER(EigrpConfigAsDefRouteIn);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}
	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteIn);
		return FAILURE;
	}
	
	if(noFlag == FALSE){
		defRouteIn	= TRUE;
	}else{
		defRouteIn	= FALSE;
	}
#ifdef _EIGRP_PLAT_MODULE
	if( (pConfAs->defRouteIn) && (*pConfAs->defRouteIn == defRouteIn) ) {
		EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteIn);
		return SUCCESS;
	}
#else//_EIGRP_PLAT_MODULE
	if((!pConfAs->defRouteIn && defRouteIn == EIGRP_DEF_DEFROUTE_IN) || 
				(pConfAs->defRouteIn && *pConfAs->defRouteIn == defRouteIn)){
		EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteIn);
		return FAILURE;
	}
#endif//_EIGRP_PLAT_MODULE
	if(!pConfAs->defRouteIn){
		pConfAs->defRouteIn	= (S32 *)EigrpPortMemMalloc(sizeof(S32));
		if(!pConfAs->defRouteIn){	
			EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteIn);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs->defRouteIn);

		*pConfAs->defRouteIn	= defRouteIn;
	}else{
		if(defRouteIn != EIGRP_DEF_DEFROUTE_IN){
			*pConfAs->defRouteIn =  defRouteIn;
			EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteIn);
			return SUCCESS;
		}

		EigrpPortMemFree((void *)pConfAs->defRouteIn);
		pConfAs->defRouteIn	= NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteIn);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpConfigAsDefRouteOut

	Desc:	This function is to Output default routing information
		
	Para: 	asNum	- the autonomous system number
			noFlag	- the flag of negative command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpConfigAsDefRouteOut(S32 noFlag, U32 asNum)
{
	EigrpConfAs_pt	pConfAs;
	S32 defRouteOut;

	EIGRP_FUNC_ENTER(EigrpConfigAsDefRouteOut);
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		if(pConfAs->asNum == asNum){
			break;
		}
	}
	if(!pConfAs){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteOut);
		return FAILURE;
	}
	
	if(noFlag == FALSE){
		defRouteOut	= TRUE;
	}else{
		defRouteOut	= FALSE;
	}
#ifdef _EIGRP_PLAT_MODULE
	if((pConfAs->defRouteOut && *pConfAs->defRouteOut == defRouteOut)){
		EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteOut);
		return SUCCESS;
	}
#else// _EIGRP_PLAT_MODULE
	if((!pConfAs->defRouteOut && defRouteOut == EIGRP_DEF_DEFROUTE_OUT) || 
				(pConfAs->defRouteOut && *pConfAs->defRouteOut == defRouteOut)){
		EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteOut);
		return FAILURE;
	}
#endif// _EIGRP_PLAT_MODULE
	if(!pConfAs || !pConfAs->defRouteOut){
		pConfAs->defRouteOut	= (S32 *)EigrpPortMemMalloc(sizeof(S32));
		if(!pConfAs->defRouteOut){	
			EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteOut);
			return FAILURE;
		}
		EIGRP_ASSERT((U32)pConfAs->defRouteOut);

		*pConfAs->defRouteOut	= defRouteOut;
	}else{
		if(defRouteOut != EIGRP_DEF_DEFROUTE_OUT){
			*pConfAs->defRouteOut =  defRouteOut;
			EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteOut);
			return SUCCESS;
		}

		EigrpPortMemFree((void *)pConfAs->defRouteOut);
		pConfAs->defRouteOut	= NULL;
	}

	EIGRP_FUNC_LEAVE(EigrpConfigAsDefRouteOut);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdProc

	Desc:	This function is to actually process every command.
		
	Para: 	pCmd	- pointer to the command
	
	Ret:		NONE
************************************************************************************/

void	EigrpCmdProc(EigrpCmd_pt pCmd)
{
	EigrpPdb_st		*pdb;
	EigrpIntf_pt		pEigrpIntf;
	EigrpDbgCtrl_pt	pDbgCtrl;
	U32	msgnum;
	S32	retVal = FAILURE;
	EIGRP_FUNC_ENTER(EigrpCmdProc);
	pdb	= NULL;
#ifdef _EIGRP_PLAT_MODULE		
	zebraEigrpCmdSetupResponse(NULL, FAILURE, FALSE);
#endif// _EIGRP_PLAT_MODULE		
	switch(pCmd->type){
#ifdef _EIGRP_PLAT_MODULE
		case	EIGRP_CMD_TYPE_ROUTER_ID:
			if(pCmd->noFlag == FALSE){
				zebraEigrpIpSetRouterId(pCmd->asNum, pCmd->u32Para1);
			}else{
				zebraEigrpIpSetRouterId(pCmd->asNum, pCmd->u32Para1);
			}
			retVal = SUCCESS;
			break;
#endif/* _EIGRP_PLAT_MODULE	 */		
		case	EIGRP_CMD_TYPE_ROUTER:
			retVal	= EigrpConfigRouter(pCmd->noFlag, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}
			
			if(pCmd->noFlag == FALSE){
				EigrpProcRouterEigrp(pCmd->asNum);
			}else{
				EigrpProcNoRouterEigrp(pCmd->asNum);
			}
			break;

		case	EIGRP_CMD_TYPE_AUTOSUM:
			retVal	= EigrpConfigAutoSummary(pCmd->noFlag, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			if(pCmd->noFlag == FALSE){
				/* To enable  the auto summary */
				EigrpIpProcessAutoSummaryCommand(pdb, TRUE);
			}else{
				/* To disable  the auto summary */
				EigrpIpProcessAutoSummaryCommand(pdb, FALSE);
			}
			break;

		case	EIGRP_CMD_TYPE_DEF_METRIC:
			retVal	= EigrpConfigDefMetric(pCmd->noFlag, pCmd->asNum, (EigrpVmetric_st *)pCmd->vsPara1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			EigrpIpProcessDefaultmetricCommand(pCmd->noFlag, pdb, (EigrpVmetric_st *)pCmd->vsPara1);
			break;

		case	EIGRP_CMD_TYPE_DISTANCE:
			retVal	= EigrpConfigDistance(pCmd->noFlag, pCmd->asNum, pCmd->u32Para1, pCmd->u32Para2);
			if(retVal == FAILURE){
				break;
			}
				
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			EigrpIpProcessDistanceCommand(pdb, pCmd->u32Para1, pCmd->u32Para2);
			break;

		case	EIGRP_CMD_TYPE_WEIGHT:
			retVal	= EigrpConfigMetricWeight(pCmd->noFlag, pCmd->asNum, pCmd->u32Para1, pCmd->u32Para2,
												pCmd->u32Para3, pCmd->u32Para4, pCmd->u32Para5);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			if(pCmd->noFlag == FALSE){
				if(pdb->k1 == pCmd->u32Para1 && 	pdb->k2 == pCmd->u32Para2 &&
							pdb->k3 == pCmd->u32Para3 && pdb->k4 == pCmd->u32Para4 &&
							pdb->k5 == pCmd->u32Para5){
					break;
				}

				pdb->k1	= pCmd->u32Para1;
				pdb->k2	= pCmd->u32Para2;
				pdb->k3	= pCmd->u32Para3;
				pdb->k4	= pCmd->u32Para4;
				pdb->k5	= pCmd->u32Para5;
			}else{
				if(pdb->k1 == EIGRP_K1_DEFAULT && pdb->k2 == EIGRP_K2_DEFAULT &&
							pdb->k3 == EIGRP_K3_DEFAULT && pdb->k4 == EIGRP_K4_DEFAULT &&
							pdb->k5 == EIGRP_K5_DEFAULT){
					break;
				}

				pdb->k1	= EIGRP_K1_DEFAULT;
				pdb->k2	= EIGRP_K2_DEFAULT;
				pdb->k3	= EIGRP_K3_DEFAULT;
				pdb->k4	= EIGRP_K4_DEFAULT;
				pdb->k5	= EIGRP_K5_DEFAULT;
			}
			EigrpIpProcessMetricweightCommand(pdb);
			break;
  
		case	EIGRP_CMD_TYPE_NETWORK:
			retVal	= EigrpConfigNetwork(pCmd->noFlag, pCmd->asNum, pCmd->u32Para1, pCmd->u32Para2);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			
			//设置默认路由ID
#ifdef _EIGRP_PLAT_MODULE
			//EigrpIpSetRouterId(pdb->ddb, pCmd->u32Para1, pCmd->u32Para2);
#else//_EIGRP_PLAT_MODULE
			//EigrpIpSetRouterId(pdb->ddb);
#endif//_EIGRP_PLAT_MODULE
			//if((pdb->ddb)->routerId == EIGRP_IPADDR_ZERO)
			//	(pdb->ddb)->routerId = pCmd->u32Para1;
			///	
			EigrpIpProcessNetworkCommand(pCmd->noFlag, pdb, pCmd->u32Para1, pCmd->u32Para2);
			break;

		case	EIGRP_CMD_TYPE_INTF_PASSIVE:
			retVal	= EigrpConfigIntfPassive(pCmd->noFlag, pCmd->ifName, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}

			EigrpIpProcessPassiveIntfCommand(pCmd->noFlag, pdb, pEigrpIntf);
			break;

		case	EIGRP_CMD_TYPE_INTF_INVISIBLE:
			retVal	= EigrpConfigIntfInvisible(pCmd->noFlag, pCmd->ifName, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}

			EigrpIpProcessInvisibleIntfCommand(pCmd->noFlag, pdb, pEigrpIntf);
			break;

		case	EIGRP_CMD_TYPE_NEI:
			retVal	= EigrpConfigNeighbor(pCmd->noFlag, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessNeighborCommand(pCmd->noFlag, pdb, pCmd->u32Para1);
			break;

		case	EIGRP_CMD_TYPE_INTF_AUTHKEY:
			retVal	= EigrpConfigIntfAuthKey(pCmd->noFlag, pCmd->ifName, pCmd->asNum, (U8 *)pCmd->vsPara1);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetAuthenticationKey(pdb, pCmd->ifName, (U8 *)pCmd->vsPara1);
			break;

		case	EIGRP_CMD_TYPE_INTF_AUTHKEYID:
			retVal	= EigrpConfigIntfAuthid(pCmd->noFlag, pCmd->ifName, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetAuthenticationKeyidCommand(pdb, pCmd->ifName, pCmd->u32Para1);
			break;

		case	EIGRP_CMD_TYPE_INTF_AUTHMODE:
			retVal	= EigrpConfigIntfAuthMode(pCmd->noFlag, pCmd->ifName, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetAuthenticationMode(pCmd->noFlag, pdb, pCmd->ifName);
			break;

		case	EIGRP_CMD_TYPE_INTF_BANDWIDTH_PERCENT:
			retVal	= EigrpConfigIntfBandwidth(pCmd->noFlag, pCmd->ifName, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetBandwidthPercentCommand(pdb, pCmd->ifName, pCmd->u32Para1);
			break;

		case	EIGRP_CMD_TYPE_INTF_BANDWIDTH:
			retVal	= EigrpConfigIntfBw(pCmd->noFlag, pCmd->ifName, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetBandwidthCommand(pCmd->noFlag, pdb, pCmd->ifName, pCmd->u32Para1);
			break;
			
		case	EIGRP_CMD_TYPE_INTF_DELAY:
			retVal	= EigrpConfigIntfDelay(pCmd->noFlag, pCmd->ifName, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetDelayCommand(pCmd->noFlag, pdb, pCmd->ifName, pCmd->u32Para1);
			break;
			
#ifdef _EIGRP_UNNUMBERED_SUPPORT
		case	EIGRP_CMD_TYPE_INTF_UUAI_PARAM:
			EigrpIpProcessIfSetUUaiBandwidthCommand(pCmd->asNum, pCmd->ifName);
			break;
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
			
		case	EIGRP_CMD_TYPE_INTF_HELLO:
			retVal	= EigrpConfigIntfHello(pCmd->noFlag, pCmd->ifName, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetHelloIntervalCommand(pCmd->noFlag, pdb, pCmd->ifName, pCmd->u32Para1);
			break;

		case	EIGRP_CMD_TYPE_INTF_HOLD:
			retVal	= EigrpConfigIntfHold(pCmd->noFlag, pCmd->ifName, pCmd->asNum, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetHoldtimeCommand(pCmd->noFlag, pdb, pCmd->ifName, pCmd->u32Para1);
			break;

		case	EIGRP_CMD_TYPE_INTF_SPLIT:
			retVal	= EigrpConfigIntfSplit(pCmd->noFlag, pCmd->ifName, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSetSplitHorizonCommand(pCmd->noFlag, pdb, pCmd->ifName);
			break;

		case	EIGRP_CMD_TYPE_INTF_SUM:
			retVal	= EigrpConfigIntfSummary(pCmd->noFlag, pCmd->ifName, pCmd->asNum,  
													pCmd->u32Para1, pCmd->u32Para2);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessIfSummaryAddressCommand(pCmd->noFlag, pdb, pCmd->ifName, 
													pCmd->u32Para1, pCmd->u32Para2);
			break;
#ifdef _EIGRP_UNNUMBERED_SUPPORT
		case	EIGRP_CMD_TYPE_INTF_UAI_P2MP:
			retVal	= EigrpConfigIntfUaiP2mp(pCmd->noFlag, pCmd->ifName);
			if(retVal == FAILURE){
				break;
			}
			EigrpIpProcessIfUaiP2mp(pCmd->noFlag, pCmd->ifName, 0);
			break;
#endif /* _EIGRP_UNNUMBERED_SUPPORT */

		case	EIGRP_CMD_TYPE_NET_METRIC:
			EigrpIpProcessNetMetricCommand(pCmd->noFlag, pCmd->u32Para1, pCmd->vsPara1);
			retVal = SUCCESS;
			break;

#ifdef _EIGRP_VLAN_SUPPORT 
		case	EIGRP_CMD_TYPE_INTF_UAI_P2MP_VLAN_ID:
			EigrpIpProcessIfUaiP2mp(pCmd->noFlag, NULL, pCmd->u32Para1);
			break;
#endif /* _EIGRP_VLAN_SUPPORT */

#ifdef _EIGRP_UNNUMBERED_SUPPORT
		case	EIGRP_CMD_TYPE_INTF_UAI_P2MP_NEI:
			EigrpIpProcessIfUaiP2mp_Nei(pCmd->noFlag, pCmd->u32Para1, pCmd->u32Para2);
			break;
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */	
		case	EIGRP_CMD_TYPE_REDIS:
			retVal	= EigrpConfigAsRedis(pCmd->noFlag, pCmd->asNum, pCmd->vsPara1, pCmd->u32Para1);
			if(retVal == FAILURE){
				break;
			}
			
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessRedisCommand(pCmd->noFlag, pdb, pCmd->vsPara1);
			break;

		case	EIGRP_CMD_TYPE_DEF_ROUTE_IN:
			retVal	= EigrpConfigAsDefRouteIn(pCmd->noFlag, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessDefaultRouteCommand(pCmd->noFlag, pdb, TRUE);
			break;

		case	EIGRP_CMD_TYPE_DEF_ROUTE_OUT:
			retVal	= EigrpConfigAsDefRouteOut(pCmd->noFlag, pCmd->asNum);
			if(retVal == FAILURE){
				break;
			}

			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}

			EigrpIpProcessDefaultRouteCommand(pCmd->noFlag, pdb, FALSE);
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, FALSE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF_DETAIL:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32(*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, TRUE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF_SINGLE:
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32(*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, TRUE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF_AS:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32(*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, TRUE);
			retVal = SUCCESS;	
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32(*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, TRUE);
			retVal = SUCCESS;	
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF_AS_DETAIL:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32(*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, TRUE);
			retVal = SUCCESS;	
			break;

		case	EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE_DETAIL:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
				EigrpShowInterface(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32(*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, TRUE);
			retVal = SUCCESS;	
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, FALSE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI_AS:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, FALSE);
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI_DETAIL:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, TRUE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI_INTF:
			 pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, FALSE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI_INTF_DETAIL:
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, TRUE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
			EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, FALSE);
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF_DETAIL:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			pEigrpIntf = EigrpIntfFindByName(pCmd->ifName);
			 if(!pEigrpIntf){
			 	retVal = FAILURE;
				break;
			}
			EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, pEigrpIntf, TRUE);
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_SHOW_NEI_AS_DETAIL:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			EigrpShowNeighbors(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, NULL, TRUE);
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TOPO_ALL:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowTopology(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, EIGRP_TOP_ALL);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TOPO_SUM:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowTopology(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, EIGRP_TOP_SUMMARY);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TOPO_ACT:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowTopology(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, EIGRP_TOP_ACTIVE);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TOPO_FS:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowTopology(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, EIGRP_TOP_FS);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TOPO_AS_ALL:
			pdb	= EigrpIpFindPdb(pCmd->asNum);
			if(!pdb){
				retVal = FAILURE;
				break;
			}
			EigrpShowTopology(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, EIGRP_TOP_ALL);
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TOPO_AS_ZERO:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowTopology(pCmd->name, pCmd->vsPara1, pCmd->u32Para1,
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb, EIGRP_TOP_ZERO);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_TRAFFIC:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpShowTraffic(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb->ddb);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_PROTOCOL:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpUtilShowProtocol(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_ROUTE:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpUtilShowRoute(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_STRUCT:
			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				EigrpUtilShowStruct(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2, pdb);
			}
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_SHOW_DEBUG:
			EigrpUtilShowDebugInfo(pCmd->name, pCmd->vsPara1, pCmd->u32Para1, 
									(S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2);
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_SEND_UPDATE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_SEND_UPDATE;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_SEND_UPDATE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_SEND_UPDATE);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_SEND_QUERY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_SEND_QUERY;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_SEND_QUERY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_SEND_QUERY);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_DBG_SEND_REPLY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_SEND_REPLY;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_SEND_REPLY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_SEND_REPLY);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_DBG_SEND_HELLO:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_SEND_HELLO;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_SEND_HELLO:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_SEND_HELLO);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_SEND_ACK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_SEND_ACK;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_SEND_ACK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_SEND_ACK);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			 break;
		 
		case	EIGRP_CMD_TYPE_DBG_SEND:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
					if(!pDbgCtrl){
						retVal = FAILURE;
						break;
					}
			
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_SEND;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_SEND:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_SEND);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;


		case	EIGRP_CMD_TYPE_DBG_RECV_UPDATE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_RECV_UPDATE;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_RECV_UPDATE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_RECV_UPDATE);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;


		case	EIGRP_CMD_TYPE_DBG_RECV_QUERY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_RECV_QUERY;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_RECV_QUERY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_RECV_QUERY);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_DBG_RECV_REPLY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_RECV_REPLY;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_RECV_REPLY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_RECV_REPLY);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

	
		case	EIGRP_CMD_TYPE_DBG_RECV_HELLO:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_RECV_HELLO;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_RECV_HELLO:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_RECV_HELLO);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_RECV_ACK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_RECV_ACK;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_RECV_ACK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_RECV_ACK);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_RECV:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_RECV;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_RECV:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_RECV);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;


		case	EIGRP_CMD_TYPE_DBG_PACKET:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_NO_DBG_PACKET:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_DETAIL;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_DETAIL);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_UPDATE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_DETAIL_UPDATE;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_UPDATE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_DETAIL_UPDATE);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_QUERY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_DETAIL_QUERY;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_QUERY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_DETAIL_QUERY);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_REPLY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_DETAIL_REPLY;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_REPLY:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_DETAIL_REPLY);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_HELLO:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_DETAIL_HELLO;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_HELLO:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_DETAIL_HELLO);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_ACK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_PACKET_DETAIL_ACK;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_ACK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_PACKET_DETAIL_ACK);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

			
		case	EIGRP_CMD_TYPE_DBG_EVENT:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_EVENT;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_EVENT:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_EVENT);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_DBG_TIMER:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_TIMER;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_TIMER:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_TIMER);

			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_TASK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_TASK;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_TASK:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_TASK);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_DBG_ROUTE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_ROUTE;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_ROUTE:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				retVal = FAILURE;
				break;
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_ROUTE);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_DBG_INTERNAL:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}

			}
			pDbgCtrl->flag |= DEBUG_EIGRP_INTERNAL;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_INTERNAL:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}

			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_INTERNAL);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		case	EIGRP_CMD_TYPE_DBG_ALL:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag |= DEBUG_EIGRP_ALL;
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;

		case	EIGRP_CMD_TYPE_NO_DBG_ALL:
			pDbgCtrl	= EigrpUtilDbgCtrlFind(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
			if(!pDbgCtrl){
				pDbgCtrl	= EigrpUtilDbgCtrlCreate(pCmd->name, pCmd->vsPara1, pCmd->u32Para1);
				if(!pDbgCtrl){
					retVal = FAILURE;
					break;
				}
			}
			pDbgCtrl->flag &= (~DEBUG_EIGRP_ALL);
			pDbgCtrl->funcShow	= (S32 (*)(U8 *, void *, U32, U32, U8 *))pCmd->vsPara2;
			retVal = SUCCESS;
			break;
			
		default:
			EIGRP_ASSERT(0);
			retVal = FAILURE;
			break;
	}
#ifdef _EIGRP_PLAT_MODULE		
	zebraEigrpCmdSetupResponse(pCmd, retVal, FALSE);
#endif// _EIGRP_PLAT_MODULE		
	EigrpPortMemFree(pCmd);
	if(pdb){
		(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgnum);
		if(msgnum){
			EigrpIpLaunchPdbJob(pdb);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpCmdProc);
	return;
}

/************************************************************************************

	Name:	EigrpCmdApiAuthKeyStr

	Desc:	This function process "eigrp auth keychain" command for interface ifName, set the 
			shared password to KeyStr, enable MD5 auth.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
			keyStr	- pointer to the string which the shared password will be set to 
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
			noFlag		- the flag of negative command
************************************************************************************/

S32	EigrpCmdApiAuthKeyStr(S32 noFlag, U32 asNum, U8 *ifName, U8 *keyStr)
{
	EigrpCmd_pt		pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	pCmd->type		= EIGRP_CMD_TYPE_INTF_AUTHKEY;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;
	
	if(noFlag == FALSE){
		pCmd->vsPara1	= EigrpPortMemMalloc(EigrpPortStrLen(keyStr) + 1);
		if(!pCmd->vsPara1){	
			EigrpPortMemFree(pCmd);
			return FAILURE;
		}
		EigrpPortStrCpy((U8 *)pCmd->vsPara1, keyStr);
	}else{
		pCmd->vsPara1	= NULL;
	}
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);
	
#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiAuthKeyid

	Desc:	This function process "eigrp auth keyid" command, set MD5 auth keyid for interface
			ifName to keyId. 
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
			keyId	- the keyid for the interface
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiAuthKeyid(S32 noFlag, U32 asNum, U8 *ifName, U32 keyId)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_AUTHKEYID;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;

	if(noFlag == FALSE){
		pCmd->u32Para1	= keyId;
	}else{
		pCmd->u32Para1	= EIGRP_AUTH_KEYID_DEF;
	}

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);
#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiAuthMd5mode

	Desc:	This function process "eigrp auth mode md5" command, set auth mode to MD5 for interface
			ifName.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiAuthMd5mode(S32 noFlag, U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_AUTHMODE;
	pCmd->noFlag	= noFlag;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiBwPercent

	Desc:	This function process "eigrp bandwidth" command, limit bandwidth used by eigrp protocol
			of interface ifName to percent%. 
		
	Para:	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
			percent	- the percent of the bandwidth used by eigrp protocol of interface
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiBwPercent(S32 noFlag, U32 asNum, U8 *ifName, U32 percent)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_BANDWIDTH_PERCENT;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;

	if(noFlag == FALSE){
		pCmd->u32Para1	= percent;
	}else{
		pCmd->u32Para1	= 0;
	}

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiBandwidth

	Desc:	This function process "eigrp bandwidth" command, limit bandwidth used by eigrp protocol
			of interface ifName to percent%. 
		
	Para:	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
			percent	- the percent of the bandwidth used by eigrp protocol of interface
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiBandwidth(S32 noFlag, U32 asNum, U8 *ifName, U32 bw)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);

	if(!EigrpPortMemCmp(ifName, "uai", 3) || !EigrpPortMemCmp(ifName, "UAI", 3)){
		_EIGRP_DEBUG("It  can not set bandwidth on UAI interface, please set on SEI(vlan) interface.");
		return FAILURE;
	}

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_BANDWIDTH;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;

	if(noFlag == FALSE){
		pCmd->u32Para1	= bw;
	}else{
		pCmd->u32Para1	= EIGRP_DEF_IF_BANDWIDTH;
	}

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	usrAppSetBandwidth(S32 noFlag, U32 asNum, U8 *ifName, U32 bw)
{
	if(asNum == 0){
		return FAILURE;
	}
	
	return EigrpCmdApiBandwidth(noFlag, asNum, ifName, bw);
}

/************************************************************************************

	Name:	EigrpCmdApiBandwidth

	Desc:	This function process "eigrp bandwidth" command, limit bandwidth used by eigrp protocol
			of interface ifName to percent%. 
		
	Para:	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
			percent	- the percent of the bandwidth used by eigrp protocol of interface
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/
#ifdef _EIGRP_UNNUMBERED_SUPPORT
S32	EigrpCmdApiUUaiParam(U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_UUAI_PARAM;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */
/************************************************************************************

	Name:	EigrpCmdApiBandwidth

	Desc:	This function process "eigrp bandwidth" command, limit bandwidth used by eigrp protocol
			of interface ifName to percent%. 
		
	Para:	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
			percent	- the percent of the bandwidth used by eigrp protocol of interface
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiDelay(S32 noFlag, U32 asNum, U8 *ifName, U32 delay)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);

	if(!EigrpPortMemCmp(ifName, "uai", 3) || !EigrpPortMemCmp(ifName, "UAI", 3)){
		_EIGRP_DEBUG("It  can not set bandwidth on UAI interface, please set on SEI(vlan) interface.");
		return FAILURE;
	}

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_DELAY;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;

	if(noFlag == FALSE){
		pCmd->u32Para1	= delay;
	}else{
		pCmd->u32Para1	= EIGRP_DEF_IF_DELAY * 25600;
	}

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	usrAppSetDelay(S32 noFlag, U32 asNum, U8 *ifName, U32 delay)
{
	if(asNum == 0){
		return FAILURE;
	}
	
	return EigrpCmdApiDelay(noFlag, asNum, ifName, delay);
}

/************************************************************************************

	Name:	EigrpCmdApiHelloInterval

	Desc:	This function process "eigrp hello interval" command, set hello interval of interface  
 			ifName to interval seconds.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name

			interval	- the hello interval
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiHelloInterval(S32 noFlag, U32 asNum, U8 *ifName, U32 interval)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	pCmd->type		= EIGRP_CMD_TYPE_INTF_HELLO;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;
	if(noFlag == FALSE){
		pCmd->u32Para1	= interval;
	}else{
		pCmd->u32Para1	= 0;
	}

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}



/************************************************************************************

	Name:	EigrpCmdApiHoldtime

	Desc:	This function process "eigrp holdTime" command, set holdTime of interface ifName to 
			holdTime seconds.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name

			holdTime	- holdtime of interface
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiHoldtime(S32 noFlag, U32 asNum, U8 *ifName, U32 holdTime)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_HOLD;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->noFlag	= noFlag;
	if(noFlag == FALSE){
		pCmd->u32Para1	= holdTime;
	}else{
		pCmd->u32Para1	= 0;
	}

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}



/************************************************************************************

	Name:	EigrpCmdApiSplitHorizon

	Desc:	This function process "eigrp splitHorizon" command, enable splitHorizon  of interface  
			ifName.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name

	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiSplitHorizon(S32 noFlag, U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_SPLIT;
	pCmd->noFlag	= noFlag;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiSummaryAddress

	Desc:	This function process "eigrp summary address" command,configure summary address 
			ipAddr/ipMask for interface ifName.  
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name

			ipAddr	- the summary  ip address
			ipMask	- the summary ip mask
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiSummaryAddress(S32 noFlag, U32 asNum, U8 *ifName, U32 ipAddr, U32 ipMask)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_SUM;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	pCmd->u32Para1	= ipAddr & ipMask;
	pCmd->u32Para2	= ipMask;
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiSeiPort

	Desc:	This function process "eigrp sei port" command, set an ethernet port
			to "switch ethernet interface"(SEI).  
		
	Para: 	



	
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/
#ifdef _EIGRP_UNNUMBERED_SUPPORT
S32	EigrpCmdApiUaiP2mpPort(S32 noFlag, U8 *seiName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_UAI_P2MP;
	EigrpPortStrCpy(pCmd->ifName, seiName);
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiUaiP2mpPort_vlanid(S32 noFlag, U32 vlan_id)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_UAI_P2MP_VLAN_ID;
	pCmd->u32Para1 = vlan_id;
	pCmd->noFlag	= noFlag;

	if(vlan_id <= 14){
		if(!noFlag){
			;//gpEigrp->vlanFlag[vlan_id] = 1;
		}else{
			;//gpEigrp->vlanFlag[vlan_id] = 0;
		}
	}
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiUaiP2mpPort_Nei(S32 noFlag, U32 nei, U32 vlan_id)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_UAI_P2MP_NEI;
	pCmd->u32Para1 = nei;
	pCmd->u32Para2 = vlan_id;
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
S32	usrAppUaiP2mpSetNei(S32 noFlag, U32 nei, U32 vlan_id)
{
	return EigrpCmdApiUaiP2mpPort_Nei(noFlag, nei, vlan_id);
}
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
/************************************************************************************

	Name:	EigrpCmdApiRouteTypeAtoi

	Desc:	This function return route type given route type name. 
		
	Para: 	str	- pointer to the string which indicate the given route type name
	
	Ret:		route type 
************************************************************************************/

S32	EigrpCmdApiRouteTypeAtoi(S8 *str)
{
	S32 i, rtType = -1;
		
	static struct    
	{
	        S32 type;
	        S32 str_min_len;
	        S8 *str;
	}     redist_type[]	= {
		{EIGRP_ROUTE_KERNEL, 1, "kernel"},
		{EIGRP_ROUTE_CONNECT, 1, "connected"},
		{EIGRP_ROUTE_STATIC, 1, "static"},
		{EIGRP_ROUTE_RIP, 1, "rip"},
		{EIGRP_ROUTE_OSPF, 1, "ospf"},
		{EIGRP_ROUTE_IGRP, 1, "igrp"},
		{EIGRP_ROUTE_EIGRP, 1, "eigrp"},
		{EIGRP_ROUTE_BGP, 1, "bgp"},
		{0, 0, NULL}      };

	EIGRP_FUNC_ENTER(EigrpCmdApiRouteTypeAtoi);
	for(i = 0; redist_type[ i ].str; i++){
		if(EigrpPortMemCmp((U8 *)redist_type[ i ].str, (U8 *)str, redist_type[ i ].str_min_len) != 0){
	    		continue;
		}

		rtType = redist_type[ i ].type;
		break;
	}
	EIGRP_FUNC_LEAVE(EigrpCmdApiRouteTypeAtoi);

	return rtType;
}

/************************************************************************************

	Name:	EigrpCmdApiRedistType

	Desc:	This function process "redistribute {kernel|connected|static|rip|igrp|ospf}" command.
		
	Para: 	asNum	- the autonomous system number
			name	- pointer to the string which indicate the redistributed route name
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiRedistType(S32 noFlag, U32 asNum, U8 *name, U32 srcProc, U32 againflag)
{
	EigrpRedis_st *redis_info;
	S32 rtType = -1;
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	
	rtType = EigrpCmdApiRouteTypeAtoi(name);
	if(rtType == -1){
		_EIGRP_DEBUG("Unsupported type %s%s", name, EIGRP_NEW_LINE_STR);
		return FAILURE;
	}
	if(rtType == EIGRP_ROUTE_EIGRP && srcProc == asNum){
		return	FAILURE;
	}
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	redis_info = (EigrpRedis_st *) EigrpPortMemMalloc(sizeof(EigrpRedis_st));
	if(!redis_info){            
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}

	redis_info->srcProto	= rtType;
	redis_info->srcProc	= srcProc;
	redis_info->dstProto	= EIGRP_ROUTE_EIGRP;
	redis_info->dstProc	= asNum;
	BIT_SET(redis_info->flag, EIGRP_REDISTRIBUTE_FLAG_TYPE); 

	pCmd->type		= EIGRP_CMD_TYPE_REDIS;
	pCmd->asNum	= asNum;
	pCmd->vsPara1	= (void *)redis_info;
	pCmd->noFlag	= noFlag;
	pCmd->u32Para1	= againflag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}



/************************************************************************************

	Name:	EigrpCmdApiNetMetric

	Desc:	This function process "set eigrp-net A.B.C.D metric <1-1000000000> 
			" command.
		
	Para: 	noFlag	- negative metric
			ipNet		- the dstination network from eigrp
			vMetric	- 0-delay 1-bw 2-mtu 3-hopcount 4-reliability 5-load
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiNetMetric(S32 noFlag, U32 ipNet, void *vMetric)
{
	EigrpCmd_pt	pCmd;
	EigrpVmetric_st	*pVmetric;
	S32 retVal;

	EIGRP_ASSERT(ipNet);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	pVmetric = EigrpPortMemMalloc(sizeof(EigrpVmetric_st));
	EigrpPortMemSet((U8 *)pVmetric, 0, sizeof(EigrpVmetric_st));
	EigrpPortMemCpy((U8 *)pVmetric, (U8 *)vMetric, sizeof(EigrpVmetric_st));
	pCmd->noFlag		= noFlag;
	pCmd->type		= EIGRP_CMD_TYPE_NET_METRIC;
	pCmd->u32Para1	= ipNet;
	pCmd->vsPara1	= pVmetric;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiRedistTypeRoutemap

	Desc:	This function process "redistribute {kernel|connected|static|rip|igrp|ospf|bgp} 
			route-map NAME" command.
		
	Para: 	asNum	- the autonomous system number
			name	- pointer to the string which indicate the redistributed route name
			rtMap	- pointer to the given route map name
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiRedistTypeRoutemap(S32 noFlag, U32 asNum, U8 *name, U8 *rtMap)
{
	EigrpRedis_st *redis_info;
	S32 rtType = -1;
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	redis_info = (EigrpRedis_st *) EigrpPortMemMalloc(sizeof(EigrpRedis_st));
	if(!redis_info){            
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}
	rtType = EigrpCmdApiRouteTypeAtoi(name);
	if(rtType == -1){
		EigrpPortMemFree(redis_info);
		EigrpPortMemFree(pCmd);
		_EIGRP_DEBUG("Unsupported type %s%s", name, EIGRP_NEW_LINE_STR);
		return FAILURE;
	}       

	redis_info->srcProto	= rtType;
	redis_info->srcProc	= 0;
	redis_info->dstProto	= EIGRP_ROUTE_EIGRP;
	redis_info->dstProc	= asNum;
	EigrpPortMemCpy(redis_info->rtMapName, rtMap, EIGRP_MAX_ROUTEMAPNAME_LEN);
	redis_info->rtMapName[EIGRP_MAX_ROUTEMAPNAME_LEN]	= '\0';
	BIT_SET(redis_info->flag, EIGRP_REDISTRIBUTE_FLAG_RTMAP);

	pCmd->type		= EIGRP_CMD_TYPE_REDIS;
	pCmd->asNum	= asNum;
	pCmd->vsPara1	= (void *)redis_info;
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiRedistTypeMetric

	Desc:	This function process "redistribute {kernel|connected|static|rip|igrp|ospf|bgp} metric
			<1> <2> <3> <4> <5>" command, set redistribute route vector metric 
		
	Para: 	asNum	- the autonomous system number
			name	- pointer to the string which indicate the redistributed route name
			m1,m2,m3,m4,m5		- the redistribute route vector metric
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiRedistTypeMetric(S32 noFlag, U32 asNum, U8 *name, U32 m1, U32 m2, U32 m3, U32 m4, U32 m5)
{
	EigrpRedis_st *redis_info;
	S32 rtType = -1;
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	redis_info = (EigrpRedis_st *) EigrpPortMemMalloc(sizeof(EigrpRedis_st));
	if(!redis_info){            
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}
	rtType = EigrpCmdApiRouteTypeAtoi(name);
	if(rtType == -1){
		EigrpPortMemFree(redis_info);
		EigrpPortMemFree(pCmd);
		_EIGRP_DEBUG("Unsupported type %s%s", name, EIGRP_NEW_LINE_STR);
		return FAILURE;
	}       

	redis_info->srcProto		= rtType;
	redis_info->srcProc	= 0;
	redis_info->dstProto		= EIGRP_ROUTE_EIGRP;
	redis_info->dstProc	= asNum;

	redis_info->vmetric[0]	= m1;
	redis_info->vmetric[1]	= m2;
	redis_info->vmetric[2]	= m3;
	redis_info->vmetric[3]	= m4;
	redis_info->vmetric[4]	= m5;
	BIT_SET(redis_info->flag, EIGRP_REDISTRIBUTE_FLAG_METRIC);

	pCmd->type		= EIGRP_CMD_TYPE_REDIS;
	pCmd->asNum	= asNum;
	pCmd->vsPara1	= (void *)redis_info;
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiRedistTypeMetricRoutemap

	Desc:	This function process "redistribute {kernel|connected|static|rip|igrp|ospf|bgp} metric 
			<1> <2> <3> <4> <5> route-map NAME " command
		
	Para: 	asNum	- the autonomous system number
			name	- pointer to the string which indicate the redistributed route name
			m1,m2,m3,m4,m5		- the redistribute route vector metric
			rtMap	- pointer to the route map name
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiRedistTypeMetricRoutemap(U32 asNum, U8 *name, 
													U32 m1, U32 m2, U32 m3, U32 m4, U32 m5,
													U8 *rtMap)
{
	EigrpRedis_st *redis_info;
	S32 rtType = -1;
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	redis_info = (EigrpRedis_st *) EigrpPortMemMalloc(sizeof(EigrpRedis_st));
	if(!redis_info){            
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}
	rtType = EigrpCmdApiRouteTypeAtoi(name);
	if(rtType == -1){
		EigrpPortMemFree(redis_info);
		EigrpPortMemFree(pCmd);
		_EIGRP_DEBUG("Unsupported type %s%s", name, EIGRP_NEW_LINE_STR);
		return FAILURE;
	}       

	redis_info->srcProto	= rtType;
	redis_info->srcProc	= 0;
	redis_info->dstProto	= EIGRP_ROUTE_EIGRP;
	redis_info->dstProc	= asNum;

	redis_info->vmetric[0]	= m1;
	redis_info->vmetric[1]	= m2;
	redis_info->vmetric[2]	= m3;
	redis_info->vmetric[3]	= m4;
	redis_info->vmetric[4]	= m5;
	EigrpPortMemCpy(redis_info->rtMapName, rtMap, EIGRP_MAX_ROUTEMAPNAME_LEN);
	redis_info->rtMapName[EIGRP_MAX_ROUTEMAPNAME_LEN]	= '\0';
	BIT_SET(redis_info->flag, EIGRP_REDISTRIBUTE_FLAG_MTRT);

	pCmd->type		= EIGRP_CMD_TYPE_REDIS;
	pCmd->asNum	= asNum;
	pCmd->vsPara1	= (void *)redis_info;
	pCmd->noFlag	= FALSE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiDefaultRouteIn

	Desc:	This function process "default-information in" command, enable eigrp process asNum 
			receive default route update packet.
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiDefaultRouteIn(S32 noFlag ,U32 asNum)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_DEF_ROUTE_IN;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDefaultRouteOut

	Desc:	This function process "default-information out" command, enable eigrp process asNum 
 			send default route update packet.
			
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiDefaultRouteOut(S32 noFlag, U32 asNum)
{
	EigrpCmd_pt	pCmd;

	EIGRP_ASSERT(asNum);
	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_DEF_ROUTE_OUT;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiRouterEigrp

	Desc:	This function called directly by cli, process "router eigrp <as>" command, start a
			eigrp process as 
			
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiRouterEigrp(S32 noFlag, U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_ROUTER;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}
/************************************************************************************

	Name:	EigrpCmdApiAutosummary

	Desc:	This function process "auto-summary" command, enable auto summary of eigrp process 
			asNum
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiAutosummary(S32 noFlag, U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_AUTOSUM;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiDefaultMetric

	Desc:	This function called by cli, process "default-metric <bandwidth> <delay> <reliability> 
			<loading> <mtu>" command 
		
	Para: 	asNum	- the autonomous system number
			band	- bandwidth of the default vector metric
			delay	- delay of the default vector metric
			reli		- reliability of the default vector metric
			load		- the load of the default vector metric
			mtu		- the max transmit unit of the default vector metric
			
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiDefaultMetric(S32 noFlag, U32 asNum, U32 band, U32 delay, U32 reli, U32 load, U32 mtu)
{
	EigrpCmd_pt	pCmd;
	EigrpVmetric_st *vmetric;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	vmetric = EigrpPortMemMalloc(sizeof(EigrpVmetric_st));
	if(!vmetric){
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}
	if(noFlag == FALSE){
		vmetric->bandwidth	= band;
		vmetric->delay		= delay;
		vmetric->reliability		= reli;
		vmetric->load			= load;
		vmetric->mtu		= mtu;
	}
	
	pCmd->type		= EIGRP_CMD_TYPE_DEF_METRIC;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	pCmd->vsPara1	= (void *)vmetric;
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDistance

	Desc:	This function process "distance <interior> <exterior>" command, set administrator 
			distance of interior and exterior of eigrp route
		
	Para: 	asNum	- the autonomous system number
			inter		- the interior distance
			exter	- the exterior distance
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
			
************************************************************************************/

S32	EigrpCmdApiDistance(S32 noFlag, U32 asNum, U32 inter, U32 exter)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_DISTANCE;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;

	if(noFlag == FALSE){
		pCmd->u32Para1	= inter;
		pCmd->u32Para2	= exter;
	}else{
		pCmd->u32Para1	= EIGRP_DEF_DISTANCE_INTERNAL;
		pCmd->u32Para2	= EIGRP_DEF_DISTANCE_EXTERNAL;
	}
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiMetricWeights

	Desc:	This function process "metric weights <tos> <k1> <k2> <k3> <k4> <k5>" command Set
			K-values
		
	Para: 	asNum	- the autonomous system number
			k1,k2,k3,k4,k5	- the K value
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiMetricWeights(S32 noFlag, U32 asNum, U32 k1, U32 k2, U32 k3, U32 k4, U32 k5)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_WEIGHT;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	if(noFlag == FALSE){
		pCmd->u32Para1	= k1;
		pCmd->u32Para2	= k2;
		pCmd->u32Para3	= k3;
		pCmd->u32Para4	= k4;
		pCmd->u32Para5	= k5;
	}else{
		pCmd->u32Para1	= EIGRP_K1_DEFAULT;
		pCmd->u32Para2	= EIGRP_K2_DEFAULT;
		pCmd->u32Para3	= EIGRP_K3_DEFAULT;
		pCmd->u32Para4	= EIGRP_K4_DEFAULT;
		pCmd->u32Para5	= EIGRP_K5_DEFAULT;
	}
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);
	
#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiNetwork

	Desc:	This function process "network <IP>" command, enable network for eigrp 
		
	Para: 	asNum	- the autonomous system number
			ipAddr	- the ip address of the given network
			ipMask	- the ip mask of the given mask
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiNetwork(S32 noFlag, U32 asNum, U32 ipAddr, U32 ipMask)
{
	EigrpCmd_pt	pCmd;

	if(!ipMask){
		_EIGRP_DEBUG("Invalid ip mask!\n");
		return FAILURE;
	}
	
	if(~ipMask & (~ipMask + 1)){
		_EIGRP_DEBUG("Invalid ip mask!\n");
		return FAILURE;
	}

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_NETWORK;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	
	pCmd->u32Para1	= ipAddr & ipMask;
	pCmd->u32Para2	= ipMask;

	if(!pCmd->u32Para1){
		_EIGRP_DEBUG("Invalid net address!\n");
		EigrpPortMemFree(pCmd);
		return FAILURE;
	}
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiPassiveInterface

	Desc:	This function process "eigrp asNum passive-interface" command, set the interface to 
			passive mode.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiPassiveInterface(S32 noFlag, U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_PASSIVE;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiInvisibleInterface

	Desc:	This function process "eigrp asNum invisible-interface" command, set the interface to 
			passive mode.
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the interface name
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiInvisibleInterface(S32 noFlag, U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_INTF_INVISIBLE;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	EigrpPortStrCpy(pCmd->ifName, ifName);
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNeighbor

	Desc:	This function process "neighbor <IP>" command.
		
	Para: 	asNum	- the autonomous system number
			ipAddr	- the data for the command
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiNeighbor(S32 noFlag, U32 asNum, U32 ipAddr)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_NEI;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	pCmd->u32Para1	= ipAddr;
	
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}



S32	EigrpCmdApiShowDebuggingEigrpByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_DEBUG;
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;

}

/************************************************************************************

	Name:	EigrpCmdApiShowDebuggingEigrp

	Desc:	This function is to show the status of the debug switches of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiShowDebuggingEigrp()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowDebuggingEigrpByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacket

	Desc:	This function is enable the packet debuging of eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacket()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketDetailByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET_DETAIL;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
	
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketDetail

	Desc:	This function is enable the detail debuging of eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketDetail()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketDetailByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketDetailByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
	
}

S32	EigrpCmdApiNoDebugPacketDetail()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketDetailByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}
	
	return SUCCESS;
}
S32	EigrpCmdApiNoDebugPacketByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacket

	Desc:	This function is to disable the packet debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacket()
{
	S32	retVal;

	retVal	= EigrpCmdApiNoDebugPacketByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketSendByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_SEND;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketSend

	Desc:	This function is to enable the all packet-send debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketSend()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketSendByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketSendUpdateByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_SEND_UPDATE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketSendUpdate

	Desc:	This function is to enable the packet-send-update debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketSendUpdate()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketSendUpdateByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketSendQueryByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_SEND_QUERY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiDebugPacketSendQuery

	Desc:	This function is to enable the packet-send-query debugging of the eigrp. 
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketSendQuery()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketSendQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketSendReplyByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_SEND_REPLY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

  /************************************************************************************

	Name:	EigrpCmdApiDebugPacketSendReply

	Desc:	This function is to enable the packet-send-reply debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	- for success
			FAILURE		- for failure
************************************************************************************/

S32	EigrpCmdApiDebugPacketSendReply()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketSendQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketSendHelloByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_SEND_HELLO;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketSendHello
	
	Desc:	This function is to enable the packet-send-hello debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketSendHello()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketSendHelloByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketSendAckByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_SEND_ACK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketSendAck

	Desc:	This function is enable the packet-send-ack debugging of the eigrp.
		
	Para: 
	
	Ret:		
************************************************************************************/

S32	EigrpCmdApiDebugPacketSendAck()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketSendAckByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}


S32	EigrpCmdApiNoDebugPacketSendByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_SEND;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketSend

	Desc:	This function is to disable all the packet-send debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketSend()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketSendByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketSendUpdateByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_SEND_UPDATE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketSendUpdate

	Desc:	This function is 
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketSendUpdate()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketSendUpdateByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketSendQueryByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;


	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_SEND_QUERY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketSendQuery

	Desc:	This function is to disable the packet-send-query debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketSendQuery()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketSendQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketSendReplyByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;
	

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_SEND_REPLY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketSendReply

	Desc:	This function is to disable the packet-send-reply debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketSendReply()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketSendReplyByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketSendHelloByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_SEND_HELLO;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketSendHello

	Desc:	This function is to disable the packet-send-hello debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketSendHello()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketSendHelloByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketSendAckByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;
	
	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_SEND_ACK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketSendAck

	Desc:	This function is to disable the packet-send-ack debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketSendAck()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketSendAckByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketRecvByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_RECV;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketRecv

	Desc:	This function is to enable the all packet-receive debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketRecv()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketRecvByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketRecvUpdateByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_RECV_UPDATE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketRecvUpdate

	Desc:	This function is to enbale the packet-receive-update debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketRecvUpdate()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketRecvUpdateByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketRecvQueryByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_RECV_QUERY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketRecvQuery

	Desc:	This function is to enable the packet-receive-query debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketRecvQuery()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketRecvQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketRecvReplyByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_RECV_REPLY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketRecvReply

	Desc:	This function is to enable the packet-receive-reply debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketRecvReply()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketRecvReplyByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketRecvHelloByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_RECV_HELLO;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketRecvHello

	Desc:	This function is to enable the packet-receive-hello debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketRecvHello()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketRecvHelloByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketRecvAckByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_RECV_ACK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);
#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugPacketRecvAck

	Desc:	This function is to enable the packet-receive-ack debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugPacketRecvAck()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketRecvAckByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketRecvByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_RECV;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketRecv

	Desc:	This function is disable all the packet-receive debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketRecv()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketRecvByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketRecvUpdateByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_RECV_UPDATE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketRecvUpdate

	Desc:	This function is to disable the packet-receive-update debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketRecvUpdate()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketRecvUpdateByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketRecvQueryByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_RECV_QUERY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketRecvQuery

	Desc:	This function is to disable the packet-receive-query debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketRecvQuery()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketRecvQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketRecvReplyByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_RECV_REPLY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketRecvReply

	Desc:	This function is to disable the packet-receive-reply debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketRecvReply()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketRecvReplyByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketRecvHelloByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_RECV_HELLO;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketRecvHello

	Desc:	This function is to disable the packet-receive-hello debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketRecvHello()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketRecvHelloByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketRecvAckByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_RECV_ACK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugPacketRecvAck

	Desc:	This function is to disable the packet-receive-ack debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugPacketRecvAck()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketRecvAckByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}


S32	EigrpCmdApiDebugPacketDetailUpdateByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_UPDATE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiDebugPacketDetailUpdate

	Desc:	This function is to enable update detail debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/



S32	EigrpCmdApiDebugPacketDetailUpdate()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketDetailUpdateByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketDetailQueryByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_QUERY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiDebugPacketDetailQuery()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketDetailQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketDetailReplyByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_REPLY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiDebugPacketDetailReply()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketDetailReplyByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketDetailHelloByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_HELLO;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiDebugPacketDetailHello()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketDetailHelloByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiDebugPacketDetailAckByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_ACK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiDebugPacketDetailAck()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugPacketDetailAckByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketDetailUpdateByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_UPDATE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiNoDebugPacketDetailUpdate()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketDetailUpdateByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketDetailQueryByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_QUERY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiNoDebugPacketDetailQuery()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketDetailQueryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketDetailReplyByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_REPLY;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiNoDebugPacketDetailReply()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketDetailReplyByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketDetailHelloByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_HELLO;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiNoDebugPacketDetailHello()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketDetailHelloByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	

	return SUCCESS;
}

S32	EigrpCmdApiNoDebugPacketDetailAckByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_ACK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

S32	EigrpCmdApiNoDebugPacketDetailAck()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugPacketDetailAckByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	return SUCCESS;
}

S32	EigrpCmdApiDebugEventByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_EVENT;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */

}

/************************************************************************************

	Name:	EigrpCmdApiDebugEvent

	Desc:	This function is to enable all the event debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugEvent()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugEventByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiNoDebugEventByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_EVENT;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugEvent

	Desc:	This function is to disable all the event debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugEvent()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugEventByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugTimerByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_TIMER;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiDebugTimer

	Desc:	This function is to enable all the timer debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugTimer()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugTimerByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiNoDebugTimerByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_TIMER;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}


/************************************************************************************

	Name:	EigrpCmdApiNoDebugTimer

	Desc:	This function is to disable all the timer debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugTimer()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugTimerByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugRouteByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_ROUTE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugRoute

	Desc:	This function is to enable all the route debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugRoute()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugRouteByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiNoDebugRouteByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_ROUTE;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugRoute

	Desc:	This function is to disable all the route debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugRoute()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugRouteByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugTaskByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_TASK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugTask

	Desc:	This function is to enable the task debugging of the eigrp. 
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugTask()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugTaskByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiNoDebugTaskByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_TASK;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugTask

	Desc:	This function is to disable the task debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugTask()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugTaskByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugAllByTerm(U8 *name, void *pTerm, U32 id,  S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_ALL;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugAll

	Desc:	This function is to enable all the debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugAll()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugAllByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiNoDebugAllByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_ALL;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugAll

	Desc:	This function is to disable all the debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugAll()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugAllByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiDebugInternalByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_DBG_INTERNAL;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiDebugInternal

	Desc:	This function is to enable the internal debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiDebugInternal()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiDebugInternalByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiNoDebugInternalByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	pCmd->type		= EIGRP_CMD_TYPE_NO_DBG_INTERNAL;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

#ifdef _EIGRP_PLAT_MODULE
	return zebraEigrpCmdResponse();
#else	
	return SUCCESS;
#endif /* _EIGRP_PLAT_MODULE */
}

/************************************************************************************

	Name:	EigrpCmdApiNoDebugInternal

	Desc:	This function is to disable the internal debugging of the eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiNoDebugInternal()
{
	S32	retVal;
	
	retVal	= EigrpCmdApiNoDebugInternalByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}	
	
	return SUCCESS;
}

S32	EigrpCmdApiShowIntfByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF;
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntf

	Desc:	This function process "show ip eigrp interfaces" command, show summaried eigrp 
			interfaces information of all eigrp process   
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiShowIntf()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}
	
	return SUCCESS;
}

S32	EigrpCmdApiShowIntfDetailByTerm(U8 *name, void *pTerm, U32 id, S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF_DETAIL;
	
	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntfDetail

	Desc:	This function process "show ip eigrp interfaces detail" command, show detailed eigrp 
 			interfaces information of all eigrp process  
		
	Para: 	NONE
	
	Ret:		SUCCESS	
************************************************************************************/

S32	EigrpCmdApiShowIntfDetail()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfDetailByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowIntfIfByTerm(U8 *name, void *pTerm, U32 id, 
										S32 (*func)(U8 *, void *, U32, U32, U8 *), U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF_SINGLE;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntfIf

	Desc:	This function process "show ip eigrp interfaces if" command, show eigrp interface 
			infomation of interface ifName of all eigrp process
		
	Para: 	ifName	- pointer to the string which indicate the eigrp interface name
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowIntfIf(U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfIfByTerm("console", NULL, 0, NULL, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowIntfAsByTerm(U8 *name, void *pTerm, U32 id, 
										S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF_AS;
	pCmd->asNum	= asNum;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntfAs

	Desc:	This function process "show ip eigrp interfaces as" command, show eigrp interfaces
			information of process as
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowIntfAs(U32 asNum)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfAsByTerm("console", NULL, 0, NULL, asNum);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowIntfAsIfByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntfAsIf

	Desc:	This function process "show ip eigrp interfaces as if " command,, show eigrp interface
 			ifName information of process as  
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the eigrp interface name
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowIntfAsIf(U32 asNum, U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfAsIfByTerm("console", NULL, 0, NULL, asNum, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowIntfAsDetailByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF_AS_DETAIL;
	pCmd->asNum	= asNum;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntfAsDetail

	Desc:	This function process "show ip eigrp interfaces as detail" command, show detailed eigrp 
			interfaces information of process as  
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowIntfAsDetail(U32 asNum)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfAsDetailByTerm("console", NULL, 0, NULL, asNum);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowIntfAsIfDetailByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE_DETAIL;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowIntfAsIfDetail

	Desc:	This function process "show ip eigrp interfaces as if detail" command, show detailed 
			eigrp interface ifName information of process as  
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the eigrp interface name

	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowIntfAsIfDetail(U32 asNum, U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowIntfAsIfDetailByTerm("console", NULL, 0, NULL, asNum, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNei

	Desc:	This function process "show ip eigrp neighbors" command, show neighbors of all eigrp 
 			processes
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNei()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiAsByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_AS;
	pCmd->asNum	= asNum;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNeiAs

	Desc:	This function process "show ip eigrp neighbors as" command, show neighbors of eigrp 
			process as
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiAs(U32 asNum)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiAsByTerm("console", NULL, 0, NULL, asNum);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiDetailByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_DETAIL;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNeiDetail

	Desc:	This function process "show ip eigrp neighbors detail" command, show detailed neighbors
 			information of all eigrp processes
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiDetail()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiDetailByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiIfByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_INTF;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNeiIf

	Desc:	This function process "show ip eigrp neighbors if" command, show neighbors information
			of interface ifName
		
	Para: 	ifName	- pointer to the string which indicate the eigrp interface name
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiIf(U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiIfByTerm("console", NULL, 0, NULL, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiIfDetailByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_INTF_DETAIL;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNeiIfDetail

	Desc:	This function process "show ip eigrp neighbors if detail" command, show detailed 
			neighbors information of interface ifName 
		
	Para: 	ifName	- pointer to the string which indicate the eigrp interface name
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiIfDetail(U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiIfDetailByTerm("console", NULL, 0, NULL, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiAsIfByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNeiAsIf

	Desc:	This function process "show ip eigrp neighbors as if" command, show neighbors 
			information of interface ifName of eigrp process as
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the eigrp interface name
		
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiAsIf(U32 asNum, U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiAsIfByTerm("console", NULL, 0, NULL, asNum, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiAsIfDetailByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum, U8 *ifName)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF_DETAIL;
	pCmd->asNum	= asNum;
	EigrpPortStrCpy(pCmd->ifName, ifName);

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowNeiAsIfDetail

	Desc:	This function process "show ip eigrp neighbors as if detail" command, show detailed 
			neighbors information of interface ifName of eigrp process as
		
	Para: 	asNum	- the autonomous system number
			ifName	- pointer to the string which indicate the eigrp interface name
		
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiAsIfDetail(U32 asNum, U8 *ifName)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiAsIfDetailByTerm("console", NULL, 0, NULL, asNum, ifName);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowNeiAsDetailByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_NEI_AS_DETAIL;
	pCmd->asNum	= asNum;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}


/************************************************************************************

	Name:	EigrpCmdApiShowNeiAsDetail

	Desc:	This function process "show ip eigrp neighbors as detail" command, show detailed 
			neighbors  information of eigrp process as
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowNeiAsDetail(U32 asNum)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowNeiAsDetailByTerm("console", NULL, 0, NULL, asNum);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTopoByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TOPO_ALL;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTopo

	Desc:	This function process "show ip eigrp topology" command 
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTopo()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTopoByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTopoSummaryByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TOPO_SUM;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTopoSummary

	Desc:	This function process "show ip eigrp topology summary" command 
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTopoSummary()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTopoSummaryByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTopoActiveByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TOPO_ACT;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTopoActive

	Desc:	This function process "show ip eigrp topology active" command
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTopoActive()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTopoActiveByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTopoFeasibleByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TOPO_FS;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTopoFeasible

	Desc:	This function process "show ip eigrp topology feasible" command
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTopoFeasible()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTopoFeasibleByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTopoAsByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *), U32 asNum)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TOPO_AS_ALL;
	pCmd->asNum	= asNum;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTopoAs

	Desc:	This function process "show ip eigrp topology <as>" command
		
	Para: 	asNum	- the autonomous system number
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTopoAs(U32 asNum)
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTopoAsByTerm("console", NULL, 0, NULL, asNum);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTopoZeroByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TOPO_AS_ZERO;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTopoZero

	Desc:	This function process "show ip eigrp topology zero" command 
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTopoZero()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTopoZeroByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowTrafficByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_TRAFFIC;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowTraffic

	Desc:	This function process "show ip eigrp traffic" command 
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowTraffic()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowTrafficByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowProtoByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_PROTOCOL;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowProto

	Desc:	This function process "show ip protocol eigrp" command
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowProto()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowProtoByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowRouteByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_ROUTE;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowRoute

	Desc:	This function process "show ip eigrp route" command
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowRoute()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowRouteByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

S32	EigrpCmdApiShowStructByTerm(U8 *name, void *pTerm, U32 id, 
											S32 (*func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_SHOW_STRUCT;

	if(name){
		EigrpPortStrCpy(pCmd->name, name);
	}

	pCmd->vsPara1	= pTerm;
	pCmd->u32Para1	= id;
	pCmd->vsPara2	= (void *)func;

	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdApiShowStruct

	Desc:	This function process "show ip eigrp struct" command 
		
	Para: 	NONE
	
	Ret:		SUCCESS	for success
			FAILURE		for failure
************************************************************************************/

S32	EigrpCmdApiShowStruct()
{
	S32	retVal;

	retVal	= EigrpCmdApiShowStructByTerm("console", NULL, 0, NULL);
	if(retVal == FAILURE){
		return FAILURE;
	}

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpCmdEnsureSize

	Desc:	This function is to check whether the incoming buffer has enough space to contain the 
			new contents. It means that if " *bufLen - usedLen " is less than " addLen ", this
			function will apply a new space to contain all the contents.
		
	Para: 	ppBuf	- doublepointer to the incoming buffer
			bufLen	- pointer to the incomming buffer length
			usedLen	- the length of the used space
			addLen	- the length of the buffer need add
	
	Ret:		NONE
************************************************************************************/

void	EigrpCmdEnsureSize(U8 **ppBuf, U32 *bufLen, U32 usedLen, U32 addLen)
{
	U8	*temBuf;

	EIGRP_FUNC_ENTER(EigrpCmdEnsureSize);
	if(*bufLen - usedLen <= addLen){
		temBuf	= EigrpPortMemMalloc(*bufLen + addLen + 256);
		if(!temBuf){
			EigrpPortAssert(0, "EigrpCmdEnsureSize");
			EIGRP_FUNC_LEAVE(EigrpCmdEnsureSize);	
			return;
		}

		EigrpPortMemCpy(temBuf, *ppBuf, usedLen);

		*bufLen	= *bufLen + addLen + 256;
		EigrpPortMemFree(*ppBuf);
		*ppBuf	= temBuf;
	}
	EIGRP_FUNC_LEAVE(EigrpCmdEnsureSize);

	return;
}

/************************************************************************************

	Name:	EigrpCmdApiBuildRunIntfMode

	Desc:	This function is to generate the interface mode configuration command lines. All the 
			words is in the format of CISCO IOS type.
		
	Para: 	ppBuf	- doublepointer to the configuration command lines
			bufLen	- pointer to the max length of the configuration command lines
			usedLen	- pointer to the used space length
			retVal	- unused
			circId	- unused
			circName	- pointer to the interface name
	
	Ret:		NONE
************************************************************************************/

void	EigrpCmdApiBuildRunIntfMode(U8 **ppBuf, U32 *bufLen, U32 *usedLen, 
								S32 *retVal, 	U32	circId, U8 *circName)
{
	U32	cmdByte;
	U8	bufTem[512];
	
	EigrpConfIntf_pt			pConfIntf;
	EigrpConfIntfAuthkey_pt	pAuthKey;
	EigrpConfIntfAuthid_pt		pAuthId;
	EigrpConfIntfAuthMode_pt	pAuthmode;
	EigrpConfIntfPassive_pt		pPassive;
	EigrpConfIntfInvisible_pt	pInvisible;
	EigrpConfIntfBw_pt			pBandwidth;
	EigrpConfIntfHello_pt		pHello;
	EigrpConfIntfHold_pt		pHold;
	EigrpConfIntfSplit_pt		pSplit;
	EigrpConfIntfSum_pt		pSummary;

	if(!gpEigrp->conf->pConfIntf){
		return;
	}
		
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(EigrpPortStrCmp(pConfIntf->ifName, circName)){
			continue;
		}
		
		for(pAuthKey = pConfIntf->authkey; pAuthKey; pAuthKey = pAuthKey->next){
			cmdByte = sprintf(bufTem, "\teigrp %d key %s\r\n", pAuthKey->asNum, pAuthKey->auth);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pAuthId = pConfIntf->authid; pAuthId; pAuthId = pAuthId->next){
			cmdByte = sprintf(bufTem, "\teigrp %d key-id %d\r\n", pAuthId->asNum, pAuthId->authId);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pAuthmode = pConfIntf->authmode; pAuthmode; pAuthmode = pAuthmode->next){
			cmdByte = sprintf(bufTem, "\teigrp %d authMode\r\n", pAuthmode->asNum);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pPassive = pConfIntf->passive; pPassive; pPassive = pPassive->next){
			if(EIGRP_DEF_PASSIVE == FALSE){
				cmdByte = sprintf(bufTem, "\teigrp %d passive-interface\r\n", pPassive->asNum);
			}else{
				cmdByte = sprintf(bufTem, "\tno eigrp %d passive-interface\r\n", pPassive->asNum);
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pInvisible = pConfIntf->invisible; pInvisible; pInvisible = pInvisible->next){
			if(EIGRP_DEF_INVISIBLE == FALSE){
				cmdByte = sprintf(bufTem, "\tfrp %d invisible\r\n", pInvisible->asNum);
			}else{
				cmdByte = sprintf(bufTem, "\tno frp %d invisible\r\n", pInvisible->asNum);
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pBandwidth = pConfIntf->bandwidth; pBandwidth; pBandwidth = pBandwidth->next){
			cmdByte = sprintf(bufTem, "\teigrp %d bandwidth-percent %d\r\n", pBandwidth->asNum, pBandwidth->bandwidth);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
			cmdByte = sprintf(bufTem, "\teigrp %d hello-interval %d\r\n", pHello->asNum, pHello->hello);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
			cmdByte = sprintf(bufTem, "\teigrp %d hold-time %d\r\n", pHold->asNum, pHold->hold);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pSplit = pConfIntf->split; pSplit; pSplit = pSplit->next){
			if(pSplit->split == TRUE){
				cmdByte = sprintf(bufTem, "\teigrp %d split-horizon\r\n", pSplit->asNum);
			}else{
				cmdByte = sprintf(bufTem, "\tno eigrp %d split-horizon\r\n", pSplit->asNum);
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pSummary = pConfIntf->summary; pSummary; pSummary = pSummary->next){
			cmdByte = sprintf(bufTem, "\teigrp %d summary %s %s\r\n", pSummary->asNum,
								EigrpUtilIp2Str(pSummary->ipNet),
								EigrpUtilIp2Str(pSummary->ipMask));
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfIntf->IsSei == TRUE){
			cmdByte	= sprintf(bufTem, "\teigrp uai p2mp\n");
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}
	}

	return;
}

/************************************************************************************

	Name:	EigrpCmdApiBuildRunConfMode

	Desc:	This function is to generate the enable mode configuration command lines. All the words
			is in the format of CISCO IOS type.
		
	Para: 	Para: 	ppBuf	- doublepointer to the configuration command lines
			bufLen	- pointer to the max length of the configuration command lines
			usedLen	- pointer to the used space length
			retVal	- unused
			param5	- unused
	
	Ret:		NONE
************************************************************************************/

void	EigrpCmdApiBuildRunConfMode(U8 **ppBuf, U32 *bufLen, U32 *usedLen, 
								S32 *retVal, 	void *param5)
{
	EigrpConfAs_pt	pConfAs;
	EigrpConfAsNet_pt	pNet;
	EigrpConfAsNei_pt	pNei;
	U32	cmdByte;
	
	U8	bufTem[512];

	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		cmdByte	= sprintf(bufTem, "!\r\nrouter eigrp %d\r\n", pConfAs->asNum);
		EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
		EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
		*usedLen = *usedLen + cmdByte;

		for(pNet = pConfAs->network; pNet; pNet = pNet->next){
			cmdByte	= sprintf(bufTem, "\tnetwork %s %s\r\n", EigrpUtilIp2Str(pNet->ipNet),
														EigrpUtilIp2Str(pNet->ipMask));
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->autoSum && *pConfAs->autoSum != EIGRP_DEF_AUTOSUM){
			if(*pConfAs->autoSum == TRUE){
				cmdByte	= sprintf(bufTem, "\tauto-summary\r\n");
			}else{
				cmdByte	= sprintf(bufTem, "\tno auto-summary\r\n");
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->defMetric){
			cmdByte	= sprintf(bufTem, "\tdefault-metric %d %d %d %d %d\r\n",
								pConfAs->defMetric->bandwidth,
								pConfAs->defMetric->delay,
								pConfAs->defMetric->reliability,
								pConfAs->defMetric->load,
								pConfAs->defMetric->mtu);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->distance){
			cmdByte	= sprintf(bufTem, "\tdistance %d %d\r\n", pConfAs->distance->interDis, pConfAs->distance->exterDis);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->metricWeight){
			cmdByte	= sprintf(bufTem, "\tmetric weights %d %d %d %d %d\r\n", 
											pConfAs->metricWeight->k1, 
											pConfAs->metricWeight->k2, 
											pConfAs->metricWeight->k3, 
											pConfAs->metricWeight->k4, 
											pConfAs->metricWeight->k5);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pNei = pConfAs->nei; pNei; pNei = pNei->next){
			cmdByte	= sprintf(bufTem, "\tneighbor %s\r\n", EigrpUtilIp2Str(pNei->ipAddr));
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->defRouteIn && *pConfAs->defRouteIn != EIGRP_DEF_DEFROUTE_IN){
			if(*pConfAs->defRouteIn == TRUE){
				cmdByte	= sprintf(bufTem, "\tdefault-information in\r\n");
			}else{
				cmdByte	= sprintf(bufTem, "\tno default-information in\r\n");
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->defRouteOut && *pConfAs->defRouteOut != EIGRP_DEF_DEFROUTE_OUT){
			if(*pConfAs->defRouteOut == TRUE){
				cmdByte	= sprintf(bufTem, "\tdefault-information out\r\n");
			}else{
				cmdByte	= sprintf(bufTem, "\tno default-information out\r\n");
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

	}

	return; 
}

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
/************************************************************************************

	Name:	EigrpCmdApiBuildRunIntfMode_Router

	Desc:	This function is to generate the interface mode configuration command lines. All the 
			words is in the format of CISCO IOS type.
		
	Para: 	ppBuf	- doublepointer to the configuration command lines
			bufLen	- pointer to the max length of the configuration command lines
			usedLen	- pointer to the used space length
			retVal	- unused
			circId	- unused
			circName	- pointer to the interface name
	
	Ret:		NONE
************************************************************************************/

void	EigrpCmdApiBuildRunIntfMode_Router(U8 **ppBuf, U32 *bufLen, U32 *usedLen, U8 *circName)
{
	U32	cmdByte;
	U8	bufTem[512];
	
	EigrpConfIntf_pt			pConfIntf;
	EigrpConfIntfAuthkey_pt	pAuthKey;
	EigrpConfIntfAuthid_pt		pAuthId;
	EigrpConfIntfAuthMode_pt	pAuthmode;
	EigrpConfIntfPassive_pt		pPassive;
	EigrpConfIntfInvisible_pt	pInvisible;
	EigrpConfIntfBw_pt			pBandwidth;
	EigrpConfIntfBandwidth_pt		bw;
	EigrpConfIntfDelay_pt		delay;
	EigrpConfIntfHello_pt		pHello;
	EigrpConfIntfHold_pt		pHold;
	EigrpConfIntfSplit_pt		pSplit;
	EigrpConfIntfSum_pt		pSummary;

	if(!gpEigrp->conf->pConfIntf){
		return;
	}
		
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(EigrpPortStrCmp(pConfIntf->ifName, circName)){
			continue;
		}
		
		for(pAuthKey = pConfIntf->authkey; pAuthKey; pAuthKey = pAuthKey->next){
			cmdByte = sprintf(bufTem, " frp %d key %s\n", pAuthKey->asNum, pAuthKey->auth);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pAuthId = pConfIntf->authid; pAuthId; pAuthId = pAuthId->next){
			cmdByte = sprintf(bufTem, " frp %d key-id %d\n", pAuthId->asNum, pAuthId->authId);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pAuthmode = pConfIntf->authmode; pAuthmode; pAuthmode = pAuthmode->next){
			cmdByte = sprintf(bufTem, " frp %d authMode\n", pAuthmode->asNum);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pPassive = pConfIntf->passive; pPassive; pPassive = pPassive->next){
			if(EIGRP_DEF_PASSIVE == FALSE){
				cmdByte = sprintf(bufTem, " frp %d passive-interface\n", pPassive->asNum);
			}else{
				cmdByte = sprintf(bufTem, " no frp %d passive-interface\n", pPassive->asNum);
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pInvisible = pConfIntf->invisible; pInvisible; pInvisible = pInvisible->next){
			if(EIGRP_DEF_INVISIBLE == FALSE){
				cmdByte = sprintf(bufTem, " frp %d invisible\n", pInvisible->asNum);
			}else{
				cmdByte = sprintf(bufTem, " no frp %d invisible\n", pInvisible->asNum);
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pBandwidth = pConfIntf->bandwidth; pBandwidth; pBandwidth = pBandwidth->next){
			cmdByte = sprintf(bufTem, " frp %d bandwidth-percent %d\n", pBandwidth->asNum, pBandwidth->bandwidth);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(bw = pConfIntf->bw; bw; bw = bw->next){
			cmdByte = sprintf(bufTem, " frp %d bandwidth %d\n", bw->asNum, bw->bandwidth);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(delay = pConfIntf->delay; delay; delay = delay->next){
			cmdByte = sprintf(bufTem, " frp %d delay %d\n", delay->asNum, delay->delay);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}
		/*hold-time MUST before hello-interval*/
		for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
			cmdByte = sprintf(bufTem, " frp %d hold-time %d\n", pHold->asNum, pHold->hold);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		/*hello-interval MUST after hold-time*/
		for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
			cmdByte = sprintf(bufTem, " frp %d hello-interval %d\n", pHello->asNum, pHello->hello);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pSplit = pConfIntf->split; pSplit; pSplit = pSplit->next){
			if(pSplit->split == TRUE){
				cmdByte = sprintf(bufTem, " frp %d split-horizon\n", pSplit->asNum);
			}else{
				cmdByte = sprintf(bufTem, " no frp %d split-horizon\n", pSplit->asNum);
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pSummary = pConfIntf->summary; pSummary; pSummary = pSummary->next){
			cmdByte = sprintf(bufTem, " frp %d summary %s %s\n", pSummary->asNum,
								EigrpUtilIp2Str(pSummary->ipNet),
								EigrpUtilIp2Str(pSummary->ipMask));
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}
		
		if(pConfIntf->IsSei == TRUE){
			cmdByte	= sprintf(bufTem, " frp uai mp2mp\n");
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		
	}

	return;
}

/************************************************************************************

	Name:	EigrpCmdApiBuildRunConfMode_Router

	Desc:	This function is to generate the enable mode configuration command lines. All the words
			is in the format of CISCO IOS type.
		
	Para: 	Para: 	ppBuf	- doublepointer to the configuration command lines
			bufLen	- pointer to the max length of the configuration command lines
			usedLen	- pointer to the used space length
			retVal	- unused
			param5	- unused
	
	Ret:		NONE
************************************************************************************/

void	EigrpCmdApiBuildRunConfMode_Router(U8 **ppBuf, U32 *bufLen, U32 *usedLen)
{
	EigrpConfAs_pt	pConfAs;
	EigrpConfAsNet_pt	pNet;
	EigrpConfAsNei_pt	pNei;
	U32	cmdByte;	
#ifdef _EIGRP_VLAN_SUPPORT	
	U32 i;
#endif//_EIGRP_VLAN_SUPPORT	
	struct EigrpConfAsRedis_ *pRedis;
	
	U8	bufTem[512];

	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		cmdByte	= sprintf(bufTem, "!\r\nrouter frp %d\n", pConfAs->asNum);
		EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
		EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
		*usedLen = *usedLen + cmdByte;

		for(pNet = pConfAs->network; pNet; pNet = pNet->next){
			cmdByte	= sprintf(bufTem, "  network %s %s\n", EigrpUtilIp2Str(pNet->ipNet),
														EigrpUtilIp2Str(pNet->ipMask));
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->autoSum && *pConfAs->autoSum != EIGRP_DEF_AUTOSUM){
			if(*pConfAs->autoSum == TRUE){
				cmdByte	= sprintf(bufTem, "  auto-summary\n");
			}else{
				cmdByte	= sprintf(bufTem, "  no auto-summary\n");
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->redis){
			for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
				cmdByte	= sprintf(bufTem, "  redistribute %s",	pRedis->protoName);
	/** 目前命令行不支持，暂时不打开
				if(pRedis->srcProc){
					cmdByte	+= sprintf(bufTem + cmdByte, " %d", pRedis->srcProc);
				}
				
				if(pRedis->vmetric){
					cmdByte	+= sprintf(bufTem + cmdByte, " metric %d %d %d %d %d",
													pRedis->vmetric->bandwidth,
													pRedis->vmetric->delay,
													pRedis->vmetric->reliability,
													pRedis->vmetric->load,
													pRedis->vmetric->mtu);
				}
				if(pRedis->rtMapName[0]){
					cmdByte	+= sprintf(bufTem + cmdByte, " route-map %s", redis->rtMapName);
				}
	*/
				cmdByte	+=sprintf(bufTem + cmdByte, "\n");
				
				EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
				EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
				*usedLen = *usedLen + cmdByte;
			}
		}
		
		if(pConfAs->defMetric){
			cmdByte	= sprintf(bufTem, "  default-metric %d %d %d %d %d\n",
								pConfAs->defMetric->bandwidth,
								pConfAs->defMetric->delay,
								pConfAs->defMetric->reliability,
								pConfAs->defMetric->load,
								pConfAs->defMetric->mtu);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->distance){
			cmdByte	= sprintf(bufTem, "  distance %d %d\n", pConfAs->distance->interDis, pConfAs->distance->exterDis);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->metricWeight){
			cmdByte	= sprintf(bufTem, "  metric weights tos %d %d %d %d %d\n", 
											pConfAs->metricWeight->k1, 
											pConfAs->metricWeight->k2, 
											pConfAs->metricWeight->k3, 
											pConfAs->metricWeight->k4, 
											pConfAs->metricWeight->k5);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		for(pNei = pConfAs->nei; pNei; pNei = pNei->next){
			cmdByte	= sprintf(bufTem, "  neighbor %s\n", EigrpUtilIp2Str(pNei->ipAddr));
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->defRouteIn && *pConfAs->defRouteIn != EIGRP_DEF_DEFROUTE_IN){
			if(*pConfAs->defRouteIn == TRUE){
				cmdByte	= sprintf(bufTem, "  default-information in\n");
			}else{
				cmdByte	= sprintf(bufTem, "  no default-information in\n");
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

		if(pConfAs->defRouteOut && *pConfAs->defRouteOut != EIGRP_DEF_DEFROUTE_OUT){
			if(*pConfAs->defRouteOut == TRUE){
				cmdByte	= sprintf(bufTem, "  default-information out\n");
			}else{
				cmdByte	= sprintf(bufTem, "  no default-information out\n");
			}
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}

	}
#ifdef _EIGRP_VLAN_SUPPORT
	for(i = 0; i < 15; i++){
		if(gpEigrp->vlanFlag[i]){
			cmdByte	= sprintf(bufTem, "frp u-s-map vlan-id %d\n", i);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}
	}
#endif//_EIGRP_VLAN_SUPPORT
#ifdef _DC_	
	cmdByte = DcCmdApiBuildRunConfMode_Router(bufTem);
	if(cmdByte != 0){
		EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
		EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
		*usedLen = *usedLen + cmdByte;
	}
#endif
	return; 
}


#endif

#ifdef _EIGRP_PLAT_MODULE
int	zebraEigrpCmdApiRouterId(U32 noFlag, U32 asNum, U32 routeid)
{
	EigrpCmd_pt	pCmd;

	pCmd	= (EigrpCmd_pt)EigrpPortMemMalloc(sizeof(EigrpCmd_st));
	if(!pCmd){
		EIGRP_ASSERT(0);
		return FAILURE;
	}

	pCmd->type		= EIGRP_CMD_TYPE_ROUTER_ID;
	pCmd->asNum	= asNum;
	pCmd->noFlag	= noFlag;
	pCmd->u32Para1	= routeid;
	EigrpUtilQue2Enqueue(gpEigrp->cmdQue, (EigrpQueElem_st *)pCmd);

	return SUCCESS;
}
#endif/* _EIGRP_PLAT_MODULE */


