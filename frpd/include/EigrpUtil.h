#ifndef _EIGRP_UTIL_H_
#define _EIGRP_UTIL_H_

#ifdef __cplusplus
extern"C"
{
#endif

/* Be attention to this constant, it maybe not apporiate. */
/* Sizeof(EigrpDualDdb_st) = 2048 = nethashlen *8 + the rest of EigrpDualDdb_st is about 612. and 
  * so the EigrpUtilNetHash len is about 160 . I change it to 128. */

#define	EIGRP_NET_HASH_LEN			128
#define	EIGRP_NET_HASH_MOD(h)		((h) % EIGRP_NET_HASH_LEN)
#define	EIGRP_CHUNK_FLAG_DYNAMIC	0

/* the following time is all measured by second */
#define	EIGRP_ELAPSED_TIME(timestp)			(EigrpPortGetTimeSec() - (timestp))
#define	EIGRP_GET_TIME_STAMP(timestp)			((timestp) = EigrpPortGetTimeSec())
#define	EIGRP_TIMER_STOP(timestp)				((timestp) = 0)
#define	EIGRP_TIMER_RUNNING(timestp)			((timestp))
#define	EIGRP_TIMER_START(timestp, interval)				((timestp) = EigrpPortGetTimeSec() + (interval))
#define	EIGRP_TIMER_RUNNING_AND_SLEEPING(timestp)		((timestp))
#define	EIGRP_TIMER_START_JITTERED(timestp, interval,jitter)	((timestp) = EigrpPortGetTimeSec() + (interval))
#define	EIGRP_TIMER_UPDATEM(timestp, interval)				((timestp) += (interval))
#define	EIGRP_CLOCK_IN_INTERVAL(timestp, interval)			((EigrpPortGetTimeSec()-(timestp))<=(interval))
#define	EIGRP_CLOCK_OUTSIDE_INTERVAL(timestp, interval)	((EigrpPortGetTimeSec()-(timestp))>(interval))

typedef	struct	EigrpDll_{
	struct	EigrpDll_		*forw;
	struct	EigrpDll_		*back;
}EigrpDll_st, *EigrpDll_pt;


typedef struct EigrpMgdTimer_{
	struct EigrpMgdTimer_ *parent;
	struct EigrpMgdTimer_ *leaf;
	struct EigrpMgdTimer_ *subTree;
	struct EigrpMgdTimer_ *next;
	
	U8	flag;
	void	*context;
	S32	type;	
	U32	time;		/* second */
	
	void	*sysTimer;		
}EigrpMgdTimer_st;

#define	EIGRP_MGDF_START	0x01
#define	EIGRP_MGDF_LEAF		0x02
#define	EIGRP_MGDF_SUBTREE	0x04
#define	EIGRP_MGDF_TREE		0x08

/* chunk type and function */
typedef struct EigrpChunk_{
	U32	size;
	U16	maxLen;
	U16	queLen;
}EigrpChunk_st;

#define	EIGRP_CHUNK_DESTROY(chunks){ \
	EIGRP_ASSERT(!chunks->queLen); \
	EigrpPortMemFree(chunks); \
	chunks = (EigrpChunk_st*)0;\
}

#define	EIGRP_SCALED_BANDWIDTH(band)	(((band)>>2) ? 2500000000UL/((band)/4) : 2500000000UL/9600)
#define	EIGRP_QUEUE_PARA_SIZE			16
#define	EIGRP_PREFIX_TO_MASK(plen)		(0xffffffff << (32 - (plen)))

typedef struct EigrpQueElem_ {
	struct EigrpQueElem_ *next;
	S8	data[EIGRP_QUEUE_PARA_SIZE];
} EigrpQueElem_st;

typedef struct EigrpQue_ {
	struct EigrpQue_ *next;
	struct EigrpQue_ *prev;
	U32	id;
	struct EigrpQueElem_ *head;
	U32	count;
	U32	MaxLen;
	void	*lock;
}EigrpQue_st;

