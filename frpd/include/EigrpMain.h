#ifndef _EIGRP_MAIN_H
#define _EIGRP_MAIN_H

#ifdef	__cplusplus
extern	"C"  {
#endif

#define	EIGRP_UTIL_ROUND_UP(a, size)	(((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))
#define	EIGRP_UTIL_PAGE_SIZE			16

#define	EIGRP_KEYID_MAX	 	0x7FFFFFFF
#define	EIGRP_KEYID_MIN 		0
#define	EIGRP_KEYID_LIMIT		0,	EIGRP_KEYID_MAX

/* The following is bogus.  We should really figure out how big we can make this based on the MTU
  * of the interface and protocol.  However, there's no way to deal with the situation where the
  * size of the sequence TLV is too large, other than to simply drop it, along with the multicast
  * packet that it is associated with, and unicast the packet instead. */
#define	EIGRP_MAX_SEQ_TLV		1500		/* Max size of a Sequence TLV */
#define	EIGRP_PDB_WORK_QUE_LEN 		5000		/*  256  */
#define	EIGRP_PDB_CMD_QUE_LEN		1000		/*  32   */

#define	EIGRP_REDISTRIBUTE_FLAG_SET    	0x0001
#define	EIGRP_REDISTRIBUTE_FLAG_UNSET  	0x0002
#define	EIGRP_REDISTRIBUTE_FLAG_METRIC	0x0010
#define	EIGRP_REDISTRIBUTE_FLAG_TYPE		0x0020
#define	EIGRP_REDISTRIBUTE_FLAG_RTMAP	0x0040
#define	EIGRP_REDISTRIBUTE_FLAG_TAG		0x0080
#define	EIGRP_REDISTRIBUTE_FLAG_WEIGHT	0x0100
#define	EIGRP_REDISTRIBUTE_FLAG_MTRT	(EIGRP_REDISTRIBUTE_FLAG_METRIC|EIGRP_REDISTRIBUTE_FLAG_RTMAP)
#define	EIGRP_MAX_ROUTEMAPNAME_LEN		32

enum{
	EIGRP_DISTRIBUTE_IN = 0,
	EIGRP_DISTRIBUTE_OUT,
	EIGRP_DISTRIBUTE_MAX
};

enum {
	EIGRP_PKT_ACCEPT, 
	EIGRP_PKT_REJECT_WITH_ACK, 
	EIGRP_PKT_REJECT
};

/* Init bit definition. First unicast transmitted Update has this bit set in the flags field of the
  * fixed header. It tells the neighbor to down-load his topology table. */
#define	EIGRP_INIT_FLAG	0x01

typedef struct EigrpRedisEntry_{
	struct EigrpRedisEntry_		*next;
	EigrpVmetric_st			*vmetric;		/* The metric to be used in the redis-import. It is optional */
	void		*rtMap;		/* The higher level */
	S8		rtMapName[EIGRP_MAX_ROUTEMAPNAME_LEN+1];

	U32		proto;		/* The protocol to be redis-import */
	U16		process;	/* The process id of the protocol. It is optional. */
	U32		rcvProto;	/* Nonsense. */
	U16		rcvProc;		/* Nonsense */

	U16		index;		
	U8		flag;		/* The status of this entry. */
}EigrpRedisEntry_st, *EigrpRdisEntry_pt;

typedef struct EigrpRedis_{
	struct EigrpRedis_ *next;
	U8  		flag;
	U32  	vmetric[5];
	S8   		rtMapName[EIGRP_MAX_ROUTEMAPNAME_LEN+1];
	U32  	tagVal;
	U32  	weight;
	U32  	srcProto;
	U16   	srcProc;
	U32  	dstProto;
	U16   	dstProc;
}EigrpRedis_st;

typedef	struct EigrpRedisParam_{
	U32	srcproto;
	U32	srcProc;
	U32	metric;
	U32	ipAddr;
	U32	ipMask;
	U32	gateWay;
	U32	intfId;
	U8	ifName[32];
}EigrpRedisParam_st, *EigrpRedisParam_pt;

/* Packet Descriptor Queue Element
  * This data structure is enqueued to represent the desire to transmit a packet to a particular
  * peer, or on a particular interface.  The packet is represented by an EigrpPackDesc_st
  * structure. */
typedef struct EigrpPktDescQelm_{
	struct EigrpPktDescQelm_ *next;	/* Next element in queue */
	EigrpPackDesc_st *pktDesc;		/* Pointer to packet descriptor */
	
	U32 sentTime;		/* Time transmitted(for RTT) */
	U32 xmitTime;		/* Time to send(for rexmits) */
}EigrpPktDescQelm_st;

/* IDB linkage
  *
  * An thread of these is tied to the IDB (actually it's in the ipinfo field). This provides a
  * quick way to find the IIDB given the IDB. */
typedef struct EigrpIdbLink_{
	struct EigrpIdbLink_	*next;	/* Next one in thread */
	
	EigrpDualDdb_st		*ddb;	/* DDB of owning process */
	EigrpIdb_st			*iidb;	/* IIDB associated with this IDB */
}EigrpIdbLink_st;

/* Generic TLV type used for packet decoding. */
typedef struct EigrpGenTlv_{
	EIGRP_PKT_TL
}EigrpGenTlv_st;

/* TLV for multicast sequence number to go with sequence TLV.
  *
  * This was added to the hello containing the sequence TLV so that the hello packet could be more
  * tightly bound to the multicast packet bearing the CR bit that follows it.  The sequence number
  * of the impending multicast is carried herein. */
typedef struct EigrpNextMultiSeq_{
	EIGRP_PKT_TL
	U32 seqNum;		/* Sequence number of associated packet */
} EigrpNextMultiSeq_st;

typedef struct EigrpParamTlv_{
	EIGRP_PKT_TL
	S8	k1;				/* Bandwidth factor */
	S8	k2;				/* Load factor */
	S8	k3;				/* Delay factor */
	S8	k4;				/* Reliability bias */
	S8	k5;				/* Reliability factor */
	S8	pad;			/* Pad, set to zero, ignored on receipt */
	U16	holdTime;		/* How long we ignore lost hello's */
} EigrpParamTlv_st;

/* this type may result error if the operate system is not network byte order . remarked by  */
typedef struct EigrpAuthTlv_{
	EIGRP_PKT_TL
	U16 authType;		
	U16 	authLen;
	U32	keyId;
	U32	pad[3];
	U32	digest[4];
}EigrpAuthTlv_st;

typedef struct EigrpVerTlv_{
	EIGRP_PKT_TL
	EigrpSwVer_st version;
}EigrpVerTlv_st;

typedef struct EigrpPdb_{
	struct EigrpPdb_	*next;
	EigrpDualDdb_st			*ddb;
	void		*rtTbl;
	S8		name[20];
	S8		pname[20];
	U32		bcastTime,			bcastTimeDef;
	U32		invalidTime,			invalidTimeDef;
	U32		holdTime,			holdTimeDef;
	U32		flushTime,			flushTimeDef;
	U8		multiPath,			multiPathDef;
	U8		distance,			distDef;
	U8		distance2,			dist2Def;
	U8		variance;
	U32		process;
	U32		k1, 		k2, 		k3, 		k4, 		k5;
	U8		maxHop,			maxHopDef;	/* no userful. */
	EigrpVmetric_st		vMetricDef;
	U32			workQueId;					/* event queue */
	U32			cmdQueId;					/* command queue */
	EigrpSched_pt		eventJob;
	EigrpNetEntry_st		netLst;
	EigrpQue_st 		*sumQue;
	EigrpRedisEntry_st		*redisLst;
	EigrpOffset_st    	offset[EIGRP_OFFSET_LEN];	/* offset in/out infomation */

	S32 			rtOutDef;
	S32 			rtInDef;
	S32			cdOut;
	S32			cdIn;
	S32			metricFlagDef;
	S32			sumAuto;

	EigrpNeiEntry_st *nbrLst;

	void		(*query)(struct EigrpPdb_ *, void *);
	void		(*aclChanged)(struct EigrpPdb_ *, void *);
	void		(*hopCntChange)(struct EigrpPdb_ *, U8);
}EigrpPdb_st;

typedef struct EigrpSum_{
	struct EigrpSum_ *next;
	U32 		address;
	U32 		mask;
	EigrpQue_st 	*idbQue;
	U32		minMetric;
	S32      	beingRecal;
}EigrpSum_st;

#define EIGRP_SUM_TYPE_AUTO	0x01
#define EIGRP_SUM_TYPE_CFG		0x02
typedef struct EigrpSumIdb_{
	struct 	EigrpSumIdb_ *next;
	struct	EigrpIntf_	*idb;
	U8	flag;					/* TRUE if configured,FALSE if automatic */
}EigrpSumIdb_st;

#define	EIGRPF_ON			0x01
#define	EIGRPF_TERMINATE	0x02
#define	EIGRPF_BROADCAST	0x04
#define	EIGRPF_SOURCE		0x08
#define	EIGRPF_RECONFIG	0x10

#define	EIGRP_B_DELAY     25600L		/* 1 ms  */
#define	EIGRP_S_DELAY     512000L	/* 20 ms */

/* show topology detail */
#define	EIGRP_TOP_SUMMARY	1
#define	EIGRP_TOP_PASSIVE		2
#define	EIGRP_TOP_PENDING		3
#define	EIGRP_TOP_ACTIVE		4
#define	EIGRP_TOP_ZERO		5
#define	EIGRP_TOP_FS			6
#define	EIGRP_TOP_ALL			7

#define	EIGRP_PKT_SIZE			1500

/* Internet address. */
#define	EIGRP_IP_INVALID_ADDR(ip)	(((ip) & HTONL(0x000000ff)) == 0 || 	\
										((ip) & HTONL(0x000000ff)) == HTONL(255) || 	\
										((ip) & HTONL(0xff000000)) == HTONL(0xff000000) || 	\
										(EigrpUtilIsMcast(ip) == TRUE) ||	\
										(EigrpUtilIsLbk(ip) == TRUE) || 	\
										(EigrpUtilIsBadClass(ip) == TRUE) || 		\
										((ip) & HTONL(0xff000000)) == HTONL(0x00000000))	

