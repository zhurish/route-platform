#ifndef _EIGRP_CMD_H_
#define _EIGRP_CMD_H_

#ifdef __cplusplus
extern"C"
{
#endif

struct EigrpAuthInfo_;
struct EigrpRedis_;

enum	EigrpCmdType_{
	EIGRP_CMD_TYPE_GLOBAL,
	EIGRP_CMD_TYPE_INTF,
#ifdef _EIGRP_PLAT_MODULE
	/* 设置Router ID 参数 的命令 */
	EIGRP_CMD_TYPE_ROUTER_ID,
#endif// _EIGRP_PLAT_MODULE	

	EIGRP_CMD_TYPE_ROUTER,
	EIGRP_CMD_TYPE_AUTOSUM,
	EIGRP_CMD_TYPE_DEF_METRIC,
	EIGRP_CMD_TYPE_DISTANCE,
	EIGRP_CMD_TYPE_WEIGHT,
	EIGRP_CMD_TYPE_NETWORK,
	EIGRP_CMD_TYPE_REDIS,
	EIGRP_CMD_TYPE_NET_METRIC,
	EIGRP_CMD_TYPE_DEF_ROUTE_IN,
	EIGRP_CMD_TYPE_DEF_ROUTE_OUT,
	
	EIGRP_CMD_TYPE_INTF_AUTHKEYID,
	EIGRP_CMD_TYPE_INTF_AUTHKEY,
	EIGRP_CMD_TYPE_INTF_AUTHMODE,
	EIGRP_CMD_TYPE_INTF_BANDWIDTH_PERCENT,
	EIGRP_CMD_TYPE_INTF_BANDWIDTH,
	EIGRP_CMD_TYPE_INTF_DELAY,
	
#ifdef _EIGRP_UNNUMBERED_SUPPORT
	EIGRP_CMD_TYPE_INTF_UUAI_PARAM,/*cwf 20121225 for set uuai band and delay*/
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
	EIGRP_CMD_TYPE_INTF_HELLO,
	EIGRP_CMD_TYPE_INTF_HOLD,
	EIGRP_CMD_TYPE_INTF_SUM,
	EIGRP_CMD_TYPE_INTF_SUMNO,
	EIGRP_CMD_TYPE_INTF_SPLIT,
	EIGRP_CMD_TYPE_INTF_PASSIVE,
	EIGRP_CMD_TYPE_INTF_INVISIBLE,
	EIGRP_CMD_TYPE_INTF_OSIN,
	EIGRP_CMD_TYPE_INTF_OSOUT,
#ifdef _EIGRP_UNNUMBERED_SUPPORT	
	EIGRP_CMD_TYPE_INTF_UAI_P2MP,
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */	
#ifdef _EIGRP_VLAN_SUPPORT
	EIGRP_CMD_TYPE_INTF_UAI_P2MP_VLAN_ID,
#endif /* _EIGRP_VLAN_SUPPORT */
#ifdef _EIGRP_UNNUMBERED_SUPPORT	
	EIGRP_CMD_TYPE_INTF_UAI_P2MP_NEI,
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */
	EIGRP_CMD_TYPE_NEI,
	
	EIGRP_CMD_TYPE_SHOW_INTF,
	EIGRP_CMD_TYPE_SHOW_INTF_DETAIL,
	EIGRP_CMD_TYPE_SHOW_INTF_SINGLE,
	EIGRP_CMD_TYPE_SHOW_INTF_AS,
	EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE,
	EIGRP_CMD_TYPE_SHOW_INTF_AS_DETAIL,
	EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE_DETAIL,

	EIGRP_CMD_TYPE_SHOW_NEI,
	EIGRP_CMD_TYPE_SHOW_NEI_AS,
	EIGRP_CMD_TYPE_SHOW_NEI_DETAIL,
	EIGRP_CMD_TYPE_SHOW_NEI_INTF,
	EIGRP_CMD_TYPE_SHOW_NEI_INTF_DETAIL,
	EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF,
	EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF_DETAIL,
	EIGRP_CMD_TYPE_SHOW_NEI_AS_DETAIL,
	
	EIGRP_CMD_TYPE_SHOW_TOPO_ALL,
	EIGRP_CMD_TYPE_SHOW_TOPO_SUM,
	EIGRP_CMD_TYPE_SHOW_TOPO_ACT,
	EIGRP_CMD_TYPE_SHOW_TOPO_FS,
	EIGRP_CMD_TYPE_SHOW_TOPO_AS_ALL,
	EIGRP_CMD_TYPE_SHOW_TOPO_AS_ZERO,

	EIGRP_CMD_TYPE_SHOW_TRAFFIC,
	EIGRP_CMD_TYPE_SHOW_PROTOCOL,
	EIGRP_CMD_TYPE_SHOW_ROUTE,
	EIGRP_CMD_TYPE_SHOW_STRUCT,
	EIGRP_CMD_TYPE_SHOW_DEBUG,

	EIGRP_CMD_TYPE_DBG_SEND_UPDATE,
	EIGRP_CMD_TYPE_DBG_SEND_QUERY,
	EIGRP_CMD_TYPE_DBG_SEND_REPLY,
	EIGRP_CMD_TYPE_DBG_SEND_HELLO,
	EIGRP_CMD_TYPE_DBG_SEND_ACK	,
	EIGRP_CMD_TYPE_DBG_SEND,
	
	EIGRP_CMD_TYPE_DBG_RECV_UPDATE,
	EIGRP_CMD_TYPE_DBG_RECV_QUERY,
	EIGRP_CMD_TYPE_DBG_RECV_REPLY,
	EIGRP_CMD_TYPE_DBG_RECV_HELLO,
	EIGRP_CMD_TYPE_DBG_RECV_ACK,
	EIGRP_CMD_TYPE_DBG_RECV,
	
	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_UPDATE,
	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_QUERY,
	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_REPLY,
	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_HELLO,
	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_ACK,
	EIGRP_CMD_TYPE_DBG_PACKET_DETAIL,
	
	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_UPDATE,
	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_QUERY,
	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_REPLY,
	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_HELLO,
	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_ACK,
	EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL,
	
	EIGRP_CMD_TYPE_DBG_PACKET,
	EIGRP_CMD_TYPE_DBG_EVENT,
	EIGRP_CMD_TYPE_DBG_TIMER,
	EIGRP_CMD_TYPE_DBG_TASK,
	EIGRP_CMD_TYPE_DBG_ROUTE,
	EIGRP_CMD_TYPE_DBG_INTERNAL,
	EIGRP_CMD_TYPE_DBG_ALL,

	EIGRP_CMD_TYPE_NO_DBG_SEND_UPDATE,
	EIGRP_CMD_TYPE_NO_DBG_SEND_QUERY,
	EIGRP_CMD_TYPE_NO_DBG_SEND_REPLY,
	EIGRP_CMD_TYPE_NO_DBG_SEND_HELLO,
	EIGRP_CMD_TYPE_NO_DBG_SEND_ACK	,
	EIGRP_CMD_TYPE_NO_DBG_SEND,
	
	EIGRP_CMD_TYPE_NO_DBG_RECV_UPDATE,
	EIGRP_CMD_TYPE_NO_DBG_RECV_QUERY,
	EIGRP_CMD_TYPE_NO_DBG_RECV_REPLY,
	EIGRP_CMD_TYPE_NO_DBG_RECV_HELLO,
	EIGRP_CMD_TYPE_NO_DBG_RECV_ACK,
	EIGRP_CMD_TYPE_NO_DBG_RECV,
	
	EIGRP_CMD_TYPE_NO_DBG_PACKET,
	EIGRP_CMD_TYPE_NO_DBG_EVENT,
	EIGRP_CMD_TYPE_NO_DBG_TIMER,
	EIGRP_CMD_TYPE_NO_DBG_TASK,
	EIGRP_CMD_TYPE_NO_DBG_ROUTE,
	EIGRP_CMD_TYPE_NO_DBG_INTERNAL,

#ifdef  _EIGRP_PLAT_MODULE	
	EIGRP_CMD_TYPE_NO_DBG_ALL,
	EIGRP_CMD_TYPE_MAX
#else
	EIGRP_CMD_TYPE_NO_DBG_ALL
#endif	/* #ifdef _EIGRP_PLAT_MODULE */
	
};

#define	DEBUG_EIGRP_PACKET_SEND_UPDATE		0x00000001
#define	DEBUG_EIGRP_PACKET_SEND_QUERY		0x00000002
#define	DEBUG_EIGRP_PACKET_SEND_REPLY		0x00000004
#define	DEBUG_EIGRP_PACKET_SEND_HELLO		0x00000008
#define	DEBUG_EIGRP_PACKET_SEND_ACK			0x00000010
#define	DEBUG_EIGRP_PACKET_SEND				(DEBUG_EIGRP_PACKET_SEND_UPDATE |	\
												DEBUG_EIGRP_PACKET_SEND_QUERY | \
												DEBUG_EIGRP_PACKET_SEND_REPLY | \
												DEBUG_EIGRP_PACKET_SEND_HELLO | \
												DEBUG_EIGRP_PACKET_SEND_ACK)

