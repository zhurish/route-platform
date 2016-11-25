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

	Module:	EigrpDual

	Name:	EigrpDual.c

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

extern	Eigrp_pt	gpEigrp;

/************************************************************************************

	Name:	EigrpDualQueryOrigin2str

	Desc:	This function is to return a string describing the origin of the query.

			The origin could be:
				1. Clear			
				2. Local			
				3. Multi			
				4. Successor		
		
	Para: 	qo		- the origin of the query
	
	Ret:		pointer to the string describing the origin of the query.
************************************************************************************/

S8 *EigrpDualQueryOrigin2str(U32 qo)
{
	static S8 *const dual_qo_str[ EIGRP_DUAL_QOCOUNT ]	=
		{"Clear", "Local origin", "Multiple Origins", "Successor Origin"};

	EIGRP_FUNC_ENTER(EigrpDualQueryOrigin2str);
	
	if(qo >= EIGRP_DUAL_QOCOUNT){
		EIGRP_FUNC_LEAVE(EigrpDualQueryOrigin2str);
		return NULL;
	}

	EIGRP_FUNC_LEAVE(EigrpDualQueryOrigin2str);
	
	return(dual_qo_str[ qo ]);
}

/************************************************************************************

	Name:	EigrpDualRouteType2string

	Desc:	This function is to return a string describing the origion of the given route.
 
 			The origion could be:
				1. Internal 
				2. Connected
				3. Redistributed	
				4. Summary
		
	Para: 	rtorigin		- the origin of the given route
	
	Ret:		pointer to the string describing the origion of the given route.
************************************************************************************/

S8 *EigrpDualRouteType2string(U32 rtorigin)
{
	static S8 *const dual_rto_str[ EIGRP_ORG_COUNT]	=
		{"Internal", "Connected", "Redistributed", "Summary"};

	EIGRP_FUNC_ENTER(EigrpDualRouteType2string);
	if(rtorigin >= EIGRP_ORG_COUNT){
		EIGRP_FUNC_LEAVE(EigrpDualRouteType2string);
		return NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRouteType2string);
	
	return dual_rto_str[rtorigin];
}

/************************************************************************************

	Name:	EigrpDualRouteEvent2String

	Desc:	This function is to return a string describing the type of the event.

			The type could be:
				1. Route Up		This means that a new route comes.
				2. Route Down	This means that a old route should be delete.
				3. Route Changed	This means that a route changes its parameters.
		
	Para: 	type		- type of the event
	
	Ret:		pointer to the string describing the type of the event.
************************************************************************************/

S8 *EigrpDualRouteEvent2String(U32 type)
{
	static S8 *const dual_rtevent_str[ EIGRP_DUAL_RTEVCOUNT]	=
		{"Route Up", "Route Down", "Route Changed"};

	EIGRP_FUNC_ENTER(EigrpDualRouteEvent2String);
	if(type >= EIGRP_DUAL_RTEVCOUNT){
		EIGRP_FUNC_LEAVE(EigrpDualRouteEvent2String);
		return NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRouteEvent2String);
	
	return dual_rtevent_str[ type];
}

/************************************************************************************

	Name:	EigrpDualSernoEarlier

	Desc:	This function is  just a simple unsigned compare, to check if A is less than B.
					
	Para: 	serno_a		- serial number A
			serno_b		- serial number B
	
	Ret:		TRUE 	for serial number A is before serial number B;
			FALSE	for otherwise
************************************************************************************/

S32	EigrpDualSernoEarlier(U32 serno_a, U32 serno_b)
{
	return(serno_a < serno_b);
}

/************************************************************************************

	Name:	EigrpDualSernoLater

	Desc:	This function is just a simple unsigned compare, to check if A is greater than B. 
			Returns TRUE if serial number A is after serial number B,
			else return FALSE.
		
	Para: 	serno_a		- serial number A
			serno_b		- serial number B
	
	Ret:		TRUE 	for serial number A is after serial number B;
			FALSE	for otherwise

************************************************************************************/

S32	EigrpDualSernoLater(U32 serno_a, U32 serno_b)
{
	return (serno_a > serno_b);
}

/************************************************************************************

	Name:	EigrpDualThreadToDrdb

	Desc:	This function returns a pointer to a DRDB, given a pointer to the embedded thread
			entry.
			The caller is responsible for making sure that the thread entry is in fact embedded 
			in a DRDB.
			All this optimizes down to a simple indexed reference.
		
	Para: 	thread		- pointer to the embedded thread entry
			
	Ret:		pointer to the DRDB
************************************************************************************/

EigrpDualRdb_st *EigrpDualThreadToDrdb(EigrpXmitThread_st *thread)
{
	S8				*ptr;
	U32			offset;
	EigrpDualRdb_st	*drdb;
	
	EIGRP_FUNC_ENTER(EigrpDualThreadToDrdb);	
	offset	= EIGRP_MEMBER_OFFSET(EigrpDualRdb_st, thread);
	ptr		= ((S8 *)thread) - offset;
	drdb		= (EigrpDualRdb_st*)((void *)ptr);
	EIGRP_FUNC_LEAVE(EigrpDualThreadToDrdb);
	
	return drdb;
}

/************************************************************************************

	Name:	EigrpDualThreadToDndb

	Desc:	This function returns a pointer to a DNDB, given a pointer to the embedded thread
 			entry.
			The caller is responsible for making sure that the thread entry is in  fact embedded
			in a DNDB.
			All this optimizes down to a simple indexed reference.
		
	Para: 	thread		- pointer to the embedded thread entry
				
	Ret:		pointer to the DNDB
************************************************************************************/

EigrpDualNdb_st *EigrpDualThreadToDndb(EigrpXmitThread_st *thread)
{
	EigrpDualNdb_st	*dndb;
	S8	*ptr;
	U32	offset;

	EIGRP_FUNC_ENTER(EigrpDualThreadToDndb);
	offset	= EIGRP_MEMBER_OFFSET(EigrpDualNdb_st, xmitThread);
	ptr		= ((S8 *)thread) - offset;
	dndb	= (EigrpDualNdb_st*)((void *)ptr);
	EIGRP_FUNC_LEAVE(EigrpDualThreadToDndb);
	
	return(dndb);
}

/************************************************************************************

	Name:	EigrpDualXmitThreaded

	Desc:	This function checks if the entry is on a thread. 
		
	Para: 	entry	- pointer to the given thread
			
	Ret:		TRUE 	for the entry is on a thread;
			FALSE	for otherwise		
************************************************************************************/

S32	EigrpDualXmitThreaded(EigrpXmitThread_st *entry)
{
	return(entry->prev != NULL);
}

/************************************************************************************

	Name:	EigrpDualIidbOnQuiescentList

	Desc:	This function checks if the interface iidb  is on the quiescent list.
					
	Para: 	iidb		- pointer to the interface iidb
	
	Ret:		TRUE 	for the IIDB is on the quiescent list; 
			FALSE 	for otherwise
************************************************************************************/

S32	EigrpDualIidbOnQuiescentList(EigrpIdb_st *iidb)
{
	return(iidb->prevQuiescent != NULL);
}

/************************************************************************************

	Name:	EigrpDualRemoveIidbFromQuiescentList

	Desc:	This function removes the IIDB from the quiescent list if it's there. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the IIDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRemoveIidbFromQuiescentList(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpDualRemoveIidbFromQuiescentList);
	if(EigrpDualIidbOnQuiescentList(iidb)){
		*iidb->prevQuiescent = iidb->nextQuiescent; /* Delink forward */
		if(iidb->nextQuiescent){
			iidb->nextQuiescent->prevQuiescent = iidb->prevQuiescent;
		}
		iidb->nextQuiescent	= NULL;
		iidb->prevQuiescent	= NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRemoveIidbFromQuiescentList);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualAddIidbToQuiescentList

	Desc:	This function add the IIDB to the quiescent list if it's not already there. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the IIDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualAddIidbToQuiescentList(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpDualAddIidbToQuiescentList);
	if(iidb->xmitAnchor || EigrpDualIidbOnQuiescentList(iidb)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualAddIidbToQuiescentList);
		return;
	}
	
	iidb->nextQuiescent	= ddb->quieIidb;
	iidb->prevQuiescent	= &ddb->quieIidb;
	ddb->quieIidb	= iidb;
	if(iidb->nextQuiescent){
		iidb->nextQuiescent->prevQuiescent = &iidb->nextQuiescent;
	}
	EIGRP_FUNC_LEAVE(EigrpDualAddIidbToQuiescentList);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualXmitUnthreadUnanchored

	Desc:	This function delink a transmit thread entry that is guaranteed to be unanchored. 
 
			Standard dual-linkage dance.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			entry	- pointer to the transmit thread entry
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualXmitUnthreadUnanchored(EigrpDualDdb_st *ddb, EigrpXmitThread_st *entry)
{
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualXmitUnthreadUnanchored);
	if(!EigrpDualXmitThreaded(entry)){
		EIGRP_FUNC_LEAVE(EigrpDualXmitUnthreadUnanchored);
		return;
	}

	if(entry->anchor){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualXmitUnthreadUnanchored);
		return;
	}

	/* This should twist your mind.  ;-)  The tail pointer(a **pointer) must always be non-null, 
	  * as it points to either the head pointer, or the next pointer in the last entry.  The
	  * contents of where the tail pointer points to must always be null, since it will point to 
	  * the next pointer of the last guy on the list. */
	if(!ddb->threadTail || *ddb->threadTail){ /* Can't happen */
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualXmitUnthreadUnanchored);
		return;
	}
	if(ddb->threadTail == &entry->next){ /* We're on the end */
		ddb->threadTail = entry->prev;
	}else{			/* Tail doesn't match us */
		if(!entry->next){	/* We better not be last */
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualXmitUnthreadUnanchored);
			return;
		}
	}

	/* Adjust the pointers of our neighbors. */
	*entry->prev = entry->next;
	if(entry->next){
		entry->next->prev = entry->prev;
	}
	entry->prev = NULL;
	entry->next = NULL;
	EIGRP_FUNC_LEAVE(EigrpDualXmitUnthreadUnanchored);

	return;
}

/************************************************************************************

	Name:	EigrpDualNextSerno

	Desc:	This function get next serial number. 
			Serial number 0 is never used (we special-case this value elsewhere).
			The serial number space cannot wrap at this time;  in order to make
			this work we have to renumber all of the thread entries, which is
			a significant pain.
 
 			We observe that if there are ten route changes per second, constantly,
			it will take over 23 years before the serial number space wraps.  So
			we punt.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		the next serial number
************************************************************************************/

U32	EigrpDualNextSerno(EigrpDualDdb_st *ddb)
{
	U32	retval;

	EIGRP_FUNC_ENTER(EigrpDualNextSerno);
	retval = ddb->nextSerNo;

	ddb->nextSerNo++;
	if(!ddb->nextSerNo){
		EIGRP_ASSERT(0);
		ddb->nextSerNo++;
	}
	EIGRP_FUNC_LEAVE(EigrpDualNextSerno);
	
	return	retval;
}

/************************************************************************************

	Name:	EigrpDualAnchorEntry

	Desc:	This function anchors a thread entry. 
			If the entry is unanchored, an anchor entry will be created. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			entry	- pointer to the thread
	
	Ret:		pointer to the anchor entry.
************************************************************************************/

EigrpXmitAnchor_st *EigrpDualAnchorEntry(EigrpDualDdb_st *ddb, EigrpXmitThread_st *entry)
{
	EigrpXmitAnchor_st	*anchor;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualAnchorEntry);
	anchor = entry->anchor;
	if(!anchor){
		anchor = (EigrpXmitAnchor_st *)EigrpUtilChunkMalloc(gpEigrp->anchorChunks);
		if(!anchor){
			EIGRP_FUNC_LEAVE(EigrpDualAnchorEntry);
			return NULL;	
		}
		anchor->thread	= entry;
		entry->anchor	= anchor;
	}

	anchor->count++;
	EIGRP_FUNC_LEAVE(EigrpDualAnchorEntry);
	
	return	anchor;
}

/************************************************************************************

	Name:	EigrpDualUnanchorEntry

	Desc:	This function releases an anchor. 
			If this is the last anchoring, we free the anchor structure.  If we're anchoring a
			dummy entry, the dummy entry is delinked and freed. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			entry	- pointer to the thread to be unachored
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualUnanchorEntry(EigrpDualDdb_st *ddb, EigrpXmitThread_st *entry)
{
	EigrpXmitAnchor_st	*anchor;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualUnanchorEntry);
	anchor = entry->anchor;
	if(!anchor){ 			/* We shouldn't even be here. */
		EIGRP_FUNC_LEAVE(EigrpDualUnanchorEntry);
		return;
	}

	anchor->count--;
	if(anchor->count < 0){	
		EIGRP_ASSERT(0);
		anchor->count = 0;
	}

	/* If it's the last reference, free the entry. */
	if(anchor->count == 0){
		entry->anchor		= NULL;
		anchor->thread	= NULL;	/* Keep things tidy. */
		EigrpUtilChunkFree(gpEigrp->anchorChunks, anchor);
		anchor = NULL;
	}

	/* If we freed the anchor and this is a dummy, free it. */
	if(anchor == NULL && entry->dummy){
		EigrpDualXmitUnthreadUnanchored(ddb, entry);
		EigrpUtilChunkFree(gpEigrp->dummyChunks, entry);
	}
	EIGRP_FUNC_LEAVE(EigrpDualUnanchorEntry);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualXmitUnthread

	Desc:	This function unthreads an entry from the DDB thread.  
			If the entry is anchored, it is replaced by a dummy entry.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			entry	- pointer to the entry to be unthreaded
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualXmitUnthread(EigrpDualDdb_st *ddb, EigrpXmitThread_st *entry)
{
	EigrpXmitAnchor_st	*anchor;
	EigrpXmitThread_st	*thread_dummy;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualXmitUnthread);
	if(!EigrpDualXmitThreaded(entry)){
		EIGRP_FUNC_LEAVE(EigrpDualXmitUnthread);
		return;				/* Nothing to do. */
	}

	anchor = entry->anchor;
	if(!anchor){			
		/* Simple case, unanchored. */
		EigrpDualXmitUnthreadUnanchored(ddb, entry);
		EIGRP_FUNC_LEAVE(EigrpDualXmitUnthread);
		return;
	}

	/* Allocate the dummy entry. */
	thread_dummy = (EigrpXmitThread_st *) EigrpUtilChunkMalloc(gpEigrp->dummyChunks);
	if(!thread_dummy){	
		EigrpDualXmitUnthreadUnanchored(ddb, entry);
		EIGRP_FUNC_LEAVE(EigrpDualXmitUnthread);
		return;
	}

	/* Copy the real entry into the dummy, including all pointers. */
	*thread_dummy = *entry;

	/* Mark the dummy as such.  Link the dummy and the anchor together. */
	thread_dummy->dummy	= TRUE;
	anchor->thread		= thread_dummy;

	/* Now clean up the original entry. */
	entry->anchor = NULL;	/*It's not anchored any longer. */
	EigrpDualXmitUnthreadUnanchored(ddb, entry);

	/* Now link the neighbors to the dummy(the other way is already done). */
	*thread_dummy->prev = thread_dummy;
	if(thread_dummy->next){
		thread_dummy->next->prev = &thread_dummy->next;
	}else{
		ddb->threadTail = &thread_dummy->next;
	}
	EIGRP_FUNC_LEAVE(EigrpDualXmitUnthread);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualFreeData

	Desc:	This function checks to see if the PDM data block needs to be freed.
 			If so, free the data block and set it to NULL.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			data		- doublepointer to the PDM data block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualFreeData(EigrpDualDdb_st *ddb, void **data)
{
	EIGRP_FUNC_ENTER(EigrpDualFreeData);
	if(*data != NULL){
		EigrpUtilChunkFree(ddb->extDataChunk, *data);
		*data = NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpDualFreeData);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualRdbDelete

	Desc:	This function remove an rdb from the topology table.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destinatiion network entry
			drdb		- pointer the the rdb which need to be freed
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRdbDelete(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb)
{
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualRdbDelete);
	/* Freak if this DRDB is threaded to a peer. */
	if(!drdb){
		EIGRP_FUNC_LEAVE(EigrpDualRdbDelete);
		return;
	}

	if(EigrpDualXmitThreaded(&drdb->thread)){ 
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualRdbDelete);
		return;
	}
	
	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RDBDELETE, dndb->dest, dndb->dest, drdb->nextHop);
	EigrpDualFreeData(ddb, &drdb->extData);

	EigrpPortMemFree(drdb);
	EIGRP_FUNC_LEAVE(EigrpDualRdbDelete);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualBuildRoute

	Desc:	This function converts dndb and drdb to a temp route.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb
			drdb		- pointer to the given drdb
			route	- pointer to the temp route
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualBuildRoute(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb, EigrpDualNewRt_st *route)
{
	EIGRP_FUNC_ENTER(EigrpDualBuildRoute);
	route->dest.address	= dndb->dest.address;
	route->dest.mask	= dndb->dest.mask;
	route->nextHop		= drdb->nextHop;
	route->origin		= drdb->origin;
	route->idb			= drdb->iidb;
	route->intf			= drdb->intf;
	EIGRP_FUNC_LEAVE(EigrpDualBuildRoute);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualZapDrdb

	Desc:	This function is used to remove routes(dndb,drdb) from both the topology table and the
			routing table.
   			  
 			EigrpDualZapDrdb is a dual internal function, it is not well to been invoked by other 
 			functions which are not in EigrpDual.c.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb 
			drdb		- pointer to the given drdb
			notify	- If it is TRUE,then signals the routing table of the event.
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualZapDrdb(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb, S32 notify)
{
	EigrpDualNewRt_st	route;
	EigrpDualRdb_st	*cur_drdb;
	EigrpDualRdb_st	**prev;
	S32	successors;

	EIGRP_FUNC_ENTER(EigrpDualZapDrdb);
	successors	= dndb->succNum;
	prev			= &dndb->rdb;
	cur_drdb		= *prev;
	while(cur_drdb){
		if(cur_drdb == drdb){
			*prev = drdb->next;/*将drdb从链表中删除*/
			if(dndb->succ == cur_drdb){
				dndb->succ = NULL;
			}
			EigrpDualBuildRoute(ddb, dndb, drdb, &route);
			EigrpDualRdbDelete(ddb, dndb, cur_drdb);
			if(successors > 0){
				dndb->succNum--;
			}
			if(notify && ddb->rtDelete){
				(*ddb->rtDelete)(ddb, &route);		/* EigrpIpRtDelete */
			}
			EIGRP_FUNC_LEAVE(EigrpDualZapDrdb);
			return;
		}
		prev = &cur_drdb->next;
		cur_drdb = *prev;
		successors--;
	}
	EIGRP_FUNC_LEAVE(EigrpDualZapDrdb);
	
	return;
}

static const S8 dual_removendb[]	= "DUAL: No routes.  Flushing dest %s";

/************************************************************************************

	Name:	EigrpDualDndbDelete

	Desc:	This function is used to remove an dndb from the topology table.  It's up to the caller
			to make sure the rdb's have all been freed.  Call the ddb cleanup routine if present.
			NULL out the supplied ddb pointer. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the dndb
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualDndbDelete(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp)
{
	EigrpDualNdb_st *dndb;
	EigrpDualNdb_st **prev;

	EIGRP_FUNC_ENTER(EigrpDualDndbDelete);
	dndb = *dndbp;
	if(!dndb){
		EIGRP_FUNC_LEAVE(EigrpDualDndbDelete);
		return;
	}

 	/* The thread refCnt must be zero and the DNDB must not be on the change queue. */
	if(dndb->xmitThread.refCnt || dndb->flagOnChgQue){ 
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualDndbDelete);
		return;
	}

	EigrpDualXmitUnthread(ddb, &dndb->xmitThread);

	prev = &ddb->topo[ (*ddb->ndbBucket)(&dndb->dest) ];
	while(*prev){
		if(*prev == dndb){
			*prev = dndb->next;
			dndb->next = NULL;
			EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_NDBDELETE, dndb->dest, dndb->dest, one);
			dndb->flagDel = TRUE;

			EigrpPortMemFree(dndb);
			dndb = (EigrpDualNdb_st*) 0;
			*dndbp = NULL;
			ddb->routes--; 
			EIGRP_FUNC_LEAVE(EigrpDualDndbDelete);
			return;
		}
		prev = &((*prev) ->next);
	}
	EIGRP_FUNC_LEAVE(EigrpDualDndbDelete);

	return;
}

/************************************************************************************

	Name:	EigrpDualCleanupDndb

	Desc:	This function cleans up a DNDB after transmission has completed. This consists of 
			clearing the appropriate sendflags mask, and if the destination is unreachable, destroy
			the DNDB.
 
 			The DNDB pointer will be nulled out if the DNDB is destroyed. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the dndb
			send_mask	- the appropriate sendflags mask
	
	Ret:		NONE	
************************************************************************************/