#define	EIGRP_IN_CLASSA(a)		((((U32)(a)) & 0x80000000) == 0)
#define	EIGRP_IN_CLASSB(a)		((((U32)(a)) & 0xc0000000) == 0x80000000)
#define	EIGRP_IN_CLASSC(a)		((((U32)(a)) & 0xe0000000) == 0xc0000000)
#define	EIGRP_IN_CLASSD(a)		((((U32)(a)) & 0xf0000000) == 0xe0000000)
#define	EIGRP_IN_CLASSA_NET	0xff000000
#define	EIGRP_IN_CLASSB_NET	0xffff0000
#define	EIGRP_IN_CLASSC_NET	0xffffff00
#define	EIGRP_IN_CLASSD_NET	0xF0000000
#define	EIGRP_IN_NET0(a)		((((U32) (a)) & 0xff000000) == 0x00000000)

#define	EIGRP_MEMBER_OFFSET(type, member)	((U32)((U8 *)&((type *)0)->member - (U8 *)0))
#define	EIGRP_DUAL_LOG(a,b,c, d, e)

#define	EIGRP_DEF_LOG_ADJ_CHANGE		FALSE /* Default is to not log */
#if(EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_COMMERCIAL)
#define	EIGRP_DEF_HELLOTIME				(5 * EIGRP_MSEC_PER_SEC)
#define	EIGRP_DEF_HOLDTIME				(EIGRP_DEF_HELLOTIME * 3)
#elif (EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_YEZONG)
#define	EIGRP_DEF_HELLOTIME				(5 * EIGRP_MSEC_PER_SEC)
#define	EIGRP_DEF_HOLDTIME				(EIGRP_DEF_HELLOTIME * 3)
#elif (EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_DIYUWANG)
#define	EIGRP_DEF_HELLOTIME				(6 * EIGRP_MSEC_PER_SEC)
#define	EIGRP_DEF_HOLDTIME				(20 * EIGRP_MSEC_PER_SEC)
#endif
#ifdef INCLUDE_SATELLITE_RESTRICT
#define	EIGRP_DEF_SATELLITE_RTT			1500
#endif