#define	DEBUG_EIGRP_PACKET_RECV_UPDATE		0x00000100
#define	DEBUG_EIGRP_PACKET_RECV_QUERY		0x00000200
#define	DEBUG_EIGRP_PACKET_RECV_REPLY		0x00000400
#define	DEBUG_EIGRP_PACKET_RECV_HELLO		0x00000800
#define	DEBUG_EIGRP_PACKET_RECV_ACK			0x00001000
#define	DEBUG_EIGRP_PACKET_RECV				(DEBUG_EIGRP_PACKET_RECV_UPDATE |	\
												DEBUG_EIGRP_PACKET_RECV_QUERY |	\
												DEBUG_EIGRP_PACKET_RECV_REPLY |	\
												DEBUG_EIGRP_PACKET_RECV_HELLO |	\
												DEBUG_EIGRP_PACKET_RECV_ACK)
												
#define	DEBUG_EIGRP_PACKET_DETAIL_UPDATE	0x00010000		
#define	DEBUG_EIGRP_PACKET_DETAIL_QUERY	0x00020000		
#define	DEBUG_EIGRP_PACKET_DETAIL_REPLY		0x00040000	
#define	DEBUG_EIGRP_PACKET_DETAIL_HELLO		0x00080000	
#define	DEBUG_EIGRP_PACKET_DETAIL_ACK		0x00100000	
#define	DEBUG_EIGRP_PACKET_DETAIL			(DEBUG_EIGRP_PACKET_DETAIL_UPDATE |	\
												DEBUG_EIGRP_PACKET_DETAIL_QUERY |	\
												DEBUG_EIGRP_PACKET_DETAIL_REPLY |	\
												DEBUG_EIGRP_PACKET_DETAIL_HELLO |	\
												DEBUG_EIGRP_PACKET_DETAIL_ACK)

