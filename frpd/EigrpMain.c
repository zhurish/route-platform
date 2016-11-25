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

	Module:	EigrpMain

	Name:	EigrpMain.c

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

Eigrp_pt	gpEigrp = NULL;
/*
#undef	EIGRP_TRC(_type_, ...)//			fprintf(stdout, __VA_ARGS__);	
#define	EIGRP_TRC(_type_, ...)			fprintf(stdout, __VA_ARGS__)
*/											


extern	S8 *EigrpTimerType[];

/************************************************************************************

	Name:	EigrpMulticastTransmitBlocked

	Desc:	This function will return TRUE if the interface is blocked for transmitting multicast
 			packets.  This is the case if a multicast has been transmitted but it has not yet been
 			acknowledged by all of the receivers, and the multicast flow timer has not expired.
 
 			Note that this routine will return FALSE if there are multicasts queued to be
 			transmitted but no transmission is outstanding. Don't confuse this routine with 
 			EigrpIntfIsFlowReady(below).	
			
	Para: 	iidb		- pointer to the eigrp interface
	
	Ret:		
************************************************************************************/

S32	EigrpMulticastTransmitBlocked(EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpMulticastTransmitBlocked);
	/* It's flow-blocked if there's a multicast packet lingering. */
	EIGRP_FUNC_LEAVE(EigrpMulticastTransmitBlocked);
	return(iidb->lastMultiPkt != NULL);
}

/************************************************************************************

	Name:	EigrpStartMcastFlowTimer

	Desc:	This function is to start the flow control timer.  We set it to a multiple of the 
			average SRTT, with bounds.  For non-multicast interfaces we include the	number of peers
			multiplied by the pacing interval, since these packets	are sent serially at the pacing
			interval.
			
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the eigrp interface
	
	Ret:		
************************************************************************************/

void	EigrpStartMcastFlowTimer(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	U32 timer_value;
	U32 mean_srtt;
	
	EIGRP_FUNC_ENTER(EigrpStartMcastFlowTimer);

	timer_value	= 0;
	if(EigrpDualPeerCount(iidb) > 0){

		mean_srtt	= iidb->totalSrtt / EigrpDualPeerCount(iidb);
		/* Set basic value based on average SRTT. */
		timer_value	= mean_srtt *EIGRP_FLOW_SRTT_MULTIPLIER;

		/* Throw in a delay for NBMA interfaces. */
		if(iidb->useMcast != TRUE){
			timer_value	+= iidb->sndInterval[ EIGRP_RELIABLE_QUEUE ] * EigrpDualPeerCount(iidb);
		}
	}

	/* Place an absolute lower bound in case pacing is turned off. */
	timer_value	= MAX(EIGRP_MIN_MCAST_FLOW_DELAY, timer_value);
	iidb->lastMcastFlowDelay	= timer_value; /* For display purposes */
	EigrpUtilMgdTimerStart(&iidb->flowCtrlTimer, (S32) timer_value);
	EIGRP_FUNC_LEAVE(EigrpStartMcastFlowTimer);

	return;
}

/************************************************************************************

	Name:	EigrpAllocPktbuff

	Desc:	This function is to get a buffer to build packet into.
		
	Para: 	nbytes	- length of the data field
	
	Ret:		pointer to the eigrp packet header
************************************************************************************/

EigrpPktHdr_st *EigrpAllocPktbuff(U32 nbytes)
{
	EigrpPktHdr_st *eigrph;

	EIGRP_FUNC_ENTER(EigrpAllocPktbuff);
	eigrph = EigrpPortMemMalloc((U32)(EIGRP_HEADER_BYTES + nbytes));
	if(!eigrph){
		EIGRP_FUNC_LEAVE(EigrpAllocPktbuff);
		return NULL;
	}
	EigrpUtilMemZero((void *) eigrph, EIGRP_HEADER_BYTES + nbytes);
	EIGRP_FUNC_LEAVE(EigrpAllocPktbuff);
	
	return(eigrph);
}

/************************************************************************************

	Name:	EigrpNetworkMasksCleanup

	Desc:	This function is to clean up all the eigrp network masks allocated before.
		
	Para: 	NONE	
	
	Ret:		NONE
************************************************************************************/

void	EigrpNetworkMasksCleanup()
{
	S32 id;

	EIGRP_FUNC_ENTER(EigrpNetworkMasksCleanup);
	for(id = 0; id <= 32; id++){
		EigrpPortMemFree(gpEigrp->inetMask[id]);
		gpEigrp->inetMask[id]	=NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpNetworkMasksCleanup);
	
	return;
}

/************************************************************************************

	Name:	EigrpNetworkMasksInit

	Desc:	This function is to init all eigrp network masks.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpNetworkMasksInit()
{
	S32 id;
	U32 addr;

	EIGRP_FUNC_ENTER(EigrpNetworkMasksInit);
	gpEigrp->inetMask[33]	= NULL;

	addr = 0;
	for(id = 0; id <= 32; id++){
		gpEigrp->inetMask[id]	= EigrpPortSockBuildIn(0, addr);
		addr = addr >> 1 | 0x80000000;
	}
	EIGRP_FUNC_LEAVE(EigrpNetworkMasksInit);

	return;
}

/************************************************************************************

	Name:	EigrpAdjustHandleArray

	Desc:	This function is to ensure that the handle array is big enough for the new peer;  if 
			it isn't, it will be reallocated.
			
	Para: 	handles	- pointer to the handle struct which contains the handle array
			handle_number	-the handle index for the new peer
	
	Ret:		
************************************************************************************/

S32	EigrpAdjustHandleArray(EigrpHandle_st *handles,   S32 handle_number)
{
	U32	*new_handle_array;
	U32	new_handle_array_size;

	EIGRP_FUNC_ENTER(EigrpAdjustHandleArray);
	if(handle_number == EIGRP_NO_PEER_HANDLE){
		EIGRP_FUNC_LEAVE(EigrpAdjustHandleArray);
		return FALSE;
	}

	/* If the handle is within range, we're done. */
	if(handles->arraySize > EIGRP_HANDLE_TO_CELL(handle_number)){
		EIGRP_FUNC_LEAVE(EigrpAdjustHandleArray);
		return TRUE;
	}

	/* We've gotten too big.  Allocate a new array. */
	new_handle_array_size	= EIGRP_HANDLE_TO_CELL(handle_number) + 1;
	new_handle_array		= EigrpPortMemMalloc(EIGRP_HANDLE_MALLOC_SIZE(new_handle_array_size));
	if(!new_handle_array){
		EIGRP_FUNC_LEAVE(EigrpAdjustHandleArray);
		return FALSE;
	}
	EigrpUtilMemZero((void *) new_handle_array, EIGRP_HANDLE_MALLOC_SIZE(new_handle_array_size));

	/* Gotten the bigger and better one.  Copy over the old one, if present. */
	if(handles->array){
		EigrpPortMemCpy((void *) new_handle_array,
						(void *) handles->array,			
						(U32)EIGRP_HANDLE_MALLOC_SIZE(handles->arraySize));
		EigrpPortMemFree(handles->array);
	}
	handles->array = new_handle_array;
	handles->arraySize = new_handle_array_size;
	EIGRP_FUNC_LEAVE(EigrpAdjustHandleArray);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpKickPacingTimer

	Desc:	This function is to starts a pacing timer if it is stopped, but leaves it alone if it
			is already running.
			
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the eigrp interface
			delay	- lingering time to start the timer
			
	Ret:		NONE
************************************************************************************/

void	EigrpKickPacingTimer(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, U32 delay)
{
	EigrpMgdTimer_st *timer;

	EIGRP_FUNC_ENTER(EigrpKickPacingTimer);
	timer = &iidb->sndTimer;
	if(!EigrpUtilMgdTimerRunning(timer)){
		EigrpUtilMgdTimerStart(timer, (S32) delay);
	}
	EIGRP_FUNC_LEAVE(EigrpKickPacingTimer);

	return;
}

/************************************************************************************

	Name:	EigrpFreeQelm

	Desc:	This function is to free a queue element after unbinding it from the packet descriptor.
 			Manipulates the reference count in the packet descriptor.  Frees the	packet 
 			descriptor if the last reference is being unbound.
 			
 			This is also the locus of control for passing Transmit Ready indications back to DUAL.
 			We will alert DUAL if the packet is reliable.
		
	Para: 	ddb		- pointer to the dual descriptor block
			qelm	- pointer to the queue element to be freed
			iidb		- pointer to the correlative eigrp interface of the queue
			peer		- pointer to the correlative neighbor
	
	Ret:		NONE
************************************************************************************/

void	EigrpFreeQelm(EigrpDualDdb_st *ddb, EigrpPktDescQelm_st *qelm, EigrpIdb_st *iidb, EigrpDualPeer_st *peer)
{
	EigrpPackDesc_st *pktDesc;

	EIGRP_FUNC_ENTER(EigrpFreeQelm);
	if(qelm == NULL){
		EIGRP_FUNC_LEAVE(EigrpFreeQelm);
		return;
	}

	pktDesc = qelm->pktDesc;
	if(pktDesc){
		/* Drop the reference count. */
		pktDesc->refCnt--;
		if(pktDesc->refCnt < 0){
			EIGRP_ASSERT(0);
		}

		/* Notify DUAL if appropriate. */
		if(pktDesc->flagSeq){
			EigrpDualXmitContinue(ddb, iidb, pktDesc, peer);
		}

		/* If all references are gone, free the packet. */
		if(pktDesc->refCnt <= 0){
			EigrpUtilChunkFree(gpEigrp->pktDescChunks, pktDesc);
		}
	}
	EigrpUtilChunkFree(gpEigrp->pktDescQelmChunks, qelm);
	EIGRP_FUNC_LEAVE(EigrpFreeQelm);

	return;
}

/************************************************************************************

	Name:	EigrpCleanupMultipak

	Desc:	This function is to clean out the multicast packet pointer.  We need to coordinate
			things carefully, so we do it in one place.  How unusual!
 
			Make sure that we zap lastMultiPkt before freeing the queue element, since DUAL may
			call back on that thread to see if we're flow control ready.
			
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the correlative eigrp interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpCleanupMultipak(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpPktDescQelm_st *qelm;

	EIGRP_FUNC_ENTER(EigrpCleanupMultipak);
	qelm = iidb->lastMultiPkt;
	if(qelm == NULL){
		EIGRP_FUNC_LEAVE(EigrpCleanupMultipak);
		return;
	}

	iidb->lastMultiPkt		= NULL;
	iidb->mcastStartRefcount	= 0;
	EigrpFreeQelm(ddb, qelm, iidb, NULL);
	EIGRP_FUNC_LEAVE(EigrpCleanupMultipak);

	return;
}

/************************************************************************************

	Name:	EigrpUpdateMcastFlowState

	Desc:	This function is to update the multicast flow control state.  Decides whether or not
			the interface should remain flow-blocked, and updates the state as necessary.
 
 			We make the interface flow-control ready if either all copies of the packet have been 
 			acknowledged, or if the timer has expired and at least one copy of the packet has been
 			acknowledged.
 
 			The multicast flow control timer is used to wake up the interface from other threads(by
 			forcing it to expire).  This particular condition is noticable by virtue of the timer 
 			having expired but there being no pending multicast packet.	
			
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the correlative eigrp interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpUpdateMcastFlowState(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpPktDescQelm_st	*qelm;
	EigrpPackDesc_st			*pktDesc;
	S32					just_became_ready;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpUpdateMcastFlowState);
	just_became_ready = FALSE;
	qelm = iidb->lastMultiPkt;
	if(qelm){
		pktDesc = qelm->pktDesc;
	}else{
		pktDesc = NULL;
	}

	/* If it's not blocked and the timer isn't running, we're already done. */
	if(!EigrpMulticastTransmitBlocked(iidb) &&
		!EigrpUtilMgdTimerRunning(&iidb->flowCtrlTimer)){
		EIGRP_FUNC_LEAVE(EigrpUpdateMcastFlowState);
		return;
	}

	/* If all copies of the packet have been acknowledged, we are now flow control ready. We 
	  * determine this by looking at the pktDesc refCnt; if it is == 1, ours is the only copy. */
	if(pktDesc && pktDesc->refCnt == 1){
		just_became_ready = TRUE;
	}else{

		/* If the timer has just expired, check to see if anybody has acked the last multicast
		  * packet or not.  If not, there's no reason to go on;  set the timer for another round.
		  * Otherwise, stop the timer and continue. */
		if(EigrpUtilMgdTimerExpired(&iidb->flowCtrlTimer)){
			if(pktDesc && pktDesc->refCnt == iidb->mcastStartRefcount){
				EigrpStartMcastFlowTimer(ddb, iidb);
				EIGRP_FUNC_LEAVE(EigrpUpdateMcastFlowState);
				return;
			}
			EigrpUtilMgdTimerStop(&iidb->flowCtrlTimer);
		}

		/* If the timer has expired and there has been at least one acknowledgement, we are now
		  * flow ready.  If there is no packet pending, we are now flow ready(having been
		  * kicked). */
		if(!EigrpUtilMgdTimerRunning(&iidb->flowCtrlTimer)){
			if(pktDesc){
				if((pktDesc->refCnt < iidb->mcastStartRefcount) || iidb->mcastStartRefcount <= 1){
					/* Just in case */
					just_became_ready = TRUE;
				}
			}else{			/* No packet pending */
				just_became_ready = TRUE;
			}
		}
	}

	/* Now ready.  Free the queue element(making us flow ready) and kick the pacing timer.  Kill
	  * the flow control timer if it's still running. Tell the upper levels. */
	if( just_became_ready){
		EigrpDualLogXportAll(ddb, EIGRP_DUALEV_MCASTRDY, &iidb->idb, NULL);

		EigrpCleanupMultipak(ddb, iidb); /* May free pktDesc! */
		EigrpKickPacingTimer(ddb, iidb, 0);
		EigrpUtilMgdTimerStop(&iidb->flowCtrlTimer);
	}
	EIGRP_FUNC_LEAVE(EigrpUpdateMcastFlowState);

	return;
}

/************************************************************************************

	Name:	EigrpDequeueQelm

	Desc:	This function is to dequeue and free the first queue element from a peer queue. 
			Unbinds(and potentially frees) the corresponding packet descriptor.  May make the
			associated interface multicast flow ready again.
			
	Para: 	ddb		- pointer to the dual descriptor block
			peer		- pointer to the correlative peer
			qtype	- the type of queue
	
	Ret:		NONE
************************************************************************************/

void	EigrpDequeueQelm(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, U32 qtype)
{
	EigrpPktDescQelm_st	*qelm;
	EigrpIdb_st			*iidb;
	U32					seqNum;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDequeueQelm);
	/* Pull the entry from the queue. */
	qelm = (EigrpPktDescQelm_st *) EigrpUtilQue2Dequeue(peer->xmitQue[ qtype ]);
	if(qelm){
		/* Something is there? */
		seqNum = qelm->pktDesc->ackSeqNum; /* Grab it before we free! */

		/* Free the queue element and perhaps the packet as well. */
		EigrpFreeQelm(ddb, qelm, peer->iidb, peer);

		/* If the packet is sequenced and the sequence number matches the last multicast sent,
		  * update the flow control state. */
		iidb = peer->iidb;
		if(qtype == EIGRP_RELIABLE_QUEUE && seqNum == iidb->lastMcastSeq){
			EigrpUpdateMcastFlowState(ddb, peer->iidb);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDequeueQelm);

	return;
}

/************************************************************************************

	Name:	EigrpCalculatePacingInterval

	Desc:	This function is to calculate the pacing interval, given the bandwidth, packet size,
			and whether the packet is sequenced or not.
			
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- the eigrp interface on which the packet will be sent
			length	- the packet size
			flagseq	- the sign for judging whether the packet need reliable transmission or not
	
	Ret:		pacing interval
************************************************************************************/

U32	EigrpCalculatePacingInterval(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, U32 length, S32 flagSeq)
{
	U32 interval, bandwidth, packet_size_bits_x_100, denominator, peer_count;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpCalculatePacingInterval);
	bandwidth = iidb->idb ? iidb->idb->bandwidth : 0;
	if(bandwidth == 0){
		bandwidth = 1;
	}

	/* The calculation is effectively(data_size / (fraction *bandwidth)). We reorder things to
	  * avoid truncation and to minimize the number of multiplications and divisions necessary. */
	packet_size_bits_x_100 = (length + ddb->pktOverHead + EIGRP_HEADER_BYTES) * 8 * 100;
	denominator = bandwidth *iidb->bwPercent;
	if(denominator == 0){
		denominator = 1;
	}
	denominator *= 1024;   
	interval = packet_size_bits_x_100 / denominator;

	if(flagSeq){
		/* Sequenced packets are subject to a minimum interval, and are scaled upward according to 
		  * the number of neighbors on multicast-capable interfaces. */
		peer_count = EigrpDualPeerCount(iidb);
		if((iidb->useMcast == TRUE) && (peer_count > 1)){
			interval *= peer_count / 2;
		}
		interval = MAX(EIGRP_MIN_PACING_IVL, interval);

	}else{
		/* Unsequenced packets run at wire speed above T1. */
		if(bandwidth >= EIGRP_NO_UNRELIABLE_DELAY_BW){
			interval = 0;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpCalculatePacingInterval);
	
	return(interval);
}

/************************************************************************************

	Name:	EigrpAdjustPeerArray

	Desc:	This function is to set the peer array entry to accommodate the new peer.  If the array
			isn't 	big enough, reallocate it.
			
	Para: 	ddb		- pointer to the dual descriptor block
			peer		- pointer to the new peer
			handle	- the handle index of new peer
	
	Ret:		
************************************************************************************/

S32	EigrpAdjustPeerArray(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, S32 handle)
{
	EigrpDualPeer_st	**new_table;
	U32			new_table_size;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpAdjustPeerArray);
	/* If the array is too small, reallocate it. */
	if(handle >= (S32)ddb->peerArraySize){
		new_table_size	= MAX((S32)(ddb->peerArraySize + PEER_TABLE_INCREMENT), handle + 1); /* Be robust. */
		new_table		= EigrpPortMemMalloc(sizeof(EigrpDualPeer_st *) *new_table_size);
		if(!new_table){ 
			EIGRP_FUNC_LEAVE(EigrpAdjustPeerArray);
			return FALSE;
		}
		EigrpUtilMemZero((void *) new_table, sizeof(EigrpDualPeer_st *) *new_table_size);
		if(ddb->peerArray){
			EigrpPortMemCpy((void *) new_table, (void *) ddb->peerArray, sizeof(EigrpDualPeer_st *) *ddb->peerArraySize);
			EigrpPortMemFree(ddb->peerArray);
		}
		ddb->peerArray		= new_table;
		ddb->peerArraySize	= new_table_size;
	}
	ddb->peerArray[ handle ]	= peer;
	EIGRP_FUNC_LEAVE(EigrpAdjustPeerArray);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpFindTempIidb

	Desc:	This function is to find EIGRP idb structure for given ddb on the temporary queue. This
 			queue is only used when an EIGRP interface configuration parameter is set but the
 			interface is not yet configured to run EIGRP over. Once it is configured, the data 
 			structure is moved from the ddb->temIidbQue to ddb->iidbQue.
		
	Para: 	ddb		- pointer to the dual descriptor block
			pEigrpIntf	- pointer to the correlative eigrp interface
	
	Ret:		pointer to the EIGRP idb structure needed on the temporary queue
************************************************************************************/

EigrpIdb_st *EigrpFindTempIidb(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpFindTempIidb);
	for(iidb = (EigrpIdb_st*) ddb->temIidbQue->head; iidb; iidb = iidb->next){
		if(iidb->idb == pEigrpIntf){
			break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpFindTempIidb);
	
	return(iidb);
}

/************************************************************************************

	Name:	EigrpSetPacingIntervals

	Desc:	This function is to set the pacing intervals based on the interface bandwidth and peer
			count.
		
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the eigrp interface
	
	Ret:		
************************************************************************************/

void	EigrpSetPacingIntervals(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	/* The send intervals are calculated as a function of the bandwidth. The target is to limit
 	  * EIGRP to a fraction of the bandwidth.  This is calculated based on the size of an ACK for
 	  * the unreliable queue, and the size of an MTU-sized update for the reliable queue.
 	  *
	  * For links above 1Mbps, the interpacket gap for unreliable packets is eliminated.
	  *
	  * The interpacket gap for reliable packets has an absolute minimum; this is done because in 
	  * practice the CPU load of processing the incoming packets becomes the limiting factor.
	  *
	  * The interpacket gap for reliable packets also factors in the number of peers on the 
	  * interface, if the interface is multicast-capable. This is done in an attempt to avoid
	  * flooding any receivers, since an event on a LAN can cause everyone to transmit more or less 
	  * simultaneously.  If all transmitters back off when there are a lot of them, then the
	  * receivers won't be quite as stressed.  We multiply the pacing interval by half the number
	  * of neighbors.
	  *
	  * The interval calculated here is the base interval for the largest packets. When a packet is
	  * actually transmitted, the instantaneous interval is based on the size of the packet.  The
	  * base value is used in other calculations that include the pacing interval(such as the
	  * multicast flow control timer).s */

	/* First the reliable queue. */
	EIGRP_FUNC_ENTER(EigrpSetPacingIntervals);
	iidb->sndInterval[ EIGRP_RELIABLE_QUEUE ]	=
	EigrpCalculatePacingInterval(ddb, iidb, iidb->maxPktSize, TRUE);

	/* Now the unreliable queue. */
	iidb->sndInterval[ EIGRP_UNRELIABLE_QUEUE ]	=
		EigrpCalculatePacingInterval(ddb, iidb, 0, FALSE);

	/* Use the base value as our current value. */
	iidb->lastSndIntv[ EIGRP_RELIABLE_QUEUE ]	= iidb->sndInterval[ EIGRP_RELIABLE_QUEUE ];
	iidb->lastSndIntv[ EIGRP_UNRELIABLE_QUEUE ]	= iidb->sndInterval[ EIGRP_UNRELIABLE_QUEUE ];
	EIGRP_FUNC_LEAVE(EigrpSetPacingIntervals);

	return;
}

/************************************************************************************

	Name:	EigrpCreateIidb

	Desc:	This function is to allocate and initialize an IIDB.
 
  			Returns a pointer to the IIDB, or NULL if no memory.
 
			This routine does not link the IIDB into any data structures.  Note that
			the nullIidb is never so linked.
		
	Para: 	ddb		- pointer to dual descriptor block 
			pEigrpIntf	- pointer to the eigrp interface in IP layer 
	
	Ret:		
************************************************************************************/

EigrpIdb_st *EigrpCreateIidb(EigrpDualDdb_st *ddb,  EigrpIntf_pt pEigrpIntf)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpCreateIidb);
	/* Grab some memory. */
	iidb = EigrpPortMemMalloc(sizeof(EigrpIdb_st));
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpCreateIidb);
		return NULL;
	}
	EigrpUtilMemZero((void *) iidb, sizeof(EigrpIdb_st));

	/* Initialize the queues. */

	iidb->xmitQue[ EIGRP_UNRELIABLE_QUEUE ]	= EigrpUtilQue2Init();
	iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]		= EigrpUtilQue2Init();

	iidb->idb				= pEigrpIntf;
	iidb->helloInterval		= EIGRP_DEF_HELLOTIME;
	iidb->holdTime			= EIGRP_DEF_HOLDTIME;
	iidb->splitHorizon			= EIGRP_DEF_SPLITHORIZON;
	iidb->bwPercent	= EIGRP_DEF_BANDWIDTH_PERCENT;
	iidb->holdDown			= EIGRP_DEF_HOLDDOWN;
	iidb->authMode			= FALSE;

	if(pEigrpIntf){
		iidb->useMcast	= BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST) ? TRUE : FALSE;
		iidb->delay			= BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST) ? EIGRP_B_DELAY : EIGRP_S_DELAY;
	}
	/* Initialize managed timers. */
	EigrpUtilMgdTimerInitParent(&iidb->peerTimer, &ddb->peerTimer, NULL, NULL);
	EigrpUtilMgdTimerInitParent(&iidb->holdingTimer, &ddb->masterTimer, NULL, NULL);
	EigrpUtilMgdTimerInitLeaf(&iidb->sndTimer, &ddb->intfTimer, EIGRP_IIDB_PACING_TIMER, iidb, FALSE);
	EigrpUtilMgdTimerInitLeaf(&iidb->helloTimer, &ddb->helloTimer, EIGRP_IIDB_HELLO_TIMER, iidb, FALSE);
	EigrpUtilMgdTimerInitLeaf(&iidb->flowCtrlTimer, &ddb->masterTimer, EIGRP_FLOW_CONTROL_TIMER, iidb, FALSE);
	EigrpUtilMgdTimerInitLeaf(&iidb->pktTimer, &ddb->masterTimer, EIGRP_PACKETIZE_TIMER, iidb, FALSE);
	EigrpUtilMgdTimerInitLeaf(&iidb->aclChgTimer, &ddb->masterTimer, EIGRP_IIDB_ACL_CHANGE_TIMER, iidb, FALSE);

	EigrpSetPacingIntervals(ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpCreateIidb);

	return(iidb);
}

/************************************************************************************

	Name:	EigrpGetIidb

	Desc:	This function is to get EIGRP idb from idb queue. If it doesn't exist, it will create
			one and put it on a temporary list. 
			
	Para: 	ddb		- pointer to dual descriptor block 
			pEigrpIntf	- pointer the given EIGRP interface
	
	Ret:		the pointer to the EIGRP idb needed
************************************************************************************/

EigrpIdb_st *EigrpGetIidb(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpGetIidb);
	iidb = EigrpFindIidb(ddb, pEigrpIntf);

	/* iidb not created yet. Allocate and put on temporary queue. */
	if(!iidb){
		iidb = EigrpFindTempIidb(ddb, pEigrpIntf);
		if(!iidb){
			iidb = EigrpCreateIidb(ddb, pEigrpIntf);
			if(!iidb){
				EIGRP_FUNC_LEAVE(EigrpGetIidb);
				return NULL;
			}
			EigrpUtilQue2Enqueue(ddb->temIidbQue, (EigrpQueElem_st *)iidb);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpGetIidb);
	
	return(iidb);
}

/************************************************************************************

	Name:	EigrpAllocSendBuffer

	Desc:	This function is to allocate an output buffer.
		
	Para: 	maxsize		- the max size of the output buffer
	
	Ret:		NONE
************************************************************************************/

void	EigrpAllocSendBuffer(U32 maxsize)
{
	EIGRP_FUNC_ENTER(EigrpAllocSendBuffer);
	/* Round it up to a page size */
	maxsize = EIGRP_UTIL_ROUND_UP(maxsize, EIGRP_UTIL_PAGE_SIZE);

	if(maxsize > gpEigrp->sendBufLen){
		/* This request is larger than previous requests */
		if(gpEigrp->sendBuf){
			/* Free the old buffer */
			EigrpPortMemFree(gpEigrp->sendBuf);
		}

		/* Allocate send buffer */
		gpEigrp->sendBuf = (U8 *) EigrpPortMemMalloc(maxsize);
		if(!gpEigrp->sendBuf){	 
			EIGRP_FUNC_LEAVE(EigrpAllocSendBuffer);
			return;
		}
		EIGRP_ASSERT((U32)gpEigrp->sendBuf);

		EigrpUtilMemZero(gpEigrp->sendBuf, maxsize);
		gpEigrp->sendBufLen = maxsize;
		EIGRP_TRC(DEBUG_EIGRP_TASK,"EIGRP-SYS: Allocate send buffer %d bytes\n",
					gpEigrp->sendBufLen);
	}
	EIGRP_FUNC_LEAVE(EigrpAllocSendBuffer);

	return;
}

