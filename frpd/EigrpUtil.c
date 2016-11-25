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

	Module:	Eigrputil

	Name:	Eigrputil.c

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

S8 *EigrpTimerType[] =
{
	"EIGRP_IIDB_PACING_TIMER",
	"EIGRP_PEER_SEND_TIMER",
	"EIGRP_IIDB_HELLO_TIMER",
	"EIGRP_PEER_HOLDING_TIMER",
	"EIGRP_FLOW_CONTROL_TIMER",
	"EIGRP_ACTIVE_TIMER",
	"EIGRP_PACKETIZE_TIMER",
	"EIGRP_PDM_TIMER",
	"EIGRP_IIDB_ACL_CHANGE_TIMER",
	"EIGRP_DDB_ACL_CHANGE_TIMER",
	NULL
};
extern	Eigrp_pt	gpEigrp;

/************************************************************************************

	Name:	EigrpUtilMgdTimerType

	Desc:	This function is to get the type of the given eigrp managed timer.
		
	Para: 	timer	- eigrp managed timer
	
	Ret:		the type of the managed timer
************************************************************************************/

S32	EigrpUtilMgdTimerType(EigrpMgdTimer_st *timer)
{
	return timer->type;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerContext

	Desc:	This function is to get the pointer to the extra data of an eigrp leaf timer.
		
	Para: 	timer	- eigrp managed timer
	
	Ret:		the pointer to the extra data of the given timer
************************************************************************************/

void *EigrpUtilMgdTimerContext(EigrpMgdTimer_st *timer)
{
	return timer->context;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerRunning

	Desc:	This function is to find out if the given timer is still running.
		
	Para: 	timer	- eigrp managed timer
	
	Ret:		TRUE 	for the timer is still running;
			FALSE 	for it is not. 
************************************************************************************/

S32	EigrpUtilMgdTimerRunning(EigrpMgdTimer_st *timer)
{
	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerRunning);	
	if(BIT_TEST(timer->flag, EIGRP_MGDF_START)){
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerRunning);
		return TRUE;
	}	
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerRunning);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerExpTime

	Desc:	This function is to get the expiring time of the given timer.
		
	Para: 	timer	- eigrp managed timer
	
	Ret:		the expiring time of the given timer
************************************************************************************/

U32	EigrpUtilMgdTimerExpTime(EigrpMgdTimer_st *timer)
{
	return timer->time;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerLeftSleeping

	Desc:	This function is to get the left time of an eigrp managed timer.
		
	Para: 	timer	- eigrp managed timer
	
	Ret:		the left time of the given timer
************************************************************************************/

U32	EigrpUtilMgdTimerLeftSleeping(EigrpMgdTimer_st *timer)
{
	U32	timeSec;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerLeftSleeping);
	timeSec	= EigrpPortGetTimeSec();
	if(timer->time <= timeSec){
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerLeftSleeping);
		return 0;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerLeftSleeping);
	
	return(timer->time - timeSec) * 1000;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerIsParent

	Desc:	This function is find out if an eigrp managed timer is parent timer.
		
	Para: 	timer	- eigrp managed timer
	
	Ret:		TRUE 	for the timer is a parent timer;
			FALSE 	for it is not.
************************************************************************************/

S32	EigrpUtilMgdTimerIsParent(EigrpMgdTimer_st *timer)
{
	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerIsParent);
	if(BIT_TEST(timer->flag, EIGRP_MGDF_SUBTREE)){
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerIsParent);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerIsParent);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerInitParent

	Desc:	This function is to initialize an eigrp parent timer.
		
	Para: 	child		- the child timer 
			parent	- the parent timer
			callbackfuc	- callback function when time is out
			time_data	- pointer to the data of child timer's systimer
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMgdTimerInitParent(EigrpMgdTimer_st *child,  EigrpMgdTimer_st *parent, void (*callbackfuc)(void *),
										void *time_data)
{
	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerInitParent);
	EIGRP_TRC(DEBUG_EIGRP_TIMER, "TIMERS: Init parent timers : parent %d, child %d\n", (U32)parent, (U32)child);

	child->flag = EIGRP_MGDF_SUBTREE;
	if(parent){
		child->next		= parent->subTree;
		parent->subTree	= child;
	}else{
		EIGRP_ASSERT(!child->sysTimer);
		child->sysTimer	= EigrpPortTimerCreate(callbackfuc, time_data, 1);
		child->context	= callbackfuc;
	}
	child->leaf		= (EigrpMgdTimer_st*) 0;
	child->subTree	= (EigrpMgdTimer_st*) 0;
	child->parent	= parent;
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerInitParent);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerInitLeaf

	Desc:	This function is to init an eigrp leaf timer.
		
	Para: 	child		- the child timer 
			parent	- the parent timer
			type		- the type of child timer
			context	- pointer to the extra data of child leaf timer
			create	- unused
	
	Ret:		NONE		
************************************************************************************/

void	EigrpUtilMgdTimerInitLeaf(EigrpMgdTimer_st *child, EigrpMgdTimer_st *parent, S32 type, void *context, S32 create)
{
	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerInitLeaf);
	EIGRP_TRC(DEBUG_EIGRP_TIMER, "TIMERS: Init leaf timers : parent %d, child %d timer-type %s\n",
				(U32)parent, (U32)child, EigrpTimerType[type]);
	child->flag		= EIGRP_MGDF_LEAF;
	child->type		= type;
	child->context	= context;
	child->next		= parent->leaf;
	parent->leaf		= child;
	child->parent	= parent;
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerInitLeaf);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerDelete

	Desc:	This function is to delete an eigrp managed timer.
		
	Para: 	timer	- the managed timer to be deleted
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMgdTimerDelete(EigrpMgdTimer_st *timer)
{
	EigrpMgdTimer_st *parent, *tmptimer;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerDelete);
	EIGRP_TRC(DEBUG_EIGRP_TIMER, "TIMERS: Delete timer : %d timer-type %s\n",
				(U32)timer, EigrpTimerType[timer->type]);

	if(BIT_TEST(timer->flag, EIGRP_MGDF_START) && BIT_TEST(timer->flag, EIGRP_MGDF_LEAF)){
		EigrpUtilMgdTimerStop(timer);
	}

	BIT_RESET(timer->flag, EIGRP_MGDF_START);
	parent = timer->parent;
	if(!parent){
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerDelete);
		return;
	}

	if(BIT_TEST(timer->flag, EIGRP_MGDF_LEAF)){
		if(parent->leaf == timer){
			parent->leaf = timer->next;
			EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerDelete);
			return;
		}
		tmptimer = parent->leaf;
	}else{
		if(parent->subTree == timer){
			parent->subTree = timer->next;
			EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerDelete);
			return;
		}
		tmptimer = parent->subTree;
	}
	while(tmptimer && (tmptimer->next != timer)){
		tmptimer = tmptimer->next;
	}
	if(tmptimer && (tmptimer->next == timer)){
		tmptimer->next = timer->next;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerDelete);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerStart

	Desc:	This function is activate an leaf timer and the timing will start after a given length of time.
		
	Para: 	timer	- the eigrp managed timer to be started
			delay	- lingering time for starting the timer
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMgdTimerStart(EigrpMgdTimer_st *timer, U32 delay)
{
	U32	timeSec;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerStart);
	timeSec	= EigrpPortGetTimeSec();

	EIGRP_ASSERT((U32)timer);

	EIGRP_TRC(DEBUG_EIGRP_TIMER, "TIMERS: Start timer : %d timer-type %s\n",
				(U32)timer, EigrpTimerType[ timer->type ]);
	/* Set the flag */
	BIT_SET(timer->flag, EIGRP_MGDF_START);

	/* Get the delay and the expire time, at least delay 1 sec */
	delay = (delay + 500) / EIGRP_MSEC_PER_SEC;

	if(!delay){
		delay++;
	}

	timer->time = timeSec + delay;
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerStart);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerStop

	Desc:	This function is to stop an eigrp managed timer.
		
	Para: 	timer	- the eigrp managed timer to be stoped
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMgdTimerStop(EigrpMgdTimer_st *timer)
{
	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerStop);
	if(!EigrpUtilMgdTimerRunning(timer)){
		EIGRP_TRC(DEBUG_EIGRP_TIMER, "trying to stop a non-started timer !\n");
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerStop);
		return;
	}
	EIGRP_TRC(DEBUG_EIGRP_TIMER, "TIMERS: Stop timer : %d timer-type %s\n",
				(U32)timer, EigrpTimerType[ timer->type ]);
	BIT_RESET(timer->flag, EIGRP_MGDF_START);
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerStop);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerExpired

	Desc:	This function is to find out if there is some leaf timers expired on the given timer tree.
		
	Para: 	timer	- an eigrp managed timer
	
	Ret:		TRUE 	for there is some leaf timers expired on the given timer tree;
			FALSE 	for there is not; 
************************************************************************************/

S32	EigrpUtilMgdTimerExpired(EigrpMgdTimer_st *timer)
{
	EigrpMgdTimer_st *tmptimer;
	U32	timeSec;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerExpired);
	timeSec	= EigrpPortGetTimeSec();

	if(BIT_TEST(timer->flag, EIGRP_MGDF_LEAF)){
		if(timer->time <= timeSec){
			EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerExpired);
			return TRUE;
		}else{
			EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerExpired);
			return FALSE;
		}
	}

	for(tmptimer = timer->leaf; tmptimer; tmptimer = tmptimer->next){
		if((tmptimer->time <= timeSec)
			&& BIT_TEST(tmptimer->flag, EIGRP_MGDF_START)){
			EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerExpired);
			return TRUE;
		}
	}
	for(tmptimer = timer->subTree; tmptimer; tmptimer = tmptimer->next){
		if(EigrpUtilMgdTimerExpired(tmptimer)){
			EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerExpired);
			return TRUE;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerExpired);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerFirstExpired

	Desc:	This function is to return the first expired timer on the given timer tree.
		
	Para: 	timer	- an eigrp managed timer
	
	Ret:		the first expired timer on the given timer tree or NULL if the first expired timer
			is not exist or its time is out 
************************************************************************************/

EigrpMgdTimer_st*EigrpUtilMgdTimerFirstExpired(EigrpMgdTimer_st *timer)
{
	EigrpMgdTimer_st *firsttimer;
	U32	timeSec;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerFirstExpired);
	firsttimer = EigrpUtilMgdTimerMinExpired(timer);

	timeSec	= EigrpPortGetTimeSec();

	if(firsttimer && (firsttimer->time <= timeSec)){
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerFirstExpired);
		return firsttimer;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerFirstExpired);
	
	return NULL;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerMinExpired

	Desc:	This function is to return the first timer on the given timer tree in spite of whether it expired.
		
	Para: 	timer	- an eigrp managed timer
	
	Ret:		the first timer
************************************************************************************/