#define	DEBUG_EIGRP_PACKET					(DEBUG_EIGRP_PACKET_SEND |	\
												DEBUG_EIGRP_PACKET_RECV |\
												DEBUG_EIGRP_PACKET_DETAIL)
#define	DEBUG_EIGRP_EVENT				0x02000000
#define	DEBUG_EIGRP_TIMER				0x04000000
#define	DEBUG_EIGRP_TASK				0x08000000
#define	DEBUG_EIGRP_ROUTE			0x10000000
#define	DEBUG_EIGRP_INTERNAL			0x20000000
#define	DEBUG_EIGRP_OTHER			0x40000000
#define	DEBUG_EIGRP_ALL				0xffffffff

typedef	struct EigrpConf_{
	struct EigrpConfAs_	*pConfAs;

	struct EigrpConfIntf_	*pConfIntf;
}EigrpConf_st, *EigrpConf_pt;

typedef	struct EigrpConfAsDistance_{
	U32	interDis;
	U32	exterDis;
}EigrpConfAsDistance_st, *EigrpConfAsDistance_pt;

typedef	struct EigrpConfAsMetricWeight_{
	U32	k1, k2, k3, k4, k5;
}EigrpConfAsMetricWeight_st, *EigrpConfAsMetricWeight_pt;

typedef	struct EigrpConfAsNet_{
	struct EigrpConfAsNet_		*next;
	struct EigrpConfAsNet_		*prev;
	
	U32	ipNet;
	U32	ipMask;
}EigrpConfAsNet_st, *EigrpConfAsNet_pt;

