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

	Module:	Eigrpd

	Name:	Eigrpd.c

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

	Name:	EigrpPrintThreadSerno

	Desc:	This function prints the serial number associated with a thread entry, or "none" if no
			entry is present.
		
	Para: 	pshowptr		- pointer to the buffer which is used to store the serial number
			thread		- pointer to the given thread entry
	
	Ret:		NONE
************************************************************************************/

void	EigrpPrintThreadSerno(struct EigrpXmitThread_ *thread, struct EigrpDbgCtrl_ *pDbg)
{
	EIGRP_FUNC_ENTER(EigrpPrintThreadSerno);
	/*  we do not printf in this function */
	if(thread){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "%d", thread->serNo);
	}else{
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "<none>");
	}
	EIGRP_FUNC_LEAVE(EigrpPrintThreadSerno);
	
	return;
}

/************************************************************************************

	Name:	EigrpPrintSendFlag

	Desc:	This function prints the contents of the DNDB/DRDB send_flag bits.
		
	Para: 	
	
	Ret:		
************************************************************************************/

S8 *EigrpPrintSendFlag(U16 send_flag)
{
	static S8 str_send[4][8]	 = {
		", R",
		", Q",
		", U",
		"",
	};
	S32 index = send_flag/2;
	
	EIGRP_FUNC_ENTER(EigrpPrintSendFlag);
	if((send_flag == 0) || (send_flag > 4)){
		EIGRP_FUNC_LEAVE(EigrpPrintSendFlag);
		return str_send[3];
	}
	EIGRP_FUNC_LEAVE(EigrpPrintSendFlag);
	
	return str_send[index]; 
}

/************************************************************************************

	Name:	EigrpShowTopology

	Desc:	This function shows the topology table of eigrp process ddb.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			which	- the topology table detail
	
	Ret:		0	for success
			-1 	for fail
************************************************************************************/