#define	EIGRP_DEF_ACTIVE_TIME				(3 * EIGRP_MSEC_PER_MIN)
#define	EIGRP_DEF_PASSIVE					FALSE
#define	EIGRP_DEF_INVISIBLE				FALSE
#define	EIGRP_DEF_BANDWIDTH_PERCENT	50 /* Use 50% of the link by default */
#define	EIGRP_DEF_HOLDDOWN				(5*EIGRP_MSEC_PER_SEC)
#define	EIGRP_DEF_AUTOSUM				TRUE
#define	EIGRP_DEF_SPLITHORIZON			TRUE
#define	EIGRP_DEF_DEFROUTE_IN			TRUE
#define	EIGRP_DEF_DEFROUTE_OUT			TRUE
#define	EIGRP_DEF_AUTHMODE				FALSE

#if 1
#define	EIGRP_DEF_IF_BANDWIDTH		100000
#define	EIGRP_DEF_IF_DELAY				0

#else
/*add by cwf 20121222 for test*/
#define	EIGRP_DEF_IF_BANDWIDTH		100
#define	EIGRP_DEF_IF_DELAY				1000
#endif


#define	PEER_TABLE_INCREMENT		10		/* Number of entries to add */
#define	TIMER_LIMITER				20		/* No more than 20 timers per pass */
#define	EIGRP_FUNC_STACK_SIZE	32768