typedef	struct EigrpConfAsNei_{
	struct EigrpConfAsNei_	*next;
	struct EigrpConfAsNei_	*prev;

	U32	ipAddr;
}EigrpConfAsNei_st, *EigrpConfAsNei_pt;

typedef	struct EigrpConfAsRedis_{
	struct EigrpConfAsRedis_	*next;
	struct EigrpConfAsRedis_	*prev;

	U8	protoName[64];
	U32	srcProc;
}EigrpConfAsRedis_st, *EigrpConfAsRedis_pt;

typedef	struct EigrpConfAs_{
	struct EigrpConfAs_	*prev;
	struct EigrpConfAs_	*next;

	U32	asNum;
	S32	*autoSum;
	struct EigrpVmetric_		*defMetric;
	struct EigrpConfAsDistance_	*distance;
	struct EigrpConfAsMetricWeight_	*metricWeight;
	struct EigrpConfAsNet_		*network;
	struct EigrpConfAsNei_		*nei;
	struct EigrpConfAsRedis_	*redis;

	S32	*defRouteIn, *defRouteOut;
}EigrpConfAs_st, *EigrpConfAs_pt;

typedef	struct EigrpConfIntfPassive_{
	struct EigrpConfIntfPassive_	*prev;
	struct EigrpConfIntfPassive_	*next;

	U32	asNum;
}EigrpConfIntfPassive_st, *EigrpConfIntfPassive_pt;

typedef	struct EigrpConfIntfInvisible_{
	struct EigrpConfIntfInvisible_	*prev;
	struct EigrpConfIntfInvisible_	*next;

	U32	asNum;
}EigrpConfIntfInvisible_st, *EigrpConfIntfInvisible_pt;

typedef	struct EigrpConfIntfAuthkey_{
	struct EigrpConfIntfAuthkey_	*next;
	struct EigrpConfIntfAuthkey_	*prev;

	U8	auth[32];
	U32	asNum;
}EigrpConfIntfAuthkey_st, *EigrpConfIntfAuthkey_pt;

typedef	struct EigrpConfIntfAuthid_{
	struct EigrpConfIntfAuthid_	*next;
	struct EigrpConfIntfAuthid_	*prev;

	U8	authId;
	U32	asNum;
}EigrpConfIntfAuthid_st, *EigrpConfIntfAuthid_pt;

typedef	struct EigrpConfIntfAuthMode_{
	struct EigrpConfIntfAuthMode_	*next;
	struct EigrpConfIntfAuthMode_	*prev;

	S32	authMode;
	U32	asNum;
}EigrpConfIntfAuthMode_st, *EigrpConfIntfAuthMode_pt;

typedef	struct EigrpConfIntfBw_{
	struct EigrpConfIntfBw_	*next;
	struct EigrpConfIntfBw_	*prev;

	U32	bandwidth;
	U32	asNum;
}EigrpConfIntfBw_st, *EigrpConfIntfBw_pt;

typedef	struct EigrpConfIntfBandwidth_{
	struct EigrpConfIntfBandwidth_	*next;
	struct EigrpConfIntfBandwidth_	*prev;

	U32	bandwidth;
	U32	asNum;
}EigrpConfIntfBandwidth_st, *EigrpConfIntfBandwidth_pt;