void	EigrpDualCleanupDndb(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp, U32 send_mask)
{
	EigrpDualNdb_st *dndb;

	EIGRP_FUNC_ENTER(EigrpDualCleanupDndb);
	dndb = *dndbp;
	dndb->sndFlag &= ~send_mask;
	if(dndb->sndFlag == 0 && dndb->rdb == NULL){
		EigrpDualDndbDelete(ddb, dndbp);
	}
	EIGRP_FUNC_LEAVE(EigrpDualCleanupDndb);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualMoveXmit

	Desc:	This function is used to move a xmit thread entry from its current position to the end
			of the thread.  
			If the entry is currently anchored in its current position, create a dummy thread entry
			before moving it. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			entry	- pointer to the given xmit thread
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualMoveXmit(EigrpDualDdb_st *ddb, EigrpXmitThread_st *entry)
{
	EIGRP_FUNC_ENTER(EigrpDualMoveXmit);
	/* Unthread the entry.  A dummy will be created if anchored. */
	EigrpDualXmitUnthread(ddb, entry);

	/* Now link the entry to the tail. */
	entry->prev		= ddb->threadTail;	/* Set prev */
	*ddb->threadTail	= entry;					/* Set prev's next */
	ddb->threadTail	= &entry->next;		/* Set tail */
	entry->serNo	= EigrpDualNextSerno(ddb);	/* Set serial # */
	entry->next		= NULL;					/* Just to make sure. */
	entry->anchor	= NULL;					/* Ditto */

	EIGRP_FUNC_LEAVE(EigrpDualMoveXmit);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualCleanupDrdb

	Desc:	This function is used to clean up a DRDB after a reply is acknowledged (or the peer is 
			being deleted). 

			Note that this may also delete the DNDB! Callers beware!
		
	Para: 	ddb		- pointer to the dual descriptor block 
			drdb		- pointer to the given DRDB
			zap_rdb	- If it is TRUE, the DRDB will be deleted if conditions are right;  if FALSE, only
					  the send flag will be manipulated.
	
	Ret:		NONe
************************************************************************************/

void	EigrpDualCleanupDrdb(EigrpDualDdb_st *ddb, EigrpDualRdb_st *drdb, S32 zap_rdb)
{
	EigrpDualNdb_st *dndb;
	S32 some_flag_set;

	EIGRP_FUNC_ENTER(EigrpDualCleanupDrdb);
	drdb->sndFlag &= ~EIGRP_DUAL_SEND_REPLY;
	dndb = drdb->dndb;
	if(!EigrpDualDndbActive(dndb) &&
			drdb->metric == EIGRP_METRIC_INACCESS &&
			drdb->sndFlag == 0 && zap_rdb){
		EigrpDualZapDrdb(ddb, dndb, drdb, TRUE);
	}

	/*  Now scan all DRDBs on this DNDB.  If none of them have send_flag set, clear the DNDB
	  * send_flag bit, and try to clean up the DNDB as well. */
	drdb	= dndb->rdb;
	some_flag_set	= FALSE;
	while(drdb){
		if(drdb->sndFlag){
			some_flag_set = TRUE;
			break;
		}
		drdb = drdb->next;
	}

	if(!some_flag_set){
		EigrpDualCleanupDndb(ddb, &dndb, EIGRP_DUAL_SEND_REPLY);
	}
	EIGRP_FUNC_LEAVE(EigrpDualCleanupDrdb);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualProcessAckedReply

	Desc:	This function is used to process the final acknowledgement of a reply. Unthread the 
			DRDBs as appropriate.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- unused
			pktDesc	- pointer to the packet descriptor
			peer		- pointer to the peer which the packet should be sent to 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualProcessAckedReply(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc, EigrpDualPeer_st *peer)
{
	EigrpXmitThread_st	*thread, *next_thread;
	EigrpXmitAnchor_st	*anchor;
	EigrpDualRdb_st		*drdb;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualProcessAckedReply);
	if(!peer){
		peer = pktDesc->peer;
	}
	if(!peer){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedReply);
		return;
	}
	anchor = pktDesc->pktDescAnchor;
	if(!anchor){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedReply);
		return;
	}

	/* Walk the thread, unlinking entries until we hit an entry that comes after the end of the
	  * packet. */
	thread = anchor->thread;
	while(thread){
		next_thread = thread->next;

		/* If we're past the end of the packet, bail. */
		if(EigrpDualSernoLater(thread->serNo, pktDesc->serNoEnd)){
			break;
		}
		if(!thread->dummy){
			if(!thread->drdb){
				EIGRP_ASSERT(0);
			}
			EigrpDualXmitUnthread(ddb, thread);
			
			/* Clear the DRDB send flag, and clean it up if it's all done. */
			drdb = EigrpDualThreadToDrdb(thread);
			EigrpDualLogXmit(ddb, EIGRP_DUALEV_REPLYACK, &drdb->dndb->dest,
					           &drdb->dndb->dest, &peer->source);
			EigrpDualCleanupDrdb(ddb, drdb, TRUE);
		}
		thread = next_thread;
	}

	/* Unanchor the packet start. */
	EigrpDualUnanchorEntry(ddb, anchor->thread);
	EIGRP_FUNC_LEAVE(EigrpDualProcessAckedReply);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualDropDndbRefcount

	Desc:	This function decrease the refCnt in the DNDB thread.  
			If it's gone to zero, clear the send_flags and see about deleting the DNDB.
 
 			Does nothing if the thread is a dummy.
 
 			This may delete the DNDB from the chain and free it! Do not reference the thread entry
 			or the DNDB in which it is embedded after calling this  routine! This function  decrease 
 			the refCnt in the DNDB thread.  If it's gone to zero, clear the send_flags and see
 			about deleting the DNDB.

			Does nothing if the thread is a dummy.

			This may delete the DNDB from the chain and free it! Do not reference the thread entry
			or the DNDB in which it is embedded after calling this routine!
 		
	Para: 	ddb		- pointer to the dual descriptor block 
			thread	- pointer to the DNDB thread
			pktDesc	- pointer to the packet descriptor
			iidb		- pointer to the EIGRP interface which contains interface on IP layer.
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualDropDndbRefcount(EigrpDualDdb_st *ddb, EigrpXmitThread_st *thread, EigrpPackDesc_st *pktDesc, EigrpIdb_st *iidb)
{
	EigrpDualNdb_st *dndb;
	U32 event;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualDropDndbRefcount);
	if(thread->dummy){
		EIGRP_FUNC_LEAVE(EigrpDualDropDndbRefcount);
		return;
	}

	if(thread->drdb){
		/* Shouldn't be here!*/
		EIGRP_ASSERT(0);
	}
	dndb = EigrpDualThreadToDndb(thread);
	if(pktDesc){
		if(pktDesc->opcode == EIGRP_OPC_UPDATE){
			event	= EIGRP_DUALEV_UPDATEACK;
		}else{
			event	= EIGRP_DUALEV_QUERYACK;
		}
	}else{
		if(dndb->sndFlag & EIGRP_DUAL_MULTI_UPDATE){
			event	= EIGRP_DUALEV_UPDATEACK;
		}else{
			event	= EIGRP_DUALEV_QUERYACK;
		}
	}
	EigrpDualLogXmit(ddb, event, &dndb->dest, &dndb->dest, &iidb->idb);
	thread->refCnt--;
	if(thread->refCnt < 0){ 
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualDropDndbRefcount);
		return;
	}

	if(thread->refCnt == 0){
		dndb->flagBeingSent	= FALSE; /* Everyone is done with it. */
		dndb->flagSplitHorizon	= TRUE; /* Ditto. */
		if(dndb->flagOnChgQue){ 
			/* Better not be. */
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualDropDndbRefcount);
			return;
		}

		/* Clear the split-horizon suppression flag only if this is an update. A bit of paranoia;
		  * we want to make absolutely sure that we don't leave any old info behind after going
		  * passive. */
		if(pktDesc && pktDesc->opcode == EIGRP_OPC_UPDATE){
			dndb->flagTellEveryone = FALSE; /* Done here as well. */
		}
		EigrpDualCleanupDndb(ddb, &dndb, EIGRP_DUAL_MULTI_MASK);
	}
	EIGRP_FUNC_LEAVE(EigrpDualDropDndbRefcount);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualProcessAckedMulticast

	Desc:	This function process the acknowledgement of a multicast update or query.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- the EIGRP interface on which the update or query was received
			pktDesc	- pointer to the packet descriptor
			peer		- unused
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualProcessAckedMulticast(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc, EigrpDualPeer_st *peer)
{
	EigrpXmitAnchor_st	*anchor;
	EigrpXmitThread_st	*thread, *next_thread;
	
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualProcessAckedMulticast);
	if(!iidb){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedMulticast);
		return;
	}

	/* It better be anchored! */
	anchor = pktDesc->pktDescAnchor;
	if(!anchor){
		/* Can't happen */
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedMulticast);
		return;
	}
	thread = anchor->thread;
	if(!thread){	
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedMulticast);
		return;
	}
	if(thread->serNo != pktDesc->serNoStart){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedMulticast);
		return;
	}

	/* Drop the refCnt in each DNDB acknowledged by the packet.  If the refCnt goes to zero, clear
	  * the sendflags on the DNDB and try cleaning it up. */
	while(thread){
		next_thread	= thread->next; /* We may free the DNDB! */

		/* Bail if we're past the end of the packet. */
		if(EigrpDualSernoLater(thread->serNo, pktDesc->serNoEnd)){
			break;
		}

		if(!thread->drdb){
			/* Ignore intermixed replies */
			EigrpDualDropDndbRefcount(ddb, thread, pktDesc, iidb);

		}
		thread	= next_thread;
	}

	/* Unanchor the start of the packet. */
	EigrpDualUnanchorEntry(ddb, anchor->thread);
	EIGRP_FUNC_LEAVE(EigrpDualProcessAckedMulticast);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualProcessAckedStartup

	Desc:	This function process the acknowledgement of a unicast update sent in the peer startup
			stream.
 
 			The fact that the startup process is still happening is tracked by  lastStartupSerNo;  
			if nonzero, we're still awaiting acknowledgement.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- unused
			pktDesc	- pointer to the packet descriptor
			peer		- pointer to the peer which the packet will be sent to 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualProcessAckedStartup(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc, EigrpDualPeer_st *peer)
{
	EigrpXmitAnchor_st *anchor;
	EigrpXmitThread_st *thread;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualProcessAckedStartup);
	/* There better be a peer! */
	if(!peer){
		peer = pktDesc->peer;
	}
	if(!peer){
		/* Can't happen */
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
		return;
	}

	/* By definition our INIT packet has been acked;  duly note it. */
	peer->flagNeedInitAck = FALSE;

	/* If the acknowledged packet is a dummy, we're done. */
	if(pktDesc->serNoStart == 0){
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
		return;
	}

	/* We better be expecting it. */
	if(!peer->lastStartupSerNo){
		/* Can't happen */
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
		return;
	}

	/* Get the anchored thread point. */
	anchor	= pktDesc->pktDescAnchor;
	if(!anchor){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
		return;
	}
	thread	= anchor->thread;
	if(thread->serNo != pktDesc->serNoStart){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
		return;
	}

	/* Log it.  We do funny checks because we don't really have a dest. */
	if(!ddb->flagDbgPeerSet || (*ddb->addressMatch)(&peer->source, &ddb->dbgPeer)){
		EigrpDualLogXmitAll(ddb, EIGRP_DUALEV_STARTUPACK, &peer->source, &pktDesc->serNoStart);
	}

	/* If the end of the acknowledged packet is after what should have been the end of the startup
	  * stream, there's a problem. */
	if(EigrpDualSernoLater(pktDesc->serNoEnd, peer->lastStartupSerNo)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
		return;
	}

	/* If the acknowledged packet ends at the end of the startup stream, we're done. */
	if(pktDesc->serNoEnd == peer->lastStartupSerNo){
		peer->lastStartupSerNo = 0;
	}

	/* Release the anchor. */
	if(anchor){
		EigrpDualUnanchorEntry(ddb, anchor->thread);
	}
	EIGRP_FUNC_LEAVE(EigrpDualProcessAckedStartup);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualPacketizeThread

	Desc:	This function is used to packetize thread into eigrp data packet for  send.
 
 			Walk the thread, until one packet's worth of information has been seen, or
			a target has been matched, or a packet type mismatch occurs.
 
 
 			If packet_type is NULL, this is a peer startup, and updates are sent
			for all passive DNDBs found on the thread.  Otherwise, a group of packets
			of the same type will be found and the packet type(in terms of a send_flag)
			is returned.

			A pointer to the peer is returned if the packet type is a reply.

			Returns the ending serial number and a pointer to just past the last
			entry put in the packet.  If nothing was put in the packet, the serial
			number will be zero.

			This routine will size worst-case packets;  it is assumed that an
			entry for every DNDB will be in the packet.  The application of filtering
			and split horizon rules is done at packet generation time for simplicity.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the EIGRP interface on which the pakcet will be sent
			peer		- doublepointer to the peer which the packet should be sent to 
			packet_type	- If packet_type is NULL, this is a peer startup, and updates are sent for
						   all passive DNDBs found on the thread.  Otherwise, a group of packets
						   of the same type will be found and the packet type(in terms of a 
						   send_flag)	is returned.
			thread	- pointer to the thread which is to be packetized
			target_serno	- If target_serno is 0, no target match is done.
			serNoEnd		- pointer to the ending serial number
	
	Ret:		pointer to just past the last entry put in the packet.

************************************************************************************/

EigrpXmitThread_st *EigrpDualPacketizeThread(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb,
													EigrpDualPeer_st **peer, U32 *packet_type,
													EigrpXmitThread_st *thread, U32 target_serno,
													U32 *serNoEnd)
{
	EigrpDualNdb_st		*dndb;
	EigrpDualRdb_st		*drdb;
	EigrpXmitThread_st	*prev_thread;
	U32	bytes_left, itemSize;
	S32	skip_entry, peerHandle;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualPacketizeThread);
	bytes_left	= iidb->maxPktSize - EIGRP_HEADER_BYTES;  /* Max payload */

	/* Compute packet size(including Auth TLV), 如果不包含AuthTLV, 配置了Auth后, 会导致 
	  * EigrpDualBuildPacket 报大小不够无论是否配置了Auth, 此处都应该减; 有可能此时没有配Auth,到
	  * dual_buildpacket时配了*/
	bytes_left	= bytes_left - sizeof(EigrpAuthTlv_st);

	prev_thread	= NULL;
	peerHandle	= EIGRP_NO_PEER_HANDLE;
	if(peer){
		*peer	= NULL;
	}

	while(thread){
		/* Stop if we've hit the target. */
		if(target_serno && !EigrpDualSernoEarlier(thread->serNo, target_serno)){
			break;
		}

		if(!thread->dummy){	/*If it's for real */

			/* Stop if we hit an entry with a different packet type to send. If it's the first time
			  * , note the packet type in the DNDB. Minor kludge--if no packet_type pointer is
			  * supplied, we will send all DNDBs, including ones with no send_flag set. This is 
			  * done for startup updates. */
			if(thread->drdb){	
				/*It's a DRDB (reply) */
				drdb		= EigrpDualThreadToDrdb(thread);
				dndb	= drdb->dndb;
			}else{
				drdb		= NULL;
				dndb	= EigrpDualThreadToDndb(thread);
			}
			skip_entry = FALSE;
			if(packet_type){
				/* Skip entries with no sendflag.  DRDBs will always be sent since they are on the
				  * thread ephemerally. */
				if(!drdb && !(dndb->sndFlag & EIGRP_DUAL_MULTI_MASK)){
					skip_entry = TRUE;
				}else{
					/* Any packet type previously determined? */
					if(*packet_type){
						/* If we have been sending Replies, the new entry must be a DRDB for the
						  * correct peer. */
						if(*packet_type == EIGRP_DUAL_SEND_REPLY){
							if(!drdb || (drdb->handle != peerHandle)){
								break;
							}
						}else{
							/* We've been sending updates or queries.  If this entry is a reply
							  * (DRDB), stop if it's on our interface(because we have to send it),
							  * or skip it otherwise. */
							if(drdb){
								if(drdb->iidb == iidb){
									break;
								}else{
									skip_entry = TRUE;
								}
							}else{
								/* Not a reply.  Stop if this entry is the wrong type. */
								if((dndb->sndFlag & (U32)EIGRP_DUAL_MULTI_MASK) != *packet_type){
									break;
								}
							}
						}
					}else{
						/* No previous packet type found.  If this is a reply, skip it if we're on
						  * the wrong interface. If it's the right interface, note the packet type
						  * and peer.
						  *
						  * For non-replies, note the packet type. */
						if(drdb){	
							if(drdb->iidb == iidb){
								*packet_type = EIGRP_DUAL_SEND_REPLY;
								peerHandle	= drdb->handle;
								if(peer){
									*peer = EigrpHandleToPeer(ddb, peerHandle);
								}
							}else{	
								skip_entry	= TRUE;
							}
						}else{
							*packet_type	= dndb->sndFlag & EIGRP_DUAL_MULTI_MASK;
						}
					}
				}
			}else{			/* !packet_type */
				/* This is a peer startup situation.  Skip DRDBs but don't skip any DNDBs. */
				if(drdb){
					skip_entry	= TRUE;
				}
			}

			if(!skip_entry){
				/* Got a live one.  Ask the PDM how big it will be. */
				itemSize	= (*ddb->itemSize)(dndb);

				/*If it's too big to fit, bail.  Otherwise, charge for it. */
				if(itemSize > bytes_left){
					break;
				}
				bytes_left -= itemSize;

				/* Mark as being sent, since we're committed to sending it. */
				dndb->flagBeingSent = TRUE;

				/* Log it if this is not a startup packet.  Startups are logged elsewhere. */
				if(packet_type){
					switch(*packet_type){
						case EIGRP_DUAL_MULTI_UPDATE:
							EigrpDualLogXmit(ddb, EIGRP_DUALEV_UPDATEPACK, &dndb->dest,
											&dndb->dest, &iidb->idb);
							break;
							
						case EIGRP_DUAL_MULTI_QUERY:
							EigrpDualLogXmit(ddb, EIGRP_DUALEV_QUERYPACK, &dndb->dest,
											&dndb->dest, &iidb->idb);
							break;
							
						case EIGRP_DUAL_SEND_REPLY:
							if(peer && *peer){	
								EigrpDualLogXmit(ddb, EIGRP_DUALEV_REPLYPACK, &dndb->dest,
												&dndb->dest, &(*peer)->source);
							}

							break;
							
						default:
							break;
					}
				}
			}
		}

		/* Move to the next thread entry. */
		prev_thread = thread;
		thread = thread->next;
	}

	/* Note the ending serial number, if there is anything here. */
	if(prev_thread){
		/* The packet has contents */
		*serNoEnd	= prev_thread->serNo;
	}else{
		/* Nothing there */
		*serNoEnd	= 0;
	}
	EIGRP_FUNC_LEAVE(EigrpDualPacketizeThread);
	
	return(thread);
}

/************************************************************************************

	Name:	EigrpDualOpcode

	Desc:	This function translates dual internal packet type to eigrp opcode type. 

			The  opcode could be:
				1. EIGRP_OPC_UPDATE 
				2. EIGRP_OPC_QUERY
				3. EIGRP_OPC_REPLY
				4. EIGRP_OPC_ILLEGAL
		
	Para: 	dual_ptype	- the dual internal packet type
	
	Ret:		eigrp opcode type
************************************************************************************/

S32	EigrpDualOpcode(S32 dual_ptype)
{
	EIGRP_FUNC_ENTER(EigrpDualOpcode);
	switch(dual_ptype){
		case EIGRP_DUAL_MULTI_UPDATE:
			EIGRP_FUNC_LEAVE(EigrpDualOpcode);
			return(EIGRP_OPC_UPDATE);
			
		case EIGRP_DUAL_MULTI_QUERY:
			EIGRP_FUNC_LEAVE(EigrpDualOpcode);
			return(EIGRP_OPC_QUERY);
			
		case EIGRP_DUAL_SEND_REPLY:
			EIGRP_FUNC_LEAVE(EigrpDualOpcode);
			return(EIGRP_OPC_REPLY);
			
		default:
			EIGRP_FUNC_LEAVE(EigrpDualOpcode);
			return(EIGRP_OPC_ILLEGAL);
	}
}

/************************************************************************************

	Name:	EigrpDualPacketizeInterface

	Desc:	This function is used to figure out the contents of the next packet for this interface,
			and call the transport to create and enqueue the packet descriptor for it.  
			The packet will actually be built when the transport calls us back through the buildPkt
			callback.	
 			We assume that the spot in the chain to packetize is pointed to by
			iidb->xmitAnchor. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the given interface on whch the packet will be sent 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualPacketizeInterface(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpXmitThread_st	*thread, *first_entry;
	EigrpPackDesc_st		*pktDesc;
	EigrpXmitAnchor_st	*anchor;
	EigrpDualPeer_st		*peer;
	EigrpDualRdb_st	*drdb;

	S8	ctemp[ 80 ];
	U32	packet_type, serNoStart, serNoEnd, opcode, event;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualPacketizeInterface);
	ctemp[0]	= '\0';

	anchor = iidb->xmitAnchor;
	if(!anchor){
		/* Can't happen */
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualPacketizeInterface);
		return;
	}

	/* We need to ignore multicast flow control if we're about to packetize a reply packet, but not
	  * if we're going to send a multicast.  In order to decide, we need to look at the type of the
	  * first entry that we're going to packetize. In order to do *that*, we have to skip any dummy
	  * entries on the thread, as well as any DRDBs(replies) that aren't for this interface.  So
	  * let's do that first. */
	thread = anchor->thread;
	while(thread){
		if(!thread->dummy){
			/* Not a dummy */
			if(!thread->drdb){ 	/* Not a reply */
				break;
			}
			drdb	= EigrpDualThreadToDrdb(thread);
			if(drdb->iidb == iidb) {  /* The right interface */
				break;
			}
		}
		thread	= thread->next;
	}
	/* return here no error */
	if(thread){
		/* Call the omnibus packetizer to do the work. */
		packet_type	= 0;
		serNoStart	= thread->serNo;
		first_entry	= thread;
		thread		= EigrpDualPacketizeThread(ddb, iidb, &peer, &packet_type, thread, 0, &serNoEnd);

		/* Figure out the EIGRP opcode. */
		opcode	= EigrpDualOpcode((S32)packet_type);
		if(opcode == EIGRP_OPC_ILLEGAL){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualPacketizeInterface);
			return;
		}

		if(opcode == EIGRP_OPC_REPLY){
			
			sprintf_s(ctemp, sizeof(ctemp),", peer %s", (*ddb->printAddress)(&peer->source));
		}

		EIGRP_TRC(DEBUG_EIGRP_INTERNAL, "Intf %s packetized %s %d-%d %s\n",
					iidb->idb ? (S8 *)iidb->idb->name : "Null0", EigrpOpercodeItoa((S32)opcode),
					serNoStart, serNoEnd, ctemp);

		/* Now ask the transport to enqueue the packet descriptor. */
		pktDesc	= EigrpEnqueuePak(ddb, peer, iidb, TRUE);

		/* Write down the good stuff in the packet descriptor. */
		if(pktDesc){
			pktDesc->opcode			= opcode;
			pktDesc->serNoStart		= serNoStart;
			pktDesc->serNoEnd		= serNoEnd;
			pktDesc->pktDescAnchor	= EigrpDualAnchorEntry(ddb, first_entry);
			EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
			if(peer){
				EigrpDualLogXport(ddb, EIGRP_DUALEV_REPLYENQ, &peer->source, &peer->source, &peer->iidb->idb);
			}else{
				if(opcode == EIGRP_OPC_UPDATE){
					event	= EIGRP_DUALEV_UPDATEENQ;
				}else{
					event	= EIGRP_DUALEV_QUERYENQ;
				}
				EigrpDualLogXportAll(ddb, event, &iidb->idb, NULL);
			}
		}

	}

	/* Re-anchor the IIDB at the new position. */
	if(thread){			/* More to go on the thread */
		iidb->xmitAnchor	= EigrpDualAnchorEntry(ddb, thread);
	}else{				/* End of the line */
		iidb->xmitAnchor	= NULL;
		EigrpDualAddIidbToQuiescentList(ddb, iidb);
	}
	/* Release the old IIDB anchor. */
	EigrpDualUnanchorEntry(ddb, anchor->thread);
	EIGRP_FUNC_LEAVE(EigrpDualPacketizeInterface);

	return;
}

/************************************************************************************

	Name:	EigrpDualProcessAckedPacket

	Desc:	This function is to process packet which has been acked. The final acknowledgement has
			arrived for a packet.  Clean up after the packet as appropriate.  
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the interface on which the ACK was received
			pktDesc	- pointer to the descriptor of the packet which has been acked
			peer		- pointer to the peer who has sent the ACK
				
	Ret:		TRUE	for the acked packet was processed.
************************************************************************************/

S32	EigrpDualProcessAckedPacket(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb,
										EigrpPackDesc_st *pktDesc, EigrpDualPeer_st *peer)

{
	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualProcessAckedPacket);
	/* Cleanup is according to packet type.  Unknown types are the purview of the PDM, and are
	  * handled elsewhere. */
	switch(pktDesc->opcode){
		case EIGRP_OPC_UPDATE:
			if(!pktDesc->flagPktDescMcast){ /* A startup update */
				EigrpDualProcessAckedStartup(ddb, iidb, pktDesc, peer);
			}else{			/* A multicast update */
				EigrpDualProcessAckedMulticast(ddb, iidb, pktDesc, peer);
			}
			break;

		case EIGRP_OPC_QUERY:
			EigrpDualProcessAckedMulticast(ddb, iidb, pktDesc, peer);
			break;

		case EIGRP_OPC_REPLY:
			EigrpDualProcessAckedReply(ddb, iidb, pktDesc, peer);
			break;

		default:
			EIGRP_FUNC_LEAVE(EigrpDualProcessAckedPacket);
			return FALSE;
	}

	EIGRP_FUNC_LEAVE(EigrpDualProcessAckedPacket);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpDualXmitContinue

	Desc:	This function is to see if there's something to do on an interface as far as
			transmission goes.

			If the PDM has any other data to send, we call the PDM first to see if it wishes to
			send anything or not.  We tell the PDM whether or not there are DUAL packets waiting
			to go, so that it can make the precedence decision.

			This routine is called when we get a "flow-ready" callback from the transport, and when
			the interface packetization timer expires.

			If completed_packet is non-NULL, it is a pointer to packet that has completed 
			transmission.  If completed_peer is non-null, the completed packet was acknowledged by
			that peer;  if it is NULL but completed_packet is non-null, the interface is no longer
			blocked for multicast by the packet. Note that this does not necessarily mean that the
			packet is done being transmitted everywhere;  if the refCnt in the packet descriptor is
			0, this means that the packet has been successfully been transmitted everywhere and the
			resources behind that packet may be released.  (The refCnt may be greater than 0 if the
			multicast transmission timer expired, for instance.)
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the given interface
			completed_packet		- If it is non-NULL, it is a pointer to packet  that has
								   completed transmission.
			completed_peer		- If it is non-null, the completed packet was acknowledged by that
								   peer;  if it is NULL but completed_packet is non-null, the 
								   interface is no longer blocked for multicast by the packet.
			
	Ret:		NONE
************************************************************************************/

void	EigrpDualXmitContinue(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb,
								EigrpPackDesc_st *completed_packet, EigrpDualPeer_st *completed_peer)
{
	S32	packets_pending;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualXmitContinue);
	/* Make sure that the DDB change queue has been flushed before going on.  This is necessary
	  * because we may here due to a peer going down(flushing out the peer transmit queue) in which
	  * case we may have half-sent updates pending. */
	EigrpDualFinishUpdateSend(ddb);

	/* If this packet has been fully ACKed, do any necessary cleanup. */
	if(completed_packet && !completed_packet->refCnt){
		if(EigrpDualProcessAckedPacket(ddb, iidb, completed_packet, completed_peer)){
			completed_packet	= NULL;
		}
	}

	/* See what's waiting. */
	packets_pending	= (iidb->xmitAnchor != NULL);

	/* Give the PDM a chance to handle the ACK and send another packet. */
	if(ddb->tansReady){
		(*ddb->tansReady)(ddb, iidb, completed_packet, completed_peer, packets_pending);	/*	EigrpIpTransportReady		*/
	}

	/* If there's something pending, go call the packetizer.  It may do nothing if flow-control
	  * conditions aren't conducive. */
	if(packets_pending){
		EIGRP_TRC(DEBUG_EIGRP_PACKET, "EIGRP: Packets pending on %s\n", iidb->idb ? (S8 *)iidb->idb->name : "Null0");
		EigrpDualPacketizeInterface(ddb, iidb);
	}
	EIGRP_FUNC_LEAVE(EigrpDualXmitContinue);

	return;
}

/************************************************************************************

	Name:	EigrpDualXmitDrdb

	Desc:	This function thread a DRDB for transmission. 
 			Links the DRDB onto the thread, and links the peer onto the IIDB for transmission and
 			anchors the chain if this was the first unpacketized DRDB on the thread. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			drdb		- pointer to the given DRDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualXmitDrdb(EigrpDualDdb_st *ddb, EigrpDualRdb_st *drdb)
{
	EigrpDualPeer_st		*peer;
	EigrpXmitThread_st	*thread;
	EigrpIdb_st	*iidb;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualXmitDrdb);
	peer	= EigrpHandleToPeer(ddb, drdb->handle);
	if(!peer){			
		EIGRP_FUNC_LEAVE(EigrpDualXmitDrdb);
		return;
	}
	iidb	= peer->iidb;

	/* (Re)link it to the end of the thread. */
	thread	= &drdb->thread;

	EigrpDualMoveXmit(ddb, thread);

	/* If the IIDB is quiescent, remove it from the quiescent list, anchor it at this entry, and
	  * kick the packetizing timer. */
	if(EigrpDualIidbOnQuiescentList(iidb)){
		if(iidb->xmitAnchor){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualXmitDrdb);
			return;
		}
		EigrpDualRemoveIidbFromQuiescentList(ddb, iidb);
		iidb->xmitAnchor	= EigrpDualAnchorEntry(ddb, thread);
		if(!EigrpUtilMgdTimerRunning(&iidb->pktTimer)){
			EIGRP_TRC(DEBUG_EIGRP_TIMER, "EIGRP-SYS: IIDB packetize timer started\n");
			EigrpUtilMgdTimerStart(&iidb->pktTimer, EIGRP_PACKETIZATION_DELAY);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualXmitDrdb);

	return;
}

/************************************************************************************

	Name:	EigrpDualXmitDndb

	Desc:	This function thread a DNDB for transmission.  
			Links the DNDB into the thread.  Anchors all quiescent IIDBs to this entry and starts 
			their packetization timers.
 
 			The DNDB pointer will be NULLed out if the DNDB is deleted.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the given DNDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualXmitDndb(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp)
{
	EigrpIdb_st		*iidb;
	EigrpXmitThread_st	*thread;
	EigrpDualNdb_st		*dndb;
	
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualXmitDndb);
	dndb	= *dndbp;
	thread	= &dndb->xmitThread;

	/* (Re)link it to the end of the thread. */
	EigrpDualMoveXmit(ddb, thread);

	/* Set the refCnt to the number of interfaces with live peers. */
	thread->refCnt = ddb->activeIntfCnt;

	/* Proceed if there's somebody to send this route to. */
	if(thread->refCnt){
		/* If the route is in the process of being sent in its old state and we don't know whether
		  * everyone has gotten it yet or not, set the "tell_everyone" flag to cause split horizon
		  * to be suppressed on the route.  Otherwise we could get into serious trouble if the new
		  * route is split horizoned on a different interface than the old one. */
		if(dndb->flagBeingSent){
			dndb->flagTellEveryone = TRUE;
			EigrpDualLog(ddb, EIGRP_DUALEV_OBE, &dndb->dest, &dndb->dest, &thread->refCnt);
		}

		/* Anchor all quiescent IIDBs to this entry. */
		while((iidb = ddb->quieIidb)){
			if(iidb->xmitAnchor){
				EIGRP_ASSERT(0);
				EIGRP_FUNC_LEAVE(EigrpDualXmitDndb);
				return;
			}
			iidb->xmitAnchor = EigrpDualAnchorEntry(ddb, thread);

			EigrpDualRemoveIidbFromQuiescentList(ddb, iidb);
			if(!EigrpUtilMgdTimerRunning(&iidb->pktTimer)){
				EIGRP_TRC(DEBUG_EIGRP_INTERNAL, "starting %s packetize timer\n", 
							iidb->idb ? (S8 *)iidb->idb->name : "Null0");

				EigrpUtilMgdTimerStart(&iidb->pktTimer, EIGRP_PACKETIZATION_DELAY);
			}
		}
	}else{				/* No peers anywhere */
		/* Nobody to tell.  Wipe out the send flag.  Note that it is not critical that we relinked
		  * this DNDB, as the order of the thread doesn't matter if there are no peers anywhere.
		  * I just like having the thread order reflect the temporal ordering of events. */
		EigrpDualCleanupDndb(ddb, dndbp, EIGRP_DUAL_MULTI_MASK);
	}
	EIGRP_FUNC_LEAVE(EigrpDualXmitDndb);

	return;
}

