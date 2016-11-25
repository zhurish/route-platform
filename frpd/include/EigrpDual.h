#ifndef _EIGRP_DUAL_H
#define _EIGRP_DUAL_H

#ifdef __cplusplus
extern"C"
{
#endif

struct EigrpParamTlv_;
struct EigrpIntf_;

/***add by cwf 20130109  备用未知(update不更新本地非用户接口IP地址)*****/
#define LOCAL_WIREDNET_SUPRESS 		0
/***end add by cwf 20130109  备用未知(update不更新本地非用户接口IP地址)*****/

#define	EIGRP_DUAL_LOG_ALL(ddb, event, d1, d2)		EigrpDualLogAll(ddb, event, &(d1), &(d2))

#define	EIGRP_ACL_CHANGE_DELAY			(10*EIGRP_MSEC_PER_SEC)    /* Delay before tearing down nbrs */
#define	EIGRP_MIN_PKT_DESC				20000		/* org:5000 */  /* Malloc in blocks of 50 */
#define	EIGRP_MIN_PKT_DESC_QELM			40000		/* org:10000 */	   /* Malloc in blocks of 200 */
#define	EIGRP_DUAL_DEF_MAX_EVENTS		500			/* Default event buffer size */
#define	EIGRP_DUAL_EXT_DATA_CHUNK_SIZE	60000		/* 100000 */  /* 5000 */	/* Extdata chunk block size */ 
#define	EIGRP_DUAL_WORKQ_CHUNK_SIZE	10000		/* 1000 */	/* Workq chunk block size */
#define	EIGRP_PACKETIZATION_DELAY		5000		/* Milliseconds */

#define	EIGRP_MSEC_PER_SEC				1000		/* 1000 ms/s */
#define	EIGRP_MSEC_PER_MIN				60000		/* 60000 ms/min */
#define	EIGRP_ACTIVE_SCAN_FREQUENCY		(EIGRP_MSEC_PER_MIN)


#define	EIGRP_METRIC_IGRP_SHIFT			8		/* Convert IGRP to EIGRP metrics */

/* Maximum time to delay sending a hello after sending an update with init bit set. */

#define	EIGRP_INIT_HELLO_DELAY	(2 * EIGRP_MSEC_PER_SEC)
#define	EIGRP_JITTER_PCT			15	/* Jitter timers by 15 percent */

/* Default setting for EIGRP route holdDown */
#define	EIGRP_MAX_PPS				100		/* Max PPS of reliable pkts per i/f */
#define	EIGRP_MIN_PACING_IVL		(EIGRP_MSEC_PER_SEC/EIGRP_MAX_PPS) /* Min pacing interval */

#define	EIGRP_HEADER_BYTES		sizeof(EigrpPktHdr_st)
#define	EIGRP_MAX_BYTES			1500
#if(EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_COMMERCIAL)
#define	EIGRP_HELLO_HDR_BYTES	(sizeof(EigrpParamTlv_st) + sizeof(EigrpVerTlv_st))
#elif (EIGRP_PRODUCT_TYPE == EIGRP_PRODUCT_TYPE_YEZONG)
#define	EIGRP_HELLO_HDR_BYTES	(sizeof(EigrpParamTlv_st))
#endif
/* flag field definitions */
#define	EIGRP_INFO_EXT			0x01   /* Consider this from an external source */
#define	EIGRP_INFO_CD			0x02   /* Candidate default route */

/* Values for circuit pacing */
#define	EIGRP_NO_UNRELIABLE_DELAY_BW 1000 /* Kbps, above this no ack pacing */

/* EIGRP_OPC_UPDATE and EIGRP_OPC_REQUEST are defined as 1 and 2 respectively in igrp.h */
#define	EIGRP_OPC_UPDATE		1
#define	EIGRP_OPC_REQUEST		2
#define	EIGRP_OPC_QUERY		3
#define	EIGRP_OPC_REPLY		4
#define	EIGRP_OPC_HELLO		5
#define	EIGRP_OPC_IPXSAP		6
#define	EIGRP_OPC_PROBE		7		/* For test purposes */
#define	EIGRP_OPC_ACK			8		/* Really a kludge for debugging */
#define	EIGRP_OPC_TOTAL		9		/* 0 isn't used... */
#define	EIGRP_OPC_ILLEGAL		0xffff

/* TLV type definitions.  Generic(protocol-independent) TLV types are defined here. 
  * Protocol-specific ones are defined elsewhere.  IP TLVs are in the 0x0100 range, ATALK in 
  * 0x0200, and IPX in 0x0300. */
#define	EIGRP_PARA				0x0001
#define	EIGRP_AUTH				0x0002
#define	EIGRP_SEQUENCE		0x0003
#define	EIGRP_SW_VERSION		0x0004
#define	EIGRP_NEXT_MCAST_SEQ	0x0005

/* value for vector metric of composite metric */
#define	EIGRP_METRIC_INACCESS			0xffffffff
#define	EIGRP_METRIC_INACCESS_HALF	0x80000000

/* Magic macros for handling the reply status handle arrays.  They're bit arrays, one per peer. */
#define	EIGRP_HANDLE_MALLOC_SIZE(array_size)	((array_size) * sizeof(U32))
#define	EIGRP_BITSIZE_ULONG					(sizeof(U32) * 8)
#define	EIGRP_HANDLE_TO_CELL(h)				((h) / EIGRP_BITSIZE_ULONG)
#define	EIGRP_CELL_TO_HANDLE(c)				((c) * EIGRP_BITSIZE_ULONG)
#define	EIGRP_HANDLE_TO_BIT(h)				(1 << ((h) % EIGRP_BITSIZE_ULONG))
#define	EIGRP_ULONG_HANDLE(f, h)				(f[EIGRP_HANDLE_TO_CELL(h)])
#define	EIGRP_CLEAR_HANDLE(f, h)				(EIGRP_ULONG_HANDLE((f), (h)) &= ~(EIGRP_HANDLE_TO_BIT(h)))
#define	EIGRP_SET_HANDLE(f, h)					(EIGRP_ULONG_HANDLE((f), (h)) |= EIGRP_HANDLE_TO_BIT(h))
#define	EIGRP_TEST_HANDLE(f, h)				(EIGRP_ULONG_HANDLE((f), (h)) & EIGRP_HANDLE_TO_BIT(h))

#define	EIGRP_DUAL_EVENTS_BUF_SIZE	24
#define	EIGRP_MAX_ROUTE				0x100000

/* Values for retransmission control. */
#define	EIGRP_INIT_RTO					(2 * EIGRP_MSEC_PER_SEC)	/* Initial RTO value */
#define	EIGRP_RTO_LOWER				200		/* Absolute minimum RTO */
#define	EIGRP_RTO_UPPER				(5*EIGRP_MSEC_PER_SEC)	/* Beyond this it's silly */
#define	EIGRP_RTO_SRTT_MULTIPLIER	6	/* Multiply by SRTT to get RTO */
#define	EIGRP_VERSION					2
#define	EIGRP_RETRY_LIMIT				16
#define	EIGRP_RTO_PACING_MULTIPLIER	6	/* Bound RTO by N pacing ivls */
#define	EIGRP_FLOW_SRTT_MULTIPLIER	4	/* Mcast flow timer = N *avg SRTT */
#define	EIGRP_MIN_MCAST_FLOW_DELAY	50	/* Minimum multicast flow timer */
#define	EIGRP_MAX_PPS					100		/* Max PPS of reliable pkts per i/f */
#define	EIGRP_MIN_PACING_IVL			(EIGRP_MSEC_PER_SEC/EIGRP_MAX_PPS) /* Min pacing interval */

/* Kludge space for AT-EIGRP cablerange TLV, pending reworking of how the hello tlvs are
  * constructed. */
#define	EIGRP_HELLO_HOSUCCESS_BYTE_KLUDGE	(sizeof(EigrpGenTlv_st) + 2*sizeof(U32))
#define	EIGRP_K_PARA_LEN				5			/* Length of k value list in packets */

/* Count of number of unicasts that may be sent on an interface before a multicast is required.
  * This keeps the unicast packets from locking out the multicasts. */
#define	EIGRP_MAX_UNICAST_PKT		5

/* Count of number of unreliable packets that may be sent to a peer before a reliable packet is
  * required.  This keeps the unreliable packets from locking out the reliable ones. */
#define EIGRP_MAX_UNRELIABLE_PKT		5

#define	EIGRP_PKT_TL \
		U16 type;        /* Type of data following */             \
		U16 length;      /* Length in bytes of data following */

#define	EIGRP_PKT_METRIC \
		S8 delay[4];    /* delay, in ??? of microseconds */      \
		S8 bandwidth[4];    /* bandwidth, in units of 1 Kbit/sec */  \
		S8 mtu[3];  /* MTU, in octets */                     \
		S8 hopcount;     /* Number of hops to destination */      \
		S8 reliability; /* percent packets successfully tx/rx */ \
		S8 load;        /* percent of channel occupied */        \
		S8 mreserved[2]; /* fill out to long word boundary */

/* Transmit queue types.  There is a separate transmit queue for reliable and unreliable packets,
  * both at the peer level and at the interface level.
  *
  * Please don't change the order of the constants below;  some code assumes that the first queue 
  * is the unreliable one. */
enum{
	EIGRP_UNRELIABLE_QUEUE = 0, 
	EIGRP_RELIABLE_QUEUE, 
	EIGRP_QUEUE_TYPES
};

/* Work queue types.  This is used to tag work elements on the work queue for processing
  * redistributed, lost, and backup entries. */
enum {
	EIGRP_WORK_IFC_ADD_EVENT= 0,
	EIGRP_WORK_IFC_DELETE_EVENT,
	EIGRP_WORK_IFC_UP_EVENT,
	EIGRP_WORK_IFC_DOWN_EVENT,
	EIGRP_WORK_IFC_ADDRESS_ADD_EVENT,
	EIGRP_WORK_IFC_ADDRESS_DELETE_EVENT,
	EIGRP_WORK_ZAP_IF_EVENT,
	EIGRP_WORK_REDIST_CONNECT,
	EIGRP_WORK_IF_DOWN_EVENT,
	EIGRP_WORK_LOST,
	EIGRP_WORK_BACKUP,
	EIGRP_WORK_CONNSTATE,
	EIGRP_WORK_REDISTALL,
	EIGRP_WORK_REDIS_DELETE,
	EIGRP_WORK_SUMMARY,			/* ip needs this */
	EIGRP_WORK_PASSIVEINTF,		/* as well as this */
	EIGRP_WORK_UNREDIST,			/* appletalk needs this */
	EIGRP_WORK_SENDSAP,			/* ipx needs this */
	EIGRP_WORK_TABLERELOAD,
	EIGRP_WORK_INTEXTSTATE,
	EIGRP_WORK_WATER_MARK,
	EIGRP_WORK_NETWORK_CMD,
	EIGRP_WORK_AUTOSUM_CMD,
	EIGRP_WORK_SUMMARY_CMD,
	EIGRP_WORK_IPADDR_CMD,
	EIGRP_WORK_SETHELLO_CMD,
	EIGRP_WORK_SETHOLDTIME_CMD,
	EIGRP_WORK_SETSPLIT_CMD,
	EIGRP_WORK_SETBANDPERCENT_CMD,
	EIGRP_WORK_SETAUTHKEY_CMD,
	EIGRP_WORK_SETAUTHKEYID_CMD,
	EIGRP_WORK_SETAUTHMODE_CMD,
	EIGRP_WORK_OFFSETIN_CMD,
	EIGRP_WORK_OFFSETOUT_CMD,
	EIGRP_WORK_PASSIVEINTF_CMD,
	EIGRP_WORK_DISTANCE_CMD,
	EIGRP_WORK_REDIS_CMD,
	EIGRP_WORK_DEFMETRIC_CMD,
	EIGRP_WORK_ACL_CMD,
	EIGRP_WORK_KVALUES_CMD,
	EIGRP_WORK_DEFINFO_CMD,
	EIGRP_WORK_SHOW_INTERFACE_CMD,
	EIGRP_WORK_SHOW_NEIGHBOR_CMD,
	EIGRP_WORK_SHOW_TRAFFIC_CMD,
	EIGRP_WORK_SHOW_PROTOCOL_CMD,
	EIGRP_WORK_SHOW_ROUTE_CMD,
	EIGRP_WORK_SHOW_STRUCT_CMD,
	EIGRP_WORK_SHOW_TOPOLOGY_CMD,
	EIGRP_WORK_NEIGHBOR_CMD
};

/* Managed timer types */
enum {
	EIGRP_IIDB_PACING_TIMER, 
	EIGRP_PEER_SEND_TIMER, 
	EIGRP_IIDB_HELLO_TIMER,
	EIGRP_PEER_HOLDING_TIMER, 
	EIGRP_FLOW_CONTROL_TIMER, 
	EIGRP_ACTIVE_TIMER,
	EIGRP_PACKETIZE_TIMER, 
	EIGRP_PDM_TIMER, 
	EIGRP_IIDB_ACL_CHANGE_TIMER,
	EIGRP_DDB_ACL_CHANGE_TIMER
};

enum {
	EIGRP_DUALEV_GENERIC = 1,
	EIGRP_DUALEV_UPDATE_SEND,
	EIGRP_DUALEV_UPDATE_SEND2,
	EIGRP_DUALEV_STATE,
	EIGRP_DUALEV_CLEARHANDLE1,
	EIGRP_DUALEV_CLEARHANDLE2,
	EIGRP_DUALEV_REPLYFREE,
	EIGRP_DUALEV_SENDREPLY,
	EIGRP_DUALEV_FPROMOTE,
	EIGRP_DUALEV_RTINSTALL,
	EIGRP_DUALEV_RTDELETE,
	EIGRP_DUALEV_NDBDELETE,
	EIGRP_DUALEV_RDBDELETE,
	EIGRP_DUALEV_ACTIVE,
	EIGRP_DUALEV_NOTACTIVE,
	EIGRP_DUALEV_SETREPLY,
	EIGRP_DUALEV_SPLIT,
	EIGRP_DUALEV_SEARCHFS,
	EIGRP_DUALEV_FCSAT1,
	EIGRP_DUALEV_FCSAT2,
	EIGRP_DUALEV_FCNOT,
	EIGRP_DUALEV_RCVUP1,
	EIGRP_DUALEV_RCVUP2,
	EIGRP_DUALEV_RCVUP3,
	EIGRP_DUALEV_RCVQ1,
	EIGRP_DUALEV_RCVQ2,
	EIGRP_DUALEV_RCVR1,
	EIGRP_DUALEV_RCVR2,
	EIGRP_DUALEV_PEERDOWN1,
	EIGRP_DUALEV_PEERDOWN2,
	EIGRP_DUALEV_PEERUP,
	EIGRP_DUALEV_IFDEL1,
	EIGRP_DUALEV_IFDEL2,
	EIGRP_DUALEV_IFDEL3,
	EIGRP_DUALEV_LOST,
	EIGRP_DUALEV_CONNDOWN,
	EIGRP_DUALEV_CONNCHANGE,
	EIGRP_DUALEV_REDIST1,
	EIGRP_DUALEV_REDIST2,
	EIGRP_DUALEV_SIA,
	EIGRP_DUALEV_SIA2,
	EIGRP_DUALEV_SIA3,
	EIGRP_DUALEV_SIA4,
	EIGRP_DUALEV_SIA5,
	EIGRP_DUALEV_NORESP,
	EIGRP_DUALEV_MCASTRDY,
	EIGRP_DUALEV_INITSTUCK,
	EIGRP_DUALEV_NOINIT,
	EIGRP_DUALEV_HOLDTIMEEXP,
	EIGRP_DUALEV_ACKSUPPR,
	EIGRP_DUALEV_UCASTSENT,
	EIGRP_DUALEV_MCASTSENT,
	EIGRP_DUALEV_PKTSENT2,
	EIGRP_DUALEV_PKTSUPPR,
	EIGRP_DUALEV_PKTNOSEND,
	EIGRP_DUALEV_PKTRCV,
	EIGRP_DUALEV_PKTRCV2,
	EIGRP_DUALEV_PKTRCV3,
	EIGRP_DUALEV_BADSEQ,
	EIGRP_DUALEV_REPLYACK,
	EIGRP_DUALEV_REPLYPACK,
	EIGRP_DUALEV_REPLYENQ,
	EIGRP_DUALEV_REPLYXMIT,
	EIGRP_DUALEV_REPLYXMIT2,
	EIGRP_DUALEV_UPDATEACK,
	EIGRP_DUALEV_UPDATEPACK,
	EIGRP_DUALEV_UPDATEENQ,
	EIGRP_DUALEV_UPDATEXMIT,
	EIGRP_DUALEV_UPDATEXMIT2,
	EIGRP_DUALEV_QUERYACK,
	EIGRP_DUALEV_QUERYPACK,
	EIGRP_DUALEV_QUERYENQ,
	EIGRP_DUALEV_QUERYXMIT,
	EIGRP_DUALEV_QUERYXMIT2,
	EIGRP_DUALEV_STARTUPACK,
	EIGRP_DUALEV_STARTUPPACK,
	EIGRP_DUALEV_STARTUPENQ,
	EIGRP_DUALEV_STARTUPENQ2,
	EIGRP_DUALEV_STARTUPXMIT,
	EIGRP_DUALEV_STARTUPXMIT2,
	EIGRP_DUALEV_LASTPEER,
	EIGRP_DUALEV_FIRSTPEER,
	EIGRP_DUALEV_QSHON,
	EIGRP_DUALEV_QSHOFF,
	EIGRP_DUALEV_SETRD,
	EIGRP_DUALEV_OBE,
	EIGRP_DUALEV_UPDATESQUASH,
	EIGRP_DUALEV_EMPTYCHANGEQ,
	EIGRP_DUALEV_REQUEUEUCAST,
	EIGRP_DUALEV_XMITTIME,
	EIGRP_DUALEV_SUMREV,
	EIGRP_DUALEV_SUMREV2,
	EIGRP_DUALEV_SUMREV3,
	EIGRP_DUALEV_SUMDEP,
	EIGRP_DUALEV_GETSUM,
	EIGRP_DUALEV_GETSUM2,
	EIGRP_DUALEV_PROCSUM,
	EIGRP_DUALEV_PROCSUM2,
	EIGRP_DUALEV_IGNORE,
	EIGRP_DUALEV_UNKNOWN,
	EIGRP_DUALEV_COUNT			/* Number of event types */
};

/* Query-origin states.  Used in paramater block and flags in topology table. */
enum {
	EIGRP_DUAL_QOCLEAR= 0,	/* Don't be confused; this isn't passive */
	EIGRP_DUAL_QOLOCAL,		/* We are the origin of this query */
	EIGRP_DUAL_QOMULTI,		/* Multiple computations for this dest */
	EIGRP_DUAL_QOSUCCS,		/* Successor is origin of this query */
	EIGRP_DUAL_QOCOUNT		/* Number of entries */
};

/* Topology table structures. */
enum {							/* Routing information origins */
	EIGRP_ORG_EIGRP = 0,		/* eigrp destination */
	EIGRP_ORG_CONNECTED,		/* connected destination */
	EIGRP_ORG_REDISTRIBUTED,	/* redistributed destination */
	EIGRP_ORG_RSTATIC,	/* redistribute static destination */
	EIGRP_ORG_RCONNECT,	/* redistribute Conn destination */
	EIGRP_ORG_SUMMARY,		/* summary destination */
	EIGRP_ORG_COUNT			/* summary destination */
};

/* Internal event indentifiers */
enum {
	EIGRP_DUAL_RTUP,			/* New route came up */
	EIGRP_DUAL_RTDOWN,		/* Old route disappeared */
	EIGRP_DUAL_RTCHANGE,		/* Existing route changed metric */
	EIGRP_DUAL_RTEVCOUNT		/* Number of event types */
};

enum {
	EIGRP_DUAL_NONE,
	EIGRP_DUAL_NUM,
	EIGRP_DUAL_HEX_NUM,
	EIGRP_DUAL_NET,
	EIGRP_DUAL_ADDR,
	EIGRP_DUAL_STRING,
	EIGRP_DUAL_SWIF,
	EIGRP_DUAL_BOOL,
	EIGRP_DUAL_RTORIGIN,
	EIGRP_DUAL_QUERY_ORIGIN,
	EIGRP_DUAL_RTEVENT,
	EIGRP_DUAL_PKT_TYPE,
	EIGRP_DUAL_ETYPE_COUNT			/* Number of types */
};

/* Event log structure.  These are built within a circular buffer hung from the DDB.  The buffer
  * size is configurable. */
typedef struct EigrpDualEvent_{
	U32	code;               /* Event Code */
	S8	buf[EIGRP_DUAL_EVENTS_BUF_SIZE]; /* This should really be a union of */
	                                /* all the different combinations */
	                                /* of interesting data used */
}EigrpDualEvent_st;

/* When we want to display the event log, we want to copy it atomically so that it will maintain
  * consistency for the arbitrarily long time it may take to format it.  Rather than copying it
  * into one giant block(which we may have trouble obtaining), we copy it into a linked list of
  * smaller blocks. */
#define	EIGRP_DUAL_SHOW_EVENT_FRAG_SIZE		65536 /* Target block size, give or take */
#define	EIGRP_DUAL_SHOW_EVENT_ENTRY_COUNT	(EIGRP_DUAL_SHOW_EVENT_FRAG_SIZE / sizeof(EigrpDualEvent_st))
typedef struct EigrpDualShowEventBlock_{
	struct EigrpDualShowEventBlock_ *next_block; /* List pointer */
	EigrpDualEvent_st	event_entry[EIGRP_DUAL_SHOW_EVENT_ENTRY_COUNT]; /* The events */
}EigrpDualShowEventBlock_st;

typedef struct EigrpDualShowEventDecode_{
	S8	*message;
	U32	data1, data2;
}EigrpDualShowEventDecode_st;

typedef struct EigrpSwVer_{
	S8	majVer;	
	S8	minVer;
	
	S8	eigrpMajVer;	/* EIGRP major revision */
	S8	eigrpMinVer;		/* EIGRP minor revision */
} EigrpSwVer_st, *EigrpSwVer_pt;

/* Anchor entry.  This data structure is used for anchoring DNDBs or DRDBs on the transmit thread.
  * This is done as a separate structure so that the DNDB can be moved while anchored(if it is,
  * a dummy xmit thread entry will be put in its place).  We need to keep the real DNDB/DRDB in its
  * new place, since it is linked to other things in a spectacular fashion. */
typedef struct EigrpXmitAnchor_{
	S32	count;		/* Count of anchors */
	struct EigrpXmitThread_ *thread; /* Anchored entry */
}EigrpXmitAnchor_st;

#define	EIGRP_MIN_ANCHOR	20000     /* org:5000*/	/* Block size for EigrpUtilChunkMalloc */
#define	EIGRP_CR_FLAG		0x02

/* Thread entry structure.  This structure is used to thread DNDBs and DRDBs together to form the
  * threads used for maintaining the state of which information is to be transmitted.
  *
  * DNDBs and DRDBs are threaded together in a single thread that hangs from the DDB.  The entries
  * are in the order in which events took place to cause the information to be transmitted.  Each
  * entry has a serial number which is always in ascending order(circularly) on the thread.  Each
  * interface has a state machine which walks down the thread.  Groups of DNDBs and DRDBs which are
  * consecutive and can be carried in the same packet bundled together into packets and transmitted.
  * The packetization is triggered by flow-control-ready events coming from the transport.
  *
  * DNDBs stay permanently threaded so that new peers can receive a full dump of the topology table;
  * DRDBs are only threaded until the reply packet that was generated from them is acknowledged.
  *
  * An entry can be "anchored" by a number of things(IIDBs, packet descriptors, *etc.)  This means
  * that a pointer is being held to this entry, so it cannot be freed or moved. If the entry
  * containing this thread element changes and must be moved, it is copied first to a dummy entry.
  * This keeps the entry intact(with its serial number) in the same place in the thread, and a new 
  * entry is created and linked on the end.
  *
  * If this entry is anchored, it will have a pointer to an anchor entry. */
typedef struct EigrpXmitThread_{
	struct EigrpXmitThread_ *next;		/* Next guy in chain */
	struct EigrpXmitThread_ **prev;	/* Prev guy's next ptr(or head) */
	
	U32	serNo;						/* Serial number of this entry */
	S32	refCnt;						/* Number of active senders */
	EigrpXmitAnchor_st *anchor;		/* Pointer to anchor, or NULL */
	S8	dummy:1;					/* TRUE if this is a dummy entry */
	S8	drdb:1;						/* TRUE if this is in a DRDB */
}EigrpXmitThread_st;

/* Unpacked(non packet) version of above */
typedef struct EigrpVmetric_{
	U32	delay;
	U32	bandwidth;
	U32	mtu;
	U32	hopcount;
	U32	reliability;
	U32	load;
}EigrpVmetric_st;

typedef	struct	EigrpNetMetric_{
	struct	EigrpNetMetric_		*forw;
	struct	EigrpNetMetric_		*back;
	
	U32		ipNet;
	U32		metric;
	EigrpVmetric_st *vecmetric;
}EigrpNetMetric_st, *EigrpNetMetric_pt;

typedef struct EigrpNetEntry_{		/* List for some use*/
	struct EigrpNetEntry_ *next;
	struct EigrpNetEntry_ *back;
	
	U32 address;
	U32 mask;
}EigrpNetEntry_st;

#define	EIGRP_SOCK_LIST(list, entry)			{ for(entry = (list)->next; entry != list; entry = entry->next)
#define	EIGRP_SOCK_LIST_END(list, entry)	if(entry == list) entry = (EigrpNetEntry_st *) 0; }

/* rdbtype -- route descriptor blocks.
  *
  * In DUAL, there can be multiple routes to a destination network. The routing, (sometimes called
  * "path") information is stored in these route descriptor blocks.
  *
  * Routes can be introduced into the DUAL topology database via DUAL (which are native routes) or
  * by route redistribution from other routing protocols; such routes are called "external" routes
  * and are tagged as such at the point in the DUAL topology where they are first introduced into
  * the topology table.
  *
  * Externally derived routes may carry external routing information which is specific to the
  * routing protocol which is the origin of the route. A pointer to this is kept off the side of
  * the rdb in the 'data' field. */
typedef struct EigrpDualRdb_{
	struct EigrpDualRdb_ *next;	/* Pointer to next rdb */
	struct EigrpDualNdb_ *dndb;	/* Pointer back to owning DNDB */
	
	EigrpXmitThread_st thread;		/* Linkage for reply transmission */
	U32 		nextHop;			/* Possible successor */
	S32 		handle;				/* Handle of advertising peer */
	U32 		infoSrc;				/* Who, if anyone, told us */
	struct EigrpIdb_ *iidb;			/* Interface this info came in on */
	
	U32 		succMetric;			/* Metric(neighbor's view) */		/* NBR REPORTED RD TIGER	*/
	U32 		metric;				/* Composite metric(our view) */	/* WE CALCULATED METRIC 	*/
	EigrpVmetric_st 	vecMetric;	/* Vectorized metric */

	U32		origin;				/* Origin of this information */
	U16		sndFlag;			/* Send update, query, or reply */
	S8		opaqueFlag;			/* Flags maintained by PDM */
	
	S32		flagKeep:1;			/* Keep rdist dest during tbl reload */
	S32		flagExt:1;			/* Is this externally derived dest? */
	S32		flagIgnore:1;		/* Used for inbound filtering */
	S32		flagDel:1;			/* DRDB has been freed */
	S32		flagIidbDel:1;		/* IIDB has been deleted */

	struct	EigrpIntf_	*intf;	/* save ifap(idb) when iidb is delete */
	void		*extData;			/* Pointer to external info */
	U16		rtType;

	U32		originRouter;			/*zhenxl_20130116 Origin router ID of this route.*/
}EigrpDualRdb_st;

#define	EIGRP_NO_PEER_HANDLE 	-1		/* Handle for nonexistent peer */

/* Handle structure.  This is effectively a bitmap, one for each peer. One of these exists in the
  * DDB, and one in each IIDB as well.  This makes it efficient to set the reply status table in
  * the face of split horizon, etc. */
typedef struct EigrpHandle_{
	U16	used;			/* Number of bits set */
	U16	arraySize;		/* Size of handle array in words */
	U32	*array;			/* Allocated array of handles */
}EigrpHandle_st;

/* ndbtype	-- network descriptor block.
  *
  * Used to decribe a network in the the topology table. In DUAL nomenclature, a 'network' is a
  * destination to the routing protocol.
  *
  * Networks may have more than one route by which we can reach that network. These are kept off
  * the ndb in the rdb chain; the first rdb in the rdb chain is the "first next hop" (called
  * 'successor' in the DUAL nomenclature), with less optimal routes being next in the rdb chain. */
typedef struct EigrpDualNdb_{
	struct EigrpDualNdb_ *next;		/* Pointer to next ndb in has thread */
	struct EigrpDualNdb_ *nextChg;		/* Next dndb in change queue thread */

	EigrpXmitThread_st	xmitThread;	/* Transmit thread */
	EigrpNetEntry_st		dest;		/* Destination network */
	U32 		metric;					/* RD - reported metric(composite) */
	U32 		oldMetric;				/* Successors metric before active */	/* FD TIGER	*/
	EigrpVmetric_st 		vecMetric;	/* RD - reported metric(vectorized) */

	U32 		origin;					/* Query-origin flag */
	U16 		sndFlag;				/* Send update, query, or reply */
	U16 		succNum;				/* # of successors for this dest */
	
	EigrpHandle_st 	replyStatus;		/* Reply status table handles */
	S8		opaqueFlagOld;			/* Flags maintained by PDM */
	
	S32		flagExt:1;				/* {in, ex}ternal of 1st successor */
	S32		flagSplitHorizon:1;		/* Used for split horizoning queries */
	S32		flagBeingSent:1;			/* Set if DNDB is being sent */
	S32		flagTellEveryone:1;		/* Suppress split horizon */
	S32		flagDel:1;				/* DNDB has been freed */
	S32		flagOnChgQue:1;			/* Set if on change thread */
	
	EigrpDualRdb_st	*rdb;			/* Next hops.  1st one is successor */
	EigrpDualRdb_st	*succ;			/* Old successor when active.  If */
									/* NULL then rdb is old successor */
	struct EigrpIdb_	*shQueryIdb;	/* Split-horiz i/f for queries */
	U32 activeTime;					/* Time when this dest went active */
	S32		exterminate;			/*zhenxl_20130117 DNDB is invalid, it will be advertised with infinity metric and distroied soon.*/
}EigrpDualNdb_st;

#define	EIGRP_MIN_THREAD_DUMMIES	20000	/* org:5000*/ /*EigrpUtilChunkMalloc size */

#define	EIGRP_DUAL_LOGGING_DUAL_DEFAULT	FALSE
#define	EIGRP_DUAL_LOGGING_XPORT_DEFAULT	FALSE
#define	EIGRP_DUAL_LOGGING_XMIT_DEFAULT	FALSE

/* send_flag definitions.  Order is important.  Reply is first, followed by the remaining unicasts,
  * followed by all multicasts. */
#define	EIGRP_DUAL_SEND_REPLY	0x1    /* Send a unicast reply */
#define	EIGRP_DUAL_MULTI_QUERY	0x2    /* Send a multicast query */
#define	EIGRP_DUAL_MULTI_UPDATE	0x4    /* Send a multicast update */

#define	EIGRP_DUAL_MULTI_MASK		(EIGRP_DUAL_MULTI_UPDATE | EIGRP_DUAL_MULTI_QUERY)

/* EIGRP fixed header definition. */
typedef struct EigrpPktHdr_{
	S8	version;
	S8	opcode;
	U16	checksum;
	U32	flags;
	U32	sequence;
	U32	ack;
	U32	asystem;
}EigrpPktHdr_st, *EigrpPktHdr_pt;

/* Return values from advertiseRequest */
enum{
	EIGRP_ROUTE_TYPE_SUPPRESS,		/* Don't advertise it */
	EIGRP_ROUTE_TYPE_ADVERTISE,		/* Advertise it */
	EIGRP_ROUTE_TYPE_POISON			/* Advertise it with infinity metric */
};

/* peertype	-- descriptor block for a EIGRP peer.
  *
  * Peers descriptor blocks are instantiated and maintained by the EIGRP transport, not the DUAL
  * routing engine.
  *
  * Peer descriptors contain all the information necessary for the operation of the reliable 
  * transport. */
typedef struct EigrpDualPeer_{
	struct EigrpDualPeer_ *next;		/* Next peer at this hash location */
	struct EigrpDualPeer_ *nextPeer;	/* Next peer in global list */

	U32		lastStartupSerNo;		/* Serial # of end of startup stream */
	U32		lastSeqNo;				/* Last rcvd sequence number */
	U32		source;					/* Peer id */
	U32		routerId;				/* Peer router #, for protocols w/o  unique addresses */
	
	struct EigrpIdb_ *iidb;				/* Interface peer discovered on */
	EigrpMgdTimer_st holdingTimer;	/* Holding timer for peer */
	U32		lastHoldingTime;			/* Holding time rcvd in last hello */
	U32		uptime;					/* Timestamp when peer came up */
	
	EigrpQue_st *xmitQue[EIGRP_QUEUE_TYPES];	/* Reliable & unreliable xmit queues */
	U32		unrelyLeftToSend;		/* Unrely left before rely sent */
	U32		retransCnt;				/* Count of rexmissions to this peer */
	U32		pktFirstSndTime;		/* Time current packet first sent */
	U32		reInitStart;				/* Time we started reinit */
	U16		retryCnt;				/* Rxmit count for packet on queue */
	
	S32		flagCrMode:1;			/* Conditional Received Mode set */
	S32		flagNeedInit:1;			/* TRUE if next pkt should have INIT */
	S32		flagNeedInitAck:1;		/* Need ACK of our INIT packet */
	S32		flagGoingDown:1;		/* Peer being cleared by command */
	S32		flagComingUp:1;			/* Peer is just coming up */
	S32		flagPeerDel:1;			/* Peer has been freed */
	
	U32		srtt;					/* Smoothed round trip time */
	U32		rto;						/* Retransmission time out */
	
	S32 		peerHandle;				/* Unique handle assigned to peer */
	EigrpSwVer_st 	peerVer;		/* Software version running on nbr */
	EigrpMgdTimer_st	peerSndTimer;	/* Timer for peer(re)transmission */
	
	void		*protoData;				/* Protocol specific data about nbr */
	U32 		sndProbeSeq;			/* Seq number of next probe to send */
	U32 		crSeq;					/* Expected seq # of CR packet, or 0 */
	S8		*downReason;			/* Reason that neighbor went down */
	U32		helloSize;  			
#ifdef INCLUDE_SATELLITE_RESTRICT
	EigrpPktRecord_st		firstRecvedHello;/*t2 hello*/
	U32		rtt;
#endif
}EigrpDualPeer_st;

/* EIGRP revision level
  *
  * The revision level is used to track upward compatible changes in the protocol, so that newer
  * code can know whether or not it may exploit these changes. Non-upward compatible changes should
  * be signalled by changing the "version" field in the EIGRP header(and woe be unto you). */
#define	EIGRP_VER_SOFT_MAJ	1		/* Lube job, tires rotated */
#define	EIGRP_VER_SOFT_MIN	2		/* It may be back in the shop soon */
#define	EIGRP_VER_SYS_MAJ		12
#define	EIGRP_VER_SYS_MIN		2

#define	EIGRP_AUTH_TYPE		0x0002
#define	EIGRP_AUTH_LEN		0x0010
#define	EIGRP_AUTH_KEYID_DEF		0x0001
typedef struct EigrpAuthInfo_{
	U16	type;
	U32	keyId;
	U8	authData[32];
	U8   	end;
}EigrpAuthInfo_st;

#define	EIGRP_OFFSET_OUT		0
#define	EIGRP_OFFSET_IN		1
#define	EIGRP_OFFSET_LEN		2
typedef struct EigrpOffset_{
	U32  acl;
	U32  offset;
} EigrpOffset_st;

/*EigrpRedisEntry_st type  */
typedef struct EigrpNeiEntry_{
	struct EigrpNeiEntry_	*next;
	
	U32  address;
}EigrpNeiEntry_st;

/* Structure definition for interfaces that a respective ddb is configured for.
  *
  * All reliable packets(unicast or multicast) are initially enqueued on the IIDB xmitQue. This
  * ensures that the packets never get out of order. As packets are pulled off this queue, they are
  * given a sequence number and are either sent immediately(for multicasts) or re-queued on the
  * peer queue(for unicasts).
  *
  * If the transport tells us that the interface is multicast flow-ready, we will send the next
  * packet(or go quiescent) even though not all peers may have acknowledged the packet(and in
  * fact the packet may be retransmitted.) */
typedef struct EigrpIdb_{
	struct EigrpIdb_ *next;				/* For the DDB queue */
	struct EigrpIdb_ *nextQuiescent;		/* Link for DDB quiescent list */
	struct EigrpIdb_ **prevQuiescent; 		/* Back link */

	struct EigrpIntf_	*idb;				/*Interface EIGRP is enabled for */
	EigrpXmitAnchor_st *xmitAnchor;		/* Anchor for current packet */
	EigrpHandle_st handle;					/* Handles for peers on this i/f */

	EigrpQue_st *xmitQue[EIGRP_QUEUE_TYPES];	/* Interface transmit queues */
	struct EigrpPktDescQelm_ *lastMultiPkt;	/* Last mcast packet on i/f */

	U32		totalSrtt;					/* Sum of all peer SRTTs */
	U32		helloInterval;				/* Configured hello interval for idb */
	U32		holdTime;					/* Configured holdTime for idb */
	U32		holdDown;					/* Configured holdDown for EIGRP */
	U32		maxPktSize;					/* Packet MTU, with header, minus overhead */
	S32		splitHorizon;					/* Perform split horison on this idb */
	S32		goingDown;					/* TRUE if the IIDB is to be deleted */
	S32		useMcast;					/* TRUE if we should use real mcast */
	S32		passive;						/* TRUE if this interface is passive */
	S32		invisible;					/* TRUE if this interface is invisible */
	S32		mcastEnabled;				/* TRUE if mcast filter is set up */
	
	EigrpMgdTimer_st	sndTimer;			/* Used to pace xmits on this i/f */
	EigrpMgdTimer_st	peerTimer;			/* Master for all peers on this i/f */
	EigrpMgdTimer_st	holdingTimer;		/* Master for holding timers on i/f */
	EigrpMgdTimer_st	helloTimer;			/* Fires when we need to send hello */
	EigrpMgdTimer_st	flowCtrlTimer;		/* Multicast flow control timer */
	EigrpMgdTimer_st	pktTimer;			/* Delay before packetization */
	EigrpMgdTimer_st	aclChgTimer;		/* Delay after access list change */
	
	U32		lastMcastFlowDelay;			/* Last delay for flow control timer */
	U32		bwPercent;					/* % of bandwidth for EIGRP */
	U32		delay;						/* delay for EIGRP */
	U32		sndInterval[EIGRP_QUEUE_TYPES];	/* Base packet gap, per queue */
	U32		lastSndIntv[EIGRP_QUEUE_TYPES];	/* Current packet gap */
	
	U32		unicastLeftToSnd;			/* Count of ucasts before next mcast */
	U32		mcastStartRefcount;			/* Refcount on mcast when sent */
	U32		lastMcastSeq;				/* Sequence number of last multicast */
	U32		mcastSent[EIGRP_QUEUE_TYPES];	/* Count of un/reliable mcasts */
	U32		unicastSent[EIGRP_QUEUE_TYPES];	/* Count of un/reliable ucasts */
	U32		mcastExceptionSent;			/* Count of mcasts sent as ucasts */
	U32		crPktSent;					/* Count of mcasts sent with CR */
	U32		ackSupp;					/* Count of suppressed ACK packets */
	U32		retransSent;					/* Count of retransmitted packets */
	U32		outOfSeqRcvd;				/* Count of out-of-sequence packets */
	U32		sndProbeSeq;				/* Sequence number of probe to send */

	S32		authSet;						/* Set auth info */
	EigrpAuthInfo_st authInfo;				/* Authentication information */
	S32		authMode;          				/* Md5 authenticate */
	EigrpOffset_st offset[EIGRP_OFFSET_LEN];		/* offset-list in/out information */
} EigrpIdb_st;

/* Scratch table for figuring out interface changes.  There's one entry per IIDB in the system. 
  * It's used only by EigrpDualRouteUpdate, but it's used a lot, so we hang it off of the DDB to
  * avoid thrashing. */
typedef struct EigrpDualIidbScratch_{
	EigrpIdb_st *iidb;						/*IIDB in question */
	S32 found;							/* TRUE if we had a succ on this i/f */
}EigrpDualIidbScratch_st;

/* Packet Descriptor
  *
  * This data structure is used to abstractly represent the contents of a packet.  Note that for
  * most packets, the contents are represented as a range of serial numbers and the packet is not
  * actually generated until the time of transmission.
  *
  * If the sequence number is 0 and the packet is sequenced, this indicates that the packet hasn't 
  * been sent yet(not a retransmission).  A nonzero sequence number indicates that the packet has
  * been sent already.
  *
  * Note that ACKs are represented as Hellos with the flagAckPkt bit set. The sequence number value
  * to acknowledge in this case is carried in the ackSeqNum field.  For all sequenced packets, this
  * field carries the packet sequence number once it is assigned.
  *
  * If the packet is a unicast, the "peer" variable is set to point at the peer structure.
  *
  * Pregenerated packets are denoted by pregen_packet being non-NULL.  In this case the packet was
  * generated when the descriptor was enqueued, and so no callback is made to the PDM to build the
  * packet when it is ready to be transmitted(since the packet has already been built). */
typedef struct EigrpPackDesc_{
	EigrpDualPeer_st		*peer;			/* Peer to send to(or NULL) */
	EigrpXmitAnchor_st	*pktDescAnchor;	/* Anchor for serNoStart */
	
	U32 		serNoStart;					/* Starting serial number */
	U32 		serNoEnd;					/* Ending serial number */
	EigrpPktHdr_st *preGenPkt;			/* Pregenerated packet */
	U32 		length;						/* Length of packet */
	U32 		refCnt;						/* Count of elements pointing here */
	U32 		ackSeqNum;					/* Ack/Seq number of this packet */
	
	S8		flagSetInit:1;				/* Set if INIT flag should be sent */
	S8		flagSeq:1;					/* Set if packet is sequenced */
	S8		flagPktDescMcast:1;			/* Set if sent as a multicast */
	S8		flagAckPkt:1;				/* Set if this is an ACK */
	S8		opcode;						/* Packet opcode */
}EigrpPackDesc_st;
  
/* Protocol independent MIB counters. Send counts must precede receive counts and values must be
  * u_longs, see EigrpUpdateMib(). */
typedef struct EigrpMib_{
	U32		helloSent;
	U32		helloRcvd;
	U32		queriesSent;
	U32		queriesRcvd;
	U32		updateSent;
	U32		updateRcvd;
	U32		repliesSent;
	U32		repliesRcvd;
	U32		ackSent;
	U32		ackRcvd;
}EigrpMib_st;

/* Route Parameters
  *
  * This data structure is passed around when a route has been received (or redistributed).  It
  * carries all the parameters as received. */
typedef struct EigrpDualNewRt_{
	EigrpNetEntry_st 	dest;		/* Destination address/mask */
	U32 		nextHop;			/* Next hop */
	U32 		infoSrc;				/*Info source */
	EigrpIdb_st *idb;				/*Interface */
	struct EigrpIntf_	*intf;
	EigrpVmetric_st 	vecMetric;	/* Vectorized metric */
	U32 		metric;				/* Composite metric */
	U32 		succMetric;			/* Successor metric */
	U32		origin;				/* Route origin */
	
	S32 		flagIgnore	:1;		/*Ignore route due to filtering */
	S32 		flagExt		:1;		/* Route is external */
	S32 		flagKeep	:1;		/* Keep dest during table reload */
	
	void 	*data;				/* PDM-specific data */
	S8 		opaqueFlag;			/* PDM-specific flags */
	S32 		rtType;         	
	S32 		asystem;

	U32		originRouter;		/*zhenxl_20130116 Origin router ID of this route.*/
}EigrpDualNewRt_st;

/* Dual descriptor block.
  * Explanation of fields:
  *  next             Pointer to next ddb in a queue of ddbs.
  * Static entries in the ddb.
  *  pdb              Pointer to protocol descriptor block.  This is a very IP centric data 
  *			    structure.  It should probably be a generic pointer used by the protocol specific
  *			    routines. Examples of information in this structure should be things like input and
  *			    output filters, distance info, process name, process id, jump vectors to various
  *                     hooks and handlers.
  *  cleanup        This S32 tells us whether we need to delete any rdb's from the topology table.
  *                     This occurs when a destination has gone passive and some(or all) of the
  *                     next hops are reporting a metric of infinity.
  * Jump vectors to protocol specific support routines.  Note: we may not need the first two 
  * entries.  Time will tell.
  *  (*acceptPeer)() Find out if we should accept this peer
  *  (*itemAdd)()    Add a route to a packet.  Called from DUAL when the packet is being
  *				constructed.
  *  (*addrcopy)()    Copy an EIGRP address
  *  (*addressMatch)()   Return true if the two addresses passed in are equal.
  *  (*advertiseRequest)()  Called so the protocol dependent components can do filtering and
  *                                   aggregation on per interface basis.
  *  (*buildPkt)() Called by the driver to build a protocol-specific packet based on information in
  *                     the packet descriptor.
  *  (*compareExt()  Compare the contents of a DRDB's external information to help decide whether
  *                          the route has changed.
  *  (*enable_process_wait)()  Called by the hello process to turn on a PDM-specific process
  *						   predicate.
  *  (*exteriorCheck)()  Test whether the interior/exterior state of a route has changed.
  *  (*exteriorFlag)()  Find a feasible successor with the exterior bit set.
  *  (*headerPtr)()   Returns pointer to beginning of EIGRP header.
  *  (*hellohook)()   Called after the hello packet has been allocated and filled with the initial
  *                         tlvs. This vector is optional; if left as NULL, won't be called. After
  *				 this vector is called, the hello packet is sent. Note that if the hellohook 
  *				 function appends data to the end of the hello packet after the initial tlvs, it
  *				 should note the small allocated size of the hello buffer or allocate/copy/insert
  *				 a new buffer. If this returns FALSE, the hello should NOT be sent!
  *  (*iidbCleanUp)() Called to do any PDM-specific IIDB cleanup before the IIDB is destroyed.
  *  (*iidbShowDetail)()  Display PDM-specific IIDB details.
  *  (*input_packet)()  Fetch the next input packet.
  *  (*intfChg)()  Called to signal a change in interface metric.
  *  (*interface_up)()  Returns a protocol-specific view of whether an interface is up or down.
  *  (*itemSize)()   Returns the byte size of a route item.
  *  (*lastPeerDeleted)()  Called for PDM-specific cleanup after the last neighbor on an interface
  *					  goes down.
  *  (*localAddress)()   Returns TRUE if the specified address belongs to this system.
  *  (*mmetricFudge)() Returns an amount to be added to the route metric.
  *  (*ndbBucket)()   Returns the NDB hash bucket for the destination.
  *  (*ndb_done)()    Called for PDM-specific processing before an NDB is deleted.
  *  (*ndb_new)()     Called for PDM-specific processing when an NDB is created.
  *  (*networkMatch)()    Compares two destinations.
  *  (*paklen)()	     Returns the data length of a received EIGRP packet.
  *  (*pdmTimerExpiry)()  Called when the PDM timer expires.
  *  (*peerBucket)()  Returns the peer hash bucket for an address.
  *  (*peerShowHook)()  Called to do PDM-specific peer detail display.
  *  (*peerStateHook)()  Called to do PDM-specific peer validation.
  *  (*printAddress)()   return pointer to sprintf a eigrp neighbor address printed in the
  *				     protocol's native format.
  *  (*printNet)()    return pointer to sprintf a eigrp network number printed in the protocol's
  *                        native format.
  *  (*rtDelete)()    Delete an entry from the routing table.
  *  (*rtgetmetric)() Return the best metric for this destination.  This routine may be scrapped as
  *				well.  We can get the best metric used in the routing table when we do an update.
  *                   	This is done by comparing the currently believed best metric in the routing
  *				table with the metric of the nexthop gateway we just successfully installed.
  *                   Although it may still be required for sending update
  *                   query or reply packets.
  *  (*rtModify)()    Notify routing table that subsequent update is to be taken regardless of
  *				metric.  This usually boils down to a call to _rtdelete(), but is stubbed out in
  *				where _rtupdate() can accept any information blindly.
  *  (*rtUpdate)()    Update the routing table.  If the next hop information matches information
  *				 already in the routing table and it is the only next hop for this destination,
  *				 blindly overwrite new information with old regardless of the offered metric.
  *				 Return true if the update was successful.
  *  (*sndPkt)()  Called to write the specified packet to the specified peer. Peer address will be
  *			   NULL if the packet should be broadcast.
  *  (*tansReady)()  Called by the DUAL when a peer or interface becomes flow-control ready. 
  *				 Guaranteed to be called only from the routing process thread.  The interface
  *		     		 pointer is always non-NULL.  The peer pointer is NULL when an interface
  *				 becomes flow-ready for multicast, and is non-null when a peer becomes flow-ready
  *				 for unicast. If the pktDesc is non-null, it points to a packet that has just been
  *				 acknowledged by the peer(peer will always be non-NULL if so).  info_pending is 
  *				 TRUE if DUAL has information to send.  This callback provides a hook for the PDM
  *				 to add packets to the transmit flow, and to clean up after packets that it added 
  *				 are acknowledged.
 */
typedef struct EigrpDualDdb_{
	struct EigrpDualDdb_	*next;      			/* Pointer to next ddb */

	struct EigrpPdb_		*pdb;                  		/* Pointer to pdb */
	EigrpDualPeer_st		*peerLst;			/* Full list of all peers */
	EigrpDualPeer_st		 **peerArray; 		/* Array of peer pointers, indexed by handle */
	U32 		peerArraySize;					/* Number of entries in peer array */
	EigrpDualPeer_st		*peerCache[EIGRP_NET_HASH_LEN]; 	/* Hash into peers we've heard */
	
	EigrpHandle_st		handle;				/* Array of peer handle bits */
	EigrpDualNdb_st		*topo[EIGRP_NET_HASH_LEN];		/* Hash into topology table */
	U32		routes;

	S8		*name;							/* Name of protocol specific user of ddb */
	S32		index;							/*Index into dual_ddbtable */
	U32		pktOverHead;					/* Overhead, in bytes, for this protocol */
	EigrpXmitThread_st	*threadHead;		/* Head of transmit DNDB thread */
	EigrpXmitThread_st	**threadTail;		/* Tail of transmit DNDB thread */
	
	U32		nextSerNo;						/* Next serial number to use */
	EigrpDualNdb_st		*chgQue;			/* Thread of newly-changed DNDBs */
	EigrpDualNdb_st		**chgQueTail;		/* Tail of change queue */

	EigrpIdb_st		*nullIidb;				/* IIDB for Null0 */
	EigrpIdb_st		*quieIidb;				/* List of quiescent interfaces */
	EigrpQue_st		*iidbQue;				/* Interfaces this ddb is configured on */
	EigrpQue_st		*temIidbQue;			/* Used to hold config info before if enabled*/
	EigrpQue_st		*passIidbQue;			/* Passive interfaces for this ddb */
	S8		k1, k2, k3, k4, k5;				/* K-values transmitted in hellos */
	S8		maxHopCnt;						/* Hopcount for infinity metric */
	
	S32		flagDbgTargeSet:1;				/* Is the following structure setup? */
	S32		flagDbgPeerSet:1;				/* Same as above, but for peer filtering */
	S32		flagDdbLoggingEnabled:1;		/* Event logging is enabled */
	S32		flagDualLogging:1;				/* Logging DUAL events */
	S32		flagDualXportLogging:1;			/* Logging transport events */
	S32		flagDualXmitLogging:1;			/* Logging transmission events */
	S32		flagKillAll:1;						/* Mass suicide if an SIA occurs */
	S32		flagGoingDown:1;				/* Process is dying */
	S32		flagLogAdjChg:1;					/* Logging peer changes */
	S32		flagLogEvent:1;					/* TRUE if log from evt Q per-event */
	
	U32		asystem;						/* AS number used to store in packets */
	U32		sndSeqNum;						/* Last sequence number used to send packet */
	U32		activeIntfCnt;					/* # of interfaces with active peers */
	U32		routerId;						/* ID of this router */ /* Router ID from which we heard this */
	U32		scratchTblSize;					/* Number of entries in scratch table */
	EigrpDualIidbScratch_st		*scratch;		/* Scratchpad for EigrpDualRouteUpdate */
	EigrpNetEntry_st	dbgTarget;				/* Used for filtering debugging output */
	U32		dbgPeer;						/* Used for filtering debugging output */
	U32		activeTime;						/* Maximum age of an active route */
	EigrpMgdTimer_st		masterTimer;		/* Master timer for the DDB */
	
	EigrpMgdTimer_st		intfTimer;			/* Parent for all interface timers */
	EigrpMgdTimer_st		peerTimer;			/* Grandparent for all peer timers */
	EigrpMgdTimer_st		activeTimer;			/* Timer for checking SIA routes */
	EigrpMgdTimer_st		helloTimer;			/* Parent for all hello timers */
	EigrpMgdTimer_st		aclChgTimer;		/* Delay after access list change */
	EigrpMgdTimer_st		pdmTimer;			/* Timer for the PDM */

	EigrpChunk_st		*extDataChunk;			/* Chunks for external data */
	EigrpChunk_st		*workQueChunk;			/* Chunks for the work queue */

	EigrpMib_st		mib;					/*  MIB variables  */
	U32				maxEvent;				/* Size of event buffer */
	EigrpDualEvent_st	*events;				/* DUAL event log */
	EigrpDualEvent_st	*eventCurPtr;			/* Next entry for event log */
	EigrpDualEvent_st	*siaEvent;				/* DUAL event log from last SIA */
	EigrpDualEvent_st	*siaEventPtr;			/* Pointer to event log from SIA */
	U32				eventCnt;				/*serial # of event */

	U32		(*addrCopy)(U32 *, S8 *);
	S32		(*acceptPeer)(struct EigrpDualDdb_ *, U32 *,  EigrpIdb_st *);
	S32		(*addressMatch)(U32 *, U32 *);
	S32		(*networkMatch)(EigrpNetEntry_st *, EigrpNetEntry_st *);
	S32		(*localAddress)(struct EigrpDualDdb_ *, U32 *, struct EigrpIntf_ *);
	S8		*(*printAddress)(U32*);
	S8		*(*printNet)(EigrpNetEntry_st*);
	U32		(*advertiseRequest)(struct EigrpDualDdb_ *,	EigrpDualNdb_st *, struct EigrpIntf_ *);
	EigrpPktHdr_st	*(*buildPkt)(struct EigrpDualDdb_ *, EigrpDualPeer_st *,    EigrpPackDesc_st *, S32 *);
	S32		(*compareExt)(void *, void *);
	void		(*exteriorCheck)(struct EigrpDualDdb_ *, EigrpDualNdb_st *, S8 *);
	S8		(*exteriorFlag)(EigrpDualNdb_st *);
	EigrpPktHdr_st	*(*headerPtr)(void *);
	S32		(*hellohook)(struct EigrpDualDdb_ *, EigrpPktHdr_st *, S32 *, void *,	 S8 *);
	void		(*iidbCleanUp)(struct EigrpDualDdb_ *, EigrpIdb_st *);
	void		(*iidbShowDetail)(struct EigrpDualDdb_ *, EigrpIdb_st *);
	void		(*intfChg)(struct EigrpDualDdb_ *, EigrpIdb_st *);
	U32		(*itemSize)(EigrpDualNdb_st *);
	U32		(*itemAdd)(struct EigrpDualDdb_ *, EigrpIdb_st *, EigrpDualNdb_st *,  void *, U32, S32, S8, EigrpDualPeer_st *);
	void		(*lastPeerDeleted)(struct EigrpDualDdb_ *, EigrpIdb_st *);
	U32		(*mmetricFudge)(struct EigrpDualDdb_ *, EigrpNetEntry_st *, EigrpIdb_st *, S8);
	S32		(*ndbBucket)(EigrpNetEntry_st *);
	void		(*pdmTimerExpiry)(struct EigrpDualDdb_ *, EigrpMgdTimer_st *);
	S32		(*peerBucket)(U32 *);
	void		(*peerShowHook)(struct EigrpDualDdb_ *, EigrpDualPeer_st *);
	void		(*peerStateHook)(struct EigrpDualDdb_ *, EigrpDualPeer_st *, S32);
	void		(*rtDelete)(struct EigrpDualDdb_ *, EigrpDualNewRt_st*);
	void		(*rtModify)(struct EigrpDualDdb_ *, EigrpDualNewRt_st*);
	S32		(*rtUpdate)(struct EigrpDualDdb_ *, EigrpDualNdb_st *, EigrpDualRdb_st *, S32 *);
	void		(*sndPkt)(EigrpPktHdr_st *, S32, EigrpDualPeer_st *, EigrpIdb_st *,   S32);
	void		(*tansReady)(struct EigrpDualDdb_ *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *, S32);
	void		(*setMtu)(EigrpIdb_st *);

	void		*threadAtciveTimer;
	S32		masterTimerProc;
	S32		helloTimerProc;
	S32		siaTimerProc;
}EigrpDualDdb_st;

/* show dual event sense */
#define	EIGRP_EVENT_CUR 	0
#define	EIGRP_EVENT_SIA 	1
#define	EIGRP_DUAL_SPLIT_HORIZON_QUERIES	1

enum{
	EIGRP_STARTUP_THREAD, 
	EIGRP_MCAST_THREAD, 
	EIGRP_UCAST_THREAD
};

enum{
	EIGRP_RECV_UPDATE,
	EIGRP_RECV_QUERY,
	EIGRP_RECV_REPLY,
};

S8	*EigrpDualQueryOrigin2str(U32);
S8	*EigrpDualRouteType2string(U32);
S8	*EigrpDualRouteEvent2String(U32);
S32	EigrpDualSernoEarlier(U32, U32);
S32	EigrpDualSernoLater(U32, U32);
EigrpDualRdb_st *EigrpDualThreadToDrdb(EigrpXmitThread_st *);
EigrpDualNdb_st *EigrpDualThreadToDndb(EigrpXmitThread_st *);
S32	EigrpDualXmitThreaded(EigrpXmitThread_st *);
S32	EigrpDualIidbOnQuiescentList(EigrpIdb_st *);
void	EigrpDualRemoveIidbFromQuiescentList(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpDualAddIidbToQuiescentList(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpDualXmitUnthreadUnanchored(EigrpDualDdb_st *, EigrpXmitThread_st *);
U32	EigrpDualNextSerno(EigrpDualDdb_st *);
EigrpXmitAnchor_st *EigrpDualAnchorEntry(EigrpDualDdb_st *, EigrpXmitThread_st *);
void	EigrpDualUnanchorEntry(EigrpDualDdb_st *, EigrpXmitThread_st *);
void	EigrpDualXmitUnthread(EigrpDualDdb_st *, EigrpXmitThread_st *);
void	EigrpDualFreeData(EigrpDualDdb_st *, void **);
void	EigrpDualRdbDelete(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *);
void	EigrpDualBuildRoute(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *, EigrpDualNewRt_st*);
void	EigrpDualZapDrdb(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *, S32);
void	EigrpDualDndbDelete(EigrpDualDdb_st *, EigrpDualNdb_st **);
void	EigrpDualCleanupDndb(EigrpDualDdb_st *, EigrpDualNdb_st **, U32);
void	EigrpDualMoveXmit(EigrpDualDdb_st *, EigrpXmitThread_st *);
void	EigrpDualCleanupDrdb(EigrpDualDdb_st *, EigrpDualRdb_st *, S32);
void	EigrpDualProcessAckedReply(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *);
void	EigrpDualDropDndbRefcount(EigrpDualDdb_st *, EigrpXmitThread_st *, EigrpPackDesc_st *, EigrpIdb_st *);
void	EigrpDualProcessAckedMulticast(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *);
void	EigrpDualProcessAckedStartup(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *);
EigrpXmitThread_st *EigrpDualPacketizeThread(EigrpDualDdb_st *, EigrpIdb_st *, EigrpDualPeer_st **, U32 *, EigrpXmitThread_st *, U32, U32 *);
S32	EigrpDualOpcode(S32);
void	EigrpDualPacketizeInterface(EigrpDualDdb_st *, EigrpIdb_st *);
S32	EigrpDualProcessAckedPacket(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *);
void	EigrpDualXmitContinue(EigrpDualDdb_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpDualPeer_st *);
void	EigrpDualXmitDrdb(EigrpDualDdb_st *, EigrpDualRdb_st *);
void	EigrpDualXmitDndb(EigrpDualDdb_st *, EigrpDualNdb_st **);
void	EigrpDualFinishUpdateSendGuts(EigrpDualDdb_st *);
void	EigrpDualNewPeer(EigrpDualDdb_st *, EigrpDualPeer_st *);
S32	EigrpDualSplitHorizon(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpIdb_st *);
U32 EigrpDualShouldAdvertise(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpIdb_st *);
EigrpPktHdr_st *EigrpDualBuildPacket(EigrpDualDdb_st *, EigrpIdb_st *,EigrpDualPeer_st *, EigrpPackDesc_st *, S32 *);
void	EigrpDualLastPeerDeleted(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpDualFirstPeerAdded(EigrpDualDdb_st *, EigrpIdb_st *);
EigrpDualRdb_st *EigrpDualSuccessor(EigrpDualNdb_st *);
void	EigrpDualState(EigrpDualDdb_st *, EigrpDualNdb_st *, U32);
void	EigrpDualClearHandle(EigrpDualDdb_st *, EigrpDualNdb_st *, S32 );
void	EigrpDualPromote(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *);
void	EigrpDualRtupdate(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *);
void	EigrpDualRdbClear(EigrpDualDdb_st *, EigrpDualNdb_st **, EigrpDualRdb_st *);
void	EigrpDualSendQuery(EigrpDualDdb_st *, EigrpDualNdb_st **);
S32	EigrpDualActive(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *);
EigrpDualNdb_st *EigrpDualNdbLookup(EigrpDualDdb_st *, EigrpNetEntry_st *);
EigrpDualRdb_st *EigrpDualRdbLookup(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpIdb_st *, U32 *);
void	EigrpDualSendReply(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpDualRdb_st *);
void	EigrpDualSetFD(EigrpDualNdb_st *, U32 );
void	EigrpDualMaxRD(EigrpDualDdb_st *, EigrpDualNdb_st *);
U32	EigrpDualSetRD(EigrpDualDdb_st *, EigrpDualNdb_st *);
S32	EigrpDualCompareDrdb(EigrpDualDdb_st *, EigrpDualNewRt_st *, EigrpDualRdb_st *);
U32	EigrpDualUpdateTopo(EigrpDualDdb_st *, S32, S32 *, EigrpDualNewRt_st *, EigrpDualNdb_st **, EigrpDualRdb_st **, S32 *, U32);
void	EigrpDualScanUpdate(EigrpDualDdb_st *, EigrpDualNdb_st *);
void	EigrpDualRouteUpdate(EigrpDualDdb_st *, EigrpDualNdb_st **);
void	EigrpDualSetDistance(EigrpDualDdb_st *, EigrpDualNdb_st *);
S32	EigrpDualTestFC(EigrpDualDdb_st *, EigrpDualNdb_st *);
void	EigrpDualFCSatisfied(EigrpDualDdb_st *, EigrpDualNdb_st **, EigrpDualRdb_st *, S32);
S32	EigrpDualFC(EigrpDualDdb_st *, EigrpDualNdb_st **, EigrpDualRdb_st *, S32);
void	EigrpDualTransition(EigrpDualDdb_st *, EigrpDualNdb_st **, EigrpDualRdb_st *);
void	EigrpDualCleanupReplyStatus(EigrpDualDdb_st *, EigrpDualNdb_st **, EigrpDualRdb_st *);
void	EigrpDualValidateNdbRoute(EigrpDualDdb_st *, EigrpDualNdb_st *);
void	EigrpDualSendUpdate_int(EigrpDualDdb_st *, EigrpDualNdb_st *, S8 *, S8 *, U32);
#define EigrpDualSendUpdate(ddb_m, dndb_m, reason_m)	EigrpDualSendUpdate_int(ddb_m, dndb_m, reason_m, __FILE__, __LINE__)
void	EigrpDualValidateRoute(EigrpDualDdb_st *, EigrpNetEntry_st *);
void	EigrpDualReloadProtoTable(EigrpDualDdb_st *);
void	EigrpDualRecvUpdate(EigrpDualDdb_st *, EigrpDualNewRt_st *);
void	EigrpDualRecvQuery(EigrpDualDdb_st *, EigrpDualNewRt_st *);
void	EigrpDualRecvReply(EigrpDualDdb_st *, EigrpDualNewRt_st *);
void	EigrpDualRecvPacket(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpDualNewRt_st *, U32);
void	EigrpDualPeerDown(EigrpDualDdb_st *, EigrpDualPeer_st *);
void	EigrpDualIfDelete(EigrpDualDdb_st *, EigrpIdb_st *);
void	EigrpDualConnRtchange(EigrpDualDdb_st *, EigrpDualNewRt_st *, U32);
void	EigrpDualRtchange(EigrpDualDdb_st *, EigrpDualNewRt_st *, U32);
void	EigrpDualLostRoute(EigrpDualDdb_st *, EigrpNetEntry_st *);
EigrpDualNdb_st *EigrpDualTableWalk(EigrpDualDdb_st *, EigrpDualNdb_st *, S32 *);
void	EigrpDualCleanUp(EigrpDualDdb_st *);
void	EigrpDualSiaCopyLog(EigrpDualDdb_st *);
void	EigrpDualUnstickDndb(EigrpDualDdb_st *, EigrpDualNdb_st *);
void	EigrpDualScanActive(EigrpDualDdb_st *, S32 *);	/* tigerwh 120608  */
EigrpDualRdb_st *EigrpDualRouteLookup(EigrpDualDdb_st *, EigrpNetEntry_st*, EigrpIdb_st *, U32 *);
S8 *EigrpDualShowEventParam(EigrpDualDdb_st *, U32, S8 *, S8 *, U32, S32 *);
void	EigrpDualFormatEventRecord(EigrpDualDdb_st *, S8 *, U32, EigrpDualEvent_st *, U32, S32);
void	EigrpDualLogEvent(EigrpDualDdb_st *, U32, void *, void *);
S32	EigrpDualInitEvents(EigrpDualDdb_st *, U32, S32);
void	EigrpDualClearEventLogging(EigrpDualDdb_st *);
void	EigrpDualClearLog(EigrpDualDdb_st *);
void	EigrpDualAdjustScratchPad(EigrpDualDdb_st *);
void	EigrpDualProcSiaTimerCallbk(void *);
void	EigrpDualOnTaskActiveTimer(EigrpDualDdb_st *);
void	EigrpDualInitTimers(EigrpDualDdb_st *);
S32	EigrpDualInitDdb(EigrpDualDdb_st *, S8 *, U32, U32);
void	EigrpDualFreeShowBlocks(EigrpDualShowEventBlock_st *);
void	EigrpDualShowEvents(EigrpDualDdb_st *, S32, U32 , U32);
U32	EigrpDualComputeMetric(U32, U32, U32, U32, U32, U32, U32, U32, U32);
U32	EigrpDualPeerCount(EigrpIdb_st *);
void	EigrpDualLogXportAll(EigrpDualDdb_st *, U32, void *, void *);
void	EigrpDualFinishUpdateSend(EigrpDualDdb_st *);
S32	EigrpDualRouteIsExternal(EigrpDualNdb_st *);
struct EigrpIntf_ *EigrpDualIdb(EigrpIdb_st *);
S32	EigrpDualDndbActive(EigrpDualNdb_st *);
U32	EigrpDualIgrpMetricToEigrp(U32);
S32	EigrpDualKValuesMatch(EigrpDualDdb_st *, struct EigrpParamTlv_ *);
void	EigrpDualLogXmitAll(EigrpDualDdb_st *, U32, void *, void *);
void	EigrpDualLogAll(EigrpDualDdb_st *, U32, void *, void *);
void	EigrpDualLogXmit(EigrpDualDdb_st *, U32, EigrpNetEntry_st *, void *, void *);
void	EigrpDualDebugEnqueuedPacket(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpIdb_st *, EigrpPackDesc_st *);
void	EigrpDualLogXport(EigrpDualDdb_st *, U32, U32 *, void *, void *);
void	EigrpDualLog(EigrpDualDdb_st *, U32, EigrpNetEntry_st *, void *, void *);
void	EigrpDualDebugSendPacket(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpIdb_st *, EigrpPackDesc_st *, EigrpPktHdr_st *, U16);

#ifdef __cplusplus
}
#endif

#endif