U32	EigrpShowTopology(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *), 
							EigrpDualDdb_st *ddb, S32 which)
{
	EigrpXmitThread_st	*thread;
	EigrpDualNdb_st	*dndb = NULL, *next_dndb = NULL ;
	EigrpDualRdb_st	*drdb = NULL, *next_drdb = NULL;
	EigrpIdb_st		*iidb;
	EigrpHandle_st	reply_status;
	EigrpDualPeer_st	*peer;
	EigrpIntf_pt		pEigrpIntf;
	EigrpDbgCtrl_st	dbg;
	S32	i, banner, active, connectFlag, redistributed, rStatic, summary, FSonly, display_drdb;
	U32	handlesize, thread_count, drdb_count, dummy_count, handle_num, usedLen;

	static const S8 ipeigrp_top_banner[]	={
		"\r\nIP-EIGRP Topology Table for process %d :"
	};

	static const S8 topo_banner[]	={
		"\n\rCodes: P - Passive, A - Active, U - Update, Q - Query, R - Reply, "
		"\r\n       r - Reply status\n\r"
	};

	EIGRP_FUNC_ENTER(EigrpShowTopology);
	usedLen	= 0;
	EigrpUtilMemZero((void *) &reply_status, (U32)(sizeof(reply_status)));

	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;

	/* 输出各进程头 */
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, ipeigrp_top_banner, ddb->asystem);

	/* 开始--显示拓扑表概要: show ip eigrp topology summary */
	if(which == EIGRP_TOP_SUMMARY){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\n\rHead serial ");
		
		EigrpPrintThreadSerno(ddb->threadHead, &dbg);
		
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", next serial %d", ddb->nextSerNo);
	}
	thread_count	= 0;
	dummy_count	= 0;
	drdb_count	= 0;
	thread = ddb->threadHead;
	while(thread){
		thread_count++;
		if(thread->dummy){
		    dummy_count++;
		}else if(thread->drdb){
		    drdb_count++;
		}
		thread = thread->next;
	}
	if(which == EIGRP_TOP_SUMMARY){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n%d routes, %d pending replies, %d dummies",
				              			      (thread_count - drdb_count - dummy_count), drdb_count, dummy_count);
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n%s enabled on %d interfaces, neighbors present on %d "
										"interfaces", ddb->name, ddb->iidbQue->count, ddb->activeIntfCnt);
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\n\rQuiescent interfaces: ");
		iidb = ddb->quieIidb;
		while(iidb){
			EIGRP_SHOW_SAFE_SPRINTF(&dbg, " %s", iidb->idb ? (S8 *)iidb->idb->name : "Null0");
			iidb = iidb->nextQuiescent;
		}

		EIGRP_FUNC_LEAVE(EigrpShowTopology);
		return 0;
	}
	/* 结束--显示拓扑表概要: show ip eigrp topology summary */

	/* 下面是以不同的参数来显示拓扑表的各项路由信息*/

	/* 复制一份应答状态表 */
	handlesize = EIGRP_HANDLE_MALLOC_SIZE(ddb->handle.arraySize);
	if(handlesize){
		reply_status.array = EigrpPortMemMalloc(handlesize);
	}
	if(handlesize && !reply_status.array){
		EIGRP_FUNC_LEAVE(EigrpShowTopology);
		return -1;
	}
	if(handlesize && reply_status.array){
		EigrpUtilMemZero((void *)reply_status.array, handlesize);
	}

	/* 循环显示所有需要显示的拓扑表项 */
	banner = FALSE;

	for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
		for(dndb = ddb->topo[ i ]; dndb; dndb = dndb->next){
			/* 处理每一个dndb */
			/* 1.显示dndb信息 */
			active = EigrpDualDndbActive(dndb);
			reply_status.used		= dndb->replyStatus.used;
			reply_status.arraySize	= dndb->replyStatus.arraySize;
			if(active){
				EigrpPortMemCpy((U8 *)reply_status.array, 	(U8 *) dndb->replyStatus.array,
									(U32)(EIGRP_HANDLE_MALLOC_SIZE(reply_status.arraySize)));
			}
			FSonly = FALSE;
			switch(which){
				case EIGRP_TOP_PASSIVE:
					if(active){
						next_dndb = dndb->next;
						continue;
					}
					break;
					
				case EIGRP_TOP_ACTIVE:
					if(!active){
						next_dndb = dndb->next;
						continue;
					}
					break;
					
				case EIGRP_TOP_ZERO:
					if(dndb->succNum > 0){
						next_dndb = dndb->next;
						continue;
					}
					break;
					
				case EIGRP_TOP_PENDING:
					if(!dndb->sndFlag){
						next_dndb = dndb->next;
						continue;
					}
					break;
					
				case EIGRP_TOP_FS:
					/* Print only links that have a reported distance less than FD. */
					FSonly = TRUE;
					break;
					
				case EIGRP_TOP_ALL:
					break;
					
				default:
					break;
			}

			/* 拓扑表的说明栏 */
			if(!banner){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg,  topo_banner);
				banner	= TRUE;
			}
			
			EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n%s %s, %d %successors, FD is ", (active) ? "A" : "P",
											(*ddb->printNet)(&dndb->dest), dndb->succNum, dndb->succ ? "S" : "s");
			if(active){
				if(dndb->oldMetric == (U32) - 1){
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, "Inaccessible");
				}else{
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, "%u", dndb->oldMetric);
				}
			}else{
				if(!dndb->rdb || dndb->rdb->metric == (U32) - 1){
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, "Inaccessible");
				}else{
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, "%u", dndb->rdb->metric);
				}
			}

			EIGRP_SHOW_SAFE_SPRINTF(&dbg, "%s", EigrpPrintSendFlag(dndb->sndFlag));

			if(which == EIGRP_TOP_ALL){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", serno %d", dndb->xmitThread.serNo);
				if(dndb->xmitThread.refCnt){
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", refCnt %d",
					dndb->xmitThread.refCnt);
				}
				if(dndb->xmitThread.anchor){
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", anchored");
				}
			}

			/* Indicate number of outstanding replies. */
			if(reply_status.used != 0){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n    %d replies", reply_status.used);
			}
			if(active){
				/* 显示何时进入Active意义并不大而应该再显示已经进入Active多长时间 */
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", active : %d(from %u)", EIGRP_ELAPSED_TIME(dndb->activeTime),
						                    dndb->activeTime);
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", query-origin: %s", EigrpDualQueryOrigin2str(dndb->origin));
			}

			/* 输出一次1: 输出dndb信息 */
			/* 输出,并将指针指回到缓冲区头.可能造成等待,等待时可能会在函数dual_dndbdelete()中删除
			  *dndb */


			/* 2.处理每一个Drdb */
			for(drdb = dndb->rdb; drdb; drdb = drdb->next){
				/* 只有show ip eigrp top all 时才显示非FS的路由 */
				display_drdb = !(FSonly && (drdb->succMetric >= dndb->oldMetric));
				/* mem_lock(drdb); */
				/* 显示下一跳和Metric */
				connectFlag	= (drdb->origin == EIGRP_ORG_CONNECTED);
				redistributed	= (drdb->origin == EIGRP_ORG_REDISTRIBUTED);
				rStatic			= (drdb->origin == EIGRP_ORG_RSTATIC);
				summary		= (drdb->origin == EIGRP_ORG_SUMMARY);

				if(display_drdb){
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n       %s via %s",
										(dndb->succ == drdb) ? "*" : " ",
										summary ? "Summary" : connectFlag ? "Connected" :
										redistributed ? "Redistributed" :
										rStatic ? "Rstatic" :	
										(*ddb->printAddress)(&drdb->nextHop));

					if(drdb->succMetric == EIGRP_METRIC_INACCESS){
						EIGRP_SHOW_SAFE_SPRINTF(&dbg, " (Infinity/Infinity)");
					}else if(drdb->metric == EIGRP_METRIC_INACCESS){
						EIGRP_SHOW_SAFE_SPRINTF(&dbg, " (Infinity/%u)", drdb->succMetric);
					}else if(!connectFlag){
						EIGRP_SHOW_SAFE_SPRINTF(&dbg, " (%u/%u)", drdb->metric,
											drdb->succMetric);
					}

					/* 应答状态标记 */
					if(active && (drdb->handle != EIGRP_NO_PEER_HANDLE) &&
								EigrpTestHandle(ddb, &reply_status, drdb->handle)){
						EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", r");
						EigrpClearHandle(ddb, &reply_status, drdb->handle);
					}

					/* update, query, reply 报文发送标记*/
					EIGRP_SHOW_SAFE_SPRINTF(&dbg, "%s", EigrpPrintSendFlag(drdb->sndFlag));

					/* 下一跳的接口 */
					if(drdb->iidb){
						pEigrpIntf	= EigrpDualIdb(drdb->iidb);
 						EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", %s", pEigrpIntf ? (S8 *)pEigrpIntf->name : "Null0");
					}

					/* 传输标记 */
					if(drdb->thread.serNo){
						EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", serno %d", drdb->thread.serNo);
					}
					if(drdb->thread.anchor){
						EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", anchored");
					}
				}  /* if(display_drdb) */


			} /* for drdb loop */

			/* 3.显示dndb应答信息 */
			/* 显示正在等待那些邻居的应答, 前面已经显示过应答标记位的不再显示 */
			if(active && reply_status.used){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n    Remaining replies:");
				for(handle_num = 0;
							(handle_num < EIGRP_CELL_TO_HANDLE(reply_status.arraySize));
							handle_num++){
						if(EigrpTestHandle(ddb, &reply_status, handle_num)){
							peer = EigrpHandleToPeer(ddb, handle_num);
							if(peer){
								EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n         via %s, r, %s",
													(*ddb->printAddress)(&peer->source),
													peer->iidb->idb ? (S8 *)peer->iidb->idb->name : "Null0");
							}else{
								EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n         via unallocated handle %d",
															handle_num);
							}

							EigrpClearHandle(ddb, &reply_status, handle_num);
						}
				}
			}

		} /* for dndb loop */
	} /* for topology */

	if(reply_status.array){
		EigrpPortMemFree(reply_status.array);
		reply_status.array = (U32*) 0;
	}
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n");

	EIGRP_FUNC_LEAVE(EigrpShowTopology);

	return 0;
}