EigrpMgdTimer_st*EigrpUtilMgdTimerMinExpired(EigrpMgdTimer_st *timer)
{
	EigrpMgdTimer_st *tmptimer, *ttimer, *mintimer;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerMinExpired);
	mintimer = (EigrpMgdTimer_st*) 0;

	for(tmptimer = timer->leaf; tmptimer; tmptimer = tmptimer->next){
		if(!BIT_TEST(tmptimer->flag, EIGRP_MGDF_START)){
			continue;
		}
		if(!mintimer){
			mintimer = tmptimer;
		}else{
			if(tmptimer->time < mintimer->time){
				mintimer = tmptimer;
			}
		}
	}

	for(tmptimer = timer->subTree; tmptimer; tmptimer = tmptimer->next){
		ttimer = EigrpUtilMgdTimerMinExpired(tmptimer);
		if(ttimer){
			if(!mintimer){
				mintimer = ttimer;
			}else{
				if(ttimer->time < mintimer->time){
					mintimer = ttimer;
				}
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerMinExpired);
	
	return mintimer;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerSetExptime

	Desc:	This function is to set the expire time for the given timer.
		
	Para: 	timer	- an eigrp managed timer
			exptime	- the expired time when timer is started
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMgdTimerSetExptime(EigrpMgdTimer_st *timer, U32 exptime)
{
	U32 delay;
	U32	timeSec;

	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerSetExptime);
	if(!BIT_TEST(timer->flag, EIGRP_MGDF_LEAF)){
		EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerSetExptime);
		return;
	}

	timeSec	= EigrpPortGetTimeSec();

	if(exptime <= timeSec){
		delay = 0;
	}else{
		delay = exptime - timeSec;
	}

	EigrpUtilMgdTimerStart(timer, delay * EIGRP_MSEC_PER_SEC);
	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerSetExptime);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilMgdTimerStartJittered

	Desc:	This function is to set the expire time for the given timer.
		
	Para: 	timer	- an eigrp managed timer
			interval	- lingering time for starting the timer 	
			
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMgdTimerStartJittered(EigrpMgdTimer_st *timer, U32 interval, S32 jitter)
{
	EIGRP_FUNC_ENTER(EigrpUtilMgdTimerStartJittered);
	
	EigrpUtilMgdTimerStart(timer, interval);

	EIGRP_FUNC_LEAVE(EigrpUtilMgdTimerStartJittered);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilInNetof

	Desc:	This function is return the natural subnet address of the given ip address.
		
	Para: 	in	- ip address
	
	Ret:		the natural subnet address of the given ip address
************************************************************************************/

U32	EigrpUtilInNetof(U32 in)
{
	U32 i = NTOHL(in);
	U32 net;

	EIGRP_FUNC_ENTER(EigrpUtilInNetof);
	if(EIGRP_IN_CLASSA(i)){
		net = i & EIGRP_IN_CLASSA_NET;
	}else if(EIGRP_IN_CLASSB(i)){
		net = i & EIGRP_IN_CLASSB_NET;
	}else if(EIGRP_IN_CLASSC(i)){
		net = i & EIGRP_IN_CLASSC_NET;
	}else if(EIGRP_IN_CLASSD(i)){
		net = i & EIGRP_IN_CLASSD_NET;
	}else{
		EIGRP_FUNC_LEAVE(EigrpUtilInNetof);
		return(0);
	}
	EIGRP_FUNC_LEAVE(EigrpUtilInNetof);

	return(net);
}

/************************************************************************************

	Name:	EigrpUtilNetHash

	Desc:	This function is to get the hash value of the given ip address.
		
	Para: 	ipaddr	- ip address
	
	Ret:		the hash value of the given ip address
************************************************************************************/

U32	EigrpUtilNetHash(U32 ipaddr)
{
	U32 ulNum;

	EIGRP_FUNC_ENTER(EigrpUtilNetHash);
	ulNum = EigrpUtilInNetof(ipaddr);
	if(!ulNum){
		EIGRP_FUNC_LEAVE(EigrpUtilNetHash);
		return 0;
	}
	while((ulNum & 0xff) == 0){
		ulNum >>= 8;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilNetHash);

	return(EIGRP_NET_HASH_MOD(ulNum));
}

/************************************************************************************

	Name:	EigrpUtilChunkMalloc

	Desc:	This function is to alloc a big block of memory for the given chunk.
		
	Para: 	chunks	- pointer to the given chunk
	
	Ret:		pointer to the new memory. If failed, return NULL		
************************************************************************************/

void *EigrpUtilChunkMalloc(EigrpChunk_st *chunks)
{
	void *quelem = (void *) 0;

	EIGRP_FUNC_ENTER(EigrpUtilChunkMalloc);
	if(chunks && (chunks->queLen < chunks->maxLen)){
		quelem = EigrpPortMemMalloc(chunks->size);
		if(!quelem){	 
			EIGRP_FUNC_LEAVE(EigrpUtilChunkMalloc);
			return NULL;
		}
	}else{
		EIGRP_ASSERT((U32)0);
		EIGRP_FUNC_LEAVE(EigrpUtilChunkMalloc);
		return NULL;
	}
	if(!quelem){
		EIGRP_ASSERT((U32)0);
	}else{
		(void)EigrpUtilMemZero(quelem, chunks->size);
		chunks->queLen ++;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilChunkMalloc);

	return quelem;
}

/************************************************************************************

	Name:	EigrpUtilChunkFree

	Desc:	This function is to free a chunk memory.
		
	Para: 	chunks	-pointer to the given chunk
			freep	- the memory to be freed
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilChunkFree(EigrpChunk_st *chunks, void *freep)
{
	EIGRP_FUNC_ENTER(EigrpUtilChunkFree);
	if(chunks && chunks->queLen){
		EigrpPortMemFree(freep);
		freep = 0;
		chunks->queLen--;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilChunkFree);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilChunkCreate

	Desc:	This function is to create a chunk.
		
	Para: 	size		- chunks' size
			maxlen	- chunks' maxLen
	
	Ret:		the new chunk
************************************************************************************/

EigrpChunk_st *EigrpUtilChunkCreate(U32 size, U16 maxlen, U16 flag, void *context, U16 len, S8 *chunkname)
{
	EigrpChunk_st *chunks;

	EIGRP_FUNC_ENTER(EigrpUtilChunkCreate);
	chunks = (EigrpChunk_st *)EigrpPortMemMalloc(sizeof(EigrpChunk_st));
	if(!chunks){	 
		EIGRP_FUNC_LEAVE(EigrpUtilChunkCreate);
		return NULL;
	}
	chunks->size	= size;	 
	chunks->maxLen	= maxlen;
	chunks->queLen	= 0;
	EIGRP_FUNC_LEAVE(EigrpUtilChunkCreate);
	
	return chunks;
}

/************************************************************************************

	Name:	EigrpUtilQueGetById

	Desc:	This function is to get a point to a queue, given the queue index.
		
	Para: 	qid		- the queue index
	
	Ret:		the queue whose index is equal to the given queue index, or NULL if nothing is 
			found.
************************************************************************************/

EigrpQue_st *EigrpUtilQueGetById(U32 qid)
{
	EigrpQue_st	*x;

	EIGRP_FUNC_ENTER(EigrpUtilQueGetById);
	if(!gpEigrp->queLst){
		EIGRP_FUNC_LEAVE(EigrpUtilQueGetById);
		return NULL;
	}

	for(x = gpEigrp->queLst; x; x = x->next){
		if(x->id == qid){
			EIGRP_FUNC_LEAVE(EigrpUtilQueGetById);
			return x;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpUtilQueGetById);
	
	return NULL;
}

/************************************************************************************

	Name:	EigrpUtilQueCreate

	Desc:	This function is to create a new queue.
		
	Para: 	QueueLen	- the length of the new queue
			qid			- pointer to the index of the new queue
	
	Ret:		0, if failed, return 1
************************************************************************************/

S32	EigrpUtilQueCreate(U32 QueueLen, U32 *qid)
{
	EigrpQue_st	*q;
	U32			id;

	EIGRP_FUNC_ENTER(EigrpUtilQueCreate);
	EigrpPortSemBTake(gpEigrp->semSyncQ); 

	/* fetch a lowest available id */
	if(gpEigrp->queLst){
		id = gpEigrp->queId;
		do{  	
		      for(q=gpEigrp->queLst; q; q = q->next){
				if(q->id==id){
					break;
				} 
		      }
		      if(!q){
			  	break;
		      	}
		     	id++;
		}while(1);
	}else{ 
		id = gpEigrp->queId = 0;
	}

	/* get a new queue head */
	q = EigrpPortMemMalloc(sizeof(EigrpQue_st));
	if(!q){
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueCreate);
		return 1;
	}
	EigrpUtilMemZero((void *)q, sizeof(EigrpQue_st));

	gpEigrp->queId = q->id = id;
	gpEigrp->queId ++;
	q->MaxLen = QueueLen;

	if(!gpEigrp->queLst){
		gpEigrp->queLst = q;
	}else{
		q->next = gpEigrp->queLst;
		gpEigrp->queLst->prev = q;
		gpEigrp->queLst = q;
	}

	*qid = id;

	EigrpPortSemBGive(gpEigrp->semSyncQ); 
	EIGRP_FUNC_LEAVE(EigrpUtilQueCreate);
	
	return 0;
}

/************************************************************************************

	Name:	EigrpUtilQueDelete

	Desc:	This function is to delete a queue.
		
	Para: 	qid		- the deleting queue index
	
	Ret:		0, if failed, return 1
************************************************************************************/

S32	EigrpUtilQueDelete(U32 qid)
{
	EigrpQue_st	*q;

	EIGRP_FUNC_ENTER(EigrpUtilQueDelete);
	EigrpPortSemBTake(gpEigrp->semSyncQ); 
	q = EigrpUtilQueGetById(qid);

	if(!q){
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueDelete);
		return 1;
	}

	/* if the queue's count is not 0, delete the all queue unit */
	if(q->count){
		EigrpQueElem_st	*x;
		while(q->head){
			x = q->head;
			q->head = q->head->next;
			EigrpPortMemFree(x);
		}
	}

	/*dequeue this queue head */
	if(gpEigrp->queLst == q){
		gpEigrp->queLst = gpEigrp->queLst->next;
		if(gpEigrp->queLst){
			gpEigrp->queLst->prev = NULL;
		}
	}else{
		EIGRP_ASSERT((U32)q->prev);
		q->prev->next = q->next;
		if(q->next){
			q->next->prev = q->prev;
		}
	}

	gpEigrp->queId = qid;

	EigrpPortMemFree(q);

	EigrpPortSemBGive(gpEigrp->semSyncQ); 
	EIGRP_FUNC_LEAVE(EigrpUtilQueDelete);
	
	return 0;
}

/************************************************************************************

	Name:	EigrpUtilQueGetMsgNum

	Desc:	This function is to get elem number of a given queue.
		
	Para: 	id		- the given queue index
			msgnum	- pointer to the elem number of a given queue
			
	Ret:		0, if failed, return 1
************************************************************************************/

S32	EigrpUtilQueGetMsgNum(U32 id, U32 *msgnum)
{
	EigrpQue_st	*x;

	EIGRP_FUNC_ENTER(EigrpUtilQueGetMsgNum);
	EigrpPortSemBTake(gpEigrp->semSyncQ); 

	x = EigrpUtilQueGetById(id);

	if(x){
		*msgnum = x->count;
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueGetMsgNum);
		return 0;
	}else{
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		*msgnum = 0;
		EIGRP_FUNC_LEAVE(EigrpUtilQueGetMsgNum);
		return 1;
	}
}

/************************************************************************************

	Name:	EigrpUtilQueFetchElem

	Desc:	This function is to get one elem from a given queue.
		
	Para: 	id		- the given queue index
			msgbuf	- pointer to the data of the elem
	
	Ret:		0, if failed, return 1
************************************************************************************/

S32	EigrpUtilQueFetchElem(U32 id, U32 *msgbuf)
{
	EigrpQue_st	*x;
	EigrpQueElem_st	*xunit;

	EIGRP_FUNC_ENTER(EigrpUtilQueFetchElem);
	EigrpPortSemBTake(gpEigrp->semSyncQ); 

	x = EigrpUtilQueGetById(id);

	/* no such queue or no queue unit on earth */
	if(!x || !x->head){
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueFetchElem);
		return 1;
	}

	/* get the unit and dequeue the unit first */
	xunit	= x->head;
	x->head	= x->head->next;
	x->count--;

	EigrpPortMemCpy((S8 *)msgbuf,xunit->data, EIGRP_QUEUE_PARA_SIZE);
	EigrpPortMemFree(xunit);

	EigrpPortSemBGive(gpEigrp->semSyncQ); 
	EIGRP_FUNC_LEAVE(EigrpUtilQueFetchElem);
	
	return 0;
}

/************************************************************************************

	Name:	EigrpUtilQueWriteElem

	Desc:	This function is to send one elem to a given queue.
		
	Para: 	id		- the given queue index
			msgbuf	- pointer to the data of elem
			
	Ret:		0, if failed, return 1
************************************************************************************/

S32	EigrpUtilQueWriteElem(U32 id, U32 *msgbuf)
{
	EigrpQue_st	*x;
	EigrpQueElem_st	*xunit, *u, *up;

	EIGRP_FUNC_ENTER(EigrpUtilQueWriteElem);
	EigrpPortSemBTake(gpEigrp->semSyncQ); 
	x = EigrpUtilQueGetById(id);

	if(!x){
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueWriteElem);
		return 1;
	}

	/* check the limit */
	if(x->count >= x->MaxLen){
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueWriteElem);
		return 1;
	}

	/* get a unit with copyed data */
	xunit = (EigrpQueElem_st*)EigrpPortMemMalloc(sizeof(EigrpQueElem_st));
	if(!xunit){	 
		EIGRP_FUNC_LEAVE(EigrpUtilQueWriteElem);
		return 0;
	}
	EigrpUtilMemZero((void *)xunit, sizeof(EigrpQueElem_st));
	EigrpPortMemCpy(xunit->data, (S8 *)msgbuf, EIGRP_QUEUE_PARA_SIZE);

	/* enqueue the unit */
	if(!x->head){
		x->head = xunit;
		x->count ++;
		EigrpPortSemBGive(gpEigrp->semSyncQ); 
		EIGRP_FUNC_LEAVE(EigrpUtilQueWriteElem);
		return 0;
	}
	/* go through the queue and get the last unit into up */
	up = NULL;
	for(u=x->head; u; u=u->next){
		up = u;
	}
	
	/* append the unit into the tail */
	up->next = xunit;
	x->count ++;

	EigrpPortSemBGive(gpEigrp->semSyncQ); 
	EIGRP_FUNC_LEAVE(EigrpUtilQueWriteElem);
	
	return 0;
}

/************************************************************************************

	Name:	EigrpUtilQue2Init

	Desc:	This function is to initialize a simple queue.
		
	Para: 	NONE
	
	Ret:		pointer of the queue
************************************************************************************/

EigrpQue_st *EigrpUtilQue2Init()
{
	EigrpQue_st *pHead;

	EIGRP_FUNC_ENTER(EigrpUtilQue2Init);
	pHead	= EigrpPortMemMalloc(sizeof(EigrpQue_st));
	if(!pHead){	 
		EIGRP_FUNC_LEAVE(EigrpUtilQue2Init);
		return NULL;
	}
	
	pHead->head	= NULL;
	pHead->count	= 0;

	pHead->lock	= EigrpPortSemBCreate(TRUE);
	EIGRP_FUNC_LEAVE(EigrpUtilQue2Init);
	
	return pHead;
}

/************************************************************************************

	Name:	EigrpUtilQue2Enqueue

	Desc:	This function is send an elem into a simple queue, given a pointer to the queue.
		
	Para: 	pHead	- pointer to the queue
			pElem	- pointer to the elem
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilQue2Enqueue(EigrpQue_st *pHead, EigrpQueElem_st *pElem)
{
	EigrpQueElem_st	*pTem;
	
	EIGRP_FUNC_ENTER(EigrpUtilQue2Enqueue);
	EigrpPortSemBTake(pHead->lock);

	pTem	= pHead->head;
	if(!pTem){
		pElem->next		= pHead->head;
		pHead->head	= pElem;
	}else{
		while(pTem->next){
			pTem = pTem->next;
		}

		pElem->next		= pTem->next;
		pTem->next		= pElem;
	}
	pHead->count++;

	EigrpPortSemBGive(pHead->lock);
	EIGRP_FUNC_LEAVE(EigrpUtilQue2Enqueue);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilQue2Unqueue

	Desc:	This function is delete one given elem from the given queue.
		
	Para: 	pHead	- pointer to the queue
			pElem	- pointer to the elem
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilQue2Unqueue(EigrpQue_st *pHead, EigrpQueElem_st *pElem)
{
	EigrpQueElem_st	*pTem;

	EIGRP_FUNC_ENTER(EigrpUtilQue2Unqueue);
	EigrpPortSemBTake(pHead->lock);
	
	EigrpPortAssert((U32)(pHead->head && pHead->count), "");
	if(pElem == pHead->head){
		pHead->head	= pElem->next;
		pHead->count--;

		EigrpPortSemBGive(pHead->lock);
		EIGRP_FUNC_LEAVE(EigrpUtilQue2Unqueue);
		return;
	}
	
	for(pTem = pHead->head; pTem; pTem = pTem->next){
		if(pTem->next == pElem){
			break;
		}
	}

	EigrpPortAssert((U32)pTem, "");

	if(!pTem){	
		EigrpPortSemBGive(pHead->lock);
		EIGRP_FUNC_LEAVE(EigrpUtilQue2Unqueue);
		return;
	}
	pTem->next	= pElem->next;
	pHead->count--;

	EigrpPortSemBGive(pHead->lock);
	EIGRP_FUNC_LEAVE(EigrpUtilQue2Unqueue);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilQue2Dequeue

	Desc:	This function is to get one elem from the given simple queue.
		
	Para: 	pHead	- pointer to the queue
			
	Ret:		pointer to the elem
************************************************************************************/

EigrpQueElem_st	*EigrpUtilQue2Dequeue(EigrpQue_st *pHead)
{
	EigrpQueElem_st	*pElem;
	
	EIGRP_FUNC_ENTER(EigrpUtilQue2Dequeue);
	EigrpPortSemBTake(pHead->lock);

	pElem	= pHead->head;
	if(!pElem){
		EigrpPortSemBGive(pHead->lock);
		EIGRP_FUNC_LEAVE(EigrpUtilQue2Dequeue);
		return NULL;
	}

	EIGRP_ASSERT(pHead->count);
	pHead->head	= pElem->next;
	
	pHead->count--;
	
	EigrpPortSemBGive(pHead->lock);
	EIGRP_FUNC_LEAVE(EigrpUtilQue2Dequeue);
	
	return pElem;
}

/************************************************************************************

	Name:	EigrpUtilSchedInit

	Desc:	This function is to initialize eigrp sched data.
		
	Para: 	NONE	
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilSchedInit()
{
	EigrpPortAssert((U32)!gpEigrp->schedLst, "");

	EIGRP_FUNC_ENTER(EigrpUtilSchedInit);
	gpEigrp->schedLst	= (EigrpSched_pt)EigrpPortMemMalloc(sizeof(EigrpSched_st));
	if(!gpEigrp->schedLst){	 
		EIGRP_FUNC_LEAVE(EigrpUtilSchedInit);
		return;
	}
	EigrpPortMemSet((U8 *)gpEigrp->schedLst, 0, sizeof(EigrpSched_st));
	gpEigrp->schedLst->sem	= EigrpPortSemBCreate(TRUE);
	EIGRP_FUNC_LEAVE(EigrpUtilSchedInit);

	return;
}

/************************************************************************************

	Name:	EigrpUtilSchedClean

	Desc:	This function is to clean all the eigrp jobs and free eigrp sched data.
		
	Para: 	NONE
		
	Ret:		NONE	
************************************************************************************/