/************************************************************************************

	Name:	EigrpAllocRecvBuffer

	Desc:	This function is to allocate an input buffer
		
	Para: 	maxsize		- the max size of the input buffer
	
	Ret:		NONE
************************************************************************************/

void    EigrpAllocRecvBuffer(U32 maxsize)
{
	EIGRP_FUNC_ENTER(EigrpAllocRecvBuffer);
	if(maxsize > gpEigrp->recvBufLen){
		/* Round it up to a page size */
		maxsize = EIGRP_UTIL_ROUND_UP(maxsize, EIGRP_UTIL_PAGE_SIZE);

		if(gpEigrp->recvBuf){
			/* Free the old buffer */
			EigrpPortMemFree(gpEigrp->recvBuf);
		}

		/* Allocate recv buffer */
		gpEigrp->recvBuf = (U8 *) EigrpPortMemMalloc(maxsize);
		if(!gpEigrp->recvBuf){	 
			EIGRP_FUNC_LEAVE(EigrpAllocRecvBuffer);
			return;
		}
		EIGRP_ASSERT((U32)gpEigrp->recvBuf);
		gpEigrp->recvBufLen = maxsize;

		EIGRP_TRC(DEBUG_EIGRP_TASK,"EIGRP-SYS: Allocate recv buffer %d bytes\n",
					gpEigrp->recvBufLen);
	}
	EIGRP_FUNC_LEAVE(EigrpAllocRecvBuffer);

	return;
}

/************************************************************************************

	Name:	EigrpInit

	Desc:	This function is to do initialization for eigrp. It should be called when the system starts up.
		
	Para: 	NONE
		
	Ret:		NONE	
************************************************************************************/
/* Start Edit By  : AuthorName:zhurish : 2016/01/16 16 :51 :45  : Comment:?????1??????:.?��?????????��?????��?o???��???linux??��?��??????�㨨???��? */
#ifdef _EIGRP_PLAT_MODULE
void	EigrpInit(int daemon)
#else//_EIGRP_PLAT_MODULE
void	EigrpInit()
#endif//_EIGRP_PLAT_MODULE
/* End Edit By  : AuthorName:zhurish : 2016/01/16 16 :51 :45  */
{
	U32	flag;
	S32	retVal;

	gpEigrp	= (Eigrp_pt)EigrpPortMemMalloc(sizeof(Eigrp_st));
	if(!gpEigrp){
		return;
	}
	EigrpPortMemSet((U8 *)gpEigrp, 0, sizeof(Eigrp_st));

	gpEigrp->semSyncQ		= EigrpPortSemBCreate(TRUE);
	if(!gpEigrp->semSyncQ){	 
		return;
	}
	
	gpEigrp->protoQue	= EigrpUtilQue2Init();
	if(!gpEigrp->protoQue){	 
		return;
	}
	gpEigrp->ddbQue	= EigrpUtilQue2Init();
	if(!gpEigrp->ddbQue){		 
		return;
	}
	gpEigrp->cmdQue	= EigrpUtilQue2Init();
	if(!gpEigrp->cmdQue){		 
		return;
	}
	gpEigrp->conf		= EigrpPortMemMalloc(sizeof(EigrpConf_st));
	if(!gpEigrp->conf){	 
		return;
	}
#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	//gpEigrp->timerHdr	= EigrpPortMemMalloc(sizeof(EigrpLinuxTimer_st));
	//EigrpPortMemSet((U8 *)&gpEigrp->timerHdr, 0, sizeof(EigrpLinuxTimer_st));
	//gpEigrp->timerSem	= EigrpPortSemBCreate(TRUE);
#endif
#ifdef _EIGRP_PLAT_MODULE_TIMER
	//��ʼ����ʱ��������ݽṹ
	EigrpPortMemSet((U8 *)&gpEigrp->timerHdr, 0, sizeof(EigrpLinuxTimer_st)* _EIGRP_TIMER_MAX);
#endif// _EIGRP_PLAT_MODULE_TIMER

	gpEigrp->strBuf	= (U8 *)EigrpPortMemMalloc(2048);
	if(!gpEigrp->strBuf){	 
		return;
	}
	gpEigrp->bufLen	= 2048;
	gpEigrp->usedLen	= 0;

	gpEigrp->funcCnt	= 0;
	gpEigrp->funcMax	= EIGRP_FUNC_STACK_SIZE;
#if		(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD || EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	EigrpIntfInit();
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
/* Start Edit By  :zhurish : 2016/01/16 17 :3 :18  : û��·��ƽ̨�³�ʼ��ϵͳ�ӿڱ� */
#ifdef EIGRP_PLAT_ZEBRA	
	EigrpIntfInit();
#endif//EIGRP_PLAT_ZEBRA
/* End Edit By  : zhurish : 2016/01/16 17 :3 :18  */
#endif
	EigrpUtilSchedInit();	
	EigrpNetworkMasksInit();
	
	/* Make socket. */
	gpEigrp->socket = EigrpSocketInit(&gpEigrp->pktMaxSize);
	if(gpEigrp->socket < 0){
		EigrpPortAssert(0, "Very strong system error.\n");
		EIGRP_TRC(DEBUG_EIGRP_ALL,"Can't create eigrp socket\n");
		return;
	}
	flag	= DEBUG_EIGRP_INTERNAL | DEBUG_EIGRP_TASK;
	EIGRP_TRC(flag,"EIGRP-SYS: Create a eigrp sock %d\n", gpEigrp->socket);
	/* Allocate the buffers */
	EigrpAllocSendBuffer(EIGRP_PKT_SIZE);
	EigrpAllocRecvBuffer(EIGRP_PKT_SIZE);
	
	EigrpPortCallBackInit();
#ifdef INCLUDE_EIGRP_SNMP
	eigrpSnmpInit();
#endif

/* Start Edit By  : AuthorName:zhurish : 2016/01/16 17 :3 :39  : ����ȫ���ź���*/
#ifdef _EIGRP_PLAT_MODULE
	gpEigrp->EigrpSem = EigrpPortSemBCreate(TRUE);
	if(!gpEigrp->EigrpSem){	 
		return;
	}
#endif// _EIGRP_PLAT_MODULE
/* End Edit By  : AuthorName:zhurish : 2016/01/16 17 :3 :39  */

/* Start Edit By  : AuthorName:zhurish : 2016/01/16 17 :4 :16  : �Ż��������� */
#ifdef _EIGRP_PLAT_MODULE

#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)

	gpEigrp->taskContainer = (void *)EigrpPortTaskStart("tFRP", daemon, EigrpMain);

#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)

/*	EigrpPortRouteMapInit();*/
	EigrpPortRegIntfCallbk();
	EigrpPortRegRtCallbk();
	EigrpEventReadAdd();

#else//_EIGRP_PLAT_MODULE
/* End Edit By  : AuthorName:zhurish : 2016/01/16 17 :4 :16  */

#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD || EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
#if(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	gpEigrp->taskContainer = (void *)1;
	EigrpPortTaskStart("tFRP", daemon, EigrpMain);
#else//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	gpEigrp->taskContainer = (void *)EigrpPortTaskStart("tFRP", 130, EigrpMain);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
/*	EigrpPortRouteMapInit();*/
	EigrpPortRegIntfCallbk();
	EigrpPortRegRtCallbk();
	EigrpEventReadAdd();
#elif(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	EigrpEventReadAdd();/*by lihui 20090924*/

	EigrpPortAssert(!gpEigrp->taskContainer, "");
#if(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	gpEigrp->taskContainer = (void *)1;
	EigrpPortTaskStart("tFRP", daemon, EigrpMain);

#else//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	gpEigrp->taskContainer = (void *)EigrpPortTaskStart("tFRP", 130, EigrpMain);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	EigrpPortRegIntfCallbk();
	EigrpPortRegRtCallbk();
	
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#endif//_EIGRP_PLAT_MODULE

#ifdef _DC_
	retVal	= EigrpPortDcSockCreate();
	if(retVal == FAILURE){
		printf("EigrpInit: failed to create DC socket.\n");
	}
#endif//_DC_


/* Start Edit By  : AuthorName:zhurish : 2016/01/16 17 :4 :40  : �Ż��������� */
#ifdef _EIGRP_PLAT_MODULE

#if(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	gpEigrp->taskContainer = (void *)1;
	EigrpPortTaskStart("tFRP", daemon, EigrpMain);

#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)

#endif//_EIGRP_PLAT_MODULE
/* End Edit By  : AuthorName:zhurish : 2016/01/16 17 :4 :40  */


	return;
}
void	EigrpConfAsFree()
{
	EigrpConfAs_st	*pConfAs;
	EigrpConfAsNet_pt	pNet;
	EigrpConfAsNei_pt	pNei;
	EigrpConfAsRedis_pt	pRedis;
	
	
	while(gpEigrp->conf->pConfAs){
		pConfAs	= gpEigrp->conf->pConfAs;
		gpEigrp->conf->pConfAs	= pConfAs->next;
		if(pConfAs->defMetric){
			EigrpPortMemFree((void *)pConfAs->defMetric);
		}

		if(pConfAs->distance){
			EigrpPortMemFree((void *)pConfAs->distance);
		}

		if(pConfAs->metricWeight){
			EigrpPortMemFree((void *)pConfAs->metricWeight);
		}

		while(pConfAs->network){
			pNet	= pConfAs->network;
			pConfAs->network = pNet->next;
			
			if(pNet->next){
				pNet->next->prev	= pNet->prev;
			}

			if(pNet->prev){
				pNet->prev->next	= pNet->next;
			}

			EigrpPortMemFree((void *)pNet);
		}
		
		while(pConfAs->nei){
			pNei	= pConfAs->nei;
			pConfAs->nei = pNei->next;
			
			if(pNei->next){
				pNei->next->prev	= pNei->prev;
			}

			if(pNei->prev){
				pNei->prev->next	= pNei->next;
			}

			EigrpPortMemFree((void *)pNei);
		}
		
		while(pConfAs->redis){
			pRedis	= pConfAs->redis;
			pConfAs->redis = pRedis->next;
			
			if(pRedis->next){
				pRedis->next->prev	= pRedis->prev;
			}

			if(pRedis->prev){
				pRedis->prev->next	= pRedis->next;
			}

			EigrpPortMemFree((void *)pRedis);
		}
		
		EigrpPortMemFree((void *)pConfAs);
		
	}

	return;
}

void	EigrpConfIntfFree()
{
	EigrpConfIntf_st	*pConfIntf;
	EigrpConfIntfPassive_pt	pPassive;
	EigrpConfIntfAuthkey_pt	pAuthKey;
	EigrpConfIntfAuthid_pt		pAuthId;
	EigrpConfIntfAuthMode_pt	pAuth;
	EigrpConfIntfBw_pt			pBandWidth;
	EigrpConfIntfHello_pt		pHello;
	EigrpConfIntfHold_pt		pHold;
	EigrpConfIntfSplit_pt		pSplit;
	EigrpConfIntfSum_pt		pSum;
	
	while(gpEigrp->conf->pConfIntf){
		pConfIntf	= gpEigrp->conf->pConfIntf;
		gpEigrp->conf->pConfIntf	= pConfIntf->next;

		while(pConfIntf->passive){
			pPassive	= pConfIntf->passive;
			pConfIntf->passive = pPassive->next;
			
			if(pPassive->next){
				pPassive->next->prev	= pPassive->prev;
			}

			if(pPassive->prev){
				pPassive->prev->next	= pPassive->next;
			}

			EigrpPortMemFree((void *)pPassive);
		}
		
		while(pConfIntf->authkey){
			pAuthKey	= pConfIntf->authkey;
			pConfIntf->authkey = pAuthKey->next;
			
			if(pAuthKey->next){
				pAuthKey->next->prev	= pAuthKey->prev;
			}

			if(pAuthKey->prev){
				pAuthKey->prev->next	= pAuthKey->next;
			}

			EigrpPortMemFree((void *)pAuthKey);
		}
		
		while(pConfIntf->authid){
			pAuthId	= pConfIntf->authid;
			pConfIntf->authid = pAuthId->next;
			
			if(pAuthId->next){
				pAuthId->next->prev	= pAuthId->prev;
			}

			if(pAuthId->prev){
				pAuthId->prev->next	= pAuthId->next;
			}

			EigrpPortMemFree((void *)pAuthId);
		}
		
		if(pConfIntf->next){
			pConfIntf->next->prev	= pConfIntf->prev;
		}

		while(pConfIntf->authmode){
			pAuth	= pConfIntf->authmode;
			pConfIntf->authmode = pAuth->next;
			
			if(pAuth->next){
				pAuth->next->prev	= pAuth->prev;
			}

			if(pAuth->prev){
				pAuth->prev->next	= pAuth->next;
			}

			EigrpPortMemFree((void *)pAuth);
		}
		
		while(pConfIntf->bandwidth){
			pBandWidth	= pConfIntf->bandwidth;
			pConfIntf->bandwidth = pBandWidth->next;
			
			if(pBandWidth->next){
				pBandWidth->next->prev	= pBandWidth->prev;
			}

			if(pBandWidth->prev){
				pBandWidth->prev->next	= pBandWidth->next;
			}

			EigrpPortMemFree((void *)pBandWidth);
		}
				
		while(pConfIntf->hello){
			pHello	= pConfIntf->hello;
			pConfIntf->hello = pHello->next;
			
			if(pHello->next){
				pHello->next->prev	= pHello->prev;
			}

			if(pHello->prev){
				pHello->prev->next	= pHello->next;
			}

			EigrpPortMemFree((void *)pHello);
		}
		
		while(pConfIntf->hold){
			pHold	= pConfIntf->hold;
			pConfIntf->hold = pHold->next;
			
			if(pHold->next){
				pHold->next->prev	= pHold->prev;
			}

			if(pHold->prev){
				pHold->prev->next	= pHold->next;
			}

			EigrpPortMemFree((void *)pHold);
		}
		
		while(pConfIntf->split){
			pSplit	= pConfIntf->split;
			pConfIntf->split = pSplit->next;
			
			if(pSplit->next){
				pSplit->next->prev	= pSplit->prev;
			}

			if(pSplit->prev){
				pSplit->prev->next	= pSplit->next;
			}

			EigrpPortMemFree((void *)pSplit);
		}
		
		while(pConfIntf->summary){
			pSum	= pConfIntf->summary;
			pConfIntf->summary = pSum->next;
			
			if(pSum->next){
				pSum->next->prev	= pSum->prev;
			}

			if(pSum->prev){
				pSum->prev->next	= pSum->next;
			}

			EigrpPortMemFree((void *)pSum);
		}
		
		EigrpPortMemFree((void *)pConfIntf);
		
	}

	return;
}


void EigrpFree()
{	
	EigrpIntf_pt	pIntf;
	EigrpPdb_st	*pdb, *pdbTmp;
	EigrpDualDdb_st	*ddb, *ddbTmp;
	EigrpCmd_pt	pCmd, pCmdNext;	
	EigrpSched_st	*pNextSched;

 	EigrpPortCallBackClean();
	EigrpPortTaskDelete();
	
	/* delete the configuration */
	EigrpConfAsFree();
	EigrpConfIntfFree();
	EigrpPortMemFree((void *)gpEigrp->conf);
	
	EigrpPortMemFree((void *)gpEigrp->strBuf);
	EigrpPortMemFree((void *)gpEigrp->sendBuf);
	EigrpPortMemFree((void *)gpEigrp->recvBuf);

	
	if(gpEigrp->protoQue){
		for(pdb= (struct EigrpPdb_ *)gpEigrp->protoQue->head; pdb;){
			pdbTmp	= pdb->next;
			EigrpPortTimerDelete(&pdb->ddb->masterTimer.sysTimer);
			EigrpPortTimerDelete(&pdb->ddb->activeTimer.sysTimer);
			EigrpPortTimerDelete(&pdb->ddb->helloTimer.sysTimer);
			EigrpIpCleanup(pdb);
			pdb	= pdbTmp;
		}
		EigrpPortMemFree((void *)gpEigrp->protoQue->lock);
		EigrpPortMemFree((void *)gpEigrp->protoQue);
	}
	
	/* free the ddb queue */
	if(gpEigrp->ddbQue){
		for(ddb= (struct EigrpDualDdb_ *)gpEigrp->ddbQue->head; ddb;){
			ddbTmp	= ddb->next;
			EigrpFreeDdb(ddb);
			ddb	= ddbTmp;
		}
		EigrpPortMemFree((void *)gpEigrp->ddbQue->lock);
		EigrpPortMemFree((void *)gpEigrp->ddbQue);
	}

	/* delete the logical interface */
	while(gpEigrp->intfLst){
		pIntf	= gpEigrp->intfLst;
		gpEigrp->intfLst	= pIntf->next;

		EigrpPortMemFree(pIntf);
	}

	/* free the command queue */
	if(gpEigrp->cmdQue){
		for(pCmd = (struct EigrpCmd_ *)gpEigrp->cmdQue->head; pCmd; pCmd= pCmdNext){
			pCmdNext = pCmd->next;
			EigrpUtilQue2Unqueue(gpEigrp->cmdQue , (struct EigrpQueElem_ *)pCmd);
			EigrpPortMemFree((void *)pCmd);
		}
		EigrpPortMemFree((void *)gpEigrp->cmdQue->lock);
		EigrpPortMemFree((void *)gpEigrp->cmdQue);
	}
	/* free the schedlst */

	while(gpEigrp->schedLst){
		pNextSched = gpEigrp->schedLst->forw;
		if(gpEigrp->schedLst->sem){	
			EigrpPortMemFree((void *)gpEigrp->schedLst->sem);
		}		
		EigrpPortMemFree((void *)gpEigrp->schedLst);
		gpEigrp->schedLst = pNextSched;
	}

	EigrpNetworkMasksCleanup();
	
	EigrpPortSemBDelete(gpEigrp->semSyncQ);
#ifdef _EIGRP_PLAT_MODULE
	if(gpEigrp->EigrpSem) 
		EigrpPortSemBDelete(gpEigrp->EigrpSem);
#endif// _EIGRP_PLAT_MODULE
#ifdef _DC_
	if(gpEigrp->dcSock){
		EigrpPortDcSockRelease(gpEigrp->dcSock);
		gpEigrp->dcSock	= 0;
	}
#if(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
	if(gpEigrp->dcFDR){
		EigrpPortMemFree(gpEigrp->dcFDR);
		gpEigrp->dcFDR	= NULL;
	}
#endif
#endif
	EigrpPortMemFree(gpEigrp);

	return;
}


/************************************************************************************

	Name:	EigrpMain

	Desc:	This function is the main routine of eigrp.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/
void EigrpMain()
{
	EigrpSched_pt	pSched;
	EigrpCmd_pt	pCmd;
	EigrpPdb_st	*pdb;
	S32	(*func)(void *);
	U32	msgNum;
	U32	tmpCnt = 0;
#ifdef _DC_
	S32	retVal;
	U8	*pktBuf;
	U32	pktLen;
#endif

	EIGRP_FUNC_ENTER(EigrpMain);
#ifdef _DC_
	pktBuf	= EigrpPortMemMalloc(1024 * 2);
#endif
	while(gpEigrp->taskContainer){
		if(tmpCnt++ > 50){
			tmpCnt = 0;
#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
		sleep(1);
#endif	
#if(EIGRP_OS_TYPE==EIGRP_OS_VXWORKS)
		taskDelay(sysClkRateGet());
#endif			
			//EigrpPortSleep(20);
		}
		EigrpPortSleep(100);
EigrpUtilDndbHealthy(__FILE__, __LINE__);

#ifdef _EIGRP_PLAT_MODULE
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);
#endif//_EIGRP_PLAT_MODULE

		pSched = EigrpUtilSchedGet();
		if(pSched){
			func	= (S32 (*)(void *))pSched->func;
			func(pSched->data); 

			EigrpPortMemFree(pSched);
			msgNum	= 0;

			for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
				(void)EigrpUtilQueGetMsgNum(pdb->workQueId, &msgNum);
				if(msgNum){
					break;
				}
			}

			if(msgNum){
#ifdef _EIGRP_PLAT_MODULE
				if(gpEigrp->EigrpSem)
					EigrpPortSemBGive(gpEigrp->EigrpSem);
#endif//_EIGRP_PLAT_MODULE
				continue;	/* If there is any event to be processed, do it first. */
			}
		}
EigrpUtilDndbHealthy(__FILE__, __LINE__);
		{
			/* On some platform, system interface reporting is very
			   slow. When it arrives after command, the interface 
			   related command will not funciton.
			   So, we need to delay command process.
			   tigerwh 100203 
			*/
			static U32 initTime = 0;
			
			if(initTime == 0){
				initTime	= EigrpPortGetTimeSec();
			}
			if(EigrpPortGetTimeSec() - initTime > 3){
				pCmd	= (EigrpCmd_pt)EigrpUtilQue2Dequeue(gpEigrp->cmdQue);
				if(pCmd){
					EigrpCmdProc(pCmd);
#ifdef _EIGRP_PLAT_MODULE
					if(gpEigrp->EigrpSem)
						EigrpPortSemBGive(gpEigrp->EigrpSem);
						//printf("EigrpPortSemBGive: 0\n");
#endif//_EIGRP_PLAT_MODULE
					continue;
				}
			}
		}
EigrpUtilDndbHealthy(__FILE__, __LINE__);
		for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
			if(pdb->ddb->masterTimerProc == TRUE){
				pdb->ddb->masterTimerProc	= FALSE;
				EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcManagedTimers, pdb->ddb);
			}
			if(pdb->ddb->helloTimerProc == TRUE){
				pdb->ddb->helloTimerProc	= FALSE;
				EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcHelloTimers, pdb->ddb);
			}
			if(pdb->ddb->siaTimerProc == TRUE){
				pdb->ddb->siaTimerProc	= FALSE;
				EigrpUtilSchedAdd((S32 (*)(void *))EigrpDualOnTaskActiveTimer, pdb->ddb);
			}
		}
EigrpUtilDndbHealthy(__FILE__, __LINE__);

/*		EigrpPortSleep(15000);*/
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
/*09.9.10 songjinliang*/
#ifdef EIGRP_PLAT_ZEBOS	
		EigrpPortZebraGetMsg();//zebosƽ̨��ȡ�ͻ��˻�����Ϣ
#endif//EIGRP_PLAT_ZEBOS
#ifdef EIGRP_PLAT_ZEBRA	
		ZebraClientEigrpUtilSched();//zebraƽ̨��ȡzclient�ͻ�����Ϣ
#endif//EIGRP_PLAT_ZEBRA
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)

/* Start Edit By  : AuthorName:zhurish : 2016/01/16 17 :6 :26  : �����¼������� */
#ifdef _EIGRP_PLAT_MODULE
		ZebraEigrpCmdSched();
		ZebraEigrpUtilSched();
#endif	//_EIGRP_PLAT_MODULE
/* End Edit By  : AuthorName:zhurish : 2016/01/16 17 :6 :26  */
		
EigrpUtilDndbHealthy(__FILE__, __LINE__);

#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
		//EigrpUtilLinuxTimerCheck();
#endif
#ifdef _EIGRP_PLAT_MODULE_TIMER
		if(gpEigrp && gpEigrp->timerHdr)//���ȶ�ʱ��
		{
			EigrpUtilTimerCallRunning(gpEigrp->timerHdr, _EIGRP_TIMER_MAX);
		}
#endif// _EIGRP_PLAT_MODULE

EigrpUtilDndbHealthy(__FILE__, __LINE__);

#ifdef INCLUDE_EIGRP_SNMP
		eigrpSnmpProc();
#endif
EigrpUtilDndbHealthy(__FILE__, __LINE__);

#ifdef _DC_
		pktLen	= 1024 * 2;
		retVal	= EigrpPortDcRecv(pktBuf, &pktLen);
		if(retVal == SUCCESS){
			EigrpPortProcDcPkt(pktBuf, pktLen);
		}
#endif

EigrpUtilDndbHealthy(__FILE__, __LINE__);
#ifdef _EIGRP_PLAT_MODULE
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);
#endif//_EIGRP_PLAT_MODULE
/*EigrpUtilDndbHealthy(__FILE__, __LINE__);*/
	}

#ifdef _DC_
	if(pktBuf){
		EigrpPortMemFree(pktBuf);
	}
#endif

	EIGRP_FUNC_LEAVE(EigrpMain);

	/* Not reached. */
	return;
}

/************************************************************************************

	Name:	EigrpSetHandle

	Desc:	This function is to set a peer's handle in a handle structure.
		
	Para: 	ddb		- pointer to dual descriptor block 
			handles	- pointer to the handle to be set
			handle_number	- the peer's handle index
	
	Ret:		
************************************************************************************/

void	EigrpSetHandle(EigrpDualDdb_st *ddb, EigrpHandle_st *handles, S32 handle_number)
{
	EIGRP_FUNC_ENTER(EigrpSetHandle);
	if(handle_number == EIGRP_NO_PEER_HANDLE){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpSetHandle);
		return;
	}
	if((U32)handle_number >= EIGRP_CELL_TO_HANDLE(handles->arraySize)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpSetHandle);
		return;
	}
	if(EIGRP_TEST_HANDLE(handles->array, handle_number)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpSetHandle);
		return;
	}
	EIGRP_SET_HANDLE(handles->array, handle_number);
	handles->used++;
	EIGRP_FUNC_LEAVE(EigrpSetHandle);

	return;
}

/************************************************************************************

	Name:	EigrpCompareSeq

	Desc:	This function is to compare two EIGRP unsigned 32-bit sequence numbers.
 
 			This scheme treats sequence numbers as a circular space;  the temporal	ordering of the
 			two sequence numbers is based on which interpretation puts them closer together on the
 			circle.  This game is done by taking advantage of the fact that subtraction is
 			effectively circular.
 
  			Lessons learned from the timer system...
		
	Para: 	seq1		- one sequence number
			seq2		- the other sequence number
	
	Ret:		 1	for seq1 is later than seq2, 
 			 -1	for seq1 is earlier than seq2,
 			 0	for if seq1 equals seq2,
************************************************************************************/

S32	EigrpCompareSeq (U32 seq1, U32 seq2)
{
	S32 seq_diff;

	EIGRP_FUNC_ENTER(EigrpCompareSeq);
	seq_diff = seq1 - seq2;
	if(seq_diff == 0){
		EIGRP_FUNC_LEAVE(EigrpCompareSeq);
		return(0);
	}

	if(seq_diff > 0){
		EIGRP_FUNC_LEAVE(EigrpCompareSeq);
		return(1);
	}
	EIGRP_FUNC_LEAVE(EigrpCompareSeq);

	return( -1);
}

/************************************************************************************

	Name:	EigrpAllocHandle

	Desc:	This function is to find first free handle, set it and adjust as necessary.  Adjusts
			peer array as well.
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the neighbor for which the handle is set 
	
	Ret:		NONE
************************************************************************************/

