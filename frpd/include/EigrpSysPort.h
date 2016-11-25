#ifndef	_EIGRP_SYSPORT_H_
#define	_EIGRP_SYSPORT_H_

#ifdef __cplusplus 
extern "C"{
#endif

#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS	//·��ƽ̨֧����Ҫ�����ݽṹ
struct prefix;
struct rib;
struct nsm_msg_header;
struct cli;
#endif//EIGRP_PLAT_ZEBOS
#ifdef EIGRP_PLAT_ZEBRA	//·��ƽ̨֧����Ҫ�����ݽṹ
#endif//EIGRP_PLAT_ZEBRA
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)

#ifndef	TRUE
#define	TRUE 	(1)
#endif

#ifndef	FALSE
#define	FALSE 	(0)
#endif

#define	FAILURE		(-1)
#define	SUCCESS	(1)

#ifndef	NULL
#define	NULL        (void *)0
#endif

#ifndef	U8
	typedef unsigned char		U8;
#endif

#ifndef	S8
	typedef char				S8;
#endif

#ifndef	U16
	typedef unsigned short		U16;
#endif

#ifndef	S16
	typedef short				S16;
#endif

#ifndef	U32
	typedef unsigned int		U32;
#endif

#ifndef	S32
	typedef int				S32;
#endif

#ifndef	BIT_SET
#define	BIT_SET(f, b)			((f) |= b)
#endif

#ifndef	BIT_RESET
#define	BIT_RESET(f, b)			((f) &= ~(b))
#endif

#ifndef	BIT_FLIP
#define	BIT_FLIP(f, b)			((f) ^= (b))
#endif

#ifndef	BIT_TEST
#define	BIT_TEST(f, b)			((f) & (b))
#endif

#ifndef	BIT_MATCH
#define	BIT_MATCH(f, b)(((f) & (b)) == (b))
#endif

#ifndef	BIT_COMPARE
#define	BIT_COMPARE(f, b1, b2)  (((f) & (b1)) == b2)
#endif

#ifndef	BIT_MASK_MATCH
#define	BIT_MASK_MATCH(f, g, b)(!(((f) ^ (g)) & (b)))
#endif

#ifndef	MAX
#define	MAX(a,b)	(((a)>(b))?(a):(b))
#endif

#ifndef	MIN
#define	MIN(a,b)		(((a)>(b))?(b):(a))
#endif

#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#ifndef _EIGRP_PLAT_MODULE
	#ifndef _INTEL_
	#define	_INTEL_
	#endif
	
	#ifndef	LITTLE_ENDIAN
	#define	LITTLE_ENDIAN 0
	#endif
	
	#ifndef BIG_ENDIAN
	#define	BIG_ENDIAN 1
	#endif 
	
	#ifdef _INTEL_
	#define	BYTE_ORDER      LITTLE_ENDIAN
	#else
	#define	BYTE_ORDER      BIG_ENDIAN
	#endif
	
	#ifdef HTONL
	#undef	HTONL
	#endif
	
	#ifdef HTONS
	#undef	HTONS
	#endif 
	
	#ifdef NTOHL
	#undef	NTOHL
	#endif
	
	#ifdef NTOHS
	#undef	NTOHS
	#endif 

	#if	(BYTE_ORDER	== LITTLE_ENDIAN)
		#define HTONL(x)	((((x) & 0x000000ff) << 24) | \
					(((x) & 0x0000ff00) <<  8) | \
					(((x) & 0x00ff0000) >>  8) | \
					(((x) & 0xff000000) >> 24))
	
		#define HTONS(x)	((((x) & 0x00ff) << 8) | \
					(((x) & 0xff00) >> 8))
	#else
		#define HTONL(x)	x
		#define HTONS(x)	x
	#endif
	#define	NTOHL(ip)	HTONL((ip))
	#define	NTOHS(ip)	HTONS((ip))
		
#else//_EIGRP_PLAT_MODULE
	
	#undef HTONL
	#define	HTONL	htonl
	//#endif
	#undef NTOHL
	#define	NTOHL	ntohl
	//#endif
	#undef HTONS
	#define	HTONS	htons
	//#endif
	#undef NTOHS
	#define	NTOHS	ntohs
	//#endif
	#ifndef BYTE_ORDER
		#define BYTE_ORDER	_BYTE_ORDER//(_BYTE_ORDER/_BIG_ENDIAN/_LITTLE_ENDIAN)
	#endif//BYTE_ORDER
	#ifndef BIG_ENDIAN
		#define BIG_ENDIAN	_BIG_ENDIAN	/* least-significant byte first (vax, pc) */
	#endif//BIG_ENDIAN
	#ifndef LITTLE_ENDIAN
		#define LITTLE_ENDIAN	_LITTLE_ENDIAN	/* most-significant byte first (IBM, net) */
	#endif//LITTLE_ENDIAN
	