void	EigrpUtilSchedClean()
{
	EigrpSched_pt		pTem;
	
	EIGRP_FUNC_ENTER(EigrpUtilSchedClean);
	EigrpPortAssert((U32)gpEigrp->schedLst, "");
	EigrpPortSemBTake(gpEigrp->schedLst->sem);

	while(1){
		EigrpPortSemBGive(gpEigrp->schedLst->sem);
		pTem	= EigrpUtilSchedGet();
		EigrpPortSemBTake(gpEigrp->schedLst->sem);

		if(!pTem){
			break;
		}

		EigrpPortMemFree(pTem);
	}

	pTem	= gpEigrp->schedLst;
	gpEigrp->schedLst	= NULL;
	
	EigrpPortSemBDelete(gpEigrp->schedLst->sem);
	gpEigrp->schedLst->sem	= NULL;
	
	EigrpPortMemFree(pTem);
	EIGRP_FUNC_LEAVE(EigrpUtilSchedClean);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilSchedAdd

	Desc:	This function is to create an eigrp sched data and insert it into the job list.
		
	Para: 	func		- pointer to the callback function of the new sched
			param	- pointer to the data of the new sched
	
	Ret:		pointer to the new sched
************************************************************************************/

EigrpSched_pt	EigrpUtilSchedAdd(S32 (*func)(void *), void *param)
{
	EigrpSched_pt		pSched, pPrev;
	
	EIGRP_FUNC_ENTER(EigrpUtilSchedAdd);
	EigrpPortAssert((U32)gpEigrp->schedLst, "");
	EigrpPortSemBTake(gpEigrp->schedLst->sem);

	pSched			= (EigrpSched_pt)EigrpPortMemMalloc(sizeof(EigrpSched_st));
	if(!pSched){	 
		EIGRP_FUNC_LEAVE(EigrpUtilSchedAdd);
		return NULL;
	}
	EigrpPortMemSet((U8 *)pSched, 0, sizeof(EigrpSched_st));

	pSched->func		= func;
	pSched->data		= param;
	
	if(!gpEigrp->schedLst->forw){
		/* I am the first one.*/
		EigrpPortAssert((U32)!gpEigrp->schedLst->count, "");

		gpEigrp->schedLst->forw	= pSched;
	}else{
		EigrpPortAssert((U32)gpEigrp->schedLst->count, "");

		pPrev	= gpEigrp->schedLst->forw;
		while(pPrev->forw){
			pPrev	= pPrev->forw;
		}
		
		pPrev->forw			= pSched;
	}

	gpEigrp->schedLst->count++;
	EigrpPortSemBGive(gpEigrp->schedLst->sem);
	EIGRP_FUNC_LEAVE(EigrpUtilSchedAdd);

	return pSched;
}

/************************************************************************************

	Name:	EigrpUtilSchedGet

	Desc:	This function is to get one eigrp job from the job list.
		
	Para: 	NONE
	
	Ret:		pointer to the eigrp job 
************************************************************************************/

EigrpSched_pt	EigrpUtilSchedGet()
{
	EigrpSched_pt		pSched;
	
	EIGRP_FUNC_ENTER(EigrpUtilSchedGet);
	EigrpPortAssert((U32)gpEigrp->schedLst, "");
	EigrpPortSemBTake(gpEigrp->schedLst->sem);

	pSched	= NULL;
	if(!gpEigrp->schedLst->forw){
		EigrpPortAssert((U32)!gpEigrp->schedLst->count, "");
	}else{
		EigrpPortAssert((U32)gpEigrp->schedLst->count, "");

		gpEigrp->schedLst->count--;
		pSched	= gpEigrp->schedLst->forw;
		
		gpEigrp->schedLst->forw	= pSched->forw;
	}

	EigrpPortSemBGive(gpEigrp->schedLst->sem);
	EIGRP_FUNC_LEAVE(EigrpUtilSchedGet);
	
	return pSched;
}

/************************************************************************************

	Name:	EigrpUtilSchedCancel

	Desc:	This function is find the given sched in the eigrp job list and get rid of it.
		
	Para: 	pSched	- pointer to the sched  to be got rid of 
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilSchedCancel(EigrpSched_pt pSched)
{
	EigrpSched_pt		pPrev;
	
	EIGRP_FUNC_ENTER(EigrpUtilSchedCancel);
	EigrpPortAssert((U32)gpEigrp->schedLst, "");
	EigrpPortSemBTake(gpEigrp->schedLst->sem);

	if(gpEigrp->schedLst->forw == pSched){
		gpEigrp->schedLst->forw	= pSched->forw;
	
	}else{
		/* Just to make sure that this sched is in the list */
		for(pPrev = gpEigrp->schedLst->forw; pPrev && pPrev->forw != pSched; pPrev = pPrev->forw){
			;
		}
		EigrpPortAssert((U32)pPrev, "");	/* This line is very strict. So it can be changed.*/
		
		pPrev->forw	= pSched->forw;
	}

	EigrpPortMemFree(pSched);
	gpEigrp->schedLst->count--;
	
	EigrpPortSemBGive(gpEigrp->schedLst->sem);
	EIGRP_FUNC_LEAVE(EigrpUtilSchedCancel);
	
	return;
}

U8	EigrpIpStr[32][32];	
/************************************************************************************

	Name:	EigrpUtilIp2Str

	Desc:	This function is to change an ip address to a ascii string.
		
	Para: 	ipAddr		- ip address
	
	Ret:		pointer to the ascii string
************************************************************************************/

U8	*EigrpUtilIp2Str(U32 ipAddr)
{
	static U32 IpStrCount = 0;
	
	EIGRP_FUNC_ENTER(EigrpUtilIp2Str);
	IpStrCount = (IpStrCount + 1) % 16;

	sprintf(&EigrpIpStr[IpStrCount][0], "%d.%d.%d.%d", (ipAddr & 0xff000000) >> 24,
					(ipAddr & 0xff0000) >> 16, (ipAddr & 0xff00) >> 8, ipAddr & 0xff);
	EIGRP_FUNC_LEAVE(EigrpUtilIp2Str);
	
	return (&EigrpIpStr[IpStrCount][0]);
}


U32	EigrpUtilMask2Len(U32 mask)
{
	U32 cnt;

	cnt	= 0;
	while(mask & 0x80000000){
		cnt ++;

		mask	= mask << 1;
	}
	
	return cnt;
}


U32	EigrpUtilLen2Mask(U32 len)
{
	U32	mask;

	mask = 0;	
	while(len){
		mask	= (mask >> 1) | 0x80000000;
		len --;
	}

	return mask;
}


/************************************************************************************

	Name:	EigrpUtilMemZero

	Desc:	This function is to set all the bytes in the given memory to zero.
		
	Para: 	buff		- pointer to the memory
			length	- the length of the memory
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMemZero(void *buff, U32 length)
{
	EIGRP_FUNC_ENTER(EigrpUtilMemZero);
	EigrpPortMemSet((U8 *)buff, 0, length);
	EIGRP_FUNC_LEAVE(EigrpUtilMemZero);
	
	return;
}

/************************************************************************************

	Name:	EIGRP_ASSERT

	Desc:	This function is to do the assert.
		
	Para: 	val	- a integer ,do assert when it values zero.
	
	Ret:		NONE
************************************************************************************/
/*
void	EIGRP_ASSERT(U32 val)
{
	EIGRP_FUNC_ENTER(EIGRP_ASSERT);
	EigrpPortAssert(val, "");
	EIGRP_FUNC_LEAVE(EIGRP_ASSERT);

	return;
}
*/


/************************************************************************************

	Name:	EigrpUtilIsLbk

	Desc:	This function is to judge if the given ip address is loop back address.
		
	Para: 	ipAddr	- ip address
		
	Ret:		TRUE	for the given ip address is loop back address
			FALSE	for it is not
************************************************************************************/

S32	EigrpUtilIsLbk(U32 ipAddr)
{
	EIGRP_FUNC_ENTER(EigrpUtilIsLbk);
	if((ipAddr & 0xff000000) == 0x7f000000){
		EIGRP_FUNC_LEAVE(EigrpUtilIsLbk);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilIsLbk);

	return FALSE;
}

/************************************************************************************

	Name:	EigrpUtilIsMcast

	Desc:	This function is to judge if the given ip address is multicast address.
		
	Para: 	ipAddr	- ip address
	
	Ret:		TRUE	for the given ip address is multicast address
			FALSE	for it is not
************************************************************************************/

S32	EigrpUtilIsMcast(U32 ipAddr)
{
	EIGRP_FUNC_ENTER(EigrpUtilIsMcast);
	if((ipAddr & 0xf0000000) == 0xe0000000){
		EIGRP_FUNC_LEAVE(EigrpUtilIsMcast);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilIsMcast);

	return FALSE;
}

/************************************************************************************

	Name:	EigrpUtilIsBadClass

	Desc:	This function is to judge if the class of the incoming ip address can be used by eigrp.
		
	Para: 	ipAddr	- ip address
	
	Ret:		TRUE	for the ip address can be used by eigrp
			FALSE	for it is not
************************************************************************************/

S32	EigrpUtilIsBadClass(U32 ipAddr)
{
	EIGRP_FUNC_ENTER(EigrpUtilIsBadClass);
	if((ipAddr & 0xf0000000) == 0xf0000000){
		EIGRP_FUNC_LEAVE(EigrpUtilIsBadClass);
		return TRUE;
	}
	EIGRP_FUNC_LEAVE(EigrpUtilIsBadClass);

	return FALSE;
}

/************************************************************************************

	Name:	EigrpUtilRbXTreeInit

	Desc:	This function is to init one red-black extended tree, given the compare function.
		
	Para: 	TreeX	- pointer to the red-black extended tree
			compare	- pointer to the compare callback function of TreeX
			del		- pointer to the del callback function of TreeX

	Ret:		NONE		
************************************************************************************/

void	EigrpUtilRbXTreeInit(EigrpRbTreeX_pt TreeX, S32 (* compare)(EigrpRbNodeX_pt, EigrpRbNodeX_pt), void(*del)(EigrpRbNodeX_pt)) 
{
	EIGRP_FUNC_ENTER(EigrpUtilRbXTreeInit);
	TreeX->root 			= &TreeX->tail;
	TreeX->tail.rbColorX 	= EigrpRbBlack;	
	TreeX->compare		= compare;
	TreeX->del			= del;
	TreeX->tail.lChildX 	= TreeX->tail.rChildX = TreeX->tail.parentX = &TreeX->tail;
	EIGRP_FUNC_LEAVE(EigrpUtilRbXTreeInit);

	return;
}

/************************************************************************************

	Name:	EigrpUtilRbXTreeDestroy

	Desc:	This function is to destroy the given red-black extended tree.
		
	Para: 	Treex	- pointer to the red-black extended tree 
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilRbXTreeDestroy(EigrpRbTreeX_pt TreeX)
{
	EigrpRbNodeX_pt	pNode;

	EIGRP_FUNC_ENTER(EigrpUtilRbXTreeDestroy);
	while((U32)(pNode = EigrpUtilRbXGetFirst(TreeX))){
		EigrpUtilRbXDelete(TreeX, pNode);
		TreeX->del(pNode);
	}

	EigrpPortMemFree(TreeX);
	EIGRP_FUNC_LEAVE(EigrpUtilRbXTreeDestroy);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilRbXTreeCleanUp

	Desc:	This function is to clean up the given red-black extened tree.
		
	Para: 	Treex	- pointer to the red-black extended tree 
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilRbXTreeCleanUp(EigrpRbTreeX_pt TreeX)
{
	EigrpRbNodeX_pt 	pNode;

	EIGRP_FUNC_ENTER(EigrpUtilRbXTreeCleanUp);
	while((U32)(pNode = EigrpUtilRbXGetFirst(TreeX))){
		EigrpUtilRbXDelete(TreeX, pNode);
		TreeX->del(pNode);
	}
	EIGRP_FUNC_LEAVE(EigrpUtilRbXTreeCleanUp);

	return;
}

/************************************************************************************

	Name:	EigrpUtilRbXInsert

	Desc:	This function is to insert a node to the given rbX tree.
		
	Para: 	TreeX	- pointer to the rbX tree
			inNd		- pointer to the node to be inserted
	
	Ret:		1	for it is inserted
			0	for it is not so
************************************************************************************/

S32	EigrpUtilRbXInsert(EigrpRbTreeX_pt TreeX, EigrpRbNodeX_pt inNd)
{
	EigrpRbNodeX_pt 	x, y,z;
	S32       		c;

	EIGRP_FUNC_ENTER(EigrpUtilRbXInsert);

	y = &TreeX->tail;
	x = TreeX->root;
	while (x != &TreeX->tail){
		y = x;
		c = TreeX->compare(inNd, x);
		if (c == 0){
			EIGRP_FUNC_LEAVE(EigrpUtilRbXInsert);
			return 0;
		}
		if (c < 0){
			x = x->lChildX;
		}else{
			x = x->rChildX;
		}
	}
	
	inNd->parentX 	= y;
	inNd->lChildX 	= inNd->rChildX = &TreeX->tail;
	if (y == &TreeX->tail){
		TreeX->root = inNd;
	}else{
		c = TreeX->compare(inNd, y);
		if (c < 0){
			y->lChildX = inNd;
		}else{
			y->rChildX = inNd;
		}
	}
	
	inNd->rbColorX = EigrpRbRed;
	while (inNd != TreeX->root && inNd->parentX->rbColorX == EigrpRbRed) {
		EigrpPortAssert(inNd != &TreeX->tail, "");
		EigrpPortAssert(inNd->parentX != &TreeX->tail, "");
		EigrpPortAssert(inNd->parentX->parentX != &TreeX->tail, "");
		
		if (inNd->parentX == inNd->parentX->parentX->lChildX){
			z = inNd->parentX->parentX->rChildX;
			if (z != &TreeX->tail && z->rbColorX == EigrpRbRed){
				inNd->parentX->rbColorX 			= EigrpRbBlack;
				z->rbColorX 						= EigrpRbBlack;
				inNd->parentX->parentX->rbColorX = EigrpRbRed;
				inNd 							= inNd->parentX->parentX;
			}else{
				if (inNd == inNd->parentX->rChildX) {
					inNd = inNd->parentX;
					EigrpUtilRbXLeftRotate(TreeX, inNd);
				}				
				inNd->parentX->rbColorX 			= EigrpRbBlack;
				inNd->parentX->parentX->rbColorX 	= EigrpRbRed;
				EigrpUtilRbXRightRotate(TreeX, inNd->parentX->parentX);
			}
		}else{
			z = inNd->parentX->parentX->lChildX;
			if (z != &TreeX->tail && z->rbColorX == EigrpRbRed) {
				inNd->parentX->rbColorX 			= EigrpRbBlack;
				z->rbColorX 						= EigrpRbBlack;
				inNd->parentX->parentX->rbColorX 	= EigrpRbRed;
				inNd 							= inNd->parentX->parentX;
			}else{
				if (inNd == inNd->parentX->lChildX) {
					inNd = inNd->parentX;
					EigrpUtilRbXRightRotate(TreeX, inNd);
				}				
				inNd->parentX->rbColorX 			= EigrpRbBlack;
				inNd->parentX->parentX->rbColorX = EigrpRbRed;
				EigrpUtilRbXLeftRotate(TreeX, inNd->parentX->parentX);
			}
		}
	}
	TreeX->root->rbColorX = EigrpRbBlack;
	EIGRP_FUNC_LEAVE(EigrpUtilRbXInsert);

	return 1; /* inNd is inserted */	
}