/************************************************************************************

	Name:	EigrpDualFinishUpdateSendGuts

	Desc:	This function install the contents of the change queue.  
 			This is the final phase of this kludge.  We have deferred rethreading DNDBs until an 
 			entire packet's worth of changes has been processed.  This is done because the receipt
 			of a reply that makes us go passive may result in a whole bunch of alternating 
 			reply/update pairs, resulting in a zillion little packets.  Instead, EigrpDualSendUpdate
 			stashes the changed DNDBs in the change queue, and then at the end of packet processing
 			they are all installed in the thread.  (Of course this means that everyone that	calls 
 			EigrpDualFC must play this game as well, but them's the breaks).
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualFinishUpdateSendGuts(EigrpDualDdb_st *ddb)
{
	EigrpDualNdb_st	*dndb;
	U32	count;
	
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualFinishUpdateSendGuts);
	/* Walk the change queue, moving the DNDBs into the thread. */
	count = 0;
	while((dndb = ddb->chgQue)){
		if(!dndb->flagOnChgQue){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualFinishUpdateSendGuts);
			return;
		}
		ddb->chgQue			= dndb->nextChg;
		dndb->flagOnChgQue	= FALSE;
		dndb->nextChg		= NULL;	/* Nice 'n' paranoid */
		EigrpDualXmitDndb(ddb, &dndb);
		count++;
	}
	ddb->chgQueTail = &ddb->chgQue; /*It's empty now */
	EigrpDualLogAll(ddb, EIGRP_DUALEV_EMPTYCHANGEQ, &count, NULL);
	EIGRP_FUNC_LEAVE(EigrpDualFinishUpdateSendGuts);

	return;
}

/************************************************************************************

	Name:	EigrpDualNewPeer

	Desc:	This function process the creation of a new peer.  We enqueue the topology database as 
			unicast packets for the peer, up to the current anchor point of the interface(everything 
			after that will be sent as multicast updates).
 
 			This routine periodically suspends!  Beware.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the the new peer
		
	Ret:		NONE
************************************************************************************/

void	EigrpDualNewPeer(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	EigrpXmitAnchor_st	*start_anchor;
	EigrpXmitThread_st	*thread;
	EigrpIdb_st		*iidb;
	EigrpPackDesc_st	*pktDesc;
	U32	target_serno, serNoStart, serNoEnd;
	S32	first_packet;
	U32	old_packet_size = 0;
	void	*idb;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualNewPeer);
	iidb	= peer->iidb;
	idb	= iidb->idb;
	EIGRP_DUAL_LOG_ALL(ddb, EIGRP_DUALEV_PEERUP, peer->source, idb);
	thread = ddb->threadHead;	/* Go to the head of the class */

	/* Save the old "iidb->maxPktSize" value ,and we use at least 1500 in startup update packets */
	old_packet_size = iidb->maxPktSize;
	if(iidb->maxPktSize < 1500){
		iidb->maxPktSize = 1500;
	}

	/* Run from the head of the chain until the current IIDB anchor point. */
	if(iidb->xmitAnchor){
		target_serno = iidb->xmitAnchor->thread->serNo;
	}else{
		target_serno = 0;
	}

	/* Loop until we get to the end of the road for this interface. */
	first_packet = TRUE;
	start_anchor = NULL;
	while(thread && thread->serNo != target_serno){
		/* Top of packet loop */

		/* Determine the serial number range for the next packet. */
		serNoStart = thread->serNo;
		start_anchor = EigrpDualAnchorEntry(ddb, thread);
		thread = EigrpDualPacketizeThread(ddb, iidb, NULL, NULL, thread, target_serno, &serNoEnd);

		/* Log it.  We do funny checks because we don't really have a dest. */
		if(!ddb->flagDbgPeerSet || (*ddb->addressMatch)(&peer->source, &ddb->dbgPeer)){
			EigrpDualLogXmitAll(ddb, EIGRP_DUALEV_STARTUPPACK, &peer->source, &serNoStart);
		}

		/* Bail from the loop if the packetizer returned nothing. */
		if(!serNoEnd){
			break;
		}

		/* Note the ending serial number in the peer structure.  This will come in handy in order
		  * to decide when we've seen all of our startup packets acked.	*/
		peer->lastStartupSerNo = serNoEnd;

		/* Enqueue the packet descriptor. */
		pktDesc = EigrpEnqueuePak(ddb, peer, iidb, TRUE);
		EigrpDualLogXport(ddb, EIGRP_DUALEV_STARTUPENQ, &peer->source, &peer->source, &iidb->idb);
		EigrpDualLogXport(ddb, EIGRP_DUALEV_STARTUPENQ2, &peer->source, &serNoStart, &pktDesc->ackSeqNum);

		/* Note the various parameters and set the INIT flag properly. */
		if(pktDesc){
			pktDesc->opcode			= EIGRP_OPC_UPDATE;
			pktDesc->serNoStart		= serNoStart;
			pktDesc->serNoEnd		= serNoEnd;
			pktDesc->flagSetInit		= first_packet;
			pktDesc->pktDescAnchor	= start_anchor;
			start_anchor				= NULL;
			first_packet				= FALSE;
			EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
		}

		/* Now the pause that refreshes. */
	}

	if(start_anchor){
		EigrpDualUnanchorEntry(ddb, start_anchor->thread);
	}

	/* If we haven't managed to send a packet yet, do so now(send a null UPDATE) as required by 
	  * law. */
	if(first_packet){
		pktDesc = EigrpEnqueuePak(ddb, peer, iidb, TRUE);
		if(pktDesc){
			pktDesc->opcode		= EIGRP_OPC_UPDATE;
			pktDesc->serNoStart	= pktDesc->serNoEnd = 0;
			pktDesc->flagSetInit	= TRUE;
			EigrpDualDebugEnqueuedPacket(ddb, peer, iidb, pktDesc);
		}
		peer->lastStartupSerNo = 0;
	}

 	/* Restore the old "iidb->maxPktSize" value  */
	if(old_packet_size){
		iidb->maxPktSize = old_packet_size;
	}

	/* Final validity check.  We better have hit the target if there was one. */
	if(target_serno){
		if(!thread || thread->serNo != target_serno){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualNewPeer);
			return;
		}
	}else{
		if(thread){
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpDualNewPeer);
			return;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualNewPeer);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualSplitHorizon

	Desc:	This function determine if split horizon should be used on this interface.  

					
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destination network entry
			iidb		- pointer to the given interface
	
	Ret:		TRUE  	for this destination should be suppressed due to the splithorizon rule
************************************************************************************/

S32	EigrpDualSplitHorizon(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpIdb_st *iidb)
{
	EigrpDualRdb_st	*drdb;
	S32	successors;

	EIGRP_FUNC_ENTER(EigrpDualSplitHorizon);
	/* This is an indication that we are active for this destination and the first entry may not be
	  * the successor before going active. */
	if(dndb->succ){
		drdb = dndb->succ;
		if(drdb->iidb == iidb){
			EIGRP_FUNC_LEAVE(EigrpDualSplitHorizon);
			return(iidb->splitHorizon);
		}
		EIGRP_FUNC_LEAVE(EigrpDualSplitHorizon);
		return FALSE;
	}

	successors	= dndb->succNum;
	drdb = dndb->rdb;
	while((drdb != NULL) && (successors > 0)){
		if(drdb->iidb == iidb){
			EIGRP_FUNC_LEAVE(EigrpDualSplitHorizon);
			return(iidb->splitHorizon);
		}
		drdb	= drdb->next;
		successors--;
	}

	EIGRP_FUNC_LEAVE(EigrpDualSplitHorizon);
	
	return FALSE;
}

/************************************************************************************

	Name:	EigrpDualShouldAdvertise

	Desc:	This function is  used to ask protocol dependent protocol if this destination should be 
			advertised out the interface.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destination network entry
			iidb		- pointer to the given interface
	
	Ret:		EIGRP_ROUTE_TYPE_ADVERTISE			for advertise it
			EIGRP_ROUTE_TYPE_SUPPRESS			for do not advertise it
			EIGRP_ROUTE_TYPE_POISON			for Advertise it with infinity metric 
************************************************************************************/

U32 EigrpDualShouldAdvertise(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpDualShouldAdvertise);
	if(!ddb->advertiseRequest){
		EIGRP_FUNC_LEAVE(EigrpDualShouldAdvertise);
		return(EIGRP_ROUTE_TYPE_ADVERTISE);
	}
	EIGRP_FUNC_LEAVE(EigrpDualShouldAdvertise);

	return ((*ddb->advertiseRequest)(ddb, dndb, iidb->idb));/*EigrpIpAdvertiseRequest*/
}

/************************************************************************************

	Name:	

	Desc:	This function is used to build a packet to be transmitted, based on the information 
 			saved in a packet descriptor.
 
 			Called by the transport when it's actually time to send a packet.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the interface the on which the packet will be sent
			peer		- pointer to the peer which the packet will be sent to
			pktDesc	- pointer to the packet descriptor
			packet_suppressed		- 
	
	Ret:		
************************************************************************************/

EigrpPktHdr_st *EigrpDualBuildPacket(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpDualPeer_st *peer,
										EigrpPackDesc_st *pktDesc, S32 *packet_suppressed)
{
	EigrpXmitThread_st	*thread;
	EigrpPktHdr_st	*eigrp;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	S8		*packet_ptr;
	U32		thread_type, bytes_left, itemSize, payload_size;
	S32		advertise_unreachable, advertise_item;
	void		*log_param;
	U32		event, temp;
	char 		ifName[64];
	EigrpIntf_pt pEigrpIntf;
	//if(app_debug)printf("EigrpDualBuildPacket(%s, %s) Enter\n", iidb->idb->name, EigrpOpercodeItoa(pktDesc->opcode));

	EIGRP_FUNC_ENTER(EigrpDualBuildPacket);
	log_param			= NULL;			
	event				= EIGRP_DUALEV_UNKNOWN;
	*packet_suppressed	= FALSE;
	payload_size			= 0;

	/* Decide what to do based on the packet type. */
	switch(pktDesc->opcode){
		/* First the DNDB-derived packets. */
		case EIGRP_OPC_UPDATE:
		case EIGRP_OPC_QUERY:
			if(pktDesc->opcode == EIGRP_OPC_UPDATE && !pktDesc->flagPktDescMcast){
				thread_type = EIGRP_STARTUP_THREAD;
			}else{
				thread_type = EIGRP_MCAST_THREAD;
			}
			break;

		/* Now the DRDB-derived packets. */
		case EIGRP_OPC_REPLY:
			thread_type = EIGRP_UCAST_THREAD;
			break;

		/* Anything else must belong to the PDM.  Call them. */
		default:
			if(!ddb->buildPkt){	
				EIGRP_ASSERT(0);
				*packet_suppressed = TRUE;
				EIGRP_FUNC_LEAVE(EigrpDualBuildPacket);
				return NULL;
			}
			eigrp = (*ddb->buildPkt)(ddb, peer, pktDesc, packet_suppressed);
			EIGRP_FUNC_LEAVE(EigrpDualBuildPacket);
			return eigrp;
	}

	if(pktDesc->pktDescAnchor){
		thread = pktDesc->pktDescAnchor->thread;
	}else{
		thread = NULL;
	}

	/* If there's nothing there, suppress the packet, unless this is the first packet of the
	  * startup thread(we have to send a null packet in that case). */
	if(!pktDesc->flagSetInit && !thread){
		*packet_suppressed = TRUE;
		EIGRP_FUNC_LEAVE(EigrpDualBuildPacket);
		return NULL;
	}

	/* Now construct the packet. */
	/* We use at least 1500 mtu in startup update packet */
	if((thread_type == EIGRP_STARTUP_THREAD) && (iidb->maxPktSize < 1500)){
		eigrp		= EigrpAllocPktbuff(1500);
		bytes_left	= 1500 - EIGRP_HEADER_BYTES;
	}else{
		eigrp		= EigrpAllocPktbuff((U16)(iidb->maxPktSize));
		bytes_left	= iidb->maxPktSize - EIGRP_HEADER_BYTES;
	}

	if(eigrp){
		packet_ptr	= (S8 *)EigrpPacketData(eigrp);
		if(iidb->authSet && (iidb->authMode == TRUE)){
			bytes_left	-= sizeof(EigrpAuthTlv_st);
			packet_ptr	+= sizeof(EigrpAuthTlv_st);
			payload_size	+= sizeof(EigrpAuthTlv_st);
		}

		/* Troll the thread until we're past the end of the packet. */
		while(thread){
			if(!thread->dummy){	/* It's real */
				/* Get the DNDB pointer. */
				if(thread->drdb){
					drdb		= EigrpDualThreadToDrdb(thread);
					dndb	= drdb->dndb;
				}else{
					dndb	= EigrpDualThreadToDndb(thread);
				}

				/* Apply the appropriate test. */
				advertise_unreachable = FALSE;
				advertise_item		= FALSE;
				log_param		= NULL;

				switch(thread_type){
					/* The peer startup update thread. */
					case EIGRP_STARTUP_THREAD:
						/* Send it if passive and not otherwise blocked. */
						if(!thread->drdb &&
								!EigrpDualDndbActive(dndb) &&
								dndb->rdb &&
								(EigrpDualShouldAdvertise(ddb, dndb, iidb) == EIGRP_ROUTE_TYPE_ADVERTISE) &&
								!EigrpDualSplitHorizon(ddb, dndb, iidb)){
							advertise_item	= TRUE;
							log_param		= &peer->source;
							event			= EIGRP_DUALEV_STARTUPXMIT;
						}
						break;

					/* Multicast updates and queries. */
					case EIGRP_MCAST_THREAD:
						if(!thread->drdb){
							/* If split horizon is enabled on the interface, don't send anything,
							  * except when the DNDB says to ignore split horizon(in which case we
							  * send an unreachable). */
							if(EigrpDualSplitHorizon(ddb, dndb, iidb)){
								if(!dndb->flagSplitHorizon || dndb->flagTellEveryone){
									advertise_item		= TRUE;
									advertise_unreachable	= TRUE;
								}
							}else{
								/* No split horizon.  Always advertise queries, but allow updates
								  * to be suppressed by filters. */
								switch(EigrpDualShouldAdvertise(ddb, dndb, iidb)){
									case EIGRP_ROUTE_TYPE_ADVERTISE:
										advertise_item	= TRUE;
										break;

									case EIGRP_ROUTE_TYPE_POISON:
										advertise_item		= TRUE;
										advertise_unreachable	= TRUE;
										break;

									case EIGRP_ROUTE_TYPE_SUPPRESS:
										if(pktDesc->opcode == EIGRP_OPC_QUERY){
											advertise_item		= TRUE;
											advertise_unreachable	= TRUE;
										}
										break;
								}
							}

							/* Last-ditch stupidity avoidance.  If this is a query, enforce the 
							  * split-horizoning determination we made when we went active. 
							  * In particular, if this is the split horizon interface, we must
							  * never send any query on it.  Conversely, if this is not the split
							  * horizon interface we must *always*send a query on it. */
							if(pktDesc->opcode == EIGRP_OPC_QUERY){
								if(dndb->shQueryIdb == iidb){
									if(advertise_item){
										advertise_item = FALSE;
										EigrpDualLogXmit(ddb,
														EIGRP_DUALEV_QSHON,
														&dndb->dest,
														&dndb->dest,
														&iidb->idb);
									}
								}else{
									if(!advertise_item){
										advertise_item		= TRUE;
										advertise_unreachable	= TRUE;
#if	EIGRP_DUAL_SPLIT_HORIZON_QUERIES	/* Don't whimper otherwise. */
										EigrpDualLogXmit(ddb, EIGRP_DUALEV_QSHOFF, &dndb->dest, &dndb->dest, &iidb->idb);
#endif

									}
								}
								event = EIGRP_DUALEV_QUERYXMIT;
							}else{
								event = EIGRP_DUALEV_UPDATEXMIT;
								if(EigrpDualDndbActive(dndb)){
									advertise_item = FALSE;
									EigrpDualLog(ddb, EIGRP_DUALEV_UPDATESQUASH,
												&dndb->dest, &dndb->dest, &iidb->idb);
								}
							}
							log_param = &iidb->idb;
						}
						break;

					case EIGRP_UCAST_THREAD:
						/* Always advertise, but make it unreachable if we are suppressing updates
						  * on this interface. */
						if(thread->drdb){
							advertise_item	= TRUE;
							if((EigrpDualShouldAdvertise(ddb, dndb, iidb) != EIGRP_ROUTE_TYPE_ADVERTISE) ||
									EigrpDualSplitHorizon(ddb, dndb, iidb)){
								advertise_unreachable	= TRUE;
							}
							log_param	= &peer->source;
							event = EIGRP_DUALEV_REPLYXMIT;
						}
						break;
				}

				/* tigerwh 120811 add begin */
				if(dndb->rdb->origin == EIGRP_ORG_CONNECTED && dndb->rdb->iidb->invisible == TRUE){
					advertise_item	= FALSE;
				}
				/* tigerwh 120811 add end */

				/* Whew.  If we're supposed to advertise this item, call the PDM to actually build
				  * the item.  If we're advertising a query and the active timer hasn't been 
				  * started, do it now.  (This keeps us from starting the active timer until we've 
				  * at least had an opportunity to transmit the packet.) */
				if(advertise_item){
#if LOCAL_WIREDNET_SUPRESS			
/***add by cwf 20130109  备用位置 (update不更新本地非用户口ip地址)*****/	
					if(NULL == EigrpIntfFindByDestNet(dndb->dest.address & dndb->dest.mask)) {
#endif
						itemSize = (*ddb->itemAdd)(ddb, iidb, dndb, packet_ptr, bytes_left, advertise_unreachable, pktDesc->opcode, peer);
						if(!itemSize){
							EigrpPortMemFree(eigrp);
							eigrp	= (EigrpPktHdr_st*) 0;
							EIGRP_ASSERT(0);
							EIGRP_FUNC_LEAVE(EigrpDualBuildPacket);
							return NULL;
						}
						packet_ptr += itemSize;
						bytes_left -= itemSize;
						payload_size += itemSize;
						if(pktDesc->opcode == EIGRP_OPC_QUERY && !EIGRP_TIMER_RUNNING(dndb->activeTime)){
							EIGRP_GET_TIME_STAMP(dndb->activeTime);
						}
#if LOCAL_WIREDNET_SUPRESS
/***add by cwf 20130109  备用位置 (update不更新本地非用户口ip地址)*****/
					}
#endif
				}
			}

			/* Advance to the next item on the thread.  Bail if it's past the end of our packet. */
			thread = thread->next;
			if(!thread){
				break;
			}
			if(EigrpDualSernoLater(thread->serNo, pktDesc->serNoEnd)){
				break;
			}	
		}		/* while(thread) */

		/* If nothing was added to the packet, and it's not the initial packet, suppress it. */
		if(!payload_size && !pktDesc->flagSetInit){
			EigrpPortMemFree(eigrp);
			eigrp = NULL;
			*packet_suppressed = TRUE;
		}
		pktDesc->length = payload_size;
	}
	EIGRP_FUNC_LEAVE(EigrpDualBuildPacket);

	return eigrp;
}

/************************************************************************************

	Name:	EigrpDualLastPeerDeleted

	Desc:	This function is used to handle the deletion of the last peer on an interface. All of the
			peers have already been cleaned up, and with them all of the outstanding unicast packets.
 
 			We need to clean up the IIDB queues, which will in turn undo all the	linkages.  Once
 			we do that, all that should remain is the transmit anchor(pointing at unpacketized 
 			DNDBs). We then walk the thread, dropping the refCnt on everything to show that we're
 			not going to send 'em.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the given interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualLastPeerDeleted(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpXmitThread_st	*thread, *next_thread;
	EigrpXmitAnchor_st	*anchor;
	EigrpDualNdb_st		*dndb;

	EIGRP_FUNC_ENTER(EigrpDualLastPeerDeleted);

	EigrpDualLogXmitAll(ddb, EIGRP_DUALEV_LASTPEER, &iidb->idb, NULL);

	/* Make sure there are no straggling half-sent Updates. */
	EigrpDualFinishUpdateSend(ddb);

	/* Clean up the IIDB queues. */
	EigrpFlushIidbXmitQueues(ddb, iidb);
	if(ddb->lastPeerDeleted){
		(*ddb->lastPeerDeleted)(ddb, iidb);
	}

	/* Walk down the thread from the transmit anchor, reducing the refCnt by 1 on each entry. This
	  * may clear the DNDB sendflag and in turn free the DNDB. */
	anchor	= iidb->xmitAnchor;
	if(anchor){
		thread	= anchor->thread;
		while(thread){
			next_thread	= thread->next;
			if(!thread->dummy && !thread->drdb){
				dndb	= EigrpDualThreadToDndb(thread);
				EigrpDualDropDndbRefcount(ddb, thread, NULL, iidb);
			}
			thread	= next_thread;
		}

		/* Free the anchor whilst here. */
		EigrpDualUnanchorEntry(ddb, anchor->thread);
		iidb->xmitAnchor	= NULL;
	}

	/* Note that the interface is now inactive. */
	ddb->activeIntfCnt--;

	/* Remove the interface from the quiescent list. */
	EigrpDualRemoveIidbFromQuiescentList(ddb, iidb);
	EIGRP_FUNC_LEAVE(EigrpDualLastPeerDeleted);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualFirstPeerAdded

	Desc:	This function handle the addition of the first peer on an interface.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the given interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualFirstPeerAdded(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpDualFirstPeerAdded);
	EigrpDualLogXmitAll(ddb, EIGRP_DUALEV_FIRSTPEER, &iidb->idb, NULL);
	ddb->activeIntfCnt++;		/* Note that the interface is active */
	EigrpDualAddIidbToQuiescentList(ddb, iidb);	/* Show that it's ready */
	EIGRP_FUNC_LEAVE(EigrpDualFirstPeerAdded);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualSuccessor

	Desc:	This function extract the successor from the dndb.
		
	Para: 	dndb	- pointer to the given dndb
	
	Ret:		pointer to the successor
************************************************************************************/

EigrpDualRdb_st *EigrpDualSuccessor(EigrpDualNdb_st *dndb)
{
	return (dndb->succ ? dndb->succ : dndb->rdb);
}

static const S8 state_transition[]	= "DUAL: Going from state %d to state %d";
/************************************************************************************

	Name:	EigrpDualState

	Desc:	This function is used to change the state of a dual destination.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the dual destination
			state	- pointer to the new state
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualState(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, U32 state)
{
	EIGRP_FUNC_ENTER(EigrpDualState);
	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_STATE, dndb->dest, dndb->origin, state);
	dndb->origin	= state;

	EIGRP_FUNC_LEAVE(EigrpDualState);
	return;
}

/************************************************************************************

	Name:	EigrpDualClearHandle

	Desc:	This function clear the bit in the reply-status table when we receive a reply.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destination network entry
			handle	- the handle index
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualClearHandle(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, S32 handle)
{
	U32 handle_count;

	EIGRP_FUNC_ENTER(EigrpDualClearHandle);
	if(handle == EIGRP_NO_PEER_HANDLE){
		EIGRP_FUNC_LEAVE(EigrpDualClearHandle);
		return;
	}

	if(EigrpTestHandle(ddb, &dndb->replyStatus, handle)){
		EigrpClearHandle(ddb, &dndb->replyStatus, handle);
		handle_count	= dndb->replyStatus.used;
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_CLEARHANDLE1, dndb->dest, dndb->dest, handle_count);
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_CLEARHANDLE2, dndb->dest, handle, dndb->replyStatus.array[0]);
	}
	EIGRP_FUNC_LEAVE(EigrpDualClearHandle);
	
	return;
}

static const S8 dualnoentry[]	="DUAL: No entry for destination %s in topology table.";
/************************************************************************************

	Name:	EigrpDualPromote

	Desc:	This function is use to promote the passed in rdb in the dndb's rdb chain.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb
			drdb		- pointer to the given rdb
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualPromote(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb)
{
	EigrpDualRdb_st *first_rdb, **prev;

	EIGRP_FUNC_ENTER(EigrpDualPromote);
	first_rdb = dndb->rdb;

	/* No point in promoting if it is already at the head of the chain. If we take the first clause
	  * in the external check below, we can break the ordering assumptions of the caller because it
	  * will place this entry second in the list rather than first, possibly causing an infinite
	  * loop. */
	if(first_rdb == drdb){
		EIGRP_FUNC_LEAVE(EigrpDualPromote);
		return;
	}

	/* First, fixup previous rdb's next pointer */
	prev = &dndb->rdb;
	while(*prev && *prev != drdb){
		prev = &((*prev) ->next);
	}
	
	if(*prev == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualPromote);
		return;
	}
	*prev = drdb->next;

	/*  Now promote it to the head of the chain.  But if we have multiple equal [best] cost
	  * successors, we need to make sure we don't promote an external path over an internal one.
         * Simply put it after the first [internal] entry. */
	if((dndb->succNum > 1) && (!first_rdb->flagExt)){
		drdb->next		= dndb->rdb->next;
		dndb->rdb->next	= drdb;
	}else{
		drdb->next	= dndb->rdb;
		dndb->rdb	= drdb;
	}
	EIGRP_FUNC_LEAVE(EigrpDualPromote);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualRtupdate

	Desc:	This function updates routing table. Update successors cell in the dndb of the topology  
			table. Possibly promote the newly installed successor in the rdb chain.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the dndb of the topology table
			drdb		- pointer to the given route
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRtupdate(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb)
{
	EigrpDualRdb_st	*first_rdb;
	S32		success, promote;

	EIGRP_FUNC_ENTER(EigrpDualRtupdate);
	/* Don't update routing table if it is a directly connected or a redistributed route 
	  * destination.  It was already done S32 before we got here.  Also, don't let anything
	  * override a connected route.  True we shouldn't have more than one of these in the rdb chain
	  * at once, but at least this keeps us from doing something stupid like sending perpetual
	  * updates fedback by a neighboring router.
	  *
	  * Summary rdbs and redistributed rdbs may coexist in the rdb chain, and could also lead to 
	  * perpetual update feedback. */
	success	= FALSE;
	first_rdb	= dndb->rdb;
	switch(drdb->origin){
		case EIGRP_ORG_CONNECTED:
			dndb->succNum = 1;
			/* Let's avoid the problem of more than one connected path to a destination with 
			  * different metrics causing an update storm because of a changing metric. Keep the
			  * lowest metric at the top of the list.  The same goes for redistrubuted below. */
			if(first_rdb->origin == EIGRP_ORG_CONNECTED){
				/* if(dndb->metric <= drdb->metric)*/
				if(first_rdb->metric <= drdb->metric) {
					success = FALSE;
					break;
				}
			}
			success	= TRUE;
			break;

		case EIGRP_ORG_RSTATIC:
		case EIGRP_ORG_RCONNECT:
			dndb->succNum = 1;
			break;	
		case EIGRP_ORG_REDISTRIBUTED:
			dndb->succNum = 1;
			/* Promote redistributed rdbs, except over connected rdbs */
			if(first_rdb->origin != EIGRP_ORG_CONNECTED){
				/* N.B. See the comment in the connected case above. */
				if(first_rdb->origin == EIGRP_ORG_REDISTRIBUTED){
					if(first_rdb->metric <= drdb->metric){
						success = FALSE;
						break;
					}
				}
				success	= TRUE;
			}else{
				success	= FALSE;
			}
			break;
			
		case EIGRP_ORG_SUMMARY:
		case EIGRP_ORG_EIGRP:
			success = (*ddb->rtUpdate)(ddb, dndb, drdb, &promote);
			/* EigrpIpRtUpdate */
			success = (success && promote);
			break;
	}
#if 0
#if	(EIGRP_ROUTE_REDIS_RULE_TYPE == EIGRP_ROUTE_REDIS_RULE_B)
	success = (*ddb->rtUpdate)(ddb, dndb, drdb, &promote);
	/* EigrpIpRtUpdate */
	success = (success && promote);
#endif
#endif

	/* We need to determine if *_rtupdate() indicated it should be promoted in the rdb chain. */
	if(success){
		EIGRP_TRC(DEBUG_EIGRP_ROUTE, "EIGRP-DUAL: RT installed %s via %s\n",
					(*ddb->printNet)(&dndb->dest),
					(*ddb->printAddress)(&drdb->nextHop));
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RTINSTALL, dndb->dest, dndb->dest, drdb->nextHop);
		EigrpDualPromote(ddb, dndb, drdb);
	}
	EIGRP_FUNC_LEAVE(EigrpDualRtupdate);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualRdbClear

	Desc:	This function silently remove the rdb entry from the topology table. If the DNDB is 
			deleted, the supplied pointer will be nulled out.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the given DNDB
			drdb		- pointer to the rdb which is to be removed
	
	Ret:		