void	EigrpAllocHandle(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	U32	the_bits, cell;
	S32	handle;
	U32	*cell_ptr, *target;
	EigrpIdb_st	*iidb;

	EIGRP_FUNC_ENTER(EigrpAllocHandle);
	/* First search for a word with a clear bit. */

	cell_ptr	= ddb->handle.array;
	target	= NULL;
	handle	= EIGRP_NO_PEER_HANDLE;
	for(cell = 0; cell < ddb->handle.arraySize; cell++){
		if(*cell_ptr != ~0){
			target = cell_ptr;
			break;
		}
		cell_ptr++;
	}

	/* If we found a word with a free bit, find the bit. */
	if(target){
		the_bits = *target;
		handle = EIGRP_CELL_TO_HANDLE(cell);
		while(the_bits & 0x1){		/* Better terminate! */
			the_bits >>= 1;
			handle++;
		}
	}else{
		/* No words with any free bits.  Grow the DDB handle array in reaction to this unfortunate
		  * turn of events.  We then use the first bit in the new word. */
		handle = EIGRP_CELL_TO_HANDLE(ddb->handle.arraySize);
		if(!EigrpAdjustHandleArray(&ddb->handle, handle)){ 
			handle = EIGRP_NO_PEER_HANDLE;
		}
	}

	/* Now try to grow the IIDB handles array if necessary. */
	iidb = peer->iidb;
	if(!EigrpAdjustHandleArray(&iidb->handle, handle)){ 
		handle = EIGRP_NO_PEER_HANDLE;
	}

	if(handle != EIGRP_NO_PEER_HANDLE){	/* We've got all our memory */
		/* Add the peer array entry and set the bits in the handles. */
		if(EigrpAdjustPeerArray(ddb, peer, handle)){ /* It worked */
			EigrpSetHandle(ddb, &ddb->handle, handle);
			EigrpSetHandle(ddb, &iidb->handle, handle);
			EigrpSetPacingIntervals(ddb, iidb); /* This may change pacing */
		}
	}
	peer->peerHandle = handle;
	EIGRP_FUNC_LEAVE(EigrpAllocHandle);

	return;
}

/************************************************************************************

	Name:	EigrpFindhandle

	Desc:	This function is to scan peerCache looking for nexthop.
		
	Para: 	ddb		- pointer to dual descriptor block 
			nexthop	- pointer to the nexthop needed
			pEigrpIntf	- pointer to the given eigrp interface
	
	Ret:		the handle index, or EIGRP_NO_PEER_HANDLE if there is no peer
************************************************************************************/

S32	EigrpFindhandle(EigrpDualDdb_st *ddb,   U32 *nexthop, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualPeer_st *peer;
	S32 handle;

	EIGRP_FUNC_ENTER(EigrpFindhandle);
	peer = EigrpFindPeer(ddb, nexthop, pEigrpIntf);
	if(peer != NULL){
		handle = peer->peerHandle;
		/* We probably should do something other than return the buggar if it is not already 
		  * allocated to this peer. */
		if(!EigrpTestHandle(ddb, &ddb->handle, handle)){
			EIGRP_ASSERT(0);
		}	
		EIGRP_FUNC_LEAVE(EigrpFindhandle);
		return(handle);
	}else{
		EIGRP_FUNC_LEAVE(EigrpFindhandle);
		return(EIGRP_NO_PEER_HANDLE);
	}
}

/************************************************************************************

	Name:	EigrpClearHandle

	Desc:	This function is to clear a freed handle in a handle structure.
		
	Para: 	ddb		- pointer to dual descriptor block 
			handles	- pointer to the given handle structure
			handle_number	- the index of the freed handle
	
	Ret:		NONE
************************************************************************************/

void	EigrpClearHandle(EigrpDualDdb_st *ddb, EigrpHandle_st *handles, U32 handle_number)
{
	EIGRP_FUNC_ENTER(EigrpClearHandle);
	if(handle_number == EIGRP_NO_PEER_HANDLE){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpClearHandle);
		return;
	}
	if(handle_number >= EIGRP_CELL_TO_HANDLE(handles->arraySize)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpClearHandle);
		return;
	}
	if(!EIGRP_TEST_HANDLE(handles->array, handle_number)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpClearHandle);
		return;
	}
		EIGRP_CLEAR_HANDLE(handles->array, handle_number);
	if(!handles->used){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpClearHandle);
		return;
	}
	handles->used--;
	EIGRP_FUNC_LEAVE(EigrpClearHandle);

	return;
}

/************************************************************************************

	Name:	EigrpFreehandle

	Desc:	This function is to clear out handle bit assigned to this peer.
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the given peer
	
	Ret:		NONE
************************************************************************************/

void	EigrpFreehandle(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	EigrpIdb_st *iidb;
	U32	handle;

	EIGRP_FUNC_ENTER(EigrpFreehandle);
	handle	= peer->peerHandle;

	iidb	= peer->iidb;
	EigrpClearHandle(ddb, &ddb->handle, handle);
	EigrpClearHandle(ddb, &iidb->handle, handle);
	EigrpSetPacingIntervals(ddb, iidb); /* This may change pacing. */

	if(ddb->peerArraySize > handle){
		ddb->peerArray[ handle ]	= NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpFreehandle);

	return;
}

/************************************************************************************

	Name:	EigrpTestHandle

	Desc:	This function is to see if a handle is set in a handle structure.
		
	Para: 	ddb		- pointer to dual descriptor block 
			handles	- pointer to the given handle structure
			handle_number	- the handle number to be tested
	
	Ret:		TRUE	for this handle set in the handle structure
			FALSE	for it is not so
************************************************************************************/

S32	EigrpTestHandle(EigrpDualDdb_st *ddb, EigrpHandle_st *handles, U32 handle_number)
{
	EIGRP_FUNC_ENTER(EigrpTestHandle);
	if(handle_number == EIGRP_NO_PEER_HANDLE){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpTestHandle);
		return FALSE;
	}
	if(handle_number >= EIGRP_CELL_TO_HANDLE(handles->arraySize)){
		EIGRP_FUNC_LEAVE(EigrpTestHandle);
		return FALSE;			/* Out of range... */
	}
	EIGRP_FUNC_LEAVE(EigrpTestHandle);
	
	return(EIGRP_TEST_HANDLE(handles->array, handle_number));
}

/************************************************************************************

	Name:	EigrpHandleToPeer

	Desc:	This function is to return a pointer to the peer structure, given the peer handle 
			number.
		
	Para: 	ddb		- pointer to dual descriptor block 
			handle	- the given peer handle
	
	Ret:		pointer to the peer structure
************************************************************************************/

EigrpDualPeer_st *EigrpHandleToPeer(EigrpDualDdb_st *ddb, U32 handle)
{
	EIGRP_FUNC_ENTER(EigrpHandleToPeer);
	if(handle == EIGRP_NO_PEER_HANDLE){
		EIGRP_FUNC_LEAVE(EigrpHandleToPeer);
		return NULL;
	}
	if((U32) handle >= ddb->peerArraySize){         	/* Covers negatives as well */
		EIGRP_FUNC_LEAVE(EigrpHandleToPeer);
		return NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpHandleToPeer);
	
	return(ddb->peerArray[ handle]);
}

/************************************************************************************

	Name:	EigrpShouldAcceptSeqPacket

	Desc:	This function is to return an indication as to whether the packet should be accepted
			(and acknowledged), rejected with acknowledgement, or rejected without comment.
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the peer which the packet should be sent to
			eigrp	- pointer to received packet header
			sequence	-received packet sequence number
	Ret:		                the indication as to whether the packet should be accepted
************************************************************************************/

U32 EigrpShouldAcceptSeqPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpPktHdr_st *eigrp, U32 sequence)
{
	EIGRP_FUNC_ENTER(EigrpShouldAcceptSeqPacket);
	/* Is the packet in sequence? */
	if(EigrpCompareSeq(sequence, peer->lastSeqNo) > 0){

		/* Yep.  Is the CR bit set? */
		if(NTOHL(eigrp->flags) & EIGRP_CR_FLAG){

			/* Yep.  Are we in CR mode? */
			if(peer->flagCrMode){

				/* Yep.  Did the Sequence TLV come with a sequence number? */
				if(peer->crSeq){

					/* Yep.  Is it correct? */
					if(peer->crSeq == sequence){
						EIGRP_FUNC_LEAVE(EigrpShouldAcceptSeqPacket);

						/* Yep.  Accept the packet. */
						return(EIGRP_PKT_ACCEPT);

					}else{		/* Sequence mismatch */
						EIGRP_FUNC_LEAVE(EigrpShouldAcceptSeqPacket);
						/* Bad sequence.  Don't accept. */
						return( EIGRP_PKT_REJECT);
					}
				}else{		/* No sequence received */
					EIGRP_FUNC_LEAVE(EigrpShouldAcceptSeqPacket);
					/* Unsequenced CR packet.  Accept it. */
					return(EIGRP_PKT_ACCEPT);
				}
			}else{			/* Not in CR mode */
				EIGRP_FUNC_LEAVE(EigrpShouldAcceptSeqPacket);
				/* CR packet not in CR mode.  Don't accept. */
				return( EIGRP_PKT_REJECT);
			}
		}else{			/* CR flag clear */
			EIGRP_FUNC_LEAVE(EigrpShouldAcceptSeqPacket);
			/* No CR flag on the packet.  Accept it. */
			return(EIGRP_PKT_ACCEPT);
		}
	}else{				/* Out of sequence */
		/* Out of sequence.  Don't accept it, but acknowledge it. */
		peer->iidb->outOfSeqRcvd++;
		EIGRP_FUNC_LEAVE(EigrpShouldAcceptSeqPacket);
		return( EIGRP_PKT_REJECT_WITH_ACK);
	}
}

/************************************************************************************

	Name:	EigrpCancelCrMode

	Desc:	This function is to cancel any pending Conditional Receive state
		
	Para: 	peer		- pointer to the neighbor which the pending Conditional Receive state belongs to
	
	Ret:		NONE
************************************************************************************/

void	EigrpCancelCrMode(EigrpDualPeer_st *peer)
{
	EIGRP_FUNC_ENTER(EigrpCancelCrMode);
	peer->flagCrMode		= FALSE;
	peer->crSeq	= 0;
	EIGRP_FUNC_LEAVE(EigrpCancelCrMode);

	return;
}

/************************************************************************************

	Name:	EigrpAcceptSequencedPacket

	Desc:	This function is to process sequence number from received EIGRP packet. 
 
 			Tracks the expected setting of the CR bit as well, and updates the peer lastSeqNo field
 			if the packet is accepted.
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the peer who sent the packet
			eigrp	- pointer t0 the eigrp packet
			
	
	Ret:		TRUE	for the packet is to be accepted
			FALSE	for it is not so 
************************************************************************************/

S32	EigrpAcceptSequencedPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpPktHdr_st *eigrp)
{
	U32 sequence;
	S32 result;
	U32 opcode;

	EIGRP_FUNC_ENTER(EigrpAcceptSequencedPacket);
	sequence	= NTOHL(eigrp->sequence);
	opcode	= eigrp->opcode;
	if(opcode == EIGRP_OPC_HELLO && NTOHL(eigrp->ack)){
		opcode = EIGRP_OPC_ACK;
	}

	/* Do what our handler tells us to do. */
	switch(EigrpShouldAcceptSeqPacket(ddb, peer, eigrp, sequence)){
		case EIGRP_PKT_ACCEPT:
			peer->lastSeqNo = sequence;
			result = TRUE;
			break;

		case EIGRP_PKT_REJECT:
			result = FALSE;
			break;

		case EIGRP_PKT_REJECT_WITH_ACK:
			EigrpSendAck(ddb, peer, sequence);
			if(sequence != peer->lastSeqNo){ /* Not a duplicate! */
				EigrpDualLogXport(ddb, EIGRP_DUALEV_BADSEQ, &peer->source, &peer->source, &opcode);
			}
			result = FALSE;
			break;

		default:         				/* Shouldn't happen. */
			result = FALSE;
			break;
	}

	/* Cancel CR mode if it was set. */
	if(peer->flagCrMode){
		EigrpCancelCrMode(peer);
	}
	EIGRP_FUNC_LEAVE(EigrpAcceptSequencedPacket);

	return(result);
}

/************************************************************************************

	Name:	EigrpPakSanity

	Desc:	This function is to do the sanity check  on an incoming EIGRP packet.
 			Return pointer to EIGRP header if packet passed checks, otherwise NULL.
		
	Para: 	ddb		- pointer to dual descriptor block 
			eigrph	- pointer t0 the eigrp packet
			size		- packet size
			source	- pointer to the packet source address ,unused for ths function
			pEigrpIntf	- the eigrp interface on which the packet is received
	
	Ret:		TRUE	for packet passed checks
			FALSE	for it is not so
************************************************************************************/

S32	EigrpPakSanity(EigrpDualDdb_st *ddb, EigrpPktHdr_st *eigrph, U32 size, U32 *source, EigrpIntf_pt pEigrpIntf)
{
	U16	checksum, checksum2;
	EIGRP_FUNC_ENTER(EigrpPakSanity);
	/* Kill brain dead interfaces which are down but still receive. */
	if(!BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
		EIGRP_FUNC_LEAVE(EigrpPakSanity);
		return FALSE;
	}

	/* Check version number.  */
	if(eigrph->version != EIGRP_VERSION){
		EIGRP_FUNC_LEAVE(EigrpPakSanity);
		return FALSE;
	}
	/* Verify checksum in datagram. */
	checksum = eigrph->checksum; /* for md5 authentication */
	eigrph->checksum = 0;
	checksum2 = EigrpUtilGetChkSum((void *) eigrph, size);
	if(checksum2 != checksum){
		printf("EIGRP: checksum error!\n");
		EIGRP_FUNC_LEAVE(EigrpPakSanity);
		return FALSE;
	}

	/* Validate autonomous system number */
	if(NTOHL(eigrph->asystem) != ddb->asystem){
		EIGRP_FUNC_LEAVE(EigrpPakSanity);
		return FALSE;
	}
	EIGRP_FUNC_LEAVE(EigrpPakSanity);
	
	return TRUE;
}
  
/************************************************************************************

	Name:	EigrpSendAck

	Desc:	This function is to enqueue an acknowledgement for the given peer and sequence number.
 
 			We will not enqueue the ACK if we are waiting for our INIT packet to be acknowledged; 
 			rather, we will carry the ACK in an upcoming data packet(such as the retransmission of
 			the INIT).
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the given peer
			peer		- the given seq
	
	Ret:		NONE
************************************************************************************/

void	EigrpSendAck(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, U32 seq)
{
	EigrpPackDesc_st	*pktDesc;
	EigrpIdb_st	*iidb;

	EIGRP_FUNC_ENTER(EigrpSendAck);

	iidb = peer->iidb;
	if(peer->flagNeedInitAck){		/* We don't send ACKs yet. */
		EIGRP_FUNC_LEAVE(EigrpSendAck);
		return;
	}

	pktDesc = EigrpEnqueuePak(ddb, peer, iidb, FALSE);
	if(pktDesc){
		pktDesc->opcode			= EIGRP_OPC_HELLO;
		pktDesc->ackSeqNum	= seq;
		pktDesc->flagAckPkt		= TRUE;
		EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
	}
	EIGRP_FUNC_LEAVE(EigrpSendAck);

	return;
}

/************************************************************************************

	Name:	EigrpTakeAllNbrsDown

	Desc:	This function is to take all neighbors down. This is called when global IGRP parameters
			are changed.
 
 			This routine can be safely called from other threads, since it schedules the peers to
 			be taken down;  the process does the real work.
		
	Para: 	ddb		- pointer to dual descriptor block 
			reason	- pointer the string which indicates ther event reason
	
	Ret:		
************************************************************************************/

void	EigrpTakeAllNbrsDown(EigrpDualDdb_st *ddb, S8 *reason)
{
	EigrpDualPeer_st *peer, *next;

	EIGRP_FUNC_ENTER(EigrpTakeAllNbrsDown);
	for(peer = ddb->peerLst; peer; peer = next){
		next = peer->nextPeer;
		EigrpTakePeerDown(ddb, peer, FALSE, reason);
	}
	EIGRP_FUNC_LEAVE(EigrpTakeAllNbrsDown);

	return;
}
  
/************************************************************************************

	Name:	EigrpFindIidb

	Desc:	This function is to find EIGRP iidb structure for given ddb and idb.
		
	Para: 	ddb		- pointer to dual descriptor block 
			pEigrpIntf	- pointer to the given idb
	
	Ret:		pointer to the EIGRP iidb ,or NULL if nothing is found
************************************************************************************/

EigrpIdb_st *EigrpFindIidb(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf)
{
	EigrpIdbLink_st *current_link;

	EIGRP_FUNC_ENTER(EigrpFindIidb);
	/* Walk the thread, looking for a DDB match. */
	if(!pEigrpIntf || !ddb){
		EIGRP_FUNC_LEAVE(EigrpFindIidb);
		return NULL;
	}

	current_link = pEigrpIntf->private;

	while(current_link){
		if(current_link->ddb == ddb){
			break;
		}
		current_link = current_link->next;
	}

	if(current_link){			/* Found it! */
		EIGRP_FUNC_LEAVE(EigrpFindIidb);
		return(current_link->iidb);
	}else{				/* Darn. */
		EIGRP_FUNC_LEAVE(EigrpFindIidb);
		return NULL;
	}
}

/************************************************************************************

	Name:	EigrpFindPeer

	Desc:	This function is to find peer from ddb with matching address.
		
	Para: 	ddb		- pointer to dual descriptor block 
			address	- pointer to the given ip address
			pEigrpIntf	- the interface on which the packet is received
	
	Ret:		
************************************************************************************/

EigrpDualPeer_st *EigrpFindPeer(EigrpDualDdb_st *ddb, U32 *address, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualPeer_st *peer;
	S32 slot;

	EIGRP_FUNC_ENTER(EigrpFindPeer);
	if(address == NULL){
		EIGRP_FUNC_LEAVE(EigrpFindPeer);
		return NULL;
	}

	/* 0.0.0.0 will not be our peer. */
	if(*address == 0){
		EIGRP_FUNC_LEAVE(EigrpFindPeer);
		return NULL;
	}

	slot = (*ddb->peerBucket)(address);
	for(peer = ddb->peerCache[ slot ]; peer; peer = peer->next){
		if(peer->iidb->idb != pEigrpIntf){
			continue;
		}
		if((*ddb->addressMatch)(&peer->source, address)){
			EIGRP_FUNC_LEAVE(EigrpFindPeer);
			return(peer);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpFindPeer);
	
	return NULL;
}

/************************************************************************************

	Name:	

	Desc:	This function is to return the type of the transmit queue(reliable/unreliable) from
			which the next packet should be taken to send to the peer.
 
 			The rule is that unrelilable packets take precedence until peer->unrelyLeftToSend is
 			zero, at which point a reliable packet will be sent(to keep the unreliables from
 			starving the reliable queue).
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the peer
	
	Ret:		EIGRP_RELIABLE_QUEUE or EIGRP_UNRELIABLE_QUEUE
************************************************************************************/

U32 EigrpNextPeerQueue(EigrpDualDdb_st *ddb,  EigrpDualPeer_st *peer)
{
	EigrpPktDescQelm_st *qelm;

	EIGRP_FUNC_ENTER(EigrpNextPeerQueue);
	if(peer->xmitQue[ EIGRP_UNRELIABLE_QUEUE ]->count){

		/* Send unreliable packets if there are some present, and if we're still allowed to send 
		  * them, or if there are no reliable packets to send, or if the reliable packet is waiting
		  * to be sent. */
		qelm = (EigrpPktDescQelm_st*)peer->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head;
		if(peer->unrelyLeftToSend || !qelm){
			EIGRP_FUNC_LEAVE(EigrpNextPeerQueue);
			return(EIGRP_UNRELIABLE_QUEUE);
		}
		if(EIGRP_TIMER_RUNNING_AND_SLEEPING(qelm->xmitTime)){
			EIGRP_FUNC_LEAVE(EigrpNextPeerQueue);
			return(EIGRP_UNRELIABLE_QUEUE);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpNextPeerQueue);

	return( EIGRP_RELIABLE_QUEUE);
}

/************************************************************************************

	Name:	EigrpDeferPeerTimer

	Desc:	This function is to set the peer timer to expire at the same time as the IIDB circuit
 			pacing timer.  If the IIDB timer isn't running, don't touch the peer timer.
 
			Returns TRUE if the timer was set, FALSE if not.
		
	Para: 	ddb		- pointer to dual descriptor block 
			iidb		- pointer to the given IIDB
			peer		- pointer o the given peer
	
	Ret:		
************************************************************************************/

S32	EigrpDeferPeerTimer(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpDualPeer_st *peer)
{
	U32 exptime;

	EIGRP_FUNC_ENTER(EigrpDeferPeerTimer);
	if(EigrpUtilMgdTimerRunning(&iidb->sndTimer)){
		exptime = EigrpUtilMgdTimerExpTime(&iidb->sndTimer);
		EigrpUtilMgdTimerSetExptime(&peer->peerSndTimer, exptime);
		EIGRP_FUNC_LEAVE(EigrpDeferPeerTimer);
		return TRUE;
	}else{
		EIGRP_FUNC_LEAVE(EigrpDeferPeerTimer);
		return FALSE;
	}
}

/************************************************************************************

	Name:	EigrpStartPeerTimer

	Desc:	This function is to start the transmit/retransmit timer for a peer.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the IIDB structure 
			peer		- pointer to the given peer
	
	Ret:		NONE
************************************************************************************/

void	EigrpStartPeerTimer(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpDualPeer_st *peer)
{
	EigrpPktDescQelm_st	*qelm;
	EigrpQue_st			*send_queue;

	EIGRP_FUNC_ENTER(EigrpStartPeerTimer);
	/* If both transmit queues are empty, stop the timer and bail. */
	send_queue = peer->xmitQue[ EigrpNextPeerQueue(ddb, peer) ];

	if(!send_queue->head || !send_queue->count){
		if(EigrpUtilMgdTimerRunning(&peer->peerSndTimer)){
			EigrpUtilMgdTimerStop(&peer->peerSndTimer);
		}
		EIGRP_FUNC_LEAVE(EigrpStartPeerTimer);
		return;
	}

	/* Grab the first element on the queue. */
	qelm = (EigrpPktDescQelm_st*)send_queue->head;

	/* If there is no transmit time set on this packet, set it to expire at the next pacing
	  * interval on the interface.  If the interface timer isn't running, set the peer timer to
	  * expire now. */
	if(!EIGRP_TIMER_RUNNING(qelm->xmitTime)){
		if(!EigrpDeferPeerTimer(ddb, iidb, peer)){
			EigrpUtilMgdTimerStart(&peer->peerSndTimer, 0);
		}
	}else{

		/* Set the timer to the time when the packet is to be sent. */
		EigrpUtilMgdTimerSetExptime(&peer->peerSndTimer, qelm->xmitTime);
	}
	EIGRP_FUNC_LEAVE(EigrpStartPeerTimer);

	return;
}

/************************************************************************************

	Name:	

	Desc:	This function is to perform cleanup after a packet has been acknowledged(or a unicast
 				
			transmission has been suppressed).
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer the peer which the ACK should be sent to
			pktDesc	- pointer to the packet descriptor
	
	Ret:		NONE
************************************************************************************/

void	EigrpPostAck(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpPackDesc_st *pktDesc)
{
	EIGRP_FUNC_ENTER(EigrpPostAck);
	/* Dequeue the packet and free the appropriate stuff. */
	EigrpDequeueQelm(ddb, peer, EIGRP_RELIABLE_QUEUE);
	peer->retryCnt = 0;

	/* Kick the peer timer based on the next item on the queue(if any). */
	EigrpStartPeerTimer(ddb, peer->iidb, peer);
	EIGRP_FUNC_LEAVE(EigrpPostAck);

	return;
}

/************************************************************************************

	Name:	EigrpUpdateSrtt

	Desc:	This function is to update the peer SRTT.  Maintains the IIDB total SRTT field(which 
			allows us to calculate an average SRTT for multicast flow control purposes).
		
	Para: 	peer		- pointer to the given peer
			new_value	- the new value of SRTT
	
	Ret:		NONE
************************************************************************************/

void	EigrpUpdateSrtt(EigrpDualPeer_st *peer, U32 new_value)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpUpdateSrtt);
	iidb = peer->iidb;

	/* Remove the old value from total and replace it with the new value. */
	new_value = MAX(new_value, 1);	/* It takes some time... */
	if(iidb){
		iidb->totalSrtt -= peer->srtt;
		iidb->totalSrtt += new_value;
	}
	peer->srtt = new_value;
	EIGRP_FUNC_LEAVE(EigrpUpdateSrtt);

	return;
}

/************************************************************************************

	Name:	

	Desc:	This function uses retransmission timeout estimation procedure from RFC 793 (TCP).
 			All times are in milliseconds.
 
 			rtt = is calculated round trip time.
			srtt = is a smoothed round trip time which is bounded.
			rto = retransmission timeout.
 			
	Para: 	peer		- pointer to the given peer
			rtt		- calculated round trip time
	
	Ret:		NONE
************************************************************************************/

void	EigrpCalculateRto(EigrpDualPeer_st *peer, U32 rtt)
{
	EIGRP_FUNC_ENTER(EigrpCalculateRto);
	if(!peer->srtt){
		EigrpUpdateSrtt(peer, rtt);
	}else{
		/* srtt = (srtt * .8) + (rtt * .2) */
		EigrpUpdateSrtt(peer, (peer->srtt * 8 ) / 10 + (rtt * 2) / 10);
	}

	/* The baseline RTO is EIGRP_RTO_MULTIPLIER * RTT. */
	peer->rto = peer->srtt *EIGRP_RTO_SRTT_MULTIPLIER;

	/* Apply an absolute lower bound. */
	peer->rto = MAX(peer->rto, EIGRP_RTO_LOWER);

	/* Apply a lower bound based on the interface pacing timer. */
	peer->rto = MAX(peer->rto,
						peer->iidb->sndInterval[ EIGRP_RELIABLE_QUEUE ] *
						EIGRP_RTO_PACING_MULTIPLIER);

	/* Apply an upper bound. */
	peer->rto = MIN(peer->rto, EIGRP_RTO_UPPER);
	EIGRP_FUNC_LEAVE(EigrpCalculateRto);

	return;
}

/************************************************************************************

	Name:	EigrpProcessAck

	Desc:	This function is to process a received acknowledgement.  Potentially dequeue a packet
			on the retransmission queue.  Alert the higher levels.
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the peer who sent the ACK
			ack_seq	- the sequence number of the received ACK packet
			
	Ret:		NONE
************************************************************************************/