/************************************************************************************

	Name:	EigrpUtilRbXDelete

	Desc:	This function is to delete a given node from the given rbX tree.
		
	Para: 	TreeX	- pointer to the rbX tree
			inNd		- pointer to the node to be deleted
	
	Ret:		NONE
************************************************************************************/

void  EigrpUtilRbXDelete( EigrpRbTreeX_pt TreeX, EigrpRbNodeX_pt delNd)
{
	EigrpRbNodeX_pt x, y;
	
	EIGRP_FUNC_ENTER(EigrpUtilRbXDelete);

	if(!TreeX || !delNd){
		EIGRP_FUNC_LEAVE(EigrpUtilRbXDelete);
		return;
	}

	EigrpPortAssert(delNd != &TreeX->tail, "");
	EigrpPortAssert(TreeX->tail.rbColorX == EigrpRbBlack, "");
	
	if (delNd->lChildX == &TreeX->tail || delNd->rChildX == &TreeX->tail){
		y = delNd;
	}else{
		y = EigrpUtilRbXGetNext(TreeX,delNd);
	}

	if(!y){
		EIGRP_FUNC_LEAVE(EigrpUtilRbXDelete);
		return;
	}
	EigrpPortAssert(y != &TreeX->tail, "");
	if (y->lChildX != &TreeX->tail){
		x = y->lChildX;
	}else{
		x = y->rChildX;
	}
	
	x->parentX = y->parentX;
	if (y->parentX == &TreeX->tail){
		TreeX->root = x;
	}else {
		EigrpPortAssert(y->parentX != &TreeX->tail, "");
		if (y == y->parentX->lChildX){
			y->parentX->lChildX = x;
		}else{
			y->parentX->rChildX = x;
		}
	}

	if (y->rbColorX == EigrpRbBlack){
		EigrpUtilRbXDeleteFixup(TreeX,x);
	}
	if (y != delNd) {
		if (TreeX->root == delNd){
			TreeX->root = y;
		}
		y->parentX 	= delNd->parentX;
		y->lChildX 	= delNd->lChildX;
		y->rChildX 	= delNd->rChildX;
		y->rbColorX 	= delNd->rbColorX;
		if (y->parentX != &TreeX->tail)
			if (y->parentX->lChildX == delNd){
				y->parentX->lChildX = y;
			}else{ 
				y->parentX->rChildX = y;
			}
			if (y->lChildX != &TreeX->tail) {
				EigrpPortAssert(y->lChildX->parentX == delNd, "");
				y->lChildX->parentX = y;
			}
			if (y->rChildX != &TreeX->tail){
				y->rChildX->parentX = y;
			}
		}
	delNd->lChildX=delNd->rChildX=delNd->parentX=&TreeX->tail;
	EigrpPortAssert(TreeX->tail.rbColorX == EigrpRbBlack, "");
	EIGRP_FUNC_LEAVE(EigrpUtilRbXDelete);

	return;
}

/************************************************************************************

	Name:	EigrpUtilRbXDeleteFixup

	Desc:	This function is to delete a given node from the given rbX tree and fix up it.
		
	Para: 	TreeX	- pointer to the rbX tree
			inNd		- pointer to the node to be deleted
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilRbXDeleteFixup(EigrpRbTreeX_pt TreeX, EigrpRbNodeX_pt x)
{
  	EigrpRbNodeX_pt w;
  	
	EIGRP_FUNC_ENTER(EigrpUtilRbXDeleteFixup);
  	while (x != TreeX->root && x->rbColorX == EigrpRbBlack) {
  		EigrpPortAssert(TreeX->tail.rbColorX == EigrpRbBlack, "");
  		EigrpPortAssert(x->parentX != &TreeX->tail, "");
  		
  		if (x == x->parentX->lChildX) {
  			w = x->parentX->rChildX;
  			if (w->rbColorX == EigrpRbRed) {
  				w->rbColorX 			= EigrpRbBlack;
  				x->parentX->rbColorX = EigrpRbRed;
  				EigrpUtilRbXLeftRotate(TreeX,x->parentX);
  				w = x->parentX->rChildX;
  			}
  			if (w == &TreeX->tail){
  				break;
  			}
  			if (w->lChildX->rbColorX == EigrpRbBlack && w->rChildX->rbColorX == EigrpRbBlack) {
  				w->rbColorX 	= EigrpRbRed;
  				x 			= x->parentX;
  			}else{
  				if (w->rChildX->rbColorX == EigrpRbBlack) {
  					w->lChildX->rbColorX 	= EigrpRbBlack;
  					w->rbColorX 			= EigrpRbRed;
  					EigrpUtilRbXRightRotate(TreeX,w);
  					w = x->parentX->rChildX;
  				}

  				w->rbColorX = x->parentX->rbColorX;
  				x->parentX->rbColorX = EigrpRbBlack;
  				w->rChildX->rbColorX 	= EigrpRbBlack;
  				EigrpUtilRbXLeftRotate( TreeX,x->parentX);
  				x = TreeX->root;
  			}
  		}else{
  			w = x->parentX->lChildX;
  			if (w->rbColorX == EigrpRbRed) {
  				w->rbColorX 			= EigrpRbBlack;
  				x->parentX->rbColorX = EigrpRbRed;
  				EigrpUtilRbXRightRotate( TreeX,x->parentX);
  				w = x->parentX->lChildX;
  			}		
  			if (w == &TreeX->tail){
  				break;
  			}
  			if (w->lChildX->rbColorX == EigrpRbBlack && w->rChildX->rbColorX == EigrpRbBlack) {
  				w->rbColorX 	= EigrpRbRed;
  				x 			= x->parentX;
  			}else{
  				if (w->lChildX->rbColorX == EigrpRbBlack) {
  					w->rChildX->rbColorX 	= EigrpRbBlack;
  					w->rbColorX 			= EigrpRbRed;
  					EigrpUtilRbXLeftRotate(TreeX,w);
  					w = x->parentX->lChildX;
  				}			
  				w->rbColorX 			= x->parentX->rbColorX;
  				x->parentX->rbColorX = EigrpRbBlack;
  				w->lChildX->rbColorX 	= EigrpRbBlack;
  				EigrpUtilRbXRightRotate( TreeX,x->parentX);
  				x = TreeX->root;
  			}	
  		}
  	}
  	x->rbColorX = EigrpRbBlack;
	EIGRP_FUNC_LEAVE(EigrpUtilRbXDeleteFixup);

	return;
  }

/************************************************************************************

	Name:	EigrpUtilRbXLeftRotate

	Desc:	This function is to left rotate the given rbX tree.
		
	Para: 	TreeX	- pointer to the given rbX tree
			x	- pointer to the axes node
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilRbXLeftRotate(EigrpRbTreeX_pt TreeX, EigrpRbNodeX_pt x)
{
	EigrpRbNodeX_pt y;
	
	EIGRP_FUNC_ENTER(EigrpUtilRbXLeftRotate);
	EigrpPortAssert(x != &TreeX->tail, "");
	EigrpPortAssert(x->rChildX != &TreeX->tail, "");
	
	y 			= x->rChildX;
	x->rChildX 	= y->lChildX;
	if (y->lChildX != &TreeX->tail){
		y->lChildX->parentX 	= x;
	}
	y->parentX = x->parentX;
	
	if (x->parentX == &TreeX->tail){
		TreeX->root = y;
	}else{ 
		if (x == x->parentX->lChildX){
			x->parentX->lChildX = y;
		}else{
			x->parentX->rChildX = y;
		}
	}
	y->lChildX 	= x;
	x->parentX 	= y;
	EIGRP_FUNC_LEAVE(EigrpUtilRbXLeftRotate);

	return;
}

/************************************************************************************

	Name:	EigrpUtilRbXRightRotate

	Desc:	This function is to right rotate the given rbX tree.
		
	Para: 	TreeX	- pointer to the given rbX tree
			x	- pointer to the axes node
	
	Ret:		
************************************************************************************/

void	EigrpUtilRbXRightRotate(EigrpRbTreeX_pt TreeX,EigrpRbNodeX_pt x)
{
 	EigrpRbNodeX_pt y;
 	
 	EIGRP_FUNC_ENTER(EigrpUtilRbXRightRotate);
	EigrpPortAssert(x != &TreeX->tail, "");
 	EigrpPortAssert(x->lChildX != &TreeX->tail, "");
 	
 	y = x->lChildX;
 	x->lChildX = y->rChildX;
 	if (y->rChildX != &TreeX->tail){
 		y->rChildX->parentX 	= x;
 	}
 	y->parentX= x->parentX;
 	
 	if (x->parentX == &TreeX->tail){
 		TreeX->root = y;
 	}else{
 		if (x == x->parentX->rChildX){
 			x->parentX->rChildX = y;
 		}else{
 			x->parentX->lChildX = y;
 		}
 	}
 	y->rChildX = x;
 	x->parentX = y;
	EIGRP_FUNC_LEAVE(EigrpUtilRbXRightRotate);

	return;
 }

/************************************************************************************

	Name:	EigrpUtilRbXFind

	Desc:	This function is to find if there is a node in the given rbX tree, which is same to the
			given node.
		
	Para: 	TreeX		- pointer to the given rbX tree
			fNd			- pointer to the given node
	
	Ret:		the pointer to the node which is same to the given node,or NULL if nothing is found
************************************************************************************/

EigrpRbNodeX_pt	EigrpUtilRbXFind( EigrpRbTreeX_pt TreeX, EigrpRbNodeX_pt fNd)
{
	EigrpRbNodeX_pt 	x;
	S32    c;
	
	EIGRP_FUNC_ENTER(EigrpUtilRbXFind);
	x = TreeX->root;
	while (x != &TreeX->tail) {
		c = TreeX->compare(fNd, x);
		if (c == 0){
			break;
		}
		if (c < 0){
			x = x->lChildX;
		}else{
			x = x->rChildX;
		}
	}
	if (x == &TreeX->tail){
		EIGRP_FUNC_LEAVE(EigrpUtilRbXFind);
		return(EigrpRbNodeX_pt)0;
	}else{
		EIGRP_FUNC_LEAVE(EigrpUtilRbXFind);
		return x;
	}
}

/************************************************************************************

	Name:	EigrpUtilRbXGetFirst

	Desc:	This function is get the first node of the given rbX tree.
		
	Para: 	TreeX		- pointer to the given rbX tree
	
	Ret:		pointer to the first node
************************************************************************************/

EigrpRbNodeX_pt	EigrpUtilRbXGetFirst(EigrpRbTreeX_pt TreeX)
{
	EigrpRbNodeX_pt x;
	
	EIGRP_FUNC_ENTER(EigrpUtilRbXGetFirst);
	if (TreeX->root == &TreeX->tail){
		EIGRP_FUNC_LEAVE(EigrpUtilRbXGetFirst);
		return NULL;
	}else{
		x = TreeX->root;
		while (x->lChildX != &TreeX->tail){
			x = x->lChildX;
			EigrpPortAssert((S32)x->lChildX, "");	/* IF this is triggered, some corrupt happened. tiger added 021212 */
		}
	}  
	EIGRP_FUNC_LEAVE(EigrpUtilRbXGetFirst);

	return x;
}

/************************************************************************************

	Name:	EigrpUtilRbXGetNext

	Desc:	This function is to get the next node to the given node in the given rbX tree.
		
	Para: 	TreeX		- pointer to the given rbX tree
			x			- pointer to the given node
	
	Ret:		pointer to the next node
************************************************************************************/

EigrpRbNodeX_pt	EigrpUtilRbXGetNext( EigrpRbTreeX_pt TreeX, EigrpRbNodeX_pt x)
{
 	EigrpRbNodeX_pt y;
 	
 	EIGRP_FUNC_ENTER(EigrpUtilRbXGetNext);
	if(x==(EigrpRbNodeX_pt)0){
		EIGRP_FUNC_LEAVE(EigrpUtilRbXGetNext);
 		return(EigrpRbNodeX_pt)NULL;
 	}

 	if (x->rChildX != &TreeX->tail){
		EigrpPortAssert((S32)x->rChildX, ""); /*  IF this is triggered, some corrupt happened. tiger added 021212 */
 	
 		y = x->rChildX;
 		while (y->lChildX != &TreeX->tail){
 			y = y->lChildX;
 		}
 	}else{
 		y = x->parentX;
 		while (y != &TreeX->tail && x == y->rChildX) {
 			x = y;
 			y = x->parentX;
 		}
 	}
 		
 	if (y == &TreeX->tail){
		EIGRP_FUNC_LEAVE(EigrpUtilRbXGetNext);
 		return(EigrpRbNodeX_pt) 0;
 	}else{
		EIGRP_FUNC_LEAVE(EigrpUtilRbXGetNext);
 		return y;
 	}
}

/************************************************************************************

	Name:	EigrpUtilRtTreeCreate

	Desc:	This function is to create an eigrp routing tree.
		
	Para: 	NONE
	
	Ret:		pointer to the new eigrp routing tree
************************************************************************************/

EigrpRtTree_pt  EigrpUtilRtTreeCreate()
{
	EigrpRtTree_pt	pTree;

	EIGRP_FUNC_ENTER(EigrpUtilRtTreeCreate);
	pTree	= (EigrpRtTree_pt)EigrpPortMemMalloc(sizeof(EigrpRtTree_st));
	if(!pTree){
		EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can not malloc memory in EigrpUtilRtTreeCreate.\n");
		EIGRP_FUNC_LEAVE(EigrpUtilRtTreeCreate);
		return NULL;
	}
	EigrpUtilRbXTreeInit((EigrpRbTreeX_pt)pTree, EigrpUtilRtNodeCompare, (void(*)(EigrpRbNodeX_pt))EigrpUtilRtNodeCleanUp);
	EIGRP_FUNC_LEAVE(EigrpUtilRtTreeCreate);
	
	return pTree;
}

/************************************************************************************

	Name:	EigrpUtilRtTreeDestroy

	Desc:	This function is destroy an eigrp routing tree.
		
	Para: 	pTree		- pointer the eigrp routing tree to be destoryed
	
	Ret:		NONe
************************************************************************************/