/************************************************************************************

	Name:	EigrpShowNeighbors

	Desc:	This function is called by EIGRP protocol dependent functions. Shows neighbors
			infomation of interface pEigrpIntf.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the given interface
			show_detail	- the sign of print the detail information
	
	Ret:		0
************************************************************************************/

U32	EigrpShowNeighbors(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *), 
							EigrpDualDdb_st *ddb, EigrpIntf_pt pEigrpIntf, S32 show_detail)
{
	EigrpDualPeer_st	*peer;
	S32	banner;
	U32	holdTime, t, t1, t2, t3, seq_number;
	EigrpDbgCtrl_st	dbg;

	static const S8 peer_banner[]	={
		" H       Address     Interface   Hold    Uptime     SRTT    RTO    Q   Seq"
		"\r\n                                  (sec)              (ms)          Cnt  Num"
	};

	EIGRP_FUNC_ENTER(EigrpShowNeighbors);
	
	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term		= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;

	
	/* 每个进程标题 */
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\nIP-EIGRP %s for process %d :\r\n", "neighbors", ddb->asystem);

	banner = FALSE;
	/* 每次显示一个邻居的信息 */
	for(peer = ddb->peerLst; peer; peer = peer->nextPeer){
		if(pEigrpIntf && peer->iidb->idb != pEigrpIntf){
			continue;
		}

	    /* 显示栏头,各列的名称单位 */
		if(!banner){
			EIGRP_SHOW_SAFE_SPRINTF(&dbg, peer_banner);
			banner	= TRUE;
		}

		holdTime = EigrpUtilMgdTimerLeftSleeping(&peer->holdingTimer) / EIGRP_MSEC_PER_SEC;

		t	= EigrpPortGetTimeSec() - peer->uptime;
		if(t > EIGRP_SEC_PER_DAY){
			t1 = t / EIGRP_SEC_PER_DAY;
			t2 = (t - t1 * EIGRP_SEC_PER_DAY) / EIGRP_SEC_PER_HOUR;
			t3 = (t - t1 * EIGRP_SEC_PER_DAY - t2 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
		}else{
			t1 = t / EIGRP_SEC_PER_HOUR;
			t2 = (t - t1 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
			t3 = t % EIGRP_SEC_PER_MIN;
		}
		seq_number = peer->lastSeqNo;
		if(seq_number >= 10000){
			seq_number %= 10000;
		}
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
										"\r\n%3d %14s %12s    %-3d   %2d:%2d:%2d    %-4d   %-5d   %-2d  %d",
										peer->peerHandle,
										(*ddb->printAddress)(&peer->source),
										peer->iidb->idb ? (S8 *)peer->iidb->idb->name : "Null0",
										holdTime, t1, t2, t3, peer->srtt, peer->rto,
										peer->xmitQue[EIGRP_UNRELIABLE_QUEUE]->count +
										peer->xmitQue[EIGRP_RELIABLE_QUEUE]->count,
										seq_number);

		if(show_detail ==  TRUE){
			if(peer->lastStartupSerNo){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
												"\r\n   Last startup serial %d",
												peer->lastStartupSerNo);
			}

			EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
											"\r\n   Version %d.%d/%d.%d, ",
											peer->peerVer.majVer,
											peer->peerVer.minVer,
											peer->peerVer.eigrpMajVer,
											peer->peerVer.eigrpMinVer);

			EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
											"Retrans: %d, Retries: %d",
											peer->retransCnt, peer->retryCnt);

			if(peer->flagNeedInit){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", Waiting for Init");
			}

			if(peer->flagNeedInitAck){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", Waiting for Init Ack");
			}

			if(EIGRP_TIMER_RUNNING(peer->reInitStart)){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", reinit for %d sec",
												EIGRP_ELAPSED_TIME(peer->reInitStart));
			}

		}

	} 

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n");

	EIGRP_FUNC_LEAVE(EigrpShowNeighbors);

	return 0;
}