************************************************************************************/

void	EigrpDualRdbClear(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp, EigrpDualRdb_st *drdb)
{
	EigrpDualNdb_st *dndb;

	EIGRP_FUNC_ENTER(EigrpDualRdbClear);
	dndb	= *dndbp;
	EigrpDualZapDrdb(ddb, dndb, drdb, TRUE); 
	if((dndb->rdb == NULL) && (dndb->sndFlag == 0)){
		EigrpDualDndbDelete(ddb, dndbp);
	}
	EIGRP_FUNC_LEAVE(EigrpDualRdbClear);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualSendQuery

	Desc:	This function is used to enqueue query packet for a dndb into the send queue.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the given dndb 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualSendQuery(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp)
{
	EigrpDualNdb_st *dndb;

	EIGRP_FUNC_ENTER(EigrpDualSendQuery);
	dndb	= *dndbp;
	dndb->sndFlag &= ~EIGRP_DUAL_MULTI_MASK;
	dndb->sndFlag |= EIGRP_DUAL_MULTI_QUERY;

	EigrpDualXmitDndb(ddb, dndbp);
	EIGRP_FUNC_LEAVE(EigrpDualSendQuery);

	return;
}

const S8 dual_activestring[]	= "DUAL: Dest %s %sentering active state.";

/************************************************************************************

	Name:	EigrpDualActive

	Desc:	This function try to change a dndb to active state. Create a mini reply-status table 
			for this destination only. Bump the count in the ndb and flag multicast queries to be
			sent.
 
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb
			query_drdb	- pointer to the DRDB for the guy that sent us a query if we're going
						   active upon receipt of a query.
	
	Ret:		TRUE	for we've gone active
			FALSE	for it was suppressed for some reason.
************************************************************************************/

S32	EigrpDualActive(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *query_drdb)
{
	EigrpDualRdb_st	*drdb;
	EigrpIdb_st		*iidb, *split_horizon_iidb;
	EigrpHandle_st	*handle, *dndb_handles;
	S32	peer_count, 	splitHorizon;
	U32	index, reply_status_bytes;

	EIGRP_FUNC_ENTER(EigrpDualActive);
	handle	= &ddb->handle;
	dndb_handles	= &dndb->replyStatus;

	if(!handle->used){
		/* No peers at all */
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_NOTACTIVE, dndb->dest, dndb->dest, zero);
		EIGRP_FUNC_LEAVE(EigrpDualActive);
		return FALSE;
	}

#if	EIGRP_DUAL_SPLIT_HORIZON_QUERIES
	/* If we were about to send an update, AND we were going to temporarily disable split horizon,
	  * then we MUST continue to disable it, even in this case.  Failure to do so would result in 
	  * old stale information in some neighboring topology tables. This is the same case that shows
	  * up in _routeupdate().  See that function for more info. */
	if(dndb->sndFlag & EIGRP_DUAL_MULTI_MASK){
		/* Always suppress split horizon if anything's happening. */

		splitHorizon = FALSE;
	}else{
		splitHorizon = TRUE;
	}
#else					/* EIGRP_DUAL_SPLIT_HORIZON_QUERIES */
	splitHorizon = FALSE;
#endif					/* EIGRP_DUAL_SPLIT_HORIZON_QUERIES */

	drdb = EigrpDualSuccessor(dndb);

	/* Figure out how many peers need to receive this query.  We check the "flagBeingSent" flag in
	  * anticipation of the "flagTellEveryone" flag being set at the end of this routine.  We want
	  * to suppress split horizon in this case. */
	peer_count = handle->used; /* Total peer count */
	split_horizon_iidb = NULL;
	if(drdb && !dndb->flagBeingSent && (splitHorizon == TRUE)){
		iidb = drdb->iidb;
		if(iidb){
			/* Is split horizon enabled on this interface? */
			if(iidb->splitHorizon == TRUE){
				/* Are we processing a query from our successor? */
				if(query_drdb){
					if(query_drdb == drdb){
						split_horizon_iidb = iidb;
					}
				}else{
					/* We aren't processing a query.  Are we in multiple diffusing computations for
					  * this dest and did we receive a query from our successor while active? */
					if(dndb->origin == EIGRP_DUAL_QOMULTI){
						split_horizon_iidb = iidb;
					}
				}
			}
		}
	}

	/* If there's a split horizoned interface, subtract those peers. */
	if(split_horizon_iidb){
		peer_count -= EigrpDualPeerCount(split_horizon_iidb);
	}
	dndb->shQueryIdb = split_horizon_iidb; /* Remember it for later. */

	/* Return false if split horizon nailed this destination 	*/
	if(peer_count == 0){
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_NOTACTIVE, dndb->dest, dndb->dest, one);
		EIGRP_FUNC_LEAVE(EigrpDualActive);
		return FALSE;
	}

	/* Alright, we've got somebody to query.  Create a reply status table. */
	if(EigrpDualDndbActive(dndb)){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualActive);
		return FALSE;
	}

	reply_status_bytes	= EIGRP_HANDLE_MALLOC_SIZE(handle->arraySize);

	if(dndb_handles->array){
		EigrpPortMemFree(dndb_handles->array);
	}
	dndb_handles->array	= EigrpPortMemMalloc(reply_status_bytes);
	if(dndb_handles->array == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualActive);
		return FALSE;
	}
	EigrpUtilMemZero((void *) dndb_handles->array, reply_status_bytes);
	dndb_handles->arraySize	= handle->arraySize;

	/* First set bits for all of the peers. */
	EigrpPortMemCpy((U8 *) dndb_handles->array, (U8 *) handle->array, (U32)reply_status_bytes);
	dndb_handles->used	= handle->used;

	/* Now clear all of the bits for peers on the split horizoned interface,  if there is one. */
	if(split_horizon_iidb){
		handle = &split_horizon_iidb->handle;
		if(handle->arraySize > dndb_handles->arraySize){
			EIGRP_ASSERT(0);
			EigrpPortMemFree(dndb_handles->array);
			EigrpPortMemSet((U8 *)dndb_handles, 0, sizeof(EigrpHandle_st));
			EIGRP_FUNC_LEAVE(EigrpDualActive);
			return FALSE;
		}
		for(index = 0; index < handle->arraySize; index++){
			dndb_handles->array[ index ] &= ~(handle->array[ index ]);
		}
		dndb_handles->used -= handle->used;
	}

	/* Guess we ARE going active after all.  We may have a pending update  that needs to be 
	  * cleared. */
	EigrpDualSendQuery(ddb, &dndb);

	if(dndb == NULL){			/* Can't happen; we have peers */
		EIGRP_ASSERT(0);
		EigrpPortMemFree(dndb_handles->array);
		EigrpPortMemSet((U8 *)dndb_handles, 0, sizeof(EigrpHandle_st));
		EIGRP_FUNC_LEAVE(EigrpDualActive);
		return FALSE;
	}

	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_ACTIVE, dndb->dest, dndb->dest, peer_count);

	/* There is an implicit assumption elsewhere that we only have 1 former feasible successor
	  * while active.  Specifically, in EigrpDualSplitHorizon. This is of course assuming this
	  * successor isn't just now in the process of going away.  The caller EigrpDualPeerDown() has
	  * set this to zero in this case and please don't touch it. */
	if(dndb->succNum != 0){
		dndb->succNum	= 1;
	}

	/* We need to have this here so that when we get ready to send the query and we have changed
	  * the query-origin flag from one of the two local states to multiple successor origin before
	  * the query goes out we loose track of whether we should split horizon. This flag ensures
	  * this does not happen.
	  *
	  * Note: A little history behind the above comment.  At one time, _getnext() used to use the 
	  * query-origin flag to determine when to disable split horizon.  The problem with that is 
	  * explained above. Therefore we needed an additional flag(stack variable splitHorizon) to
	  * force this state regardless of the state changes between now and when we eventually get
	  * around to sending the query. */
	dndb->flagSplitHorizon = (split_horizon_iidb != NULL);
	EIGRP_FUNC_LEAVE(EigrpDualActive);

	return TRUE;
}

/************************************************************************************

	Name:	EigrpDualNdbLookup

	Desc:	This function is used to find the matching dndb for dest in the topology table.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dest		- pointer to the destination
	
	Ret:		pointer to the matching dndb, or NULL if nothing is found
************************************************************************************/

EigrpDualNdb_st *EigrpDualNdbLookup(EigrpDualDdb_st *ddb, EigrpNetEntry_st *dest)
{
	EigrpDualNdb_st	*dndb;
	S32	slot;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualNdbLookup);
	slot		= (*ddb->ndbBucket)(dest);
	dndb	= ddb->topo[ slot ];
	while(dndb){
		if((*ddb->networkMatch)(dest, &dndb->dest)){
			EIGRP_FUNC_LEAVE(EigrpDualNdbLookup);
			return(dndb);
		}
		dndb	= dndb->next;
	}
	EIGRP_FUNC_LEAVE(EigrpDualNdbLookup);
	
	return NULL;
}

/************************************************************************************

	Name:	EigrpDualRdbLookup

	Desc:	This function is used to find the matching rdb in the topology table. 

	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given destination network entry
			iidb		- pointer to the given Eigrp interface
			nexthop	- pointer to the given next hop
	
	Ret:		pointer to the matching rdb or NULL if nothing is found or the dndb is null
************************************************************************************/

EigrpDualRdb_st *EigrpDualRdbLookup(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpIdb_st *iidb, U32 *nexthop)
{
	EigrpDualRdb_st	*drdb;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualRdbLookup);
	if(dndb == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualRdbLookup);
		return NULL;
	}
	drdb = dndb->rdb;
	while(drdb){	
		if((iidb == drdb->iidb) && (*ddb->addressMatch)(nexthop, &drdb->nextHop)){
			EIGRP_FUNC_LEAVE(EigrpDualRdbLookup);
			return(drdb);
		}
		drdb = drdb->next;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRdbLookup);
	
	return NULL;
}

/************************************************************************************

	Name:	EigrpDualSendReply

	Desc:	This function enqueues reply packet for a drdb into the send queue. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the destination network entry
			drdb		- pointer to the given drdb
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualSendReply(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, EigrpDualRdb_st *drdb)
{
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualSendReply);
	if((dndb == NULL) || (drdb == NULL)){
		EIGRP_FUNC_LEAVE(EigrpDualSendReply);
		return;
	}

	drdb->sndFlag |= EIGRP_DUAL_SEND_REPLY;
	dndb->sndFlag |= EIGRP_DUAL_SEND_REPLY; /* Don't let it get freed! */
	EigrpDualXmitDrdb(ddb, drdb);

	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_SENDREPLY, dndb->dest, dndb->dest, drdb->nextHop);
	EIGRP_FUNC_LEAVE(EigrpDualSendReply);

	return;
}

/************************************************************************************

	Name:	EigrpDualSetFD

	Desc:	This function set dndb's FD to metric.
 
 			The metric not ever allowing zero.  This way connected routes pass the feasibility 
 			condition test without a special check.

			This is not the case any longer.  We now make certain that the computed metrics are 
			never less than one.  This seems like a safer bet anyway.
		
	Para: 	dndb	- pointer to the given dndb
			metric	- the dndb's new FD
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualSetFD(EigrpDualNdb_st *dndb, U32 metric)
{
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualSetFD);
	dndb->oldMetric = metric;
	EIGRP_FUNC_LEAVE(EigrpDualSetFD);

	return;
}

/************************************************************************************

	Name:	EigrpDualMaxRD

	Desc:	This function set dndb's RD to a metric of infinity
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualMaxRD(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualMaxRD);
	dndb->metric			= EIGRP_METRIC_INACCESS;
	dndb->vecMetric.delay		= EIGRP_METRIC_INACCESS;

	EigrpDualLog(ddb, EIGRP_DUALEV_SETRD, &dndb->dest, &dndb->dest, &dndb->metric);
	EIGRP_FUNC_LEAVE(EigrpDualMaxRD);

	return;
}

/************************************************************************************

	Name:	EigrpDualSetRD

	Desc:	This function set the reported distance so that we report the correct metric when the 
			next update, query, or reply is sent.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the dndb whose RD will be set
	
	Ret:		the dndb's reported distance
************************************************************************************/

U32	EigrpDualSetRD(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualSetRD);
	dndb->metric	= dndb->rdb->metric;
	dndb->vecMetric	= dndb->rdb->vecMetric;
	EigrpDualLog(ddb, EIGRP_DUALEV_SETRD, &dndb->dest, &dndb->dest, &dndb->metric);

	EIGRP_FUNC_LEAVE(EigrpDualSetRD);
	
	return(dndb->metric);
}

/************************************************************************************

	Name:	EigrpDualCompareDrdb

	Desc:	This function compares incoming drdb.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- pointer to the incoming route
			drdb		- pointer to the current DRDB
	
	Ret:		FALSE 	for the incoming route is identical to the DRDB, 
			TRUE 	for it is not (meaning that we need to take action).

************************************************************************************/

S32	EigrpDualCompareDrdb(EigrpDualDdb_st *ddb, EigrpDualNewRt_st *route, EigrpDualRdb_st *drdb)
{
	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualCompareDrdb);
	if(ddb->compareExt != NULL){
		if(!(*ddb->compareExt)(route->data, drdb->extData)){/*EigrpIpCompareExternal*/
			EIGRP_FUNC_LEAVE(EigrpDualCompareDrdb);
			return TRUE;
		}
	}

	if((drdb->metric == route->metric) &&
			(drdb->vecMetric.delay == route->vecMetric.delay) &&
			(drdb->vecMetric.bandwidth == route->vecMetric.bandwidth) &&
			(drdb->vecMetric.mtu == route->vecMetric.mtu) &&
			(drdb->vecMetric.hopcount == route->vecMetric.hopcount) &&
			(drdb->vecMetric.reliability ==
			route->vecMetric.reliability) &&
			(drdb->vecMetric.load == route->vecMetric.load) &&
			(drdb->flagExt == route->flagExt) &&
			(drdb->opaqueFlag == route->opaqueFlag) &&
			(drdb->origin == route->origin) &&
			(drdb->flagKeep == route->flagKeep) &&
			(drdb->flagIgnore == route->flagIgnore)){
		EIGRP_FUNC_LEAVE(EigrpDualCompareDrdb);
		return FALSE;
	}
	EIGRP_FUNC_LEAVE(EigrpDualCompareDrdb);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpDualUpdateTopo

	Desc:	This function updates the topology table with(possibly) new information.  
	
 			Note: route->data may contain malloc'd data whose pointer needs to be saved in a drdb, 
 			or the data should be freed.
 
 			Returns a pointer to the dndb and drdb, if any.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			createjunk	- it is TRUE if we should create a new DNDB even if the destination is to
						   be ignored or is unreachable.
			update_needed	-  If it is non-NULL, it points to a S32 which we will set to be TRUE 
							    if an update needs to be sent in response to this destination.
			route		- pointer to the route which contain the new information
			new_dndb	- pointer to the created dndb
			new_drdb	- pointer to the created drdb
			created_drdb	- If a DRDB is created for this route, created_drdb will be returned 
						   TRUE. 

	
	Ret:		neighbor's old metric or inaccessible if this is a new entry.  Also, return the 
			old metric(calculated by our point of view) in route->metric.

************************************************************************************/

U32	EigrpDualUpdateTopo(EigrpDualDdb_st *ddb, S32 createjunk,
                                   S32 * update_needed, EigrpDualNewRt_st *route,
                                   EigrpDualNdb_st **new_dndb, EigrpDualRdb_st **new_drdb,
                                   S32 *created_drdb, U32 recvFlag)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	EigrpDualRdb_st	**prev;
	U32	old_metric;
	S32	slot, successors, peerHandle;

	EIGRP_VALID_CHECK(0);

	EIGRP_FUNC_ENTER(EigrpDualUpdateTopo);
#if LOCAL_WIREDNET_SUPRESS		
/***add by cwf 20130109  备用位置 (update不更新本地非用户口ip地址)*****/	
	if(NULL != EigrpIntfFindByDestNet(route->dest.address & route->dest.mask)){
		*update_needed = FALSE;
		EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
		return 0;
	}
#endif

	*new_dndb	= NULL;
	*new_drdb	= NULL;

	if(created_drdb){         
		*created_drdb = FALSE;
	}

	/* This little sanity check should be pulled out once the infosource field is fully 
	  * implemented.  For now they need to be the same. */
	dndb = EigrpDualNdbLookup(ddb, &route->dest);
	
	if(route->origin != EIGRP_ORG_SUMMARY){/* let the no-autosummary work */
		if(dndb && dndb->rdb){
			drdb	= dndb->rdb;
			if(recvFlag == EIGRP_RECV_UPDATE && drdb->origin == EIGRP_ORG_SUMMARY){	/* do not miss the reply */
				if(update_needed){
					*update_needed = FALSE;
				}
				_EIGRP_DEBUG("%s:do not miss the reply \n",__func__);
				EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
				return 0;
			}
		}
	}
	if(dndb == NULL){
		if(!createjunk && (route->flagIgnore ||(route->metric == EIGRP_METRIC_INACCESS))){
			/* Received a useless update - ignore it.  Route may have been administratively
			  * prohibited. */
			EigrpDualLog(ddb, EIGRP_DUALEV_IGNORE, &route->dest, &route->dest, &route->metric);
			EigrpDualFreeData(ddb, &route->data);
			EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
			_EIGRP_DEBUG("%s:EIGRP_METRIC_INACCESS createjunk %d route->flagIgnore %d route->metric %d\n",__func__,createjunk ,route->flagIgnore ,route->metric);
			return EIGRP_METRIC_INACCESS;
		}
		if((ddb->routes > EIGRP_MAX_ROUTE) && update_needed){
			EigrpDualFreeData(ddb, &route->data);
			EIGRP_TRC(DEBUG_EIGRP_OTHER,  "dual can't added route. route count exceed the upper limit %d", 
						EIGRP_MAX_ROUTE);
			_EIGRP_DEBUG("dual can't added route. route count exceed the upper limit %d", 
						EIGRP_MAX_ROUTE);
			EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
			return EIGRP_METRIC_INACCESS;
		}
		dndb = EigrpPortMemMalloc(sizeof(EigrpDualNdb_st));
		if(dndb == NULL){
			EigrpDualFreeData(ddb, &route->data);
			EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
			_EIGRP_DEBUG("%s:EigrpDualFreeData %d", __func__);
			return EIGRP_METRIC_INACCESS;
		}
		ddb->routes++; 
		EigrpUtilMemZero((void *) dndb, sizeof(EigrpDualNdb_st));
		/* Fill in the destination address into the ndb. Set FD and RD to infinity. */
		dndb->dest = route->dest;
		EigrpDualSetFD(dndb, EIGRP_METRIC_INACCESS);
		EigrpDualMaxRD(ddb, dndb);
		dndb->succNum	= 0;
		dndb->origin		= EIGRP_DUAL_QOLOCAL;

		/* This field is used to determine when to send out an update in the event the metric has
		  * not changed, but the {in, ex}ternal disposition has. It is set here(initialization),
		  * and when it is checked(should be in _scanupdate). */
		dndb->flagExt = route->flagExt;

		/* If the client protocol wants to know that we have created a new network, tell it now. */

		/* Setup hash bucket pointing to dndb and link it into the chain. */
		slot = (*ddb->ndbBucket)(&route->dest);
		dndb->next = ddb->topo[ slot ];
		ddb->topo[ slot ] = dndb;
	}else{
		/* Write down what the opaque flags used to be set to before revising topology table. */
		if(ddb->exteriorFlag){
			dndb->opaqueFlagOld = (*ddb->exteriorFlag)(dndb);
		}	
	} /* end of else */

	*new_dndb = dndb;

	drdb = EigrpDualRdbLookup(ddb, dndb, route->idb, &route->nextHop);
	if(drdb == NULL){
		drdb = EigrpPortMemMalloc(sizeof(EigrpDualRdb_st));

		if(drdb == NULL){
			EigrpDualFreeData(ddb, &route->data);
			EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
			return(EIGRP_METRIC_INACCESS);
		}
		EigrpUtilMemZero((void *) drdb, sizeof(EigrpDualRdb_st));
		if(created_drdb){
			*created_drdb = TRUE;
		}
		drdb->thread.drdb	= TRUE;
		drdb->dndb			= dndb;
		drdb->iidb			= route->idb;

		drdb->succMetric	= EIGRP_METRIC_INACCESS;
		drdb->handle			= EIGRP_NO_PEER_HANDLE;

		/* Fill in next hop and infosource. */
		drdb->nextHop	= route->nextHop;
		drdb->infoSrc		= route->infoSrc;

		/* Now link it into the chain and set the old metric correctly. Be careful not to insert
		  * ahead of our current best cost successor(s) if any exists.  Also, if we are active,
		  * we may not be able to depend on the setting of successors.  This being the case, insert
		  * this new information after the first entry if we are active. */
		if(!route->flagExt){	
			prev = &dndb->rdb;
			if(*prev && route->metric >= (*prev)->metric){
				if(!EigrpDualDndbActive(dndb)){
					successors = dndb->succNum;
					while(successors--){
						if(!*prev){
							EIGRP_ASSERT(0);
							break;
						}
						prev = &((*prev) ->next);
					}
				}else{
					if(dndb->rdb != NULL){
						prev = &((*prev) ->next);
					}else if(dndb->succNum > 0){
						EIGRP_ASSERT(0);
					}
				}
			}
			while(*prev){	
				if(route->metric <= (*prev)->metric && !(*prev)->flagExt){	/* put the eigrp route behind the external route */
					break;
				}
				prev	= &((*prev)->next);
			}
			drdb->next = *prev;
			*prev = drdb;
		}else{	/* we make the external route  be the first rdb of dndb*/
			drdb->next	= dndb->rdb;
			dndb->rdb	= drdb;
			drdb->originRouter = route->originRouter;	/*zhenxl_20130116*/
		}
		
	}

	/* Update the peer handle.  It should only be changing if the DRDB handle was previously
	  * EIGRP_NO_PEER_HANDLE (either it's new, or it's an old DRDB that was still active when our
	  * peer went down, and who has now risen from the grave). */
	if(route->idb){
		peerHandle = EigrpFindhandle(ddb, &route->nextHop, EigrpDualIdb(route->idb));
		if(drdb->handle != peerHandle){
			if(drdb->handle != EIGRP_NO_PEER_HANDLE){
				EIGRP_ASSERT(0);
				EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
				return 0;
			}
			drdb->handle = peerHandle;
		}
	}

	*new_drdb = drdb;

	/* A comparison of the pending drdb.  If it is an exact copy of an existing drdb in the 
	  * topology table, don't bother linking it in - This is also fix for CSCdi40200. */
	if(update_needed && *update_needed){
		*update_needed = EigrpDualCompareDrdb(ddb, route, drdb);

		/* We must process update in the following cases here, or dndb may have no successor in 
		  * topology */
		if(dndb && (*update_needed == FALSE) &&
				(dndb->succNum == 0) && (drdb->metric != EIGRP_METRIC_INACCESS)){
			*update_needed = TRUE;
		}
	}

	drdb->flagIgnore 		= route->flagIgnore;
	drdb->metric 			= route->metric;

	old_metric 			= drdb->succMetric;
	drdb->succMetric		= route->succMetric;
	drdb->vecMetric		= route->vecMetric;
	drdb->flagExt			= route->flagExt;
	drdb->opaqueFlag		= route->opaqueFlag;
	drdb->flagKeep		= route->flagKeep;
	drdb->origin			= route->origin;
	drdb->rtType			= route->rtType;

	/* Replace any existing drdb->extData with any new data which has been temporarily hung off the
	  * ddb. */
	EigrpDualFreeData(ddb, &drdb->extData);
	drdb->extData = route->data;
	EIGRP_FUNC_LEAVE(EigrpDualUpdateTopo);
	_EIGRP_DEBUG("%s:return old_metric %d", __func__);
	return old_metric;
}