typedef	struct	EigrpSched_{
	struct	EigrpSched_	*forw;
#ifdef _EIGRP_PLAT_MODULE
	S32	(*func)( void *);
#else//_EIGRP_PLAT_MODULE
	S32	(*func)( );
#endif	//_EIGRP_PLAT_MODULE
	void	*data;

	U32	count;	/* This member is only for the head of the list. */
	void	*sem;	/* This member is only for the head of the list. */
}EigrpSched_st, *EigrpSched_pt;

#define	EigrpRbBlack 	1
#define	EigrpRbRed 		0

typedef struct EigrpRbNodeX_{
	struct EigrpRbNodeX_	*lChildX, *rChildX, *parentX;
	S32   rbColorX;
} EigrpRbNodeX_st, *EigrpRbNodeX_pt;

typedef struct EigrpRbTreeX_{
	EigrpRbNodeX_pt    root;
	EigrpRbNodeX_st   	tail;
	S32       (*compare)(EigrpRbNodeX_pt, EigrpRbNodeX_pt);
	void       (*del)(EigrpRbNodeX_pt);
}EigrpRbTreeX_st, *EigrpRbTreeX_pt;

typedef struct EigrpRtNode_{
	EigrpRbNodeX_st 	node;
	
	U32	dest;	/* Dest network addr, which is addr&mask;*/
	U32	mask;	/* Dest network mask; */
	struct EigrpRtEntry_	*rtEntry;
	struct EigrpRtEntry_	*defEntry;
	
	void	*extData;	
}EigrpRtNode_st, *EigrpRtNode_pt;

typedef struct EigrpRtTree_{
	EigrpRbTreeX_st  	tree;
}EigrpRtTree_st, *EigrpRtTree_pt;

typedef struct EigrpRtEntry_{
	struct EigrpRtEntry_	*next;
	
	U32	dest;
	U32	mask;
	U32	gateWay;
	U32	intfId;	
	U32	metric;
	U32	type;
	U32	flag;
	
	U32	sec;
	U32	milSec;
	U32	timeout;

	void *timer;
	void	*node;
	void	*extData;
}EigrpRtEntry_st, *EigrpRtEntry_pt;

#define	EIGRP_MD5_S11		7
#define	EIGRP_MD5_S12		12
#define	EIGRP_MD5_S13		17
#define	EIGRP_MD5_S14		22
#define	EIGRP_MD5_S21		5
#define	EIGRP_MD5_S22		9
#define	EIGRP_MD5_S23		14
#define	EIGRP_MD5_S24		20
#define	EIGRP_MD5_S31		4
#define	EIGRP_MD5_S32		11
#define	EIGRP_MD5_S33		16
#define	EIGRP_MD5_S34		23
#define	EIGRP_MD5_S41		6
#define	EIGRP_MD5_S42		10
#define	EIGRP_MD5_S43		15
#define	EIGRP_MD5_S44		21

/* EIGRP_MD5_F, EIGRP_MD5_G, EIGRP_MD5_H and EIGRP_MD5_I are basic MD5 functions. */
#define	EIGRP_MD5_F(x, y, z)		(((x) & (y)) | ((~x) & (z)))
#define	EIGRP_MD5_G(x, y, z)	(((x) & (z)) | ((y) & (~z)))
#define	EIGRP_MD5_H(x, y, z)	((x) ^ (y) ^ (z))
#define	EIGRP_MD5_I(x, y, z)		((y) ^ ((x) | (~z)))

/* EIGRP_MD5_ROTATE_LEFT rotates x left n bits. */
#define	EIGRP_MD5_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* EIGRP_MD5_FF, EIGRP_MD5_GG, EIGRP_MD5_HH, and EIGRP_MD5_II transformations for rounds 1, 2, 3, 
  * and 4. Rotation is separate from addition to prevent recomputation. */