/************************************************************************************

	Name:	EigrpPrintAnchoredSerno

	Desc:	This function prints the serial number associated with an anchored thread, or "none" if
			none is present.
		
	Para: 	pshowptr		- pointer to the string which store the serial number 
			anchor		- pointer to the anchor of the thread
	
	Ret:		NONE
************************************************************************************/

void	EigrpPrintAnchoredSerno(EigrpXmitAnchor_st *anchor, EigrpDbgCtrl_pt pDbg)
{
	EigrpXmitThread_st	*thread;

	EIGRP_FUNC_ENTER(EigrpPrintAnchoredSerno);
	thread = NULL;
	if(anchor){
		thread = anchor->thread;
	}
	EigrpPrintThreadSerno(thread, pDbg);
	EIGRP_FUNC_LEAVE(EigrpPrintAnchoredSerno);
	
	return;
}

/************************************************************************************

	Name:	EigrpShowInterface

	Desc:	This function display EIGRP interface-specific information. 
		
	Para: 	ddb		- pointer to the dual descriptor block 
			pEigrpIntf	- pointer to the given EIGRP interface
			show_detail	- the sign of print the detail information
	
	Ret:		0
************************************************************************************/

U32	EigrpShowInterface(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *), EigrpDualDdb_st *ddb, 
							EigrpIntf_pt pEigrpIntf, S32 show_detail)
{
	EigrpIdb_st		*iidb, *next ;
	EigrpXmitThread_st	*thread;
	U32				pending_routes;
	EigrpDbgCtrl_st	dbg;

	static const S8 if_banner[]	={
		"\r\n                    Xmit Queue   Mean   Pacing Time   Multicast    Pending"
		"\r\nInterface    Peers  Un/Reliable  SRTT   Un/Reliable   Flow Timer   Routes"
	};

	EIGRP_FUNC_ENTER(EigrpShowInterface);
	iidb = next	= NULL;

	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;
	/* 输出每个进程的头和标题栏 */
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\nIP-EIGRP %s for process %d :", "interfaces", ddb->asystem);
	EIGRP_SHOW_SAFE_SPRINTF(&dbg, if_banner);

	/* 每次循环显示一个接口的信息 */
	for(iidb = (EigrpIdb_st*) ddb->iidbQue->head; iidb; iidb = next){

		/* 可以选择只显示一个接口,但是目前总是显示所有接口 */
		if(pEigrpIntf && (iidb->idb != pEigrpIntf)){
			next = iidb ->next;
			continue;
		}

	    /* Count the pending routes. */
		pending_routes = 0;
		if(iidb->xmitAnchor){
			thread = iidb->xmitAnchor->thread;
			while(thread){
				if(!thread->dummy){
					pending_routes++;
				}
				thread = thread->next;
			}
		}

		/* Print out the good stuff. */
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
										"\r\n%9s    %-8d  %-3d/%3d   %-5d   %-4d/%4d       %-4d      %-5d",
										iidb->idb ? (S8 *)iidb->idb->name : "Null0" /* short_namestring */, EigrpDualPeerCount(iidb),
										iidb->xmitQue[EIGRP_UNRELIABLE_QUEUE]->count,
										iidb->xmitQue[ EIGRP_RELIABLE_QUEUE ]->count,
										iidb->totalSrtt / MAX(1, EigrpDualPeerCount(iidb)),
										iidb->sndInterval[ EIGRP_UNRELIABLE_QUEUE ],
										iidb->sndInterval[ EIGRP_RELIABLE_QUEUE ],
										iidb->lastMcastFlowDelay, pending_routes);

		/* Print detailed information if asked. */
		if(show_detail == TRUE){
			EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n  Next xmit serial ");
			EigrpPrintAnchoredSerno(iidb->xmitAnchor, &dbg);

			if(EigrpUtilMgdTimerRunning(&iidb->pktTimer)){
				EIGRP_SHOW_SAFE_SPRINTF(&dbg, ", packetize pending");
			}

		        EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
											"\r\n  Un/reliable mcasts: %d/%d  Un/reliable ucasts: %d/%d",
											iidb->mcastSent[ EIGRP_UNRELIABLE_QUEUE ],
											iidb->mcastSent[ EIGRP_RELIABLE_QUEUE ],
											iidb->unicastSent[ EIGRP_UNRELIABLE_QUEUE ],
											iidb->unicastSent[ EIGRP_RELIABLE_QUEUE ]);

		        EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
											"\r\n  Mcast exceptions: %d  CR packets: %d  ACKs suppressed: %d",
							                            iidb->mcastExceptionSent, iidb->crPktSent,
							                            iidb->ackSupp);

			EIGRP_SHOW_SAFE_SPRINTF(&dbg, 
											"\r\n  Retransmissions sent: %d  Out-of-sequence rcvd: %d",
											iidb->retransSent, iidb->outOfSeqRcvd);
		} /* if(show_detail) */
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "%s", EIGRP_NEW_LINE_STR);

		next = iidb->next;
	}   /* for loop */

	if(!ddb->iidbQue->head){
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n\r\n");
	}else{
		EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n");
	}
	
	EIGRP_FUNC_LEAVE(EigrpShowInterface);
	
	return 0;
}