/************************************************************************************

	Name:	EigrpDualScanUpdate

	Desc:	This function is used to scan the rdb chain and attempt to install into the routing table.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the dndb which contains the rdb chain
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualScanUpdate(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EigrpDualRdb_st	*drdb;
	EigrpDualRdb_st	*next;
	S8	opaque_flags_new, *update_reason;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualScanUpdate);
	/* Initialize successor count, to avoid relying upon protocol dependent update routine to set 
	  * it when no paths are in the routing table.  Also, remember who our old successor was so
	  * before updating the routing table so in case the routing table selects a non feasible 
	  * successor we keep the SDAG intact when going active. */
	drdb	= dndb->rdb;
	dndb->succNum	= 0;
	while(drdb){

		/* Remember next rdb in case we promote this route when we attempt to install it. */
		next	= drdb->next;
		if(!drdb->flagIgnore){
			EigrpDualRtupdate(ddb, dndb, drdb);
		}
		drdb = next;
	}

	if(dndb->succNum == 0){
		/* No successors?  If we have a summary rdb, consider that to be a feasible successor even
		  * though it wasn't accepted by the protocol specific routing table. */
		for(drdb = dndb->rdb; drdb; drdb = drdb->next){
			if((drdb->origin == EIGRP_ORG_SUMMARY) && (drdb->metric != EIGRP_METRIC_INACCESS)){
				dndb->succNum	= 1;
				EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_FPROMOTE, dndb->dest, dndb->dest, drdb->metric);
				EigrpDualPromote(ddb, dndb, drdb);

				break;
			}
		}
	}
	/* See if the PDM has a reason for us to send an update. */
	if(ddb->exteriorCheck != NULL){
		(*ddb->exteriorCheck)(ddb, dndb, &opaque_flags_new);
	}else{
		opaque_flags_new = 0;
	}

	update_reason = NULL;
	/* This is the case where the destination is about to go away and we are going to tell the
	  * world about it. */
	if(dndb->rdb == NULL){
		update_reason = "rt gone";
	}else{
		/* The external field in the ndb is only checked here to determine when to send an update
		  * in the case of the path changing {in, ex}ternal disposition. It is also checked in the 
		  * caller _routeupdate().  It is only set upon initialization(should be in _updatetopology)
		  * and here after the destination is updated. */
		if(dndb->rdb->flagExt != dndb->flagExt){
			dndb->flagExt = dndb->rdb->flagExt;
			if(dndb->flagExt){
				update_reason = "rt now ext";
			}else{
				update_reason = "rt now int";
			}
		}else{
			/*  Check for other reasons why we need to send an update.
			  *  - the metric has changed
			  *  - PDM flag information has changed
			  *  - we used to have a feasible successor but now we don't */
			if((dndb->oldMetric != EIGRP_METRIC_INACCESS) && (dndb->succNum == 0)){
				update_reason = "rt now infea";
			}else if(dndb->rdb->metric != dndb->metric){
				update_reason = "metric chg";
			}else if(dndb->opaqueFlagOld != opaque_flags_new){
				if(opaque_flags_new){
					update_reason	= "rt now ext2";
				}else{
					update_reason	= "rt now int2";
				}
			}
		}
	}
	if(update_reason){
		EigrpDualSendUpdate(ddb, dndb, update_reason);
	}
	EIGRP_FUNC_LEAVE(EigrpDualScanUpdate);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualRouteUpdate

	Desc:	This function does some cleanup and attempt to install destination in routing table.
			This only happens when we've found a feasible successor and remain in passive state. 
			Send an update about this destination if the old best metric is different than the new 
			best metric.
 
 			The DNDB pointer is nulled out if the DNDB is freed.
 
 			This routine needs to be to fixed to cleanup the routing table routes before installing 
 			new routes due to the problem created when the metric increases.  Try to figure out a 
 			way to avoid scanning the rdb's twice, once to delete and once to add. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the destination network entry
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRouteUpdate(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	EigrpDualRdb_st	**prev_drdb;
	EigrpDualNewRt_st	route;
	S32	successors, iidb_count, iidb_ix;
	S32	diddle_split_horizon, flagDel;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualRouteUpdate);
	dndb	= *dndbp;


	
	if(!ddb->scratch){
		EIGRP_FUNC_LEAVE(EigrpDualRouteUpdate);
		return;	    /* We're so hosed it's not funny */
	}

	/*  This function sets the splitHorizon flag in the dndb.  It is  used by _getnext() to 
	  * determine when to ignore split horizon. We must send updates with infinity metric over
	  * split horizoned interfaces when we acquire a new interface.  This function first saves 
	  * interfaces which are currently in use, then checks it against interfaces in use once the
	  * routing table has been updated.  If there are any new interfaces that weren't used before,
	  * split horizon is temporarily disabled. However, if we find ourselves running through this
	  * routine more than once before the update is sent, we could find ourselves in a position of
	  * nullifying this effect. Take care to leave splitHorizon disabled if the update send_flag is
	  * set when we enter this routine.
	  *
	  * Yet another condition for disabling split horizon.  If dndb->succ is non-NULL, we may have
	  * changed interfaces before going active. Now that we are passive that is the only indication
	  * that this has occured and we must disable split horizon without even having to scan all the
	  * interfaces for it.  Set the diddle flag accordingly. */
	if(dndb->succ != NULL){
		diddle_split_horizon	= FALSE;
		dndb->flagSplitHorizon	= FALSE;
		dndb->succ			= NULL;
	}else{
		if((dndb->sndFlag & EIGRP_DUAL_MULTI_MASK) && !dndb->flagSplitHorizon){
			diddle_split_horizon	= FALSE;
		}else{
			diddle_split_horizon	= TRUE;
		}
	}

	drdb			= dndb->rdb;
	prev_drdb	= &dndb->rdb;
	successors	= dndb->succNum;
	iidb_count	= 0;
	while(drdb){
		flagDel = FALSE;

		/* First, squirrel away the known interfaces used to get to successors. */
		if((successors-- > 0)){
			/* Look for an existing match. */
			for(iidb_ix = 0; iidb_ix < iidb_count; iidb_ix++){
				if(ddb->scratch[ iidb_ix ].iidb == drdb->iidb){
					break;
				}
			}

			/* If no match, add the new one. */

			if(iidb_ix == iidb_count){
				ddb->scratch[ iidb_count ].iidb = drdb->iidb;
				ddb->scratch[ iidb_count ].found = FALSE;
				iidb_count++;
			}
		}

		/* See if this route is marked with infinity.  If so, remove it and move on. */
		if(drdb->metric == EIGRP_METRIC_INACCESS){
			/* But only remove the routing table entry if we have something to say to this link
			  * about this destination. */
			if(drdb->sndFlag == 0 && dndb->exterminate == FALSE/*zhenxl_20130117*/){
				/* we have never installed summary route in our route table. */
				if(drdb->origin == EIGRP_ORG_SUMMARY){
					EigrpDualZapDrdb(ddb, dndb, drdb, FALSE);
				}else{
					EigrpDualZapDrdb(ddb, dndb, drdb, TRUE);
				}
				flagDel = TRUE;
			}else if(ddb->rtDelete){
				EigrpDualBuildRoute(ddb, dndb, drdb, &route);
				(*ddb->rtDelete)(ddb, &route);
			}

			/* Only delete entries when the metric gets worse OR the route changes 
			  * internal/external disposition.  This minimizes the number of times routing table
			  * caches get invalidated.  If it is a simple matter of a metric decrease from the
			  * same nexthop then it makes no sense to upset the fast switching path. */
		}else if(((drdb->origin == EIGRP_ORG_EIGRP) ||
													(drdb->origin == EIGRP_ORG_SUMMARY))){
			if(ddb->rtModify){
				EigrpDualBuildRoute(ddb, dndb, drdb, &route);
				(*ddb->rtModify)(ddb, &route);
			}
		}
		if(!flagDel){
			prev_drdb = &drdb->next;
		}
		drdb = *prev_drdb;
	}
	EigrpDualScanUpdate(ddb, dndb);
	if((dndb->rdb == NULL) && (dndb->sndFlag == 0)){
		EigrpDualDndbDelete(ddb, dndbp);
		dndb = *dndbp;			/* May be gone! */
	}else{
		/*
		*If we are forced to go active, then bail.
		*/
		if(dndb->succ != NULL){
			EIGRP_FUNC_LEAVE(EigrpDualRouteUpdate);
			return;
		}

		/* Now, scan the successors to see if we have acquired a new interface that we haven't used
		  * before.  We could find ourselves having not set send_flag in the dndb yet.  This can
		  * happen because there is information _scanupdate() is not privy to.  That is when 
		  * everything remains the same (metric, flags, etc.) except we change which interface we
		  * are using to get to the destination.  Two cases are covered, the first case in the loop
		  * , as stated above, we added a new interface. The second case is when we lose an 
		  * interface.  Losing an interface case we don't have to disable split horizon. Simply
		  * send the update. */
		if(diddle_split_horizon){
			dndb->flagSplitHorizon = TRUE;
		}
		drdb			= dndb->rdb;
		successors	= dndb->succNum;
		while(drdb && (successors-- > 0)){
			for(iidb_ix = 0; iidb_ix < iidb_count; iidb_ix++){
				if(drdb->iidb == ddb->scratch[ iidb_ix ].iidb){
					ddb->scratch[ iidb_ix ].found = TRUE;
					break;
				}
			}

			/* Didn't find this interface.  Disable split horizon and exit the loop. If we haven't 
			  * already set the update flag, do so.  This is the first case described above. */
			if(iidb_ix == iidb_count){
				dndb->flagSplitHorizon = FALSE;
				EigrpDualSendUpdate(ddb, dndb, "new if");
				EIGRP_FUNC_LEAVE(EigrpDualRouteUpdate);
				return;
			}
			drdb = drdb->next;
		}
		/* And this is the second case.  See above. */
		for(iidb_ix = 0; iidb_ix < iidb_count; iidb_ix++){
			if(ddb->scratch[ iidb_ix ].found == FALSE){
				dndb->flagSplitHorizon = FALSE;
				EigrpDualSendUpdate(ddb, dndb, "lost if");
				break;
			}
		} /* end of for */
	}
	EIGRP_FUNC_LEAVE(EigrpDualRouteUpdate);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualSetDistance

	Desc:	This function is used to dndb's  FD and RD. 
 
 			Feasibility condition has been met.  Also, run through the rdb chain zapping entries
 			that are not feasible successors.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb
	
	Ret:		
************************************************************************************/

void	EigrpDualSetDistance(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EigrpDualRdb_st *drdb;
	U32 metric;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualSetDistance);
	if(dndb->succ != NULL){
		/* The routing table selected a successor that does not meet the feasibility
		  * condition.  Simply bail and let the caller do the FC check again.  Of course it will
		  * fail because _rtupdate() has set the old successors to ignore. */ 
		EIGRP_FUNC_LEAVE(EigrpDualSetDistance);
		return;
	}

	drdb = dndb->rdb;
	if((drdb != NULL) && (dndb->succNum > 0)){
		/*
		* Update RD and FD.
		*/
		metric = EigrpDualSetRD(ddb, dndb);
		if(metric < dndb->oldMetric){
			EigrpDualSetFD(dndb, metric);
		}
	}else{
		/* At this point, two possibilities exist.  We have no successors in the routing table or
		  * we are about to delete this route.  Therefore, we probably have something to send.  If
		  * so, set RD to infinity since we have either no neighbors or no successors to the
		  * destination.  Also set FD to infinity as well for dual. */
		EigrpDualMaxRD(ddb, dndb);
	}
	EIGRP_FUNC_LEAVE(EigrpDualSetDistance);

	return;
}

static const S8 dual_found[]	= " found";

/************************************************************************************

	Name:	EigrpDualTestFC

	Desc:	This function checks to see if we have a feasible successor for this destination.
  
 			Either we are about to to active for this dest(depending on the outcome of this 
 			routine) or we are comming out of active state.
			This is the SNC(source node condition) case.
			  1. Find the minimum metric to the destination.
			  2. Find all neighbor metrics that are less than FD.
			  3. Of those neighbors, if our metric through them is equal to the minimum metric
			      found  in 1. above, we have met the feasibility condition.
			This seems a bit restrictive.  Why not simply use step two as the feasibility
			condition? Because you may not select the minimum distance to the destination.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given destination
	
	Ret:		TRUE	for we have a feasible successor for this destination
			FALSE	for it is not 
************************************************************************************/

S32	EigrpDualTestFC(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EigrpDualRdb_st *drdb, *best;
	U32	Dmin;
	S32	fc;

	EIGRP_FUNC_ENTER(EigrpDualTestFC);
	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_SEARCHFS, dndb->dest, dndb->dest, dndb->oldMetric);

	if(dndb->oldMetric == EIGRP_METRIC_INACCESS){
		EIGRP_FUNC_LEAVE(EigrpDualTestFC);
		return TRUE;
	}

	best	= NULL;
	drdb	= dndb->rdb;
	fc	= FALSE;
	Dmin	= EIGRP_METRIC_INACCESS;
	while(drdb != NULL){

		/* Don't consider a feasible successor if the interface to it is down. This check is
		  * probably done in the protocol dependent routing table update routines anyway.  We must
		  * be consistent. */
		if(!drdb->flagIgnore && (drdb->metric <= Dmin)){
			if(drdb->succMetric < dndb->oldMetric){
				fc = TRUE;
				if(drdb->metric < Dmin){
					Dmin = drdb->metric;
					best = drdb;
				}else if(best == NULL){
					best = drdb;	/* make sure it points to something */
				}
			}else{
				if(drdb->metric < Dmin){
					Dmin = drdb->metric;
					fc = FALSE;
				}
			}
		}
		drdb = drdb->next;
	}
	EIGRP_FUNC_LEAVE(EigrpDualTestFC);

	return(fc);
}

/************************************************************************************

	Name:	EigrpDualFCSatisfied

	Desc:	This function is come here when the feasibility condition has been satisfied.
 
 			The DNDB pointer is NULLed out if the DNDB is freed.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the DNDB
			drdb		- pointer to the rdb queried
			rcv_query	- it is TRUE if we're processing a query.
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualFCSatisfied(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp, EigrpDualRdb_st *drdb, S32 rcv_query)
{
	EigrpDualNdb_st	*dndb;

	EIGRP_FUNC_ENTER(EigrpDualFCSatisfied);
	dndb = *dndbp;

	/* Feasibility condition is satisfied. */
	switch(dndb->origin){
		case EIGRP_DUAL_QOCLEAR:
			break;
			
		case EIGRP_DUAL_QOLOCAL:
			if(rcv_query){
				EigrpDualSendReply(ddb, dndb, drdb);
			}
			break;
			
		case EIGRP_DUAL_QOMULTI:
		case EIGRP_DUAL_QOSUCCS:
			EigrpDualSendReply(ddb, dndb, EigrpDualSuccessor(dndb));
			break;
			
		default:
			break;
	}

	/* We are simply updating the routing table with probable new information. We may be able to be
	  * smarter about calling routeupdate. */
	EigrpDualRouteUpdate(ddb, dndbp);
	if(*dndbp){
		EigrpDualSetDistance(ddb, *dndbp);
	}
	EIGRP_FUNC_LEAVE(EigrpDualFCSatisfied);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualFC

	Desc:	This function is used to calculate feasibility condition.

			The DNDB pointer is NULLed out if the DNDB is freed.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the DNDB
			drdb		- pointer to the route queried
			rcv_query	- it is TRUE if we're processing a query.
	
	Ret:		TRUE	for the condition is satisfied.
			FALSe	for it is not

************************************************************************************/

S32	EigrpDualFC(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp, EigrpDualRdb_st *drdb, S32 rcv_query)
{
	S32	fc;
	U32	metric;
	EigrpDualNdb_st	*dndb;

	EIGRP_VALID_CHECK(0);
	
	EIGRP_FUNC_ENTER(EigrpDualFC);
	dndb	= *dndbp;

	fc	= EigrpDualTestFC(ddb, dndb);
	if(fc == FALSE){
		//if(app_debug)printf("EigrpDualFC 1\n");
		/*zhenxl_20130117 On the EIGRP_ROUTE_DOWN event, if we are the originner of the external route,
			we should just distroy the route and advertise it with infinity metric.
			We needn't QUERY it to our neighers.*/
		if(drdb->flagExt && drdb->originRouter == ddb->routerId){
			printf("EigrpDualFC 2\n");
			dndb->exterminate = TRUE;
			EigrpDualSetFD(dndb, EIGRP_METRIC_INACCESS);
			EigrpDualFCSatisfied(ddb, dndbp, drdb, rcv_query);
			EIGRP_FUNC_LEAVE(EigrpDualFC);
			return TRUE;
		}

		/* Try going active. */
		if(!EigrpDualActive(ddb, dndb, (rcv_query ? drdb : NULL))){
			/* Didn't go active.  Accept the best cost successor at this point. */
			/* Bob thinks that maybe we should just call EigrpDualTransition here. */
			EigrpDualSetFD(dndb, EIGRP_METRIC_INACCESS);
			EigrpDualFCSatisfied(ddb, dndbp, drdb, rcv_query);
			EIGRP_FUNC_LEAVE(EigrpDualFC);
			return TRUE;
		}

		/* EIGRP_GET_TIME_STAMP(dndb->activeTime); */
		/* Feasibility condition is not satisfied. */
		if(dndb->succNum == 0){
			EigrpDualMaxRD(ddb, dndb);
		}else{
			/* If FD is set to zero, this is an indication that the caller wanted to force active
			  * state, so here we are. Therefore, we need to set RD to infinity beforecontinuing.
			  * This usually happens in lostroute and when the routing table accepts a non-feasible
			  * successor. */
			if(dndb->oldMetric == 0){
				EigrpDualMaxRD(ddb, dndb);
				metric	= dndb->rdb->metric;
			}else{
				metric	= EigrpDualSetRD(ddb, dndb);
			}

			EigrpDualSetFD(dndb, metric);
		}

		/* If we are processing a query from other than our successor, send a reply. */
		if(drdb && (drdb != EigrpDualSuccessor(dndb)) && rcv_query){
			EigrpDualSendReply(ddb, dndb, drdb);
		}
	}else{
		/*zhenxl_20130118 On EIGRP_ROUTE_MODIF event, route deleting and route adding occur in series.
		We should just send UPDATE.*/
		if(dndb->exterminate == TRUE){
			dndb->exterminate = FALSE;
		}

		EigrpDualFCSatisfied(ddb, dndbp, drdb, rcv_query);
		dndb = *dndbp;			/*It might be gone. */

		/* Check to see if the routing table accepted a successor that does not meet the FC.  If so
		  * , run through the diffusing computation.  Recursion is okay at this point since we are 
		  * returning immediately. */
		if(dndb && (dndb->succ != NULL)){
			EIGRP_FUNC_LEAVE(EigrpDualFC);
			return(EigrpDualFC(ddb, dndbp, drdb, rcv_query));
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualFC);
	
	return(fc);
}

/************************************************************************************

	Name:	EigrpDualTransition

	Desc:	This function try to change from one query state to another state. 
 
 			We have no outstanding replies.  Figure out if we should go passive.  In two cases we 
 			need to run the feasibility condition again to make a determination whether we should 
 			go passive.  If the condition fails, we need to restart the diffusing computation(i.e.,
 			remain active).
 
 			Note:  Be very careful.  drdb could be set to NULL.
 
 			The DNDB may be freed;  if so, the pointer is NULLed.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the DNDB
			drdb		- pointer to the rdb queried
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualTransition(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp, EigrpDualRdb_st *drdb)
{
	EigrpDualNdb_st	*dndb;
	S32	fc;
#if 0
	static U32	transTimes = 0;

	if(!(transTimes++ % 50)){
		printf("EigrpDualTransition: transTimes:%d\n", transTimes);
	}

	if(transTimes > 5000){
		gEigrpInvalid	= TRUE;
	}
#endif
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualTransition);
	dndb	= *dndbp;
	switch(dndb->origin){
		case EIGRP_DUAL_QOCLEAR:
			(void)EigrpDualFC(ddb, dndbp, drdb, FALSE);
			dndb = *dndbp;			/* Maybe freed. */
			if(dndb){
				EigrpDualState(ddb, dndb, EIGRP_DUAL_QOLOCAL);
			}
			break;

		case EIGRP_DUAL_QOLOCAL:

			/* Simply select the minimum metric.  Short circuit the FC test so it passes by setting
			  * old_metric to infinity. */
			EigrpDualSetFD(dndb, EIGRP_METRIC_INACCESS);
			(void)EigrpDualFC(ddb, dndbp, drdb, FALSE);
			dndb = *dndbp;
			break;

		case EIGRP_DUAL_QOMULTI:
			fc	= EigrpDualFC(ddb, dndbp, drdb, FALSE);
			dndb	= *dndbp;
			if(dndb){
				if(fc == FALSE){
					EigrpDualState(ddb, dndb, EIGRP_DUAL_QOSUCCS);
				}else{
					EigrpDualState(ddb, dndb, EIGRP_DUAL_QOLOCAL);
				}
			}
			break;

		case EIGRP_DUAL_QOSUCCS:

			/* Simply select the minimum metric.  Short circuit the FC test so it passes by setting
			  * FD to infinity. */
			EigrpDualSetFD(dndb, EIGRP_METRIC_INACCESS);
			(void)EigrpDualFC(ddb, dndbp, drdb, FALSE);
			dndb	= *dndbp;
			if(dndb){
				EigrpDualState(ddb, dndb, EIGRP_DUAL_QOLOCAL);
			}
			break;
			
		default:
			EIGRP_TRC(DEBUG_EIGRP_OTHER,  "DUAL: origin = %d at %d for %s from %s\n",
								dndb->origin, __LINE__,
								(*ddb->printNet)(&dndb->dest),
								(*ddb->printAddress)(&drdb->nextHop));

			break;
	}
	EIGRP_FUNC_LEAVE(EigrpDualTransition);

	return;
}

/************************************************************************************

	Name:	EigrpDualCleanupReplyStatus

	Desc:	This function checks reply status table to see if it is empty;  if so, free it and 
			possibly go passive.
 
 			The DNDB may be freed;  if so, the pointer is NULLed.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndbp	- doublepointer to the DNDB
			drdb		- pointer to the rdb queried
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualCleanupReplyStatus(EigrpDualDdb_st *ddb, EigrpDualNdb_st **dndbp, EigrpDualRdb_st *drdb)
{
	EigrpDualNdb_st *dndb;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualCleanupReplyStatus);
	dndb	= *dndbp;
	if(!dndb->replyStatus.used){ /* No replies left */
		/* Received last reply.  Possibly send reply to old successor. Go to passive state for this
		  * destination.  Stop the active timer. */
		EIGRP_TIMER_STOP(dndb->activeTime);
		EigrpPortMemFree(dndb->replyStatus.array);
		dndb->replyStatus.array = NULL;
		dndb->replyStatus.arraySize = 0;
		dndb->shQueryIdb = NULL;
		EIGRP_TRC(DEBUG_EIGRP_OTHER,  "DUAL: Freeing reply status table\n");
		EigrpDualLog(ddb, EIGRP_DUALEV_REPLYFREE, &dndb->dest, &dndb->dest, NULL);
		EigrpDualTransition(ddb, dndbp, drdb);
	}
	EIGRP_FUNC_LEAVE(EigrpDualCleanupReplyStatus);

	return;
}

/************************************************************************************

	Name:	EigrpDualValidateNdbRoute

	Desc:	This function updates routing table with information from an dndb, provided we are in
			passive state.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given dndb
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualValidateNdbRoute(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualValidateNdbRoute);
	if(EigrpDualDndbActive(dndb)){
		EIGRP_FUNC_LEAVE(EigrpDualValidateNdbRoute);
		return;
	}
	EigrpDualFC(ddb, &dndb, dndb->rdb, FALSE);
	EIGRP_FUNC_LEAVE(EigrpDualValidateNdbRoute);
	
	return;
}


/************************************************************************************

	Name:	EigrpDualSendUpdate

	Desc:	This function is used flag a DNDB to be sent as an Update.  Sticks the DNDB onto the 
			change queue.

  			NOTE: EigrpDualFinishUpdateSend() "MUST" be called after this routine is called to
  			flush the change queue.  This is done by littering the PDM with calls to that routine
  			after every call into DUAL (sigh).
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given DNDB
			reason	- pointer the string which indicate update reason
	
	Ret:		
************************************************************************************/

void	EigrpDualSendUpdate_int(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, S8 *reason, S8 *file, U32 line)
{
	EIGRP_VALID_CHECK_VOID();
	
	/* We may still have the MULTQUERY bit set in the DNDB even though we're passive, since the 
	  * reply status bits are cleared on the arrival of replies, whereas the MULTQUERY bit is 
	  * cleared when the query is acknowledged(if the ACK is lost, it may take a bit). Since we
	  * can't have both the MULTUPDATE and MULTQUERY bits set, we clear the MULTQUERY bit.
	  *
	  * If the route is active, however, we just bail, since we will be sending an update when the
	  * route goes passive again. */
	EIGRP_FUNC_ENTER(EigrpDualSendUpdate);
	if(dndb->sndFlag & EIGRP_DUAL_MULTI_QUERY){
		if(EigrpDualDndbActive(dndb)){
			EIGRP_FUNC_LEAVE(EigrpDualSendUpdate);
			return;			/* Wait for it to go passive again. */
		}
		dndb->sndFlag &= ~EIGRP_DUAL_MULTI_MASK;
	}
	dndb->sndFlag |= EIGRP_DUAL_MULTI_UPDATE;

	/* Instead of calling EigrpDualXmitDndb here to thread the DNDB, we instead stick the DNDB onto
	  * the change queue;  it will be unqueued shortly when we finish processing the current
	  * packet.  (If we're not processing a packet this will happen right away).  This kludge is to
	  * group all of the updates together in the case where we're going passive and we need to send
	  * a whole buttload of replies and updates(otherwise we'd alternate and it would take a 
	  * zillion packets). */
	if(!dndb->flagOnChgQue){
		*ddb->chgQueTail		= dndb; /* Add to the tail. */
		ddb->chgQueTail		= &dndb->nextChg;
		dndb->nextChg		= NULL;
		dndb->flagOnChgQue	= TRUE;
	}

	EigrpDualLog(ddb, EIGRP_DUALEV_UPDATE_SEND, &dndb->dest, &dndb->dest, &dndb->metric);
	EigrpDualLog(ddb, EIGRP_DUALEV_UPDATE_SEND2, &dndb->dest, reason, &dndb->vecMetric.delay);

	EIGRP_FUNC_LEAVE(EigrpDualSendUpdate);
	return;
}