void  EigrpUtilRtTreeDestroy(EigrpRtTree_pt pTree)
{
	EIGRP_FUNC_ENTER(EigrpUtilRtTreeDestroy);
	EigrpUtilRbXTreeDestroy((EigrpRbTreeX_pt)pTree);
	EIGRP_FUNC_LEAVE(EigrpUtilRtTreeDestroy);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilRtTreeCleanUp

	Desc:	This function is to clean up an eigrp routing tree.
		
	Para: 	pTree		- pointer the eigrp routing tree to be cleaned up
	
	Ret:		NONE
************************************************************************************/

void  EigrpUtilRtTreeCleanUp(EigrpRtTree_pt pTree)
{
	EIGRP_FUNC_ENTER(EigrpUtilRtTreeCleanUp);
	EigrpUtilRbXTreeCleanUp((EigrpRbTreeX_pt)pTree);
	EIGRP_FUNC_LEAVE(EigrpUtilRtTreeCleanUp);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilRtNodeGetFirst

	Desc:	This function is to get the first node in the given eigrp routing tree.
		
	Para: 	Tr	- pointer to the eigrp routing tree
	
	Ret:		pointer to the first node in the given eigrp routing tree.
************************************************************************************/

EigrpRtNode_pt  EigrpUtilRtNodeGetFirst(EigrpRtTree_pt Tr) 
{
	EigrpRtNode_pt retVal;

	EIGRP_FUNC_ENTER(EigrpUtilRtNodeGetFirst);
	retVal = (EigrpRtNode_pt)EigrpUtilRbXGetFirst(&Tr->tree);
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeGetFirst);
	
	return retVal;
}

/************************************************************************************

	Name:	EigrpUtilRtNodeGetNext

	Desc:	This function is to get the next node to the given one in the given routing tree.
		
	Para: 	Tr	- pointer to the eigrp routing tree
			x	- pointer to the given node
	
	Ret:		pointer to the next node 
************************************************************************************/

EigrpRtNode_pt  EigrpUtilRtNodeGetNext(EigrpRtTree_pt Tr, EigrpRtNode_pt x)
{
	EigrpRtNode_pt retVal;

	EIGRP_FUNC_ENTER(EigrpUtilRtNodeGetNext);
	retVal = (EigrpRtNode_pt)EigrpUtilRbXGetNext(&Tr->tree, (EigrpRbNodeX_pt) x);
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeGetNext);
	
	return retVal;
}

/************************************************************************************

	Name:	EigrpUtilRtNodeCompare

	Desc:	This function is to do the compare for two routing nodes.
		
	Para: 	x	- pointer to one routing node
			y	- pointer to the other routing node
	
	Ret:		0	for the two routing nodes are the same node
			integer(not zero)	for they are not the same node 
************************************************************************************/

S32	EigrpUtilRtNodeCompare(EigrpRbNodeX_pt x, EigrpRbNodeX_pt y)
{
	S32 	result;

	/* The route should use the HOST sequence. */
	EIGRP_FUNC_ENTER(EigrpUtilRtNodeCompare);
	result	= ((EigrpRtNode_pt)x)->dest - ((EigrpRtNode_pt)y)->dest;

	if (result == 0){
		result	= ((EigrpRtNode_pt)x)->mask - ((EigrpRtNode_pt)y)->mask;
	}	
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeCompare);
	
	return(result);
}

/************************************************************************************

	Name:	EigrpUtilRtNodeCleanUp

	Desc:	This function is to clean up all the routing entries attached to the given routing node.
		
	Para: 	pNode	- pointer to the given routing node
	
	Ret:		NONE
************************************************************************************/

void  EigrpUtilRtNodeCleanUp(EigrpRtNode_pt pNode)
{
  	EigrpRtEntry_pt	pNextEntry;

	EIGRP_FUNC_ENTER(EigrpUtilRtNodeCleanUp);
	EigrpPortAssert((U32)pNode->rtEntry, "Met a pNode without any pEntry.\n");

	while(pNode->rtEntry){
		pNextEntry = pNode->rtEntry->next;
		EigrpPortMemFree(pNode->rtEntry);
		pNode->rtEntry = pNextEntry;
	}

	EigrpPortMemFree(pNode);
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeCleanUp);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilRtNodeFindExact

	Desc:	This function is to find the exact routing node in the given routing tree, given the 
			dest ip address and mask.
		
	Para: 	Tr		- pointer to the given routing tree
			dest		- the exact routing node destination ip address
			mask	- mask of the exact routing node destination ip address
	
	Ret:		pointer to the exact routing node
************************************************************************************/

 EigrpRtNode_pt  EigrpUtilRtNodeFindExact(EigrpRtTree_pt Tr, U32 dest, U32 mask)
{
	EigrpRtNode_st  	x;
  	EigrpRtNode_pt	retVal;

	EIGRP_FUNC_ENTER(EigrpUtilRtNodeFindExact);
	x.dest	= dest & mask;
	x.mask	= mask;

	retVal = (EigrpRtNode_pt)EigrpUtilRbXFind(&Tr->tree, (EigrpRbNodeX_pt)&x); 
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeFindExact);
	
	return retVal;
}

/************************************************************************************

	Name:	EigrpUtilRtNodeAdd

	Desc:	This function is to add one route into the eigrp routing table. 
		
	Para: 	rtTable		- pointer to the given routing table
			ipAddr		- new routing node's destination address
			mask		- mask of new routing node's destination address
	
	Ret:		pointer to the new routeing node
************************************************************************************/

EigrpRtNode_pt	EigrpUtilRtNodeAdd(EigrpRtTree_pt rtTable, U32 ipAddr, U32 ipMask)
{
	EigrpRtNode_pt	pRtNode;
	S32	retVal;

	EIGRP_FUNC_ENTER(EigrpUtilRtNodeAdd);
	pRtNode	= (EigrpRtNode_pt)EigrpPortMemMalloc(sizeof(EigrpRtNode_st));
	if(!pRtNode){
		EIGRP_TRC(DEBUG_EIGRP_OTHER, "Can not malloc mem in EigrpUtilRtNodeAdd.\n");
		EIGRP_FUNC_LEAVE(EigrpUtilRtNodeAdd);
		return NULL;
	}

	pRtNode->dest	= ipAddr & ipMask;
	pRtNode->mask	= ipMask;

	retVal = EigrpUtilRbXInsert(&rtTable->tree, (EigrpRbNodeX_pt)pRtNode);
	if(!retVal){
		EIGRP_FUNC_LEAVE(EigrpUtilRtNodeAdd);
		return NULL;
	}
	
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeAdd);
	return pRtNode;
}

/************************************************************************************

	Name:	EigrpUtilRtNodeDel

	Desc:	This function is to delete one route from the given eigrp routing table.
		
	Para: 	rtTable	- pointer to the given routing table
			pRt		- pointer to the routing node deleted

	Ret:		NONE
************************************************************************************/

void	EigrpUtilRtNodeDel(EigrpRtTree_pt rtTable, EigrpRtNode_pt pRt)
{
	EigrpRtNode_pt	pNode;

	EIGRP_FUNC_ENTER(EigrpUtilRtNodeDel);
	pNode	= (EigrpRtNode_pt)pRt;
	pRt		= (void *)EigrpUtilRtNodeFindExact(rtTable, pNode->dest, pNode->mask);
	if(pRt){
		EigrpUtilRbXDelete(&rtTable->tree, (EigrpRbNodeX_pt)pNode);
	}
	EIGRP_FUNC_LEAVE(EigrpUtilRtNodeDel);
	
	return;
}

/************************************************************************************

	Name:	EigrpUtilGetNaturalMask

	Desc:	This function is to get the natural mask of the given ip address.
		
	Para: 	ipNet	- the given ip address
	
	Ret:		the natural mask of the given ip address
************************************************************************************/

U32	EigrpUtilGetNaturalMask(U32 ipNet)
{
	EIGRP_FUNC_ENTER(EigrpUtilGetNaturalMask);
	if((ipNet & 0x80000000) == 0){
		/* Class A */
		EIGRP_FUNC_LEAVE(EigrpUtilGetNaturalMask);
		return(0xff000000);
	}
	if((ipNet & 0xc0000000) == 0){
		/* Class B */
		EIGRP_FUNC_LEAVE(EigrpUtilGetNaturalMask);
		return(0xffff0000);
	}
	if(ipNet & 0xe0000000){
		/* Class C */
		EIGRP_FUNC_LEAVE(EigrpUtilGetNaturalMask);
		return(0xffffff00);
	}

	/* We dont support get net for multicast addr. */
	EIGRP_FUNC_LEAVE(EigrpUtilGetNaturalMask);
	return 0;
}

/************************************************************************************

	Name:	EigrpUtilGetChkSum

	Desc:	This function is to get the check sum of the given memory block.
		
	Para: 	buf		- pointer to the given memory block.
			length 	- the length of the given memory block
	
	Ret:		check sum of the given memory block 
************************************************************************************/

U16    EigrpUtilGetChkSum(U8 *buf ,  U32 length)
{
	U32	wordNum;
	U16	*u16Point;
	U32	ckSum;

	EIGRP_FUNC_ENTER(EigrpUtilGetChkSum);
	ckSum = 0;
	if(length <= 0){
		EIGRP_FUNC_LEAVE(EigrpUtilGetChkSum);
		return ckSum;
	}

	u16Point	= (U16 *)buf;
	for(wordNum = length >> 1; wordNum; wordNum--){
		ckSum	= ckSum + *u16Point;
		u16Point++;
	}

	if(length - (length >> 1)*2){
#if (BYTE_ORDER == BIG_ENDIAN)/*modify_zhenxl_20120618*/
		ckSum += 0 | ((*(U8 *)u16Point) << 8);
#else
		ckSum	= ckSum + *(buf + length - 1);
#endif
	}

	ckSum	= (ckSum >> 16) + (ckSum & 0xffff);
	ckSum	= ckSum + (ckSum >> 16);
	ckSum	= ~ckSum;
	EIGRP_FUNC_LEAVE(EigrpUtilGetChkSum);

	return ckSum;
}