/************************************************************************************

	Name:	EigrpShowTraffic

	Desc:	This function show traffic infomation of eigrp process ddb.
			
	Para: 	ddb		- pointer to the dual descriptor block 
	
	Ret:		0
************************************************************************************/

U32	EigrpShowTraffic(U8 *name, void *pTerm, U32 id, S32 (* func)(U8 *, void *, U32, U32, U8 *),
						EigrpDualDdb_st *ddb)
{
	EigrpDbgCtrl_st	dbg;

	EIGRP_FUNC_ENTER(EigrpShowTraffic);
	if(name){
		EigrpPortStrCpy(dbg.name, name);
	}
	dbg.term	= pTerm;
	dbg.id	= id;
	dbg.funcShow	= func;
	

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\nIP-EIGRP Traffic Statistics for process %d :", ddb->asystem);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n  Hellos sent/received: %d/%d", ddb->mib.helloSent,
						                  ddb->mib.helloRcvd);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n  Updates sent/received: %d/%d", ddb->mib.updateSent,
						                  ddb->mib.updateRcvd);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n  Queries sent/received: %d/%d", ddb->mib.queriesSent,
						                  ddb->mib.queriesRcvd);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n  Replies sent/received: %d/%d", ddb->mib.repliesSent,
						                  ddb->mib.repliesRcvd);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n  Acks sent/received: %d/%d", ddb->mib.ackSent,
						                  ddb->mib.ackRcvd);

	EIGRP_SHOW_SAFE_SPRINTF(&dbg, "\r\n\r\n");


	EIGRP_FUNC_LEAVE(EigrpShowTraffic);

	return 0;
}