/************************************************************************************

	Name:	EigrpDualValidateRoute

	Desc:	This function updates routing table with current information in the topology table, if
			it exists and we are in passive state.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dest		- pointer to the destination which contain the current information
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualValidateRoute(EigrpDualDdb_st *ddb, EigrpNetEntry_st *dest)
{
	EigrpDualNdb_st *dndb;

	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualValidateRoute);
	dndb = EigrpDualNdbLookup(ddb, dest);
	if(dndb == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualValidateRoute);
		return;
	}
	EigrpDualValidateNdbRoute(ddb, dndb);	/* May free the DNDB! */
	EIGRP_FUNC_LEAVE(EigrpDualValidateRoute);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualReloadProtoTable

	Desc:	This function is called when  Protocol requests re-populating of its routing table. 
			Walk through topology table and attempt to re-establish routes.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualReloadProtoTable(EigrpDualDdb_st *ddb)
{
	EigrpDualNdb_st *dndb, *next;
	EigrpDualRdb_st *drdb;
	S32 i;
/*	U32	dndbCnt;*/
	
	EIGRP_VALID_CHECK_VOID();
	
	EIGRP_FUNC_ENTER(EigrpDualReloadProtoTable);
	if(ddb == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualReloadProtoTable);
		return;
	}

/*	dndbCnt	= 0;*/
	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = ddb->topo[ i ]; dndb; dndb = next){
/*			if(dndbCnt++ > 50){
				gEigrpInvalid	= TRUE;
			}*/
			next = dndb->next;
			/* Look to see if the first entry is a redistributed route. If we have a redistributed
			  * route at all it *should*be the first and only entry in the rdb chain.  If we do
			  * have one, get rid of it and check the feasibility codition.  It would actually be
			  * nicer if we could delay  going active for the redist'ed routes because in the
			  * normal case we will be getting the information right back.  If that is the case, we
			  * could avoid a diffusing computation. But a delay would delay finding an alternate
			  * path if the dest is truly not comming back. */
			drdb = dndb->rdb;
			if(drdb != NULL && drdb->origin == EIGRP_ORG_REDISTRIBUTED && drdb->flagKeep == FALSE){
				drdb->metric	= EIGRP_METRIC_INACCESS;
				drdb->succMetric	= EIGRP_METRIC_INACCESS;
				drdb->vecMetric.delay	= EIGRP_METRIC_INACCESS;
			}
			EigrpDualValidateNdbRoute(ddb, dndb);	/* May free DNDB */
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualReloadProtoTable);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualRecvUpdate

	Desc:	This function process an update, or a simulation thereof (such as a redistribution event).
			
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- pointer to the updated route or the redistributed route
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRecvUpdate(EigrpDualDdb_st *ddb, EigrpDualNewRt_st *route)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	U32 CD;			/* Neighbor's old metric */
	S32 eigrp_origin, update_needed;
	U32	flag, recvFlag;

	EIGRP_FUNC_ENTER(EigrpDualRecvUpdate);
	update_needed	= TRUE;
	recvFlag	= EIGRP_RECV_UPDATE;
	CD	= EigrpDualUpdateTopo(ddb, FALSE, &update_needed, route, &dndb, &drdb, NULL, recvFlag);
	if(!update_needed){
		_EIGRP_DEBUG("%s:!update_needed\n",__func__);
		EIGRP_FUNC_LEAVE(EigrpDualRecvUpdate);
		return;
	}
	if((dndb == NULL) || (drdb == NULL)){
		EIGRP_FUNC_LEAVE(EigrpDualRecvUpdate);
		return;
	}
	eigrp_origin	= (route->origin == EIGRP_ORG_EIGRP);
	flag	= DEBUG_EIGRP_PACKET_DETAIL_UPDATE;
	flag	|= DEBUG_EIGRP_ROUTE;
	EIGRP_TRC(flag, "DUAL: EigrpDualRecvUpdate(): %s via %s metric %u/%u\n",
				(*ddb->printNet)(&route->dest),
				eigrp_origin ? (*ddb->printAddress)(&route->nextHop) :
				EigrpDualRouteType2string(route->origin), route->metric,
				route->succMetric);

	if(eigrp_origin){
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVUP1, route->dest, route->dest,
							route->nextHop);
	}else{
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVUP3, route->dest, route->dest,
							route->origin);
	}

	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVUP2, route->dest, route->metric,
						route->succMetric);

	if(!EigrpDualDndbActive(dndb)){
		/* Passive state.  Don't run the FC check if all paths to the destination are unreachable.
		  * However, if this newly learned path is not unreachable, then do the FC check anyway. */
		if((dndb->oldMetric != EIGRP_METRIC_INACCESS) ||
				(drdb->succMetric != EIGRP_METRIC_INACCESS)){
				_EIGRP_DEBUG("%s:May free DNDB! \n",__func__);
				
			(void)EigrpDualFC(ddb, &dndb, drdb, FALSE); /* May free DNDB! */
		}else{
			if(drdb->sndFlag == 0){
				EigrpDualRdbClear(ddb, &dndb, drdb);
			}
		}
	}else{
		/* Active state. */
		if((dndb->succNum > 0) && (drdb == EigrpDualSuccessor(dndb))){
			/*  Update came from successor. */
			if(CD < drdb->succMetric){
				/* Metric increase. */
				switch(dndb->origin){
					case EIGRP_DUAL_QOLOCAL:
						_EIGRP_DEBUG("%s:EIGRP_DUAL_QOLOCAL\n",__func__);
						EigrpDualState(ddb, dndb, EIGRP_DUAL_QOCLEAR);
						break;
						
					case EIGRP_DUAL_QOSUCCS:
						_EIGRP_DEBUG("%s:EIGRP_DUAL_QOSUCCS \n",__func__);
						EigrpDualState(ddb, dndb, EIGRP_DUAL_QOMULTI);
						break;
						
					default:
						break;
				}  /* end if switch */
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualRecvUpdate);

	return;
}

/************************************************************************************

	Name:	EigrpDualRecvQuery

	Desc:	This function is used to process a query.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- pointer to the route queried
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRecvQuery(EigrpDualDdb_st *ddb, EigrpDualNewRt_st *route)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	S32 fc;
	U32	recvFlag;

	EIGRP_FUNC_ENTER(EigrpDualRecvQuery);
	recvFlag	= EIGRP_RECV_QUERY;
	(void)EigrpDualUpdateTopo(ddb, TRUE, NULL, route, &dndb, &drdb, NULL, recvFlag);
	if((dndb == NULL) || (drdb == NULL)){
		EIGRP_FUNC_LEAVE(EigrpDualRecvQuery);
		return;
	}
	EIGRP_TRC(DEBUG_EIGRP_ROUTE, "EIGRP-DUAL: EigrpDualRecvQuery():%s via %s metric %u/%u, RD is %u\n",
				(*ddb->printNet)(&route->dest),
				(*ddb->printAddress)(&route->nextHop),
				route->metric, route->succMetric, dndb->metric);
	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVQ1, route->dest, route->dest, route->nextHop);
	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVQ2, route->dest, route->metric, route->succMetric);

	if(!EigrpDualDndbActive(dndb)){
		/* Passive state. */
		if((dndb->oldMetric == EIGRP_METRIC_INACCESS) &&
			(drdb->succMetric == EIGRP_METRIC_INACCESS)){
			EigrpDualSendReply(ddb, dndb, drdb);
		}else{
			fc	= EigrpDualFC(ddb, &dndb, drdb, TRUE); /* May free DNDB! */
			if(dndb){
				if(!fc){
					if(drdb == EigrpDualSuccessor(dndb)){
						EigrpDualState(ddb, dndb, EIGRP_DUAL_QOSUCCS);
					}else{
						EigrpDualState(ddb, dndb, EIGRP_DUAL_QOLOCAL);
					}
				}
			}
		}
	}else{
		/* Active state */
		if((dndb->succNum > 0) && (drdb == EigrpDualSuccessor(dndb))){
			if(dndb->origin!=EIGRP_DUAL_QOSUCCS){
				EigrpDualState(ddb, dndb, EIGRP_DUAL_QOMULTI);
			}
			/*zhenxl_20130121 TODO*/
			if(dndb->flagExt){
				EigrpDualSendReply(ddb, dndb, drdb);
			}
		}else{
			EigrpDualSendReply(ddb, dndb, drdb);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualRecvQuery);

	return;
}

/************************************************************************************

	Name:	EigrpDualRecvReply

	Desc:	This function is used to process a reply.  This could be a big problem if we receive a 
			reply out of the blue when we are not expecting one.  For example, if our successor 
			sends a reply with EIGRP_METRIC_INACCESS, then we will silently delete the entry from
			the topology table without ever testing the FC or sending an update or notifying the 
			routing table. We probably need a check for this.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- pointer to the route replied
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRecvReply(EigrpDualDdb_st *ddb, EigrpDualNewRt_st *route)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	S32	created_drdb;
	U32	recvFlag;

	EIGRP_FUNC_ENTER(EigrpDualRecvReply);
	recvFlag	= EIGRP_RECV_REPLY;
	(void)EigrpDualUpdateTopo(ddb, TRUE, NULL, route, &dndb, &drdb, &created_drdb, recvFlag);
	if((dndb == NULL) || (drdb == NULL)){
		EIGRP_FUNC_LEAVE(EigrpDualRecvReply);
		return;
	}

	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVR1, route->dest, route->dest, route->nextHop);
	EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_RCVR2, route->dest, route->metric, route->succMetric);

	if(dndb->replyStatus.used > 0){
		EigrpDualClearHandle(ddb, dndb, drdb->handle);
		EigrpDualCleanupReplyStatus(ddb, &dndb, drdb); /* May free DRDB */

		/* If the DRDB didn't exist before we got the reply, and the route is still active, and the
		  * metric is infinity, delete the DRDB so we don't suck up hideous amounts of memory
		  * waiting to go passive. */
		if(created_drdb && EigrpDualDndbActive(dndb) &&
				drdb->succMetric == EIGRP_METRIC_INACCESS &&
				!drdb->flagDel){
			EigrpDualZapDrdb(ddb, dndb, drdb, FALSE);
		}
	}else{
		if(drdb->succMetric == EIGRP_METRIC_INACCESS){
			if(drdb->sndFlag == 0){
				EigrpDualRdbClear(ddb, &dndb, drdb);
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualRecvReply);

	return;
}

/************************************************************************************

	Name:	EigrpDualRecvPacket

	Desc:	This function is used to process a received packet.
 
 			It is assumed that all the good stuff has been stuffed into the appropriate DDB
 			variables. This will change when we rip them out of the DDB, of course.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- unused
			route	- pointer to the route queried/updated/replied


			pkt_type	- the type of the received packet
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualRecvPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpDualNewRt_st *route, U32 pkt_type)
{
	EIGRP_FUNC_ENTER(EigrpDualRecvPacket);
	switch(pkt_type){
		case EIGRP_OPC_UPDATE:
			_EIGRP_DEBUG("EigrpDualRecvUpdate\n");
			EigrpDualRecvUpdate(ddb, route);
			break;
			
		case EIGRP_OPC_QUERY:
			_EIGRP_DEBUG("EigrpDualRecvQuery\n");
			EigrpDualRecvQuery(ddb, route);
			break;
			
		case EIGRP_OPC_REPLY:
			_EIGRP_DEBUG("EigrpDualRecvReply\n");
			EigrpDualRecvReply(ddb, route);
			break;
			
		default:
			break;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRecvPacket);

	return;
}

/************************************************************************************

	Name:	EigrpDualPeerDown

	Desc:	This function is called by protocol-specific peerdown routines(toward the end of the 
			spindly thread) when a peer goes away.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- pointer to the peer which is overtime
	
	Ret:		
************************************************************************************/

void	EigrpDualPeerDown(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer)
{
	EigrpDualNdb_st	*dndb, *next_dndb;
	EigrpDualRdb_st	*drdb;
	EigrpIntf_pt		pEigrpIntf;
	EigrpIdb_st		*iidb;
	U32	infosource;
	S32	slot, handle;

	EIGRP_FUNC_ENTER(EigrpDualPeerDown);
	iidb	= peer->iidb;
	pEigrpIntf	= iidb->idb;
	handle		= peer->peerHandle;
	infosource	= peer->source;
	EIGRP_TRC(DEBUG_EIGRP_TASK, "DUAL: linkdown(): start - %s via %s\n", 
				(*ddb->printAddress)(&infosource), pEigrpIntf->name);
	EIGRP_DUAL_LOG_ALL(ddb, EIGRP_DUALEV_PEERDOWN1, infosource, pEigrpIntf);
	EIGRP_TRC(DEBUG_EIGRP_EVENT, "Peer %s going down\n", (*ddb->printAddress)(&peer->source));

	/* Free the peer handle.  This avoids setting reply status bits for us! */
	EigrpFreehandle(ddb, peer);

	/* Now clean out any topology table entries for this peer. */
	for(slot = 0; slot < EIGRP_NET_HASH_LEN; slot++){
		/* Top of this hash line.  Walk the DNDBs on it. */
		for(dndb = ddb->topo[ slot ]; dndb; dndb = next_dndb){
			next_dndb	= dndb->next; /*It may be freed! */
			drdb			= dndb->rdb;
			EIGRP_TRC(DEBUG_EIGRP_EVENT, "DUAL: Destination %s\n",
						(*ddb->printNet)(&dndb->dest));

			/* Walk each DRDB on this DNDB looking for a match. */
			while(drdb &&
					(!(*ddb->addressMatch)(&infosource, &drdb->infoSrc) ||
					(drdb->iidb != iidb))){
				drdb	= drdb->next;
			}

			/* It gets interesting if we matched our dying peer. */
			if(drdb){

				/* Unthread the DRDB if it was being transmitted.  We don't want to free the DRDB 
				  * just yet, as we need it for the FC checks and such that follow. */
				if(drdb->sndFlag){
					EigrpDualXmitUnthread(ddb, &drdb->thread);
					EigrpDualCleanupDrdb(ddb, drdb, FALSE);
				}

				/* Disassociate from the peer. */
				if(drdb->handle != EIGRP_NO_PEER_HANDLE){
					if(drdb->handle != handle){
						EIGRP_ASSERT(0);
						EIGRP_FUNC_LEAVE(EigrpDualPeerDown);
						return;
					}
					drdb->handle = EIGRP_NO_PEER_HANDLE;
				}

				/* Set D_jk^i to infinity. */
				drdb->succMetric = EIGRP_METRIC_INACCESS;

				/* Set D_j^i to infinity. */
				drdb->vecMetric.delay = EIGRP_METRIC_INACCESS;
				drdb->metric = EIGRP_METRIC_INACCESS;
				if(!EigrpDualDndbActive(dndb)){

					/* Passive state. */
					if(dndb->succNum == 1 && drdb == dndb->rdb){
						dndb->succNum = 0;

						/* D_j^i is already set to infinity above. */
						(void)EigrpDualFC(ddb, &dndb, drdb, FALSE); /* May free DNDB! */
					}else{

						/* Wipe out the DRDB and possibly the DNDB if nobody's still trying to send
						  * it anywhere. */
						if(drdb->sndFlag == 0){
							EigrpDualZapDrdb(ddb, dndb, drdb, TRUE);
							if((dndb->rdb == NULL) && (dndb->sndFlag == 0)){
								EigrpDualDndbDelete(ddb, &dndb);
							}
						}
					}
				}else{
					EigrpDualClearHandle(ddb, dndb, handle);
					if((dndb->succNum > 0) && (drdb == EigrpDualSuccessor(dndb))){
						dndb->succNum = 0;

						/* D_j^i is already set to infinity above. */
						EigrpDualState(ddb, dndb, EIGRP_DUAL_QOCLEAR);
					}
					EigrpDualCleanupReplyStatus(ddb, &dndb, drdb);
				}
			}else{			/* No matching RDB found */
				if(EigrpDualDndbActive(dndb)){

					/* Active state.  We didn't find an rdb for this destination. However, we may
					  * still have to clear the bit in the reply-status table.  The handle was
					  * passed in by the caller. */
					EigrpDualClearHandle(ddb, dndb, handle);
					EigrpDualCleanupReplyStatus(ddb, &dndb, NULL);
				}
			}
		}
	}

	/* Get rid of the peer structure. */

	EigrpDestroyPeer(ddb, peer);
	EIGRP_TRC(DEBUG_EIGRP_TASK, "DUAL: linkdown(): finish\n");

	EigrpDualLogAll(ddb, EIGRP_DUALEV_PEERDOWN2, &handle, NULL);
	EigrpDualFinishUpdateSend(ddb);
	EIGRP_FUNC_LEAVE(EigrpDualPeerDown);

	return;
}

/************************************************************************************

	Name:	EigrpDualIfDelete

	Desc:	This function is called when a interface is deleted.
 
 			Adjust for an interface being deleted.  Everything for this interface should be gone
 			except for active successors;  we zap their IIDB pointers, so we don't have any dangles
 			, and whimper if we find anything else.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			iidb		- pointer to the inteface deleted
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualIfDelete(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb)
{
	EigrpIntf_pt	pEigrpIntf;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;
	S32	slot, active;

	EIGRP_FUNC_ENTER(EigrpDualIfDelete);
	if(iidb == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualIfDelete);
		return;
	}
	pEigrpIntf = iidb->idb;
	EIGRP_TRC(DEBUG_EIGRP_EVENT, "DUAL: EigrpDualIfDelete(): %s is being deleted\n",
				pEigrpIntf->name);
	EigrpDualLogAll(ddb, EIGRP_DUALEV_IFDEL1, &pEigrpIntf, NULL);

	/* Walk the topology table. */
	for(slot = 0; slot < EIGRP_NET_HASH_LEN; slot++){
		for(dndb = ddb->topo[ slot ]; dndb; dndb = dndb->next){
			active	= (EigrpDualDndbActive(dndb));

			/* Walk each DRDB on the DNDB. */
			for(drdb = dndb->rdb; drdb; drdb = drdb->next){

				/* Look for a matching IIDB pointer. */
				if(drdb->iidb == iidb){
					/* Got a match.  If this DRDB isn't on an active DNDB, whine. No whining if the
					  * process is dying, however. */
					if(!active && !ddb->flagGoingDown){
						EigrpDualLogAll(ddb, EIGRP_DUALEV_IFDEL3, &dndb->dest, &drdb->nextHop);
					}

					/* we must deassociate the link between drdb and the dying iidb. */
					drdb->iidb	= NULL;

					/* we save intf when iidb is deleted. EigrpIpRtDelete() will use it. */
					drdb->intf	= pEigrpIntf;

				}
			}
		}
	}
	EigrpDualLogAll(ddb, EIGRP_DUALEV_IFDEL2, NULL, NULL);
	EIGRP_TRC(DEBUG_EIGRP_EVENT, "DUAL: EigrpDualIfDelete(): finish\n");
	EIGRP_FUNC_LEAVE(EigrpDualIfDelete);

	return;
}

/************************************************************************************

	Name:	EigrpDualConnRtchange

	Desc:	This function is called when a directly connected route has been changed. Yet another entry 
			point into dual.  This is used to revise connected metric routes in the topology table.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- the directly connected route which has been changed
			event	- the route changing event, adding a route or deleting a route
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualConnRtchange(EigrpDualDdb_st *ddb, EigrpDualNewRt_st *route, U32 event)
{
	EigrpIntf_pt	pEigrpIntf;

	EIGRP_FUNC_ENTER(EigrpDualConnRtchange);
	pEigrpIntf	= EigrpDualIdb(route->idb);
	if(!pEigrpIntf){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpDualConnRtchange);
		return;
	}

	route->flagExt	= FALSE;
	route->origin		= EIGRP_ORG_CONNECTED;
	route->data		= NULL;
	route->flagIgnore	= FALSE;
	if(event == EIGRP_DUAL_RTDOWN){
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_CONNDOWN, route->dest, route->dest, pEigrpIntf);
	}else{
		route->succMetric	= 0;
		EIGRP_DUAL_LOG(ddb, EIGRP_DUALEV_CONNCHANGE, route->dest, route->dest, pEigrpIntf);
	}
	EIGRP_TRC(DEBUG_EIGRP_ROUTE, "DUAL: EigrpDualConnRtchange(): %s via %s\n",
				(*ddb->printNet)(&route->dest), pEigrpIntf->name);
	EigrpDualRecvUpdate(ddb, route);
	EIGRP_FUNC_LEAVE(EigrpDualConnRtchange);
		
	return;
}

/************************************************************************************

	Name:	EigrpDualRtchange

	Desc:	This function process a routing table change.  This happens when one of the following
 			occurs.
 			1) A route comes up or goes down from another process we are redistributing.
			2) The metric changes for a route we are redistributing from another process and it has 
				a comparable metric(equal administrative  distance in IP speak)
			Note: Do not accept a redistributed route that has a computed metric of zero.  This 
				  occurs when a static route points to Null 0. The workaround for this is to set a 
				  default-metric.  This is a protocol dependent observation.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			route	- pointer to the route which has been changed
			event	- the route changing event, adding a route or deleting a route
	
	Ret:		
************************************************************************************/

void	EigrpDualRtchange(EigrpDualDdb_st *ddb, EigrpDualNewRt_st *route, U32 event)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;

	EIGRP_FUNC_ENTER(EigrpDualRtchange);
	dndb = EigrpDualNdbLookup(ddb, &route->dest);
	_EIGRP_DEBUG(" %s :\n",__func__);
	switch(event){
		case EIGRP_DUAL_RTUP:
		case EIGRP_DUAL_RTCHANGE:

			/* If they are trying to change/add an infinity metric, they are not doing the correct
			  * actions. Add robustness and ignore this route. */
			if(route->metric == EIGRP_METRIC_INACCESS){
				EigrpDualFreeData(ddb, &route->data);
				EIGRP_FUNC_LEAVE(EigrpDualRtchange);
				return;
			}
			route->succMetric = 0;
			break;
			
		case EIGRP_DUAL_RTDOWN:
			if(dndb == NULL){
				EigrpDualFreeData(ddb, &route->data);
				EIGRP_FUNC_LEAVE(EigrpDualRtchange);
				return;
			}
			drdb = EigrpDualRdbLookup(ddb, dndb, route->idb, &route->nextHop);
			if(drdb == NULL){
				EigrpDualFreeData(ddb, &route->data);
				EIGRP_FUNC_LEAVE(EigrpDualRtchange);
				return;
			}

			/*  These three assignments were probably already done by the caller, but just in
			  * case... */
			route->succMetric			= EIGRP_METRIC_INACCESS;
			route->metric			= EIGRP_METRIC_INACCESS;
			route->vecMetric.delay	= EIGRP_METRIC_INACCESS;
			break;

		default:
			break;
	}
	
	EigrpDualLog(ddb, EIGRP_DUALEV_REDIST1, &route->dest, &route->dest, &route->origin);
	EigrpDualLog(ddb, EIGRP_DUALEV_REDIST2, &route->dest, &event, NULL);
	EIGRP_TRC(DEBUG_EIGRP_ROUTE, "DUAL: EigrpDualRtchange(): %s via %s metric %u/%u\n",
				(*ddb->printNet)(&route->dest),
				route->intf? (S8 *)route->intf->name : (*ddb->printAddress)(&route->nextHop),
				route->metric,
				route->succMetric);
	EigrpDualRecvUpdate(ddb, route);
	EIGRP_FUNC_LEAVE(EigrpDualRtchange);

	return;
}

/************************************************************************************

	Name:	EigrpDualLostRoute

	Desc:	This function is called when a route derived from this process gets overwritten by 
 			another process.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dest		- pointer to the destination address of the route which has been
					   overwritten
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualLostRoute(EigrpDualDdb_st *ddb, EigrpNetEntry_st *dest)
{
	EigrpDualNdb_st *dndb;
	U32 zero = 0;
	U32 one = 1;

	EIGRP_FUNC_ENTER(EigrpDualLostRoute);
	dndb = EigrpDualNdbLookup(ddb, dest);
	if(dndb != NULL){

		/* Let's put a little sanity check in here.  If we already have a non eigrp derived 
		  * destination, then don't do anything. That just means we were needlessly called.  Not to
		  * mention, it can break the FC check if called in the wrong order. Also, please, please,
		  * do not send an update or futz with metrics if we are active for this destination. */
		if((EigrpDualDndbActive(dndb)) ||
				(dndb->rdb == NULL) ||
				(dndb->rdb->origin != EIGRP_ORG_EIGRP)){
			EigrpDualLog(ddb, EIGRP_DUALEV_LOST, dest, dest, &zero);
			EIGRP_FUNC_LEAVE(EigrpDualLostRoute);
			return;
		}

		/* Force active state.  It would be nice if we could know which successors were tossed out
		  * by the routing table so that we could ignore them and check the feasibility condition. 
		  * But unfortunately, we don't.  Therefore we have to force active state. */
		EigrpDualLog(ddb, EIGRP_DUALEV_LOST, dest, dest, &one);
		EigrpDualSetFD(dndb, 0);
		(void)EigrpDualFC(ddb, &dndb, dndb->rdb, FALSE); /* May free DNDB! */
	}
	EIGRP_FUNC_LEAVE(EigrpDualLostRoute);

	return;
}

/************************************************************************************

	Name:	EigrpDualTableWalk

	Desc:	This function traverse topology table for caller. Return next dndb in table.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the current dndb
			index	- pointer to index of the next dndb in the hash table
	
	Ret:		NONE
************************************************************************************/

EigrpDualNdb_st *EigrpDualTableWalk(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb, S32 *index)
{
	EigrpDualNdb_st *dndb2;
	S32 i;

	EIGRP_FUNC_ENTER(EigrpDualTableWalk);
	/* If still on linked list, return next pointer. */
	if(dndb && dndb->next){
		EIGRP_FUNC_LEAVE(EigrpDualTableWalk);
		return(dndb->next);
	}

	/* Start down hash table. */
	i = (dndb) ? *index + 1 : *index;

	for( ; i < EIGRP_NET_HASH_LEN; i++){
		/*	process_may_suspend(); */
		for(dndb2 = ddb->topo[ i ]; dndb2; dndb2 = dndb2->next){
			/* Found a non-NULL entry, update index and return pointer. */
			*index = i;
			EIGRP_FUNC_LEAVE(EigrpDualTableWalk);
			return(dndb2);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualTableWalk);
	
	return NULL;
}

/************************************************************************************

	Name:	EigrpDualCleanUp

	Desc:	This function clear out topology table completely.  This process is going away.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualCleanUp(EigrpDualDdb_st *ddb)
{
	EigrpDualNdb_st		*dndb;
	EigrpDualRdb_st		*drdb;
	EigrpXmitThread_st	*entry;
	EigrpXmitAnchor_st 	*anchor;
	S32		index;

	EIGRP_FUNC_ENTER(EigrpDualCleanUp);
	for(index = 0; index < EIGRP_NET_HASH_LEN; index++){
		/* process_may_suspend(); */
		while((dndb = ddb->topo[ index ]) != NULL){
			/* First, scan rdb chain clearing it out and any external data structures. */
			while((drdb = dndb->rdb) != NULL){
				EigrpDualZapDrdb(ddb, dndb, drdb, TRUE);
			}

			/* Clear out the reply-status table if it exists, wipe out the ndb, and move on. */
			if(EigrpDualDndbActive(dndb)){
				if(dndb->replyStatus.array){
					EigrpPortMemFree(dndb->replyStatus.array);
				}
				dndb->replyStatus.array		= NULL;
				dndb->replyStatus.used		= 0;
				dndb->replyStatus.arraySize	= 0;
			}
			EigrpDualDndbDelete(ddb, &dndb);
		}
	}

	/* we must scan the transmit DNDB threads to release remain dummy dndbs */
	if(ddb->threadHead){
		entry = ddb->threadHead;
		ddb->threadHead = entry->next;
		anchor = entry->anchor;
		if(anchor){
			entry->anchor = NULL;
			anchor->thread = NULL;
			EigrpUtilChunkFree(gpEigrp->anchorChunks, anchor);
			anchor = NULL;
		}
		if(entry->dummy){
			EigrpDualXmitUnthreadUnanchored(ddb, entry);
			EigrpUtilChunkFree(gpEigrp->dummyChunks, entry);
		}
	}

	if(ddb->events){
		/* Don't use log function, ddb->events is not needed. */ 
		ddb->events = (EigrpDualEvent_st*) 0;
	}
	if(ddb->siaEvent){
		EigrpPortMemFree(ddb->siaEvent);
		ddb->siaEvent = (EigrpDualEvent_st*) 0;
	}
	EIGRP_FUNC_LEAVE(EigrpDualCleanUp);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualSiaCopyLog

	Desc:	This function copy the event log in an SIA situation.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualSiaCopyLog(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpDualSiaCopyLog);
	/* If we found any stuck peers, copy the log, but only once. */
	if(!ddb->siaEvent){
		/* Kill all peers to trigger a pseudo-SIA on our neighbors if asked. */
		if(ddb->flagKillAll){
			EigrpTakeAllNbrsDown(ddb, "all neighbors killed");
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualSiaCopyLog);

	return;
}