static U8 MD5Padding[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/************************************************************************************

	Name:	EigrpUtilMD5Init

	Desc:	This function is to begin an MD5 operation, writing a new context.
		
	Para: 	context		- pointer to the context for writing
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMD5Init(EigrpMD5Ctx_pt context)
{
	EIGRP_FUNC_ENTER(EigrpUtilMD5Init);
	context->count[0] = context->count[1] = 0;
	/* Load magic initialization constants. */
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
	EIGRP_FUNC_LEAVE(EigrpUtilMD5Init);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMD5Update

	Desc:	This function is to process a message block, and updating the context.
		
	Para: 	context		- pointer to the context for updating
			input		- pointer to the inputting buffer
			inputLen		- length of the inputting buffer

	Ret:		NONE
************************************************************************************/

void	EigrpUtilMD5Update(EigrpMD5Ctx_pt context, U8 *input, U32 inputLen)
{
	U32 i, index, partLen;

	EIGRP_FUNC_ENTER(EigrpUtilMD5Update);
	/* Compute number of bytes mod 64 */
	index = (U32)((context->count[0] >> 3) & 0x3F);

	/* Update number of bits */
	if((context->count[0] += ((U32)inputLen << 3)) < ((U32)inputLen << 3)){
		context->count[1]++;
	}
	context->count[1] += ((U32)inputLen >> 29);

	partLen = 64 - index;

	/* Transform as many times as possible. */
	if(inputLen >= partLen){
		EigrpPortMemCpy((U8 *)&context->buffer[index], (U8 *)input, partLen);
		EigrpUtilMD5Transform (context->state, context->buffer);

		for(i = partLen; i + 63 < inputLen; i += 64){
			EigrpUtilMD5Transform (context->state, &input[i]);
		}

		index = 0;
	}else{
		i = 0;
	}
	
	EigrpPortMemCpy((U8 *)&context->buffer[index], (U8 *)&input[i], inputLen-i);
	EIGRP_FUNC_LEAVE(EigrpUtilMD5Update);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMD5Final

	Desc:	This function is to do the MD5 finalization. It ends an MD5 message-digest operation, 
			writing the message digest and zeroizing the context.
		
	Para: 	digest		- pointer to the MD5 message digest
			context		- pointer to the context for zeroizing
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMD5Final(U8 *digest, EigrpMD5Ctx_pt context)
{
	U8 bits[8];
	U32 index, padLen;

	EIGRP_FUNC_ENTER(EigrpUtilMD5Final);
	/* Save number of bits */
	EigrpUtilMD5Encode(bits, context->count, 8);

	/* Pad out to 56 mod 64. */
	index	= (U32)((context->count[0] >> 3) & 0x3f);
	padLen	= (index < 56) ? (56 - index) : (120 - index);
	EigrpUtilMD5Update(context, MD5Padding, padLen);

	/* Append length (before padding) */
	EigrpUtilMD5Update(context, bits, 8);

	/* Store state in digest */
	EigrpUtilMD5Encode(digest, context->state, 16);

	/* Zeroize sensitive information. */
	EigrpPortMemSet((U8 *)context, 0, sizeof(*context));
	EIGRP_FUNC_LEAVE(EigrpUtilMD5Final);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMD5Transform

	Desc:	This function is to do the MD5 basic transformation.
		
	Para: 	state		- pointer to the MD5 context state
			block		- pointer to the MD5 context inputting buffer
		
	Ret:		NONE	
************************************************************************************/

void	EigrpUtilMD5Transform(U32 *state, U8 *block)
{
	U32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	EIGRP_FUNC_ENTER(EigrpUtilMD5Transform);
	EigrpUtilMD5Decode(x, block, 64);

	/* Round 1 */
	EIGRP_MD5_FF(a, b, c, d, x[ 0], EIGRP_MD5_S11, 0xd76aa478); /* 1 */
	EIGRP_MD5_FF(d, a, b, c, x[ 1], EIGRP_MD5_S12, 0xe8c7b756); /* 2 */
	EIGRP_MD5_FF(c, d, a, b, x[ 2], EIGRP_MD5_S13, 0x242070db); /* 3 */
	EIGRP_MD5_FF(b, c, d, a, x[ 3], EIGRP_MD5_S14, 0xc1bdceee); /* 4 */
	EIGRP_MD5_FF(a, b, c, d, x[ 4], EIGRP_MD5_S11, 0xf57c0faf); /* 5 */
	EIGRP_MD5_FF(d, a, b, c, x[ 5], EIGRP_MD5_S12, 0x4787c62a); /* 6 */
	EIGRP_MD5_FF(c, d, a, b, x[ 6], EIGRP_MD5_S13, 0xa8304613); /* 7 */
	EIGRP_MD5_FF(b, c, d, a, x[ 7], EIGRP_MD5_S14, 0xfd469501); /* 8 */
	EIGRP_MD5_FF(a, b, c, d, x[ 8], EIGRP_MD5_S11, 0x698098d8); /* 9 */
	EIGRP_MD5_FF(d, a, b, c, x[ 9], EIGRP_MD5_S12, 0x8b44f7af); /* 10 */
	EIGRP_MD5_FF(c, d, a, b, x[10], EIGRP_MD5_S13, 0xffff5bb1); /* 11 */
	EIGRP_MD5_FF(b, c, d, a, x[11], EIGRP_MD5_S14, 0x895cd7be); /* 12 */
	EIGRP_MD5_FF(a, b, c, d, x[12], EIGRP_MD5_S11, 0x6b901122); /* 13 */
	EIGRP_MD5_FF(d, a, b, c, x[13], EIGRP_MD5_S12, 0xfd987193); /* 14 */
	EIGRP_MD5_FF(c, d, a, b, x[14], EIGRP_MD5_S13, 0xa679438e); /* 15 */
	EIGRP_MD5_FF(b, c, d, a, x[15], EIGRP_MD5_S14, 0x49b40821); /* 16 */

	/* Round 2 */
	EIGRP_MD5_GG(a, b, c, d, x[ 1], EIGRP_MD5_S21, 0xf61e2562); /* 17 */
	EIGRP_MD5_GG(d, a, b, c, x[ 6], EIGRP_MD5_S22, 0xc040b340); /* 18 */
	EIGRP_MD5_GG(c, d, a, b, x[11], EIGRP_MD5_S23, 0x265e5a51); /* 19 */
	EIGRP_MD5_GG(b, c, d, a, x[ 0], EIGRP_MD5_S24, 0xe9b6c7aa); /* 20 */
	EIGRP_MD5_GG(a, b, c, d, x[ 5], EIGRP_MD5_S21, 0xd62f105d); /* 21 */
	EIGRP_MD5_GG(d, a, b, c, x[10], EIGRP_MD5_S22,  0x2441453); /* 22 */
	EIGRP_MD5_GG(c, d, a, b, x[15], EIGRP_MD5_S23, 0xd8a1e681); /* 23 */
	EIGRP_MD5_GG(b, c, d, a, x[ 4], EIGRP_MD5_S24, 0xe7d3fbc8); /* 24 */
	EIGRP_MD5_GG(a, b, c, d, x[ 9], EIGRP_MD5_S21, 0x21e1cde6); /* 25 */
	EIGRP_MD5_GG(d, a, b, c, x[14], EIGRP_MD5_S22, 0xc33707d6); /* 26 */
	EIGRP_MD5_GG(c, d, a, b, x[ 3], EIGRP_MD5_S23, 0xf4d50d87); /* 27 */
	EIGRP_MD5_GG(b, c, d, a, x[ 8], EIGRP_MD5_S24, 0x455a14ed); /* 28 */
	EIGRP_MD5_GG(a, b, c, d, x[13], EIGRP_MD5_S21, 0xa9e3e905); /* 29 */
	EIGRP_MD5_GG(d, a, b, c, x[ 2], EIGRP_MD5_S22, 0xfcefa3f8); /* 30 */
	EIGRP_MD5_GG(c, d, a, b, x[ 7], EIGRP_MD5_S23, 0x676f02d9); /* 31 */
	EIGRP_MD5_GG(b, c, d, a, x[12], EIGRP_MD5_S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
	EIGRP_MD5_HH(a, b, c, d, x[ 5], EIGRP_MD5_S31, 0xfffa3942); /* 33 */
	EIGRP_MD5_HH(d, a, b, c, x[ 8], EIGRP_MD5_S32, 0x8771f681); /* 34 */
	EIGRP_MD5_HH(c, d, a, b, x[11], EIGRP_MD5_S33, 0x6d9d6122); /* 35 */
	EIGRP_MD5_HH(b, c, d, a, x[14], EIGRP_MD5_S34, 0xfde5380c); /* 36 */
	EIGRP_MD5_HH(a, b, c, d, x[ 1], EIGRP_MD5_S31, 0xa4beea44); /* 37 */
	EIGRP_MD5_HH(d, a, b, c, x[ 4], EIGRP_MD5_S32, 0x4bdecfa9); /* 38 */
	EIGRP_MD5_HH(c, d, a, b, x[ 7], EIGRP_MD5_S33, 0xf6bb4b60); /* 39 */
	EIGRP_MD5_HH(b, c, d, a, x[10], EIGRP_MD5_S34, 0xbebfbc70); /* 40 */
	EIGRP_MD5_HH(a, b, c, d, x[13], EIGRP_MD5_S31, 0x289b7ec6); /* 41 */
	EIGRP_MD5_HH(d, a, b, c, x[ 0], EIGRP_MD5_S32, 0xeaa127fa); /* 42 */
	EIGRP_MD5_HH(c, d, a, b, x[ 3], EIGRP_MD5_S33, 0xd4ef3085); /* 43 */
	EIGRP_MD5_HH(b, c, d, a, x[ 6], EIGRP_MD5_S34,  0x4881d05); /* 44 */
	EIGRP_MD5_HH(a, b, c, d, x[ 9], EIGRP_MD5_S31, 0xd9d4d039); /* 45 */
	EIGRP_MD5_HH(d, a, b, c, x[12], EIGRP_MD5_S32, 0xe6db99e5); /* 46 */
	EIGRP_MD5_HH(c, d, a, b, x[15], EIGRP_MD5_S33, 0x1fa27cf8); /* 47 */
	EIGRP_MD5_HH(b, c, d, a, x[ 2], EIGRP_MD5_S34, 0xc4ac5665); /* 48 */

	/* Round 4 */
	EIGRP_MD5_II(a, b, c, d, x[ 0], EIGRP_MD5_S41, 0xf4292244); /* 49 */
	EIGRP_MD5_II(d, a, b, c, x[ 7], EIGRP_MD5_S42, 0x432aff97); /* 50 */
	EIGRP_MD5_II(c, d, a, b, x[14], EIGRP_MD5_S43, 0xab9423a7); /* 51 */
	EIGRP_MD5_II(b, c, d, a, x[ 5], EIGRP_MD5_S44, 0xfc93a039); /* 52 */
	EIGRP_MD5_II(a, b, c, d, x[12], EIGRP_MD5_S41, 0x655b59c3); /* 53 */
	EIGRP_MD5_II(d, a, b, c, x[ 3], EIGRP_MD5_S42, 0x8f0ccc92); /* 54 */
	EIGRP_MD5_II(c, d, a, b, x[10], EIGRP_MD5_S43, 0xffeff47d); /* 55 */
	EIGRP_MD5_II(b, c, d, a, x[ 1], EIGRP_MD5_S44, 0x85845dd1); /* 56 */
	EIGRP_MD5_II(a, b, c, d, x[ 8], EIGRP_MD5_S41, 0x6fa87e4f); /* 57 */
	EIGRP_MD5_II(d, a, b, c, x[15], EIGRP_MD5_S42, 0xfe2ce6e0); /* 58 */
	EIGRP_MD5_II(c, d, a, b, x[ 6], EIGRP_MD5_S43, 0xa3014314); /* 59 */
	EIGRP_MD5_II(b, c, d, a, x[13], EIGRP_MD5_S44, 0x4e0811a1); /* 60 */
	EIGRP_MD5_II(a, b, c, d, x[ 4], EIGRP_MD5_S41, 0xf7537e82); /* 61 */
	EIGRP_MD5_II(d, a, b, c, x[11], EIGRP_MD5_S42, 0xbd3af235); /* 62 */
	EIGRP_MD5_II(c, d, a, b, x[ 2], EIGRP_MD5_S43, 0x2ad7d2bb); /* 63 */
	EIGRP_MD5_II(b, c, d, a, x[ 9], EIGRP_MD5_S44, 0xeb86d391); /* 64 */

	state[0]	+= a;
	state[1]	+= b;
	state[2]	+= c;
	state[3]	+= d;

	/* Zeroize sensitive information. */
	EigrpPortMemSet((U8 *)x, 0, sizeof(x));
	EIGRP_FUNC_LEAVE(EigrpUtilMD5Transform);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMD5Encode

	Desc:	This function is to encode input(U32) into output(U8).
		
	Para: 	output	- pointer to the given output string
			input	- pointer to the given input integer
			len		- length of the output in bytes
			
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMD5Encode(U8 *output, U32 *input, U32 len)
{
	U32 i, j;

	EIGRP_FUNC_ENTER(EigrpUtilMD5Encode);
	for(i = 0, j = 0; j < len; i++, j += 4){
		output[j]	= (U8)(input[i] & 0xff);
		output[j+1]	= (U8)((input[i] >> 8) & 0xff);
		output[j+2]	= (U8)((input[i] >> 16) & 0xff);
		output[j+3]	= (U8)((input[i] >> 24) & 0xff);
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMD5Encode);

	return;
}

/************************************************************************************

	Name:	EigrpUtilMD5Decode

	Desc:	This function is to decode input(U8) into output(U32).
		
	Para: 	output	- pointer to the given output string
			input	- pointer to the given input integer
			len		- length of the output in bytes
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilMD5Decode(U32 *output, U8 *input, U32 len)
{
	U32 i, j;

	EIGRP_FUNC_ENTER(EigrpUtilMD5Decode);
	for(i = 0, j = 0; j < len; i++, j += 4){
		output[i] = ((U32)input[j]) | (((U32)input[j+1]) << 8) | (((U32)input[j+2]) << 16) | (((U32)input[j+3]) << 24);
	}
	EIGRP_FUNC_LEAVE(EigrpUtilMD5Decode);

	return;
}

/************************************************************************************

	Name:	EigrpUtilGenMd5Digest

	Desc:	This function is to generate the md5 digest.
		
	Para: 	buf		- pointer to the inputting buffer for updating
			leng		- lengthe of the inputting buffer
			digest	- pointer to the MD5 message digest for MD5 finalization
	
	Ret:		NONE
************************************************************************************/

void	EigrpUtilGenMd5Digest(U8 *buf, U32 len, U8 *digest)
{
	EigrpMD5Ctx_st ctx;
	
	EIGRP_FUNC_ENTER(EigrpUtilGenMd5Digest);
	EigrpUtilMD5Init(&ctx);
	EigrpUtilMD5Update(&ctx, buf, len);
	EigrpUtilMD5Final(digest, &ctx);
	EIGRP_FUNC_LEAVE(EigrpUtilGenMd5Digest);

	return;
}

/************************************************************************************

	Name:	EigrpUtilVerifyMd5

	Desc:	This function is verify the eigrp md5 digest.
		
	Para: 	eigrph		- pointer to the header of eigrp packet
			iidb			- pointer to the eigrp interface
			pakLen		- packet length without packet header
	
	Ret:		TRUE	for success
			FALSE 	for failure
************************************************************************************/

S32	EigrpUtilVerifyMd5(EigrpPktHdr_st *eigrph, EigrpIdb_st *iidb, U32 paklen)
{
	EigrpGenTlv_st	*tlv_hdr;
	EigrpAuthTlv_st	*auth_tlv;
	U8	save_digest[16];
	EigrpMD5Ctx_st	ctx;
	U8	digest[ EIGRP_AUTH_LEN ];

	EIGRP_FUNC_ENTER(EigrpUtilVerifyMd5);
	if(!iidb->authSet || (iidb->authMode == FALSE)){
		EIGRP_FUNC_LEAVE(EigrpUtilVerifyMd5);
		return FALSE;
	}

	if((paklen) < sizeof(EigrpGenTlv_st)){	
		EIGRP_FUNC_LEAVE(EigrpUtilVerifyMd5);
		return TRUE;
	}

	tlv_hdr = (EigrpGenTlv_st *)(eigrph + 1);
	if(NTOHS(tlv_hdr->type) != EIGRP_AUTH ){
		EIGRP_FUNC_LEAVE(EigrpUtilVerifyMd5);
		return TRUE;
	}
	
	auth_tlv	= (EigrpAuthTlv_st*) tlv_hdr;

	if(NTOHL(auth_tlv->keyId) != iidb->authInfo.keyId){
		EIGRP_FUNC_LEAVE(EigrpUtilVerifyMd5);
		return TRUE;
	}

	EigrpPortMemCpy((U8 *)save_digest, (U8 *)auth_tlv->digest, 16);

	EigrpPortMemCpy((U8 *)auth_tlv->digest, iidb->authInfo.authData, 16);
	EigrpUtilMD5Init(&ctx);
	EigrpUtilMD5Update(&ctx, (U8 *)eigrph, paklen);
	EigrpUtilMD5Final(digest, &ctx);

	if(EigrpPortMemCmp((U8 *)save_digest, (U8 *)digest, EIGRP_AUTH_LEN)){  
		EIGRP_FUNC_LEAVE(EigrpUtilVerifyMd5);
		return TRUE;
	}else{
		EIGRP_FUNC_LEAVE(EigrpUtilVerifyMd5);
		return FALSE;
	}
}

/************************************************************************************

	Name:	EigrpUtilShowProtocol

	Desc:	This function prints protocol information of eigrp process pdb.
		
	Para: 	pdb		- pointer to the eigrp process pdb
	
	Ret:		SUCCESS
************************************************************************************/

S32	EigrpUtilShowProtocol(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *),
								EigrpPdb_st *pdb)
{
	EigrpDbgCtrl_st	dbg;

	EIGRP_FUNC_ENTER(EigrpUtilShowProtocol);
	
	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\nRouting Protocol is \"eigrp %d\"%s", 
									pdb->process, EIGRP_NEW_LINE_STR);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Default networks %s in outgoing updates\r\n",
								    	(pdb->rtOutDef == TRUE) ? "flagged": "unflagged");
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Default networks %s accepted from incoming updates\r\n",
								    	(pdb->rtInDef == TRUE) ? "flagged": "unflagged");
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP metric weight K1=%d, K2=%d, K3=%d, K4=%d, K5=%d\r\n",
		         						 pdb->k1, pdb->k2, pdb->k3, pdb->k4, pdb->k5);
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP maximum hopcount %d\r\n", 
									pdb->ddb->maxHopCnt);
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, " EIGRP maximum metric variance 1\r\n");

	/* Redistribute information. */
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Redistributing:\r\n");
	EigrpConfigWriteEigrpRedistribute(pdb, 0, &dbg);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Automatic network summarization is in %s\r\n",
							          (pdb->sumAuto == TRUE) ? "effect" : "off");
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Maximum path: %d\r\n", pdb->multiPath);
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Distance: internal %d external %d\r\n",
									pdb->distance, pdb->distance2);
	EigrpConfigWriteEigrpNetwork(pdb, 0, &dbg);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  Routing for Networks:\r\n");

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpUtilShowRoute

	Desc:	This function prints route information of eigrp process pdb 
		
	Para: 	pdb		- pointer to the eigrp process pdb
	
	Ret:		SUCCESS
************************************************************************************/