void	EigrpProcessAck(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, U32 ack_seq)
{
	EigrpPktDescQelm_st	*qelm;
	EigrpPackDesc_st		*pktDesc;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	struct EigrpXmitThread_ *thread;
	S8	opcode;
	_EIGRP_DEBUG("EigrpProcessAck Enter\n");

	EIGRP_FUNC_ENTER(EigrpProcessAck);
	/* Get the first sequenced packet on the queue. */
	qelm = (EigrpPktDescQelm_st*)peer->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head;
	if(!qelm) {        				/* Nothing there. */
		EIGRP_FUNC_LEAVE(EigrpProcessAck);
		return;
	}
	pktDesc = qelm->pktDesc;
	
#if 0	   /*zhangming_130131 TODO:~{SCSZ=b>v7V2fA4B7B7SIUp54#,7G7V2fA4B7?ID\SPNJLb#,PhR*MjIF~}*/
	if(!pktDesc->pktDescAnchor || !pktDesc){ 	/*zhangming_130130*/
		return;
	}
	
	thread = pktDesc->pktDescAnchor->thread;
	opcode = pktDesc->opcode;
#endif

	//if(app_debug)printf("EigrpProcessAck 1 thread->refCnt=%d\n", thread->refCnt);

	/* If the acknowledgement doesn't match the packet, ignore it. */
	if(ack_seq != pktDesc->ackSeqNum){
		EIGRP_FUNC_LEAVE(EigrpProcessAck);
		return;
	}

	/* Calculate the RTT if we didn't retransmit. */
	if(!peer->retryCnt){
		if(EIGRP_TIMER_RUNNING(qelm->sentTime)){ /* Should be, but... */
			EigrpCalculateRto(peer, (U32)EIGRP_ELAPSED_TIME(qelm->sentTime) * EIGRP_MSEC_PER_SEC);
		}
	}

	/* Clean up. */
	EigrpPostAck(ddb, peer, pktDesc);
//	if(app_debug)printf("EigrpProcessAck 2 thread->refCnt=%d\n", thread->refCnt);
#if 0     /*zhangming_130131 TODO:~{SCSZ=b>v7V2fA4B7B7SIUp54#,7G7V2fA4B7?ID\SPNJLb#,PhR*MjIF~}*/
	/*zhenxl_20130117 The DNDB had been advertised with infinity metric, now we should distroy it.*/
	if(opcode == EIGRP_OPC_UPDATE){
		if(thread->drdb){
			drdb = EigrpDualThreadToDrdb(thread);
			dndb = drdb->dndb;
		}else{
			dndb = EigrpDualThreadToDndb(thread);
		}
		if(dndb->exterminate == TRUE && thread->refCnt <= 0){
			while((drdb = dndb->rdb) != NULL){
				EigrpDualZapDrdb(ddb, dndb, drdb, TRUE);
			}
			
			/* Clear out the reply-status table if it exists, wipe out the ndb, and move on. */
			if(EigrpDualDndbActive(dndb)){
				if(dndb->replyStatus.array){
					EigrpPortMemFree(dndb->replyStatus.array);
				}
				dndb->replyStatus.array = NULL;
				dndb->replyStatus.used = 0;
				dndb->replyStatus.arraySize = 0;
			}
			EigrpDualDndbDelete(ddb, &dndb);
		}
	}
#endif
	EIGRP_FUNC_LEAVE(EigrpProcessAck);

	return;
}

/************************************************************************************

	Name:	EigrpNeighborDown
	
	Desc:	This function is to take a neighbor down.  This starts the twisty passage to down-ness.

			We guarantee that we are on the router process thread, so we can do this operation
			atomically.
 
 			This calls the neighbor_down entry in the protocol-specific code, which does some 
 			cleanup and then calls the DUAL code to clean up, and finally calls the
 			EigrpDestroyPeer function.
		
	Para: 	ddb		- pointer to the dual descriptor block
			peer		- pointer to the peer that will be token down
	
	Ret:		NONE
************************************************************************************/

void	EigrpNeighborDown(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	EigrpIdb_st *iidb;
	U32  peerAddr;
	_EIGRP_DEBUG("EigrpNeighborDown Enter\n");

	EIGRP_FUNC_ENTER(EigrpNeighborDown);
	/* Inform the DUAL finite state machine. */
	peer->flagGoingDown	= TRUE;		/* We're half-down */
	peer->downReason	= " ";
	iidb = peer->iidb;

	peerAddr = peer->source;
	EigrpDualPeerDown(ddb, peer);
#ifdef CETC50_API
/***********add 20121126 for ~{SC;'5c6/L,=SHk~}***********************/
	leafNodeDynamicHelloMechanism(0, peerAddr, iidb->idb->name);
/********end add 20121126 for ~{SC;'5c6/L,=SHk~}***********************/
#endif
	/* Fake an SIA so that everybody saves their log, if so desired. */
	if(ddb->flagKillAll){
		EigrpDualSiaCopyLog(ddb);
	}
	EIGRP_FUNC_LEAVE(EigrpNeighborDown);

	return;
}

/************************************************************************************

	Name:	EigrpReinitPeer

	Desc:	This function is reinit an eigrp neighbor data structure.
		
	Para: 	ddb		- pointer to the dual descriptor block
			peer		- pointer to the eigrp neighbor data structure which need to be reinit
	
	Ret:		NONE
************************************************************************************/

void	EigrpReinitPeer(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpReinitPeer);
	iidb = peer->iidb;

	if(!EIGRP_TIMER_RUNNING(peer->reInitStart)){
		EIGRP_GET_TIME_STAMP(peer->reInitStart);
	}

	if(EIGRP_CLOCK_IN_INTERVAL(peer->reInitStart, peer->lastHoldingTime / EIGRP_MSEC_PER_SEC)){
		EIGRP_FUNC_LEAVE(EigrpReinitPeer);
		return;
	}

	EigrpNeighborDown(ddb, peer);	/* Peer is now invalid! */

	/* Make sure hello timer is going to go off soon, so that we expedite getting in sync with this
	  * neighbor. */
	if(EigrpUtilMgdTimerLeftSleeping(&iidb->helloTimer) >  EIGRP_INIT_HELLO_DELAY){
		EigrpUtilMgdTimerStart(&iidb->helloTimer, EIGRP_INIT_HELLO_DELAY);
	}
	EIGRP_FUNC_LEAVE(EigrpReinitPeer);

	return;
}

/************************************************************************************

	Name:	EigrpTakePeerDown

	Desc:	This function is to take down a single peer.  
 
 			This routine can be safely called from other threads, since it schedules the peers to be
 			taken down;  the process does the real work.
 
 			However, if the "destroy_now" flag is set, the peer is destroyed immediately.  This 
 			must only be called when the process is being destroyed.
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the peer that will be token down
			destory_now	- the sign by which we judge whether the peer need to be deleted 
			immediately
			reason	- pointer to the string which indicate the event reason
	
	Ret:		NONE
************************************************************************************/

void	EigrpTakePeerDown_int(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, S32 destroy_now, S8 *reason, S8 *file, int line)
{
	EIGRP_FUNC_ENTER(EigrpTakePeerDown);
	peer->flagGoingDown	= TRUE;		/* Don't let a Hello sneak in there */
	peer->downReason	= reason;
	if(destroy_now){
		EigrpNeighborDown(ddb, peer);	/* Do it now. */
	}else{
		EigrpUtilMgdTimerStart(&peer->holdingTimer, 0); /* Defer it. */
	}
	EIGRP_FUNC_LEAVE(EigrpTakePeerDown);

	return;
}

/************************************************************************************

	Name:	EigrpProcessStarting

	Desc:	This function is to do common processing when an EIGRP process starts.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpProcessStarting(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpProcessStarting);
	if(gpEigrp->rtrCnt == 0){
		/* First one? */
		/* First the packet descriptors */
		gpEigrp->pktDescChunks = EigrpUtilChunkCreate(sizeof(EigrpPackDesc_st),
													EIGRP_MIN_PKT_DESC,
													EIGRP_CHUNK_FLAG_DYNAMIC,
													NULL, 0,
													"EIGRP packet descriptors");

		/* Now the packet descriptor queue elements. */
		gpEigrp->pktDescQelmChunks = EigrpUtilChunkCreate(sizeof(EigrpPktDescQelm_st),
													EIGRP_MIN_PKT_DESC_QELM,
													EIGRP_CHUNK_FLAG_DYNAMIC,
													NULL, 0,
													"EIGRP queue elements");

		/* Now the anchor entries. */
		gpEigrp->anchorChunks = EigrpUtilChunkCreate(sizeof(EigrpXmitAnchor_st),
													EIGRP_MIN_ANCHOR,
													EIGRP_CHUNK_FLAG_DYNAMIC, NULL, 0,
													"EIGRP anchor entries");

		/* Now the dummy thread entries. */
		gpEigrp->dummyChunks = EigrpUtilChunkCreate(sizeof(EigrpXmitThread_st),
													EIGRP_MIN_THREAD_DUMMIES,
													EIGRP_CHUNK_FLAG_DYNAMIC,
													NULL, 0,
													"EIGRP dummy thread entries");

		/* Now the input packet headers. */
	}
	gpEigrp->rtrCnt++;
	EIGRP_FUNC_LEAVE(EigrpProcessStarting);

	return;
}

/************************************************************************************

	Name:	EigrpUpdatePeerHoldtimer

	Desc:	This function is to process for handling EIGRP adjacency management.
		
	Para: 	ddb		- pointer to dual descriptor block 
			source	- pointer the ip address of peer 
			pEigrpIntf	- pointer to the eigrp interface on which the packet received
	
	Ret:		
************************************************************************************/

S32	EigrpUpdatePeerHoldtimer(EigrpDualDdb_st *ddb, U32 *source, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualPeer_st *peer;

	EIGRP_FUNC_ENTER(EigrpUpdatePeerHoldtimer);
	/* Got a packet.  Look for a peer. */
	peer = EigrpFindPeer(ddb, source, pEigrpIntf);

	if(peer){
		/* If the peer is going down, burn the packet. */
		if(peer->flagGoingDown){
			EIGRP_FUNC_LEAVE(EigrpUpdatePeerHoldtimer);
			return TRUE;
		}
/* tigerwh 120607 begin */
if(peer->lastHoldingTime < 15 * 1000){
	peer->lastHoldingTime	= 15 * 1000;
}
/* tigerwh 120607 end */


		/* Push the holding time back up. */
		EigrpUtilMgdTimerStart(&peer->holdingTimer, (S32)(peer->lastHoldingTime));
	}
	EIGRP_FUNC_LEAVE(EigrpUpdatePeerHoldtimer);

	return FALSE;
}

/************************************************************************************

	Name:	EigrpLogPeerChange

	Desc:	This function is to log a change in peer status.
		
	Para: 	ddb		- pointer to the dual descriptor block
			peer		- unused
			sense	- unused
			msg		- unused
	
	Ret:		NONE
************************************************************************************/

void	EigrpLogPeerChange(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, S32 sense, S8 *msg)
{
	EIGRP_FUNC_ENTER(EigrpLogPeerChange);
	if(ddb->flagLogAdjChg){
		EIGRP_ASSERT(0);
	}
	EIGRP_FUNC_LEAVE(EigrpLogPeerChange);

	return;
}

/************************************************************************************

	Name:	EigrpFindSequenceTlv

	Desc:	This function is to find Sequence TLV. If found see if this system's address is in the
			list. If so, we do not go into CR-mode, and the next packet received with the CR-bit
			set, will not be accepted. If in CR-mode, it will be accepted.
 
 			A Sequnce TLV has following format:
	
			type(2 bytes)     EIGRP_SEQUENCE
			length(2 bytes)   TLV length + 4
			address-1:
				length(1 byte)   Length of address
				address(>1 byte) Value of address
			address-n:
				length(1 byte)   Length of address
				address(>1 byte) Value of address
		
	Para: 	ddb		- pointer to the dual descriptor block
			peer		- pointer to the peer who sent the packet
			tlv_hdr	- pointer to the TLV field
			paklen	- length of the TLV field
	
	Ret:		NONE
************************************************************************************/

void	EigrpFindSequenceTlv (EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpGenTlv_st *tlv_hdr, S32 pak_len)
{
	S8	*addr_ptr;
	S32	length;
	S32	len;

	EIGRP_FUNC_ENTER(EigrpFindSequenceTlv);
	/* Point to data. */
	length	= MIN(pak_len, NTOHS(tlv_hdr->length)) - sizeof(EigrpGenTlv_st);
	addr_ptr	= (S8 *)(tlv_hdr + 1);

	/* Go through each address in the Sequence TLV. If one of our addresses is found, clear CR-mode
	  * , otherwise set it. (addr_ptr+1)skips past the length byte. */
	while(length > 0){
		if((*ddb->localAddress)(ddb, (U32 *)(addr_ptr + 1), peer->iidb->idb)){
			EigrpCancelCrMode(peer);
			EIGRP_FUNC_LEAVE(EigrpFindSequenceTlv);
			return;
		}

		/* First byte of address contains length of address. */
		len = *addr_ptr + 1;
		length -= len;
		addr_ptr += len;
	}

	/* Our address not found, go into CR-mode for respective neighbor. */
	peer->flagCrMode = TRUE;
	EIGRP_FUNC_LEAVE(EigrpFindSequenceTlv);

	return;
}

/************************************************************************************

	Name:	EigrpRecvHello

	Desc:	This function is to process received hello packets.
 
 			For a Parameter TLV included in a packet, a verification of the K-values are performed
 			and the holdTime is stored in the peer cache.
 			
 			If this is a new peer, we will call DUAL to fashion the update stream	for the new
 			peer.  In this case, we may suspend!
		
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- pointer to the peer who sent the packet
			eigrp	- pointer to the packet header
			paklen	- the packet size
	
	Ret:		NONE
************************************************************************************/

void	EigrpRecvHello(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpPktHdr_st *eigrp, S32 paklen)
{

	EigrpGenTlv_st	*tlv_hdr;
	EigrpParamTlv_st	*param;
	EigrpVerTlv_st		*version_tlv;
	EigrpNextMultiSeq_st	*multiseq_tlv;
	U16	length;
	S32	new_peer, seq_tlv_found, cr_seq_found, error_found;

	EIGRP_FUNC_ENTER(EigrpRecvHello);
	paklen -= EIGRP_HEADER_BYTES;
	if(paklen <= 0){
		EIGRP_FUNC_LEAVE(EigrpRecvHello);
		return;
	}

	/* We haven't seen any CR or SEQ TLV stuff so far. */
	seq_tlv_found	= FALSE;
	cr_seq_found	= FALSE;
	error_found	= FALSE;

	tlv_hdr = (EigrpGenTlv_st *)(eigrp + 1);

	new_peer = FALSE;
	while(paklen > 0){
		switch(NTOHS(tlv_hdr->type)){
			case EIGRP_SEQUENCE:
				/* Process Sequence TLV. */
				EigrpFindSequenceTlv(ddb, peer, tlv_hdr, paklen);
				seq_tlv_found = TRUE;
				break;

			case EIGRP_PARA:
				/* Process Parameter TLV. */
				param = (EigrpParamTlv_st *) tlv_hdr;
				if(!EigrpDualKValuesMatch(ddb, param)){
					EigrpLogPeerChange(ddb, peer, FALSE, "K-value mismatch");
					EigrpNeighborDown(ddb, peer);
					EIGRP_FUNC_LEAVE(EigrpRecvHello);
					return;
				}else{
					new_peer = !EigrpUtilMgdTimerRunning(&peer->holdingTimer);
					peer->lastHoldingTime = NTOHS(param->holdTime) * EIGRP_MSEC_PER_SEC;
/* tigerwh 120607 begin */
if(peer->lastHoldingTime < 15 * EIGRP_MSEC_PER_SEC){
	peer->lastHoldingTime	= 15 * EIGRP_MSEC_PER_SEC;
}
/* tigerwh 120607 end */
					EigrpUtilMgdTimerStart(&peer->holdingTimer, (S32)(peer->lastHoldingTime));
				}
				break;

			case EIGRP_SW_VERSION:
				/* Process software version number TLV. */
				version_tlv = (EigrpVerTlv_st *) tlv_hdr;
				EigrpPortMemCpy((void *) & peer->peerVer, (void *) & version_tlv->version, sizeof(EigrpSwVer_st));
				break;

			case EIGRP_NEXT_MCAST_SEQ:
				/* Process next multicast sequence number TLV. */
				multiseq_tlv = (EigrpNextMultiSeq_st *) tlv_hdr;
				peer->crSeq = NTOHL(multiseq_tlv->seqNum);
				cr_seq_found = TRUE;
				break;

			default:
				/* Skip the unknown TLV */
				break;
		}

		length = NTOHS(tlv_hdr->length);
		if(length < sizeof(EigrpGenTlv_st)){
			error_found = TRUE;
			break;
		}
		paklen -= length;
		tlv_hdr = (EigrpGenTlv_st *)(((S8 *) tlv_hdr) + length);
	}

	/* Bail if the last TLV was short. */
	if(paklen < 0){
		error_found = TRUE;
	}

	/* If we got an error, clean up and bail out. */
	if(error_found){
		EigrpCancelCrMode(peer);
		EIGRP_FUNC_LEAVE(EigrpRecvHello);
		return;
	}

	/* If we got a CR sequence number without a Sequence TLV, cancel it. If we got a Sequence TLV
	  * without a CR sequence number, we're talking to an old-style peer;  clear any old CR
	  * sequence number for completeness. */
	if(cr_seq_found && !seq_tlv_found){
		EigrpCancelCrMode(peer);
	}else if(seq_tlv_found && !cr_seq_found){
		peer->crSeq = 0;
	}

	/* If this is a new peer, tell DUAL to send an update stream. */
	if(new_peer){
		EigrpLogPeerChange(ddb, peer, TRUE, "new adjacency");
		EigrpDualNewPeer(ddb, peer);
#ifdef CETC50_API
/***********add 20121126 for ~{SC;'5c6/L,=SHk~}***********************/
		leafNodeDynamicHelloMechanism(1, peer->source, peer->iidb->idb->name);
/********end add 20121126 for ~{SC;'5c6/L,=SHk~}***********************/		
#endif
	}
	EIGRP_FUNC_LEAVE(EigrpRecvHello);

	return;
}

/************************************************************************************

	Name:	EigrpDumpPacket

	Desc:	This function is to print the detail of the given packet.
		
	Para: 	packet	- pointer to the given packet
			size		- packet size
	
	Ret:		NONE
************************************************************************************/

void	EigrpDumpPacket(void *packet, U32 size)
{
	S32	id = 0;
	S32	left = 0;
	S8	buf[ 16 ];

	EIGRP_FUNC_ENTER(EigrpDumpPacket);
	if(!packet || !size){
		EIGRP_FUNC_LEAVE(EigrpDumpPacket);
		return;
	}
	EIGRP_TRC(DEBUG_EIGRP_OTHER, "START DUMP Packet(size =%d) :\n", size); 

	left = size;
	while(left > 0){
		if(left >= 16){
			EigrpPortMemCpy(buf, (S8 *)packet + id, 16);
			id += 16;
			left -= 16;
		}else{  /* <16 */
			EigrpUtilMemZero(buf, 16);
			EigrpPortMemCpy(buf, (S8 *)packet + id, left);
			left = 0;
		}
		EIGRP_TRC(DEBUG_EIGRP_OTHER, "%2X %2X %2X %2X %2X %2X %2X %2X | %2X %2X %2X "
													"%2X %2X %2X %2X %2X\n",
					buf[0], buf[1], buf[2], buf[3], buf[4], buf[ 5 ], buf[ 6 ], buf[ 7 ],
					buf[ 8 ], buf[ 9 ], buf[ 10 ], buf[ 11 ], buf[ 12 ], buf[ 13 ], buf[ 14 ], buf[ 15 ]); 
	}
	EIGRP_TRC(DEBUG_EIGRP_OTHER, "END DUMP Packet(size =%d) :\n", size); 
	EIGRP_FUNC_LEAVE(EigrpDumpPacket);

	return;
}

/************************************************************************************

	Name:	EigrpFlowControlTimerExpiry

	Desc:	This function is to process the expiration of an interface multicast flow control 
			timer.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			timer	- pointer to the multicast flow control timer
	
	Ret:		NONE
************************************************************************************/

void	EigrpFlowControlTimerExpiry(EigrpDualDdb_st *ddb, EigrpMgdTimer_st *timer)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpFlowControlTimerExpiry);
	iidb = EigrpUtilMgdTimerContext(timer);

	/* Update the flow control state. */
	EigrpUpdateMcastFlowState(ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpFlowControlTimerExpiry);

	return;
}

/************************************************************************************

	Name:	EigrpTakeNbrsDown

	Desc:	This function is to take all neighbors down for a given interface. This is called when
			filtering, summarization, or split-horizon is configured on an interface.
 
 			This routine can be safely called from other threads, since it schedules the peers to
 			be taken down;  the process does the real work.
 
 			If "destroy_now" is TRUE, we do it all immediately.  This should only be used when the
 			process is being destroyed!
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the given interface
			destory_now	- the sign by which we judge whether the peer need to be deleted 
			reason		- pointer to the string which indicate the event reason
	
	Ret:		NONE
************************************************************************************/

void	EigrpTakeNbrsDown_int(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf, S32 destroy_now, S8 *reason, S8 *file, int line)
{
	EigrpDualPeer_st	*peer, *next;
	EigrpIdb_st	*iidb;

	EIGRP_FUNC_ENTER(EigrpTakeNbrsDown);
	iidb = EigrpFindIidb(ddb, pEigrpIntf);
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpTakeNbrsDown);
		return;
	}
	for(peer = ddb->peerLst; peer; peer = next){
		next = peer->nextPeer;
		if(peer->iidb->idb != pEigrpIntf){
			continue;
		}
		EigrpTakePeerDown(ddb, peer, destroy_now, reason);
	}
	EIGRP_FUNC_LEAVE(EigrpTakeNbrsDown);

	return;
}

/************************************************************************************

	Name:	EigrpDumpPakdesc

	Desc:	This function is to print the packet description of the given packet.
		
	Para: 	pkeDesc		- pointer to the packet descriptor of the given packet 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDumpPakdesc(EigrpPackDesc_st *pktDesc)
{
	EIGRP_FUNC_ENTER(EigrpDumpPakdesc);
	EIGRP_TRC(DEBUG_EIGRP_OTHER, "pktDesc->peer =%X," 	
			"->pktDescAnchor =%X, "
			"->serNoStart=%d, "
			"->serNoEnd=%d, "
			"->preGenPkt=%X,->length=%d, "
			"->refCnt=%d, "
			"->ackSeqNum=%d, "
			"->flagSetInit=%d, "
			"pktDesc->flagSeq=%d, "
			"->flagPktDescMcast=%d, "
			"->flagAckPkt=%d, "
			"->opcode =%s\n",
			(U32)pktDesc->peer,
			(U32)pktDesc->pktDescAnchor,
			pktDesc->serNoStart,
			pktDesc->serNoEnd,
			(U32)pktDesc->preGenPkt,
			pktDesc->length,
			pktDesc->refCnt,
			pktDesc->ackSeqNum,
			pktDesc->flagSetInit,
			pktDesc->flagSeq,
			pktDesc->flagPktDescMcast,
			pktDesc->flagAckPkt,
			EigrpOpercodeItoa((S32)pktDesc->opcode)); 
	EIGRP_FUNC_LEAVE(EigrpDumpPakdesc);

	return;
}

/************************************************************************************

	Name:	EigrpGeneratePacket

	Desc:	This function is to call back to the protocol-specific module to generate a packet 
			based on the current descriptor, or uses the pregenerated packet.  Returns a pointer
			to the packet, with the fixed header initialized.  Sets packet_suppressed TRUE if a
			packet should have been generated but all data was overtaken by events(so the transport
			can fake an ACK).  Note that we may return NULL even with packet_suppressed FALSE;
			this indicates that we ran out of memory, so the transport treats it as if the packet
			was lost.	If "peer" is NULL, the packet is a multicast.

	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- the interface on which the packet will be sent
			peer		- pointer to the peer which the packet will be sent to ,if "peer" is NULL, 
			the packet is a multicast
			pktDesc	- pointer to the packet descriptor
	
	Ret:		pointer to the new packet header, or NULL if failed 
************************************************************************************/

EigrpPktHdr_st *EigrpGeneratePacket(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpDualPeer_st *peer, EigrpPackDesc_st *pktDesc, S32 *packet_suppressed)
{
	EigrpPktHdr_st *eigrp;

	EIGRP_FUNC_ENTER(EigrpGeneratePacket);
	*packet_suppressed = FALSE;
	/* Punt if we are going down. */
	if(peer && peer->flagGoingDown){
		EIGRP_FUNC_LEAVE(EigrpGeneratePacket);
		return NULL;
	}

	eigrp = pktDesc->preGenPkt;
	if(!eigrp){

		/* If this is an ACK, build it. */
		if(pktDesc->flagAckPkt){
			eigrp = EigrpPortMemMalloc(EIGRP_HEADER_BYTES);
			if(!eigrp){	 
				EIGRP_FUNC_LEAVE(EigrpGeneratePacket);
				return NULL;
			}
			EigrpUtilMemZero((void *) eigrp, EIGRP_HEADER_BYTES); 
			eigrp->ack = HTONL(pktDesc->ackSeqNum);
			pktDesc->ackSeqNum = 0;	/* Don't send a nonzero SEQ! */

			/* there is a hint that pktDesc->length is 0.(lijain) */
		}else{

			/* Call back to generate the packet. */
			eigrp = EigrpDualBuildPacket(ddb, iidb, peer, pktDesc, packet_suppressed);
		}
	}

	if(!eigrp){
		EIGRP_FUNC_LEAVE(EigrpGeneratePacket);
		return NULL;
	}

	/* Set the ACK field(for non-ACKs;  we've already done ACKs above.) For point-to-point hellos,
	  * don't fill in the ACK field;  this turns them into ACKs. */
	if(!pktDesc->flagAckPkt){
		if(peer && pktDesc->opcode != EIGRP_OPC_HELLO){
			eigrp->ack = HTONL(peer->lastSeqNo);
		}else{
			eigrp->ack = 0;
		}
	}

	/* Set INIT and sequence number(0 if unreliable packet). */
	if(pktDesc->flagSetInit){
		eigrp->flags	= HTONL(EIGRP_INIT_FLAG);
	}else{
		eigrp->flags	= 0;
	}
	eigrp->sequence	= HTONL(pktDesc->ackSeqNum);

	eigrp->version	= EIGRP_VERSION;
	eigrp->asystem	= HTONL(ddb->asystem);
	eigrp->opcode	= pktDesc->opcode;
	eigrp->checksum	= 0;
	EIGRP_FUNC_LEAVE(EigrpGeneratePacket);
	
	return(eigrp);
}

/************************************************************************************

	Name:	EigrpBuildSequenceTlv

	Desc:	This function is to insert address of peers that should not receive the next multicast
			packet to be transmitted.

			Adds the address to an existing structure, or creates one if it isn't there yet.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the given peer
			tlv		- pointer to the new TLV ,if it is NULL, alloc the memory for it
	
	Ret:		pointer to the new TLV
************************************************************************************/