#define	EIGRP_MD5_FF(a, b, c, d, x, s, ac){ \
	(a)	+= EIGRP_MD5_F((b), (c), (d)) + (x) + (U32)(ac); \
	(a)	= EIGRP_MD5_ROTATE_LEFT((a), (s)); \
	(a)	+= (b); \
}
#define	EIGRP_MD5_GG(a, b, c, d, x, s, ac){ \
	(a)	+= EIGRP_MD5_G((b), (c), (d)) + (x) + (U32)(ac); \
	(a)	= EIGRP_MD5_ROTATE_LEFT((a), (s)); \
	(a)	+= (b); \
}
#define	EIGRP_MD5_HH(a, b, c, d, x, s, ac){ \
	(a)	+= EIGRP_MD5_H((b), (c), (d)) + (x) + (U32)(ac); \
	(a)	= EIGRP_MD5_ROTATE_LEFT((a), (s)); \
	(a)	+= (b); \
}
#define	EIGRP_MD5_II(a, b, c, d, x, s, ac){ \
	(a)	+= EIGRP_MD5_I((b), (c), (d)) + (x) + (U32)(ac); \
	(a)	= EIGRP_MD5_ROTATE_LEFT((a), (s)); \
	(a)	+= (b); \
}

typedef struct EigrpMD5Ctx_{
	U32	state[4];				/* state (ABCD) */
	U32	count[2];			/* number of bits, modulo 2^64 (lsb first) */
	U8	buffer[64];			/* input buffer */
}EigrpMD5Ctx_st, *EigrpMD5Ctx_pt;

typedef struct	EigrpDbgCtrl_{
	struct	EigrpDbgCtrl_	*forw;
	struct	EigrpDbgCtrl_	*back;
	
	U8	name[64];
	void	*term;
	U32	id;
	U32	flag;

	S32 (*funcShow)(U8 *, void *, U32, U32, U8 *);
	
}EigrpDbgCtrl_st, *EigrpDbgCtrl_pt;

#define	EIGRP_SHOW_SAFE_BUF		gpEigrp->bufMini
#define	EIGRP_SHOW_SAFE_SPRINTF(termDbg, ...)	\
		{	\
			U32	byteLen	= EIGRP_SHOW_SPRINTF (EIGRP_SHOW_SAFE_BUF, sizeof(EIGRP_SHOW_SAFE_BUF), __VA_ARGS__);	\
			EigrpCmdEnsureSize((U8 **)&gpEigrp->strBuf, &gpEigrp->bufLen, gpEigrp->usedLen, byteLen);	\
			gpEigrp->usedLen += EIGRP_SHOW_SPRINTF(gpEigrp->strBuf + gpEigrp->usedLen, (gpEigrp->bufLen - gpEigrp->usedLen), "%s", gpEigrp->bufMini);	\
			EigrpPortPrintDbg(gpEigrp->strBuf, termDbg);	\
			gpEigrp->usedLen	= 0;	\
		}


#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
/* 2016-3-3 zhurish 屏蔽使用
typedef	struct	EigrpLinuxTimer_{
	struct	EigrpLinuxTimer_	*forw;
	struct	EigrpLinuxTimer_	*back;

	U32		periodSec;
	U32		trigSec;

	S32		(*func)(void *);
	void		*para;
}EigrpLinuxTimer_st, *EigrpLinuxTimer_pt;
*/
#endif

#ifdef _EIGRP_PLAT_MODULE_TIMER//zhurish 2016-2-28 

#define _EIGRP_TIMER_MAX	64

typedef	struct	EigrpLinuxTimer_{
	U32		used;//判断这个有没有使用
	U32		active;	
	struct timeval tv;
	struct timeval interuppt_tv;
	S32		(*func)(void *);
	void		*para;
}EigrpLinuxTimer_st, *EigrpLinuxTimer_pt;
#endif// _EIGRP_PLAT_MODULE_TIMER