/************************************************************************************

	Name:	EigrpDualUnstickDndb

	Desc:	This function clears a DNDB that's stuck in active.  
 
			The DNDB may be freed, along with other DNDBs in the topology table. Peers may be
			freed. Basically, we will leave things a mess.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dndb	- pointer to the given DNDB
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualUnstickDndb(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb)
{
	EigrpDualRdb_st	*drdb, *next_drdb;
	EigrpDualPeer_st	*peer, *next;
	U32	stuck_time, handle_count;

	EIGRP_FUNC_ENTER(EigrpDualUnstickDndb);
	/* we're stuck.  Duly note it. */
	handle_count	= dndb->replyStatus.used;
	stuck_time	= EIGRP_ELAPSED_TIME(dndb->activeTime);

	EIGRP_DUAL_LOG_ALL(ddb, EIGRP_DUALEV_SIA, dndb->dest, stuck_time);
	EIGRP_DUAL_LOG_ALL(ddb, EIGRP_DUALEV_SIA4, handle_count,
	dndb->replyStatus.array[0]);

	/* Walk all of the DRDBs.  If we find one associated with an active peer, knock it off of the
	  * list.  If we find a DRDB with no peer associated with it, whimper now but clean it up
	  * later.  Be careful in case the DNDB is freed or goes passive in the process. */
	for(drdb = dndb->rdb;
			(drdb && !dndb->flagDel && EigrpDualDndbActive(dndb));
			drdb = next_drdb){
		next_drdb = drdb->next; /* It may go away! */
		if(drdb->handle == EIGRP_NO_PEER_HANDLE){
			continue;
		}

		if(!EigrpTestHandle(ddb, &dndb->replyStatus, drdb->handle)){
			continue;
		}
		peer = EigrpHandleToPeer(ddb, drdb->handle);
		if(peer){
			EigrpDualLogAll(ddb, EIGRP_DUALEV_NORESP, &drdb->nextHop, &drdb->origin);
			EigrpClearHandle(ddb, &dndb->replyStatus, drdb->handle);
			EigrpTakePeerDown(ddb, peer, TRUE, "stuck in active");
		}else{	/* No peer! */
			EigrpDualLogAll(ddb, EIGRP_DUALEV_SIA5, &drdb->nextHop, &drdb->origin);
		}
	}

	/* Now walk all of the peers to see if any of them has a bit set. These would correspond to
	  * active peers with no DRDBs(shouldn't happen--but then none of this should.)  Note that the
	  * DNDB may have been deleted above, or may be deleted in the process, or may have gone
	  * passive in either case. */
	peer = ddb->peerLst;
	while(peer && !dndb->flagDel && EigrpDualDndbActive(dndb)){
		next = peer->nextPeer;
		if(EigrpTestHandle(ddb, &dndb->replyStatus, peer->peerHandle)){
			EIGRP_DUAL_LOG_ALL(ddb, EIGRP_DUALEV_SIA3, peer->source, peer->iidb->idb);
			EigrpClearHandle(ddb, &dndb->replyStatus, peer->peerHandle);
			EigrpTakePeerDown(ddb, peer, TRUE, "stuck in active");
		}
		peer = next;
	}

	/* If we have any bits left at this point, whimper one more time. This means that the peer 
	  * corresponding to the SIA bit has mysteriously disappeared.  If the DNDB has been deleted
	  * above, skip it. */
	if(!dndb->flagDel && EigrpDualDndbActive(dndb)){
		if(dndb->replyStatus.used){
			EIGRP_ASSERT(0);
			EigrpDualLogAll(ddb, 
							EIGRP_DUALEV_SIA2,
							&dndb->replyStatus.used,
							dndb->replyStatus.array);
			dndb->replyStatus.used = 0;
			EigrpDualCleanupReplyStatus(ddb, &dndb, NULL);
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualUnstickDndb);

	return;
}

/************************************************************************************

	Name:	EigrpDualScanActive

	Desc:	This function scans the topology table looking for destinations that are stuck in 
			active. It does this by looking for any active state routes with timestamps older than
			allowed. If one is found, then it will log an error and clean up the mess.
		
	Para: 	ddb		- pointer to the dual descriptor block 
		
	Ret:		NONE
************************************************************************************/

void	EigrpDualScanActive(EigrpDualDdb_st *ddb, S32 *finished)
{
	EigrpDualNdb_st	*dndb, *next_dndb;
	S32	index, found_sia, restart_hash_line;
	
	EIGRP_FUNC_ENTER(EigrpDualScanActive);
	if(ddb->activeTime == 0){
		EIGRP_FUNC_LEAVE(EigrpDualScanActive);
		return;
	}

	found_sia = FALSE;
	*finished	= TRUE;		/* tigerwh 120608  */
	for(index = 0; index < EIGRP_NET_HASH_LEN; index++){
		/*	process_may_suspend(); */

		/* If we hit a DDB that is stuck in active, we have to restart from the beginning of the
		  * hash line.  This is because when we clear the neighbor, it may make other DNDBs in the 
		  * hash line disappear as well(if there were multiple DNDBs stuck). We may process some
		  * DNDBs more than once, but that's SUCCESS (if a little wasteful).
		  *
		  * We finess this by using a do/while loop that brings us back the beginning of the hash 
		  * line if we find an active entry. */
		do{				/* Restart loop */
			restart_hash_line = FALSE;
			dndb = ddb->topo[ index ];
			while(dndb != NULL && !restart_hash_line){
				next_dndb = dndb->next;
				if(EigrpDualDndbActive(dndb)){
					/* DNDB is active.  Has it been active for too long? */
					if(EIGRP_TIMER_RUNNING(dndb->activeTime) &&
							EIGRP_CLOCK_OUTSIDE_INTERVAL(dndb->activeTime,
							(ddb->activeTime / EIGRP_MSEC_PER_SEC))){
						EigrpDualUnstickDndb(ddb, dndb);
						found_sia = TRUE;
						restart_hash_line = TRUE;

						/* tigerwh 120608  begin */
						*finished	= FALSE;	
						break;		
						/* tigerwh 120608  end */
					}
				}
				dndb = next_dndb;
			}
			/* tigerwh 120608  begin */
			if(*finished == FALSE){
				break;
			}
			/* tigerwh 120608  end */
		}while(restart_hash_line);

		/* tigerwh 120608  begin */
		if(*finished == FALSE){
			break;
		}
		/* tigerwh 120608  end */
	}
	EigrpDualFinishUpdateSend(ddb);

	/* If we found any stuck peers, copy the log, but only once. */
	if(found_sia){
		EigrpDualSiaCopyLog(ddb);
	}
	EIGRP_FUNC_LEAVE(EigrpDualScanActive);

	return;
}

/************************************************************************************

	Name:	EigrpDualRouteLookup

	Desc:	This function return the drdb for the feasible successor which matches those parameters
			currently set in the ddb.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			dest		- pointer to the destination address(one of the parameters) of the feasible
					   successor
			iidb		- pointer to the EIGRP interface(one of the parameters) of the feasible
					   successor
			nexthop	- pointer to the next hop address(one of the parameters) of the feasible
					   successor
	
	Ret:		the drdb for the feasible successor
************************************************************************************/

EigrpDualRdb_st *EigrpDualRouteLookup(EigrpDualDdb_st *ddb, EigrpNetEntry_st*dest, EigrpIdb_st *iidb, U32 *nexthop)
{
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;

	EIGRP_FUNC_ENTER(EigrpDualRouteLookup);
	dndb = EigrpDualNdbLookup(ddb, dest);
	if(dndb == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualRouteLookup);
		return NULL;
	}
	drdb = EigrpDualRdbLookup(ddb, dndb, iidb, nexthop);
	if(drdb == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualRouteLookup);
		return NULL;
	}

	/* Avoid two routes have same metric. */
	if(drdb->succMetric > dndb->oldMetric){
		EIGRP_FUNC_LEAVE(EigrpDualRouteLookup);
		return NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRouteLookup);
	
	return(drdb);
}


const U32 dual_event_length[ EIGRP_DUAL_ETYPE_COUNT ]	=
{
	0,						/* EIGRP_DUAL_NONE */
	sizeof(U32),				/* EIGRP_DUAL_NUM */
	sizeof(U32),				/* EIGRP_DUAL_HEX_NUM */
	sizeof(EigrpNetEntry_st),	/* EIGRP_DUAL_NET */
	sizeof(U32),				/* EIGRP_DUAL_ADDR */
	EIGRP_DUAL_EVENTS_BUF_SIZE / 2,	/* EIGRP_DUAL_STRING */
	sizeof(void *),				/* EIGRP_DUAL_SWIF */
	sizeof(S32),				/* DUAL_BOOLEAN */
	sizeof(U32),				/* EIGRP_DUAL_RTORIGIN */
	sizeof(U32),				/* EIGRP_DUAL_QUERY_ORIGIN */
	sizeof(U32),				/* EIGRP_DUAL_RTEVENT */
	sizeof(U32)				/* EIGRP_DUAL_PKT_TYPE */
};