EigrpGenTlv_st *EigrpBuildSequenceTlv (EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpGenTlv_st *tlv)
{
	S8	*entry_ptr;
	U16	len;
	S32	entry_length;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpBuildSequenceTlv);
	/* Malloc TLV block if not already malloced. */
	if(!tlv){
		tlv = EigrpPortMemMalloc(EIGRP_MAX_SEQ_TLV);
		if(!tlv){
			EIGRP_FUNC_LEAVE(EigrpBuildSequenceTlv);
			return NULL;
		}
		EigrpUtilMemZero((void *) tlv, EIGRP_MAX_SEQ_TLV);

		tlv->type = HTONS(EIGRP_SEQUENCE);

		/* tlv->length = sizeof(EigrpGenTlv_st); */
		tlv->length = HTONS(sizeof(EigrpGenTlv_st));
	}

	/* len = tlv->length; */
	len = NTOHS(tlv->length);
	entry_ptr = ((S8 *) tlv) + len;

	entry_length = (*ddb->addrCopy)(&peer->source, entry_ptr);
	len += entry_length;

	/* If the next entry won't fit, bail now so we don't overrun it. */
	if((EIGRP_MAX_SEQ_TLV - len) < entry_length){
		EigrpPortMemFree(tlv);
		tlv = (EigrpGenTlv_st*) 0;
		EIGRP_FUNC_LEAVE(EigrpBuildSequenceTlv);
		return NULL;
	}

	tlv->length = HTONS(len);
	EIGRP_FUNC_LEAVE(EigrpBuildSequenceTlv);
	
	return(tlv);
}

/************************************************************************************

	Name:	EigrpBuildPakdesc

	Desc:	This function is to bind a packet descriptor to a queue element.  Manipulates the
			reference count in the packet descriptor.
		
	Para: 	pktDesc		- pointer to the packet descriptor
			qelm		- pointer to the given queue element
	
	Ret:		NONE
************************************************************************************/

void	EigrpBuildPakdesc(EigrpPackDesc_st *pktDesc, EigrpPktDescQelm_st *qelm)
{
	EIGRP_FUNC_ENTER(EigrpBuildPakdesc);
	qelm->pktDesc = pktDesc;
	pktDesc->refCnt++;
	EIGRP_FUNC_LEAVE(EigrpBuildPakdesc);

	return;
}

/************************************************************************************

	Name:	EigrpEnqueuePeerQelms

	Desc:	This function is to enqueue unicast peer queue elements for this multicast packet.

			Returns a pointer to a sequence TLV if the multicast is reliable and some of the 
			recipients are not flow-control ready.
 
 			Returns TRUE if things generally went SUCCESS, FALSE if we couldn't generate	the sequence
 			TLV but needed to(so that the caller doesn't try to send the multicast packet).
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- the interface on which the packet will be sent 
			pktDesc	- pointer to the descriptor of the packet to be sent 
			seq_tlv	- pointer to the sequence TLV address
	
	Ret:		TRUE	for success
			FALSE	for fail
************************************************************************************/

S32	EigrpEnqueuePeerQelms(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc, EigrpGenTlv_st **seq_tlv)
{
	EigrpDualPeer_st			*peer;
	EigrpPktDescQelm_st	*qelm;
	U32					transmit_time;
	S32					flagSeq;
	U32		qtype;
	printf("EigrpEnqueuePeerQelms(%s, %s) Enter\n",
					iidb->idb->name,
					EigrpOpercodeItoa(pktDesc->opcode));

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpEnqueuePeerQelms);
	flagSeq = pktDesc->flagSeq;

	/* Skip the whole thing if unreliable and multicast. */
	if(!flagSeq && (iidb->useMcast == TRUE)){
		EIGRP_FUNC_LEAVE(EigrpEnqueuePeerQelms);
		return TRUE;
	}

	EIGRP_GET_TIME_STAMP(transmit_time);	/* Send the first one now? */

	/* Loop over each peer on the interface. */
	peer = ddb->peerLst;
	while(peer){
		/* Send on this peer if it is on the interface and isn't half-up. */
		if(peer->iidb == iidb && !peer->flagComingUp){

			/* Allocate a queue element. */
			qelm = (EigrpPktDescQelm_st *) EigrpUtilChunkMalloc(gpEigrp->pktDescQelmChunks);
			if(!qelm){		/* No memory */
				EIGRP_FUNC_LEAVE(EigrpEnqueuePeerQelms);
				return FALSE;		/* Pretty hopeless, actually */
			}
			EigrpBuildPakdesc(pktDesc, qelm);

			/* If this is going to be multicast and the peer queue contains some sequenced packets
			  * (not flow control ready), set up a sequence TLV. Note that we will only get here
			  * for sequenced packets, since the check at the top would have bailed out otherwise. */
			if(iidb->useMcast == TRUE){
				if(peer->xmitQue[EIGRP_RELIABLE_QUEUE]->count){
					*seq_tlv = EigrpBuildSequenceTlv(ddb, peer, *seq_tlv);
					iidb->mcastExceptionSent++;
					if(!*seq_tlv){
						EigrpPortMemFree(qelm);
						EIGRP_FUNC_LEAVE(EigrpEnqueuePeerQelms);
						return FALSE;
					}
					EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
				}else{
					/* Going out as a multicast now, which means that the unicast copy is going to 
					  * be a retransmission.  Note that we're sending it now(for RTT calculation)
					  * and note the retransmit time. */
					EIGRP_GET_TIME_STAMP(qelm->sentTime);
					EIGRP_TIMER_START(qelm->xmitTime, peer->rto / EIGRP_MSEC_PER_SEC);
					EIGRP_GET_TIME_STAMP(peer->pktFirstSndTime);
				}
			}else{
				/* The packet will be unicast.  Set the transmit time to be one pacing time after
				  * the last peer's, since we can't send them all at the same time anyhow.  We use
				  * the base interval, since we don't know how big the packet is actually going to
				  * be. */
				qelm->xmitTime = transmit_time;

				EIGRP_TIMER_UPDATEM(transmit_time, iidb->sndInterval[ EIGRP_RELIABLE_QUEUE ] / EIGRP_MSEC_PER_SEC);

				EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
			}

			qtype = flagSeq ? EIGRP_RELIABLE_QUEUE : EIGRP_UNRELIABLE_QUEUE;
			EigrpUtilQue2Enqueue(peer->xmitQue[ qtype ], (EigrpQueElem_st *)qelm);

			/* Kick the peer timer appropriately. */
			EigrpStartPeerTimer(ddb, iidb, peer);
		}
		peer = peer->nextPeer;
	}
	EIGRP_FUNC_LEAVE(EigrpEnqueuePeerQelms);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpGetSwVersion

	Desc:	This function is to store the software version in the specified location.

			This consists of two bytes of VOS version, and two bytes of EIGRP revision number. 
			This gives a hook for changing the system behavior.
		
	Para: 	verptr	- pointer to the structure of softeware version
	
	Ret:		NONE
************************************************************************************/

void	EigrpGetSwVersion(EigrpSwVer_st *verptr)
{
	EIGRP_FUNC_ENTER(EigrpGetSwVersion);
	verptr->majVer		= EIGRP_VER_SYS_MAJ;
	verptr->minVer		= EIGRP_VER_SYS_MIN;
	verptr->eigrpMajVer	= EIGRP_VER_SOFT_MAJ;
	verptr->eigrpMinVer	= EIGRP_VER_SOFT_MIN;
	EIGRP_FUNC_LEAVE(EigrpGetSwVersion);

	return;
}

/************************************************************************************

	Name:	EigrpBuildHello

	Desc:	This function is to build a hello packet, given a buffer to put it in.
 
 			Returns a pointer to the end of the packet(for adding TLVs).
		
	Para: 	iidb		- pointer to the interface on which the hello packet will be sent
			eigrp 	- pointer to the hello packet header
			K1,K2,K3,K4,K5	- the K value
			hold		- the holdtime of peer
	
	Ret:		pointer to the end of the new packet
************************************************************************************/

S8	*EigrpBuildHello(EigrpIdb_st *iidb, EigrpPktHdr_st *eigrp, U32 k1, U32 k2, U32 k3, U32 k4, U32 k5, U16 hold)
{
	EigrpAuthTlv_st	*auth_tlv;
	EigrpParamTlv_st	*param;
#if(EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_COMMERCIAL)
	EigrpVerTlv_st		*vers_tlv;
#endif

	EIGRP_FUNC_ENTER(EigrpBuildHello);
	eigrp->opcode = EIGRP_OPC_HELLO;

	if(iidb->authSet && (iidb->authMode == TRUE)){
		auth_tlv	= (EigrpAuthTlv_st *)EigrpPacketData(eigrp);
		param	= (EigrpParamTlv_st*)(auth_tlv + 1);
	}else{
		param	= (EigrpParamTlv_st *)EigrpPacketData(eigrp);
	}

	param->type		= HTONS(EIGRP_PARA);
	param->length	= HTONS(sizeof(EigrpParamTlv_st));
	param->k1		= k1;
	param->k2		= k2;
	param->k3		= k3;
	param->k4		= k4;
	param->k5		= k5;
	param->pad		= 0;
	param->holdTime	= HTONS(hold);

#if(EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_COMMERCIAL)
	{
		/* Write out a software version TLV so that systems can tell what version  of software their
		  * neighbors are running.  This to dig us out of nasty backwards compatibility problems... */
		vers_tlv			= (EigrpVerTlv_st *)(((S8 *)param) + sizeof(EigrpParamTlv_st));
		vers_tlv->type	= HTONS(EIGRP_SW_VERSION);
		vers_tlv->length	= HTONS(sizeof(EigrpVerTlv_st));
		EigrpGetSwVersion(&vers_tlv->version);
		
		EIGRP_FUNC_LEAVE(EigrpBuildHello);
		return(((S8 *) vers_tlv) + sizeof(EigrpVerTlv_st));
	}
#elif ((EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_YEZONG) || (EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_DIYUWANG))
	{
		EIGRP_FUNC_LEAVE(EigrpBuildHello);
		return((S8 *)param);
	}
#endif
}

/************************************************************************************

	Name:	EigrpUpdateMib

	Desc:	This function is to update MIB counters based on opcode.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			eigrp	- pointer to the eigrp packet header
	
	Ret:		NONE
************************************************************************************/

void	EigrpUpdateMib(EigrpDualDdb_st *ddb, EigrpPktHdr_st *eigrp, S32 send_flag)
{
	U32 *ptr = NULL;

	EIGRP_FUNC_ENTER(EigrpUpdateMib);
	switch(eigrp->opcode){
		case EIGRP_OPC_HELLO:
			ptr = (NTOHL(eigrp->ack)) ? &ddb->mib.ackSent : &ddb->mib.helloSent;
			break;
			
		case EIGRP_OPC_QUERY:
			ptr = &ddb->mib.queriesSent;
			break;
			
		case EIGRP_OPC_REPLY:
			ptr = &ddb->mib.repliesSent;
			break;
			
		case EIGRP_OPC_UPDATE:
			ptr = &ddb->mib.updateSent;
			break;
			
		default:
			break;
	}

	if(ptr){
		if(!send_flag){
			ptr++;
		}
		(*ptr) += 1;
	}
	EIGRP_FUNC_LEAVE(EigrpUpdateMib);

	return;
}

/************************************************************************************

	Name:	EigrpHeadProc

	Desc:	This function is to process received EIGRP packet. This function processes the fixed
			part of the EIGRP header. This function is network protocol independent.
 					
	Para: 	ddb		- pointer to the dual descriptor block 
			eigrph	- pointer to the eigrp packet header
			source	- pointer to the ip address of the peer who sent the packet
			pEigrpIntf	- the interface on which the packet is received
	
	Ret:		pointer to the peer structure, or NULL if the packet should be ignored.
************************************************************************************/
#ifdef INCLUDE_SATELLITE_RESTRICT
EigrpDualPeer_st	*lastSendPeer = NULL;
U32	blacklist[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
EigrpDualPeer_st *EigrpHeadProc(EigrpDualDdb_st *ddb, EigrpPktHdr_st *eigrph, U32 size, U32 *source, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualPeer_st	*peer;
	EigrpIdb_st		*iidb;
	U32	slot, seq, ack_seq, opcode, flags;
	S32	auth_res = TRUE;
#ifdef INCLUDE_SATELLITE_RESTRICT
	U32	pktSeq, pktAck, n;

	for(n = 0; n < 10; n++){
		if(blacklist[n] == *source){
			return	NULL;
		}
	}
	if(/*eigrph->ack && */eigrph->sequence && (*(U8 *)&eigrph->ack == 0x80)){
		pktSeq = NTOHL(eigrph->sequence);
		*(U8 *)&eigrph->ack = 0x00;
		pktAck = NTOHL(eigrph->ack);
		eigrph->sequence = 0;
		eigrph->ack = 0;

		/* TODO:MD5~{5D7=7(C;8cCw0W#,T]J1SC~}IP~{5XV7<r5%9\?X!#2;=SJ\:ZC{5%VP5XV77"@45DHN:N1(ND~}*/
		if(pktSeq != pEigrpIntf->ipAddr){
			S32	i;
			for(i = 0; i < 10; i ++){
				if(!blacklist[i]){
					blacklist[i] = *source;
					break;
				}
			}

			return	NULL;
		}
	}else{
		pktSeq = 0;
		pktAck = 0;
	}
#endif
	EIGRP_FUNC_ENTER(EigrpHeadProc);
	if(!EigrpPakSanity(ddb, eigrph, size, source, pEigrpIntf)){
		EIGRP_FUNC_LEAVE(EigrpHeadProc);
		return NULL;
	}

	/* Don't accept hello if this ddb is not configured on interface. */
	iidb = EigrpFindIidb(ddb, pEigrpIntf);
	if(!iidb){
		EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP: ddb not configured on %s\n", pEigrpIntf->name);
		EIGRP_FUNC_LEAVE(EigrpHeadProc);
		return NULL;
	}

	/* Ignore the packet if we're passive. */
	if(iidb->passive == TRUE){
		EIGRP_FUNC_LEAVE(EigrpHeadProc);
		return NULL;
	}

	seq		= NTOHL(eigrph->sequence);
	ack_seq	= NTOHL(eigrph->ack);
	flags		= NTOHL(eigrph->flags);
	opcode	= eigrph->opcode;
	if(opcode == EIGRP_OPC_HELLO && ack_seq){
		opcode = EIGRP_OPC_ACK;
	}

	if(opcode != EIGRP_OPC_ACK){
		/* Verify MD5 authentication. */
		if(EigrpUtilVerifyMd5(eigrph, iidb, size - EIGRP_HEADER_BYTES)){
			EIGRP_TRC(DEBUG_EIGRP_EVENT,
						"EIGRP-EVENT: ignored packet from %s opcode = %s(invalid authentication)\n",
						(*ddb->printAddress)(source), EigrpOpercodeItoa(opcode));
			auth_res = FALSE;
		}
	}

	peer = EigrpFindPeer(ddb, source, pEigrpIntf); /* May return NULL */

	if(!auth_res){
		if(peer){
			peer->flagGoingDown = TRUE;
		}
		EIGRP_FUNC_LEAVE(EigrpHeadProc);
		return NULL;
	}

	/* Print debug, if enabled. */
	EigrpUpdateMib(ddb, eigrph, FALSE);

	if(peer){
		if(peer->flagGoingDown){
			EIGRP_FUNC_LEAVE(EigrpHeadProc);
			return NULL;
		}

		/* If a packet was received with the INIT flag set, the peer wants a full topology table
		  * downloaded. The peer has gone down and come back up within the holdTime. We think the
		  * peer never went down. We need to reset the received sequence number for this peer. */
		if(flags & EIGRP_INIT_FLAG){
			if(!peer->flagNeedInit){
				/* Perhaps this is simply a retransmission of the initial update packet?  If so,
				  * don't tear down the neighbor. */
				if(seq != peer->lastSeqNo){
					EigrpNeighborDown(ddb, peer);
					EIGRP_FUNC_LEAVE(EigrpHeadProc);
					return NULL;
				}
			}else{
				peer->flagNeedInit = FALSE;
			}
		}else{
			/* Packet has INIT bit clear.  If we're in INIT state and the packet is sequenced, and
			  * it is one that we would otherwise be expected to accept, go kick things to try to 
			  * get the peer in sync with us.  If we wouldn't otherwise accept the packet, simply
			  * drop it. */
			if(peer->flagNeedInit && seq){
				if(EigrpShouldAcceptSeqPacket(ddb, peer, eigrph, seq) == EIGRP_PKT_ACCEPT){
					EigrpReinitPeer(ddb, peer);
				}
				EIGRP_FUNC_LEAVE(EigrpHeadProc);
				return NULL;
			}
		}

		/* If the packet is carrying an acknowledgement, process it. */
		if(ack_seq){
			EigrpProcessAck(ddb, peer, ack_seq);
		}

		/* If the packet is sequenced, see if it's acceptable.  If not, drop it. */
		if(seq){
			if(!EigrpAcceptSequencedPacket(ddb, peer, eigrph)){
				EIGRP_FUNC_LEAVE(EigrpHeadProc);
				return NULL;
			}
			EIGRP_TIMER_STOP(peer->reInitStart);
		}
	}else{				/* No peer */
		/*If the packet is from ourselves, drop it. */
		if(EigrpIpLocalAddr(ddb, source, pEigrpIntf)){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP: Packet from ourselves ignored\n");
			EIGRP_FUNC_LEAVE(EigrpHeadProc);
			return NULL;
		}

		/* No peer.  If the packet isn't a hello, drop it. */
		if(opcode != EIGRP_OPC_HELLO){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP: Neighbor not yet found, packet dropped\n");
			EIGRP_FUNC_LEAVE(EigrpHeadProc);
			return NULL;
		}

		/* New peer.  If we shouldn't accept it, ignore the hello. */
		if(!EigrpIpShouldAcceptPeer(ddb, source, iidb)){
			EIGRP_FUNC_LEAVE(EigrpHeadProc);
			return NULL;
		}

		/* New peer.  Create an entry for it, with all the trimmings. */
		peer = EigrpPortMemMalloc(sizeof(EigrpDualPeer_st));
		if(!peer){
			EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Create PEER %s failed\n", 
						(*ddb->printAddress)(source));
			EIGRP_FUNC_LEAVE(EigrpHeadProc);
			return NULL;
		}else{
			EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Create PEER %s SUCCESS\n", 
						(*ddb->printAddress)(source));
		}
		EigrpUtilMemZero((void *)peer, sizeof(EigrpDualPeer_st));

		peer->xmitQue[ EIGRP_UNRELIABLE_QUEUE ]	= EigrpUtilQue2Init();
		peer->xmitQue[ EIGRP_RELIABLE_QUEUE ]		= EigrpUtilQue2Init();
		peer->iidb = iidb;
		EigrpAllocHandle(ddb, peer);
		EigrpPortMemCpy((void *) &peer->source, (void *)source, sizeof(U32));
		peer->routerId		= ddb->routerId;
		peer->srtt			= 0;			/* totalSrtt depends on this! */
		peer->rto			= EIGRP_INIT_RTO;
		peer->flagNeedInit	= TRUE;
		peer->flagNeedInitAck	= TRUE;
		peer->flagComingUp	= TRUE;
		peer->protoData		= NULL;
		EIGRP_GET_TIME_STAMP(peer->uptime);

		slot = (*ddb->peerBucket)(source);
		peer->next = ddb->peerCache[ slot ];
		ddb->peerCache[ slot ]	= peer;

		peer->nextPeer	= ddb->peerLst;
		ddb->peerLst		= peer;

		EigrpUtilMgdTimerInitLeaf(&peer->peerSndTimer, &iidb->peerTimer,
								EIGRP_PEER_SEND_TIMER, peer, FALSE);
		EigrpUtilMgdTimerInitLeaf(&peer->holdingTimer, &iidb->holdingTimer,
								EIGRP_PEER_HOLDING_TIMER, peer, FALSE);
		if(EigrpDualPeerCount(iidb) == 1){	/* First one on this i/f */
			EigrpDualFirstPeerAdded(ddb, iidb);
		}

		if(ddb->peerStateHook != NULL){
			(*ddb->peerStateHook)(ddb, peer, TRUE);
		}
	}
#ifdef INCLUDE_SATELLITE_RESTRICT
	if(opcode == EIGRP_OPC_HELLO){
		U32	sec, msec;
		S32	retVal, timeTemp;

		retVal = EigrpPortGetTime(&sec, &msec, NULL);
		if(retVal == SUCCESS){
			if(!peer->firstRecvedHello.sec){
				peer->firstRecvedHello.sec = sec;
				peer->firstRecvedHello.msec = msec;
/*				peer->firstRecvedHello.ipAddr = source;*//*t2 hello id*/
			}
			if(pktSeq == pEigrpIntf->ipAddr && pktAck){
				timeTemp = (sec * 1000 - pEigrpIntf->lastSentHello.sec * 1000) + (msec - pEigrpIntf->lastSentHello.msec) - pktAck;
				peer->rtt = timeTemp;
				printf("RTT to 0x%x: %d\n", *source, timeTemp);
#if 0
				if(timeTemp >= EIGRP_DEF_SATELLITE_RTT){
					// TODO:~{P^8D~}delay~{V5~}
					pEigrpIntf->delay = 0xabcdef;//EIGRP_METRIC_INACCESS;
				}
#endif
			}
		}
	}
#endif

	/* Check for Sequence TLV. Only present in Hello packets. */
	if(eigrph->opcode == EIGRP_OPC_HELLO){
		EigrpRecvHello(ddb, peer, eigrph, (S32)size);
	}

	/* Acknowledge sequenced packets.  The ACK may later be suppressed. */
	if(seq){
		EigrpSendAck(ddb, peer, peer->lastSeqNo);
	}
	EIGRP_FUNC_LEAVE(EigrpHeadProc);

	return(peer);
}

/************************************************************************************

	Name:	EigrpGenerateMd5

	Desc:	This function is to build the md5 digest of an outgoing EIGRP packet.
  
 			This function must be called before EigrpGenerateChksum.
		
	Para: 	eigrp	- pointer to the eigrp packet header
			iidb		- pointer to the interface on which the packet will be sent
			paklen	- the packet size
	
	Ret:		NONE
************************************************************************************/

void	EigrpGenerateMd5 (EigrpPktHdr_st *eigrp, EigrpIdb_st *iidb, U32 paklen)
{
	EigrpAuthTlv_st *auth_tlv;
	U8 digest[ EIGRP_AUTH_LEN ];

	EIGRP_FUNC_ENTER(EigrpGenerateMd5);
	auth_tlv			= (EigrpAuthTlv_st*)(eigrp + 1);
	auth_tlv->type	= HTONS(EIGRP_AUTH );
	auth_tlv->length	= HTONS(sizeof(EigrpAuthTlv_st));
	auth_tlv->authType	= HTONS(EIGRP_AUTH_TYPE);
	auth_tlv->authLen	= HTONS(EIGRP_AUTH_LEN);

	auth_tlv->keyId	= HTONL(iidb->authInfo.keyId);

	auth_tlv->pad[0]	= 0;
	auth_tlv->pad[1]	= 0;
	auth_tlv->pad[2]	= 0;

	EigrpPortMemCpy((U8 *)auth_tlv->digest, iidb->authInfo.authData, 16);

	EigrpUtilGenMd5Digest((U8 *)eigrp, paklen, digest);
	EigrpPortMemCpy((void *)auth_tlv->digest, (void *)digest, EIGRP_AUTH_LEN);
	EIGRP_FUNC_LEAVE(EigrpGenerateMd5);

	return;
}

/************************************************************************************

	Name:	EigrpGenerateChksum

	Desc:	This function is to build the checksum of an outgoing EIGRP packet.
 
			The length does not include the EIGRP header.
		
	Para: 	eigrp	- pointer to the eigrp packet header
			length	- length of the packet
	
	Ret:		NONE
************************************************************************************/

void	EigrpGenerateChksum(EigrpPktHdr_st *eigrp, U32 length)
{
	U16 checksum;

	EIGRP_FUNC_ENTER(EigrpGenerateChksum);
	eigrp->checksum	= 0;
	checksum		= EigrpUtilGetChkSum((U8 *)eigrp, length + EIGRP_HEADER_BYTES);
	eigrp->checksum	= checksum;
	EIGRP_FUNC_LEAVE(EigrpGenerateChksum);

	return;
}

/************************************************************************************

	Name:	EigrpInduceError

	Desc:	
		
	Para: 
	
	Ret:		
************************************************************************************/

S32	EigrpInduceError(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, S8 *string)
{
	return FALSE;
}

/************************************************************************************

	Name:	EigrpSendHelloPacket

	Desc:	This function is to send a hello packet.  This should be used sparingly, since it 
			bypasses the circuit pacing mechanism by calling the protocol send routine directly.

			This routine does not free the packet.
		
	Para: 	ddb		- pointer to the dual descriptor block
			eigrp	- pointer to the eigrp packet header
			iidb		- pointer to the interface on which the hello packet will be sent
			bytes	- length of the packet
			priority	- the priority of sending packet
	
	Ret:		NONE
************************************************************************************/

void	EigrpSendHelloPacket(EigrpDualDdb_st *ddb, EigrpPktHdr_st *eigrp, EigrpIdb_st *iidb, S32 bytes, S32 priority)
{
	EIGRP_FUNC_ENTER(EigrpSendHelloPacket);
	eigrp->version	= EIGRP_VERSION;
	eigrp->opcode	= EIGRP_OPC_HELLO;
	eigrp->asystem	= HTONL(ddb->asystem);
	eigrp->flags		= 0;
	eigrp->ack		= 0;
	eigrp->sequence	= 0;
	if(iidb->authSet && (iidb->authMode == TRUE)){
		EigrpGenerateMd5(eigrp, iidb, (U32) bytes);
	}

	EigrpGenerateChksum(eigrp, (U32) bytes);
#ifdef INCLUDE_SATELLITE_RESTRICT
	{
		U32	sec, msec, i;
		S32	retVal, firstEmptyElem;
		EigrpDualPeer_st	*peer;

		if(eigrp->opcode == EIGRP_OPC_HELLO && !eigrp->ack){
			retVal = EigrpPortGetTime(&sec, &msec, NULL);
			if(retVal == SUCCESS){
/*
				for(i = 0; i < 10; i++){
					if(!iidb->sentHello[i].sec){
						iidb->sentHello[i].sec = sec;
						iidb->sentHello[i].msec = msec;
						iidb->sentHello[i].id = sendHelloId;
						eigrp->sequence = sendHelloId << 16 && 0xFFFF0000;
						break;
					}
				}*/
#if 0
				if(lastSendPeer){
					peer = lastSendPeer->nextPeer;
				}else{
					peer = ddb->peerLst;
				}
				while(peer){
					if(peer->iidb == iidb){
						if(peer->firstRecvedHello.sec){
							eigrp->sequence = HTONL(peer->source);
							eigrp->ack = (sec * 1000 + msec) - (peer->firstRecvedHello.sec * 1000 + peer->firstRecvedHello.msec);/*t3 - t2*/
							eigrp->ack = HTONL(eigrp->ack);
							*(U8 *)&eigrp->ack = 0x80;/*~{N*AKSk~} ACK~{!"~}UPDATE~{1(NDGx7V?*~}*/
							lastSendPeer = peer;
							break;
						}
					}

					peer = peer->nextPeer;
				}
#endif
				peer = ddb->peerLst;
				while(peer){
					if(peer->iidb == iidb){
						firstEmptyElem = -1;
						for(i = 0; i < 64; i++){
							if(iidb->idb->sentPeer[i]){
								if(iidb->idb->sentPeer[i] == peer->source){
									break;
								}
							}else if(firstEmptyElem == -1){
								firstEmptyElem = i;
							}
						}
						if(i == 64){
							if(peer->firstRecvedHello.sec){
								eigrp->sequence = HTONL(peer->source);/*MD5~{5D7=7(C;8cCw0W#,T]J1OH4+5]5XV7~}*/
								eigrp->ack = (sec * 1000 + msec) - (peer->firstRecvedHello.sec * 1000 + peer->firstRecvedHello.msec);/*t3 - t2*/
								eigrp->ack = HTONL(eigrp->ack);
								*(U8 *)&eigrp->ack = 0x80;/*~{N*AKSk~} ~{U}3#5D~}ACK~{!"~}UPDATE~{1(NDGx7V?*~}*/
								iidb->idb->sentPeer[firstEmptyElem] = peer->source;
								break;
							}
						}
					}

					peer = peer->nextPeer;
				}
				iidb->idb->lastSentHello.sec = sec;
				iidb->idb->lastSentHello.msec = msec;
				if(peer){
					iidb->idb->lastSentHello.ipAddr = peer->source;
				}
			}
		}
	}
#endif
	if(!EigrpInduceError(ddb, iidb, "Transmit")){
		(*ddb->sndPkt)(eigrp, bytes + EIGRP_HEADER_BYTES, NULL, iidb, priority);
	}

	/* Do debug stuff. */
	EigrpDualDebugSendPacket(ddb, NULL, iidb, NULL, eigrp, 0);
	EigrpUpdateMib(ddb, eigrp, TRUE);
	EIGRP_FUNC_LEAVE(EigrpSendHelloPacket);

	return;
}