S32	EigrpUtilShowRoute(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *), EigrpPdb_st *pdb)
{
	EigrpRtNode_pt	rn;
	EigrpRouteInfo_st	*eigrpInfo;
	U32				tmp;
	S32				i;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	EigrpDualDdb_st	*ddb;
	
	EigrpDbgCtrl_st	dbg;

	EIGRP_FUNC_ENTER(EigrpUtilShowRoute);
	ddb = pdb->ddb;
	
	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\nRoutes of EIGRP process %d :\r\n", pdb->process);
	for(rn = EigrpUtilRtNodeGetFirst(pdb->rtTbl); rn; rn = EigrpUtilRtNodeGetNext(pdb->rtTbl, rn)){
		eigrpInfo		= (EigrpRouteInfo_st *)rn->extData;
		if(!eigrpInfo){
			continue;
		}
		tmp	= eigrpInfo->nexthop.s_addr;
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "Dest:%s, Np:%s, Metric:%d, Dist:%d\r\n",
										 EigrpUtilIp2Str(rn->dest), EigrpUtilIp2Str(tmp), 
										 eigrpInfo->metric, eigrpInfo->distance);
		
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\t[BW:%d DLY:%d RLY:%d LOAD:%d MTU:%d]\r\n",
										 eigrpInfo->metricDetail.bandwidth,
										 eigrpInfo->metricDetail.delay,
										 eigrpInfo->metricDetail.reliability,
										 eigrpInfo->metricDetail.load,
										 eigrpInfo->metricDetail.mtu);
	}
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\nDNDB Hash table:\r\n");

	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = ddb->topo[ i ]; dndb; dndb = dndb->next){
			EIGRP_SHOW_SAFE_SPRINTF(&dbg, "D:%s(%d)\r\n", 
											(*ddb->printNet)(&dndb->dest), dndb->metric);
			for(drdb = dndb->rdb; drdb; drdb = drdb->next){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\tN:%s[%d/%d]",
												(*ddb->printAddress)(&drdb->nextHop),
												 drdb->metric, drdb->succMetric);
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  ORG:%s\r\n",
												EigrpDualRouteType2string(drdb->origin));
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\t[BW:%d DLY:%d RLY:%d LOAD:%d MTU:%d]\r\n",
												drdb->vecMetric.bandwidth,
												drdb->vecMetric.delay,
												drdb->vecMetric.reliability,
												drdb->vecMetric.load,
												drdb->vecMetric.mtu);
			}
		}
	}
	
	EIGRP_FUNC_LEAVE(EigrpUtilShowRoute);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpUtilShowIpPdb

	Desc:	This function prints basic information of eigrp process pdb.
		
	Para: 	pdb		- pointer to the eigrp process pdb
	
	Ret:		SUCCESS
************************************************************************************/

S32	EigrpUtilShowIpPdb(EigrpPdb_st *pdb, EigrpDbgCtrl_pt pDbg)
{
	EigrpRedisEntry_st	*r;

	EIGRP_FUNC_ENTER(EigrpUtilShowIpPdb);
	/*  we do not printf in this function */
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "EIGRP process-id: %d:\r\n", pdb->process);
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "pdb->name =\"%s\"\r\n", pdb->name);
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "pdb->distance =%d, pdb->distDef =%d\r\n", 
									pdb->distance, pdb->distDef);
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "pdb->distance2=%d, pdb->dist2Def=%d\r\n",
									pdb->distance2, pdb->dist2Def);
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "pdb->vMetricDef.bandwidth = %d\r\n"
									"                     .delay =%d\r\n"
									"                     .reliability = %d\r\n"
									"                     .load = %d\r\n"
									"                     .mtu = %d\r\n"
									"                     .hopcount = %d\r\n",
									pdb->vMetricDef.bandwidth,
									pdb->vMetricDef.delay,
									pdb->vMetricDef.reliability,
									pdb->vMetricDef.load,
									pdb->vMetricDef.mtu,
									pdb->vMetricDef.hopcount);

	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "pdb->redisLst:\r\n");
	for(r = pdb->redisLst; r; r = r->next){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "%d : %s",r->index, EigrpProto2str(r->proto));
		if(r->rtMapName[0]){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " \troute-map: %s \t*route-map = %X",
											r->rtMapName, (U32)r->rtMap);
		}
		if(r->vmetric){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " \tmetric = (%d,%d,%d,%d,%d)" , 
											r->vmetric->bandwidth, r->vmetric->delay,
											r->vmetric->reliability, r->vmetric->load,
											r->vmetric->mtu);
		}
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "\r\n");
	}
	
	EIGRP_FUNC_LEAVE(EigrpUtilShowIpPdb);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpUtilShowIpDdb

	Desc:	This function prints basic information of eigrp process ddb.
		
	Para: 	pdb		- pointer to the eigrp process ddb
	
	Ret:		SUCCESS
************************************************************************************/

S32	EigrpUtilShowIpDdb(EigrpDualDdb_st *ddb, EigrpDbgCtrl_pt pDbg)
{
	EigrpIdb_st		*iidb;
	EigrpDualPeer_st	*peer;

	EIGRP_FUNC_ENTER(EigrpUtilShowIpDdb);
	

	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "-------------------------------------\r\n");
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "ddb->name = \"%s\", ddb->index = %d\r\n",
									ddb->name, ddb->index);
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "ddb->(k1, k2, k3, k4, k5) = (%d, %d, %d, %d, %d)\r\n",
								         ddb->k1, ddb->k2, ddb->k3, ddb->k4, ddb->k5);
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "ddb->iidbQue:\r\n");
	for(iidb = (EigrpIdb_st *) ddb->iidbQue->head; iidb; iidb = iidb->next){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "iidb->idb->name = \"%s\"\r\n", iidb->idb->name);
		if(iidb->authSet){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " iidb->authInfo.keyId = %d\r\n"
											"iidb->authInfo.type = %d\r\n"
											"iidb->authInfo.authData[0] = 0x%X\r\n",
											iidb->authInfo.keyId,
											iidb->authInfo.type,
											iidb->authInfo.authData[0]);
		}else{
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, "iidb->authInfo not set \t");
		}
		if(iidb->authMode == TRUE){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, "iidb->authMode = MD5\r\n");
		}else{
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, "iidb->authMode not set\r\n");
		}
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "iidb->useMcast = %s\r\n", 
										(iidb->useMcast == TRUE) ? "T" : "F");
	}
	EIGRP_SHOW_SAFE_SPRINTF(pDbg, "ddb->peerLst :\r\n");
	for(peer = ddb->peerLst; peer; peer = peer->nextPeer){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, 
										"peer->address = %s  helloSize = %d  flagGoingDown = %s %s\r\n",
										EigrpUtilIp2Str(peer->source), peer->helloSize,
										peer->flagGoingDown ? "T" : "F", peer->flagGoingDown ? peer->downReason : "");
	}

	EIGRP_FUNC_LEAVE(EigrpUtilShowIpDdb);
	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpUtilShowStruct

	Desc:	This function prints information of eigrp process pdb and relative ddb .
		
	Para: 	pdb		- pointer to the eigrp process pdb
	
	Ret:		SUCCESS
************************************************************************************/

S32	EigrpUtilShowStruct(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *),
							EigrpPdb_st *pdb)
{
	EigrpDbgCtrl_st	dbg;
	
	EIGRP_FUNC_ENTER(EigrpUtilShowStruct);
	
	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n=====================================\r\n");
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "Gloable :\r\n");
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "EigrpSock = %d\t\tEigrp max paket length: %d\r\n",
									gpEigrp->socket, gpEigrp->pktMaxSize);
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "Eigrp send buffer length: %d  Eigrp receive bufer length: %d\r\n",
			        					gpEigrp->sendBufLen, gpEigrp->recvBufLen);
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "=====================================\r\n");

	EigrpUtilShowIpPdb(pdb, &dbg);
	EigrpUtilShowIpDdb(pdb->ddb, &dbg);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n------------------------------------------------\r\n");
	
	EIGRP_FUNC_LEAVE(EigrpUtilShowStruct);

	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpUtilShowDebugInfo

	Desc:	This function is to show the debug infomation of eigrp.
		
	Para: 	NONE
	
	Ret:		SUCCESS
************************************************************************************/

S32	EigrpUtilShowDebugInfo(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *))
{
	EigrpDbgCtrl_st	dbg;
	EigrpDbgCtrl_pt	pDbg;
	U32	flagTmp;
	
	EIGRP_FUNC_ENTER(EigrpUtilShowDebugInfo);
	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;

	pDbg	= EigrpUtilDbgCtrlFind(name, pTerm, id);
	if(!pDbg){
		return FAILURE;
	}
	
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "EIGRP debugging status:%s\n", EIGRP_NEW_LINE_STR);

	if(pDbg->flag  == 0xffffffff){ 
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "ALL EIGRP debugging is on%s\n", EIGRP_NEW_LINE_STR); 

		EIGRP_FUNC_LEAVE(EigrpUtilShowDebugInfo);
		return SUCCESS;
	}

	if(pDbg->flag & DEBUG_EIGRP_EVENT){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP event debugging is on%s\n", EIGRP_NEW_LINE_STR);
	}

	if(pDbg->flag & DEBUG_EIGRP_PACKET){
		flagTmp	= DEBUG_EIGRP_PACKET_DETAIL;
		flagTmp	|= DEBUG_EIGRP_ROUTE;
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet%s debugging is on%s\n", 
										(pDbg->flag & flagTmp)? " detail" : "", EIGRP_NEW_LINE_STR);
	}else{
		if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND){
			EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet send%s debugging is on%s\n",
											(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL)? " detail" : "",	EIGRP_NEW_LINE_STR);
		}else{
	  		if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_UPDATE){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet send update debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_QUERY){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet send query debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_REPLY){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet send reply debugging is on%s\n", 
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_HELLO){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet send hello debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
			}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_ACK){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet send ack debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
			}
		}
	  	
	  	if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV){
	    		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet receive%s debugging is on%s\n", 
											(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL)? " detail" : "",
											EIGRP_NEW_LINE_STR);
	  	}else{
	  	 	if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_UPDATE){
		    		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet receive update debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_QUERY){
		    		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet receive query debugging is on%s\n", 
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_REPLY){
		    		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet receive reply debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_HELLO){
		    		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet receive hello debugging is on%s\n",
												EIGRP_NEW_LINE_STR);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_ACK){
		    		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP packet receive ack debugging is on%s\n",
											EIGRP_NEW_LINE_STR);
			}
	  	}
	} 

	if(pDbg->flag & DEBUG_EIGRP_ROUTE){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP Route-Manage debugging is on%s\n",
										EIGRP_NEW_LINE_STR);
	}

	if(pDbg->flag & DEBUG_EIGRP_TIMER){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP timer debugging is on%s\n", 
										EIGRP_NEW_LINE_STR);
	}

	if(pDbg->flag & DEBUG_EIGRP_TASK){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "  EIGRP task debugging is on%s\n",
										EIGRP_NEW_LINE_STR);
	}
	EIGRP_FUNC_LEAVE(EigrpUtilShowDebugInfo);

	
	return SUCCESS;
}

/************************************************************************************

	Name:	EigrpBuildAuthInfo

	Desc:	This function is to build a md5 authInfo using the given key string and the default key
			id.
		
	Para: 	keyStr	- pointer to the given key string
	
	Ret:		pointer to the new md5 authInfo structure
************************************************************************************/

EigrpAuthInfo_st *EigrpBuildAuthInfo(S8 *keyStr)
{
	EigrpAuthInfo_st	*auth_temp;

	EIGRP_FUNC_ENTER(EigrpBuildAuthInfo);
	auth_temp = EigrpPortMemMalloc(sizeof(EigrpAuthInfo_st));
	if(!auth_temp){	 
		EIGRP_FUNC_LEAVE(EigrpBuildAuthInfo);
		return NULL;
	}
	EigrpUtilMemZero((void *)auth_temp, sizeof(EigrpAuthInfo_st)); 
	
	if(EigrpPortStrLen(keyStr) <= EIGRP_AUTH_LEN){
		EigrpPortStrCpy(auth_temp->authData, keyStr);
	}else{
		(void)EigrpPortMemCpy(auth_temp->authData, keyStr, EIGRP_AUTH_LEN);
		auth_temp->authData[EIGRP_AUTH_LEN - 1]	= '\0';
	}
	
	auth_temp->keyId = EIGRP_AUTH_KEYID_DEF;
	EIGRP_FUNC_LEAVE(EigrpBuildAuthInfo);

	return auth_temp;
}
S32	EigrpUtilCmpNet(void *net, void *pTmp)
{
	EigrpNetMetric_pt pNet;

	pNet = (EigrpNetMetric_pt)pTmp;
	if(pNet->ipNet == *(U32 *)net){
		return TRUE;
	}else{
		return FALSE;
	}
}
void	*EigrpUtilDllSearch(EigrpDll_pt *ppHead, void *x, S32 (*func)(void *, void *))
{
	EigrpDll_pt	pTem;
	
	if(!ppHead){
		EigrpPortAssert(0,"");
	}
	for(pTem = *ppHead; pTem; pTem = pTem->forw){
		if(func(x, pTem)){
			return pTem;
		}
	}

	return NULL;
}

void	EigrpUtilDllInsert(EigrpDll_pt pCur, EigrpDll_pt pPrev, EigrpDll_pt *ppHead)
{
	
	if(pCur->back || pCur->forw){
		EigrpPortAssert(0,"");
	}

	if(pPrev){
		pCur->forw	= pPrev->forw;
		pCur->back	= pPrev;
		if(pCur->forw){
			pCur->forw->back	= pCur;
		}
		pPrev->forw	= pCur;
	}else{
		pCur->forw	= *ppHead;
		pCur->back	= NULL;
		*ppHead	= pCur;
		if(pCur->forw){
			pCur->forw->back	= pCur;
		}
	}
	
	return;
}

void	EigrpUtilDllRemove(EigrpDll_pt pCur, EigrpDll_pt *ppHead)
{
	EigrpDll_pt	pTem;
	
	for(pTem = *ppHead; pTem; pTem = pTem->forw){
		if(pTem == pCur){
			
			if(*ppHead == pCur){
				*ppHead	= pCur->forw;
			}

			if(pCur->forw){
				pCur->forw->back	= pCur->back;
			}

			if(pCur->back){
				pCur->back->forw	= pCur->forw;
			}
			pCur->back	= pCur->forw	= NULL;
			return;
		}
	}
	EigrpPortAssert(0, "");

	return;
}

void EigrpUtilDllCleanUp(struct EigrpDll_ **ppLst, void(*pReleaseFun)(void **))
{
	EigrpDll_pt		pCur;

	if(!ppLst || !*ppLst){
		return;
	}

	while(*ppLst){
		pCur = *ppLst;
		*ppLst = (*ppLst)->forw;
		pReleaseFun(&pCur);
	}

	*ppLst = NULL;
	
	return;
}

void	EigrpUtilDbgCtrlFree(EigrpDbgCtrl_pt pDbgCtrl)
{
/*	if(pDbgCtrl->name){
		EigrpPortMemFree(pDbgCtrl->name);
	}*/

	if(pDbgCtrl->term){
		EigrpPortMemFree(pDbgCtrl->term);
	}

	EigrpPortMemFree(pDbgCtrl);

	return;
}

EigrpDbgCtrl_pt	EigrpUtilDbgCtrlCreate(U8 *name, void *pTerm, U32 id)
{
	EigrpDbgCtrl_pt	pDbgCtrl;

	if(!name && !pTerm && !id){
		return NULL;
	}

	pDbgCtrl	= (EigrpDbgCtrl_pt)EigrpPortMemMalloc(sizeof(EigrpDbgCtrl_st));
	if(!pDbgCtrl){
		return NULL;
	}
	EigrpPortMemSet((U8 *)pDbgCtrl, 0, sizeof(EigrpDbgCtrl_st));
	pDbgCtrl->id	= id;
	pDbgCtrl->term	= pTerm;
	if(name){
		EigrpPortStrCpy(pDbgCtrl->name, name);
	}
	pDbgCtrl->flag	= 0;
	EigrpUtilDllInsert((EigrpDll_pt)pDbgCtrl, NULL, (EigrpDll_pt *)&gpEigrp->dbgCtrlLst);
	
	return pDbgCtrl;
}