#endif//_EIGRP_PLAT_MODULE	

	#define	_PACK_STRUCT __atribute__((packed))
	
#else//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	
#ifndef _EIGRP_PLAT_MODULE	
	extern U16 NTOHS(U16);
	extern U32 NTOHL(U32);

	#ifndef HTONL
	#define	HTONL(ip)	NTOHL((ip))
	#endif

	#ifndef HTONS
	#define	HTONS(ip)	NTOHS((ip))
	#endif 

	#ifndef	LITTLE_ENDIAN
	#define	LITTLE_ENDIAN 0
	#endif

	#ifndef BIG_ENDIAN
	#define	BIG_ENDIAN 1
	#endif 

	#ifndef _INTEL_
	#define	_INTEL_
	#endif

	#ifdef _INTEL_
	#define	BYTE_ORDER      LITTLE_ENDIAN
	#else
	#define	BYTE_ORDER      BIG_ENDIAN
	#endif	
	
#else//_EIGRP_PLAT_MODULE
	
	#undef HTONL
	#define	HTONL	htonl
	//#endif
	#undef NTOHL
	#define	NTOHL	ntohl
	//#endif
	#undef HTONS
	#define	HTONS	htons
	//#endif
	#undef NTOHS
	#define	NTOHS	ntohs
	//#endif
	#ifndef BYTE_ORDER
		#define BYTE_ORDER	_BYTE_ORDER//(_BYTE_ORDER/_BIG_ENDIAN/_LITTLE_ENDIAN)
	#endif//BYTE_ORDER
	#ifndef BIG_ENDIAN
		#define BIG_ENDIAN	_BIG_ENDIAN	/* least-significant byte first (vax, pc) */
	#endif//BIG_ENDIAN
	#ifndef LITTLE_ENDIAN
		#define LITTLE_ENDIAN	_LITTLE_ENDIAN	/* most-significant byte first (IBM, net) */
	#endif//LITTLE_ENDIAN

#endif//_EIGRP_PLAT_MODULE	
	
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)

struct EigrpIdb_;
struct EigrpIntf_;
struct EigrpDualPeer_;
struct EigrpPktHdr_;
struct EigrpPdb_;
struct EigrpDualDdb_;
struct EigrpDbgCtrl_; 

#ifdef	UNUSED
#undef	UNUSED
#endif	/* UNUSED */