S32	EigrpUtilMgdTimerType(EigrpMgdTimer_st *);
void *EigrpUtilMgdTimerContext(EigrpMgdTimer_st *);
S32	EigrpUtilMgdTimerRunning(EigrpMgdTimer_st *);
U32	EigrpUtilMgdTimerExpTime(EigrpMgdTimer_st *);
U32	EigrpUtilMgdTimerLeftSleeping(EigrpMgdTimer_st *);
S32	EigrpUtilMgdTimerIsParent(EigrpMgdTimer_st *);
void	EigrpUtilMgdTimerInitParent(EigrpMgdTimer_st *,  EigrpMgdTimer_st *, void (*)(void *), void *);
void	EigrpUtilMgdTimerInitLeaf(EigrpMgdTimer_st *, EigrpMgdTimer_st *, S32, void *, S32);
void	EigrpUtilMgdTimerDelete(EigrpMgdTimer_st *);
void	EigrpUtilMgdTimerStart(EigrpMgdTimer_st *, U32);
void	EigrpUtilMgdTimerStop(EigrpMgdTimer_st *);
S32	EigrpUtilMgdTimerExpired(EigrpMgdTimer_st *);
EigrpMgdTimer_st	*EigrpUtilMgdTimerFirstExpired(EigrpMgdTimer_st *);
EigrpMgdTimer_st	*EigrpUtilMgdTimerMinExpired(EigrpMgdTimer_st *);
void	EigrpUtilMgdTimerSetExptime(EigrpMgdTimer_st *, U32);
void	EigrpUtilMgdTimerStartJittered(EigrpMgdTimer_st *, U32, S32);
U32	EigrpUtilInNetof(U32);
U32	EigrpUtilNetHash(U32);
void *EigrpUtilChunkMalloc(EigrpChunk_st *);
void	EigrpUtilChunkFree(EigrpChunk_st *, void *);
EigrpChunk_st *EigrpUtilChunkCreate(U32, U16 , U16, void *, U16, S8 *);
EigrpQue_st *EigrpUtilQueGetById(U32);
S32	EigrpUtilQueCreate(U32, U32 *);
S32	EigrpUtilQueDelete(U32);
S32	EigrpUtilQueGetMsgNum(U32, U32 *);
S32	EigrpUtilQueFetchElem(U32, U32 *);
S32	EigrpUtilQueWriteElem(U32, U32 *);
EigrpQue_st	*EigrpUtilQue2Init();
void	EigrpUtilQue2Enqueue(EigrpQue_st *, EigrpQueElem_st *);
void	EigrpUtilQue2Unqueue(EigrpQue_st *, EigrpQueElem_st *);
EigrpQueElem_st	*EigrpUtilQue2Dequeue(EigrpQue_st *);
void	EigrpUtilSchedInit();
void	EigrpUtilSchedClean();
struct	EigrpSched_	*EigrpUtilSchedAdd(S32 (*)(void *), void *);
struct	EigrpSched_	*EigrpUtilSchedGet();
void	EigrpUtilSchedCancel(struct	EigrpSched_ *);
U8	*EigrpUtilIp2Str(U32);
U32	EigrpUtilMask2Len(U32 mask);
U32	EigrpUtilLen2Mask(U32 len);
void	EigrpUtilMemZero(void *, U32);
S32	EigrpUtilIsLbk(U32);
S32	EigrpUtilIsMcast(U32);
S32	EigrpUtilIsBadClass(U32);
void	EigrpUtilRbXTreeInit(EigrpRbTreeX_pt, S32 (*)(EigrpRbNodeX_pt, EigrpRbNodeX_pt), void(*)(EigrpRbNodeX_pt));
void	EigrpUtilRbXTreeDestroy(EigrpRbTreeX_pt);
void	EigrpUtilRbXTreeCleanUp(EigrpRbTreeX_pt);
S32	EigrpUtilRbXInsert(EigrpRbTreeX_pt, EigrpRbNodeX_pt);
void	EigrpUtilRbXDelete( EigrpRbTreeX_pt, EigrpRbNodeX_pt);
void	EigrpUtilRbXDeleteFixup(EigrpRbTreeX_pt, EigrpRbNodeX_pt);
void	EigrpUtilRbXLeftRotate(EigrpRbTreeX_pt, EigrpRbNodeX_pt);
void	EigrpUtilRbXRightRotate(EigrpRbTreeX_pt,EigrpRbNodeX_pt);
EigrpRbNodeX_pt	EigrpUtilRbXFind( EigrpRbTreeX_pt, EigrpRbNodeX_pt);
EigrpRbNodeX_pt	EigrpUtilRbXGetFirst(EigrpRbTreeX_pt);
EigrpRbNodeX_pt	EigrpUtilRbXGetNext( EigrpRbTreeX_pt, EigrpRbNodeX_pt);
EigrpRtTree_pt	EigrpUtilRtTreeCreate();
void  EigrpUtilRtTreeDestroy(EigrpRtTree_pt);
void  EigrpUtilRtTreeCleanUp(EigrpRtTree_pt);
EigrpRtNode_pt	EigrpUtilRtNodeGetFirst(EigrpRtTree_pt);
EigrpRtNode_pt	EigrpUtilRtNodeGetNext(EigrpRtTree_pt, EigrpRtNode_pt);
S32	EigrpUtilRtNodeCompare(EigrpRbNodeX_pt, EigrpRbNodeX_pt);
void	EigrpUtilRtNodeCleanUp(EigrpRtNode_pt);
EigrpRtNode_pt	EigrpUtilRtNodeFindExact(EigrpRtTree_pt, U32, U32);
EigrpRtNode_pt	EigrpUtilRtNodeAdd(EigrpRtTree_pt, U32, U32);
void	EigrpUtilRtNodeDel(EigrpRtTree_pt, EigrpRtNode_pt);	
U32	EigrpUtilGetNaturalMask(U32);
U16	EigrpUtilGetChkSum(U8 *, U32);
void	EigrpUtilMD5Init(struct EigrpMD5Ctx_ *);
void	EigrpUtilMD5Update(struct EigrpMD5Ctx_ *, U8 *, U32);
void	EigrpUtilMD5Final(U8 *, struct EigrpMD5Ctx_ *);
void	EigrpUtilMD5Transform(U32 *, U8 *);
void	EigrpUtilMD5Encode(U8 *, U32 *, U32);
void	EigrpUtilMD5Decode(U32 *, U8 *, U32);
void	EigrpUtilGenMd5Digest(U8 *, U32, U8 *);
S32	EigrpUtilVerifyMd5(struct EigrpPktHdr_ *, struct EigrpIdb_ *, U32);
S32	EigrpUtilShowProtocol(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *),struct EigrpPdb_ *);
S32	EigrpUtilShowRoute(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), struct EigrpPdb_ *);
S32	EigrpUtilShowIpPdb(struct EigrpPdb_ *, EigrpDbgCtrl_pt);
S32	EigrpUtilShowIpDdb(struct EigrpDualDdb_ *, EigrpDbgCtrl_pt);
S32	EigrpUtilShowStruct(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), struct EigrpPdb_ *);
S32	EigrpUtilShowDebugInfo(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
struct EigrpAuthInfo_ *EigrpBuildAuthInfo(S8 *);
S32	EigrpUtilCmpNet(void *, void *);
void	*EigrpUtilDllSearch(EigrpDll_pt *, void *, S32 (*)(void *, void *));
void	EigrpUtilDllInsert(struct EigrpDll_ * , struct EigrpDll_ * , struct EigrpDll_ **);
void	EigrpUtilDllRemove(struct EigrpDll_ * ,struct EigrpDll_ **);
void EigrpUtilDllCleanUp(struct EigrpDll_ **, void(*)(void **));
void	EigrpUtilDbgCtrlFree(struct EigrpDbgCtrl_ *);
struct	EigrpDbgCtrl_	*EigrpUtilDbgCtrlCreate(U8 *, void *, U32);
struct	EigrpDbgCtrl_	*EigrpUtilDbgCtrlFind(U8 *, void *, U32);
S32	EigrpUtilDbgCtrlDel(U8 *, void *, U32);
void EigrpUtilOutputBuf(U32, U8 * );
S32	EigrpUtilConvertStr2Ipv4(U32 *, U8 *);
S32	EigrpUtilConvertStr2Ipv4Mask(U32 *, U8 *);
#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
//void	EigrpUtilLinuxTimerCheck();
#endif
#ifdef _EIGRP_PLAT_MODULE_TIMER
S32	EigrpUtilTimervalGet(struct timeval *val);
S32	EigrpUtilTimerCallRunning(EigrpLinuxTimer_pt timetbl, int max);
#endif//_EIGRP_PLAT_MODULE_TIMER

void	EigrpUtilDndbHealthy(U8 *, U32);
#ifdef _DC_
void	EigrpUitlDcSchedAdd();
#endif// _DC_

#ifdef __cplusplus
}
#endif

#endif