/************************************************************************************

	Name:	EigrpSendSeqHello

	Desc:	This function is to send a Hello with a Sequence TLV and a Next Multicast Sequence TLV
			inside.
			This is sent just before a multicast to limit the set of receiving peers.
			The multicast has the CR bit set to reference this sequence TLV hello.
			The sequence number of the multicast is added so that we don't have any ambiguity in 
			the face of lost packets.
		
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the  interface on which the packet will be sent
			pktDesc	- pointer to the packet descriptor
			tlv		- pointer to the TLV field
	
	Ret:		NONE
************************************************************************************/

void	EigrpSendSeqHello(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc, EigrpGenTlv_st *tlv)
{
	EigrpPktHdr_st	*eigrp;
	S32		bytes;
	void		*next_tlv;
	EigrpNextMultiSeq_st	*multiseq;

	EIGRP_FUNC_ENTER(EigrpSendSeqHello);
	/* Allocate a buffer for this pile. */
	/* we process the tlv->length as the net byte order value */
	/* bytes = EIGRP_HELLO_HDR_BYTES + tlv->length + sizeof(EigrpNextMultiSeq_st);*/
	bytes	= EIGRP_HELLO_HDR_BYTES + NTOHS(tlv->length) + sizeof(EigrpNextMultiSeq_st);
	if(iidb->authSet && (iidb->authMode == TRUE)){
		bytes += sizeof(EigrpAuthTlv_st);
	}
	eigrp	= EigrpAllocPktbuff(bytes + EIGRP_HEADER_BYTES);
	if(!eigrp){
		EIGRP_FUNC_LEAVE(EigrpSendSeqHello);
		return;
	}

	/* Build the base hello packet. */
	next_tlv	= EigrpBuildHello(iidb, eigrp, ddb->k1, ddb->k2, ddb->k3, ddb->k4, ddb->k5,
							(U16)(iidb->holdTime / EIGRP_MSEC_PER_SEC));

	/* Add the SEQUENCE TLV. */
	EigrpPortMemCpy((void *) next_tlv, (void *) tlv, (U32)(NTOHS(tlv->length)));

	next_tlv	= ((S8 *) next_tlv) + NTOHS(tlv->length);

	/* Add the multicast sequence number. */
	multiseq	= next_tlv;
	multiseq->type	= HTONS(EIGRP_NEXT_MCAST_SEQ);
	multiseq->length	= HTONS(sizeof(EigrpNextMultiSeq_st));
	multiseq->seqNum	= HTONL(pktDesc->ackSeqNum);

	/* Send it. */
	EigrpSendHelloPacket(ddb, eigrp, iidb, bytes, FALSE);
	EigrpPortMemFree(eigrp);
	eigrp = (EigrpPktHdr_st*) 0;
	EIGRP_FUNC_LEAVE(EigrpSendSeqHello);

	return;
}

/************************************************************************************

	Name:	EigrpPacingValue

	Desc:	This function returns the value of the pacing timer for an interface, based on whether
			the last packet sent was sequenced(reliable) or not, as well as the length of the 
			packet.
		
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the interface on which the packet will be sent
			pktDesc	- pointer the descriptor of the packet that will be sent
	
	Ret:		the packet packet sending interval
************************************************************************************/

U32	EigrpPacingValue(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc)
{
	U32	qtype;
	U32	interval;

	EIGRP_FUNC_ENTER(EigrpPacingValue);
	interval = EigrpCalculatePacingInterval(ddb, iidb, pktDesc->length,
											(S32)(pktDesc->flagSeq));
	qtype = pktDesc->flagSeq ? EIGRP_RELIABLE_QUEUE : EIGRP_UNRELIABLE_QUEUE;
	iidb->lastSndIntv[ qtype ] = interval; /* For display... */
	EIGRP_FUNC_LEAVE(EigrpPacingValue);
	
	return(interval);
}

/************************************************************************************

	Name:	EigrpFlowBlockInterface

	Desc:	This function is to mark an interface as flow-blocked for multicast.
		
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the given interface
			qelm	- 
	
	Ret:		NONE
************************************************************************************/

void	EigrpFlowBlockInterface(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPktDescQelm_st **qelm)
{
	/* Save a pointer to the packet, which makes the interface flow-blocked for multicast.  Whimper
	  * if there's already one there(there better not be). */
	EIGRP_FUNC_ENTER(EigrpFlowBlockInterface);
	if(iidb->lastMultiPkt){
		EIGRP_ASSERT(0);
		EigrpCleanupMultipak(ddb, iidb);
	}
	iidb->lastMultiPkt	= *qelm;
	if(*qelm){
		iidb->mcastStartRefcount = (*qelm) ->pktDesc->refCnt;
	}
	*qelm	= NULL;			/* Make sure it doesn't get freed. */

	/* Start the flow timer. */
	EigrpStartMcastFlowTimer(ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpFlowBlockInterface);

	return;
}

/************************************************************************************

	Name:	EigrpSendMulticast

	Desc:	This function is to send the supplied multicast packet.

			Assumes that the interface timer has been stopped.  It may restart it.
 
 			If the packet is reliable, or if this is an NBMA network(where we don't actually try 
 			to send multicasts), the packet will be queued appropriately on each peer's queue.
		
	Para: 	ddb		- pointer to the dual descriptor block
			iidb		- pointer to the interface on which the packet will be sent
	
	Ret:		
************************************************************************************/

void	EigrpSendMulticast(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPktDescQelm_st *qelm)
{
	EigrpPackDesc_st	*pktDesc;
	EigrpGenTlv_st	*seq_tlv;
	EigrpPktHdr_st	*eigrp;
	S32		flagSeq, tlv_success, packet_suppressed;
	U32		qtype, opcode, flags, seq, ack_seq;

	EIGRP_FUNC_ENTER(EigrpSendMulticast);
	/* Fetch the packet descriptor. */
	pktDesc			= qelm->pktDesc;
	flagSeq			= pktDesc->flagSeq;
	qtype			= flagSeq ? EIGRP_RELIABLE_QUEUE : EIGRP_UNRELIABLE_QUEUE;
	packet_suppressed	= FALSE;
	opcode			= pktDesc->opcode;	/* Make it 32 bits */
	EigrpDualLogXportAll(ddb, EIGRP_DUALEV_MCASTSENT, &iidb->idb, NULL);
	EigrpDualLogXportAll(ddb, EIGRP_DUALEV_PKTSENT2, &pktDesc->serNoStart, &pktDesc->serNoEnd);

	/* Generate the packet if we're actually sending a multicast. */
	if(iidb->useMcast == TRUE){
		eigrp = EigrpGeneratePacket(ddb, iidb, NULL, pktDesc, &packet_suppressed);
	}else{
		eigrp = NULL;
	}

	/* Go try to generate(re)transmission entries for this packet on each peer on the interface. 
	  * If this is an NBMA or P2P interface, we generate transmission entries regardless of whether
	  * the packet is sequenced or not(because we don't actually transmit multicasts on these
	  * interfaces).  Otherwise, we generate retransmission entries if the packet is sequenced.
	  *
	  * We may be given a SEQ TLV hello packet in return, to tell which  of our neighbors should
	  * not receive the multicast.  We will only get this back if multicasts are in use.
	  *
	  * Note that eigrp may be NULL if the packet was suppressed.  We must enqueue the peer qelms
	  * anyhow in order to ensure that packets are acknowledged back to DUAL in the order that they
	  * were generated. */
	seq_tlv	= NULL;
	tlv_success	= EigrpEnqueuePeerQelms(ddb, iidb, pktDesc, &seq_tlv);

	if(eigrp || (iidb->useMcast != TRUE)){ /* If there's something to send */
		if(eigrp){
			if(seq_tlv){
				/* Got a SEQ TLV.  Send it in a Hello, and set the CR flag. */
				EigrpSendSeqHello(ddb, iidb, pktDesc, seq_tlv);
				EigrpPortMemFree(seq_tlv);
				seq_tlv	= (EigrpGenTlv_st*) 0;
				eigrp->flags	= HTONL(NTOHL(eigrp->flags) | EIGRP_CR_FLAG);
				iidb->crPktSent++;
			}
			/* md5 authenticate the packet. */
			if(iidb->authSet && (iidb->authMode == TRUE)){
				EigrpGenerateMd5(eigrp, iidb, pktDesc->length);
			}
			/* Checksum the packet. */
			EigrpGenerateChksum(eigrp, pktDesc->length);
		}

		/* If not NBMA/P2P or unreliable, send the packet.
		  *
		  * If we ran out of TLV space, don't send the multicast(since otherwise it would fall into
		  * the wrong hands) and let the unicast retransmissions save our butt. */
		if(((iidb->useMcast == TRUE) && tlv_success) || !flagSeq){
			if(!eigrp){
				EIGRP_ASSERT(0);
				EIGRP_FUNC_LEAVE(EigrpSendMulticast);
				return;
			}
			if(!EigrpInduceError(ddb, iidb, "Transmit")){
				(*ddb->sndPkt)(eigrp, (S32)(pktDesc->length + EIGRP_HEADER_BYTES),
									NULL, iidb, FALSE);
			}
			iidb->mcastSent[ qtype ] ++;
			flags		= NTOHL(eigrp->flags);
			seq		= NTOHL(eigrp->sequence);
			ack_seq	= NTOHL(eigrp->ack);
			EigrpDualLogXportAll(ddb, EIGRP_DUALEV_PKTRCV2, &opcode, &flags);
			EigrpDualLogXportAll(ddb, EIGRP_DUALEV_PKTRCV3, &seq, &ack_seq);

			/* * Print debug, if enabled. */
			EigrpDualDebugSendPacket(ddb, NULL, iidb, pktDesc, eigrp, 0);
			EigrpUpdateMib(ddb, eigrp, TRUE);

			/* Kick the interface pacing timer. */
			EigrpKickPacingTimer(ddb, iidb, EigrpPacingValue(ddb, iidb, pktDesc));
			EigrpDualLogXportAll(ddb, EIGRP_DUALEV_XMITTIME, &pktDesc->length,
								&iidb->lastSndIntv[ qtype ]);
		}

		/* If sequenced, mark the interface as flow-control blocked, and save the sequence number. */
		if(flagSeq){
			EigrpFlowBlockInterface(ddb, iidb, &qelm);
			iidb->lastMcastSeq = pktDesc->ackSeqNum;
		}

		/* If the packet was pregenerated, clear the CR bit now.  Otherwise, free the packet, since
		  * it will be regenerated later if necessary. This is safe to do, since ddb->sndPkt copies
		  * the packet we give it. */
		if(eigrp){
			if(pktDesc->preGenPkt){
				eigrp->flags = HTONL(NTOHL(eigrp->flags) & (~EIGRP_CR_FLAG));
			}else{
				EigrpPortMemFree(eigrp);
				eigrp = (EigrpPktHdr_st*) 0;
			}
		}
	}else{				/* No packet to send */
		/* No packet to send.  This is usually because the contents of the packet have been 
		  * overtaken by events(or suppressed for some other reason).  We kick the pacing timer to
		  * expire immediately.
		  *
		  * If this is due to having insufficient memory, we do the same thing. */
		if(packet_suppressed){
			EigrpDualLogXportAll(ddb, EIGRP_DUALEV_PKTSUPPR, &opcode, NULL);
		}
		if(seq_tlv){
			EigrpPortMemFree(seq_tlv);			/* Free it if it's there */
			seq_tlv = (EigrpGenTlv_st*) 0;
		}

		EigrpKickPacingTimer(ddb, iidb, 0);
	}

	iidb->unicastLeftToSnd = EIGRP_MAX_UNICAST_PKT; /* We can send lots */

	/* Free the queue element. */
	if(qelm){
		EigrpFreeQelm(ddb, qelm, iidb, NULL);
	}
	EIGRP_FUNC_LEAVE(EigrpSendMulticast);

	return;
}

/************************************************************************************

	Name:	

	Desc:	This function is to process the expiration of an interface pacing timer.  This means 
			that	the pacing interval has expired and it's now safe to send another packet.
 
 			The timer will be stopped or restarted as necessary.
 
 			We decide whether the next packet sent should be a multicast or a unicast.  Given 
 			enough traffic, we will guarantee each of them some traffic so as to not starve out
 			either type.  If we don't have any of one type, we'll send the other type instead.
 
 			The IIDB reliable transmit queue contains all reliable multicasts and	initial 
 			unicasts.  When a multicast is pulled off the queue, it is immediately transmitted.
 			When a unicast is pulled off the queue, it is requeued on the peer queue.  This ensures
 			that packets stay in  the order in which they were delivered to the transport.

			If it's time for a multicast, we send it now.  If not, we stop the pacing timer in
			expectation that a peer timer will expire to send	a unicast.
			
			Unreliable multicasts are always sent ahead of reliable multicasts. At this point, only
			outgoing Hello packets fit this description. There is no mechanism to keep unreliable
			packets from hogging all the bandwidth;  this should be added if there is ever a
			chance of scads of unreliable multicasts being sent.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			timer	- pointer to the timer which is overtime
	
	Ret:		NONE
************************************************************************************/


void	EigrpIntfPacingTimerExpiry(EigrpDualDdb_st *ddb,EigrpMgdTimer_st *timer)
{
	EigrpIdb_st			*iidb;
	EigrpPktDescQelm_st	*qelm;
	EigrpDualPeer_st		*peer;
	S32					requeued_unicast;

	EIGRP_FUNC_ENTER(EigrpIntfPacingTimerExpiry);
	iidb = EigrpUtilMgdTimerContext(timer);
	EigrpUtilMgdTimerStop(timer);

	/* A loop, for picking off multiple unicasts in a row. */
	requeued_unicast = FALSE;
	while(TRUE){

		/* If there are unicasts ready to send and it is legal to send them now(we haven't hit
		  * quota yet), simply bail and the peer timer will fire for the next peer.  Note that if
		  * we are blocked because the unicast quota has been exhausted but there are no multicasts
		  * to send, we will end up sending the next unicast anyhow. */
		if(iidb->unicastLeftToSnd && EigrpUtilMgdTimerExpired(&iidb->peerTimer)){
			break;
		}

		/* Note if we are forcing a multicast transmission. */
		/* Can't send a unicast right now.  If there's an unreliable packet(multicast) on the IIDB
		  * queue, send it now. */
		qelm = (EigrpPktDescQelm_st *) EigrpUtilQue2Dequeue(iidb->xmitQue[ EIGRP_UNRELIABLE_QUEUE ]);
		if(qelm){			/* Got one */
			EigrpSendMulticast(ddb, iidb, qelm);
			break;
		}

		/* No unreliable packets available.  Go next to the reliable packet queue.  If the packet
		  * there is a multicast, send it.  If it is a unicast, requeue it for the peer and go
		  * around again. */
		qelm = (EigrpPktDescQelm_st*) iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head;
		if(qelm){ /* Got one */

			peer = qelm->pktDesc->peer;

			/* Separate out unicasts and multicasts. */
			if(peer){			/* It's a unicast */

				/* Unicast.  Queue onto the peer queue and kick the peer timer. Go back to the top
				  * of the loop on the chance that there's another unicast waiting there. */
				(void)EigrpUtilQue2Dequeue(iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]);
				EigrpUtilQue2Enqueue(peer->xmitQue[ EIGRP_RELIABLE_QUEUE ], (EigrpQueElem_st *)qelm);
				peer->flagComingUp = FALSE; /*It's ok to queue mcasts now */
				EigrpDualLogXport(ddb, EIGRP_DUALEV_REQUEUEUCAST, &peer->source,
										&peer->source, &qelm->pktDesc->ackSeqNum);
				EigrpStartPeerTimer(ddb, iidb, peer);
				requeued_unicast = TRUE;
				EIGRP_TRC(DEBUG_EIGRP_ROUTE,
							"EIGRP:  Requeued unicast from queue of %s to queue of peer %s\n",
							iidb->idb ? (S8 *)iidb->idb->name : "Null0", EigrpIpPrintAddr(&peer->source));

				continue;		/* Go around for more. */
			}else{			/* It's a multicast */

				/* Multicast packet.  If we've already requeued a unicast, then bail out;  the 
				  * unicast will be transmitted forthwith. Otherwise, if we're flow-blocked for
				  * multicast, bail out. The pacing timer will be kicked when the flow control
				  * becomes ready. */
				if(requeued_unicast)         	/* Go send the unicast. */
					break;

				if(EigrpMulticastTransmitBlocked(iidb)){
					/* Blocked? */
					EIGRP_TRC(DEBUG_EIGRP_ROUTE,"EIGRP: %s multicast flow blocked\n",
								iidb->idb ? (S8 *)iidb->idb->name : "Null0");
					break;		/* Bail out */
				}else{		/* Flow-ready */

					/* Multicast flow control ready.  Dequeue the packet and transmit it as a
					  * multicast(or pseudomulticast if this is an NBMA interface). */
					(void)EigrpUtilQue2Dequeue(iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]);
					EigrpSendMulticast(ddb, iidb, qelm);
					break;		/* Exit the loop */
				}
			}
		}
		break;				/* Get out of the loop. */
	}					/* Bottom of WHILE loop */
	EIGRP_FUNC_LEAVE(EigrpIntfPacingTimerExpiry);

	return;
}

/************************************************************************************

	Name:	EigrpRetransmitPacket

	Desc:	This function is to perform the housekeeping necessary when retransmitting a packet.
					
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the peer which the packet will be retransmitted to
	
	Ret:		TRUE 	for we've punted the link due to excessive retransmissions
			FALSE	for we're still going.
************************************************************************************/

S32	EigrpRetransmitPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	/* If we have been retransmitting this packet for at least the peer holding time and the retry
	  * count has been exceeded, declare the peer to be dead. */
	EIGRP_FUNC_ENTER(EigrpRetransmitPacket);
	if(peer->retryCnt >= EIGRP_RETRY_LIMIT &&
			EIGRP_CLOCK_OUTSIDE_INTERVAL(peer->pktFirstSndTime,
												peer->lastHoldingTime / EIGRP_MSEC_PER_SEC)){
		EigrpTakePeerDown(ddb, peer, FALSE, "retry limit exceeded");
		EIGRP_FUNC_LEAVE(EigrpRetransmitPacket);
		return TRUE;
	}

	/* Note that we're retransmitting. */
	peer->retryCnt++;

	/* Do exponential backoff. */
	peer->rto = (peer->rto * 3) / 2;
	peer->rto = MIN(peer->rto, EIGRP_RTO_UPPER);

	/* Bump the retransmission count. */
	peer->retransCnt++;
	EIGRP_FUNC_LEAVE(EigrpRetransmitPacket);

	return FALSE;
}

/************************************************************************************

	Name:	EigrpSendUnicast

	Desc:	This function is to send the next unicast packet on the queue for this peer.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- the interface on which the packet will be sent
			peer		- pointer to the peer which the packet will be sent to
	
	Ret:		NONE
************************************************************************************/

void	EigrpSendUnicast(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpDualPeer_st *peer)
{
	EigrpPktDescQelm_st	*qelm;
	EigrpPackDesc_st		*pktDesc;
	EigrpPktHdr_st		*eigrp;
	S32		flagSeq, retransmission;
	S32		suppress_ack, packet_suppressed;
	U32		qtype, opcode, flags, seq, ack_seq;

	EIGRP_FUNC_ENTER(EigrpSendUnicast);
	eigrp = NULL;

	/* Stop the timer first. */
	if(EigrpUtilMgdTimerRunning(&peer->peerSndTimer)){
		EigrpUtilMgdTimerStop(&peer->peerSndTimer);
	}

	/*If there's nothing here to send, skip it. */
	qtype = EigrpNextPeerQueue(ddb, peer);
	if(peer->xmitQue[ qtype ]->head == NULL){
		EIGRP_FUNC_LEAVE(EigrpSendUnicast);
		return;
	}

	/* Attempt to do ACK suppression.  If the top of the unreliable queue is an ACK of the last 
	  * sequence number received from the peer, and the top of the reliable queue is a packet that
	  * is ready to be transmitted, delete the ACK and force the reliable packet to be transmitted
	  * (regardless of whether a reliable or unreliable packet was scheduled to be sent now). The
	  * outgoing reliable packet will carry the proper acknowledgement.
	  *
	  * If we don't find anything on the peer reliable queue, look to see if there's a reliable
	  * unicast packet sitting on the top of the IIDB queue;  that one's just about to move over
	  * here anyhow. */
	/* Anything on the unreliable queue? */
	qelm = (EigrpPktDescQelm_st*)peer->xmitQue[ EIGRP_UNRELIABLE_QUEUE ]->head;
	if(qelm){
		pktDesc = qelm->pktDesc;

		/* Yep.  Is it a suppressable ACK? */
		if(pktDesc->flagAckPkt && peer->lastSeqNo == pktDesc->ackSeqNum){

			/* Yep.  Any reliable packets present? */
			suppress_ack = FALSE;
			qelm = (EigrpPktDescQelm_st*)peer->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head;
			if(qelm){			/* Something there? */
				pktDesc = qelm->pktDesc;

				/* Yep.  Is the reliable packet ready to go? */
				if(!EIGRP_TIMER_RUNNING_AND_SLEEPING(qelm->xmitTime)){

					/* Yep.  Delete the ACK. */
					suppress_ack = TRUE;
				}
			}else{			/* Nothing on reliable queue */
				/* Try the IIDB queue. */
				qelm = (EigrpPktDescQelm_st*) iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head;
				if(qelm){		/* Something there? */
					pktDesc = qelm->pktDesc;

					/* Yep.  Is it for us? */
					if(pktDesc->peer == peer){

						/* Yep.  Delete the ACK. */
						suppress_ack = TRUE;
					}
				}
			}

			if(suppress_ack){

				/* Do the deed. */
				EigrpDequeueQelm(ddb, peer, EIGRP_UNRELIABLE_QUEUE);
				EigrpDualLogXport(ddb, EIGRP_DUALEV_ACKSUPPR, &peer->source, &peer->source, &iidb->idb);

				qtype = EIGRP_RELIABLE_QUEUE; /* Send the reliable one! */
				iidb->ackSupp++;
			}
		}
	}

	/* Fetch the next packet without dequeueing it. */
	qelm = (EigrpPktDescQelm_st*)peer->xmitQue[ qtype ]->head;

	/* If there's no packet there, it may be because we just suppressed an ACK.  Kick the pacing
	  * timer to keep the chain going. */
	if(!qelm){
		EigrpKickPacingTimer(ddb, iidb, 0);
		EIGRP_FUNC_LEAVE(EigrpSendUnicast);
		return;
	}
	pktDesc		= qelm->pktDesc;
	flagSeq	= pktDesc->flagSeq;
	retransmission	= flagSeq && EIGRP_TIMER_RUNNING(qelm->sentTime);

	/* If this is a retransmission, twiddle the right stuff, and bail if the retransmission limit
	  * has been hit. */
	if(retransmission){
		if(EigrpRetransmitPacket(ddb, peer)){
			EIGRP_FUNC_LEAVE(EigrpSendUnicast);
			return;
		}
		iidb->retransSent++;
	}else{
		/* If this is the initial transmission of a sequenced packet, set up some fields. */
		if(flagSeq){
			EIGRP_GET_TIME_STAMP(peer->pktFirstSndTime);
		}
	}

	/* Generate the packet. */
	packet_suppressed = FALSE;

	eigrp = EigrpGeneratePacket(ddb, iidb, peer, pktDesc, &packet_suppressed);

	EigrpDualLogXport(ddb, EIGRP_DUALEV_UCASTSENT, &peer->source, &iidb->idb, &peer->source);
	EigrpDualLogXport(ddb, EIGRP_DUALEV_PKTSENT2, &peer->source, &pktDesc->serNoStart, &pktDesc->serNoEnd);
	opcode = pktDesc->opcode;	/* Make it 32 bits. */
	if(pktDesc->flagAckPkt){
		opcode = EIGRP_OPC_ACK;
	}

	/* Fill in some important fixed-header fields. */
	if(eigrp){
		/* md5 authenticate the packet. */
		if(iidb->authSet && (iidb->authMode == TRUE)){
			if((pktDesc->length >= sizeof(EigrpAuthTlv_st))          /* have room for auth TLV */
				&& (!pktDesc->flagAckPkt)){                           /* ack pack donot need auth */
				EigrpGenerateMd5(eigrp, iidb, pktDesc->length);
			}
		}

		/* Checksum the packet. */
		EigrpGenerateChksum(eigrp, pktDesc->length);

		/* Send the packet. */
		if(!EigrpInduceError(ddb, iidb, "Transmit")){
			(*ddb->sndPkt)(eigrp, (S32)(pktDesc->length + EIGRP_HEADER_BYTES), peer, iidb, FALSE);
		}
		iidb->unicastSent[ qtype ] ++;
		flags		= NTOHL(eigrp->flags);
		seq		= NTOHL(eigrp->sequence);
		ack_seq	= NTOHL(eigrp->ack);
		EigrpDualLogXport(ddb, EIGRP_DUALEV_PKTRCV2, &peer->source, &opcode, &flags);
		EigrpDualLogXport(ddb, EIGRP_DUALEV_PKTRCV3, &peer->source, &seq, &ack_seq);

		/* Print debug, if enabled. */
		EigrpDualDebugSendPacket(ddb, peer, iidb, pktDesc, eigrp, peer->retryCnt);
		EigrpUpdateMib(ddb, eigrp, TRUE);

		/* Free the buffer if we generated it locally. */
		if(!pktDesc->preGenPkt){
			EigrpPortMemFree(eigrp);
		}

		eigrp = (EigrpPktHdr_st*) 0;
	}else{				/* No packet to send */
		/* No packet. If the packet was suppressed, act as though it was sent and acknowledged. 
		  * Otherwise, act as though it was sent and lost. Only sequenced packets are ever 
		  * suppressed. */
		if(packet_suppressed){
			EigrpDualLogXport(ddb, EIGRP_DUALEV_PKTSUPPR, &peer->source, &opcode, NULL);

			/* Dequeue and free the queue element. */
			EigrpPostAck(ddb, peer, pktDesc);
			EigrpKickPacingTimer(ddb, iidb, 0); /* Don't stall the i/f! */
			EIGRP_FUNC_LEAVE(EigrpSendUnicast);
			return;
		}else{
			EigrpDualLogXport(ddb, EIGRP_DUALEV_PKTNOSEND, &peer->source, &opcode, NULL);
		}
	}

	/* Kick the interface pacing timer. */
	EigrpKickPacingTimer(ddb, iidb, EigrpPacingValue(ddb, iidb, pktDesc));
	EigrpDualLogXport(ddb, EIGRP_DUALEV_XMITTIME, &peer->source, &pktDesc->length, &iidb->lastSndIntv[ qtype ]);

	/* Make sure that we eventually send some multicasts. */
	if(iidb->unicastLeftToSnd){
		iidb->unicastLeftToSnd--;
	}

	/* Do a few things depending on whether the packet was reliable or not. */
	if(flagSeq){

		/* Note the xmit time for the RTO if not a retransmission. */
		if(!retransmission){
			EIGRP_GET_TIME_STAMP(qelm->sentTime);
		}

		/* Note the retransmit time. */
		EIGRP_TIMER_START(qelm->xmitTime, peer->rto / EIGRP_MSEC_PER_SEC);

		/* Allow unreliable packets to be sent again. */
		peer->unrelyLeftToSend = EIGRP_MAX_UNRELIABLE_PKT;
	}else{
		/* Unreliable.  Dequeue and free the queue element. */
		EigrpDequeueQelm(ddb, peer, EIGRP_UNRELIABLE_QUEUE);

		/* Charge for the unreliable packet. */
		if(peer->unrelyLeftToSend){
			peer->unrelyLeftToSend--;
		}
	}

	/* Kick the peer timer appropriately for the next packet. */
	EigrpStartPeerTimer(ddb, iidb, peer);
	EIGRP_FUNC_LEAVE(EigrpSendUnicast);
	
	return;
}

