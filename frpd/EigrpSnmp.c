#include	"./include/EigrpPreDefine.h"
#ifdef INCLUDE_EIGRP_SNMP
#ifdef __cplusplus
extern "C"
{
#endif
#include	"./include/EigrpSysPort.h"
#include	"./include/EigrpCmd.h"
#include	"./include/EigrpUtil.h"
#include	"./include/EigrpDual.h"
#include	"./include/Eigrpd.h"
#include	"./include/EigrpIntf.h"
#include	"./include/EigrpIp.h"
#include	"./include/EigrpMain.h"
#include	"./include/EigrpPacket.h"
#include	"./include/EigrpSnmp.h"

extern Eigrp_pt gpEigrp;

S32 eigrpSnmpInit()
{
	S32	retVal;

	gpEigrp->snmpSock = EigrpPortSocket(0);
	if(gpEigrp->snmpSock == FAILURE){
		_EIGRP_DEBUG("error:Eigrp snmp socket creation failed.\n");
		return	FAILURE;
	}

	retVal = EigrpPortSocketFionbio(gpEigrp->snmpSock);
	if(retVal != SUCCESS){
		_EIGRP_DEBUG("error:Eigrp snmp socket set FIONBIO failed.\n");
		return	FAILURE;
	}

	retVal = EigrpPortBind(gpEigrp->snmpSock, 0, EIGRP_SNMP_PORT);
	if(retVal != SUCCESS){
		_EIGRP_DEBUG("error:Eigrp snmp socket binding failed.\n");
		return	FAILURE;
	}

	return	SUCCESS;
}

void eigrpSnmpProc()
{
	S8	buf[EIGRP_SNMP_MAX_PKT_SIZE];
	S32	recvBytes;

	recvBytes = EigrpPortRecv(gpEigrp->snmpSock, buf, EIGRP_SNMP_MAX_PKT_SIZE, 0);
	if(recvBytes > 0){
		_EIGRP_DEBUG("dbg>>eigrpSnmpProc\t: recvBytes=%d\n", recvBytes);/*debug_zhenxl*/
		eigrpSnmpReqProc(buf, recvBytes);
	}

	return;
}

S32 eigrpSnmpAsGetAsNo(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(!pdb){
		return	FAILURE;
	}

	retAs->asNo = pdb->ddb->asystem;

	return	SUCCESS;
}

S32 eigrpSnmpAsGetNextAsNo(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(!pdb || !pdb->next){
		return	FAILURE;
	}

	retAs->asNo = pdb->next->ddb->asystem;

	return	SUCCESS;
}

S32 eigrpSnmpAsGetSubNet(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;
	EigrpDualNdb_st	*dndb;
	S32		i, len;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(!pdb){
		return	FAILURE;
	}

	len = 0;
	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = pdb->ddb->topo[i]; dndb; dndb = dndb->next){
			if(len > sizeof(retAs->asSubnet) - 18){
				break;
			}
			len += sprintf(retAs->asSubnet, "%s/%d;", EigrpUtilIp2Str(dndb->dest.address), EigrpIpBitsInMask(dndb->dest.mask));
		}
		if(len > sizeof(retAs->asSubnet) - 18){
			break;
		}
	}
	retAs->asSubnet_len = len;

	return	SUCCESS;
}

S32 eigrpSnmpAsGetNextSubNet(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;
	EigrpDualNdb_st	*dndb;
	S32		i, len;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(!pdb || !pdb->next){
		return	FAILURE;
	}

	pdb = pdb->next;

	len = 0;
	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = pdb->ddb->topo[i]; dndb; dndb = dndb->next){
			if(len > sizeof(retAs->asSubnet) - 18){
				break;
			}
			len += sprintf(retAs->asSubnet, "%s/%d;", EigrpUtilIp2Str(dndb->dest.address), EigrpIpBitsInMask(dndb->dest.mask));
		}
		if(len > sizeof(retAs->asSubnet) - 18){
			break;
		}
	}
	retAs->asSubnet_len = len;

	return	SUCCESS;
}