typedef	struct	Eigrp_{
	EigrpIntf_pt	intfLst;    
	void		*semSyncQ;
	struct EigrpSockIn_ *inetMask[34];
	struct EigrpSockIn_ *helloMultiAddr;

	U32		rtrCnt;							/* Count of running EIGRP routers */
	EigrpChunk_st		*pktDescChunks;			/* Packet descriptors */
	EigrpChunk_st		*pktDescQelmChunks; 	/* Queue elements */
	EigrpChunk_st		*anchorChunks;			/* Anchors */
	EigrpChunk_st		*dummyChunks;			/* Dummies */

	U32		pktMaxSize; 						/* Maximum packet size the kernel supports */

	EigrpQue_st		*queLst;
	U32		queId;

	EigrpSched_st		*schedLst;
	EigrpQue_st		*cmdQue;

	EigrpQue_st		*ddbQue;
	EigrpQue_st		*protoQue;

	void		*taskContainer;
	EigrpConf_st	*conf;
	S32		socket;

	U8		*recvBuf;
	U32		recvBufLen;
	U8		*sendBuf;
	U32		sendBufLen;
	
	EigrpDbgCtrl_pt	dbgCtrlLst;
	S8		*strBuf;
	S8		bufMini[256];
	U32		bufLen;
	U32		usedLen;

	U8	funcStack[EIGRP_FUNC_STACK_SIZE][64];
	U32	funcCnt;
	U32	funcMax;
	
	S8	fixLenStrBuf[2048];
	U8	firstPktbuf[2048];   /*zhangming 130108*/
	U32	firstPktCnt;
	S32	firstPktLock;

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS		
	void *eigrpZebos;
#endif// EIGRP_PLAT_ZEBOS			
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	
#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	//2016-3-3 zhurish 屏蔽使用
	//struct	EigrpLinuxTimer_	*timerHdr;
	//void		*timerSem;
#endif//(EIGRP_OS_TYPE==EIGRP_OS_LINUX)

#ifdef _EIGRP_PLAT_MODULE_TIMER
	struct	EigrpLinuxTimer_	timerHdr[_EIGRP_TIMER_MAX+1];
#endif// _EIGRP_PLAT_MODULE

#ifdef INCLUDE_EIGRP_SNMP
	S32		snmpSock;
#endif//INCLUDE_EIGRP_SNMP

#ifdef _DC_
	S32	dcSock;
	void	*dcFDR;
	U8 vlanFlag[15];
#endif//_DC_
	EigrpNetMetric_pt	pNetMetric;
#ifdef _EIGRP_PLAT_MODULE
	void	*EigrpSem;
#endif// _EIGRP_PLAT_MODULE
}Eigrp_st, *Eigrp_pt;