/************************************************************************************

	Name:	EigrpPeerSendTimerExpiry

	Desc:	This function is to process the expiration of a peer send timer.  This means that it's
			time to (re)send the first packet on the queue.
 
 			The peer timer will be stopped or restarted as necessary.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			timer	- pointer to the timer which is overtime
	
	Ret:		
************************************************************************************/

void	EigrpPeerSendTimerExpiry(EigrpDualDdb_st *ddb, EigrpMgdTimer_st *timer)
{
	EigrpDualPeer_st	*peer;
	EigrpIdb_st	*iidb;

	EIGRP_FUNC_ENTER(EigrpPeerSendTimerExpiry);
	peer	= EigrpUtilMgdTimerContext(timer);
	iidb	= peer->iidb;

	/* If the interface timer is running, punt until it is ready. */
	if(EigrpDeferPeerTimer(ddb, iidb, peer)){
		EIGRP_FUNC_LEAVE(EigrpPeerSendTimerExpiry);
		return;
	}

#ifdef INCLUDE_SATELLITE_RESTRICT
	/*~{Hg9{5=4K~}peer~{5D~}RTT~{;9C;SP<FKcMj3I#,TrOHMF3Y~}Hello~{1(NDV.Mb5D1(ND5D7"KM~}*/
	if(!peer->rtt){
		U32	exptime;
		exptime = EigrpUtilMgdTimerExpTime(&iidb->sndTimer);
		EigrpUtilMgdTimerSetExptime(&peer->peerSndTimer, exptime);
		EIGRP_FUNC_LEAVE(EigrpPeerSendTimerExpiry);
		return;
	}
#endif

	/* The interface timer isn't running.  Time for us to send our packet. */
	EigrpSendUnicast(ddb, iidb, peer);
	EIGRP_FUNC_LEAVE(EigrpPeerSendTimerExpiry);

	return;
}

/************************************************************************************

	Name:	EigrpPacketizeTimerExpire

	Desc:	This function is to process the expiration of an interface packetize timer.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			timer	- pointer to the timer which is overtime
	
	Ret:		
************************************************************************************/

void	EigrpPacketizeTimerExpire(EigrpDualDdb_st *ddb, EigrpMgdTimer_st *timer)
{
	EigrpIdb_st	*iidb;

	EIGRP_FUNC_ENTER(EigrpPacketizeTimerExpire);
	iidb = EigrpUtilMgdTimerContext(timer);

	EigrpDualXmitContinue(ddb, iidb, NULL, NULL);
	EIGRP_FUNC_LEAVE(EigrpPacketizeTimerExpire);

	return;
}

/************************************************************************************

	Name:	EigrpProcMgdTimerCallbk

	Desc:	This function is process the manged timer timeout event.
		
	Para: 	dummy	- unused
			param	- pointer to the event data
	
	Ret:		NONE
************************************************************************************/
void	EigrpProcMgdTimerCallbk(void * param)
{
	EigrpDualDdb_st	*dbd;

	EIGRP_FUNC_ENTER(EigrpProcMgdTimerCallbk);

	dbd = (EigrpDualDdb_st *)param;
	if(dbd == NULL  || ! EigrpDdbIsValid(dbd)){
		EIGRP_FUNC_LEAVE(EigrpProcMgdTimerCallbk);
		return;
	}
	//zhurish 2016-3-3 ��ʱ�ж�ִ�к���ɾ����ʱ��
	//EigrpPortTimerDelete(&dbd->masterTimer.sysTimer);
	
	dbd->masterTimerProc	= TRUE;//�����¼�����

#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/*	EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcManagedTimers, dbd);*/
#else
	//zhurish 2016-3-3 ���ΰѶ�ʱ�¼�������ȶ���
	//EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcManagedTimers, dbd);
#endif

	EIGRP_FUNC_LEAVE(EigrpProcMgdTimerCallbk);
	
	return;

}

/************************************************************************************

	Name:	EigrpProcManagedTimers

	Desc:	This function is to be called by the protocol-specific processes to service generic
			managed timers.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		1	for success
			0	for fail
************************************************************************************/

S32	EigrpProcManagedTimers(EigrpDualDdb_st *ddb)
{
	EigrpMgdTimer_st	*timer;
	S32			timer_type;
	EigrpIdb_st	*iidb;
	EigrpDualPeer_st	*peer;
	U32			timer_limiter;

	EIGRP_FUNC_ENTER(EigrpProcManagedTimers);
	/* Loop until we've run out of timers or we decide to do something else. */

	if(ddb == NULL  || ! EigrpDdbIsValid(ddb)){
		EIGRP_TRC(DEBUG_EIGRP_TASK,
					"EIGRP-task: EigrpProcManagedTimers() called with NULL parameter\n");
		EIGRP_FUNC_LEAVE(EigrpProcManagedTimers);
		return 0;
	}

	timer_limiter = TIMER_LIMITER;
	while(TRUE){
		/* Bail if we should do something else for awhile. */

		timer_limiter--;

		if(!timer_limiter){
			EigrpDualFinishUpdateSend(ddb);

			//EIGRP_ASSERT(!ddb->masterTimer.sysTimer);
			ddb->masterTimer.sysTimer = EigrpPortTimerCreate(EigrpProcMgdTimerCallbk, ddb, 1);
			EIGRP_FUNC_LEAVE(EigrpProcManagedTimers);
			return 0;
		}

		/* Give priority to the interface timers.  This is because the peer timers may have been
		  * set to expire at the same time as the interface timers, and they expect that the
		  * interface timers will have been stopped or reset to expire further in the future when
		  * they actually expire. Since the managed timer system does not sort entries that have
		  * equal expiration time, we impose order ourselves by examining the interface subtrees
		  * first. */
		timer = EigrpUtilMgdTimerFirstExpired(&ddb->intfTimer);
		if(!timer){         			/* No interface timers */
			timer = EigrpUtilMgdTimerFirstExpired(&ddb->peerTimer);
		}

		if(timer){
			timer_type = EigrpUtilMgdTimerType(timer);
			EIGRP_TRC(DEBUG_EIGRP_TIMER,"TIMERS: Interface or peer timer expired: timer:0x%x, expire time:%d, time now:%d, type %s.\n",
						   timer, timer->time, EigrpPortGetTimeSec(), EigrpTimerType[ timer_type ]);

			switch(timer_type){
				case EIGRP_IIDB_PACING_TIMER:
					/* Pacing timer expired. */
					EigrpIntfPacingTimerExpiry(ddb, timer);
					break;

				case EIGRP_PEER_SEND_TIMER:
					/* Peer transmit timer expired. */
					/* EigrpUtilMgdTimerStop(timer); */
					EigrpPeerSendTimerExpiry(ddb, timer);
					break;

				default:
					EigrpUtilMgdTimerStop(timer);
					EIGRP_ASSERT(0);
					break;
			}
		}else{
			/* Not an interface or peer timer.  Process it. */
			timer = EigrpUtilMgdTimerFirstExpired(&ddb->masterTimer);

			if(timer){
				timer_type = EigrpUtilMgdTimerType(timer);

				EIGRP_TRC(DEBUG_EIGRP_TIMER,"TIMERS: Process timer expired: timer:0x%x, expire time:%d, time now:%d, type %s.\n",
						 	  timer, timer->time, EigrpPortGetTimeSec(), EigrpTimerType[ timer_type ]);
				switch(timer_type){
					case EIGRP_PEER_HOLDING_TIMER:
						/* Peer holding timer expired. */
						peer = EigrpUtilMgdTimerContext(timer);
						EigrpUtilMgdTimerStop(timer);
						
						EIGRP_TRC(DEBUG_EIGRP_TIMER,"EIGRP: Peer Holdtime expired\n");
						EigrpDualLogXportAll(ddb, EIGRP_DUALEV_HOLDTIMEEXP, &peer->source,
									                &peer->iidb->idb);
						if(peer->flagGoingDown){ /*It's being torn down */
							EigrpLogPeerChange(ddb, peer, FALSE, peer->downReason);
						}else{ /*It really timed out */
							EigrpLogPeerChange(ddb, peer, FALSE, "holding time expired");
						}
						EigrpNeighborDown(ddb, peer);
						break;

					case EIGRP_FLOW_CONTROL_TIMER:
						/* Flow control timer expired. */
						/*EigrpUtilMgdTimerStop(timer); */
						EigrpFlowControlTimerExpiry(ddb, timer);
						break;

					case EIGRP_ACTIVE_TIMER:
						/* New method : ~{5%6@N*C?8v~}SIA~{I(Ch6(J1Fw6(ReR;8v~}ddb_tsk_active_timer */
						/* SIA timer expired. */
						EigrpUtilMgdTimerStop(timer);
						EIGRP_ASSERT(0);
						break;

					case EIGRP_PACKETIZE_TIMER:
						/* The hysteresis for packetizing just expired. */
						EIGRP_TRC(DEBUG_EIGRP_TASK,"EIGRP-SYS: IIDB packetize timer expired\n");
						EigrpUtilMgdTimerStop(timer);

						EigrpPacketizeTimerExpire(ddb, timer);

						break;

					case EIGRP_PDM_TIMER:
						/* The PDM timer expired.  Call the PDM to handle it. */
						EigrpUtilMgdTimerStop(timer);
						(*ddb->pdmTimerExpiry)(ddb, timer);
						break;

					case EIGRP_IIDB_ACL_CHANGE_TIMER:
						/* The ACL change timer for an interface expired.  Tear down the neighbors
						  * on that interface. */
						EigrpUtilMgdTimerStop(timer);
						iidb = EigrpUtilMgdTimerContext(timer);
						EigrpTakeNbrsDown(ddb, iidb->idb, FALSE,
									                  "route filter changed");
						break;

					case EIGRP_DDB_ACL_CHANGE_TIMER:
						/* The ACL change timer for the process expired.  Tear down all
						  * neighbors. */
						EigrpUtilMgdTimerStop(timer);
						EigrpTakeAllNbrsDown(ddb, "route filter changed");
						break;

					default:
						break;
				}
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpProcManagedTimers);
	
	return 1;
}

/************************************************************************************

	Name:	EigrpComputeMetric

	Desc:	Given a metric entry and the incoming interface, this function is to return the 
			composite metric If interface is NULL, compute metric from just the metric entry data.

			metric = [K1*bandwidth + (K2*bandwidth)/(256 - load) + K3*delay] *
 					 [K5/(reliability + K4)]
 
 			If K5 == 0, then there is no reliability term.
 
 			The default version of IGRP has K1 == K3 == 1, K2 == K4 == K5 == 0
		
	Para: 	ddb		- pointer to the dual descriptor block 
			m		- pointer to the vector metric
			dest		- pointer to destination network entry
			iidb		- pointer to the incoming interface
			neighbor	- pointer to the ip addess of neighbor
	
	Ret:		the composite metric 
************************************************************************************/

U32	EigrpComputeMetric(EigrpDualDdb_st *ddb, EigrpVmetric_st *m,EigrpNetEntry_st *dest, EigrpIdb_st *iidb, U32 *neighbor)
{
	U32	delay, bandwidth, offset;
	U32	reliability, metric;
	EigrpIntf_pt	pEigrpIntf;

	EIGRP_FUNC_ENTER(EigrpComputeMetric);
	pEigrpIntf	= iidb->idb;

	/* Combine factors from update and interface. Return the update factors in neighbor. */
	delay	= m->delay;
	if(ddb->mmetricFudge){
		offset	= (*ddb->mmetricFudge)(ddb, dest, iidb, EIGRP_OFFSET_IN);
		if(offset > EIGRP_METRIC_INACCESS - delay){
			delay = EIGRP_METRIC_INACCESS;
		}else{
			delay	+= offset;
		}
	}
	/* Figure out how to handle the inaccessible metric case since this is now exactly 32 bits 
	  * in size. $$$ */
	if(delay == EIGRP_METRIC_INACCESS || m->hopcount >= (U32)ddb->maxHopCnt){
		*neighbor	= EIGRP_METRIC_INACCESS;
		m->delay	= EIGRP_METRIC_INACCESS;
		EIGRP_FUNC_LEAVE(EigrpComputeMetric);
		return(EIGRP_METRIC_INACCESS);
	}
	bandwidth	= m->bandwidth;

	/* Compute the metric that our neighbor sees. */
	*neighbor = EigrpDualComputeMetric(bandwidth, m->load, delay,
	     m->reliability, ddb->k1, ddb->k2, ddb->k3,
	     ddb->k4, ddb->k5 );

	if(pEigrpIntf){
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST)){
			if(pEigrpIntf->delay > 0){
				delay += pEigrpIntf->delay * EIGRP_B_DELAY;/*hanbing_modify 120804*/
			}else{
				delay += EIGRP_B_DELAY;
			}
		}else{
			if(pEigrpIntf->delay > 0){
				delay += pEigrpIntf->delay * EIGRP_B_DELAY;/*hanbing_modify 120804*/
			}else{
				delay += EIGRP_S_DELAY;
			}
		}
		bandwidth	= MAX(bandwidth, EIGRP_SCALED_BANDWIDTH(1000 * pEigrpIntf->bandwidth) << 8 ); 
		reliability		= MIN((U32) m->reliability, 255 );

	}else{
		reliability	= (U32) m->reliability;
	}

	/* Compute the scalar metric for the current Type of Service.  We are currently assuming a
	  * default TOS of zero. */
	metric = EigrpDualComputeMetric(bandwidth, m->load, delay, 	reliability, ddb->k1, ddb->k2, ddb->k3, ddb->k4, ddb->k5 );
	m->delay		= delay;
	m->bandwidth	= bandwidth;
	m->reliability		= reliability;
	if(pEigrpIntf){	
		if(m->mtu > 0){
			m->mtu = MIN(m->mtu, (U32)pEigrpIntf->mtu);
		}else{
			m->mtu = pEigrpIntf->mtu;
		}
	}

	m->load = MAX(m->load, 1);
	m->hopcount++;
	EIGRP_FUNC_LEAVE(EigrpComputeMetric);

	return	metric;
}

/************************************************************************************

	Name:	EigrpFlushPeerPackets

	Desc:	This function is to flush all packets destined for a peer from the peer and IIDB 
			queues.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the given peer
	
	Ret:		NONE
************************************************************************************/

 void	EigrpFlushPeerPackets(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	U32		qtype;
	EigrpPktDescQelm_st	*qelm;
	EigrpPktDescQelm_st	**prev_qelm;
	EigrpPackDesc_st		*pktDesc;
	EigrpQue_st			*xmit_queue;
	EigrpIdb_st			*iidb;

	EIGRP_FUNC_ENTER(EigrpFlushPeerPackets);
	/*
	* Deallocate resources on transmit queues.
	*/
	for(qtype = 0; qtype < EIGRP_QUEUE_TYPES; qtype++){
		while(peer->xmitQue[ qtype ]->count){
			qelm = (EigrpPktDescQelm_st*)peer->xmitQue[ qtype ]->head;
			pktDesc = qelm->pktDesc;
			EigrpDequeueQelm(ddb, peer, qtype);
		}
	}

	/* Walk the interface reliable transmit list and pull any packets that are in flight there. I
	  * hate type coercion, but we can't automatically coerce from the void *below(because we are
	  * taking a pointer to it, and void **'s don't get automatically assigned). */
	iidb			= peer->iidb;
	xmit_queue	= iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ];
	prev_qelm	= (EigrpPktDescQelm_st **) & xmit_queue->head;
	while((qelm = *prev_qelm)){
		pktDesc = qelm->pktDesc;
		if(pktDesc->peer == peer){
			EigrpUtilQue2Unqueue((EigrpQue_st *)xmit_queue, (EigrpQueElem_st *)qelm);
			EigrpFreeQelm(ddb, qelm, iidb, peer);
		}else{
			prev_qelm = &qelm->next;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpFlushPeerPackets);

	return;
}

/************************************************************************************

	Name:	EigrpDestroyPeer

	Desc:	If EIGRP neighbor has gone down, this function is  to free resources.
			NOTE:  This does NOT inform dual about the neighbor going away.  If this is required,
			it is up to the caller to do so.  Alternatively, use EigrpNeighborDown()(from inside
			the router process) or EigrpTakeNbrsDown()(from anywhere).
 
 			This should only be called at the very end of the circuitous neighbor down process.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the neighbor to be destoryed
	
	Ret:		NONE
************************************************************************************/

void	EigrpDestroyPeer(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	EigrpDualPeer_st	**pptr, *prev;
	EigrpIdb_st	*iidb;
	S8			peeraddr[30];

	EIGRP_FUNC_ENTER(EigrpDestroyPeer);
	iidb = peer->iidb;

	if(ddb->peerStateHook != NULL){

		/* Do PDM-specific cleanup */
		(*ddb->peerStateHook)(ddb, peer, FALSE);
	}

	/* Flush all packets for this guy. */
	EigrpFlushPeerPackets(ddb, peer);

	sprintf_s(peeraddr, sizeof(peeraddr), "%s", (*ddb->printAddress)(&peer->source));
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP: Neighbor %s went down on %s\n", 
				(*ddb->printAddress)(&peer->source), iidb->idb ? (S8 *)iidb->idb->name : "Null0");
	/* Delink and free peer. */
	pptr = &ddb->peerCache[ (*ddb->peerBucket)(&peer->source) ];
	for( ; *pptr; pptr = &(*pptr) ->next){
		if(*pptr == peer){
			*pptr = peer->next;
			break;
		}
	}

	/* Take of single threaded list */
	if(ddb->peerLst == peer){
		ddb->peerLst = peer->nextPeer;
	}else{
		for(prev = ddb->peerLst; prev != NULL; prev = prev->nextPeer){
			if(prev->nextPeer == peer){
				prev->nextPeer = peer->nextPeer;
				break;
			}
		}
	}

	peer->next		= NULL;
	peer->nextPeer	= NULL;
	peer->iidb		= NULL;
	EigrpUtilMgdTimerDelete(&peer->peerSndTimer);
	EigrpUtilMgdTimerDelete(&peer->holdingTimer);
	peer->flagPeerDel = TRUE;		/* In case it's locked */

	if(EigrpDualPeerCount(iidb) == 0){	/* Last one on the i/f */
		EigrpDualLastPeerDeleted(ddb, iidb);
	}
	iidb->totalSrtt -= peer->srtt;

	EigrpPortMemFree(peer);
	peer = (EigrpDualPeer_st*) 0;
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: Destroy Peer %s SUCCESS\n", peeraddr);
	EIGRP_FUNC_LEAVE(EigrpDestroyPeer);
	
	return;
}

/************************************************************************************

	Name:	EigrpSendHello

	Desc:	This function is to send(multicast) generic periodic hello packet.
 
 			This is called from the Hello process, so we bypass all the fancy queueing stuff and
 			just blast it out.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the interface on which the packet will be sent
	
	Ret:		
************************************************************************************/

void	EigrpSendHello(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpPktHdr_st	*eigrp;
	S8		*tlv_ptr;
	S32		bytes, malloc_bytes;

	EIGRP_FUNC_ENTER(EigrpSendHello);
	if(BIT_TEST(iidb->idb->flags, EIGRP_INTF_FLAG_ACTIVE)){
		bytes = malloc_bytes = EIGRP_HELLO_HDR_BYTES;
		if(iidb->authSet && (iidb->authMode == TRUE)){
			malloc_bytes	+= sizeof(EigrpAuthTlv_st);
			bytes	= malloc_bytes;
		}
		if(ddb->hellohook){
			malloc_bytes	+= EIGRP_HELLO_HOSUCCESS_BYTE_KLUDGE;
		}
		eigrp	= EigrpAllocPktbuff(  malloc_bytes);
		if(eigrp){
			tlv_ptr = EigrpBuildHello(iidb, eigrp, ddb->k1, ddb->k2, ddb->k3, ddb->k4,
								ddb->k5, (U16)(iidb->holdTime / EIGRP_MSEC_PER_SEC));
			if(!ddb->hellohook || ((*ddb->hellohook)(ddb, eigrp, &bytes, iidb->idb, tlv_ptr))){
				EigrpSendHelloPacket(ddb, eigrp, iidb, bytes, TRUE);
			}

			EigrpPortMemFree(eigrp);
			eigrp = (EigrpPktHdr_st*) 0;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpSendHello);

	return;
}

/************************************************************************************

	Name:	EigrpProcHelloTimerCallbk

	Desc:	This function is to be eigrp hello timer call back.
		
	Para: 	dummy		- unused
			param		- pointer the data of event
	
	Ret:		NONE
************************************************************************************/
void	EigrpProcHelloTimerCallbk(void * param)
{
	EigrpDualDdb_st	*dbd;

	EIGRP_FUNC_ENTER(EigrpProcHelloTimerCallbk);

	dbd = (EigrpDualDdb_st *)param;
	if(dbd == NULL  || ! EigrpDdbIsValid(dbd)){
		EIGRP_FUNC_LEAVE(EigrpProcHelloTimerCallbk);
		return;
	}
	//zhurish 2016-3-3 ��ʱ�ж�ִ�к���ɾ����ʱ��
	//EigrpPortTimerDelete(&dbd->helloTimer.sysTimer);
	
	dbd->helloTimerProc	= TRUE;//�����¼�����

#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/*	EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcHelloTimers, dbd);*/
#else
	//zhurish 2016-3-3 ���ΰѶ�ʱ�¼�������ȶ���
	//EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcHelloTimers, dbd);
#endif

	EIGRP_FUNC_LEAVE(EigrpProcHelloTimerCallbk);

	return;
}

/************************************************************************************

	Name:	EigrpProcHelloTimers

	Desc:	This function is process eigrp hello timer timeout event.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		0
************************************************************************************/

S32	EigrpProcHelloTimers(EigrpDualDdb_st *ddb)
{
	EigrpIdb_st	*iidb;
	EigrpMgdTimer_st	*timer;

	EIGRP_FUNC_ENTER(EigrpProcHelloTimers);
	if(ddb == NULL || ! EigrpDdbIsValid(ddb)){
		EIGRP_TRC(DEBUG_EIGRP_TASK,
					"EIGRP-task: EigrpProcHelloTimers() called with NULL parameter\n");
		EIGRP_FUNC_LEAVE(EigrpProcHelloTimers);
		return 0;
	}
	EIGRP_TRC(DEBUG_EIGRP_TIMER,"TIMERS: Some hello timer expired.\n");

	while(TRUE){
		timer = EigrpUtilMgdTimerFirstExpired(&ddb->helloTimer);
		if(!timer){
			break;
		}
		switch(EigrpUtilMgdTimerType(timer)){
			case EIGRP_IIDB_HELLO_TIMER:
				/* Peer hello timer expired.  The send routine resets the timer. */
				EigrpUtilMgdTimerStop(timer);
				iidb = EigrpUtilMgdTimerContext(timer);
				EIGRP_TRC(DEBUG_EIGRP_TIMER,"TIMERS: hello timer expired on %s.\n", 
							iidb->idb->name);
				EigrpSendHello(ddb, iidb);

				/* Kick the timer. */
				EigrpUtilMgdTimerStartJittered(&iidb->helloTimer, (S32)(iidb->helloInterval), EIGRP_JITTER_PCT);
				break;

			default:
				EigrpUtilMgdTimerStop(timer);
				EIGRP_TRC(DEBUG_EIGRP_ALL,
							"EIGRP-task: EigrpProcHelloTimers: wrong type timer get by EigrpUtilMgdTimerFirstExpired\n");
				EIGRP_ASSERT(0);
				break;
		}
	}

	EIGRP_ASSERT(!ddb->helloTimer.sysTimer);
	ddb->helloTimer.sysTimer	= EigrpPortTimerCreate(EigrpProcHelloTimerCallbk, ddb, 1);
	EIGRP_FUNC_LEAVE(EigrpProcHelloTimers);

	return 0;
}

/************************************************************************************

	Name:	EigrpLinkNewIidb

	Desc:	This function is to lnk a new IIDB into the appropriate data structures.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the new IIDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpLinkNewIidb(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpIdbLink_st	*idb_link;

	EIGRP_FUNC_ENTER(EigrpLinkNewIidb);
	/* Link into the IDB. */
	pEigrpIntf = iidb->idb;
	idb_link = EigrpPortMemMalloc(sizeof(EigrpIdbLink_st));
	if(!idb_link){	 
		EIGRP_FUNC_LEAVE(EigrpLinkNewIidb);
		return;
	}

	idb_link->next		= pEigrpIntf->private;	 
	pEigrpIntf->private	= idb_link;
	idb_link->ddb			= ddb;
	idb_link->iidb			= iidb;
	/* set reference to ifap */
	EIGRP_FUNC_LEAVE(EigrpLinkNewIidb);

	return;
}

/************************************************************************************

	Name:	EigrpIsIntfPassive

	Desc:	This function is to judge if the given interface is passive interface from the given 
			pdb's point of view.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the given interface
		
	Ret:		TRUE	for the given interface is passive interface
			FALSE	for it is not
************************************************************************************/