S32 eigrpSnmpAsSetEnableNet(S32 asNo, eigrpAs_pt retAs)
{
	S32		noFlag, retVal;
	U32		ipAddr, mask, maskLen;
	S8		ipAddrHex[9], maskHex[3];

	/*判断是添加网段还是删除网段*/
	if(retAs->asEnableNet[0] == 'A' || retAs->asEnableNet[0] == 'a'){
		noFlag = FALSE;
	}else if(retAs->asEnableNet[0] == 'D' || retAs->asEnableNet[0] == 'd'){
		noFlag = TRUE;
	}else{
		return	FAILURE;
	}

	/*取接口地址*/
	EigrpPortMemCpy((U8 *)ipAddrHex, (U8 *)&retAs->asEnableNet[1], 8);
	ipAddrHex[8] = 0;
	retVal = EigrpPortAtoHex(FALSE, ipAddrHex, &ipAddr);
	if(retVal != SUCCESS){
		return	FAILURE;
	}

	EigrpPortMemCpy((U8 *)maskHex, (U8 *)&retAs->asEnableNet[9], 2);
	maskHex[2] = 0;
	retVal = EigrpPortAtoHex(FALSE, maskHex, &maskLen);
	if(retVal != SUCCESS){
		return	FAILURE;
	}

	mask = EigrpUtilLen2Mask(maskLen);

	retVal = EigrpCmdApiNetwork(noFlag, asNo, ipAddr, mask);
	if(retVal != SUCCESS){
		return	FAILURE;
	}

	return	SUCCESS;
}

S32 eigrpSnmpAsGetRowStatus(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(!pdb){
		return	FAILURE;
	}

	retAs->asRowStatus = 1;/*active*/

	return	SUCCESS;
}

S32 eigrpSnmpAsGetNextRowStatus(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(!pdb || !pdb->next){
		return	FAILURE;
	}

	retAs->asRowStatus = 1;/*active*/

	return	SUCCESS;
}

S32 eigrpSnmpAsSetRowStatus(S32 asNo, eigrpAs_pt retAs)
{
	EigrpPdb_st		*pdb;
	S32		retVal;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		if(pdb->ddb->asystem == asNo){
			break;
		}
	}

	if(retAs->asRowStatus == EIGRP_SNMP_ROWSTATUS_CREATEANDGO){
		if(pdb){
			return	SUCCESS;
		}
		retVal = EigrpCmdApiRouterEigrp(FALSE, asNo);
		if(retVal != SUCCESS){
			return	FAILURE;
		}
	}else if(retAs->asRowStatus == EIGRP_SNMP_ROWSTATUS_DESTROY){
		if(!pdb){
			return	FAILURE;
		}
		retVal = EigrpCmdApiRouterEigrp(TRUE, asNo);
		if(retVal != SUCCESS){
			return	FAILURE;
		}
	}

	return	SUCCESS;
}

S32 eigrpSnmpAsGetNext(eigrpAs_pt pAs)
{
	EigrpPdb_st		*pdb;
	EigrpDualNdb_st	*dndb;
	S32		i, len;

	if(pAs->asNo == 0){/*查找第一个自治域*/
		pdb = (EigrpPdb_st *)gpEigrp->protoQue->head;
	}else{/*查找指定自治域的下一个自治域*/
		for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
			if(pdb->ddb->asystem == pAs->asNo){
				pdb = pdb->next;
				break;
			}
		}
	}

	if(!pdb){
		return	FAILURE;
	}

	EigrpPortMemSet((U8 *)pAs, 0, sizeof(struct eigrpAs_));

	pAs->asNo = pdb->ddb->asystem;

	len = 0;
	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = pdb->ddb->topo[i]; dndb; dndb = dndb->next){
			if(len > sizeof(pAs->asSubnet) - 18){
				break;
			}
			len += sprintf(pAs->asSubnet, "%s/%d;", EigrpUtilIp2Str(dndb->dest.address), EigrpIpBitsInMask(dndb->dest.mask));
		}
		if(len > sizeof(pAs->asSubnet) - 18){
			break;
		}
	}
	pAs->asSubnet_len = len;
	pAs->asRowStatus = 1;/*active*/

	return	SUCCESS;
}

EigrpDualPeer_st *eigrpSnmpGetPeer(S32 asId, U32 nbrAddr)
{
	EigrpPdb_st		*pdb;
	EigrpDualPeer_st	*peer;

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		for(peer = pdb->ddb->peerLst; peer; peer = peer->nextPeer){
			if(pdb->ddb->asystem == asId && peer->source == nbrAddr){
				return	peer;
			}
		}
	}

	return	NULL;
}