EigrpDbgCtrl_pt	EigrpUtilDbgCtrlFind(U8 *name, void *pTerm, U32 id)
{
	EigrpDbgCtrl_pt	pDbgCtrlTmp;
	
	if(!name && !pTerm && !id){
		return NULL;
	}
	for(pDbgCtrlTmp = gpEigrp->dbgCtrlLst; pDbgCtrlTmp; pDbgCtrlTmp = pDbgCtrlTmp->forw){
		if(name){
			if(EigrpPortStrCmp(name, pDbgCtrlTmp->name)){
				continue;
			}
			if(pTerm != pDbgCtrlTmp->term){
				continue;
			}
			if(id != pDbgCtrlTmp->id){
				continue;
			}
			
			break;

		}else{
 			if(pTerm != pDbgCtrlTmp->term){
				continue;
			}
			if(id != pDbgCtrlTmp->id){
				continue;
			}
			
			break;
		}
		
	}
	
	if(pDbgCtrlTmp){
		return pDbgCtrlTmp;
	}

	return NULL;
	
}

S32	EigrpUtilDbgCtrlDel(U8 *name, void *pTerm, U32 id)
{
	EigrpDbgCtrl_pt	pDbgCtrlTmp;
	
	if(!name && !pTerm && !id){
		return FAILURE;
	}
	for(pDbgCtrlTmp = gpEigrp->dbgCtrlLst; pDbgCtrlTmp; pDbgCtrlTmp = pDbgCtrlTmp->forw){
		if(name){
			if(EigrpPortStrCmp(name, pDbgCtrlTmp->name)){
				continue;
			}
			if(pTerm != pDbgCtrlTmp->term){
				continue;
			}
			if(id != pDbgCtrlTmp->id){
				continue;
			}
			
			break;

		}else{
			if(pTerm != pDbgCtrlTmp->term){
				continue;
			}
			if(id != pDbgCtrlTmp->id){
				continue;
			}
			
			break;
		}
		
	}

	if(pDbgCtrlTmp){
		EigrpUtilDllRemove((EigrpDll_pt)pDbgCtrlTmp, (EigrpDll_pt *)&gpEigrp->dbgCtrlLst);
		EigrpUtilDbgCtrlFree(pDbgCtrlTmp);
		return SUCCESS;
	}

	return FAILURE;
}

void EigrpUtilOutputBuf(U32 type, U8 * buffer)
{
	EigrpDbgCtrl_pt	pDbgTmp;
	
	for(pDbgTmp = gpEigrp->dbgCtrlLst; pDbgTmp; pDbgTmp = pDbgTmp->forw){
		if(pDbgTmp->flag & type){
			EigrpPortPrintDbg(buffer, pDbgTmp);
		}
	}

	return;
}

S32	EigrpUtilConvertStr2Ipv4(U32 *pIp, U8 *str)
{
	U8	*temPoint, temIp[4], *newStart;
	U32	strLen, dots, temDigital;

	temPoint	= str;
	newStart	= str;
	strLen	= EigrpPortStrLen(str);
	dots		= 0;

	if(strLen < 7 || strLen>16){
		return FAILURE;
	}
	
	while(temPoint < str + strLen){
		if(*temPoint != '.' && (*temPoint <'0' || *temPoint > '9')){
			return FAILURE;
		}
		if(*temPoint == '.'){
			if(*(temPoint + 1) == '.' || *(temPoint + 1) == '\0'){
				return FAILURE;
			}

			*temPoint	= 0;
			temDigital	= EigrpPortAtoi(newStart);
			dots++;
			if(temDigital > 255 || dots >3 || (temDigital == 0 && dots == 1)){
				return FAILURE;
			}

			temIp[dots - 1]	= temDigital;
			
			*temPoint	= '.';
			temPoint++;
			newStart	= temPoint;
		}

		temPoint++;
	}

	if(*temPoint != '\0' || dots != 3 || newStart == temPoint){
		return FAILURE;
	}else{
		temDigital	= EigrpPortAtoi(newStart);
		if(temDigital > 255){
			return FAILURE;
		}
		temIp[3]	= temDigital;
	}
	*pIp	= temIp[0] << 24 | temIp[1] << 16 | temIp[2] << 8 | temIp[3];

	return SUCCESS;
}

S32	EigrpUtilConvertStr2Ipv4Mask(U32 *pMask, U8 *str)
{
	S32	retVal;
	U32	mask1, mask2;

	retVal	= EigrpUtilConvertStr2Ipv4(pMask, str);
	if(retVal != SUCCESS){
		*pMask	= 0;
		return FAILURE;
	}

	mask1	= *pMask;
	mask2	= ~(*pMask) + 1;
	if((mask1 & mask2) != mask2){		/*Mask hole exist.*/
		*pMask	= 0;
		return FAILURE;
	}

	return SUCCESS;
}

#ifdef _EIGRP_PLAT_MODULE_TIMER
//��ȡ��ǰϵͳ�¼�
S32	EigrpUtilTimervalGet(struct timeval *val)
{
#if(EIGRP_OS_TYPE==EIGRP_OS_VXWORKS)
	
	int ret = -1;
	struct timespec rtp;
	ret = clock_gettime (CLOCK_MONOTONIC, &rtp);
	if(ret == -1){
		return	FAILURE;
	}
	val->tv_sec = rtp.tv_sec;
	val->tv_usec = rtp.tv_nsec / 1000;
	/*
	int tick = tickGet();
	int ptick = sysClkRateGet();
	val->tv_sec	= tick / ptick;
	val->tv_usec = tick % ptick;
	val->tv_usec = (long)((double)val->tv_usec * (double)(1000/ptick));
	val->tv_usec = val->tv_usec *1000;
	*/
#elif(EIGRP_OS_TYPE==EIGRP_OS_LINUX)

	S32	retVal;
	retVal = gettimeofday(val, NULL);
	if(retVal){
		return	FAILURE;
	}
#endif
	return	SUCCESS;
}
//��鵱ǰʱ���Ƿ��Ѿ����ﶨʱ�ж�ʱ��
static S32 EigrpUtilTimerCheck(EigrpLinuxTimer_pt timetbl, struct timeval *now)
{
	struct timeval val;
	if( (timetbl == NULL)||(now == NULL) )
		return	FAILURE;
	
	val.tv_sec = timetbl->tv.tv_sec + timetbl->interuppt_tv.tv_sec;
	val.tv_usec = timetbl->tv.tv_usec + timetbl->interuppt_tv.tv_usec;
	//
	if(val.tv_sec < now->tv_sec)//
		timetbl->active = TRUE;
	else if ((val.tv_sec = now->tv_sec)&&(val.tv_usec <= now->tv_usec) )//
		timetbl->active = TRUE;
			
	if(timetbl->active == TRUE)
	{
		//��ǰʱ���Ƿ��Ѿ����ﶨʱ�ж�ʱ�䣬�����´ζ�ʱ�ж�ʱ��
		timetbl->interuppt_tv.tv_sec = now->tv_sec;
		timetbl->interuppt_tv.tv_usec = now->tv_usec;
		return SUCCESS;
	}
	return	FAILURE;
}
//���ȶ�ʱ��
S32	EigrpUtilTimerCallRunning(EigrpLinuxTimer_pt timetbl, int max)
{
	int index = 0;
	struct timeval now;
	memset(&now, 0, sizeof(now));
	if(EigrpUtilTimervalGet(&now)!=SUCCESS)//��ȡ��ǰʱ��
		return FAILURE;

	for(index = 0; index < max; index++)
	{
		if(timetbl[index].used == TRUE)//��ʱ���Ѿ�ʹ��
		{
			if(EigrpUtilTimerCheck(&timetbl[index], &now) == SUCCESS)//���ﶨʱʱ��
			if(timetbl[index].active == TRUE)
			{
				(timetbl[index].func)((U32)timetbl[index].para);//���ȶ�ʱ�жϺ���
				//timetbl[index].count --;
				//printf("%s\n",__func__);
				//EigrpUtilTimervalGet(&(timetbl[index].interuppt_tv));
				timetbl[index].active = FALSE;//�����ʱ��
				timetbl[index].used = FALSE;
			}
		}
	}	
	return SUCCESS;
}
#endif//_EIGRP_PLAT_MODULE_TIMER


#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
#if 0
void	EigrpUtilLinuxTimerCheck()
{
	EigrpLinuxTimer_pt pTimer, pTem;
	U32	timeSec;

	EIGRP_FUNC_ENTER(EigrpUtilLinuxTimerCheck);

	timeSec	= EigrpPortGetTimeSec();
	EigrpPortSemBTake(gpEigrp->timerSem);
	
	pTimer = gpEigrp->timerHdr;
	while(pTimer){
		if(pTimer->trigSec < timeSec){
			EigrpPortSemBGive(gpEigrp->timerSem);
			(*pTimer->func)(pTimer->para);
			EigrpPortSemBTake(gpEigrp->timerSem);

			/**ȷ�ϸոմ����Ķ�ʱ��û�б�ɾ��**/
			for(pTem = gpEigrp->timerHdr; pTem; pTem = pTem->forw){
				if(pTem == pTimer){
					break;
				}
			}
			if(pTem){
				pTimer->trigSec	= pTimer->periodSec + timeSec;
			}
			/**ֻҪ����������ʱ��������п��ܱ��޸ģ��ʹ�ͷ������**/
			pTimer = gpEigrp->timerHdr;
			continue;
		}

		pTimer = pTimer->forw;
	}

	EigrpPortSemBGive(gpEigrp->timerSem);

	EIGRP_FUNC_LEAVE(EigrpUtilLinuxTimerCheck);
	return;
}
#endif
#endif

U32	EigrpUtilGetHelloHoldTime(EigrpPktHdr_st *eigrp, S32 paklen)
{

	EigrpGenTlv_st	*tlv_hdr;
	EigrpParamTlv_st	*param;
	U16	length;
	U32	new_peer, error_found;

	EIGRP_FUNC_ENTER(EigrpUtilGetHelloHoldTime);
	paklen -= EIGRP_HEADER_BYTES;
	if(paklen <= 0){
		EIGRP_FUNC_LEAVE(EigrpUtilGetHelloHoldTime);
		return 0;
	}

	tlv_hdr = (EigrpGenTlv_st *)(eigrp + 1);

	new_peer = FALSE;
	while(paklen > 0){
		switch(NTOHS(tlv_hdr->type)){
			case EIGRP_PARA:
				/* Process Parameter TLV. */
				param = (EigrpParamTlv_st *) tlv_hdr;
				
				EIGRP_FUNC_LEAVE(EigrpUtilGetHelloHoldTime);
				return NTOHS(param->holdTime);

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

	EIGRP_FUNC_LEAVE(EigrpUtilGetHelloHoldTime);

	return 0;
}

void	EigrpUtilChgHelloHoldTime(void *eigrp, S32 paklen, U32 holdTime)
{
	EigrpGenTlv_st	*tlv_hdr;
	EigrpParamTlv_st	*param;
	EigrpPktHdr_st	*pEigrp;
	U16	length;
	U32	new_peer, error_found;

	EIGRP_FUNC_ENTER(EigrpUtilGetHelloHoldTime);
	paklen -= EIGRP_HEADER_BYTES;
	if(paklen <= 0 ||!holdTime){
		EIGRP_FUNC_LEAVE(EigrpUtilGetHelloHoldTime);
		return;
	}

	pEigrp = (EigrpPktHdr_st *)eigrp;
	tlv_hdr = (EigrpGenTlv_st *)((EigrpPktHdr_st *)eigrp + 1);

	new_peer = FALSE;
	while(paklen > 0){
		switch(NTOHS(tlv_hdr->type)){
			case EIGRP_PARA:
				/* Process Parameter TLV. */
				param = (EigrpParamTlv_st *) tlv_hdr;
				param->holdTime = HTONS(holdTime);
				EigrpGenerateChksum(pEigrp, paklen);
				EIGRP_FUNC_LEAVE(EigrpUtilGetHelloHoldTime);
				return;

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

	EIGRP_FUNC_LEAVE(EigrpUtilGetHelloHoldTime);

	return;
}

S32	EigrpUtilGetPktInfo(U8 *pktBuf, U32 bufLen, S32 *isEigrp, S32 *isSeqed, S32 *isHello, U32 *holdTime, U32 *seq)
{
	EigrpUdpHdr_pt	pUdp;
	EigrpPktHdr_pt	pEigrp;

	if(bufLen < sizeof(EigrpIpHdr_st) + sizeof(EigrpUdpHdr_st) + sizeof(EigrpPktHdr_st)){
		return FAILURE;
	}

	*isEigrp	= TRUE;
	*isSeqed	= FALSE;
	*isHello	= FALSE;
	
	pUdp	= (EigrpUdpHdr_pt)(pktBuf + sizeof(EigrpIpHdr_st));
	if(pUdp->dstPort!= HTONS(EIGRP_LOCAL_PORT)){
		*isEigrp	= FALSE;
		return SUCCESS;
	}

	pEigrp	= (EigrpPktHdr_pt)((U8 *)pUdp + sizeof(EigrpUdpHdr_st));
	if(pEigrp->opcode == EIGRP_OPC_HELLO){
		*isHello	= TRUE;
		*holdTime	= EigrpUtilGetHelloHoldTime(pEigrp, bufLen - sizeof(EigrpIpHdr_st));
	}

	if(pEigrp->sequence){
		*isSeqed	= TRUE;
		*seq		= NTOHL(pEigrp->sequence); 
	}

	return SUCCESS;
}

/* tigerwh 120527 */
void	EigrpUtilDndbHealthy(U8 *fileName, U32 lineNum)
{
	EigrpDualNdb_st	*dndb, *next_dndb;
	S32	index;
	EigrpPdb_st *pdb;
	EigrpDualDdb_st	*ddb;
	//printf("EigrpUtilDndbHealthy: fileName:%s, lineNum:%d\n", fileName, lineNum);
	for(pdb = (EigrpPdb_st*)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		ddb= pdb->ddb;
		if(ddb->activeTime == 0){
			continue;
		}

		for(index = 0; index < EIGRP_NET_HASH_LEN; index++){
			dndb = ddb->topo[ index ];
			while(dndb != NULL){
				next_dndb = dndb->next;
				if(next_dndb > 0x0fffffff){
					_EIGRP_DEBUG("EigrpUtilDndbHealthy: fileName:%s, lineNum:%d\n", fileName, lineNum);
					EigrpPortAssert(0, "EigrpUtilDndbHealthy: next_dndb error.\n");
				}
				dndb = next_dndb;
			}
		}
	}

	return;
}
#ifdef _DC_
void	EigrpUitlDcSchedAdd()
{
	EIGRP_FUNC_ENTER(EigrpUitlDcSchedAdd);
	
	EigrpUtilSchedAdd((S32 (*)(void *))EigrpProcDcPkt, NULL);
	EIGRP_FUNC_LEAVE(EigrpUitlDcSchedAdd);
	return;
}
#endif