/************************************************************************************

	Name:	EigrpDistributeUpdateInterface

	Desc:	This function update distribute list of interface pEigrpIntf.
		
	Para: 	pointer to the given interface
	
	Ret:		NONE
************************************************************************************/

void	EigrpDistributeUpdateInterface(EigrpIntf_pt	pEigrpIntf)
{
	void *dist;

	EIGRP_FUNC_ENTER(EigrpDistributeUpdateInterface);
	dist = EigrpPortGetDistByIfname(pEigrpIntf->name);
	if(dist){
		EigrpPortDistributeUpdate(dist);
	}
	EIGRP_FUNC_LEAVE(EigrpDistributeUpdateInterface);

	return;
}

/************************************************************************************

	Name:	EigrpDistributeUpdateAll

	Desc:	This function update all interface's distribute list.
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpDistributeUpdateAll()
{
	EigrpIntf_pt	pEigrpIntf;

	EIGRP_FUNC_ENTER(EigrpDistributeUpdateAll);
	for(pEigrpIntf = gpEigrp->intfLst; pEigrpIntf; pEigrpIntf = pEigrpIntf->next){
		EigrpDistributeUpdateInterface(pEigrpIntf);
	}

	EIGRP_FUNC_LEAVE(EigrpDistributeUpdateAll);
	
	return;
}

/************************************************************************************

	Name:	EigrpConfigWriteEigrpNetwork

	Desc:	This function is used to save config infomation,write network config. 
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			config_mode	- unused
	
	Ret:		number of the configed network
************************************************************************************/