EigrpDualPeer_st *eigrpSnmpGetNextPeer(S32 asId, U32 nbrAddr)
{
	EigrpPdb_st		*pdb;
	EigrpDualPeer_st	*peer;

	peer = NULL;
	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		for(peer = pdb->ddb->peerLst; peer; peer = peer->nextPeer){
			if(pdb->ddb->asystem == asId && peer->source == nbrAddr){
				if(peer->nextPeer){
					peer = peer->nextPeer;
				}else{
					if(pdb->next){
						pdb = pdb->next;
						peer = pdb->ddb->peerLst;
						if(!peer){
							return	NULL;
						}
					}else{
						return	NULL;
					}
				}
				break;
			}
		}
		if(peer){
			break;
		}
	}

	return	peer;
}

S32 eigrpSnmpNbrGetAsNo(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->asNo = asId;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextAsNo(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpPdb_st		*pdb;
	EigrpDualPeer_st	*peer;

	peer = NULL;
	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		for(peer = pdb->ddb->peerLst; peer; peer = peer->nextPeer){
			if(pdb->ddb->asystem == asId && peer->source == nbrAddr){
				if(peer->nextPeer){
					peer = peer->nextPeer;
				}else{
					if(pdb->next){
						pdb = pdb->next;
						peer = pdb->ddb->peerLst;
						if(!peer){
							return	FAILURE;
						}
					}else{
						return	FAILURE;
					}
				}
				break;
			}
		}
		if(peer){
			break;
		}
	}

	if(!peer){
		return	FAILURE;
	}

	retNbr->asNo = pdb->ddb->asystem;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetIpAddr(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->ipAddr = peer->source;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextIpAddr(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->ipAddr = peer->source;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetIfIndex(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->ifIndex = peer->iidb->idb->ifindex;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextIfIndex(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->ifIndex = peer->iidb->idb->ifindex;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetHoldTime(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->holdTime = EigrpUtilMgdTimerLeftSleeping(&peer->holdingTimer) / EIGRP_MSEC_PER_SEC;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextHoldTime(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->holdTime = EigrpUtilMgdTimerLeftSleeping(&peer->holdingTimer) / EIGRP_MSEC_PER_SEC;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetUpTime(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;
	U32		t, t1, t2, t3;
	S8		timeStr[9] = {0};

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	t = EigrpPortGetTimeSec() - peer->uptime;
	if(t > EIGRP_SEC_PER_DAY){
		t1 = t / EIGRP_SEC_PER_DAY;
		t2 = (t - t1 * EIGRP_SEC_PER_DAY) / EIGRP_SEC_PER_HOUR;
		t3 = (t - t1 * EIGRP_SEC_PER_DAY - t2 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
	}else{
		t1 = t / EIGRP_SEC_PER_HOUR;
		t2 = (t - t1 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
		t3 = t % EIGRP_SEC_PER_MIN;
	}
	sprintf(timeStr, "%02d:%02d:%02d", t1, t2, t3);
	EigrpPortMemCpy((U8 *)retNbr->upTime, (U8 *)timeStr, sizeof(retNbr->upTime));
	retNbr->upTime_len = 8;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextUpTime(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;
	U32		t, t1, t2, t3;
	S8		timeStr[9] = {0};

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	t = EigrpPortGetTimeSec() - peer->uptime;
	if(t > EIGRP_SEC_PER_DAY){
		t1 = t / EIGRP_SEC_PER_DAY;
		t2 = (t - t1 * EIGRP_SEC_PER_DAY) / EIGRP_SEC_PER_HOUR;
		t3 = (t - t1 * EIGRP_SEC_PER_DAY - t2 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
	}else{
		t1 = t / EIGRP_SEC_PER_HOUR;
		t2 = (t - t1 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
		t3 = t % EIGRP_SEC_PER_MIN;
	}
	sprintf(timeStr, "%02d:%02d:%02d", t1, t2, t3);
	EigrpPortMemCpy((U8 *)retNbr->upTime, (U8 *)timeStr, sizeof(retNbr->upTime));
	retNbr->upTime_len = 8;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetSrtt(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->srtt = peer->srtt;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextSrtt(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->srtt = peer->srtt;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetRto(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->rto = peer->rto;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextRto(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->rto = peer->rto;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetPktsEnqueued(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->pktsEnqueued = peer->xmitQue[EIGRP_UNRELIABLE_QUEUE]->count
						+ peer->xmitQue[EIGRP_RELIABLE_QUEUE]->count;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextPktsEnqueued(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->pktsEnqueued = peer->xmitQue[EIGRP_UNRELIABLE_QUEUE]->count
						+ peer->xmitQue[EIGRP_RELIABLE_QUEUE]->count;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetLastSeq(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->lastSeq = (peer->lastSeqNo >= 10000 ? peer->lastSeqNo %= 10000 : peer->lastSeqNo);

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextLastSeq(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->lastSeq = (peer->lastSeqNo >= 10000 ? peer->lastSeqNo %= 10000 : peer->lastSeqNo);

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetRetries(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->retries = peer->retryCnt;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextRetries(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->retries = peer->retryCnt;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetRouterId(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->routerId = peer->routerId;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNextRouterId(S32 asId, U32 nbrAddr, eigrpNbr_pt retNbr)
{
	EigrpDualPeer_st	*peer;

	peer = eigrpSnmpGetNextPeer(asId, nbrAddr);
	if(!peer){
		return	FAILURE;
	}

	retNbr->routerId = peer->routerId;

	return	SUCCESS;
}

S32 eigrpSnmpNbrGetNext(eigrpNbr_pt pNbr)
{
	EigrpPdb_st		*pdb;
	EigrpDualPeer_st	*peer;
	U32		t, t1, t2, t3;
	S8		timeStr[9] = {0};

	peer = NULL;
	if(pNbr->asNo == 0 && pNbr->ipAddr == 0){/*查找第一个邻居*/
		pdb = (EigrpPdb_st *)gpEigrp->protoQue->head;
		if(pdb){
			peer = pdb->ddb->peerLst;
		}
	}else{/*查找指定邻居的下一个邻居*/
		for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
			for(peer = pdb->ddb->peerLst; peer; peer = peer->nextPeer){
				if(pdb->ddb->asystem == pNbr->asNo && peer->source == pNbr->ipAddr){
					if(peer->nextPeer){
						peer = peer->nextPeer;
					}else{
						if(pdb->next){
							pdb = pdb->next;
							peer = pdb->ddb->peerLst;
							if(!peer){
								return	FAILURE;
							}
						}else{
							return	FAILURE;
						}
					}
					break;
				}
			}
			if(peer){
				break;
			}
		}
	}

	if(!peer){
		return	FAILURE;
	}

	EigrpPortMemSet((U8 *)pNbr, 0, sizeof(struct eigrpNbr_));

	pNbr->asNo = pdb->ddb->asystem;
	pNbr->ipAddr = peer->source;
	pNbr->ifIndex = peer->iidb->idb->ifindex;
	pNbr->holdTime = EigrpUtilMgdTimerLeftSleeping(&peer->holdingTimer) / EIGRP_MSEC_PER_SEC;
	t = EigrpPortGetTimeSec() - peer->uptime;
	if(t > EIGRP_SEC_PER_DAY){
		t1 = t / EIGRP_SEC_PER_DAY;
		t2 = (t - t1 * EIGRP_SEC_PER_DAY) / EIGRP_SEC_PER_HOUR;
		t3 = (t - t1 * EIGRP_SEC_PER_DAY - t2 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
	}else{
		t1 = t / EIGRP_SEC_PER_HOUR;
		t2 = (t - t1 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
		t3 = t % EIGRP_SEC_PER_MIN;
	}
	sprintf(timeStr, "%02d:%02d:%02d", t1, t2, t3);
	EigrpPortMemCpy((U8 *)pNbr->upTime, (U8 *)timeStr, sizeof(pNbr->upTime));
	pNbr->upTime_len = 8;
	pNbr->srtt = peer->srtt;
	pNbr->rto = peer->rto;
	pNbr->pktsEnqueued = peer->xmitQue[EIGRP_UNRELIABLE_QUEUE]->count
						+ peer->xmitQue[EIGRP_RELIABLE_QUEUE]->count;
	pNbr->lastSeq = (peer->lastSeqNo >= 10000 ? peer->lastSeqNo %= 10000 : peer->lastSeqNo);
	pNbr->retries = peer->retryCnt;
	pNbr->routerId = peer->routerId;

	return	SUCCESS;
}

S32 eigrpSnmpReqProc(S8 *pkt, U32 pktLen)
{
	eigrpSnmpReq_pt	pReq;
	eigrpSnmpObj_st	retObj, *pObj, *pObjLst;
	S32		cnt, retVal;
	U8		reqAck;

	if(!pkt || !pktLen){
		return	FAILURE;
	}

	if(pktLen > EIGRP_SNMP_MAX_PKT_SIZE){
		return	FAILURE;
	}

	pReq = (eigrpSnmpReq_pt)pkt;

	cnt = 0;
	pObjLst = NULL;
	if(pReq->accessType == EIGRP_SNMP_WALK){/*遍历请求*/
		if(pReq->obj == EIGRP_SNMP_OBJ_AS_TBL){
			eigrpAs_st as;
			EigrpPortMemSet((U8 *)&as, 0, sizeof(as));
			while(TRUE){
				retVal = eigrpSnmpAsGetNext(&as);
				if(retVal != SUCCESS){
					break;
				}
				cnt++;
				if(cnt  > EIGRP_SNMP_MAX_PKT_SIZE / sizeof(eigrpAs_st)){
					_EIGRP_DEBUG("Warning: Too many objects. Some have been discarded\n");
					break;
				}
				pObj = (eigrpSnmpObj_pt)EigrpPortMemMalloc(sizeof(struct eigrpSnmpObj_));
				if(!pObj){
					EigrpPortAssert(0, "");
					return	FAILURE;
				}
				pObj->as = as;
				EigrpUtilDllInsert((EigrpDll_pt)pObj, NULL, (EigrpDll_pt *)&pObjLst);
			}
			_EIGRP_DEBUG("dbg>>eigrpSnmpReqProc\t: WALK AsTbl Cnt=%d\n", cnt);/*debug_zhenxl*/
			eigrpSnmpRespSend(TRUE, pReq->reqId, EIGRP_SNMP_OBJ_AS_TBL, EIGRP_SNMP_WALK, pObjLst);
		}else if(pReq->obj == EIGRP_SNMP_OBJ_NBR_TBL){
			eigrpNbr_st nbr;
			EigrpPortMemSet((U8 *)&nbr, 0, sizeof(nbr));
			while(TRUE){
				retVal = eigrpSnmpNbrGetNext(&nbr);
				if(retVal != SUCCESS){
					break;
				}
				cnt++;
				if(cnt  > EIGRP_SNMP_MAX_PKT_SIZE / sizeof(eigrpNbr_st)){
					_EIGRP_DEBUG("Warning: Too many objects. Some have been discarded\n");
					break;
				}
				pObj = (eigrpSnmpObj_pt)EigrpPortMemMalloc(sizeof(struct eigrpSnmpObj_));
				if(!pObj){
					EigrpPortAssert(0, "");
					return	FAILURE;
				}
				pObj->nbr = nbr;
				EigrpUtilDllInsert((EigrpDll_pt)pObj, NULL, (EigrpDll_pt *)&pObjLst);
			}
			_EIGRP_DEBUG("dbg>>eigrpSnmpReqProc\t: WALK NbrTbl Cnt=%d\n", cnt);/*debug_zhenxl*/
			eigrpSnmpRespSend(TRUE, pReq->reqId, EIGRP_SNMP_OBJ_NBR_TBL, EIGRP_SNMP_WALK, pObjLst);
		}

		while(pObjLst){
			pObj = pObjLst;
			pObjLst = pObjLst->nxt;
			EigrpPortMemFree((void *)pObj);
		}

		return	SUCCESS;
	}

	EigrpPortMemSet((U8 *)&retObj, 0, sizeof(retObj));
	reqAck = TRUE;
	_EIGRP_DEBUG("dbg>>eigrpSnmpReqProc\t: obj=%d, acc=%d\n", pReq->obj, pReq->accessType);/*debug_zhenxl*/

	retVal = FAILURE;
	switch(pReq->obj){
		case EIGRP_SNMP_OBJ_AS_AS_NO:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpAsGetAsNo(pReq->asNo, &retObj.as);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpAsGetNextAsNo(pReq->asNo, &retObj.as);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_AS_SUB_NET:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpAsGetSubNet(pReq->asNo, &retObj.as);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpAsGetNextSubNet(pReq->asNo, &retObj.as);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_AS_ENABLE_NET:
			if(pReq->accessType == EIGRP_SNMP_SET){
				if(pReq->valCnt){
					retVal = eigrpSnmpAsSetEnableNet(pReq->asNo, (eigrpAs_pt)pReq->val);
				}
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_AS_ROW_STATUS:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpAsGetRowStatus(pReq->asNo, &retObj.as);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpAsGetNextRowStatus(pReq->asNo, &retObj.as);
			}else if(pReq->accessType == EIGRP_SNMP_SET){
				if(pReq->valCnt){
					retVal = eigrpSnmpAsSetRowStatus(pReq->asNo, (eigrpAs_pt)pReq->val);
				}
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_AS_NO:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetAsNo(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextAsNo(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_IP:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetIpAddr(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextIpAddr(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_IF_INDEX:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetIfIndex(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextIfIndex(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_HOLD_TIME:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetHoldTime(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextHoldTime(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_UP_TIME:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetUpTime(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextUpTime(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_SRTT:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetSrtt(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextSrtt(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_RTO:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetRto(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextRto(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_LAST_SEQ:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetLastSeq(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextLastSeq(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_RETRYCNT:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetRetries(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextRetries(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		case EIGRP_SNMP_OBJ_NBR_ROUTER_ID:
			if(pReq->accessType == EIGRP_SNMP_GET){
				retVal = eigrpSnmpNbrGetRouterId(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}else if(pReq->accessType == EIGRP_SNMP_GET_NEXT){
				retVal = eigrpSnmpNbrGetNextRouterId(pReq->asNo, pReq->ipAddr, &retObj.nbr);
			}
			if(retVal != SUCCESS){
				reqAck = FALSE;
			}
			break;

		default:
			reqAck = FALSE;
	}

	retVal = eigrpSnmpRespSend(reqAck, pReq->reqId, pReq->obj, pReq->accessType, &retObj);
	if(retVal != SUCCESS){
		return	FAILURE;
	}

	return	SUCCESS;
}

S32 eigrpSnmpRespSend(U8 reqAck, U32 reqId, U8 obj, U8 accsType, eigrpSnmpObj_pt pObjLst)
{
	S8		buf[EIGRP_SNMP_MAX_PKT_SIZE] = {0}, *cp;
	eigrpSnmpRsp_pt		pResp;
	eigrpSnmpObj_pt		pObj;
	S32		cnt, retVal, sendLen;

	pResp = (eigrpSnmpRsp_pt)buf;

	pResp->reqId = reqId;
	pResp->obj = obj;
	pResp->accessType = accsType;
	pResp->reqAck = reqAck;

	cnt = 0;
	sendLen = sizeof(struct eigrpSnmpRsp_);
	if(accsType == EIGRP_SNMP_GET || accsType == EIGRP_SNMP_GET_NEXT || accsType == EIGRP_SNMP_WALK){
		pObj = pObjLst;
		cp = (S8 *)(pResp->val);
		while(pObj){
			if(obj >= EIGRP_SNMP_OBJ_AS_AS_NO && obj <= EIGRP_SNMP_OBJ_AS_TBL){
				EigrpPortMemCpy((U8 *)cp, (U8 *)&pObj->as, sizeof(struct eigrpAs_));
				cp += sizeof(struct eigrpAs_);
				sendLen += sizeof(struct eigrpAs_);
				cnt++;
			}else if(obj >= EIGRP_SNMP_OBJ_NBR_AS_NO && obj <= EIGRP_SNMP_OBJ_NBR_TBL){
				EigrpPortMemCpy((U8 *)cp, (U8 *)&pObj->nbr, sizeof(struct eigrpNbr_));
				cp += sizeof(struct eigrpNbr_);
				sendLen += sizeof(struct eigrpNbr_);
				cnt++;
			}

			pObj = pObj->nxt;
		}
	}

	pResp->valCnt = cnt;/*对象数量*/

	retVal = eigrpPortSendto(gpEigrp->snmpSock, buf, sendLen, EIGRP_SNMP_AGNET_IP, EIGRP_AGENT_PORT);
	_EIGRP_DEBUG("dbg>>eigrpSnmpRespSend\t: %d OBJs, retVal=%d\n", cnt, retVal);/*debug_zhenxl*/
	if(retVal <= 0){
		return	FAILURE;
	}

	return	SUCCESS;
}
#ifdef __cplusplus
}
#endif
#endif//INCLUDE_EIGRP_SNMP