typedef	struct EigrpConfIntfDelay_{
	struct EigrpConfIntfDelay_	*next;
	struct EigrpConfIntfDelay_	*prev;

	U32	delay;
	U32	asNum;
}EigrpConfIntfDelay_st, *EigrpConfIntfDelay_pt;

typedef	struct EigrpConfIntfHello_{
	struct EigrpConfIntfHello_	*next;
	struct EigrpConfIntfHello_	*prev;

	U32	asNum;
	U32	hello;
}EigrpConfIntfHello_st, *EigrpConfIntfHello_pt;

typedef	struct EigrpConfIntfHold_{
	struct EigrpConfIntfHold_	*next;
	struct EigrpConfIntfHold_	*prev;

	U32	asNum;
	U32	hold;
}EigrpConfIntfHold_st, *EigrpConfIntfHold_pt;

typedef	struct EigrpConfIntfSplit_{
	struct EigrpConfIntfSplit_	*next;
	struct EigrpConfIntfSplit_	*prev;

	U32	asNum;
	S32	split;
}EigrpConfIntfSplit_st, *EigrpConfIntfSplit_pt;

typedef	struct EigrpConfIntfSum_{
	struct EigrpConfIntfSum_	*next;
	struct EigrpConfIntfSum_	*prev;

	U32	asNum;
	U32	ipNet;
	U32	ipMask;
}EigrpConfIntfSum_st, *EigrpConfIntfSum_pt;

typedef	struct EigrpConfIntf_{
	struct EigrpConfIntf_	*prev;
	struct EigrpConfIntf_	*next;

	U8	ifName[128];
	U32	index;

	struct EigrpConfIntfPassive_	*passive;
	struct EigrpConfIntfInvisible_	*invisible;
	struct EigrpConfIntfAuthkey_	*authkey;
	struct EigrpConfIntfAuthid_		*authid;
	struct EigrpConfIntfAuthMode_	*authmode;
	struct EigrpConfIntfBw_		*bandwidth;
	struct EigrpConfIntfBandwidth_		*bw;
	struct EigrpConfIntfDelay_		*delay;
	struct EigrpConfIntfHello_		*hello;
	struct EigrpConfIntfHold_		*hold;
	struct EigrpConfIntfSplit_		*split;
	struct EigrpConfIntfSum_		*summary;

	S32	IsSei;
}EigrpConfIntf_st, *EigrpConfIntf_pt;

typedef struct	EigrpCmd_{
	struct EigrpCmd_ *next;

	U32	type;
	S32	noFlag;
	U32	asNum;
	U8	ifName[256];
	U8	name[256];

	U32	u32Para1;
	U32	u32Para2;
	U32	u32Para3;
	U32	u32Para4;
	U32	u32Para5;
	
	void	*vsPara1;
	void	*vsPara2;

}EigrpCmd_st, *EigrpCmd_pt;

void	EigrpConfigIntfApply(U8 *);
void	EigrpConfigIntfRapplyAll();
S32	EigrpConfigRouter(S32, U32);
S32	EigrpConfigAutoSummary(S32, U32);
S32	EigrpConfigDefMetric(S32, U32, struct EigrpVmetric_ *);
S32	EigrpConfigDistance(S32, U32, U32, U32);
S32	EigrpConfigMetricWeight(S32, U32, U32, U32, U32, U32, U32);
S32	EigrpConfigNetwork(S32, U32, U32, U32);
S32	EigrpConfigIntfPassive(S32, U8 *, U32);
S32	EigrpConfigNeighbor(S32, U32, U32);
S32	EigrpConfigIntfAuthKey(S32, U8 *, U32, U8 *);
S32	EigrpConfigIntfAuthid(S32, U8 *, U32 , U32);
S32	EigrpConfigIntfAuthMode(S32, U8 *, U32);
S32	EigrpConfigIntfBandwidth(S32, U8 *, U32, U32);
S32	EigrpConfigIntfBw(S32, U8 *, U32, U32);
S32	EigrpConfigIntfDelay(S32, U8 *, U32, U32);
S32	EigrpConfigIntfHello(S32, U8 *, U32, U32);
S32	EigrpConfigIntfHold(S32, U8 *, U32, U32);
S32	EigrpConfigIntfSplit(S32, U8 *, U32);
S32	EigrpConfigIntfSummary(S32, U8 *, U32, U32, U32);
S32	EigrpConfigAsRedis(S32, U32, struct EigrpRedis_ *, U32);
S32	EigrpConfigAsDefRouteIn(S32, U32);
S32	EigrpConfigAsDefRouteOut(S32, U32);
void	EigrpCmdProc(struct EigrpCmd_ *);

