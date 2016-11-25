#ifndef _EIGRP_IP_H
#define _EIGRP_IP_H

#ifdef		__cplusplus
extern	"C"  {
#endif

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#ifdef _PACK_STRUCT
	#undef	_PACK_STRUCT
#endif
#define	_PACK_STRUCT __attribute__ ((packed))
#endif

struct EigrpRedis_;
struct EigrpSum_;

#define	EIGRP_INET_MASK_NATURAL_BYTE(x)		(gpEigrp->inetMask[EigrpIpInetMask(x)])
#define	EIGRP_INET_CLASS_VALID_BYTE(x)		BIT_MATCH(EigrpIpInetFlags(x), EIGRP_INET_CLASSF_NETWORK)

#define	EIGRP_INET_CLASSF_NETWORK		0x01
#define	EIGRP_INET_CLASSF_LOOPBACK		0x02
#define	EIGRP_INET_CLASSF_DEFAULT		0x04
#define	EIGRP_INET_CLASSF_RESERVED		0x08
#define	EIGRP_INET_CLASSF_MULTICAST		0x10
#define	EIGRP_INET_CLASSE_NET				HTONL(0xfffff000)

#define	EIGRP_INET_MASK_DEFAULT		0		/* 0.0.0.0 */
#define	EIGRP_INET_MASK_CLASSA		8		/* 255.0.0.0 */
#define	EIGRP_INET_MASK_CLASSB		16		/* 255.255.0.0 */
#define	EIGRP_INET_MASK_CLASSC		24		/* 255.255.255.0 */
#define	EIGRP_INET_MASK_CLASSD		32		/* 255.255.255.255 */
#define	EIGRP_INET_MASK_CLASSE		20		/* 255.255.240.0 */

#define	EIGRP_INET_MASK_MULTICAST	EIGRP_INET_MASK_CLASSD
#define	EIGRP_INET_MASK_HOST			32		/* 255.255.255.255 */
#define	EIGRP_INET_MASK_LOOPBACK		EIGRP_INET_MASK_HOST
#define	EIGRP_INET_MASK_INVALID		33		/* Oops! */

#define	EIGRP_IPEIGRP_REQUEST			0x0101	/* Unused */
#define	EIGRP_METRIC					0x0102
#define	EIGRP_EXTERNAL				0x0103
	
/*ip address 0.0.0.0 */
#define	EIGRP_IPADDR_ZERO				0x0

/* External TLV value definitions. */
enum IPEIGRP_PROT_ID {
	EIGRP_PROTO_NULL,
	EIGRP_PROTO_IGRP,
	EIGRP_PROTO_EIGRP,
	EIGRP_PROTO_STATIC,
	EIGRP_PROTO_RIP,
	EIGRP_PROTO_HELLO,
	EIGRP_PROTO_OSPF	,
	EIGRP_PROTO_ISIS,
	EIGRP_PROTO_EGP,
	EIGRP_PROTO_BGP,
	EIGRP_PROTO_IDRP,
	EIGRP_PROTO_CONNECTED
};

/* Used for decoding multiple destinations in an EigrpIpMetric_st */
#define	EIGRP_IP_EXT_PKT \
	U32	routerId;		/* Router ID of injecting router */       \
	U32	asystem;		/* Originating autonomous system number */ \
	U32	tag;			/* Arbitrary tag */                        \
	U32	metric;		/* External protocol metric */             \
	U16	ereserved;	/* Fluff */                                \
	U8	protocol;		/* External protocol ID */                    \
	S8	flag;			/*Internal/external flag */

#define	IPEIGRPMPDECODE \
	U8	mask;		/* Size of netmask in bits */              \
	U8	number[1];	/* 1 to 4 significant bytes of IP address */

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	typedef struct EigrpIpMpDecode_{
		IPEIGRPMPDECODE
	}_PACK_STRUCT EigrpIpMpDecode_st;

	typedef struct EigrpIpMetric_{
		EIGRP_PKT_TL
		U32 nexthop;
		EIGRP_PKT_METRIC
		EigrpIpMpDecode_st intDest[1];	/* At least one per TLV */
	}_PACK_STRUCT EigrpIpMetric_st;

	typedef struct EigrpIpExtern_{
		EIGRP_PKT_TL
		U32 nexthop;
		EIGRP_IP_EXT_PKT
		EIGRP_PKT_METRIC
		EigrpIpMpDecode_st extDest[1];	/* At least one per TLV */
	}_PACK_STRUCT EigrpIpExtern_st;
#else
#pragma pack(1)
	typedef struct EigrpIpMpDecode_{
		IPEIGRPMPDECODE
	}EigrpIpMpDecode_st;

	typedef struct EigrpIpMetric_{
		EIGRP_PKT_TL
		U32 nexthop;
		EIGRP_PKT_METRIC
		EigrpIpMpDecode_st intDest[1];	/* At least one per TLV */
	} EigrpIpMetric_st;

	typedef struct EigrpIpExtern_{
		EIGRP_PKT_TL
		U32 nexthop;
		EIGRP_IP_EXT_PKT
		EIGRP_PKT_METRIC
		EigrpIpMpDecode_st extDest[1];	/* At least one per TLV */
	} EigrpIpExtern_st;
#pragma pack()
#endif

#define	EIGRP_METRIC_TYPE_SIZE	(sizeof(EigrpIpMetric_st) - sizeof(EigrpIpMpDecode_st))
#define	EIGRP_EXTERN_TYPE_SIZE	(sizeof(EigrpIpExtern_st) - sizeof(EigrpIpMpDecode_st))
/* Kludge exterior flag bit into part of reserved field within EIGRP_PKT_METRIC */
#define	EIGRP_METRIC_TYPE_FLAGS(ptr)	(((EigrpVmetricDecode_pt)ptr)->mreserved[1])

typedef struct EigrpExtData_{
	EIGRP_IP_EXT_PKT
} EigrpExtData_st;

/* IP-EIGRP worktype for queuing redistribution route entries. */
typedef struct EigrpWork_{
	struct EigrpWork_ *next;
	U32 type;
	union {
		struct {
			void *ifp;
			U32	ipAddr;
			U32 ipMask;
			S32 index;
			S32 sense;
		} ifc;
		struct {
			struct EigrpNetEntry_  dest;
			U32 pdbindex;
			S32 event;
		} red;

		struct {
			struct EigrpNetEntry_  dest;
			void *ifp;
			S8  ifname[EIGRP_NAME_SIZE + 1];
			S32 up;
			S32 config;
		} con;

		struct {
			U32 ulindex;
			U32 mask;
			S32 sense;
		} rea;

		struct {
			struct EigrpNetEntry_  dest;
		}d;

		struct {
			void *ifp;
		} ifd;

		struct {
			struct EigrpNetEntry_  dest;
			S32 add;
			EigrpVmetric_st *vmetric;
		} sum;

		struct {
			/* add for store main address, eigrp_enqueue_ip_address_command() */
			void	*ifp;
			S8	ifname[EIGRP_NAME_SIZE + 1];
			U32	actCnt;  /* add for store num of active address, eigrp_enqueue_ip_address_command() */
			U32	neighbor;
			S32	sense;
		} pasv;
	} c;
} EigrpWork_st;

/* Return codes from EigrpIpSummaryRevise */
enum{
	EIGRP_SUM_NO_CHANGE,			/* Don't update summary */
	EIGRP_SUM_UPDATE_METRIC,		/* Update summary with new metric */
	EIGRP_SUM_FIND_METRIC		/* Search for new metric and update */
};

/* IP-EIGRP traffic statistics */
typedef struct EigrpIpTraffic_{
	U32	inputs;
	U32	outputs;
}EigrpIpTraffic_st;

/* ipeigrp hello multicast address */
#define	EIGRP_ADDR_HELLO	0xe000000a	/* 224.0.0.10 */

/* ipeigrp route event */
#define	EIGRP_ROUTE_UP		0
#define	EIGRP_ROUTE_DOWN		1
#define	EIGRP_ROUTE_MODIF		2

/* configure constant */
#define	EIGRP_MAX_DISTANCE			255
#define	EIGRP_BANDWIDTH_SCALE		0x98968000  /* 10000000*256 */
#define	EIGRP_IP_HDR_LEN				60
#define	EIGRP_DEF_MAX_ROUTES			3
#define	EIGRP_DEF_DISTANCE_INTERNAL	90
#define	EIGRP_DEF_DISTANCE_EXTERNAL	160
#define	EIGRP_DEF_VARIANCE			1
#define	EIGRP_K1_DEFAULT			1
#define	EIGRP_K2_DEFAULT			0
#define	EIGRP_K3_DEFAULT			1
#define	EIGRP_K4_DEFAULT			0
#define	EIGRP_K5_DEFAULT			0
#define	EIGRP_MAX_HOPS			100

/* ipeigrp route-type */
#define	EIGRP_RT_TYPE_INT			0
#define	EIGRP_RT_TYPE_EXT			1

#define	EIGRP_OFFSET_LIST_NO		0
#define	EIGRP_OFFSET_LIST_INTF	1
#define	EIGRP_OFFSET_LIST_GLOBAL	2

#define	EIGRP_WORK_QUE_LIMIT	 	20		/* Max 20 per pass */

EigrpVmetric_st *EigrpIpVmetricBuild(EigrpVmetric_st *);
void	EigrpIpVmetricDelete(EigrpVmetric_st *);
S32	EigrpIpGetNetsPerAddress(  struct EigrpNetEntry_  *, U32);
S32	EigrpIpGetNetsPerIdb(struct EigrpNetEntry_  *, struct EigrpIntf_ *);
S32	EigrpIpNetworkEntryDelete(struct EigrpNetEntry_  *, U32, U32);
void	EigrpIpNetworkListCleanup(struct EigrpNetEntry_  *);
struct EigrpNetEntry_ 	*EigrpIpNetworkEntryBuild(U32,	U32);
struct EigrpNetEntry_ 	*EigrpIpNetworkEntrySearch(struct EigrpNetEntry_  *, U32, U32);
S32	EigrpIpNetworkEntryInsert(struct EigrpNetEntry_  *,   U32, U32);
struct EigrpRedisEntry_	*EigrpIpRedisEntryInsert(struct EigrpRedisEntry_ **, struct EigrpRedis_*);
void	EigrpIpRedisEntryDelete(struct EigrpRedisEntry_ **, U16);
struct EigrpRedisEntry_	*EigrpIpRedisEntrySearch(struct EigrpRedisEntry_ *, U16);
struct EigrpRedisEntry_	*EigrpIpRedisEntrySearchWithProto(struct EigrpRedisEntry_ *, U32, U32);
void	EigrpIpRedisListCleanup(struct EigrpRedisEntry_ **);
#ifdef _EIGRP_PLAT_MODULE
void	EigrpIpSetRouterId(EigrpDualDdb_st *, U32, U32);
void	zebraEigrpIpSetRouterId(U32 , U32 );
#else
void	EigrpIpSetRouterId(EigrpDualDdb_st *);
#endif// _EIGRP_PLAT_MODULE
void	EigrpIpLaunchPdbJob(struct EigrpPdb_ *);
S32	EigrpIpEnqueueEvent(struct EigrpPdb_ *, EigrpWork_st *, U32);
S32	EigrpIpConnAddressActivated(struct EigrpPdb_ *, U32, U32);
U32	EigrpIpGetActiveAddrCount(struct EigrpPdb_ *, struct EigrpIntf_ *);
struct EigrpPdb_ *EigrpIpFindPdb(U32);
U32	EigrpIpGetOffset(EigrpDualDdb_st *, struct EigrpNetEntry_  *, EigrpIdb_st *, S8);
U32	EigrpIpBitsInMask(U32);
U32	EigrpIpBytesInMask(U32);
S8 *EigrpIpPrintNetwork(struct EigrpNetEntry_  *);
U32 EigrpIpSummaryRevise(EigrpDualDdb_st *, struct EigrpSum_ *, S32, U32);
void	EigrpIpEnqueueSummaryEntryEvent(struct EigrpPdb_ *, struct EigrpSum_ *, EigrpVmetric_st *, S32);
void	EigrpIpSummaryDepend(struct EigrpPdb_ *, U32, U32, EigrpVmetric_st *, U32, S32);
S32	EigrpIpRtUpdate(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *, S32 *);
S32	EigrpIpAddrMatch(U32 *, U32 *);
S8	*EigrpIpPrintAddr(U32 *);
S32	EigrpIpNetMatch(struct EigrpNetEntry_  *, struct EigrpNetEntry_  *);
U32	EigrpIpAddrCopy(U32 *, S8 *);
S32	EigrpIpLocalAddr(EigrpDualDdb_st *, U32 *, struct EigrpIntf_ *);
void	EigrpIpRtDelete(EigrpDualDdb_st *, EigrpDualNewRt_st*);
S32	EigrpIpPeerBucket(U32 *);
S32	EigrpIpNdbBucket(struct EigrpNetEntry_  *);
EigrpPktHdr_st *EigrpIpHeaderPtr(void *);
void	EigrpIpEnqueueConnRtchangeEvent(struct EigrpPdb_ *, struct EigrpIntf_ *, U32, U32,  S32, S32);
S32	EigrpIpIsExterior(U32 addr, U32 mask);
S32	EigrpIpAutoSummaryNeeded(struct EigrpPdb_ *, struct EigrpIntf_ *, U32, U32);
struct EigrpSumIdb_ *EigrpIpFindSumIdb(struct EigrpSum_ *, struct EigrpIntf_ *);
void	EigrpIpDeleteSummaryInterface(struct EigrpPdb_ *, struct EigrpSum_ *, struct EigrpSumIdb_ *);
void	EigrpIpConfigureSummaryEntry(struct EigrpPdb_ *, U32, U32, struct EigrpIntf_ *, S32, U8);
void	EigrpIpBuildAutoSummaries(struct EigrpPdb_ *, struct EigrpIntf_ *);
void	EigrpIpAdjustConnected(struct EigrpPdb_ *, struct EigrpIntf_ *, S32, S32);
void	EigrpIpEnqueueIfdownEvent(struct EigrpPdb_ *, struct EigrpIntf_ *, S32);
void	EigrpIpSetupMulticast(EigrpDualDdb_st *, struct EigrpIntf_ *, S32);
void	EigrpIpSetMtu (EigrpIdb_st *);
void	EigrpIpOnoffIdb(EigrpDualDdb_st *, struct EigrpIntf_ *, S32, S32);
void	EigrpIpEnqueueConnAddressEvent(struct EigrpPdb_ *, U32, U32, S32);
S32	EigrpIpRouteOnPdbOrNdb(void *, struct EigrpPdb_ *);
void	EigrpIpRedistConnState(struct EigrpPdb_ *, struct EigrpIntf_ *, U32, U32,  S32);
void	EigrpIpConnSummaryDepend(EigrpDualDdb_st *, struct EigrpIntf_ *, U32, U32, S32);
void	EigrpIpEnqueueRedisEntryEvent(struct EigrpPdb_ *, U16, S32);
void	EigrpIpProcessNetMetricCommand(S32, U32, void *);
void	EigrpIpProcessRedisCommand(S32, struct EigrpPdb_ *, struct EigrpRedis_ *);
void	EigrpIpProcessDefaultRouteCommand(S32, struct EigrpPdb_ *, S32);
void	EigrpIpActivateConnAddress_int(struct EigrpPdb_ *, struct EigrpIntf_ *, U32, U32,  S32, S8 *, U32);
#define EigrpIpActivateConnAddress(pdb_m, pEigrpIntf_m, address_m, mask_m,  sense_m)\
			EigrpIpActivateConnAddress_int(pdb_m, pEigrpIntf_m, address_m, mask_m,  sense_m, __FILE__, __LINE__)
void	EigrpIpProcessNetworkCommand(S32, struct EigrpPdb_ *, U32, U32);
void	EigrpIpProcessNeighborCommand(S32, struct EigrpPdb_ *, U32);
S32	EigrpIpEnqueueZapIfEvent(struct EigrpPdb_ *, struct EigrpIntf_ *);
void	EigrpIpProcessZapIfEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpIpAddressChange_int(struct EigrpIntf_ *, U32, U32, S32, S8 *, U32);
#define EigrpIpIpAddressChange(if_m, ipAddr_m, ipMask_m, adding_m)\
			EigrpIpIpAddressChange_int(if_m, ipAddr_m, ipMask_m, adding_m, __FILE__, __LINE__)
void	EigrpIpEnqueueIfAddEvent(struct EigrpPdb_ *, struct EigrpIntf_ *);
void	EigrpIpEnqueueIfDeleteEvent(struct EigrpPdb_ *, S32);
void	EigrpIpEnqueueIfUpEvent(struct EigrpPdb_ *, void *);
void	EigrpIpEnqueueIfDownEvent(struct EigrpPdb_ *, S32);
void	EigrpIpEnqueueIfAddressAddEvent(struct EigrpPdb_ *, S32, U32, U32, U8);
void	EigrpIpEnqueueIfAddressDeleteEvent(struct EigrpPdb_ *, S32, U32, U32);
void	EigrpIpProcessIfAddEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessIfDeleteEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessIfUpEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessIfAddressDeleteEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessReloadIptableEvent(struct EigrpPdb_ *);
void	EigrpIpEnqueueReloadIptableEvent(struct EigrpPdb_ *, void *);
void	EigrpIpProcessExteriorChange(struct EigrpPdb_ *, EigrpWork_st *);
S32	EigrpIpGetProtId(U32);
EigrpExtData_st *EigrpIpBuildExternalInfo(struct EigrpPdb_ *, EigrpDualDdb_st *, struct EigrpPdb_ *, EigrpDualNewRt_st *);
S32	EigrpIpPickupRedistMetric(struct EigrpPdb_ *pdb, EigrpDualNewRt_st *, U32 *, EigrpVmetric_st *);
void	EigrpIpRedistributeRoutesAllCleanup(struct EigrpPdb_ *);
void	EigrpIpRtchange_int(struct EigrpPdb_ *, EigrpDualNewRt_st *, S32, S8 *, U32);
#define EigrpIpRtchange(pdb_m, newRt_m, e_m)	EigrpIpRtchange_int(pdb_m, newRt_m, e_m, __FILE__, __LINE__)
S32	EigrpIpRedistributeRoutesCleanup(U16, S32, U32);
void	EigrpIpProcessRedisEntryEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessConnAddressEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessBackup(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessLost(struct EigrpPdb_ *, EigrpWork_st *);
S32	EigrpIpGetSummaryMetric(EigrpDualDdb_st *, struct EigrpSum_ *, EigrpDualNewRt_st *);
void	EigrpIpProcessIfdownEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessSummaryEntryEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessPassiveIntfCommand(S32, struct EigrpPdb_ *, struct EigrpIntf_ *);
void	EigrpIpProcessInvisibleIntfCommand(S32, struct EigrpPdb_ *, struct EigrpIntf_ *);
void	EigrpIpProcessConnChangeEvent(struct EigrpPdb_ *, EigrpWork_st *);
void	EigrpIpProcessDistanceCommand(struct EigrpPdb_ *, U32, U32);
void	EigrpIpProcessDefaultmetricCommand(S32, struct EigrpPdb_ *, struct EigrpVmetric_ *);
void	EigrpIpProcessMetricweightCommand(struct EigrpPdb_ *);
void	EigrpIpProcessIfSetHelloIntervalCommand(S32, struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSetHoldtimeCommand(S32, struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSetSplitHorizonCommand(S32, struct EigrpPdb_ *, U8 *);
void	EigrpIpProcessIfSetBandwidthPercentCommand(struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSetBandwidthCommand(S32, struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSetDelayCommand(S32, struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSetBandwidth(struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSetDelay(struct EigrpPdb_ *, U8 *, U32);
void	EigrpIpProcessIfSummaryAddressCommand(S32, struct EigrpPdb_ *, U8 *, U32, U32);
#ifdef _EIGRP_UNNUMBERED_SUPPORT
void	EigrpIpProcessIfUaiP2mp(S32, U8 *, U32);
void	EigrpIpProcessIfUaiP2mp_Nei(S32, U32, U32);
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
void	EigrpIpProcessIfSetAuthenticationKey(struct EigrpPdb_ *, U8 *, U8 *);
void	EigrpIpProcessIfSetAuthenticationMode(S32, struct EigrpPdb_ *, U8 *);
void eigrp_enqueue_if_authentication_keyid_command(EigrpDualDdb_st *, S8 *, U32);
void	EigrpIpProcessIfSetAuthenticationKeyidCommand(struct EigrpPdb_ *, U8 *, U32);
S32	EigrpIpShouldAcceptPeer(EigrpDualDdb_st *, U32 *,  EigrpIdb_st *);
#if	(EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_12)
S32	EigrpIpIsCfgSummary(struct EigrpSum_ *);
S32	EigrpIpAutoSummaryAdvertise(struct EigrpPdb_ *, struct EigrpSum_ *);
#endif/* (EIGRP_SUMMARY_RULE_TYPE == EIGRP_SUMMARY_RULE_VER_12) */
U32 EigrpIpAdvertiseRequest(EigrpDualDdb_st *, EigrpDualNdb_st *, struct EigrpIntf_ *);
void	EigrpIpShowSummaryEntry(struct EigrpSum_ *, S32 *, U8 , S8 *);
void	EigrpIpShowAutoSummary(struct EigrpPdb_ *);
void	EigrpIpShowSummary(struct EigrpPdb_ *);
void	EigrpIpProcessAutoSummaryCommand(struct EigrpPdb_ *, S32);
EigrpDualDdb_st *EigrpIpFindDdb(U32);
S8	EigrpIpExteriorBit(EigrpDualRdb_st *);
S8	EigrpIpExteriorFlag(EigrpDualNdb_st *);
S32	EigrpIpExteriorDiffer(EigrpDualNdb_st *, S8 *);
void	EigrpIpExteriorCheck(EigrpDualDdb_st *, EigrpDualNdb_st *, S8 *);
S32	EigrpIpCompareExternal(void *, void *);
S32	EigrpIpPeerIsFlowReady(EigrpDualPeer_st *);
void	EigrpIpTransportReady(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *, S32);
U32	EigrpIpItemSize(EigrpDualNdb_st *);
void	EigrpIpPdmTimerExpiry(EigrpDualDdb_st *, EigrpMgdTimer_st *);
S32	EigrpIpDdbInit(struct EigrpPdb_ *);
void	EigrpIpStartAclTimer(struct EigrpPdb_ *, void *);
void	EigrpIpHopcountChanged(struct EigrpPdb_ *, U8);
struct EigrpPdb_ *EigrpIpPdbInit(U32);
void	EigrpIpSummarylistCleanup(EigrpQue_st *);
void	EigrpIpIfapDown(struct EigrpPdb_ *);
void	EigrpIpCleanup(struct EigrpPdb_ *);
void	EigrpIpProcessMaxActiveTimeCommand(struct EigrpPdb_ *, U32, S32);
void	EigrpIpResetCommand(struct EigrpPdb_ *);
U8	EigrpIpInetMask(U32);
U8	EigrpIpInetFlags(U32) ;
S32	EigrpIpProcessWorkq(struct EigrpPdb_ *);
void	EigrpIpWorkqFlush(struct EigrpPdb_ *);

#ifdef	__cplusplus
}
#endif	/* end of __cplusplus */

#endif /* _EIGRP_IP_H */