const EigrpDualShowEventDecode_st dual_event_table[ EIGRP_DUALEV_COUNT ]	=
{
	{ "Not used", EIGRP_DUAL_NONE, EIGRP_DUAL_NONE },
	{ "Generic", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },					/* EIGRP_DUALEV_GENERIC */
	{ "Update sent, RD", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_UPDATE_SEND */
	{ "Update reason, delay", EIGRP_DUAL_STRING, EIGRP_DUAL_NUM },		/* EIGRP_DUALEV_UPDATE_SEND2 */
	{ "State change", EIGRP_DUAL_QUERY_ORIGIN, EIGRP_DUAL_QUERY_ORIGIN },	/* EIGRP_DUALEV_STATE */
	{ "Clr handle dest/cnt", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_CLEARHANDLE1 */
	{ "Clr handle num/bits", EIGRP_DUAL_NUM, EIGRP_DUAL_HEX_NUM },		/* EIGRP_DUALEV_CLEARHANDLE2 */
	{ "Free reply status", EIGRP_DUAL_NET, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_REPLYFREE */
	{ "Send reply", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },						/* EIGRP_DUALEV_SENDREPLY */
	{ "Forced promotion", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_FPROMOTE */
	{ "Route install", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },					/* EIGRP_DUALEV_RTINSTALL */
	{ "Route delete", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },					/* EIGRP_DUALEV_RTDELETE */
	{ "NDB delete", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },					/* EIGRP_DUALEV_NDBDELETE */
	{ "RDB delete", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },						/* EIGRP_DUALEV_RDBDELETE */
	{ "Active net/peers", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_ACTIVE */
	{ "Not active net/1=SH", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_NOTACTIVE */
	{ "Set reply-status", EIGRP_DUAL_NUM, EIGRP_DUAL_HEX_NUM },		/* EIGRP_DUALEV_SETREPLY */
	{ "Split-horizon set", EIGRP_DUAL_BOOL, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_SPLIT */
	{ "Find FS", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },						/* EIGRP_DUALEV_SEARCHFS */
	{ "FC sat nh/ndbmet", EIGRP_DUAL_ADDR, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_FCSAT1 */
	{ "FC sat rdbmet/succmet", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },		/* EIGRP_DUALEV_FCSAT2 */
	{ "FC not sat Dmin/met", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },		/* EIGRP_DUALEV_FCNOT */
	{ "Rcv update dest/nh", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_RCVUP1 */
	{ "Rcv update met/succmet", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },	/* EIGRP_DUALEV_RCVUP2 */
	{ "Rcv update dest/orig", EIGRP_DUAL_NET, EIGRP_DUAL_RTORIGIN },			/* EIGRP_DUALEV_RCVUP3 */
	{ "Rcv query dest/nh", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_RCVQ1 */
	{ "Rcv query met/succ met", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },	/* EIGRP_DUALEV_RCVQ2 */
	{ "Rcv reply dest/nh", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_RCVR1 */
	{ "Rcv reply met/succ met", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },		/* EIGRP_DUALEV_RCVR2 */
	{ "Peer down", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },					/* EIGRP_DUALEV_PEERDOWN1 */
	{ "Peer down end, handle", EIGRP_DUAL_NUM, EIGRP_DUAL_NONE },		/* EIGRP_DUALEV_PEERDOWN2 */
	{ "Peer up", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },						/* EIGRP_DUALEV_PEERUP */
	{ "i/f delete", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },						/* EIGRP_DUALEV_IFDEL1 */
	{ "i/f delete finish", EIGRP_DUAL_NONE, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_IFDEL2 */
	{ "i/f del lurker", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },					/* EIGRP_DUALEV_IFDEL3 */
	{ "Lost route 1=forceactv", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_LOST */
	{ "Conn rt down", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },					/* EIGRP_DUALEV_CONNDOWN */
	{ "Conn rt change", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },					/* EIGRP_DUALEV_CONNCHANGE */
	{ "Redist rt change", EIGRP_DUAL_NET, EIGRP_DUAL_RTORIGIN },				/* EIGRP_DUALEV_REDIST1 */
	{ "Redist rt event", EIGRP_DUAL_RTEVENT, EIGRP_DUAL_NONE },				/* EIGRP_DUALEV_REDIST2 */
	{ "SIA dest/time", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },					/* EIGRP_DUALEV_SIA */
	{ "SIA lost handles", EIGRP_DUAL_NUM, EIGRP_DUAL_HEX_NUM },		/* EIGRP_DUALEV_SIA2 */
	{ "SIA peer with no DRDB", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },			/* EIGRP_DUALEV_SIA3 */
	{ "SIA count, bits", EIGRP_DUAL_NUM, EIGRP_DUAL_HEX_NUM },			/* EIGRP_DUALEV_SIA4 */
	{ "SIA DRDB with no peer", EIGRP_DUAL_ADDR, EIGRP_DUAL_RTORIGIN },		/* EIGRP_DUALEV_SIA5 */
	{ "SIA peer with DRDB", EIGRP_DUAL_ADDR, EIGRP_DUAL_RTORIGIN },		/* EIGRP_DUALEV_NORESP */
	{ "Intf mcast ready", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },				/* EIGRP_DUALEV_MCASTRDY */
	{ "Peer stuck in INIT", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_INITSTUCK */
	{ "Peer should be in INIT", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },			/* EIGRP_DUALEV_NOINIT */
	{ "Hold time expired", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_HOLDTIMEEXP */
	{ "ACK suppressed", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_ACKSUPPR */
	{ "Unicast sent", EIGRP_DUAL_SWIF, EIGRP_DUAL_ADDR },					/* EIGRP_DUALEV_UCASTSENT */
	{ "Multicast sent", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_MCASTSENT */
	{ "Serno range", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_PKTSENT2 */
	{ "Suppressed", EIGRP_DUAL_PKT_TYPE, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_PKTSUPPR */
	{ "Not sent", EIGRP_DUAL_PKT_TYPE, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_PKTNOSEND */
	{ "Packet received", EIGRP_DUAL_SWIF, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_PKTRCV */
	{ "Opcode/Flags", EIGRP_DUAL_PKT_TYPE, EIGRP_DUAL_HEX_NUM },			/* EIGRP_DUALEV_PKTRCV2 */
	{ "Seq/AckSeq", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_PKTRCV3 */
	{ "Bad sequence", EIGRP_DUAL_ADDR, EIGRP_DUAL_PKT_TYPE },				/* EIGRP_DUALEV_BADSEQ */
	{ "Reply ACK", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },						/* EIGRP_DUALEV_REPLYACK */
	{ "Reply packetized", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_REPLYPACK */
	{ "Reply enqueued", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_REPLYENQ */
	{ "Reply transmitted", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_REPLYXMIT */
	{ "Reply delay/poison", EIGRP_DUAL_NUM, EIGRP_DUAL_BOOL },			/* EIGRP_DUALEV_REPLYXMIT2 */
	{ "Update ACK", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },						/* EIGRP_DUALEV_UPDATEACK */
	{ "Update packetized", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_UPDATEPACK */
	{ "Update enqueued", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },				/* EIGRP_DUALEV_UPDATEENQ */
	{ "Update transmitted", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_UPDATEXMIT */
	{ "Update delay/poison", EIGRP_DUAL_NUM, EIGRP_DUAL_BOOL },			/* EIGRP_DUALEV_UPDATEXMIT2 */
	{ "Query ACK", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },						/* EIGRP_DUALEV_QUERYACK */
	{ "Query packetized", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_QUERYPACK */
	{ "Query enqueued", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },				/* EIGRP_DUALEV_QUERYENQ */
	{ "Query transmitted", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_QUERYXMIT */
	{ "Query delay/poison", EIGRP_DUAL_NUM, EIGRP_DUAL_BOOL },			/* EIGRP_DUALEV_QUERYXMIT2 */
	{ "Startup ACK", EIGRP_DUAL_ADDR, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_STARTUPACK */
	{ "Startup packetized", EIGRP_DUAL_ADDR, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_STARTUPPACK */
	{ "Startup enqueued", EIGRP_DUAL_ADDR, EIGRP_DUAL_SWIF },				/* EIGRP_DUALEV_STARTUPENQ */
	{ "serno/seq #", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_STARTUPENQ2 */
	{ "Startup transmitted", EIGRP_DUAL_NET, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_STARTUPXMIT */
	{ "Startup delay/poison", EIGRP_DUAL_NUM, EIGRP_DUAL_BOOL },			/* EIGRP_DUALEV_STARTUPXMIT2 */
	{ "Last peer down", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_LASTPEER */
	{ "First peer up", EIGRP_DUAL_SWIF, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_FIRSTPEER */
	{ "Query split-horizon forced on", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },		/* EIGRP_DUALEV_QSHON */
	{ "Query split-horizon forced off", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },		/* EIGRP_DUALEV_QSHOFF */
	{ "Metric set", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },					/* EIGRP_DUALEV_SETRD */
	{ "Route OBE net/refCnt", EIGRP_DUAL_NET, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_OBE */
	{ "Update squashed", EIGRP_DUAL_NET, EIGRP_DUAL_SWIF },					/* EIGRP_DUALEV_UPDATESQUASH */
	{ "Change queue emptied, entries", EIGRP_DUAL_NUM, EIGRP_DUAL_NONE},	/* EIGRP_DUALEV_EMPTYCHANGEQ */
	{ "Requeue ucast", EIGRP_DUAL_ADDR, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_REQUEUEUCAST */
	{ "Xmit size/gap", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_XMITTIME */
	{ "Revise summary", EIGRP_DUAL_ADDR, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_SUMREV */
	{ "added/metric", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },				/* EIGRP_DUALEV_SUMREV2 */
	{ "min metric/result code", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM },		/* EIGRP_DUALEV_SUMREV3 */
	{ "queued for recalculation", EIGRP_DUAL_NONE, EIGRP_DUAL_NONE },			/* EIGRP_DUALEV_SUMDEP */
	{ "Get sum metric", EIGRP_DUAL_ADDR, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_GETSUM */
	{ "min metric", EIGRP_DUAL_NUM, EIGRP_DUAL_NONE },					/* EIGRP_DUALEV_GETSUM2 */
	{ "Process summary", EIGRP_DUAL_ADDR, EIGRP_DUAL_ADDR },				/* EIGRP_DUALEV_PROCSUM */
	{ "added/metric ptr", EIGRP_DUAL_NUM, EIGRP_DUAL_HEX_NUM },		/* EIGRP_DUALEV_PROCSUM2 */
	{ "Ignored route, metric", EIGRP_DUAL_ADDR, EIGRP_DUAL_NUM },			/* EIGRP_DUALEV_IGNORE */
	{ "Unknown", EIGRP_DUAL_NUM, EIGRP_DUAL_NUM }					/* EIGRP_DUALEV_UNKNOWN */
};
/************************************************************************************

	Name:	EigrpDualShowEventParam

	Desc:	This function decode event type, and show a particular parameter from an event. Print 
			into a supplied buffer for caller's convenience.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			type		- the event type
			bufin	- pointer to the parameter
			bufout	- pointer to the supplied buffer
			numcharout	- pointer to the bytes number of the parameter
	
	Ret:		
************************************************************************************/

S8 *EigrpDualShowEventParam(EigrpDualDdb_st *ddb, U32 type, S8 *bufin, S8 *bufout, U32 bufoutSize, S32 *numcharout)
{
	U32	data;
	S32	charcnt = 0;

	EIGRP_FUNC_ENTER(EigrpDualShowEventParam);
	data = *(U32 *) bufin;
	switch(type){
		case EIGRP_DUAL_NONE:
			charcnt = 0;
			bufout[0]	= '\0';
			break;
			
		case EIGRP_DUAL_BOOL:
			charcnt = sprintf_s(bufout, bufoutSize, "%s ", (data ? "TRUE" : "FALSE"));
			break;
			
		case EIGRP_DUAL_SWIF:
			if((void *) data != NULL){
				break;
			}
			
		case EIGRP_DUAL_NUM:
			charcnt = sprintf_s(bufout, bufoutSize, "%u ", data);
			break;
			
		case EIGRP_DUAL_HEX_NUM:
			charcnt = sprintf_s(bufout, sizeof(bufout), "%#x ", data);
			break;
			
		case EIGRP_DUAL_NET:
			charcnt = sprintf_s(bufout, bufoutSize, "%s ", (*ddb->printNet)((EigrpNetEntry_st *) bufin));
			break;
			
		case EIGRP_DUAL_ADDR:
			charcnt = sprintf_s(bufout, bufoutSize,"%s ", (*ddb->printAddress)((U32 *) bufin));
			break;
			
		case EIGRP_DUAL_STRING:
			charcnt = sprintf_s(bufout, bufoutSize,"%s ", bufin);
			break;
			
		case EIGRP_DUAL_RTORIGIN:
			charcnt = sprintf_s(bufout, bufoutSize, "%s ", EigrpDualRouteType2string((U32) data));
			break;
			
		case EIGRP_DUAL_QUERY_ORIGIN:
			charcnt = sprintf_s(bufout, bufoutSize, "%s ", EigrpDualQueryOrigin2str((U32) data));
			break;
			
		case EIGRP_DUAL_RTEVENT:
			charcnt = sprintf_s(bufout, bufoutSize, "%s ", EigrpDualRouteEvent2String((U32) data));
			break;
			
		case EIGRP_DUAL_PKT_TYPE:
			charcnt = sprintf_s(bufout, bufoutSize, "%s ", EigrpOpercodeItoa((S32) data));
			break;
			
		default:
			charcnt = sprintf_s(bufout, bufoutSize, "0x%x ", data);
			break;
	}
	*numcharout = charcnt;
	EIGRP_FUNC_LEAVE(EigrpDualShowEventParam);
	
	return(bufin + dual_event_length[ type ]);
}

/************************************************************************************

	Name:	EigrpDualFormatEventRecord

	Desc:	This function decode event type, and show the  parameter from the event. Print into a 
			supplied buffer for caller's convenience.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			buffer	- pointer to the supplied buffer
			evt_ptr	- pointer to the event
			counter	- the serial number of the event
			print_time	- unused
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualFormatEventRecord(EigrpDualDdb_st *ddb, S8 *buffer, U32 bufLen, EigrpDualEvent_st *evt_ptr, U32 counter, S32 print_time)
{
	U32	data1_type, data2_type;
	S8	*outbuf, *type, *buf;
	S32	numchar;
	const EigrpDualShowEventDecode_st *decodep;

	EIGRP_FUNC_ENTER(EigrpDualFormatEventRecord);
	decodep		= &dual_event_table[ evt_ptr->code ];
	type			= decodep->message;
	data1_type	= decodep->data1;
	data2_type	= decodep->data2;
	outbuf		= (S8 *) buffer;
	if(print_time){
		numchar = sprintf_s(outbuf, bufLen, "%4d %s: ", counter, type);
	}else{
		numchar = sprintf_s(outbuf, bufLen, "%4d %s: ", counter, type);
	}
	outbuf += numchar;
	buf = &evt_ptr->buf[0];
	buf = EigrpDualShowEventParam(ddb, data1_type, buf, outbuf, (bufLen - numchar), &numchar);
	outbuf += numchar;
	(void)EigrpDualShowEventParam(ddb, data2_type, buf, outbuf, (bufLen - numchar), &numchar);
	EIGRP_FUNC_LEAVE(EigrpDualFormatEventRecord);

	return;
}

/************************************************************************************

	Name:	EigrpDualLogEvent

	Desc:	This function log each event. This routine will be called from some other EigrpIp 
			routine with useful data. The data will be stored for later display.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			code		- the event code
			data1	- pointer to the data of the event
			data2	- pointer to the data of the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualLogEvent(EigrpDualDdb_st *ddb, U32 code, void *data1, void *data2)
{
	EigrpDualEvent_st	*curptr;
	U32	len1, len2;
	S8	*buf, log_string[ 120 ];

	EIGRP_FUNC_ENTER(EigrpDualLogEvent);
	/* If the system has crashed, forget it. */

	/* If logging is disabled, forget it. */
	if(!ddb->flagDdbLoggingEnabled){
		EIGRP_FUNC_LEAVE(EigrpDualLogEvent);
		return;
	}

	/* If no memory, forget it. */
	if(ddb->events == NULL){
		EIGRP_FUNC_LEAVE(EigrpDualLogEvent);
		return;
	}

	curptr		= ddb->eventCurPtr;
	curptr->code	= code;
	buf			= &curptr->buf[0];
	len1			= dual_event_length[ dual_event_table[ code ].data1 ];
	len2			= dual_event_length[ dual_event_table[ code ].data2 ];
	if((len1 + len2 <= EIGRP_DUAL_EVENTS_BUF_SIZE)){
		if(len1 && data1){
			EigrpPortMemCpy(buf, data1, len1);
		}
		if(len2 && data2){
			EigrpPortMemCpy(buf + len1, data2, len2);
		}
	}

	/* Bump and circle. */
	if(ddb->flagLogEvent){
		EigrpDualFormatEventRecord(ddb, log_string, sizeof(log_string), curptr, ddb->eventCnt, FALSE);
		ddb->eventCnt++;
		EIGRP_ASSERT(0);
	}
	ddb->eventCurPtr++;

	if(ddb->eventCurPtr >= (ddb->events + ddb->maxEvent)){
		ddb->eventCurPtr = ddb->events;
	}
	EIGRP_FUNC_LEAVE(EigrpDualLogEvent);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualInitEvents

	Desc:	This function allocate memory for eigrp event logging queue.
 
			If sense is FALSE, the size is set to the default.
 
	Para: 	ddb		- pointer to the dual descriptor block 
			event_buffer_size		- the queue length
			sense	- 	If it is FALSE, the size is set to the default.
	
	Ret:		TRUE 	for success
			FALSE 	for malloc failed.

************************************************************************************/

S32	EigrpDualInitEvents(EigrpDualDdb_st *ddb, U32 event_buffer_size, S32 sense)
{
	EIGRP_FUNC_ENTER(EigrpDualInitEvents);
	if(!sense){
		event_buffer_size = EIGRP_DUAL_DEF_MAX_EVENTS;
	}

	ddb->maxEvent = event_buffer_size;

	/* If it's already there, then start over. */

	if(ddb->events){
		EigrpPortMemFree(ddb->events);
		ddb->events			= NULL;
		ddb->eventCurPtr		= NULL;
	}

	/* Free the SIA buffer, in case we're resizing. */
	if(ddb->siaEvent){
		EigrpPortMemFree(ddb->siaEvent);
		ddb->siaEvent	= NULL;
		ddb->siaEventPtr	= NULL;
	}

	/* If the size is zero, don't waste the memory. */

	if(event_buffer_size == 0){
		EIGRP_FUNC_LEAVE(EigrpDualInitEvents);
		return TRUE;
	}

	/* Don't use log function, ddb->events is not needed, */
	ddb->siaEvent		= NULL;
	ddb->siaEventPtr		= NULL;
	ddb->events			= NULL;
	ddb->eventCurPtr	= NULL;
	EIGRP_FUNC_LEAVE(EigrpDualInitEvents);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpDualClearEventLogging

	Desc:	This function turn off event logging.
		
	Para: 	ddb		- pointer to the dual descriptor block 
				
	Ret:		NONE
************************************************************************************/

void	EigrpDualClearEventLogging(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpDualClearEventLogging);
	ddb->flagDdbLoggingEnabled = FALSE;
	EIGRP_FUNC_LEAVE(EigrpDualClearEventLogging);

	return;
}

/************************************************************************************

	Name:	EigrpDualClearLog

	Desc:	This function empties out all event logs, and reenabled event logging.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualClearLog(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpDualClearLog);
	(void)EigrpDualInitEvents(ddb, ddb->maxEvent, TRUE);
	ddb->flagDdbLoggingEnabled = TRUE;
	EIGRP_FUNC_LEAVE(EigrpDualClearLog);

	return;
}

/************************************************************************************

	Name:	EigrpDualAdjustScratchPad

	Desc:	This function adjust the size of the scratchpad area to match the number of IIDBs that
			are active for DUAL.  We should only adjust upward, because life is rife with race
			conditions when IIDBs get deleted(since some DRDBs received on that interface may
			linger).
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualAdjustScratchPad(EigrpDualDdb_st *ddb)
{
	U32 stsize;

	EIGRP_FUNC_ENTER(EigrpDualAdjustScratchPad);
	if(!ddb->scratchTblSize ||
			(ddb->scratchTblSize < ddb->iidbQue->count)){
		ddb->scratchTblSize = MAX(ddb->iidbQue->count, 1);

		/* Free any old one. */

		if(ddb->scratch){
			EigrpPortMemFree(ddb->scratch);
			ddb->scratch = (EigrpDualIidbScratch_st*) 0;
		}

		/* Allocate the new one.  It may be NULL. */

		stsize = ddb->scratchTblSize *sizeof(EigrpDualIidbScratch_st);
		if(stsize){
			ddb->scratch = EigrpPortMemMalloc(stsize);
			if(ddb->scratch){
				EigrpUtilMemZero((void *) ddb->scratch, stsize);
			}
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDualAdjustScratchPad);

	return;
}

/************************************************************************************

	Name:	EigrpDualProcSiaTimerCallbk

	Desc:	This function is the callback routine for SIA timer of OS system. 
 
 			Delete(disable) the OS system timer, add a timer event to the task sched.
		
	Para: 	dummy	- unused
			param	- pointer to the data of the new task
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualProcSiaTimerCallbk(void * param)
{
	EigrpDualDdb_st	*dbd;

	EIGRP_FUNC_ENTER(EigrpDualProcSiaTimerCallbk);

	dbd = (EigrpDualDdb_st *)param;
	if(dbd == NULL  || ! EigrpDdbIsValid(dbd)){
		EIGRP_FUNC_LEAVE(EigrpDualProcSiaTimerCallbk);
		return;
	}
	//zhurish 2016-3-3 屏蔽删除定时器函数
	//EigrpPortTimerDelete(&dbd->threadAtciveTimer);
	dbd->siaTimerProc	= TRUE;

#if(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/*	EigrpUtilSchedAdd((S32 (*)(void *))EigrpDualOnTaskActiveTimer, dbd);*/
#else
	//zhurish 2016-3-3 屏蔽加入调度队列操作
	//EigrpUtilSchedAdd((S32 (*)(void *))EigrpDualOnTaskActiveTimer, dbd);
#endif
	EIGRP_FUNC_LEAVE(EigrpDualProcSiaTimerCallbk);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualOnTaskActiveTimer

	Desc:	This function is the routine which truely scan the SIA timer for every DNDB.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualOnTaskActiveTimer(EigrpDualDdb_st *ddb)
{
	S32	finished;
	
	EIGRP_FUNC_ENTER(EigrpDualOnTaskActiveTimer);
	EIGRP_TRC(DEBUG_EIGRP_TIMER, "TIMERS: scan active timer expired.(at %u)\n", 
				EigrpPortGetTimeSec());
	if(!ddb || ! EigrpDdbIsValid(ddb)){
		EIGRP_FUNC_LEAVE(EigrpDualOnTaskActiveTimer);
		return;
	}

	EigrpDualScanActive(ddb, &finished);/* tigerwh 120608  */

	EIGRP_ASSERT(!ddb->threadAtciveTimer);
	ddb->threadAtciveTimer = EigrpPortTimerCreate(EigrpDualProcSiaTimerCallbk,
												(void *) ddb,
							/* tigerwh 120608  */	(finished == FALSE) ? 0: (S32)(EIGRP_ACTIVE_SCAN_FREQUENCY / EIGRP_MSEC_PER_SEC));
	EIGRP_FUNC_LEAVE(EigrpDualOnTaskActiveTimer);

	return;
}

/************************************************************************************

	Name:	EigrpDualInitTimers

	Desc:	This function initialize DDB managed timers.
		
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualInitTimers(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpDualInitTimers);
	/* Initialize the master timer for the Router process. */
	EigrpUtilMgdTimerInitParent(&ddb->masterTimer, NULL, EigrpProcMgdTimerCallbk, ddb);

	/* Initialize the master timer for the Hello process. */
	EigrpUtilMgdTimerInitParent(&ddb->helloTimer, NULL, EigrpProcHelloTimerCallbk, ddb);

	/* Initialize the children of the master timer. */
	EigrpUtilMgdTimerInitParent(&ddb->intfTimer, &ddb->masterTimer, NULL, NULL);
	EigrpUtilMgdTimerInitParent(&ddb->peerTimer, &ddb->masterTimer, NULL, NULL);

	EIGRP_ASSERT(!ddb->threadAtciveTimer);
	ddb->threadAtciveTimer = EigrpPortTimerCreate(EigrpDualProcSiaTimerCallbk, (void *) ddb,
												(S32)(EIGRP_ACTIVE_SCAN_FREQUENCY / EIGRP_MSEC_PER_SEC));


	EigrpUtilMgdTimerInitLeaf(&ddb->pdmTimer, &ddb->masterTimer, EIGRP_PDM_TIMER, NULL, FALSE);
	EigrpUtilMgdTimerInitLeaf(&ddb->aclChgTimer, &ddb->masterTimer, EIGRP_DDB_ACL_CHANGE_TIMER, NULL, FALSE);
	EIGRP_FUNC_LEAVE(EigrpDualInitTimers);

	return;
}

/************************************************************************************

	Name:	EigrpDualInitDdb

	Desc:	This function set up common DDB information

			Returns TRUE if successful.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			hello_process_name	- unused
			extdata_size			- size of the ddb's extDataChunk
			workqsize		- sizeof the ddb's workQueChunk
	
	Ret:		TRUE
************************************************************************************/

S32	EigrpDualInitDdb(EigrpDualDdb_st *ddb, S8 *hello_process_name, U32 extdata_size, U32 workq_size)
{
	EIGRP_FUNC_ENTER(EigrpDualInitDdb);
	ddb->threadTail			= &ddb->threadHead;
	ddb->chgQueTail	= &ddb->chgQue;
	ddb->nextSerNo			= 1;		/* zero is reserved */

	ddb->extDataChunk = (EigrpChunk_st *)EigrpUtilChunkCreate(extdata_size,
															EIGRP_DUAL_EXT_DATA_CHUNK_SIZE,
															EIGRP_CHUNK_FLAG_DYNAMIC,
															NULL,
															0,
															"EIGRP ExtData");
	ddb->workQueChunk = (EigrpChunk_st *)EigrpUtilChunkCreate(workq_size,
															EIGRP_DUAL_WORKQ_CHUNK_SIZE,
															EIGRP_CHUNK_FLAG_DYNAMIC,
															NULL,
															0,
															"EIGRP WorkQ");
	EigrpProcessStarting(ddb);  
	ddb->iidbQue		= EigrpUtilQue2Init();
	ddb->temIidbQue	= EigrpUtilQue2Init();
	ddb->passIidbQue	= EigrpUtilQue2Init();
	(void)EigrpDualInitEvents(ddb, EIGRP_DUAL_DEF_MAX_EVENTS, TRUE);
	ddb->masterTimerProc	= FALSE;
	ddb->helloTimerProc	= FALSE;
	ddb->siaTimerProc	= FALSE;
	EigrpDualInitTimers(ddb);
	/* ddb->flagDdbLoggingEnabled = TRUE; */
	ddb->flagDdbLoggingEnabled	= FALSE;
	ddb->flagDualLogging		= EIGRP_DUAL_LOGGING_DUAL_DEFAULT;
	ddb->flagDualXportLogging	= EIGRP_DUAL_LOGGING_XPORT_DEFAULT;
	ddb->flagDualXmitLogging	= EIGRP_DUAL_LOGGING_XMIT_DEFAULT;
	ddb->flagLogAdjChg		= EIGRP_DEF_LOG_ADJ_CHANGE;
	EigrpDualAdjustScratchPad(ddb);	/* Ensure that it has at least one. */
	EIGRP_FUNC_LEAVE(EigrpDualInitDdb);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpDualFreeShowBlocks

	Desc:	This function free up the list of blocks allocated for EigrpDualShowEvents.  Uneventful.
		
	Para: 	block_head	- pointer to the head of the blocks list
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualFreeShowBlocks(EigrpDualShowEventBlock_st *block_head)
{
	EigrpDualShowEventBlock_st	*next_block;

	EIGRP_FUNC_ENTER(EigrpDualFreeShowBlocks);
	while(block_head){
		next_block	= block_head->next_block;
		EigrpPortMemFree(block_head);
		block_head	= next_block;
	}
	EIGRP_FUNC_LEAVE(EigrpDualFreeShowBlocks);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualShowEvents

	Desc:	This function shows logged events.  It starts at the bottom	and works its way up.  
			Event types are defined above, and determine how the data will be displayed. In order 
			to avoid having to allocate another giant contiguous block of memory, we copy the log
			into a series of smaller blocks so that we can print it even if memory has become
			somewhat fragmented.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			which	- event type
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualShowEvents(EigrpDualDdb_st *ddb, S32 which, U32 start, U32 end)
{
	EigrpDualEvent_st	*copy_ptr, *src_events, *src_curptr, *src_ptr;
	EigrpDualShowEventBlock_st	*block_head, *block_cur, **block_prev;
	S32	copy_done, entry_counter;
	U32	block_counter;
	S8	print_buffer[ 120 ];
	
	EIGRP_FUNC_ENTER(EigrpDualShowEvents);
	switch(which){
		case EIGRP_EVENT_CUR:
			src_events	= ddb->events;
			src_curptr	= ddb->eventCurPtr;
			break;
			
		case EIGRP_EVENT_SIA:
			/* Show SIA event log */
			src_events = ddb->siaEvent;
			src_curptr = ddb->siaEventPtr;
			break;
			
		default:
			EIGRP_FUNC_LEAVE(EigrpDualShowEvents);
			return;
	}
	if((src_events == NULL) || (src_events->code == 0)){
		EIGRP_FUNC_LEAVE(EigrpDualShowEvents);
		return;
	}

	/* Copy the log into a linked list of smaller blocks, so that we have a prayer of being able to
	  * do this even when memory is fragmented even a little and we're using a mondo log size. 
	  * We copy in reverse order so that our list can be traversed front-to-back. */
	block_head	= NULL;
	block_prev	= &block_head;
	src_ptr		= src_curptr;
	copy_done	= FALSE;
	while(!copy_done){
		/* Create the next block.  If we run out of space, free and bail. */
		block_cur = EigrpPortMemMalloc(sizeof(EigrpDualShowEventBlock_st));
		if(block_cur == NULL){
			EigrpDualFreeShowBlocks(block_head);
			EIGRP_FUNC_LEAVE(EigrpDualShowEvents);
			return;
		}
		EigrpUtilMemZero((void *) block_cur, sizeof(EigrpDualShowEventBlock_st));

		/* Link the new block into the chain. */
		*block_prev	= block_cur;
		block_prev	= &block_cur->next_block;

		/* Copy in the elements. */
		entry_counter	= EIGRP_DUAL_SHOW_EVENT_ENTRY_COUNT;
		copy_ptr		= &block_cur->event_entry[0];
		while(entry_counter--){
			/* Back-bump the log pointer, and circle as necessary. */
			src_ptr--;
			if(src_ptr < src_events){
				src_ptr = src_events + (ddb->maxEvent - 1);
			}
			EigrpPortMemCpy((void *) copy_ptr, (void *)src_ptr, sizeof(EigrpDualEvent_st));
			copy_ptr++;
			if(src_ptr == src_curptr){
				/* We hit the end! */
				copy_done = TRUE;
				break;
			}
		}
	}

	/* Got it.  Now run down it and print it. */
	/* automore_enable(NULL); */
	block_cur		= block_head;
	copy_ptr		= &block_head->event_entry[0];
	entry_counter	= 0;
	for(block_counter = 1;
		block_counter <= ddb->maxEvent;
		block_counter++, entry_counter++, copy_ptr++){

		/* If we're past the end of the current block, advance to the next one. */
		if(entry_counter >= EIGRP_DUAL_SHOW_EVENT_ENTRY_COUNT){
			block_cur = block_cur->next_block;
			if(block_cur == NULL){
				break;
			}
			copy_ptr = &block_cur->event_entry[0];
			entry_counter = 0;
		}

		/* Bail if we're done. */
		if(copy_ptr->code == 0){
			break;
		}

		if(copy_ptr->code >= EIGRP_DUALEV_UNKNOWN){
			copy_ptr->code = EIGRP_DUALEV_UNKNOWN;
		}

		/* Format the item if we're supposed to show this one. */
		if(!start || (block_counter >= start && block_counter <= end)){
			EigrpDualFormatEventRecord(ddb, print_buffer, sizeof(print_buffer), copy_ptr, block_counter, TRUE);
		}

		/* Bail out if things are getting ugly. */
	}
	EigrpDualFreeShowBlocks(block_head);
	EIGRP_FUNC_LEAVE(EigrpDualShowEvents);

	return;
}

/************************************************************************************

	Name:	EigrpDualComputeMetric

	Desc:	This function is the 32 bit version of the igrp metric computation. It uses 64 bit 
			arithemetic(S32 S32) to take care of the overflow problem.
		
	Para: 	bw		- bandwidth of vector metric
			load		- load of vector metric
			delay	- delay of vector metric
			rely		- reliability of vector metric
			k1,k2,k3,k4,k5	- the K value
	
	Ret:		the composite metric
************************************************************************************/

U32	EigrpDualComputeMetric(U32 bw, U32 load, U32 delay, U32 rely,
							U32 k1, U32 k2, U32 k3, U32 k4, U32 k5)
{
	U32	m, metric;
	S8	half;

	EIGRP_FUNC_ENTER(EigrpDualComputeMetric);
	metric	= 0;
	half		= 0;

	if(k1){
		if(bw == EIGRP_METRIC_INACCESS){
			EIGRP_FUNC_LEAVE(EigrpDualComputeMetric);
			return EIGRP_METRIC_INACCESS;
		}
		m = bw * k1;
		if(m >= EIGRP_METRIC_INACCESS_HALF){
			m -= EIGRP_METRIC_INACCESS_HALF;
			half++;
		}
		metric += m;
	}
	if(k2){
		m = (bw * k2) / (256 - load);
		if(m >= EIGRP_METRIC_INACCESS_HALF){
			m -= EIGRP_METRIC_INACCESS_HALF;
			half++;
		}
		metric += m;
	}
	if(metric >= EIGRP_METRIC_INACCESS_HALF){
		metric -= EIGRP_METRIC_INACCESS_HALF;
		half++;
	}
	if(k3){
		if(delay == EIGRP_METRIC_INACCESS){
			EIGRP_FUNC_LEAVE(EigrpDualComputeMetric);
			return EIGRP_METRIC_INACCESS;
		}
		m = delay * k3;
		if(m >= EIGRP_METRIC_INACCESS_HALF){
			m -= EIGRP_METRIC_INACCESS_HALF;
			half++;
		}
		metric += m;
	}
	if(metric >= EIGRP_METRIC_INACCESS_HALF){
		metric -= EIGRP_METRIC_INACCESS_HALF;
		half++;
	}

	if((metric == 0)&&(half == 0)){
		EIGRP_FUNC_LEAVE(EigrpDualComputeMetric);
		return 1;
	}

	if(half >= 2){
		EIGRP_FUNC_LEAVE(EigrpDualComputeMetric);
		return EIGRP_METRIC_INACCESS;
	}
	if(half) {
		metric += EIGRP_METRIC_INACCESS_HALF;
	}
	EIGRP_FUNC_LEAVE(EigrpDualComputeMetric);

	return(metric);
}

/************************************************************************************

	Name:	EigrpDualPeerCount

	Desc:	This function returns the number of peers on an interface.
		
	Para: 	pointer to the given interface
	
	Ret:		the number of peers on the interface
************************************************************************************/

U32	EigrpDualPeerCount(EigrpIdb_st *iidb)
{
	return(iidb->handle.used);
}

/************************************************************************************

	Name:	EigrpDualLogXportAll

	Desc:	This function log a transport event if enabled, without a target check.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			event	- the event code
			param1	- pointer to the data of the event
			param2	- pointer to the data of the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualLogXportAll(EigrpDualDdb_st *ddb, U32 event, void *param1, void *param2)
{
	EIGRP_FUNC_ENTER(EigrpDualLogXportAll);
	if(ddb->flagDualXportLogging){
		EigrpDualLogEvent(ddb, event, param1, param2);
	}
	EIGRP_FUNC_LEAVE(EigrpDualLogXportAll);

	return;
}

/************************************************************************************

	Name:	EigrpDualFinishUpdateSend

	Desc:	This function Go finish sending updates if the change queue has something in it.
		
	Para: 	ddb		- pointer to the dual descriptor block 
		
	Ret:		NONE	
************************************************************************************/

void	EigrpDualFinishUpdateSend(EigrpDualDdb_st *ddb)
{
	EIGRP_FUNC_ENTER(EigrpDualFinishUpdateSend);
	if(ddb->chgQue){
		EigrpDualFinishUpdateSendGuts(ddb);
	}
	EIGRP_FUNC_LEAVE(EigrpDualFinishUpdateSend);

	return;
}

/************************************************************************************

	Name:	EigrpDualRouteIsExternal

	Desc:	This function returns TRUE if the DNDB contains an external route.
		
	Para: 	dndb	- pointer to the given DNDB
	
	Ret:		TRUE	for the DNDB contains an external route
			FALSE	for it is not
************************************************************************************/

S32	EigrpDualRouteIsExternal(EigrpDualNdb_st *dndb)
{
	EigrpDualRdb_st *drdb;

	EIGRP_FUNC_ENTER(EigrpDualRouteIsExternal);
	/* No RDBs?  Shouldn't happen... */
	drdb = dndb->rdb;
	if(!drdb){
		EIGRP_FUNC_LEAVE(EigrpDualRouteIsExternal);
		return FALSE;
	}
	EIGRP_FUNC_LEAVE(EigrpDualRouteIsExternal);

	return(drdb->flagExt);
}

/************************************************************************************

	Name:	EigrpDualIdb

	Desc:	This function return the idb given the IIDB.
		
	Para: 	iidb		- pointer to the given IIDB
	
	Ret:		the idb of the given IIDB
************************************************************************************/

EigrpIntf_pt EigrpDualIdb(EigrpIdb_st *iidb)
{
	EIGRP_FUNC_ENTER(EigrpDualIdb);
	if(!iidb){
		EIGRP_FUNC_LEAVE(EigrpDualIdb);
		return NULL;
	}
	EIGRP_FUNC_LEAVE(EigrpDualIdb);
	
	return(iidb->idb);
}

/************************************************************************************

	Name:	EigrpDualDndbActive

	Desc:	This function returns TRUE if the DNDB route is in active state
		
	Para: 	dndb	- pointer to the given DNDB
	
	Ret:		TRUE	for the DNDB route is in active state
			FALSE	for it is not
************************************************************************************/

S32	EigrpDualDndbActive(EigrpDualNdb_st *dndb)
{
	return(dndb->replyStatus.array != NULL);
}

/************************************************************************************

	Name:	EigrpDualIgrpMetricToEigrp

	Desc:	This function convert a metric scaled for IGRP into one scaled for EIGRP.
		
	Para: 	igrp_metric	- the metric scaled for IGRP
		
	Ret:		the metric scaled for EIGRP		
************************************************************************************/

U32	EigrpDualIgrpMetricToEigrp(U32 igrp_metric)
{
	return(igrp_metric << EIGRP_METRIC_IGRP_SHIFT);
}

/************************************************************************************

	Name:	EigrpDualKValuesMatch

	Desc:	This function verify that K-values of hello packet match our configured values. Do 
			comparison in such a way as to prevent the compiler from optimizing the comparison into
			short/long memory references.  Such references lead to misaligned accesses on some
			platforms.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			param	- pointer to the param TLV which contains the K value
	
	Ret:		TRUE 	for the the K-values of hello packet match out configured values
			FALSE	for it is not so
************************************************************************************/

S32	EigrpDualKValuesMatch(EigrpDualDdb_st *ddb, EigrpParamTlv_st *param)
{
	return(EigrpPortMemCmp((U8 *)&param->k1, (U8 *)&ddb->k1, EIGRP_K_PARA_LEN) == 0);
}

/************************************************************************************

	Name:	EigrpDualLogXmitAll

	Desc:	This function Log a transmit event if enabled, without a target check.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			event	- event code
			param1	- pointer to the data of the event
			param2	- pointer to the data of the event
	
	Ret:		NONE
************************************************************************************/

void	EigrpDualLogXmitAll(EigrpDualDdb_st *ddb, U32 event, void *param1, void *param2)
{
	EIGRP_FUNC_ENTER(EigrpDualLogXmitAll);
	if(ddb->flagDualXmitLogging){
		EigrpDualLogEvent(ddb, event, param1, param2);
	}
	EIGRP_FUNC_LEAVE(EigrpDualLogXmitAll);
	
	return;
}

/************************************************************************************

	Name:	EigrpDualLogAll

	Desc:	This function Log a DUAL event if enabled, without a target check.
		
	Para: 	
	
	Ret:		
************************************************************************************/

void	EigrpDualLogAll(EigrpDualDdb_st *ddb, U32 event, void *param1, void *param2)
{
	return;
}

/************************************************************************************

	Name:	EigrpDualLogXmit

	Desc:	This function Log a transmit event with a target check. 
		
	Para: 
	
	Ret:		
************************************************************************************/

void	EigrpDualLogXmit(EigrpDualDdb_st *ddb, U32 event, EigrpNetEntry_st *target, void *param1, void *param2)
{
	return;
}

/************************************************************************************

	Name:	EigrpDualDebugEnqueuedPacket

	Desc:	This function issues a debug message for an enqueued packet, if appropriate.
		
	Para: 
	
	Ret:		
************************************************************************************/

void	EigrpDualDebugEnqueuedPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpIdb_st *iidb, EigrpPackDesc_st *pktDesc)
{
	return;
}

/************************************************************************************

	Name:	EigrpDualLogXport

	Desc:	This function Log a transport event with a target check. 
		
	Para: 
	
	Ret:		
************************************************************************************/

void	EigrpDualLogXport(EigrpDualDdb_st *ddb, U32 event, U32 *target, void *param1, void *param2)
{
	return;
}

/************************************************************************************

	Name:	EigrpDualLog

	Desc:	This function Log a DUAL event with a target check.
		
	Para: 
	
	Ret:		
************************************************************************************/

void	EigrpDualLog(EigrpDualDdb_st *ddb, U32 event, EigrpNetEntry_st *target, void *param1, void *param2)
{
	return;
}

/************************************************************************************

	Name:	EigrpDualDebugSendPacket

	Desc:	This function Issues a debug message for a sent packet, if appropriate.
		
	Para: 
	
	Ret:		
************************************************************************************/

void	EigrpDualDebugSendPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpIdb_st *iidb, 
								EigrpPackDesc_st *pktDesc, EigrpPktHdr_st *eigrp, U16 retryCnt)
{
	return;
}