S32	EigrpConfigWriteEigrpNetwork(EigrpPdb_st *pdb, S32 config_mode, EigrpDbgCtrl_pt pDbg)
{
	EigrpNetEntry_st	*se;
	S32		write;
	
	EIGRP_FUNC_ENTER(EigrpConfigWriteEigrpNetwork);
	/*  we do not print in this function */
	write	= 0;
	if(pdb->netLst.next == &pdb->netLst){
		EIGRP_FUNC_LEAVE(EigrpConfigWriteEigrpNetwork);
		return write;
	}

	for(se = pdb->netLst.next; se != &pdb->netLst; se = se->next){
		if(config_mode){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " network %s %s %s",
										EigrpUtilIp2Str(se->address), EigrpUtilIp2Str(se->mask),
										EIGRP_NEW_LINE_STR);
		}else{
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, "    %s %s%s", EigrpUtilIp2Str(se->address), 
										EigrpUtilIp2Str(se->mask), EIGRP_NEW_LINE_STR);
		}
		write ++;
	}
	EIGRP_FUNC_LEAVE(EigrpConfigWriteEigrpNetwork);
	
	return write;
}

/************************************************************************************

	Name:	EigrpProto2str

	Desc:	This function return string describes of a protocol. 
		
	Para: 	pro		- the protocol 
	
	Ret:		pointer to the string which indicate the protocol
************************************************************************************/

S8 *EigrpProto2str(U32 pro)
{
	EIGRP_FUNC_ENTER(EigrpProto2str);
	switch(pro){
		case	EIGRP_ROUTE_KERNEL:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "kernel";

		case	EIGRP_ROUTE_CONNECT:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "connected";
			
		case	EIGRP_ROUTE_STATIC:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "static";
			
		case	EIGRP_ROUTE_RIP:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "rip";
			
		case	EIGRP_ROUTE_OSPF:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "ospf";
			
		case	EIGRP_ROUTE_IGRP:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "igrp";
			
		case	EIGRP_ROUTE_BGP:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "bgp";
			
		case	EIGRP_ROUTE_EIGRP:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "eigrp";

		default:
			EIGRP_FUNC_LEAVE(EigrpProto2str);
			return "unknown";
	}
}

/************************************************************************************

	Name:	EigrpConfigWriteEigrpRedistribute

	Desc:	This function is used to save config infomation,write redistribute config. 
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
			cm		- 
	
	Ret:		number of the redistribute network
************************************************************************************/

