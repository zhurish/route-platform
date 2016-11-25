#ifndef _EIGRPD_H
#define _EIGRPD_H

#ifdef __cplusplus
extern"C"
{
#endif

struct EigrpSched_;

/* show topology detail */
#define	EIGRP_TOP_SUMMARY	1
#define	EIGRP_TOP_PASSIVE		2
#define	EIGRP_TOP_PENDING		3
#define	EIGRP_TOP_ACTIVE		4
#define	EIGRP_TOP_ZERO		5
#define	EIGRP_TOP_FS			6
#define	EIGRP_TOP_ALL			7

/* EIGRP route information. */
typedef struct EigrpRouteInfo_{
	S32	type;               
	S32	process;            

	EigrpInaddr_st nexthop;
	U32	from;

	/* Which interface does this route come from. */
	U32	ifindex;

	/* Composite Metric of this route. */
	U32	metric;             
	EigrpVmetric_st metricDetail;  

	U8	distance;        
	S32	isExternal;   
}EigrpRouteInfo_st;

#define	EIGRP_TOP_BUF_SIZE	2048
#define	EIGRP_SEC_PER_MIN		60
#define	EIGRP_SEC_PER_HOUR	(EIGRP_SEC_PER_MIN * 60)		/* 60*60 = 3600 */
#define	EIGRP_SEC_PER_DAY		(EIGRP_SEC_PER_HOUR * 24)	/* 60*60*24 = 86400 */

void	EigrpPrintThreadSerno(struct EigrpXmitThread_ *, struct EigrpDbgCtrl_  *);
S8	*EigrpPrintSendFlag(U16);
U32	EigrpShowTopology(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), struct EigrpDualDdb_ *, S32);
U32	EigrpShowNeighbors(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), struct EigrpDualDdb_ *,
							struct EigrpIntf_ *, S32);
void	EigrpPrintAnchoredSerno(struct EigrpXmitAnchor_ *, struct EigrpDbgCtrl_  *);
U32	EigrpShowInterface(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), struct EigrpDualDdb_ *,
							struct EigrpIntf_ *, S32);
U32	EigrpShowTraffic(U8 *, void *, U32, S32 (*)(U8 *, void *, U32, U32, U8 *), struct EigrpDualDdb_ *);
void	EigrpDistributeUpdateInterface(struct EigrpIntf_ *);
void	EigrpDistributeUpdateAll();
S32	EigrpConfigWriteEigrpNetwork(struct EigrpPdb_ *, S32, struct EigrpDbgCtrl_ *);
S8	*EigrpProto2str(U32);
S32	EigrpConfigWriteEigrpRedistribute(struct EigrpPdb_ *, S32, struct EigrpDbgCtrl_ *);
S32	EigrpConfigWriteEigrpOffsetList(struct EigrpPdb_ *, struct EigrpDbgCtrl_ *);
void	EigrpEventReadAdd();

#ifdef __cplusplus
}
#endif

#endif /* _EIGRPD_H */
