#ifndef	_EIGRP_SNMP_H_
#define	_EIGRP_SNMP_H_
#ifdef __cplusplus 
extern "C"{
#endif

#ifdef INCLUDE_EIGRP_SNMP

#define EIGRP_SNMP_MAX_PKT_SIZE	1500
/*Subagent的IP地址*/
#define EIGRP_SNMP_AGNET_IP		0x7F000001
/*Subagent使用的端口*/
#define EIGRP_AGENT_PORT			7001
/*与Subagent通信时使用的本地端口*/
#define EIGRP_SNMP_PORT			7002

enum eigrpSnmpObjType{
	EIGRP_SNMP_OBJ_AS_AS_NO = 1,
	EIGRP_SNMP_OBJ_AS_SUB_NET,
	EIGRP_SNMP_OBJ_AS_ENABLE_NET,
	EIGRP_SNMP_OBJ_AS_ROW_STATUS,
	EIGRP_SNMP_OBJ_AS_TBL,
	EIGRP_SNMP_OBJ_NBR_AS_NO,
	EIGRP_SNMP_OBJ_NBR_IP,
	EIGRP_SNMP_OBJ_NBR_IF_INDEX,
	EIGRP_SNMP_OBJ_NBR_HOLD_TIME,
	EIGRP_SNMP_OBJ_NBR_UP_TIME,
	EIGRP_SNMP_OBJ_NBR_SRTT,
	EIGRP_SNMP_OBJ_NBR_RTO,
	EIGRP_SNMP_OBJ_NBR_PKT_ENQUEUED,
	EIGRP_SNMP_OBJ_NBR_LAST_SEQ,
	EIGRP_SNMP_OBJ_NBR_RETRYCNT,
	EIGRP_SNMP_OBJ_NBR_ROUTER_ID,
	EIGRP_SNMP_OBJ_NBR_TBL
};

enum eigrpSnmpAccsType{
	EIGRP_SNMP_GET = 1,
	EIGRP_SNMP_GET_NEXT,
	EIGRP_SNMP_RESERVED,
	EIGRP_SNMP_SET,
	EIGRP_SNMP_WALK
};

enum eigrpSnmpRowStatus{
	EIGRP_SNMP_ROWSTATUS_ACTIVE = 1,
	EIGRP_SNMP_ROWSTATUS_NOTINSERVICE,
	EIGRP_SNMP_ROWSTATUS_NOTREADY,
	EIGRP_SNMP_ROWSTATUS_CREATEANDGO,
	EIGRP_SNMP_ROWSTATUS_CREATEANDWAIT,
	EIGRP_SNMP_ROWSTATUS_DESTROY
};

typedef struct eigrpAs_{
	S32		asNo;
	S8		asSubnet[161];/*格式:A.B.C.D/M，以分号间隔*/
	U32		asSubnet_len;
	S8		asEnableNet[11];
	U32		asEnableNet_len;
	U32		asRowStatus;
}eigrpAs_st, *eigrpAs_pt;

typedef struct eigrpNbr_{
	S32		asNo;
	U32          ipAddr;
	S32            ifIndex;
	U32          holdTime;
	S8           upTime[8];
	U32          upTime_len;  /* # of char elements, not bytes */
	U32          srtt;
	U32        rto;
	U32         pktsEnqueued;
	U32        lastSeq;
	U32          retries;
	U32		routerId;
}eigrpNbr_st, *eigrpNbr_pt;

typedef struct eigrpSnmpObj_{
	struct eigrpSnmpObj_	*nxt;
	struct eigrpSnmpObj_	*pre;
	union{
		eigrpAs_st	as;
		eigrpNbr_st	nbr;
	};
}eigrpSnmpObj_st, *eigrpSnmpObj_pt;

typedef struct eigrpSnmpReq_{
	U32	reqId;		/*请求标识*/
	U8	obj;			/*访问对象*/
	U8	accessType;	/*1-get or 2-set*/
	S32	asNo;
	U32	ipAddr;
	U32	valCnt;
	U8	val[0];
}eigrpSnmpReq_st, *eigrpSnmpReq_pt;

typedef struct eigrpSnmpRsp_{
	U32	reqId;		/*报文标识*/
	U8	obj;			/*访问对象*/
	U8	accessType;	/*1-get or 2-set*/
	U8	reqAck;		/*Request 是否成功*/
	U32	valCnt;
	U8	val[0];
}eigrpSnmpRsp_st, *eigrpSnmpRsp_pt;
S32 eigrpSnmpInit();
void eigrpSnmpProc();
S32 eigrpSnmpNbrGetAsNo(S32, U32, eigrpNbr_pt);
S32 eigrpSnmpReqProc(S8 *, U32);
S32 eigrpSnmpRespSend(U8, U32, U8, U8, eigrpSnmpObj_pt);

#endif//INCLUDE_EIGRP_SNMP

#ifdef __cplusplus
}
#endif
#endif