S32	EigrpConfigWriteEigrpRedistribute(EigrpPdb_st *pdb, S32 cm, EigrpDbgCtrl_pt pDbg)
{
	EigrpRedisEntry_st	*redis;
	S32		write;

	EIGRP_FUNC_ENTER(EigrpConfigWriteEigrpRedistribute);
	write	= 0;
	/*  we do not print in this function */	
	for(redis = pdb->redisLst; redis; redis = redis->next){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "%s redistribute %s", cm ? "" : "     ",
										EigrpProto2str(redis->proto));
		if(redis->process){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, "%s %d", cm ? "     " : "", redis->process);
		}
		if(redis->vmetric){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " metric %d %d %d %d %d",
											redis->vmetric->bandwidth,
											redis->vmetric->delay,
											redis->vmetric->reliability,
											redis->vmetric->load,
											redis->vmetric->mtu);
		}
		if(redis->rtMapName[0]){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " route-map %s", redis->rtMapName);
		}
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, "\r\n");
		write ++;
	}
	
	EIGRP_FUNC_LEAVE(EigrpConfigWriteEigrpRedistribute);
	
	return write;
}

/************************************************************************************

	Name:	EigrpConfigWriteEigrpOffsetList

	Desc:	This function is used to save config infomation,write offset list config. 
		
	Para: 	pdb		- pointer to the EIGRP process descriptor block
	
	Ret:		number of the offset
***********************************************************************************/

S32	EigrpConfigWriteEigrpOffsetList(EigrpPdb_st *pdb, EigrpDbgCtrl_pt pDbg)
{
	EigrpIdb_st	*iidb;
	S32		write;

	EIGRP_FUNC_ENTER(EigrpConfigWriteEigrpOffsetList);
	write	= 0;
	/*  we do not print in this function */
	if(!pdb){
		EIGRP_FUNC_LEAVE(EigrpConfigWriteEigrpOffsetList);
		return write;
	}

	if(pdb->offset[ EIGRP_OFFSET_IN ].acl){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, " offset-list %d in %d%s",
										pdb->offset[ EIGRP_OFFSET_IN ].acl,
										pdb->offset[ EIGRP_OFFSET_IN ].offset,
										EIGRP_NEW_LINE_STR);
		write ++;
	}

	if(pdb->offset[ EIGRP_OFFSET_OUT ].acl){
		EIGRP_SHOW_SAFE_SPRINTF(pDbg, " offset-list %d out %d%s",
										pdb->offset[ EIGRP_OFFSET_OUT ].acl,
										pdb->offset[ EIGRP_OFFSET_OUT ].offset,
										EIGRP_NEW_LINE_STR);
		write ++;
	}

	for(iidb = (EigrpIdb_st *)pdb->ddb->iidbQue->head; iidb; iidb = iidb->next){
		if(iidb->offset[ EIGRP_OFFSET_IN ].acl){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " offset-list %d in %d %s%s",
											iidb->offset[ EIGRP_OFFSET_IN ].acl,
											iidb->offset[ EIGRP_OFFSET_IN ].offset,
											(S8 *)iidb->idb->name,
											EIGRP_NEW_LINE_STR);
			write ++;
		}
		if(iidb->offset[ EIGRP_OFFSET_OUT ].acl){
			EIGRP_SHOW_SAFE_SPRINTF(pDbg, " offset-list %d out %d %s%s",
											iidb->offset[ EIGRP_OFFSET_OUT ].acl,
											iidb->offset[ EIGRP_OFFSET_OUT ].offset,
											(S8 *)iidb->idb->name,
											EIGRP_NEW_LINE_STR);
			write ++;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpConfigWriteEigrpOffsetList);
	
	return write;
}

/************************************************************************************

	Name:	EigrpEventReadAdd

	Desc:	This function set a event read routine for  task eigrp_top. 
		
	Para: 	NONE
	
	Ret:		NONE
************************************************************************************/

void	EigrpEventReadAdd()
{
	EIGRP_FUNC_ENTER(EigrpEventReadAdd);
	
	EigrpUtilSchedAdd((S32 (*)(void *))EigrpRecv, NULL);
	EIGRP_FUNC_LEAVE(EigrpEventReadAdd);
	
	return;
}