#define	UNUSED(x) (x=x)
#define	RR_FUNC
#ifndef	RR_FUNC	
#define	EIGRP_FUNC_ENTER(a)		\
	if(gpEigrp->funcCnt >= 0 && gpEigrp->funcCnt < gpEigrp->funcMax - 1){	\
		sprintf(gpEigrp->funcStack[gpEigrp->funcCnt], "%s", #a);	\
	}	
#define	EIGRP_FUNC_LEAVE(a)		gpEigrp->funcCnt++;

#else//RR_FUNC
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	#define	EIGRP_FUNC_ENTER(a)		\
	if(gpEigrp){\
		if(gpEigrp->funcCnt >= gpEigrp->funcMax - 1){	\
				gpEigrp->funcCnt	= 0;\
		}	\
		sprintf(gpEigrp->funcStack[gpEigrp->funcCnt], "%s %d", #a,  __LINE__);	\
		gpEigrp->funcCnt++;	\
	}
	#define	EIGRP_FUNC_LEAVE(a)		EIGRP_FUNC_ENTER(a);

#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)		
	#define	EIGRP_FUNC_ENTER(a)		\
	if(gpEigrp){\
		if(gpEigrp->funcCnt >= gpEigrp->funcMax - 1){	\
				gpEigrp->funcCnt	= 0;\
		}	\
		sprintf(gpEigrp->funcStack[gpEigrp->funcCnt], "%s %d", #a,  __LINE__);	\
		gpEigrp->funcCnt++;	\
	}
	#define	EIGRP_FUNC_LEAVE(a)		EIGRP_FUNC_ENTER(a);
#elif	(EIGRP_OS_TYPE == EIGRP_OS_LINUX)		
	#define	EIGRP_FUNC_ENTER(a)
	#define	EIGRP_FUNC_LEAVE(a)
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#endif	//RR_FUNC

#ifndef	RR_FUNC	
#else//RR_FUNC
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	#define	EIGRP_RR_FUNC_ENTER(a)		\
	if(gpEigrp){\
		if(gpEigrp->funcCnt >= gpEigrp->funcMax - 1){	\
				gpEigrp->funcCnt	= 0;\
		}	\
		sprintf(gpEigrp->funcStack[gpEigrp->funcCnt], "%s %d", #a,  __LINE__);	\
		gpEigrp->funcCnt++;	\
	}
	#define	EIGRP_RR_FUNC_LEAVE(a)		EIGRP_FUNC_ENTER(a);

#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)		
	#define	EIGRP_RR_FUNC_ENTER(a)		\
	if(gpEigrp){\
		if(gpEigrp->funcCnt >= gpEigrp->funcMax - 1){	\
				gpEigrp->funcCnt	= 0;\
		}	\
		sprintf(gpEigrp->funcStack[gpEigrp->funcCnt], "%s %d", #a,  __LINE__);	\
		gpEigrp->funcCnt++;	\
	}
	#define	EIGRP_RR_FUNC_LEAVE(a)		EIGRP_RR_FUNC_ENTER(a);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_PIL)
#endif	//RR_FUNC

#if(EIGRP_OS_TYPE != EIGRP_OS_PIL)
#ifndef sprintf_s
#define sprintf_s(_buf_, _bufLen_, ...) sprintf(_buf_, __VA_ARGS__)
#endif//sprintf_s
#endif//(EIGRP_OS_TYPE != EIGRP_OS_PIL)

#define	EIGRP_SHOW_SPRINTF		sprintf_s
#define	EIGRP_TRC(_type_, ...) \
		EIGRP_SHOW_SPRINTF(gpEigrp->fixLenStrBuf, sizeof(gpEigrp->fixLenStrBuf), __VA_ARGS__);\
		EigrpUtilOutputBuf((_type_), gpEigrp->fixLenStrBuf);

#if 1
#if(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#undef EIGRP_SHOW_SPRINTF
#undef EIGRP_TRC


#define	EIGRP_SHOW_SPRINTF		printf
/*
#define	EIGRP_TRC(_type_, format, args...) \
		fprintf(stderr, "%s:",__func__); \
		fprintf(stderr, format, ##args); \
		fprintf(stderr, "\n");
*/
static inline int EIGRP_TRC (int type, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fflush (stderr);
  //vzlog (zl, priority, format, args);
  return 0;
}

#endif//(EIGRP_OS_TYPE != EIGRP_OS_PIL)
#endif

#define	EIGRP_NAME_SIZE		128
#define	EIGRP_NEW_LINE_STR	"\n"

#define	IPEIGRP_PROT    			88
#define	EIGRP_LOCAL_PORT    			6100

#if 0
#define	EIGRP_VALID_CHECK_VOID()	\
			static	U32	validCheckSec = 0;	\
			if(validCheckSec == 0){	\
				validCheckSec	= EigrpPortGetTimeSec();	\
			}	\
			if((EigrpPortGetTimeSec() - validCheckSec) > (60 * 60 * 24)){	\
				return;	\
			}	
#define	EIGRP_VALID_CHECK(a)	\
			static	U32	validCheckSec = 0;	\
			if(validCheckSec == 0){	\
				validCheckSec	= EigrpPortGetTimeSec();	\
			}	\
			if((EigrpPortGetTimeSec() - validCheckSec) > (60 * 60 * 24)){	\
				return a;	\
			}
#else// 0
#define	EIGRP_VALID_CHECK_VOID()
#define	EIGRP_VALID_CHECK(a)
#endif // 0

#ifdef INCLUDE_SATELLITE_RESTRICT
typedef struct EigrpPktRecord_{
	U32		sec;
	U32		msec;
	U32		ipAddr;
}EigrpPktRecord_st, *EigrpPktRecord_pt;
#endif//INCLUDE_SATELLITE_RESTRICT

typedef struct EigrpIpHdr_{
#if BYTE_ORDER == LITTLE_ENDIAN
	U8	hdrLen:4,		/* header length */
	version:4;			/* version */
#endif
#if BYTE_ORDER == BIG_ENDIAN
	U8	version:4,		/* version */
	hdrLen:4;			/* header length */
#endif
	U8	tos;				/* type of service */

	U16	length;			/* total length */
	U16	id;				/* identification */
	U16	offset;			/* fragment offset field */
	U8	ttl;				/* time to live */
	U8	protocol;		/* protocol */
	U16	chkSum;		/* checksum */
	U32	srcIp;
	U32	dstIp; 	
	/* total 20 byte.	*/
}EigrpIpHdr_st, *EigrpIpHdr_pt;

typedef struct EigrpUdpHdr_{
	U16	srcPort;		/* source port */
	U16	dstPort;		/* destination port */
	U16	length;		/* udp length */
	U16	chkSum;		/* udp checksum */
}EigrpUdpHdr_st, *EigrpUdpHdr_pt;

typedef struct EigrpInaddr_{
	U32	s_addr;
}EigrpInaddr_st;

typedef	struct EigrpSockIn_{
	U16	sin_family;	/* Address family */
	U16	sin_port;	/* Port number */
	struct EigrpInaddr_	sin_addr;	/* Internet address */

	/* Pad to size of `struct sockaddr'. */
	U8		pad[1];
}EigrpSockIn_st, *EigrpSockIn_pt;

enum	EigrpRouteType_{
	EIGRP_ROUTE_SYSTEM,
	EIGRP_ROUTE_KERNEL,
	EIGRP_ROUTE_CONNECT,
	EIGRP_ROUTE_STATIC,
	EIGRP_ROUTE_OSPF,
	EIGRP_ROUTE_RIP,
	EIGRP_ROUTE_IGRP,
	EIGRP_ROUTE_BGP,
	EIGRP_ROUTE_EIGRP,
};

typedef	struct EigrpIntfAddr_{
	U32	ipAddr;
	U32	ipMask;
	U32	ipDstAddr;
	
	U32	intfIndex;
	void	*intf;
	void	*curSysAdd;
}EigrpIntfAddr_st, *EigrpIntfAddr_pt;

enum{
	EIGRP_TERM_CONNECT_TYPE_DIRC	= 0,
	EIGRP_TERM_CONNECT_TYPE_SERIAL,
	EIGRP_TERM_CONNECT_TYPE_INTERNET,
};

enum{
	EIGRP_REDIS_RT_UP	= 1,
	EIGRP_REDIS_RT_DOWN, 
};


#if 0
#if(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#define	EIGRP_ASSERT(val)	\
		if(!(val)){	\
			FILE *fp;	\
			fp	= fopen("/usr/local/etc/eigrpDebug.txt", "a+");	\
			fprintf(fp, "%s, line:%d\n", __FILE__, __LINE__);	\
			fclose(fp);	\
		}
#elif(EIGRP_OS_TYPE == EIGRP_OS_PIL)
#define	EIGRP_ASSERT(val)	\
		if(!(val)){	\
			FILE *fp;	\
			fp	= fopen("eigrpDebug.txt", "a+");	\
			fprintf(fp, "%s, line:%d\n", __FILE__, __LINE__);	\
			fclose(fp);	\
		}
#else//(EIGRP_OS_TYPE == EIGRP_OS_PIL)
#define	EIGRP_ASSERT(val)
#endif//(EIGRP_OS_TYPE == EIGRP_OS_PIL)
#else//0
#define	EIGRP_ASSERT(val)	EigrpPortAssert(val, "")
#endif//0

int	EigrpPortTaskStart(S8 *name, int pri, void(*func)());
void	EigrpPortTaskDelete();
void	EigrpPortMemFree(void *);
void	*EigrpPortMemMalloc(U32);
void	EigrpPortMemCpy(U8 *, U8 *, U32);
S32	EigrpPortMemCmp(U8 *, U8 *, U32);
void	EigrpPortMemSet(U8 *, U8, U32);
U32	EigrpPortStrLen(U8 *);
void	EigrpPortStrCpy(U8 *, U8 *);
S32	EigrpPortStrCmp(U8 *, U8 *);
void	EigrpPortStrCat(U8 *, U8 *);
void	*EigrpPortSemBCreate(S32);
void	EigrpPortSemBGive(void	*);
void	EigrpPortSemBTake(void *);
void	EigrpPortSemBDelete(void *);
S32	EigrpPortAtoi(U8 *);
void	*EigrpPortTimerCreate(void(*)(void *), void *, U32);
void	EigrpPortTimerDelete(void **);
U32	EigrpPortGetTimeSec();
S32	EigrpPortGetTime(U32 *, U32 *, U32 *);
void	EigrpPortSleep(U32);
void	EigrpPortAssert(U32, U8 *);
S32	EigrpPortSocket(S32);
#if 0
S32	EigrpPortAtoHex(S32, S8 *, U32 *);
S32	EigrpPortBind(S32, U32, U16);
S32 EigrpPortSocketFionbio(S32);
S32	EigrpPortSockRelease(void *);
S32	EigrpPortSendto(S32, S8 *, U32, U32, U16);
S32 EigrpPortRecv(S32, S8 *, S32, S32);
S32 EigrpPortRecvfrom(S32, S8 *, S32, S32, U32 *, U16 *);
#endif
S32	EigrpPortSockCreate();
void EigrpPortSockRelease(S32);
U32	EigrpPortSetMaxPackSize(S32);
void	EigrpPortMcastOption(S32, struct EigrpIntf_ *, S32);
#ifdef _DC_
void	EigrpPortMcastOption_vlanIf(S32, U32, S32);
void	EigrpPortMcastOptionWithLayer2(S32, struct EigrpIntf_ *, S32);
#endif// _DC_
S32	EigrpPortSendIpPacket(S32, void *, U32, U8, struct EigrpSockIn_ *, struct EigrpIdb_ *);
void	EigrpPortSendPacket(struct EigrpPktHdr_ *, S32, struct EigrpDualPeer_ *, struct EigrpIdb_ *, S32);
#ifdef EIGRP_PACKET_UDP
S32	EigrpPortRecvIpPacket(S32, S32 *, void *, U32 *, U16 *, void **);
#else//EIGRP_PACKET_UDP
S32	EigrpPortRecvIpPacket(S32, S32 *, void *, void **);
#endif//EIGRP_PACKET_UDP
U32	EigrpPortSock2Ip(struct EigrpSockIn_ *);
EigrpIntfAddr_pt	EigrpPortGetFirstAddr(void *);
EigrpIntfAddr_pt	EigrpPortGetNextAddr(EigrpIntfAddr_pt);
S32 EigrpPortSetIfBandwidth(void *, U32);
S32 EigrpPortSetIfDelay(void *, U32);
#ifndef _EIGRP_PLAT_MODULE
S32 EigrpPortSetIfBandwidth_2(void *sysIf, void *srcIf);
S32 EigrpPortSetIfDelay_2(void *sysIf, void *srcIf);
#endif// _EIGRP_PLAT_MODULE
void	EigrpPortCopyIntfInfo(struct EigrpIntf_ *, void *);
void	EigrpPortGetAllSysIntf();
/*S32	EigrpPortIntfCoverPeer(U32,  struct EigrpIntf_ *);*/
void	EigrpPortCoreRtAdd(U32, U32, U32, U32, U8, U32);
void	*EigrpPortCoreRtNodeFindExact(U32, U32);
S32	EigrpPortCoreRtNodeHasConnect(void *);
S32	EigrpPortCoreRtNodeHasStatic(void *);
void	*EigrpPortCoreRtGetEigrpExt(void *);
U32	EigrpPortCoreRtGetDest(void *);
U32	EigrpPortCoreRtGetMask(void *);
U32	EigrpPortCoreRtGetConnectIntfId(void *);
void	EigrpPortCoreRtDel(U32, U32,  U32, U32);
S32	EigrpPortRedisPriorityCmp(U32 , U32);
S32	EigrpPortRedisAddRoute(struct EigrpPdb_ *, U32, U32, U32, U32, U32, U32);
S32	EigrpPortRedisDelRoute(struct EigrpPdb_ *, U32, U32, U32, U32);
void	EigrpPortRedisProcNetworkAddStatic(void *, U32 );
void	EigrpPortRedisTrigger(U32);
void	EigrpPortRedisUnTrigger(U32);
void	EigrpPortRedisAddProto(struct EigrpPdb_ *, U32, U32);
void	EigrpPortRedisDelProto(struct EigrpPdb_ *, U32, U32);
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
#ifdef EIGRP_PLAT_ZEBOS
void	EigrpPortCliPrint(struct cli *, S8 *);
S32	EigrpPortCliOut(U8 *, void *, U32, U32, U8 *);
void	eigrp_lib_cli_init (void *);
S32	frp_router_config_write (struct cli *);
S32 frp_router_interface_write(struct cli *);
void	eigrp_cli_router_init (void *);
void	eigrp_cli_eigrp_init (void *);
void	eigrp_show_init (void *);
void	eigrp_cli_init(void *);
void	EigrpPortZebraGetMsg();
S32	EigrpPortZebraRecvService (struct nsm_msg_header *, void *, void *);
void	EigrpPortZebraUnlockRtNode(void *);
S32 EigrpPortIntfLinkUp(struct nsm_msg_header *, void *, void *);
S32 EigrpPortIntfLinkDown(struct nsm_msg_header *, void *, void *);
S32 EigrpPortIntfAdd(struct nsm_msg_header *, void *, void *);
S32 EigrpPortIntfDel(struct nsm_msg_header *, void *, void *);
S32 EigrpPortAddrAdd(struct nsm_msg_header *, void *, void *);
S32 EigrpPortAddrDel(struct nsm_msg_header *, void *, void *);
void eigrp_zebra_client_init();
void eigrp_zebra_client_clean();
void EigrpPortDelZebraIfp(void *);
S32	EigrpPortZebraDebugWrite(struct cli *);
#endif//EIGRP_PLAT_ZEBOS
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
void	EigrpPortDistCallBackInit();
void	EigrpPortDistCallBackClean();
void	EigrpPortCallBackInit();
void	EigrpPortCallBackClean();
struct EigrpSockIn_ *EigrpPortSockBuildIn(U16, U32);
void	*EigrpPortGetRouteMapByName(U8 *);
S32	EigrpPortRouteMapJudge(void *, U32, U32, void *);

/* void	EigrpPortRouteMapInit(); */
S32	EigrpPortPermitIncoming(U32, U32, struct EigrpIdb_ *);
S32	EigrpPortPermitOutgoing(U32, U32, struct EigrpIdb_ *);
void	*EigrpPortGetDistByIfname(U8 *);
void	EigrpPortDistributeUpdate(void *);
S32	EigrpPortCheckAclPermission(U32, U32, U32);
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
void	EigrpPortIntfChange(void *, void *, void *);
#endif//#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
void	EigrpPortRouteProcEvent(U32 , U32 , U32 , U32 , U32 , U32 , U32 );
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_PIL)
void EigrpPortRouteChange(void *, void *);
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)||(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
#ifdef EIGRP_PLAT_ZEBOS
S32 EigrpPortRouteChange(struct nsm_msg_header *, void *, void *);
#endif//EIGRP_PLAT_ZEBOS
//·��ƽ̨��EIGRP����·�ɣ���ɢ��ȥ��������ʹ���������������ʹ��zebraEigrpCmdRoute����
void EigrpPortRouteChange(int as, int type, int proto, long ip, long mask, long next, int metric);
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)||(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)

#ifdef _EIGRP_PLAT_MODULE
//��װ���Ӻ��������ڽ���EIGRPѧϰ����·�ɣ��ο�zebraEigrpClientRouteHandle(int type, ZebraEigrpRt_pt rt)����
extern int zebraEigrpRoutehandle(int (* func)(int , void *));
#endif//_EIGRP_PLAT_MODULE

void	EigrpPortRegIntfCallbk();
void	EigrpPortRegRtCallbk();
void	EigrpPortUnRegIntfCallbk();
void	EigrpPortUnRegRtCallbk();
void  EigrpPortPrintDbg(U8 *, struct EigrpDbgCtrl_ *);
void *EigrpPortGetGateIntf(U32);



#ifdef _DC_
S32	EigrpPortDcSockCreate();
S32	EigrpPortDcSendOne(S32, U8 *, U32);
S32	EigrpPortDcRecv(U8 *, U32 *);
void	EigrpPortProcDcPkt(U8 *, U32);
void	EigrpPortSndDcUaiP2mp(S32, U8 *);
void	EigrpPortSndDcUaiP2mp_vlanid(S32, U32);
void	EigrpPortSndDcUaiP2mp_Nei(S32, U32, U32);
U8  EigrpPortCircIsUnnumbered(void *);
S32 EigrpPortSetUnnumberedIf(void *, U32);
S32 EigrpPortUnSetUnnumberedIf(void *);
U32 EigrpPortGetRouterId();
#endif//_DC_

//int EigrpPortNetworkRouterId(S32, U32);

#ifdef __cplusplus
}
#endif

#endif