S32	EigrpMulticastTransmitBlocked(EigrpIdb_st *);
void	EigrpStartMcastFlowTimer(EigrpDualDdb_st *, EigrpIdb_st *);
EigrpPktHdr_st *EigrpAllocPktbuff(U32);
void	EigrpNetworkMasksCleanup();
void	EigrpNetworkMasksInit();
S32	EigrpAdjustHandleArray(EigrpHandle_st *, S32);
void	EigrpKickPacingTimer(EigrpDualDdb_st *, EigrpIdb_st *, U32);
void	EigrpFreeQelm(EigrpDualDdb_st *, EigrpPktDescQelm_st *, EigrpIdb_st *, EigrpDualPeer_st *);
void	EigrpCleanupMultipak(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpUpdateMcastFlowState(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpDequeueQelm(EigrpDualDdb_st *, EigrpDualPeer_st *, U32);
U32	EigrpCalculatePacingInterval(EigrpDualDdb_st *, EigrpIdb_st *, U32, S32);
S32	EigrpAdjustPeerArray(EigrpDualDdb_st *, EigrpDualPeer_st *, S32);
EigrpIdb_st *EigrpFindTempIidb(EigrpDualDdb_st *, struct EigrpIntf_ *);
void	EigrpSetPacingIntervals(EigrpDualDdb_st *, EigrpIdb_st *);
EigrpIdb_st *EigrpCreateIidb(EigrpDualDdb_st *, struct EigrpIntf_ *);
EigrpIdb_st *EigrpGetIidb(EigrpDualDdb_st *, struct EigrpIntf_ *);
void	EigrpAllocSendBuffer(U32);
void	EigrpAllocRecvBuffer(U32);
/* Start Edit By  : AuthorName:zhurish : 2016/01/16 16 :51 :45  : Comment:修改操作:.增加参数，用于控制linux环境后台进程 */
#ifdef _EIGRP_PLAT_MODULE
void	EigrpInit(int daemon);
#else//_EIGRP_PLAT_MODULE
void	EigrpInit();
#endif//_EIGRP_PLAT_MODULE
/* End Edit By  : AuthorName:zhurish : 2016/01/16 16 :51 :45  */
void	EigrpConfAsFree();
void	EigrpConfIntfFree();
void	EigrpFree();
void	EigrpMain();
void	EigrpSetHandle(EigrpDualDdb_st *, EigrpHandle_st *, S32);
S32	EigrpCompareSeq (U32, U32);
void	EigrpAllocHandle(EigrpDualDdb_st *, EigrpDualPeer_st *);
S32	EigrpFindhandle(EigrpDualDdb_st *,	 U32 *, struct EigrpIntf_ *);
void	EigrpClearHandle(EigrpDualDdb_st *, EigrpHandle_st *, U32);
void	EigrpFreehandle(EigrpDualDdb_st *, EigrpDualPeer_st *);
S32	EigrpTestHandle(EigrpDualDdb_st *, EigrpHandle_st *, U32);
EigrpDualPeer_st *EigrpHandleToPeer(EigrpDualDdb_st *, U32);
U32 EigrpShouldAcceptSeqPacket(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpPktHdr_st *, U32);
void	EigrpCancelCrMode(EigrpDualPeer_st *);
S32	EigrpAcceptSequencedPacket(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpPktHdr_st *);
S32	EigrpPakSanity(EigrpDualDdb_st *, EigrpPktHdr_st *, U32, U32 *, struct EigrpIntf_ *);
void	EigrpSendAck(EigrpDualDdb_st *, EigrpDualPeer_st *, U32);
void	EigrpTakeAllNbrsDown(EigrpDualDdb_st *, S8 *);
EigrpIdb_st *EigrpFindIidb(EigrpDualDdb_st *, struct EigrpIntf_ *);
EigrpDualPeer_st *EigrpFindPeer(EigrpDualDdb_st *, U32 *, struct EigrpIntf_ *);
U32 EigrpNextPeerQueue(EigrpDualDdb_st *,  EigrpDualPeer_st *);
S32	EigrpDeferPeerTimer(EigrpDualDdb_st *, EigrpIdb_st *, EigrpDualPeer_st *);
void	EigrpStartPeerTimer(EigrpDualDdb_st *, EigrpIdb_st *, EigrpDualPeer_st *);
void	EigrpPostAck(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpPackDesc_st *);
void	EigrpUpdateSrtt(EigrpDualPeer_st *, U32);
void	EigrpCalculateRto(EigrpDualPeer_st *, U32);
void	EigrpProcessAck(EigrpDualDdb_st *, EigrpDualPeer_st *, U32);
void	EigrpNeighborDown(EigrpDualDdb_st *, EigrpDualPeer_st *);
void	EigrpReinitPeer(EigrpDualDdb_st *, EigrpDualPeer_st *);
void	EigrpTakePeerDown_int(EigrpDualDdb_st *, EigrpDualPeer_st *, S32, S8 *, S8 *, int);
#define EigrpTakePeerDown(_ddb, _peer, _destroy_now, _reason)\
	EigrpTakePeerDown_int(_ddb, _peer, _destroy_now, _reason, __FILE__, __LINE__)

void	EigrpProcessStarting(EigrpDualDdb_st *);
S32	EigrpUpdatePeerHoldtimer(EigrpDualDdb_st *, U32 *, struct EigrpIntf_ *);
void	EigrpLogPeerChange(EigrpDualDdb_st *, EigrpDualPeer_st *, S32, S8 *);
void	EigrpFindSequenceTlv (EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpGenTlv_st *, S32);
void	EigrpRecvHello(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpPktHdr_st *, S32);
void	EigrpDumpPacket(void *, U32);
void	EigrpFlowControlTimerExpiry(EigrpDualDdb_st *, EigrpMgdTimer_st *);
void	EigrpTakeNbrsDown_int(EigrpDualDdb_st *, struct EigrpIntf_ *, S32, S8 *, S8 *, int);
#define EigrpTakeNbrsDown(_ddb, _pEigrpIntf, _destroy_now, _reason)\
	EigrpTakeNbrsDown_int(_ddb, _pEigrpIntf, _destroy_now, _reason, __FILE__, __LINE__)
void	EigrpDumpPakdesc(EigrpPackDesc_st *);
EigrpPktHdr_st *EigrpGeneratePacket(EigrpDualDdb_st *, EigrpIdb_st *, EigrpDualPeer_st *, EigrpPackDesc_st *, S32 *);
EigrpGenTlv_st *EigrpBuildSequenceTlv (EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpGenTlv_st *);
void	EigrpBuildPakdesc(EigrpPackDesc_st *, EigrpPktDescQelm_st *);
S32	EigrpEnqueuePeerQelms(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpGenTlv_st **);
void	EigrpGetSwVersion(EigrpSwVer_st *);
S8	*EigrpBuildHello(EigrpIdb_st *, EigrpPktHdr_st *, U32, U32, U32, U32, U32, U16);
void	EigrpUpdateMib(EigrpDualDdb_st *, EigrpPktHdr_st *, S32);
EigrpDualPeer_st *EigrpHeadProc(EigrpDualDdb_st *, EigrpPktHdr_st *, U32, U32 *, struct EigrpIntf_ *);
void	EigrpGenerateMd5 (EigrpPktHdr_st *, EigrpIdb_st *, U32);
void	EigrpGenerateChksum(EigrpPktHdr_st *, U32);
S32	EigrpInduceError(EigrpDualDdb_st *, EigrpIdb_st *, S8 *);
void	EigrpSendHelloPacket(EigrpDualDdb_st *, EigrpPktHdr_st *, EigrpIdb_st *, S32, S32);
void	EigrpSendSeqHello(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpGenTlv_st *);
U32	EigrpPacingValue(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *);
void	EigrpFlowBlockInterface(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPktDescQelm_st **);
void	EigrpSendMulticast(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPktDescQelm_st *);
void	EigrpIntfPacingTimerExpiry(EigrpDualDdb_st *,EigrpMgdTimer_st *);
S32	EigrpRetransmitPacket(EigrpDualDdb_st *, EigrpDualPeer_st *);
void	EigrpSendUnicast(EigrpDualDdb_st *, EigrpIdb_st *, EigrpDualPeer_st *);
void	EigrpPeerSendTimerExpiry(EigrpDualDdb_st *, EigrpMgdTimer_st *);
void	EigrpPacketizeTimerExpire(EigrpDualDdb_st *, EigrpMgdTimer_st *);
void	EigrpProcMgdTimerCallbk(void *);
S32	EigrpProcManagedTimers(EigrpDualDdb_st *);
U32	EigrpComputeMetric(EigrpDualDdb_st *, EigrpVmetric_st *,EigrpNetEntry_st *, EigrpIdb_st *, U32 *);
void	EigrpFlushPeerPackets(EigrpDualDdb_st *, EigrpDualPeer_st *);
void	EigrpDestroyPeer(EigrpDualDdb_st *, EigrpDualPeer_st *);
void	EigrpSendHello(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpProcHelloTimerCallbk(void *);
S32	EigrpProcHelloTimers(EigrpDualDdb_st *);
void	EigrpLinkNewIidb(EigrpDualDdb_st *, EigrpIdb_st *);
S32	EigrpIsIntfPassive(EigrpPdb_st *, struct EigrpIntf_ *);
S32	EigrpIsIntfInvisible(EigrpDualDdb_st *, struct EigrpIntf_ *);
void	EigrpDestroyIidb(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpProcDying(EigrpDualDdb_st *);
S32	EigrpAllocateIidb(EigrpDualDdb_st *, struct EigrpIntf_ *, S32);
void	EigrpFreeDdb(EigrpDualDdb_st *);
void	EigrpDeallocateIidb(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpPassiveInterface(EigrpDualDdb_st *, EigrpIdb_st *, S32);
void	EigrpInvisibleInterface(EigrpDualDdb_st *, EigrpIdb_st *, S32);
U32	EigrpNextSeqNumber(U32);
void	EigrpSetSeqNumber(EigrpDualDdb_st *, EigrpPackDesc_st *);
EigrpPackDesc_st *EigrpEnqueuePak(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpIdb_st *, S32);
S32	EigrpIntfIsFlowReady(EigrpIdb_st *);
void	EigrpFlushXmitQueue(EigrpDualDdb_st *, EigrpIdb_st *,EigrpQue_st *);
void	EigrpFlushIidbXmitQueues(EigrpDualDdb_st *, EigrpIdb_st *);
S8 *EigrpOpercodeItoa(S32);
S32	EigrpProcRouterEigrp(U32);
S32	EigrpProcNoRouterEigrp(U32);
S32	EigrpDdbIsValid(EigrpDualDdb_st *);




/* Start Edit By  : AuthorName:zhurish : 2016/01/14 17 :51 :44  : Comment:修改操作:.无用屏蔽掉 */
#if 0
extern int socketDbgCnt;
extern	void DcPortSockHealthy();
extern	char frpDbgBuf[];
#define	FRP_SOCKET_HEALTY 	if(socketDbgCnt > 10){	\
					sprintf(frpDbgBuf, "FRP_SOCKET_HEALTY errror: file:%s, line:%d, socketDbgCnt:%d\n", __FILE__, __LINE__, socketDbgCnt);	\
					DcPortSockHealthy();	\
					}
#endif /* 0 */
/* End Edit By  : AuthorName:zhurish : 2016/01/14 17 :51 :44  */



#ifdef		__cplusplus
}
#endif	/*end of __cplusplus */

#endif	/*end of _EIGRP_MAIN_H_ */
