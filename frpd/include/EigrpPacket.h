#ifndef	_EIGRP_PACKET_H_
#define	_EIGRP_PACKET_H_

#ifdef	__cplusplus
extern	"C"  {
#endif

/* Probe packet data format.  No TLVs for simplicity. */
typedef struct EigrpProbe_{
	U32 sequence;		/* Sequence number */
}EigrpProbe_st, *EigrpProbe_pt;

/* The folowing typedef is for decoding the eigrp metric from a packet. */
typedef struct EigrpVmetricDecode_{
	EIGRP_PKT_METRIC
}EigrpVmetricDecode_st, *EigrpVmetricDecode_pt;

typedef struct EigrpFirstPkt_{
	U32 	pktLen;
	U8	pktBuf[116];
	void * pCirc;
	
}EigrpFirstPkt_st, *EigrpFirstPkt_pt;

#define	FPKT_LOCK_TAKE(lock)		\
			while((lock) == TRUE){	\
				EigrpPortSleep(1); 	\
				continue;	\
			};	\
			(lock)	= TRUE;

#define	FPKT_LOCK_GIVE(lock)		\
			(lock)	= FALSE;


S32	EigrpSocketInit(U32 *);
U32	EigrpDebugPacketType(S32, S32);
U32	EigrpDebugPacketDetailType(S32);
S32 EigrpProcPkt(U32 , U8 *, void *);
S32	EigrpRecv();
#ifdef _DC_
S32	EigrpPktCopyFromDc(U32 , U8 *, void *);  /*add by zm 130108*/
void	EigrpProcDcPkt() ;/*zhangming 130108*/
#endif// _DC_
S8	*EigrpPrintDest(S8, U8 *);
void	EigrpDebugPacketDetail(U32, EigrpPktHdr_st *, S32);
void *EigrpPacketData(EigrpPktHdr_st *);
S32	EigrpAddressDecode(EigrpIpMpDecode_st *, EigrpNetEntry_st *);
void	EigrpMetricDecode(EigrpVmetric_st *, EigrpVmetricDecode_st *);
S32	EigrpProcRoute(EigrpIpMpDecode_st *, S8 *, EigrpDualNewRt_st *, EigrpVmetricDecode_st *, 
								EigrpDualDdb_st *, U32, S32, U32, EigrpDualPeer_st *, void *);
void	EigrpRoutesDecode(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpGenTlv_st *, S32, U32);
void	EigrpProbeDecode(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpPktHdr_st *);
void	EigrpReceivedPktProcess(EigrpPdb_st *, EigrpPktHdr_st *, S32, U32 *, struct EigrpIntf_ *);
void	EigrpMetricEncode(EigrpDualDdb_st *, EigrpDualNdb_st *, EigrpIdb_st *, EigrpVmetricDecode_st *, S32, S32);
U32	EigrpAddItem(EigrpDualDdb_st *, EigrpIdb_st *, EigrpDualNdb_st *, void *, U32, S32, S8, EigrpDualPeer_st *);
EigrpPktHdr_st *EigrpBuildPacket(EigrpDualDdb_st *, EigrpDualPeer_st *, EigrpPackDesc_st *, S32 *);

#ifdef	__cplusplus
}
#endif

#endif	/* _EIGRP_PACKET_H_ */