S32	EigrpIsIntfPassive(EigrpPdb_st *pdb, EigrpIntf_pt pEigrpIntf)
{
	EigrpQue_st	*qu;
	EigrpIdb_st	*elm;

	EIGRP_FUNC_ENTER(EigrpIsIntfPassive);
	qu	= pdb->ddb->passIidbQue;

	if(qu->count == 0){
		EIGRP_FUNC_LEAVE(EigrpIsIntfPassive);
		return FALSE;
	}

	for(elm = (EigrpIdb_st*)pdb->ddb->passIidbQue->head; elm; elm = elm->next){
		if(elm->idb == pEigrpIntf){
			EIGRP_FUNC_LEAVE(EigrpIsIntfPassive);
			return TRUE;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpIsIntfPassive);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpIsIntfInvisible

	Desc:	This function is to judge if the given interface is invisible interface from the given 
			pdb's point of view.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the given interface
		
	Ret:		TRUE	for the given interface is passive interface
			FALSE	for it is not
************************************************************************************/

S32	EigrpIsIntfInvisible(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf)
{
	EigrpConfIntf_pt	pConfIntf;
	EigrpConfIntfInvisible_pt	pInvisible;
	S32 invisible;

	EIGRP_FUNC_ENTER(EigrpIsIntfInvisible);
	pInvisible	= NULL;
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(!EigrpPortStrCmp(pConfIntf->ifName, pEigrpIntf->name)){
			break;
		}
	}

	if(pConfIntf){
		for(pInvisible = pConfIntf->invisible; pInvisible; pInvisible = pInvisible->next){
			if(pInvisible->asNum == ddb->asystem){
				break;
			}
		}
	}

	if(pInvisible){
		if(EIGRP_DEF_INVISIBLE == TRUE){
			EIGRP_FUNC_LEAVE(EigrpIsIntfInvisible);
			return FALSE;
		}else{
			EIGRP_FUNC_LEAVE(EigrpIsIntfInvisible);
			return TRUE;
		}
	}else{
		EIGRP_FUNC_LEAVE(EigrpIsIntfInvisible);
		return EIGRP_DEF_INVISIBLE;
	}
}

/************************************************************************************

	Name:	EigrpDestroyIidb

	Desc:	This function is to destroy an IIDB.
 
 			This is called by the cleanup code to get rid of an IIDB when EIGRP is deconfigured
 			from an interface.
 
			The IIDB is mem_locked for every DRDB that points at it, so our freeing it may not
			completely free it(a DRDB may still be active, for example).
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pointer to the given IIDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpDestroyIidb(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpIdbLink_st	*idb_link, **prev_link;
	EigrpIntf_pt		pEigrpIntf;

	EIGRP_FUNC_ENTER(EigrpDestroyIidb);
	if(!(ddb && iidb)){	
		EIGRP_ASSERT(ddb && iidb);
		EIGRP_FUNC_LEAVE(EigrpDestroyIidb);
		return;
	}

	/* Call the PDM to do any PDM-specific cleanup. */
	/* PDM -- protocal dependent modual . */
	if(ddb->iidbCleanUp){
		(*ddb->iidbCleanUp)(ddb, iidb);
	}

	/* Whimper if there are any peers there. */
	if(EigrpDualPeerCount(iidb)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDestroyIidb);
		return;
	}

	/* Turn off all timers on the interface. */
	EigrpUtilMgdTimerDelete(&iidb->sndTimer);
	EigrpUtilMgdTimerDelete(&iidb->peerTimer);
	EigrpUtilMgdTimerDelete(&iidb->holdingTimer);
	EigrpUtilMgdTimerDelete(&iidb->helloTimer);
	EigrpUtilMgdTimerDelete(&iidb->flowCtrlTimer);
	EigrpUtilMgdTimerDelete(&iidb->pktTimer);
	EigrpUtilMgdTimerDelete(&iidb->aclChgTimer);

	/* Flush the transmit queues. */
	EigrpFlushIidbXmitQueues(ddb, iidb);

	/* Delink it from the IDB. */
	pEigrpIntf = iidb->idb;
	idb_link	= (EigrpIdbLink_st *)pEigrpIntf->private;
	prev_link	= (EigrpIdbLink_st**) &pEigrpIntf->private;

	while(idb_link){
		if(idb_link->ddb == ddb){
			*prev_link	= idb_link->next; /* Delink */
			EigrpPortMemFree(idb_link);
			idb_link	= (EigrpIdbLink_st*) 0;
			/* reset the reference to ifap */
			break;
		}
		prev_link	= &idb_link->next;
		idb_link	= idb_link->next;
	}

	/* Toss it in the trash. */
	if(iidb->handle.array){
		EigrpPortMemFree(iidb->handle.array);
		iidb->handle.array = (U32*) 0;
	}
	EigrpPortMemFree(iidb);
	iidb	= (EigrpIdb_st*) 0;
	EIGRP_FUNC_LEAVE(EigrpDestroyIidb);

	return;
}

/************************************************************************************

	Name:	EigrpProcDying

	Desc:	This function is to do common cleanup when a process goes away.
		
	Para: 	ddb		- pointer to the dual descriptor block 
		
	Ret:		NONE
************************************************************************************/

void	EigrpProcDying(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpProcDying);
	if(gpEigrp->rtrCnt == 0){
		EIGRP_ASSERT(0);
	}

	gpEigrp->rtrCnt--;		/* We're gone */

	if(gpEigrp->rtrCnt == 0){	/* Last one? */
		/* Get rid of the chunk memory allocated. */
		EIGRP_CHUNK_DESTROY(gpEigrp->pktDescChunks);
		gpEigrp->pktDescChunks		= NULL;
		EIGRP_CHUNK_DESTROY(gpEigrp->pktDescQelmChunks);
		gpEigrp->pktDescQelmChunks	= NULL;
		EIGRP_CHUNK_DESTROY(gpEigrp->anchorChunks);
		gpEigrp->anchorChunks			= NULL;
		EIGRP_CHUNK_DESTROY(gpEigrp->dummyChunks);
		gpEigrp->dummyChunks		= NULL;
		/* EIGRP_CHUNK_DESTROY(eigrp_inpak_chunks);
		  * eigrp_inpak_chunks = NULL; */
	}
	EIGRP_FUNC_LEAVE(EigrpProcDying);

	return;
}

/************************************************************************************

	Name:	EigrpAllocateIidb

	Desc:	This function is to allocate an IIDB structure for ddb and enqueue it.  Pulls an IIDB 
			from the temp queue if appropriate.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the physical interface
			passive_queue	- the sign by which we create a passive IIDB or not
	
	Ret:		TRUE	for success to allocate
			FALSE	for fail to allocate
************************************************************************************/

S32	EigrpAllocateIidb(EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf, S32 passive_queue)
{
	EigrpIdb_st *iidb;

	EIGRP_FUNC_ENTER(EigrpAllocateIidb);
	iidb = EigrpFindIidb(ddb, pEigrpIntf);
	if(iidb){
		EIGRP_FUNC_LEAVE(EigrpAllocateIidb);
		return TRUE;
	}

	/* See if iidb is already on the temp queue. Move over to active queue. Otherwise, allocate one
	  * for the active queue. */
	iidb = EigrpFindTempIidb(ddb, pEigrpIntf);
	if(iidb){
		EigrpUtilQue2Unqueue(ddb->temIidbQue, (EigrpQueElem_st *)iidb);
	}else{
		iidb = EigrpCreateIidb(ddb, pEigrpIntf);
		if(!iidb){
			EIGRP_FUNC_LEAVE(EigrpAllocateIidb);
			return FALSE;
		}
	}

	EigrpLinkNewIidb(ddb, iidb);
	if(passive_queue){
		EigrpUtilQue2Enqueue(ddb->passIidbQue, (EigrpQueElem_st *)iidb);
	}else{
		EigrpUtilQue2Enqueue(ddb->iidbQue, (EigrpQueElem_st *)iidb);
	}
	iidb->passive	= passive_queue;
	iidb->invisible	= EigrpIsIntfInvisible(ddb, pEigrpIntf);

	/* Adjust the size of the scratchpad for DUAL. */
	EigrpDualAdjustScratchPad(ddb);

	if(!passive_queue){
		/* Set the hello timer to expire now. */
		EigrpUtilMgdTimerStart(&iidb->helloTimer, EIGRP_INIT_HELLO_DELAY);
		EIGRP_TRC(DEBUG_EIGRP_TIMER,"Eigrp_allocate_iidb: set first hello timer\n");
	}
	if(ddb->setMtu){
		(*ddb->setMtu)(iidb);
	}
	EIGRP_FUNC_LEAVE(EigrpAllocateIidb);

	return TRUE;
}

/************************************************************************************

	Name:	EigrpFreeDdb

	Desc:	This function is to free resources associated withe a ddb if its routing process is 
			going away.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpFreeDdb(EigrpDualDdb_st *ddb)
{
	EigrpIdb_st		*iidb, *next;
	EigrpDualPeer_st	*peer;

	EIGRP_FUNC_ENTER(EigrpFreeDdb);
	ddb->flagGoingDown = TRUE;

	/* Dequeue ddb from queue. */
	EigrpUtilQue2Unqueue(gpEigrp->ddbQue, (EigrpQueElem_st *)ddb);

	/* Destroy any lingering peers.  Their death has been scheduled (by virtue of the protocol
	  * effectively doing a "no network <foo>") but the timers haven't had a chance to kick. This
	  * will have the side effect of finishing the destruction of a bunch of IIDBs as well(those
	  * that have active peers).  IIDBs without any peers have destruction timers running on them; 
	  * this is taken care of below. */
	while((peer = ddb->peerLst)){
		EigrpNeighborDown(ddb, peer);	 /*Deletes the peer from the DDB!*/
	}

	/* Destroy any IIDBs still linked to the DDB.  They should all be gone. */
	while((iidb = (EigrpIdb_st*) ddb->iidbQue->head)){
		EigrpDeallocateIidb(ddb, iidb);
	}

	while((iidb = (EigrpIdb_st*) ddb->passIidbQue->head)){
		EigrpDeallocateIidb(ddb, iidb);
	}

	for(iidb = (EigrpIdb_st*) ddb->temIidbQue->head; iidb; iidb = next){
		next	= iidb->next;
		EigrpPortMemFree(iidb);
		iidb	= (EigrpIdb_st*) 0;
	}

	/* Wipeout topology table and associated data structures. */
	EigrpDualCleanUp(ddb);

	EigrpUtilMgdTimerDelete(&ddb->masterTimer);
	EigrpUtilMgdTimerDelete(&ddb->helloTimer); /* It should already be stopped. */

	/* Clear threadAtciveTimer */
	EigrpPortTimerDelete(&ddb->threadAtciveTimer);

	/* Free the peer array table. */
	if(ddb->peerArray){
		EigrpPortMemFree(ddb->peerArray);
	}

	/* Release the Null0 IIDB. */
	if(ddb->nullIidb){
		EigrpPortMemFree(ddb->nullIidb);
	}

	/* Toss the scratchpad area. */
	if(ddb->scratch){
		EigrpPortMemFree(ddb->scratch);
	}

	/* Free the handles. */
	if(ddb->handle.array){
		EigrpPortMemFree(ddb->handle.array);
	}

	/* Kill off the hello process. */
	/*process_kill(ddb->hello_pid); */
	/* Free up some chunks. */
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: destroying extDataChunk->qlen = %d \n",
				ddb->extDataChunk->queLen);
	EIGRP_CHUNK_DESTROY(ddb->extDataChunk);
	EIGRP_TRC(DEBUG_EIGRP_EVENT,"EIGRP-EVENT: destroying workQueChunk->qlen = %d \n",
				ddb->workQueChunk->queLen);
	EIGRP_CHUNK_DESTROY(ddb->workQueChunk);

	/* Possibly free up common resources. */
	EigrpProcDying(ddb);

	/* All allocated memory pointed to from ddb should be gone, get rid of the whole block. */
	EigrpPortMemFree(ddb);
	ddb = (EigrpDualDdb_st*) 0;
	EIGRP_FUNC_LEAVE(EigrpFreeDdb);

	return;
}

/************************************************************************************

	Name:	EigrpDeallocateIidb

	Desc:	This function is to get rid of an iidb.

			This routine *must*be called on the process thread.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the iidb which is to be got rid of
	
	Ret:		NONE
************************************************************************************/

void	EigrpDeallocateIidb(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpDeallocateIidb);
	if(!iidb){          /* Why are we here? */
		EIGRP_FUNC_LEAVE(EigrpDeallocateIidb);
		return;
	}

	/* Take down any remaining neighbors. */
	EigrpTakeNbrsDown(ddb, iidb->idb, TRUE, "interface disabled");

	/* Dequeue the IIDB from the DDB. */
	if(iidb->passive == TRUE){
		EigrpUtilQue2Unqueue(ddb->passIidbQue, (EigrpQueElem_st *)iidb);
	}else{
		EigrpUtilQue2Unqueue(ddb->iidbQue, (EigrpQueElem_st *)iidb);
	}

	/* Adjust the scratchpad area for DUAL. */
	EigrpDualAdjustScratchPad(ddb);

	/* Remove the IIDB from the DDB quiescent list, if it's there. */
	EigrpDualRemoveIidbFromQuiescentList(ddb, iidb);

	/* Clean up the topology table. */
	EigrpDualIfDelete(ddb, iidb);
	
	/* Destroy the IIDB. */
	EigrpDestroyIidb(ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpDeallocateIidb);

	return;
}

/************************************************************************************

	Name:	EigrpPassiveInterface

	Desc:	This function is to switch an interface from active to passive or vice versa.

			sense == TRUE if interface is becoming passive
 
 			This MUST be called on the process thread.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the interface which is needed to switch
			sense	- the sign by whch we judge whether the interface is becoming passive or not
	
	Ret:		NONE
************************************************************************************/

void	EigrpPassiveInterface(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, S32 sense)
{
	EIGRP_FUNC_ENTER(EigrpPassiveInterface);
	if(sense){			/* Going passive */
		/* Take down any remaining neighbors. */
		EigrpTakeNbrsDown(ddb, iidb->idb, TRUE, "interface passive");

		/* Dequeue the IIDB from the DDB. */
		EigrpUtilQue2Unqueue(ddb->iidbQue, (EigrpQueElem_st *)iidb);

		/* Kill the hello timer. */
		EigrpUtilMgdTimerStop(&iidb->helloTimer);

		/* Requeue the IIDB onto the passive queue. */
		EigrpUtilQue2Enqueue(ddb->passIidbQue, (EigrpQueElem_st *)iidb);

	}else{				/* Going active */
		/* Requeue the IIDB appropriately. */
		EigrpUtilQue2Unqueue(ddb->passIidbQue, (EigrpQueElem_st *)iidb);
		EigrpUtilQue2Enqueue(ddb->iidbQue, (EigrpQueElem_st *)iidb);

		/* Start the hello timer. */
		EigrpUtilMgdTimerStart(&iidb->helloTimer, EIGRP_INIT_HELLO_DELAY);
	}
	iidb->passive = sense;
	EIGRP_FUNC_LEAVE(EigrpPassiveInterface);

	return;
}

/************************************************************************************

	Name:	EigrpInvisibleInterface

	Desc:	This function is to switch an interface from active to invisible or vice versa.

			sense == TRUE if interface is becoming passive
 
 			This MUST be called on the process thread.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the interface which is needed to switch
			sense	- the sign by whch we judge whether the interface is becoming invisible or not
	
	Ret:		NONE
************************************************************************************/

void	EigrpInvisibleInterface(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, S32 sense)
{
	EigrpIdb_st		*pIidb ;

	EIGRP_FUNC_ENTER(EigrpInvisibleInterface);
	
	iidb->invisible	= sense;

#if 0	
	if(sense){			/* Going passive */
		/* Take down any remaining neighbors. */
		for(pIidb = (EigrpIdb_st*) ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
			if(pIidb != iidb){
				EigrpTakeNbrsDown(ddb, pIidb->idb, TRUE, "interface invisible");
			}
		}
	}else{
		/* Going active */
	}
#endif
	
	/* Take down any remaining neighbors. */
	for(pIidb = (EigrpIdb_st*) ddb->iidbQue->head; pIidb; pIidb = pIidb->next){
		if(pIidb != iidb){
			EigrpTakeNbrsDown(ddb, pIidb->idb, TRUE, "interface invisible");
		}
	}
	EIGRP_FUNC_LEAVE(EigrpInvisibleInterface);

	return;
}/************************************************************************************

	Name:	EigrpNextSeqNumber

	Desc:	This function is to get the next sequence number.
		
	Para: 	number		- current sequence number
	
	Ret:		next sequence number
************************************************************************************/

U32	EigrpNextSeqNumber(U32 number)
{
	EIGRP_FUNC_ENTER(EigrpNextSeqNumber);
	number++;

	/* Reach upper limit of sequence space. */
	if(!number){
		number	= 1;
	}
	EIGRP_FUNC_LEAVE(EigrpNextSeqNumber);
	
	return(number);
}

/************************************************************************************

	Name:	EigrpSetSeqNumber

	Desc:	This function is to set the packet descriptor sequence number field to the next 
			available	value.  Assumes that the packet is sequenced.
 
 			Updates the DDB sequence number field.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pktDesc	- pointer to the given packet descriptor
	
	Ret:		NONE
************************************************************************************/

 void	EigrpSetSeqNumber(EigrpDualDdb_st *ddb, EigrpPackDesc_st *pktDesc)
{
	EIGRP_FUNC_ENTER(EigrpSetSeqNumber);
	ddb->sndSeqNum	= EigrpNextSeqNumber(ddb->sndSeqNum);
	pktDesc->ackSeqNum	= ddb->sndSeqNum;
	EIGRP_FUNC_LEAVE(EigrpSetSeqNumber);

	return;
}

/************************************************************************************

	Name:	EigrpEnqueuePak

	Desc:	This function is to enqueue a packet descriptor to be sent.  The packet descriptor is
			enqueued in the proper place, based on the settings of the peer, iidb, and use_seq
			parameters.
  			  			
 			The iidb parameter must always be non-NULL.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the peer which the packet will be sent to,If peer is non-NULL,
					   the packet will be unicasted;  if NULL, it will be multicasted
			iidb		- pointer the interface on which the packet will be sent
			use_seq	- the sign by which we judge whether the packet will be sent reliably or not
				          If use_seq is TRUE, the packet will be sent reliably.
	
	Ret:		pointer the the packet descriptor
************************************************************************************/

EigrpPackDesc_st *EigrpEnqueuePak(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpIdb_st *iidb, S32 use_seq)
{
	EigrpPackDesc_st		*pktDesc;
	EigrpPktDescQelm_st	*qelm;
	void					*xmit_queue;
	U32					qtype;
	S32					iidb_queue;

	EIGRP_FUNC_ENTER(EigrpEnqueuePak);
	/* Allocate a packet descriptor.  Fill it in. */
	pktDesc	= (EigrpPackDesc_st *) EigrpUtilChunkMalloc(gpEigrp->pktDescChunks);
	if(!pktDesc){
		EIGRP_FUNC_LEAVE(EigrpEnqueuePak);
		return NULL;
	}
	pktDesc->flagPktDescMcast	= (peer == NULL);

	/* If sequenced, set the bit and get a sequence number. */
	if(use_seq){
		pktDesc->flagSeq	= TRUE;
		EigrpSetSeqNumber(ddb, pktDesc);
	}

	/* Allocate a queue element for the packet.  Bind the descriptor to it. */
	qelm	= (EigrpPktDescQelm_st *) EigrpUtilChunkMalloc(gpEigrp->pktDescQelmChunks);
	if(!qelm){
		EigrpUtilChunkFree(gpEigrp->pktDescChunks, pktDesc);
		EIGRP_FUNC_LEAVE(EigrpEnqueuePak);
		return NULL;
	}
	EigrpBuildPakdesc(pktDesc, qelm);

	/* Enqueue the packet.  Unreliable unicasts go directly to the peer queue;  everything else 
	  * goes via the IIDB queue for serialization. */
	qtype	= use_seq ? EIGRP_RELIABLE_QUEUE : EIGRP_UNRELIABLE_QUEUE;
	iidb_queue	= (use_seq || (!peer));
	pktDesc->peer	= peer;
	if(iidb_queue){
		xmit_queue	= iidb->xmitQue[ qtype ];
	}else{
		xmit_queue	= peer->xmitQue[ qtype ];
	}
	EigrpUtilQue2Enqueue((EigrpQue_st *)xmit_queue, (EigrpQueElem_st *)qelm);

	/* Now kick the appropriate timer. */
	if(iidb_queue){			/* On the IIDB */
		EigrpKickPacingTimer(ddb, iidb, 1); /* Jitter just a tad */
	}else{				/* On the peer queue */
		EigrpStartPeerTimer(ddb, iidb, peer);
	}
	EIGRP_FUNC_LEAVE(EigrpEnqueuePak);
	
	return pktDesc;
}

/************************************************************************************

	Name:	EigrpIntfIsFlowReady

	Desc:	This function is to return TRUE if the interface is flow-control ready, meaning that a
			new	multicast packet would be eligible to be transmitted immediately on this
			interface(ignoring the pacing timer).  This is the case only if there are no 
			outstanding multicasts on this interface, or if there is a single outstanding multicast
			and the multicast flow control timer has expired.	This will allow a new multicast to
			be transmitted even though some peers may be slow in acknowledging the last one.
			
			If this routine returns FALSE, it is guaranteed that a transmit_done callback will 
			happen at some point in the future.
		
	Para: 	iidb		- pointer to the given interface
	
	Ret:		TRUE	- for the interface is flow-control ready
			FALSE	- for it is not
************************************************************************************/

S32	EigrpIntfIsFlowReady(EigrpIdb_st *iidb)
{
	return !(iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]->head ||
				EigrpMulticastTransmitBlocked(iidb) ||
				(iidb->goingDown == TRUE) ||
				EigrpDualPeerCount(iidb) == 0);
}

/************************************************************************************

	Name:	EigrpFlushXmitQueue

	Desc:	This function is to flush all the elements out of a transmit queue, freeing the queue
			elements.
 
 			This is used to empty out the IIDB queues before we destroy it.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the IIDB
			queue	- pointer to the transmit queue of IIDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpFlushXmitQueue(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb,EigrpQue_st *queue)
{
	EigrpPktDescQelm_st *qelm;

	EIGRP_FUNC_ENTER(EigrpFlushXmitQueue);
	while((qelm = (EigrpPktDescQelm_st *) EigrpUtilQue2Dequeue(queue))){
		EigrpFreeQelm(ddb, qelm, iidb, NULL);
	}
	EIGRP_FUNC_LEAVE(EigrpFlushXmitQueue);

	return;
}

/************************************************************************************

	Name:	EigrpFlushIidbXmitQueues

	Desc:	This function is to flush the transmit queues on the IIDB, either because the interface
			is being destroyed, or because the last peer has gone down.  Note that DUAL will be
			called back as each packet is peeled off of the reliable queue;  this will clean up the
			various DUAL transmit linkages.

			The last multicast flow control stuff is cleaned up as well.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the IIDB on which the transmit queues will be flushed
	
	Ret:		NONE
************************************************************************************/

void	EigrpFlushIidbXmitQueues(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpFlushIidbXmitQueues);
	/* Flush each of 'em. */
	EigrpFlushXmitQueue(ddb, iidb, iidb->xmitQue[ EIGRP_UNRELIABLE_QUEUE ]);
	EigrpFlushXmitQueue(ddb, iidb, iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]);

	/* Flush the multicast flow control pointer. */
	EigrpCleanupMultipak(ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpFlushIidbXmitQueues);

	return;
}

/************************************************************************************

	Name:	EigrpOpercodeItoa

	Desc:	This function is to decode an igrp/eigrp opcode type.
 
 			Note that the "ACK" opcode doesn't really exist at this time;  the caller has to play
 			the appropriate games with Hellos to find these.
		
	Para: 	opcode	- the operation code
	
	Ret:		
************************************************************************************/

S8 *EigrpOpercodeItoa(S32 opcode)
{
	static S8 *const opcode_names[ EIGRP_OPC_TOTAL ] ={
		"UNKNOWN", 	"UPDATE", 	"REQUEST",
		"QUERY", 		"REPLY", 	"HELLO",
		"IPXSAP", 		"PROBE", 	"ACK" };

	EIGRP_FUNC_ENTER(EigrpOpercodeItoa);
	if(opcode < EIGRP_OPC_TOTAL){
		EIGRP_FUNC_LEAVE(EigrpOpercodeItoa);
		return opcode_names[ opcode ];
	}else{
		EIGRP_FUNC_LEAVE(EigrpOpercodeItoa);
		return opcode_names[0];
	}
}

/************************************************************************************

	Name:	EigrpProcRouterEigrp

	Desc:	This function is to process the command " router eigrp xxx".
		
	Para: 	asNum	- the asystem number
	
	Ret:		SUCCESS ,or FAILURE if the asNum is equal to 0
************************************************************************************/

S32	EigrpProcRouterEigrp(U32 asNum)
{
	EigrpPdb_st	*pPdb;
	
	EIGRP_FUNC_ENTER(EigrpProcRouterEigrp);
	if(!asNum){
		EIGRP_FUNC_LEAVE(EigrpProcRouterEigrp);
		return FAILURE;
	}

	pPdb = EigrpIpFindPdb(asNum);
	if(pPdb){
		/* This eigrp task has exist. */
		EIGRP_FUNC_LEAVE(EigrpProcRouterEigrp);
		return SUCCESS;
	}

	EigrpIpPdbInit(asNum);
	EIGRP_FUNC_LEAVE(EigrpProcRouterEigrp);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpProcNoRouterEigrp

	Desc:	This function is to process the command " no router eigrp xxx"
		
	Para: 	asNum	- the asystem number
	
	Ret:		SUCCESS ,or FAILURE if the asNum is equal 0
************************************************************************************/

S32	EigrpProcNoRouterEigrp(U32 asNum)
{
	EigrpPdb_st	*pPdb;
	
	EIGRP_FUNC_ENTER(EigrpProcNoRouterEigrp);
	if(!asNum){
		EIGRP_FUNC_LEAVE(EigrpProcNoRouterEigrp);
		return FAILURE;
	}

	pPdb = EigrpIpFindPdb(asNum);
	if(!pPdb){
		/* This eigrp task does not  exist. */
		EIGRP_FUNC_LEAVE(EigrpProcNoRouterEigrp);
		return SUCCESS;
	}

	EigrpIpCleanup(pPdb);
	EIGRP_FUNC_LEAVE(EigrpProcNoRouterEigrp);

	return 0;
}

/************************************************************************************

	Name:	EigrpDdbIsValid

	Desc:	This function is to find out if the given ddb is still a member of some pdb.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		TRUE	for the given ddb is still a member of some pdb
			FALSE	for the given ddb does not belong any pdb
************************************************************************************/

S32	EigrpDdbIsValid(EigrpDualDdb_st *ddb)
{
	EigrpPdb_st	*pPdb;

	EIGRP_FUNC_ENTER(EigrpDdbIsValid);
	for(pPdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pPdb; pPdb = pPdb->next){
		if(pPdb->ddb == ddb){
			EIGRP_FUNC_LEAVE(EigrpDdbIsValid);
			return TRUE;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDdbIsValid);

	return FALSE;
}