S32	EigrpCmdApiAuthKeyStr(S32, U32, U8 *, U8 *);
S32	EigrpCmdApiAuthKeyid(S32, U32, U8 *, U32);
S32	EigrpCmdApiAuthMd5mode(S32, U32, U8 *);
S32	EigrpCmdApiBwPercent(S32, U32, U8 *, U32);
S32	EigrpCmdApiBandwidth(S32, U32, U8 *, U32);
#ifdef _EIGRP_UNNUMBERED_SUPPORT
S32	EigrpCmdApiUUaiParam(U32 asNum, U8 *ifName);
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */
#ifdef _EIGRP_UNNUMBERED_SUPPORT
S32	EigrpCmdApiUaiP2mpPort(S32 noFlag, U8 *seiName);
S32	EigrpCmdApiUaiP2mpPort_vlanid(S32 noFlag, U32 vlan_id);
S32	EigrpCmdApiUaiP2mpPort_Nei(S32 noFlag, U32 nei, U32 vlan_id);
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
S32	EigrpCmdApiDelay(S32 noFlag, U32 asNum, U8 *ifName, U32 delay);
S32	EigrpCmdApiHelloInterval(S32, U32, U8 *, U32);
S32	EigrpCmdApiHoldtime(S32, U32, U8 *, U32);
S32	EigrpCmdApiSplitHorizon(S32, U32, U8 *);
S32	EigrpCmdApiSummaryAddress(S32, U32, U8 *, U32, U32);
#ifdef _EIGRP_UNNUMBERED_SUPPORT
S32	EigrpCmdApiUaiP2mpPort(S32, U8 *);
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
S32	EigrpCmdApiRouteTypeAtoi(S8 *);
S32	EigrpCmdApiRedistType(S32, U32, U8 *, U32, U32);
S32	EigrpCmdApiNetMetric(S32, U32, void *);
S32	EigrpCmdApiRedistTypeRoutemap(S32,  U32, U8 *, U8 *);
S32	EigrpCmdApiRedistTypeMetric(S32, U32, U8 *, U32, U32, U32, U32, U32);
S32	EigrpCmdApiRedistTypeMetricRoutemap(U32, U8 *, U32, U32, U32, U32, U32, U8 *);
S32	EigrpCmdApiDefaultRouteIn(S32, U32);
S32	EigrpCmdApiDefaultRouteOut(S32, U32);
S32	EigrpCmdApiRouterEigrp(S32, U32);
S32	EigrpCmdApiAutosummary(S32, U32);
S32	EigrpCmdApiDefaultMetric(S32, U32, U32, U32, U32, U32, U32);
S32	EigrpCmdApiDistance(S32, U32, U32, U32);
S32	EigrpCmdApiMetricWeights(S32, U32, U32, U32, U32, U32, U32);
S32	EigrpCmdApiNetwork(S32, U32, U32, U32);
S32	EigrpCmdApiPassiveInterface(S32, U32, U8 *);
S32	EigrpCmdApiInvisibleInterface(S32, U32, U8 *);
S32	EigrpCmdApiNeighbor(S32, U32, U32);
S32	EigrpCmdApiShowDebuggingEigrpByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowDebuggingEigrp();
S32	EigrpCmdApiDebugPacketByTerm(U8 *, void *, U32, S32(*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacket();
S32	EigrpCmdApiDebugPacketDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketDetail();
S32	EigrpCmdApiNoDebugPacketDetailByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketDetail();
S32	EigrpCmdApiNoDebugPacketByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacket();
S32	EigrpCmdApiDebugPacketSendByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketSend();
S32	EigrpCmdApiDebugPacketSendUpdateByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketSendUpdate();
S32	EigrpCmdApiDebugPacketSendQueryByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketSendQuery();
S32	EigrpCmdApiDebugPacketSendReplyByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketSendReply();
S32	EigrpCmdApiDebugPacketSendHelloByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketSendHello();
S32	EigrpCmdApiDebugPacketSendAckByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketSendAck();
//系统不存在这个函数，zhurish 2016-1-22 屏蔽
//S32	EigrpCmdApiDebugPacketSendDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSendByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSend();
S32	EigrpCmdApiNoDebugPacketSendUpdateByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSendUpdate();
S32	EigrpCmdApiNoDebugPacketSendQueryByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSendQuery();
S32	EigrpCmdApiNoDebugPacketSendReplyByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSendReply();
S32	EigrpCmdApiNoDebugPacketSendHelloByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSendHello();
S32	EigrpCmdApiNoDebugPacketSendAckByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketSendAck();
S32	EigrpCmdApiDebugPacketRecvByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketRecv();
S32	EigrpCmdApiDebugPacketRecvUpdateByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketRecvUpdate();
S32	EigrpCmdApiDebugPacketRecvQueryByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketRecvQuery();
S32	EigrpCmdApiDebugPacketRecvReplyByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketRecvReply();
S32	EigrpCmdApiDebugPacketRecvHelloByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketRecvHello();
S32	EigrpCmdApiDebugPacketRecvAckByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketRecvAck();
S32	EigrpCmdApiNoDebugPacketRecvByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketRecv();
S32	EigrpCmdApiNoDebugPacketRecvUpdateByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketRecvUpdate();
S32	EigrpCmdApiNoDebugPacketRecvQueryByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketRecvQuery();
S32	EigrpCmdApiNoDebugPacketRecvReplyByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketRecvReply();
S32	EigrpCmdApiNoDebugPacketRecvHelloByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketRecvHello();
S32	EigrpCmdApiNoDebugPacketRecvAckByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketRecvAck();
S32	EigrpCmdApiDebugPacketDetailUpdateByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketDetailUpdate();
S32	EigrpCmdApiDebugPacketDetailQueryByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketDetailQuery();
S32	EigrpCmdApiDebugPacketDetailReplyByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketDetailReply();
S32	EigrpCmdApiDebugPacketDetailHelloByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketDetailHello();
S32	EigrpCmdApiDebugPacketDetailAckByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugPacketDetailAck();
S32	EigrpCmdApiNoDebugPacketDetailUpdateByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketDetailUpdate();
S32	EigrpCmdApiNoDebugPacketDetailQueryByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketDetailQuery();
S32	EigrpCmdApiNoDebugPacketDetailReplyByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketDetailReply();
S32	EigrpCmdApiNoDebugPacketDetailHelloByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketDetailHello();
S32	EigrpCmdApiNoDebugPacketDetailAckByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugPacketDetailAck();
S32	EigrpCmdApiNoDebugPacketRecvDetailByTerm(U8 *, void *, U32 , S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugEventByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugEvent();
S32	EigrpCmdApiNoDebugEventByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugEvent();
S32	EigrpCmdApiNoDebugPacketByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugTimer();
S32	EigrpCmdApiNoDebugTimerByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugTimer();
S32	EigrpCmdApiDebugRouteByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugRoute();
S32	EigrpCmdApiNoDebugRouteByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugRoute();
S32	EigrpCmdApiDebugTaskByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugTask();
S32	EigrpCmdApiNoDebugTaskByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugTask();
S32	EigrpCmdApiDebugAllByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugAll();
S32	EigrpCmdApiNoDebugAllByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugAll();
S32	EigrpCmdApiDebugInternalByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiDebugInternal();
S32	EigrpCmdApiNoDebugInternalByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiNoDebugInternal();
S32	EigrpCmdApiShowIntfByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowIntf();
S32	EigrpCmdApiShowIntfDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowIntfDetail();
S32	EigrpCmdApiShowIntfIfByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U8 *);
S32	EigrpCmdApiShowIntfIf(U8 *);
S32	EigrpCmdApiShowIntfAsByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32);
S32	EigrpCmdApiShowIntfAs(U32);
S32	EigrpCmdApiShowIntfAsIfByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32, U8 *);
S32	EigrpCmdApiShowIntfAsIf(U32, U8 *);
S32	EigrpCmdApiShowIntfAsDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32);
S32	EigrpCmdApiShowIntfAsDetail(U32);
S32	EigrpCmdApiShowIntfAsIfDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32, U8 *);
S32	EigrpCmdApiShowIntfAsIfDetail(U32, U8 *);
S32	EigrpCmdApiShowNeiByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowNei();
S32	EigrpCmdApiShowNeiAsByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32);
S32	EigrpCmdApiShowNeiAs(U32);
S32	EigrpCmdApiShowNeiDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowNeiDetail();
S32	EigrpCmdApiShowNeiIfByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U8 *);
S32	EigrpCmdApiShowNeiIf(U8 *);
S32	EigrpCmdApiShowNeiIfDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U8 *);
S32	EigrpCmdApiShowNeiIfDetail(U8 *);
S32	EigrpCmdApiShowNeiAsIfByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32, U8 *);
S32	EigrpCmdApiShowNeiAsIf(U32, U8 *);
S32	EigrpCmdApiShowNeiAsIfDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32, U8 *);
S32	EigrpCmdApiShowNeiAsIfDetail(U32, U8 *);
S32	EigrpCmdApiShowNeiAsDetailByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32);
S32	EigrpCmdApiShowNeiAsDetail(U32);
S32	EigrpCmdApiShowTopoByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowTopo();
S32	EigrpCmdApiShowTopoSummaryByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowTopoSummary();
S32	EigrpCmdApiShowTopoActiveByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowTopoActive();
S32	EigrpCmdApiShowTopoFeasibleByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowTopoFeasible();
S32	EigrpCmdApiShowTopoAsByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), U32);
S32	EigrpCmdApiShowTopoAs(U32);
S32	EigrpCmdApiShowTopoZeroByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowTopoZero();
S32	EigrpCmdApiShowTrafficByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowTraffic();
S32	EigrpCmdApiShowProtoByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowProto();
S32	EigrpCmdApiShowRouteByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowRoute();
S32	EigrpCmdApiShowStructByTerm(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *));
S32	EigrpCmdApiShowStruct();
void	EigrpCmdEnsureSize(U8 **, U32 *, U32, U32);
void	EigrpCmdApiBuildRunIntfMode(U8 **, U32 *, U32 *, S32 *, 	U32, U8 *);
void	EigrpCmdApiBuildRunConfMode(U8 **, U32 *, U32 *, S32 *, void *);
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
void	EigrpCmdApiBuildRunIntfMode_Router(U8 **ppBuf, U32 *bufLen, U32 *usedLen, U8 *circName);
void	EigrpCmdApiBuildRunConfMode_Router(U8 **ppBuf, U32 *bufLen, U32 *usedLen);
#endif /* #if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER) */

#ifdef _EIGRP_PLAT_MODULE
int	zebraEigrpCmdApiRouterId(U32 , U32 , U32 );
#endif /* _EIGRP_PLAT_MODULE */



#ifdef __cplusplus
}
#endif

#endif
