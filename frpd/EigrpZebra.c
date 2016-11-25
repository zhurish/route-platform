/*-------------------------------------------------------------------------
    
-------------------------------------------------------------------------*/
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

#ifdef _EIGRP_PLAT_MODULE
#include	"./include/EigrpZebra.h"


extern	Eigrp_pt	gpEigrp;
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
unsigned long debug_eigrp_zebra = 0;
/************************************************************************************/
struct ZebraEigrpMaster *EigrpMaster = NULL;
/************************************************************************************/
/************************************************************************************/
int zebraEigrpCmdSetupResponse(EigrpCmd_pt pCmd, S32 retVal, S32 private)
{
	if(EigrpMaster)
	{
		if(pCmd)
		{
			EigrpMaster->Response.Response = TRUE;
			EigrpMaster->Response.cmd = pCmd->type;
			EigrpMaster->Response.noFlag = pCmd->noFlag;
			EigrpMaster->Response.asNum = pCmd->asNum;
			EigrpMaster->Response.retVal = retVal;		
			EigrpMaster->Response.private = private;		
			if( (pCmd->type >= EIGRP_CMD_TYPE_SHOW_INTF)&&(pCmd->type <= EIGRP_CMD_TYPE_SHOW_DEBUG) )
			{
				EigrpMaster->Response.retVal = SUCCESS;
				return SUCCESS;
			}
		}
		else
		{
			EigrpPortMemSet((U8 *)&EigrpMaster->Response, 0, sizeof(ResponseCmd_st));
			EigrpMaster->Response.Response = FALSE;
			return SUCCESS;
		}
	}
	return FAILURE;
}
/***************************************************************************/
/***************************************************************************/
int zebraEigrpCmdResponse(void)
{
	/*
	if(pCmd)
	{
		if(pCmd->vsPara1)
			EigrpPortMemFree(pCmd->vsPara1);
		if(pCmd->vsPara2)	
			EigrpPortMemFree(pCmd->vsPara2);
	}
	*/
	if(EigrpMaster)
	{
		char *cmdstr = NULL;
		EigrpPortMemSet((U8 *)&EigrpMaster->Response, 0, sizeof(ResponseCmd_st));
		while(EigrpMaster->Response.Response != TRUE)
			EigrpPortSleep(10);
		if(EigrpMaster->Response.retVal == SUCCESS)
		{
			//EigrpPortMemSet((U8 *)&EigrpMaster->Response, 0, sizeof(ResponseCmd_st));
			return SUCCESS;
		}
		//EigrpMaster->Response.cmd = pCmd->type;
		//EigrpMaster->Response.noFlag = pCmd->noFlag;
		//EigrpMaster->Response.asNum = pCmd->asNum;
		//EigrpMaster->Response.retVal = retVal;
		if(EigrpMaster->Response.private == TRUE)
			cmdstr = ResponseCmdTable[EigrpMaster->Response.cmd+EIGRP_CMD_TYPE_MAX+EigrpMaster->Response.noFlag].cmdstr[0];
		else	
			cmdstr = ResponseCmdTable[EigrpMaster->Response.cmd].cmdstr[EigrpMaster->Response.noFlag];
		printf(" %s\n",cmdstr);
		return FAILURE;
	}
	return FAILURE;
}
/************************************************************************************/
/************************************************************************************/
/***************************************************************************/
/***************************************************************************/
ZebraEigrpSched_pt zebraEigrpUtilSchedInit()
{
	ZebraEigrpSched_st *list = NULL;

	list	= (ZebraEigrpSched_pt)EigrpPortMemMalloc(sizeof(ZebraEigrpSched_st));
	if(!list){	 
		return NULL;
	}
	EigrpPortMemSet((U8 *)list, 0, sizeof(ZebraEigrpSched_st));
	list->sem	= EigrpPortSemBCreate(TRUE);
	if(!list->sem)
	{
		EigrpPortMemFree(list);
		return NULL;
	}
	return list;
}

int zebraEigrpUtilSchedClean(ZebraEigrpSched_pt list)
{
	ZebraEigrpSched_pt		pTem = NULL;

	if((!list)||(!list->sem) )
		return FAILURE;
	EigrpPortSemBTake(list->sem);
	while(1){
		EigrpPortSemBGive(list->sem);
		pTem	= zebraEigrpUtilSchedGet(list);
		EigrpPortSemBTake(list->sem);

		if(!pTem){
			break;
		}
		EigrpPortMemFree(pTem);
		pTem = NULL;
	}
	pTem	= list;
	list	= NULL;
	
	if(pTem)
	{
		EigrpPortSemBDelete(pTem->sem);
		pTem->sem	= NULL;	
		EigrpPortMemFree(pTem);
	}
	return SUCCESS;
}

int zebraEigrpUtilSchedCancel(ZebraEigrpSched_pt list, ZebraEigrpSched_pt pSched)
{
	ZebraEigrpSched_pt		pPrev = NULL;

	if((!list)||(!list->sem)||(!pSched) )
		return FAILURE;
	EigrpPortSemBTake(list->sem);
	if(list->forw == pSched){
		list->forw	= pSched->forw;
	}else{
		/* Just to make sure that this sched is in the list */
		for(pPrev = list->forw; pPrev && pPrev->forw != pSched; pPrev = pPrev->forw){
			;
		}
		pPrev->forw	= pSched->forw;
	}
	EigrpPortMemFree(pSched);
	list->count--;
	EigrpPortSemBGive(list->sem);

	return SUCCESS;
}
/************************************************************************************

	Name:	EigrpUtilSchedAdd

	Desc:	This function is to create an eigrp sched data and insert it into the job list.
		
	Para: 	func		- pointer to the callback function of the new sched
			param	- pointer to the data of the new sched
	
	Ret:		pointer to the new sched
************************************************************************************/

ZebraEigrpSched_pt zebraEigrpUtilSchedAdd(ZebraEigrpSched_pt list, S32 (*func)(void *), void *param, int size)
{
	ZebraEigrpSched_pt		pSched = NULL, pPrev = NULL;
	
	if((!list)||(!list->sem) )
		return NULL;

	pSched	= (ZebraEigrpSched_pt)EigrpPortMemMalloc(sizeof(ZebraEigrpSched_st));
	if(!pSched){	 
		return NULL;
	}
	EigrpPortMemSet((U8 *)pSched, 0, sizeof(ZebraEigrpSched_st));

	pSched->func		= func;
	pSched->data		= param;
	/*
	pSched->data		= EigrpPortMemMalloc(size+1);
	if(pSched->data == NULL)
	{
		EigrpPortMemFree(pSched);
		return NULL;
	}
	EigrpPortMemCpy(pSched->data, param, size);
	*/
	EigrpPortSemBTake(list->sem);
	
	if(!list->forw){
		/* I am the first one.*/
		if(list->count==0)
			list->forw	= pSched;
	}else{
		if(list->count) {

			pPrev	= list->forw;
			while(pPrev->forw){
				pPrev	= pPrev->forw;
			}
			pPrev->forw			= pSched;
		}
	}
	list->count++;
	EigrpPortSemBGive(list->sem);	
	return pSched;
}

/************************************************************************************

	Name:	EigrpUtilSchedGet

	Desc:	This function is to get one eigrp job from the job list.
		
	Para: 	NONE
	
	Ret:		pointer to the eigrp job 
************************************************************************************/
ZebraEigrpSched_pt zebraEigrpUtilSchedGet(ZebraEigrpSched_pt list)
{
	ZebraEigrpSched_pt		pSched = NULL;
	
	if((!list)||(!list->sem) )
		return NULL;
	EigrpPortSemBTake(list->sem);
	pSched	= NULL;
	if(list->forw){
		if(list->count){

			list->count--;
			pSched	= list->forw;
			
			list->forw	= pSched->forw;
		}
	}
	EigrpPortSemBGive(list->sem);
	return pSched;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
//�¼����е���
int ZebraEigrpUtilSched()
{		
	if( (EigrpMaster)&&(EigrpMaster->ZebraSched) )
	{
		S32	(*func)(void *) = NULL;
		ZebraEigrpSched_pt	pSched = NULL;
		pSched = zebraEigrpUtilSchedGet(EigrpMaster->ZebraSched);
		if(pSched){
			func	= (S32 (*)(void *))pSched->func;
			if(func)
				(func)(pSched->data); 
			EigrpPortMemFree(pSched);
			return SUCCESS;
		}
	}		
	return FAILURE;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
int ZebraEigrpCmdInit()
{
	if( EigrpMaster )
	{
		EigrpMaster->ZebraCmdQue	= EigrpUtilQue2Init();
		if(!EigrpMaster->ZebraCmdQue)
		{		 
			return FAILURE;
		}
	}
	return SUCCESS;
}
/************************************************************************************/		
int ZebraEigrpCmdCancel()
{
	if( EigrpMaster )
	{
		if(EigrpMaster->ZebraCmdQue)
		{
			EigrpCmd_pt	pCmd = NULL, pCmdNext = NULL;	
			for(pCmd = (struct EigrpCmd_ *)EigrpMaster->ZebraCmdQue->head; pCmd; pCmd= pCmdNext)
			{
				pCmdNext = pCmd->next;
				EigrpUtilQue2Unqueue(EigrpMaster->ZebraCmdQue , (struct EigrpQueElem_ *)pCmd);
				EigrpPortMemFree((void *)pCmd);
			}
			EigrpPortMemFree((void *)EigrpMaster->ZebraCmdQue->lock);
			EigrpPortMemFree((void *)EigrpMaster->ZebraCmdQue);
			return SUCCESS;
		}
	}
	return FAILURE;
}
/************************************************************************************/
/************************************************************************************/
int ZebraEigrpCmdSched()
{		
	if( (EigrpMaster)&&(EigrpMaster->ZebraCmdQue) )
	{
		EigrpCmd_pt	pCmd = NULL;
		pCmd = (EigrpCmd_pt)EigrpUtilQue2Dequeue(EigrpMaster->ZebraCmdQue);
		if(pCmd)
			return ZebraEigrpCmdProc(pCmd);
	}		
	return FAILURE;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
static const char *zebra_interface_flag_dump (unsigned long flag)
{
  int separator = 0;
  static char logbuf[512];
  memset(logbuf, 0, 512);
#define IFF_OUT_LOG(X,STR) \
  if (flag & (X)) \
    { \
      if (separator) \
	strlcat (logbuf, ",", 512); \
      else \
	separator = 1; \
      strlcat (logbuf, STR, 512); \
    }
  
  strlcpy (logbuf, "<", 512);
  IFF_OUT_LOG (EIGRP_INTF_FLAG_ACTIVE, "UP");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_BROADCAST, "BROADCAST");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_DEBUG, "DEBUG");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_LOOPBACK, "LOOPBACK");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_POINT2POINT, "POINTOPOINT");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_NOTRAILERS, "NOTRAILERS");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_RUNNING, "RUNNING");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_NOARP, "NOARP");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_PROMISC, "PROMISC");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_ALLMULTI, "ALLMULTI");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_OACTIVE, "OACTIVE");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_SIMPLEX, "SIMPLEX");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_LINK0, "LINK0");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_LINK1, "LINK1");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_LINK2, "LINK2");
  IFF_OUT_LOG (EIGRP_INTF_FLAG_MULTICAST, "MULTICAST");
  strlcat (logbuf, ">", 512);
  return logbuf;
#undef IFF_OUT_LOG
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
int ZebraEigrpCmdProc(EigrpCmd_pt pCmd)
{
	zebraEigrpCmdEvent(pCmd);
	return SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
//ϵͳ�ӿڱ��ʼ��
int ZebraEigrpIntfInit(int num)
{
	if(EigrpMaster)
		EigrpMaster->eigrpiflist = NULL;
	kernel_list_init(num);
	return SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
int ZebraEigrpIntfClean()
{
	EigrpInterface_pt	pTem = NULL, pNext = NULL;
	if(EigrpMaster && EigrpMaster->eigrpiflist)
	{
		for(pTem = EigrpMaster->eigrpiflist; pTem; pTem = pNext){
			pNext = pTem->next;
			EigrpPortMemFree(pTem);
		}
	}
	EigrpMaster->eigrpiflist	= NULL;
	return SUCCESS;
}
int	ZebraEigrpIntfDel(EigrpInterface_pt pIntf)
{
	EigrpInterface_pt	pTem = NULL;
	if((!EigrpMaster) ||(! EigrpMaster->eigrpiflist) )
		return FAILURE;
	if(pIntf == NULL)
		return FAILURE;
	EIGRP_FUNC_ENTER(EigrpIntfDel);
	pTem	= NULL;	
	if(EigrpMaster->eigrpiflist == pIntf){
		EigrpMaster->eigrpiflist = pIntf->next;
	}else{
		for(pTem = EigrpMaster->eigrpiflist; pTem; pTem = pTem->next){
			if(pTem->next == pIntf){
				break;
			}
		}
		EIGRP_ASSERT((U32)pTem);
		if(!pTem){	
			EigrpPortMemFree(pIntf);
			EIGRP_FUNC_LEAVE(EigrpIntfDel);
			return FAILURE;
		}
		pTem->next= pIntf->next;
	}
	EigrpPortMemFree(pIntf);
	return SUCCESS;
}
/************************************************************************************

	Name:	EigrpIntfAdd

	Desc:	This function is to insert a new eigrp interface into the eigrp interface list.
		
	Para: 	pIntf		- pointer to the eigrp interface which is to be added
	
	Ret:		NONE
************************************************************************************/
int	ZebraEigrpIntfAdd(EigrpInterface_pt pIntf)
{
	EigrpInterface_pt pTem = NULL;
	
	if((!EigrpMaster) /*||(! EigrpMaster->eigrpiflist) */)
		return FAILURE;
	if(pIntf == NULL)
		return FAILURE;
	
	EIGRP_FUNC_ENTER(EigrpIntfAdd);
	if(EigrpMaster->eigrpiflist == NULL){
		EigrpMaster->eigrpiflist = pIntf;
		EIGRP_FUNC_LEAVE(EigrpIntfAdd);
		return FAILURE;
	}
	for(pTem = EigrpMaster->eigrpiflist; pTem; pTem = pTem->next){
		if(pTem == pIntf){
			EIGRP_FUNC_LEAVE(EigrpIntfAdd);
			return FAILURE;	/* this interface has already exist in this list. */
		}
	}
	pIntf->next	= EigrpMaster->eigrpiflist;
	EigrpMaster->eigrpiflist = pIntf;
	EIGRP_FUNC_LEAVE(EigrpIntfAdd);
	return SUCCESS;
}

static int ZebraEigrpIntfShowOne(EigrpInterface_pt pIntf)
{
	if(pIntf)
	{
		printf( "\nInterface %s %s%s",pIntf->name,BIT_TEST(pIntf->flags, EIGRP_INTF_FLAG_ACTIVE)? "up":"down","\n");
		printf( " index %d metric %d mtu %d bandwidth %d kbps delay %d msec%s",
				pIntf->ifindex,
				pIntf->metric,
				pIntf->mtu,
				pIntf->bandwidth,
				pIntf->delay,"\n");
		printf( " flag %s %s",zebra_interface_flag_dump (pIntf->flags),"\n");
		if(pIntf->ipaddress.ipAddr != 0)
		{
			printf( " inet %s netmask %s ",
					EigrpUtilIp2Str(pIntf->ipaddress.ipAddr),
					EigrpUtilIp2Str(pIntf->ipaddress.ipMask));
			if(BIT_TEST(pIntf->flags, EIGRP_INTF_FLAG_POINT2POINT))
				printf( "destnation %s \n",EigrpUtilIp2Str(pIntf->ipaddress.ipDstAddr));
			else
				printf( "broadcast %s \n",EigrpUtilIp2Str(pIntf->ipaddress.ipAddr | (~pIntf->ipaddress.ipMask)));
		}
	}
	return SUCCESS;
}
int	ZebraEigrpIntfShow()
{
	EigrpInterface_pt pTem = NULL;
	
	if((!EigrpMaster) ||(! EigrpMaster->eigrpiflist) )
		return FAILURE;
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);	
	printf("interface information\n");
	printf("=================================================================\n");
	for(pTem = EigrpMaster->eigrpiflist; pTem; pTem = pTem->next){
		ZebraEigrpIntfShowOne(pTem);
	}
	printf("=================================================================\n");
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);	
	return SUCCESS;
}
#endif// (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
/************************************************************************************/
/************************************************************************************/
/************************************************************************************

	Name:	EigrpPortGetFirstAddr

	Desc:	This function is to get the first ip addrress of the given physical interface.
		
	Para: 	pCirc	- pointer to the physical interface
	
	Ret:		pointer to structure of the first ip address,if failed or the physical interface is
			not exist, return NULL
************************************************************************************/
/*
 *zebraEigrpPortGetNextAddr and zebraEigrpPortGetFirstAddr func will lookbak
 * so set zebraEigrpPortGetNextAddr return NULL;
 */
EigrpIntfAddr_pt	zebraEigrpPortGetFirstAddr(void *pCirc)
{
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EigrpIntfAddr_pt	pAddr = NULL;
		EigrpInterface_pt pIntf = (EigrpInterface_pt)pCirc;
		if(pIntf)
		{
			pAddr	= EigrpPortMemMalloc(sizeof(EigrpIntfAddr_st));
			if(!pAddr){
				return NULL;
			}
			pAddr->ipDstAddr	= pIntf->ipaddress.ipDstAddr;
			pAddr->ipAddr	= pIntf->ipaddress.ipAddr;
			pAddr->ipMask	= pIntf->ipaddress.ipMask;
			pAddr->intf		= pCirc;
			pAddr->intfIndex 	= pIntf->ifindex;
			pAddr->curSysAdd	= NULL;//pIntf->ipaddress.curSysAdd;
			return pAddr;
		}
		return NULL;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBRA	
		EigrpIntfAddr_pt	pAddr;
		
		struct prefix *p;
		struct listnode *node;
		struct connected	*pConn;
		struct interface * ifp = (struct interface *)pCirc;
		if(ifp == NULL)
			return NULL;
		for (ALL_LIST_ELEMENTS_RO (ifp->connected, node, pConn))
		{
			p = pConn->address;
			if (p && p->family == AF_INET)
					break;	      
		}
	  
		if(!pConn){
			return NULL;
		}
		
		pAddr	= EigrpPortMemMalloc(sizeof(EigrpIntfAddr_st));
		if(!pAddr){
			return NULL;
		}
		if(if_is_pointopoint ((struct interface *)ifp)){
			pAddr->ipDstAddr	= ntohl(pConn->destination->u.prefix4.s_addr);
			pAddr->ipAddr	= ntohl(pConn->address->u.prefix4.s_addr);
			pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->destination->prefixlen);
		}else{
			pAddr->ipAddr	= ntohl(pConn->address->u.prefix4.s_addr);
			pAddr->ipDstAddr	= 0;
			pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
		}
		pAddr->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
		pAddr->intf		= ifp;
		pAddr->curSysAdd	= NULL;
		return pAddr;	
#endif// EIGRP_PLAT_ZEBRA
	}
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
/************************************************************************************

	Name:	EigrpPortGetNextAddr

	Desc:	This function is to get the next ip address of the given physical interface and ip address.
		
	Para: 	pAddr	- pointer to the logical interface which supplies the information of 
			relevant physical interface
	
	Ret:		pointer to the structure of next ip address
************************************************************************************/
//���ݽӿڵ�ַ��������ӿڵ���һ����ַ�����ﷵ�ؿ�ֵ
EigrpIntfAddr_pt	zebraEigrpPortGetNextAddr(EigrpIntfAddr_pt pAddr)
{
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{/*
		EigrpIntfAddr_pt pNext = (EigrpIntfAddr_pt)pAddr->curSysAdd;
		if(pNext)
		{
			//p->ipDstAddr	= pIntf->ipaddress.ipDstAddr;
			//p->ipAddr		= pIntf->ipaddress.ipDstAddr;
			//p->ipMask		= pIntf->ipaddress.ipAddr;
			pNext->intf		= pAddr->intf;
			pNext->intfIndex = pAddr->intfIndex;
			pNext->curSysAdd	= pAddr->curSysAdd;
			return pNext;
		}
		*/
		return NULL;
	}
#elif (EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
	//ÿ���ӿ�ֻ�е�һ����ַ��������FRP·�ɣ���������ʼ�շ���NULL
#ifdef EIGRP_PLAT_ZEBRA	
		return NULL;
#endif// EIGRP_PLAT_ZEBRA
	}
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
/************************************************************************************

	Name:	EigrpPortCopyIntfInfo

	Desc:	This function is get the infomation that eigrp interests in of the physical interface.
		
	Para: 	pEigrpIntf	- pointer to the Eigrp interface which strores received interface 
			information
			pSysIntf		- pointer to the system interface
	
	Ret:		
************************************************************************************/
void	zebraEigrpPortCopyIntfInfo(EigrpIntf_pt pEigrpIntf, void *pSysIntf)
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		//EigrpIntfAddr_pt next;
		//EigrpIntfAddr_pt addr;
		if(!pEigrpIntf)
			return;
		EigrpPortMemSet((U8 *)pEigrpIntf,0,sizeof(EigrpIntf_st));
		EigrpInterface_pt pIntf = (EigrpInterface_pt)pSysIntf;
		if(pIntf)
		{
			EigrpPortStrCpy(pEigrpIntf->name, pIntf->name);
			pEigrpIntf->ifindex 	= pIntf->ifindex;
			pEigrpIntf->flags		= pIntf->flags;	
			pEigrpIntf->mtu	= pIntf->mtu;			
			pEigrpIntf->bandwidth	= ((pIntf->bandwidth)*8)/1000;
			pEigrpIntf->metric		= (U32)pIntf->metric;
			pEigrpIntf->delay	= pIntf->delay;
			pEigrpIntf->ipAddr = pIntf->ipaddress.ipAddr;
			pEigrpIntf->remoteIpAddr = pIntf->ipaddress.ipDstAddr;
			pEigrpIntf->ipMask = pIntf->ipaddress.ipMask;
			/*
			pEigrpIntf->sysIntf = addr;
			for(next = pIntf->ipaddress.curSysAdd; next; next = next->curSysAdd)
			{
				addr = EigrpPortMemMalloc(sizeof(EigrpIntfAddr_st));
				if(addr)
				{
					addr->ipDstAddr	= next->ipDstAddr;
					addr->ipAddr		= next->ipDstAddr;
					addr->ipMask		= next->ipAddr;
					addr->intf		= pEigrpIntf;
					addr->intfIndex = pEigrpIntf->ifindex;
					addr->curSysAdd	= next;
				}
			}
			*/
			pEigrpIntf->sysCirc	= pSysIntf;
		}
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBRA
		struct connected	*pConn = NULL;
		struct listnode *cnode = NULL;
		struct prefix *p = NULL;
		struct interface	*pInterface = NULL;
		
		if(!pEigrpIntf)
			return;
		
		pInterface	= (struct interface *)pSysIntf;
		if(!pInterface)
			return;
		
		EigrpPortMemSet((U8 *)pEigrpIntf,0,sizeof(EigrpIntf_st));

		EigrpPortStrCpy(pEigrpIntf->name, pInterface->name);
		pEigrpIntf->ifindex 	= pInterface->ifindex;
	
		if(if_is_up(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
		}
		
		if(if_is_broadcast(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST);
		}
		
		if(if_is_pointopoint(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT);
		}
	
		if(if_is_multicast(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST);
		}
	
		if(if_is_loopback(pInterface)){
			BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK);
		}
		pEigrpIntf->flags = (U32)pInterface->flags;
		pEigrpIntf->mtu	= pInterface->mtu;
#if(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	#if(EIGRP_LINUX_USE_VM==TRUE)
		pEigrpIntf->bandwidth	= 100 * 1000;
	#else
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			pEigrpIntf->bandwidth	= 2048;
		}else{
			pEigrpIntf->bandwidth	= ((pInterface->bandwidth)*8)/1000;
		}
	#endif
#else
		pEigrpIntf->bandwidth	= ((pInterface->bandwidth)*8)/1000;
#endif
		pEigrpIntf->metric		= (U32)pInterface->metric;
	
		/* we copy addr info when adding addr */
		for (ALL_LIST_ELEMENTS_RO (pInterface->connected, cnode, pConn))
		{
			p = pConn->address;
	
			if (p && p->family == AF_INET)
			{
				pEigrpIntf->ipAddr = ntohl(p->u.prefix4.s_addr);
				pEigrpIntf->ipMask = EIGRP_PREFIX_TO_MASK(p->prefixlen);
				if(if_is_pointopoint(pConn->ifp))
				{
					p = pConn->destination;
					pEigrpIntf->remoteIpAddr = ntohl(p->u.prefix4.s_addr);
				}
				else
					pEigrpIntf->remoteIpAddr	= 0;	
				
				pEigrpIntf->sysCirc	= pSysIntf;
				//printf("%s::%x:%x\n",pEigrpIntf->name, pEigrpIntf->ipAddr,pEigrpIntf->ipMask);
				break;
			}	      
		}
		/*
		pEigrpIntf->ipAddr	= pConn->address->u.prefix4.s_addr;
		pEigrpIntf->ipMask	= EIGRP_PREFIX_TO_MASK(pConn->address->prefixlen);
		if(if_is_pointopoint(pConn->ifp)){
			pEigrpIntf->remoteIpAddr	= pConn->destination->u.prefix4.s_addr;
		}else{
			pEigrpIntf->remoteIpAddr	= 0;
		}
		*/
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
		//���ָ�벻��ָ����Ӧ����ָ��ϵͳ�����û�������Ľӿ�����ڵ��ַ
		//pEigrpIntf->sysCirc	= pEigrpIntf;
		pEigrpIntf->sysCirc	= pSysIntf;
#elif (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
		pEigrpIntf->sysCirc	= pSysIntf;
#endif
#endif// EIGRP_PLAT_ZEBRA		
	}
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	return;
}
/****************************************************************************/
/****************************************************************************/
void	zebraEigrpPortGetAllSysIntf()
{
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	{
		EigrpInterface_pt pInterface = NULL;
		EigrpIntf_pt	pEigrpIntf = NULL;
		if( (EigrpMaster == NULL)||(EigrpMaster->eigrpiflist == NULL) )
			return;

		for(pInterface = EigrpMaster->eigrpiflist; pInterface; pInterface = pInterface->next){
			//printf("testzebra EigrpPortGetAllSysIntf: 2 %s\n", pInterface->name);
			pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
			if(pEigrpIntf)
			{
				EigrpPortCopyIntfInfo(pEigrpIntf, pInterface);
				BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);
				EigrpIntfAdd(pEigrpIntf);
			}
		}
		return;
	}
#elif	(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
	{
#ifdef EIGRP_PLAT_ZEBRA//·��ƽ̨֧�֣���zclient��Ԫ��ȡ�ӿ�����
		return;
#else	
		struct interface *pInterface = NULL;
		EigrpIntf_pt	pEigrpIntf = NULL;
		struct listnode *node = NULL;
		if( (EigrpMaster == NULL)||(EigrpMaster->iflist == NULL) )
			return;
  		for (ALL_LIST_ELEMENTS_RO (EigrpMaster->iflist, node, pInterface))
		{
			//printf("testzebra EigrpPortGetAllSysIntf: 2 %s\n", pInterface->name);
			pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpIntf_st));
			if(pEigrpIntf)
			{
				EigrpPortCopyIntfInfo(pEigrpIntf, pInterface);
				BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);
				EigrpIntfAdd(pEigrpIntf);
			}
		}
		return;
#endif// EIGRP_PLAT_ZEBRA
	}
#endif//(EIGRP_PLAT_TYPE == EIGRP_PLAT_ROUTER)
}
/****************************************************************************/
//ɾ��һ���ӿ�
S32	zebraEigrpIntfDel(EigrpIntf_pt  intf)
{
#if	(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#ifdef EIGRP_PLAT_ZEBRA

	struct listnode *node = NULL;
	struct connected *c = NULL;
	struct interface * ifp = intf->sysCirc;
	if(!ifp)
		return FAILURE;
	for (ALL_LIST_ELEMENTS_RO (ifp->connected, node, c)) {
		connected_free (c);
	  }
	if_delete (ifp);
	return SUCCESS;
#else// EIGRP_PLAT_ZEBRA	
	intf->sysCirc = NULL;
#endif// EIGRP_PLAT_ZEBRA	

#elif	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)

#ifdef EIGRP_PLAT_ZEBRA
	intf->sysCirc = NULL;
#else// EIGRP_PLAT_ZEBRA	
	intf->sysCirc = NULL;
#endif// EIGRP_PLAT_ZEBRA
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	return SUCCESS;
}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
#ifdef EIGRP_PLAT_ZEBRA
//#define eigrp_zlog_debug zlog_debug

#define eigrp_zlog_debug(msg,args...) \
		{\
		printf("## DEBUG -->: ");\
		printf(msg,##args);\
		printf("\n");\
		}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
//����zebra��Ԫ����route id��Ϣ
static int eigrp_router_id_update_zebra (int command, struct zclient *zclient,
			     zebra_size_t length)
{
  struct prefix router_id;
  zebra_router_id_update_read(zclient->ibuf,&router_id);

  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
    {
      char buf[128];
      prefix2str(&router_id, buf, sizeof(buf));
      eigrp_zlog_debug("Zebra: recv router id update %s", buf);
    }
  if(EigrpMaster)
	  zebraEigrpCmdApiRouterId(0, EigrpMaster->asnum, NTOHL(router_id.u.prefix4.s_addr));
  return SUCCESS;
}
/****************************************************************************/
static int zebra_eigrp_cmd_interface_handle(int type, int handle, void *ifp)
{
	EigrpIntf_st	pEigrpIntf;
	if(!ifp)
		return FAILURE;
	EigrpPortCopyIntfInfo(&pEigrpIntf, ifp);
	BIT_RESET(pEigrpIntf.flags, EIGRP_INTF_FLAG_EIGRPED);
	if(type == 0)
	{
		zebraEigrpCmdInterface(handle, EigrpMaster->asnum, &pEigrpIntf);
	}
	else
	{
		zebraEigrpCmdInterfaceState(handle, EigrpMaster->asnum, &pEigrpIntf);
	}
	return SUCCESS;
}
/****************************************************************************/
/* Inteface addition message from zebra. //��ӽӿ���Ϣ */
static int eigrp_interface_add (int command, struct zclient *zclient, zebra_size_t length)
{
  struct interface *ifp;
  ifp = zebra_interface_add_read (zclient->ibuf);
  if(ifp == NULL)
	  return 0;
  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
	  eigrp_zlog_debug ("Zebra: interface add %s index %d flags %llx metric %d mtu %d",
               ifp->name, ifp->ifindex, (unsigned long long)ifp->flags,
               ifp->metric, ifp->mtu);

  zebra_eigrp_cmd_interface_handle(0, 0, ifp);
  //EigrpSysIntfAdd_ex(ifp);		
  return 0;
}
/****************************************************************************/
/****************************************************************************/
//�ӿ�ɾ��
static int eigrp_interface_delete (int command, struct zclient *zclient,
                       zebra_size_t length)
{
  struct interface *ifp;
  struct stream *s;
//  struct route_node *rn;
  s = zclient->ibuf;
  /* zebra_interface_state_read() updates interface structure in iflist */
  ifp = zebra_interface_state_read (s);
  if (ifp == NULL)
    return 0;
  if (if_is_up (ifp))
	  eigrp_zlog_debug ("Zebra: got delete of %s, but interface is still up",
               ifp->name);

  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
	  eigrp_zlog_debug
      ("Zebra: interface delete %s index %d flags %llx metric %d mtu %d",
       ifp->name, ifp->ifindex, (unsigned long long)ifp->flags, ifp->metric, ifp->mtu);

	zebra_eigrp_cmd_interface_handle(0, 1, ifp);
  //EigrpSysIntfDel_ex(ifp);	
  return 0;
}
//���ݽӿ������ڱ��ؽӿ�������ҽӿ�
static struct interface * zebra_interface_if_lookup (struct stream *s)
{
  char ifname_tmp[INTERFACE_NAMSIZ];
  /* Read interface name. */
  stream_get (ifname_tmp, s, INTERFACE_NAMSIZ);
  /* And look it up. */
  return if_lookup_by_name_len(ifname_tmp,
			       strnlen(ifname_tmp, INTERFACE_NAMSIZ));
}
/****************************************************************************/
/****************************************************************************/
//�ӿ�״̬UP�¼�
static int eigrp_interface_state_up (int command, struct zclient *zclient,
                         zebra_size_t length)
{
  struct interface *ifp;
//  struct route_node *rn;
  ifp = zebra_interface_if_lookup (zclient->ibuf);
  if (ifp == NULL)
    return 0;
  /* Interface is already up. */
  if (if_is_operative (ifp))
    {
      /* Temporarily keep ifp values. */
      struct interface if_tmp;
      memcpy (&if_tmp, ifp, sizeof (struct interface));
      zebra_interface_if_set_value (zclient->ibuf, ifp);
      if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
    	  eigrp_zlog_debug ("Zebra: Interface[%s] state update.", ifp->name);
      if (if_tmp.bandwidth != ifp->bandwidth)//�������仯
        {
          if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
        	  eigrp_zlog_debug ("Zebra: Interface[%s] bandwidth change %d -> %d.",
                       ifp->name, if_tmp.bandwidth, ifp->bandwidth);
          zebra_eigrp_cmd_interface_handle(1, 0, ifp);             
          //EigrpSysIntfUp_ex(ifp);
        }
      if (if_tmp.mtu != ifp->mtu)//mtu�����仯
       {
          if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
        	  eigrp_zlog_debug ("Zebra: Interface[%s] MTU change %u -> %u.",
                       ifp->name, if_tmp.mtu, ifp->mtu);
	  /* Must reset the interface (simulate down/up) when MTU changes. */
	  			zebra_eigrp_cmd_interface_handle(1, 0, ifp);
          //EigrpSysIntfDown_ex(ifp);
          //EigrpSysIntfUp_ex(ifp);
       }
      return 0;
    }
  zebra_interface_if_set_value (zclient->ibuf, ifp);
  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
	  eigrp_zlog_debug ("Zebra: Interface[%s] state change to up.", ifp->name);
  zebra_eigrp_cmd_interface_handle(1, 0, ifp);
  //EigrpSysIntfUp_ex(ifp);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
//�ӿ�down�¼�
static int eigrp_interface_state_down (int command, struct zclient *zclient,
                           zebra_size_t length)
{
  struct interface *ifp;
//  struct route_node *node;
  ifp = zebra_interface_state_read (zclient->ibuf);
  if (ifp == NULL)
    return 0;
  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
	  eigrp_zlog_debug ("Zebra: Interface[%s] state change to down.", ifp->name);
  zebra_eigrp_cmd_interface_handle(1, 1, ifp);
  //EigrpSysIntfDown_ex(ifp);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
//�ӿڵ�ַ�����¼�
static int eigrp_interface_address_add (int command, struct zclient *zclient,
                            zebra_size_t length)
{
  char buf[128];
  struct connected *c;
  struct prefix p;
  c = zebra_interface_address_read (command, zclient->ibuf);
  if (c == NULL)
    return 0;
  
  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
    {
      prefix2str(c->address, buf, sizeof(buf));
      eigrp_zlog_debug("Zebra: interface %s address add %s", c->ifp->name, buf);
    }
  p = *c->address;
  //if_rtflag_set (&p, ZEBRA_ROUTE_FRP);
  if(p.family == AF_INET)
    zebraEigrpCmdInterfaceAddress(0, EigrpMaster->asnum, c->ifp->ifindex, NTOHL(p.u.prefix4.s_addr), EIGRP_PREFIX_TO_MASK(p.prefixlen),0);
  //EigrpSysIntfAddrAdd_ex(c->ifp->ifindex, NTOHL(p.u.prefix4.s_addr), EIGRP_PREFIX_TO_MASK(p.prefixlen),0);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
//�ӿڵ�ַɾ���¼�
static int eigrp_interface_address_delete (int command, struct zclient *zclient, zebra_size_t length)
{
	  struct connected *c;
	  struct interface *ifp;

	  struct prefix p;

	  c = zebra_interface_address_read (command, zclient->ibuf);

	  if (c == NULL)
	  {
		  int ifindex = 0;
		  stream_set_getp(zclient->ibuf, ZEBRA_HEADER_SIZE);
		  ifindex = stream_getl (zclient->ibuf);
		  if( (ifindex > 0)&&(ifindex < 256) )
			  zebraEigrpCmdInterfaceAddress(1, EigrpMaster->asnum, ifindex, 0, 0, 0);
		  eigrp_zlog_debug("Eigrp: can't faind interface address,so use to get interface index\n");
		  return 0;
	  }
	  if (IS_DEBUG_EIGRP (zebra, ZEBRA_INTERFACE))
	  {
	      char buf[128];
	      prefix2str(c->address, buf, sizeof(buf));
	      eigrp_zlog_debug("Zebra: interface %s address delete %s", c->ifp->name, buf);
	  }

	  ifp = c->ifp;
	  p = *c->address;
	  //p.prefixlen = IPV4_MAX_PREFIXLEN;
	  //if_rtflag_unset(&p, ZEBRA_ROUTE_FRP);
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	  zebra_interface_address_delete_send (zclient, ifp, c);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	  
	  if(p.family == AF_INET)
	    zebraEigrpCmdInterfaceAddress(1, EigrpMaster->asnum, c->ifp->ifindex, NTOHL(p.u.prefix4.s_addr), EIGRP_PREFIX_TO_MASK(p.prefixlen),0);
	  //EigrpSysIntfAddrDel_ex(c->ifp->ifindex, NTOHL(p.u.prefix4.s_addr), EIGRP_PREFIX_TO_MASK(p.prefixlen));

	  return 0;
}
/****************************************************************************/
/****************************************************************************/
/* Zebra route add and delete treatment. */
//eigrp����zebra·����Ϣ��·���طֲ���Ϣ��
static int eigrp_zebra_read_ipv4 (int command, struct zclient *zclient,
                      zebra_size_t length)
{
  struct stream *s;
  struct zapi_ipv4 api;
  unsigned long ifindex;
  struct in_addr nexthop;
  struct prefix_ipv4 p;
  s = zclient->ibuf;
  ifindex = 0;
  nexthop.s_addr = 0;

  /* Type, flags, message. */
  api.type = stream_getc (s);
  api.flags = stream_getc (s);
  api.message = stream_getc (s);

  /* IPv4 prefix. */
  memset (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = stream_getc (s);
  stream_get (&p.prefix, s, PSIZE (p.prefixlen));

  if (IPV4_NET127(ntohl(p.prefix.s_addr)))
    return 0;

  /* Nexthop, ifindex, distance, metric. */
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP))
    {
      api.nexthop_num = stream_getc (s);
      nexthop.s_addr = stream_get_ipv4 (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_IFINDEX))
    {
      api.ifindex_num = stream_getc (s);
      /* XXX assert(api.ifindex_num == 1); */
      ifindex = stream_getl (s);
    }
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_DISTANCE))
    api.distance = stream_getc (s);
  if (CHECK_FLAG (api.message, ZAPI_MESSAGE_METRIC))
    api.metric = stream_getl (s);

  if (command == ZEBRA_IPV4_ROUTE_ADD)
  {
	  if(EigrpMaster)
	  	zebraEigrpCmdRoute(0, EigrpMaster->asnum, api.type, NTOHL(p.prefix.s_addr), 
	  		EIGRP_PREFIX_TO_MASK(p.prefixlen), NTOHL(nexthop.s_addr), 51200);
		  //EigrpPortRouteChange(EigrpMaster->asnum, ZEBRA_IPV4_ROUTE_ADD, api.type, NTOHL(p.prefix.s_addr), EIGRP_PREFIX_TO_MASK(p.prefixlen), NTOHL(nexthop.s_addr), 51200);
  }
  else                          /* if (command == ZEBRA_IPV4_ROUTE_DELETE) */
  {
	  if(EigrpMaster)
	  	zebraEigrpCmdRoute(1, EigrpMaster->asnum, api.type, NTOHL(p.prefix.s_addr), 
	  		EIGRP_PREFIX_TO_MASK(p.prefixlen), NTOHL(nexthop.s_addr), 51200);
		  //EigrpPortRouteChange(EigrpMaster->asnum, ZEBRA_IPV4_ROUTE_DELETE, api.type, NTOHL(p.prefix.s_addr), EIGRP_PREFIX_TO_MASK(p.prefixlen), NTOHL(nexthop.s_addr), 51200);
  }
  return 0;
}
/****************************************************************************/
/****************************************************************************/
static int eigrp_zebra_ipv4_update (int proto, int type, struct prefix_ipv4 *p, struct in_addr *nexthop, 
		    u_int32_t metric, u_char distance, int ifindex)
{
  struct zapi_ipv4 api;

  if(EigrpMaster && EigrpMaster->zclient)
  {
	  if (EigrpMaster->zclient->redist[ZEBRA_ROUTE_FRP])
	  {
		  api.type = proto;
		  api.flags = 0;
		  api.message = 0;
		  api.safi = SAFI_UNICAST;
		  SET_FLAG (api.message, ZAPI_MESSAGE_NEXTHOP);
		  api.nexthop_num = 1;
		  api.nexthop = &nexthop;
		  api.ifindex_num = 0;
		  api.ifindex = ifindex;
		  SET_FLAG (api.message, ZAPI_MESSAGE_METRIC);
		  api.metric = metric;
	
		  if (distance && distance != ZEBRA_RIP_DISTANCE_DEFAULT)
		  {
			  SET_FLAG (api.message, ZAPI_MESSAGE_DISTANCE);
			  api.distance = distance;
		  }
		  return zapi_ipv4_route (type, EigrpMaster->zclient, p, &api);
	  }
  }
  return FAILURE;
}
/****************************************************************************/
/****************************************************************************/
#endif //EIGRP_PLAT_ZEBRA	
/****************************************************************************/
static int zebra_eigrp_route_type_exchange(int eigrp, int zebra)
{
	int val = 0;
	if(eigrp)
	{
		switch(eigrp)
		{
		case EIGRP_ROUTE_SYSTEM:
			val = ZEBRA_ROUTE_SYSTEM;
			break;
		case EIGRP_ROUTE_KERNEL:
			val = ZEBRA_ROUTE_KERNEL;
			break;
		case EIGRP_ROUTE_CONNECT:
			val = ZEBRA_ROUTE_CONNECT;
			break;
		case EIGRP_ROUTE_STATIC:
			val = ZEBRA_ROUTE_STATIC;
			break;
		case EIGRP_ROUTE_OSPF:
			val = ZEBRA_ROUTE_OSPF;
			break;
		case EIGRP_ROUTE_RIP:
			val = ZEBRA_ROUTE_RIP;
			break;
		case EIGRP_ROUTE_IGRP:
			val = ZEBRA_ROUTE_FRP;
			break;
		case EIGRP_ROUTE_BGP:
			val = ZEBRA_ROUTE_BGP;
			break;
		case EIGRP_ROUTE_EIGRP:
			val = ZEBRA_ROUTE_FRP;
			break;
		}
	}
	else if(zebra)
	{
		switch(zebra)
		{
		case ZEBRA_ROUTE_SYSTEM:
			val = EIGRP_ROUTE_SYSTEM;
			break;
		case ZEBRA_ROUTE_KERNEL:
			val = EIGRP_ROUTE_KERNEL;
			break;
		case ZEBRA_ROUTE_CONNECT:
			val = EIGRP_ROUTE_CONNECT;
			break;
		case ZEBRA_ROUTE_STATIC:
			val = EIGRP_ROUTE_STATIC;
			break;
		case ZEBRA_ROUTE_OSPF:
			val = EIGRP_ROUTE_OSPF;
			break;
		case ZEBRA_ROUTE_RIP:
			val = EIGRP_ROUTE_RIP;
			break;
		//case ZEBRA_ROUTE_FRP:
		//	val = EIGRP_ROUTE_IGRP;
		//	break;
		case ZEBRA_ROUTE_BGP:
			val = EIGRP_ROUTE_BGP;
			break;
		case ZEBRA_ROUTE_FRP:
			val = EIGRP_ROUTE_EIGRP;
			break;
		}
	}
	return val;
}
/****************************************************************************/
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
static int (*EigrpRouteCallBack)(int ,int, long, long, long, int, int, int);
/****************************************************************************/
/****************************************************************************/
int zebraEigrpRouteHandleCallBack(int (*callback)(int, int, long, long, long, int, int, int))
{
	EigrpRouteCallBack = callback;
	return SUCCESS;
}
#endif// (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
/****************************************************************************/
/****************************************************************************/
//���ã�������zebra����·�������Ϣ
static int zebraEigrpAddRoute (int proto, long dest, long next, long mask, int metric, int distance, int ifindex)
{
#ifdef EIGRP_PLAT_ZEBRA
	struct prefix_ipv4 p;
	struct in_addr nexthop;
	int type;
	type = zebra_eigrp_route_type_exchange(proto, 0);
	nexthop.s_addr = HTONL(mask);
	memset (&p, 0, sizeof (struct prefix_ipv4));
	p.family = AF_INET;
	p.prefix.s_addr = HTONL(dest);
	p.prefixlen = ip_masklen (nexthop);
	nexthop.s_addr = HTONL(next);
	return eigrp_zebra_ipv4_update (type, ZEBRA_IPV4_ROUTE_ADD, &p, &nexthop, metric, distance, ifindex);
#else
	int type;
	type = zebra_eigrp_route_type_exchange(proto, 0);
	if(EigrpRouteCallBack)
		(* EigrpRouteCallBack)(RTM_ADD, type, dest, next, mask, metric, distance, ifindex);
#endif //EIGRP_PLAT_ZEBRA		
}
/****************************************************************************/
/****************************************************************************/
//���ã�������zebra����·��ɾ����Ϣ
static int zebraEigrpDelRoute (int proto, long dest, long next, long mask, int metric, int distance, int ifindex)
{
#ifdef EIGRP_PLAT_ZEBRA
	struct prefix_ipv4 p;
	struct in_addr nexthop;
	int type;
	type = zebra_eigrp_route_type_exchange(proto, 0);
	nexthop.s_addr = HTONL(mask);
	memset (&p, 0, sizeof (struct prefix_ipv4));
	p.family = AF_INET;
	p.prefix.s_addr = HTONL(dest);
	p.prefixlen = ip_masklen (nexthop);
	nexthop.s_addr = HTONL(next);
	return eigrp_zebra_ipv4_update (type, ZEBRA_IPV4_ROUTE_DELETE,  &p, &nexthop, metric, distance, ifindex);
#else
	int type;
	type = zebra_eigrp_route_type_exchange(proto, 0);	
	if(EigrpRouteCallBack)
		(* EigrpRouteCallBack)(RTM_DELETE, type, dest, next, mask, metric, distance, ifindex);
#endif //EIGRP_PLAT_ZEBRA
}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
//Eigrp·����ɢ�������ط�
static int zebraEigrpClientRouteHandle(int type, ZebraEigrpRt_pt rt)
{
	if(type == RTM_ADD)
	{
		return zebraEigrpAddRoute (rt->proto, rt->target, rt->gateway, rt->mask, rt->metric, rt->distance,rt->ifindex);
	}
	else if(type == RTM_DELETE)
	{
		return zebraEigrpDelRoute (rt->proto, rt->target, rt->gateway, rt->mask, rt->metric, rt->distance,rt->ifindex);
	}
	return FAILURE;
}
/****************************************************************************/
/****************************************************************************/
#ifdef EIGRP_PLAT_ZEBRA	
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int zebraEigrpRedistributeSet (char * type)
{
	if(EigrpMaster && EigrpMaster->zclient)	
	{
		zclient_redistribute (ZEBRA_REDISTRIBUTE_ADD, EigrpMaster->zclient, proto_redistnum(AFI_IP, type));
	}
	return CMD_SUCCESS;
}

int zebraEigrpRedistributeUnset (char *  type)
{
	if(EigrpMaster && EigrpMaster->zclient)	
	{
		zclient_redistribute (ZEBRA_REDISTRIBUTE_DELETE, EigrpMaster->zclient, proto_redistnum(AFI_IP, type));
	}
	return CMD_SUCCESS;
}

int zebraEigrpRedistributeDefaultSet  ()
{
	if(EigrpMaster && EigrpMaster->zclient)	
	{	
		zclient_redistribute_default (ZEBRA_REDISTRIBUTE_DEFAULT_ADD, EigrpMaster->zclient);
	}
	return CMD_SUCCESS;
}

int zebraEigrpRedistributeDefaultUnset ()
{
	if(EigrpMaster && EigrpMaster->zclient)	
	{	
		zclient_redistribute_default (ZEBRA_REDISTRIBUTE_DEFAULT_DELETE, EigrpMaster->zclient);
	}
	return CMD_SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
static int EigrpVtyExchange(S8 *src, S8 *dest, int size)
{
	int i = 0,j = 0;
	for(i = 0; i < size; i++)
	{
		if(src[i]=='\n')
		{
			dest[j++] = '\r';
			dest[j++] = '\n';
		}
		else
			dest[j++] = src[i];
	}
	return j;
}
/************************************************************************************/
/************************************************************************************/
//����vty��Ϣ������ˣ�������vty_out����
static int EigrpPortVtyPrint(struct vty *vty, S8 *buffer)
{
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif	
	S8 dest[4096];
	EigrpPortMemSet(dest, 0, 4096);
	EigrpVtyExchange(buffer, dest, MIN(EigrpPortStrLen(buffer), 4096));
	vty_out(vty, "%s", dest);
	EigrpPortMemSet(buffer, 0, EigrpPortStrLen(buffer));
	//vty_ping_flush (vty, 1);//quanbu
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	buffer_flush_all (vty->obuf, vty->fd);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	return SUCCESS;
}
/************************************************************************************/
S32	EigrpVtyOut(U8 *name, void *pCli, U32 id, U32 flag, U8 *buffer)
{
   if(pCli != NULL)
	  EigrpPortVtyPrint(pCli, (S8 *)buffer);
	else
	  EigrpPortMemSet(buffer, 0, EigrpPortStrLen(buffer));
	return	SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
//����log��Ϣ������ˣ�������zlog_xxx����
static int EigrpDebug(U8 *name, void *pCli, U32 id, U32 flag, U8 *buffer)
{
	zlog_debug(buffer);
	//fprintf(stderr, buffer);
	//printf("%s", buffer);
	EigrpPortMemSet(buffer, 0, EigrpPortStrLen(buffer));
	return SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
static EigrpIntf_pt	eigrp_interface = NULL;
static EigrpIntf_pt eigrp_get_current_interface(void)
{
	return eigrp_interface;
}
static int eigrp_set_current_interface(int index)
{
	if( (!gpEigrp)||(!EigrpMaster))
	{
		fprintf (stdout, "EIGRP protocol is not running, Failed config.\n");
		return -1;
	}	
	eigrp_interface = EigrpIntfFindByIndex(index);
	return 0;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
static int eigrp_router_cmd(struct vty *vty, int argc, char *argv[])
{
	  int as = atoi(argv[0]);
	  if( (!gpEigrp)||(!EigrpMaster))
	  {
		  vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		  return CMD_WARNING;
	  }	  
	  if(EigrpIpFindPdb(as))
	  {
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	  
		  SET_FLAG(sigevent_opt.babel, 1);
		  vty->node = FRP_NODE;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	 		  
		  EigrpMaster->asnum = as;
		  return CMD_SUCCESS;
	  }
	  if(EigrpCmdApiRouterEigrp(FALSE, as)== FAILURE)
	  {
		  vty_out (vty, "Can't not active FRP routting%s", VTY_NEWLINE);
		  return CMD_WARNING;
	  }
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	  
	  SET_FLAG(sigevent_opt.babel, 1);
	  vty->node = FRP_NODE;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	  

	  EigrpMaster->asnum = as;
	  vty->node = FRP_NODE;
	  //vty->index = EigrpIpFindPdb(EigrpMaster->asnum); process
	  //vty->index = EigrpMaster->asnum;	  
	  return CMD_SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
static int eigrp_no_router_cmd(struct vty *vty, int argc, char *argv[])
{
	  int as = atoi(argv[0]);	
	  if( (!gpEigrp)||(!EigrpMaster))
	  {
			vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
			return CMD_WARNING;
	  }
	  if(EigrpMaster)
	  {
		  if(EigrpMaster->asnum != as)
		  {
			  vty_out (vty, "FRP process of %d is not active%s", as,VTY_NEWLINE);
			  return CMD_WARNING;
		  }
	  }
	  if(EigrpIpFindPdb(as)==NULL)
	  {
		  vty_out (vty, "FRP process of %d is not active%s", as,VTY_NEWLINE);
		  return CMD_WARNING;
	  }	  
	  if(EigrpCmdApiRouterEigrp(TRUE, as)== FAILURE)
	  {
		  vty_out (vty, "Can't not inactive FRP routting%s", VTY_NEWLINE);
		  return CMD_WARNING;
	  }	  
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	  
	  UNSET_FLAG(sigevent_opt.babel, 1);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	  
	  return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_routing_id(struct vty *vty, int argc, char *argv[])
{
	int ret = -1;
	struct in_addr router_id;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)		  
	ret = inet_aton_q (argv[0], &router_id);
#else
	ret = inet_aton (argv[0], &router_id);	  
#endif	  
	if (!ret)
	{
	      vty_out (vty, "Please specify Router ID by A.B.C.D Invalid value:%s%s", argv[0], VTY_NEWLINE);
	      return CMD_WARNING;
	}
	if(zebraEigrpCmdApiRouterId(FALSE, EigrpMaster->asnum, NTOHL(router_id.s_addr))== FAILURE)
	{
		  vty_out (vty, "Can't not setting FRP routting router id%s", VTY_NEWLINE);
		  return CMD_WARNING;
	}	
	EigrpMaster->router_id = NTOHL(router_id.s_addr);
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_no_routing_id(struct vty *vty, int argc, char *argv[])
{
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	if(zebraEigrpCmdApiRouterId(TRUE, EigrpMaster->asnum, EigrpMaster->router_id)== FAILURE)
	{
		  vty_out (vty, "Can't not reset FRP routting router id%s", VTY_NEWLINE);
		  return CMD_WARNING;
	}	
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_network_cmd(struct vty *vty, int argc, char *argv[])
{
	  struct prefix_ipv4 p;
	  int ret;
	  U32	net, mask;
	  if( (!gpEigrp)||(!EigrpMaster))
	  {
			vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
			return CMD_WARNING;
	  }
	  if( (argc != 1)||(argv[0]==NULL))
	  {
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	  }	  
	  VTY_GET_IPV4_PREFIX ("network prefix", p, argv[0]);
	  net = NTOHL(p.prefix.s_addr);
	  mask = EIGRP_PREFIX_TO_MASK(p.prefixlen);
	  //vty_out (vty, "%s:0x%x 0x%x%s",__func__,net,mask,VTY_NEWLINE);
	  ret = EigrpCmdApiNetwork(FALSE, EigrpMaster->asnum, net ,mask);
	  if (ret == FAILURE)
	  {
	      vty_out (vty, "There is already same network statement.%s", VTY_NEWLINE);
	      return CMD_WARNING;
	  }
	  return CMD_SUCCESS;
}
static int eigrp_no_network_cmd(struct vty *vty, int argc, char *argv[])
{
	  struct prefix_ipv4 p;
	  int ret;
	  U32	net, mask;
	  if( (!gpEigrp)||(!EigrpMaster))
	  {
			vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
			return CMD_WARNING;
	  }
	  if( (argc != 1)||(argv[0]==NULL))
	  {
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	  }	
	  VTY_GET_IPV4_PREFIX ("network prefix", p, argv[0]);
	  net = NTOHL(p.prefix.s_addr);
	  mask = EIGRP_PREFIX_TO_MASK(p.prefixlen);
	  ret = EigrpCmdApiNetwork(TRUE, EigrpMaster->asnum, net ,mask);
	  if (ret == FAILURE)
	    {
		  vty_out (vty, "Can't find specified network area configuration.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	    }
	  return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_redistribute_type_cmd(struct vty *vty, int argc, char *argv[])
{
	U32	ret = -1;	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
	ret = EigrpCmdApiRedistType(FALSE, EigrpMaster->asnum, argv[0], 0, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Can't find specified network area configuration.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	zebraEigrpRedistributeSet(argv[0]);
	return CMD_SUCCESS;
}
static int eigrp_no_redistribute_type_cmd(struct vty *vty, int argc, char *argv[])
{
	U32	ret = -1;	
	if( (argc != 1)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiRedistType(TRUE, EigrpMaster->asnum, argv[0], 0, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Can't find specified network area configuration.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	zebraEigrpRedistributeUnset(argv[0]);
	return CMD_SUCCESS;
}
/************************************************************************************/
#ifdef EIGRP_REDISTRIBUTE_METRIC
static int eigrp_redistribute_metric_cmd(struct vty *vty, int argc, char *argv[])
{
	U32	ret = -1;	
	U32 m1,  m2,  m3,  m4,  m5;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 6)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
	
	m1	= EigrpPortAtoi(argv[1]);
	m2	= EigrpPortAtoi(argv[2]);
	m3	= EigrpPortAtoi(argv[3]);
	m4	= EigrpPortAtoi(argv[4]);
	m5	= EigrpPortAtoi(argv[5]);
	
	ret = EigrpCmdApiRedistTypeMetric(FALSE, EigrpMaster->asnum, argv[0],  m1,  m2,  m3,  m4,  m5);
	if (ret == FAILURE)
	{
		vty_out (vty, "Can't find specified network area configuration.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	zebraEigrpRedistributeSet(argv[0]);
	return CMD_SUCCESS;
}
static int eigrp_no_redistribute_metric_cmd(struct vty *vty, int argc, char *argv[])
{
	U32	ret = -1;	
	if( (argc != 1)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}
	
	ret = EigrpCmdApiRedistTypeMetric(TRUE, EigrpMaster->asnum, argv[0],  0,  0,  0,  0,  0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Can't find specified network area configuration.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	zebraEigrpRedistributeUnset(argv[0]);
	return CMD_SUCCESS;
}
#endif//EIGRP_REDISTRIBUTE_METRIC
/************************************************************************************/
static int eigrp_network_metric_cmd(struct vty *vty, int argc, char *argv[])
{
	S32		retVal;
	U32		ipNet, mask;
	U32		vMetric[6]; /*0-delay 1-bw 2-mtu 3-hopcount 4-reliability 5-load*/
	struct prefix_ipv4 p;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 6))
	{
		vty_out (vty, "%% Invalid value number: %d%s",argc, VTY_NEWLINE);
		return CMD_WARNING;
	}
	VTY_GET_IPV4_PREFIX ("network prefix", p, argv[0]);
	ipNet = NTOHL(p.prefix.s_addr);
	mask = EIGRP_PREFIX_TO_MASK(p.prefixlen);
	ipNet = ipNet & mask;
	if(!ipNet){
		return CMD_WARNING;
	}
	vMetric[0]	= EigrpPortAtoi(argv[1]) * 25600;
	vMetric[1]	= EigrpPortAtoi(argv[2]) * 256;
	vMetric[2]	= EigrpPortAtoi(argv[3]);
	vMetric[3]	= 0;
	vMetric[4]	= EigrpPortAtoi(argv[4]);
	vMetric[5]	= EigrpPortAtoi(argv[5]);

	retVal = EigrpCmdApiNetMetric(FALSE, ipNet, vMetric);
	if (retVal == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	return CMD_SUCCESS;
}

static int eigrp_no_network_metric_cmd(struct vty *vty, int argc, char *argv[])
{
    struct prefix_ipv4 p;
	S32		retVal;
	U32		ipNet, mask;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1))
	{
		vty_out (vty, "%% Invalid value number: %d%s",argc, VTY_NEWLINE);
		return CMD_WARNING;
	}
	VTY_GET_IPV4_PREFIX ("network prefix", p, argv[0]);
	ipNet = NTOHL(p.prefix.s_addr);
	mask = EIGRP_PREFIX_TO_MASK(p.prefixlen);
	ipNet = ipNet & mask;
	if(!ipNet){
		return CMD_WARNING;
	}
	retVal = EigrpCmdApiNetMetric(TRUE, ipNet, NULL);
	if (retVal == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_default_route_cmd(struct vty *vty, int argc, char *argv[])
{
	U32	ret = -1;	
	U32 m1,  m2,  m3,  m4,  m5;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
	if(EigrpPortMemCmp(argv[0], "in", 2) == 0)
	{
		vty_out (vty, "EigrpCmdApiDefaultRouteIn%s",VTY_NEWLINE);
		ret = EigrpCmdApiDefaultRouteIn(FALSE, EigrpMaster->asnum);
	}
	else if(EigrpPortMemCmp(argv[0], "out", 2) == 0)
	{
		vty_out (vty, "EigrpCmdApiDefaultRouteOut%s",VTY_NEWLINE);
		ret = EigrpCmdApiDefaultRouteOut(FALSE, EigrpMaster->asnum);
	}
	if (ret == FAILURE)
	{
		vty_out (vty, "Can't find specified configuration.:%s as %d %s",argv[0],EigrpMaster->asnum,VTY_NEWLINE);
		return CMD_WARNING;
	}
	//zebraEigrpRedistributeSet(argv[0]);
	return CMD_SUCCESS;
}
static int eigrp_no_default_route_cmd(struct vty *vty, int argc, char *argv[])
{
	U32	ret = -1;	
	if( (argc != 1)||(argv[0]==NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(EigrpPortMemCmp(argv[0], "in", 2) == 0)
	{
		vty_out (vty, "EigrpCmdApiDefaultRouteIn%s",VTY_NEWLINE);
		ret = EigrpCmdApiDefaultRouteIn(0, EigrpMaster->asnum);
	}
	else if(EigrpPortMemCmp(argv[0], "out", 2) == 0)
	{
		vty_out (vty, "EigrpCmdApiDefaultRouteOut%s",VTY_NEWLINE);
		ret = EigrpCmdApiDefaultRouteOut(TRUE, EigrpMaster->asnum);
	}
	if (ret == FAILURE)
	{
		vty_out (vty, "Can't find specified configuration.:%s%s",argv[0],VTY_NEWLINE);
		return CMD_WARNING;
	}
	//zebraEigrpRedistributeUnset(argv[0]);
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_hello_interval_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiHelloInterval(FALSE,EigrpPortAtoi(argv[0]), ife->name, EigrpPortAtoi(argv[1]));
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      	return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_hello_interval_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiHelloInterval(TRUE,EigrpPortAtoi(argv[0]), ife->name, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      	return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_hold_interval_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;	
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ret = EigrpCmdApiHoldtime(FALSE,EigrpPortAtoi(argv[0]), ife->name, EigrpPortAtoi(argv[1]));
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      	return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_hold_interval_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;	
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
 	EigrpCmdApiHoldtime(TRUE,EigrpPortAtoi(argv[0]), ife->name, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_metric_weights_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32	k1, k2, k3, k4, k5;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 5)||(argv[0] == NULL)||(argv[1] == NULL)||(argv[2] == NULL)||(argv[3] == NULL)||(argv[4] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}	
	k1	= EigrpPortAtoi(argv[0]);
	k2	= EigrpPortAtoi(argv[1]);
	k3	= EigrpPortAtoi(argv[2]);
	k4	= EigrpPortAtoi(argv[3]);
	k5	= EigrpPortAtoi(argv[4]);

	ret = EigrpCmdApiMetricWeights(FALSE, EigrpMaster->asnum, k1, k2, k3, k4, k5);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_metric_weights_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	ret = EigrpCmdApiMetricWeights(TRUE, EigrpMaster->asnum, 0, 0, 0, 0, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_default_metric_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	S32	band, delay, reli, load, mtu;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 5)||(argv[0] == NULL)||(argv[1] == NULL)||(argv[2] == NULL)||(argv[3] == NULL)||(argv[4] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
	band	= EigrpPortAtoi(argv[0]);
	delay	= EigrpPortAtoi(argv[1]);
	reli		= EigrpPortAtoi(argv[2]);
	load		= EigrpPortAtoi(argv[3]);
	mtu		= EigrpPortAtoi(argv[4]);

	if(band < 1/* || band > 0xFFFFFFFF*/ ||
					delay < 0/* || delay > 0xFFFFFFFF*/ ||
					reli < 0 || reli > 255 ||
					load < 1 || load > 255 ||
					mtu < 1/* || mtu > 0xFFFFFFFF*/){
		vty_out (vty, "Invalid value..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiDefaultMetric(FALSE, EigrpMaster->asnum, band, delay, reli, load, mtu);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_default_metric_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiDefaultMetric(TRUE, EigrpMaster->asnum, 0, 0, 0, 0, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_distance_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	int in,ex;
	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(argv[0])
		in	= EigrpPortAtoi(argv[0]);
	else
		in	= EIGRP_DEF_DISTANCE_INTERNAL;
	if(argv[1])	
		ex	= EigrpPortAtoi(argv[1]);	
	else
		ex = EIGRP_DEF_DISTANCE_EXTERNAL;
	
	ret = EigrpCmdApiDistance(FALSE, EigrpMaster->asnum, in, ex);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_distance_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiDistance(TRUE, EigrpMaster->asnum, 0, 0);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_passive_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;

	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	
	ret = EigrpCmdApiPassiveInterface(FALSE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_passive_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiPassiveInterface(TRUE, EigrpPortAtoi(argv[0]),  ife->name);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_auto_summary_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAutosummary(FALSE, EigrpMaster->asnum);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue on as:%d.%s",
		      EigrpMaster->asnum, VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_auto_summary_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAutosummary(TRUE, EigrpMaster->asnum);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue on as:%d.%s",
		      EigrpMaster->asnum, VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_neighber_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32	 ipAddr;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
	EigrpUtilConvertStr2Ipv4(&ipAddr,argv[0]);
	ret = EigrpCmdApiNeighbor(FALSE, EigrpMaster->asnum, ipAddr);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_neighber_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32	 ipAddr;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
	EigrpUtilConvertStr2Ipv4(&ipAddr,argv[0]);
	ret = EigrpCmdApiNeighbor(TRUE, EigrpMaster->asnum, ipAddr);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_invisible_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiInvisibleInterface(FALSE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_invisible_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	ret = EigrpCmdApiInvisibleInterface(TRUE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_summary_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32	ipAddr, ipMask;
	struct prefix_ipv4 p;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	VTY_GET_IPV4_PREFIX ("network prefix", p, argv[1]);
	ipAddr = NTOHL(p.prefix.s_addr);
	ipMask = EIGRP_PREFIX_TO_MASK(p.prefixlen);
	
	ret = EigrpCmdApiSummaryAddress(FALSE, EigrpPortAtoi(argv[0]), ife->name, ipAddr, ipMask);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_summary_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32	ipAddr, ipMask;
    	struct prefix_ipv4 p;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	VTY_GET_IPV4_PREFIX ("network prefix", p, argv[1]);
	ipAddr = NTOHL(p.prefix.s_addr);
	ipMask = EIGRP_PREFIX_TO_MASK(p.prefixlen);
	
	ret = EigrpCmdApiSummaryAddress(TRUE, EigrpPortAtoi(argv[0]), ife->name, ipAddr, ipMask);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_split_horizon_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;

	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiSplitHorizon(FALSE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue on '%s' as:%d.%s",
		    ife->name, EigrpPortAtoi(argv[0]), VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_split_horizon_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;

	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiSplitHorizon(TRUE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		vty_out (vty, "Failed to send eigrp command to command queue on '%s' as:%d.%s",
		      ife->name, EigrpPortAtoi(argv[0]), VTY_NEWLINE);
		return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_key_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;

	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAuthKeyStr(FALSE, EigrpPortAtoi(argv[0]), ife->name, argv[1]);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_key_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;

	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL))
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAuthKeyStr(TRUE, EigrpPortAtoi(argv[0]), ife->name, NULL);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_key_id_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAuthKeyid(FALSE, EigrpPortAtoi(argv[0]), ife->name, EigrpPortAtoi(argv[1]));
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_key_id_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAuthKeyid(TRUE, EigrpPortAtoi(argv[0]), ife->name, 0);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_authmode_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAuthMd5mode(FALSE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
static int eigrp_no_authmode_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiAuthMd5mode(TRUE, EigrpPortAtoi(argv[0]), ife->name);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}

static int eigrp_bandwidth_percent_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32 percent;	
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	percent = EigrpPortAtoi(argv[1]);
	if(percent < 1 || percent > 100){
		vty_out (vty,"bandwidth value is error.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	ret = EigrpCmdApiBwPercent(FALSE, EigrpPortAtoi(argv[0]), ife->name, percent);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	//ifp->bandwidth = bandwidth;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	//ife->bandwidth = bandwidth;
	return	CMD_SUCCESS;
}
static int eigrp_no_bandwidth_percent_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiBwPercent(TRUE, EigrpPortAtoi(argv[0]), ife->name, 0);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	//ifp->bandwidth = EIGRP_DEF_IF_BANDWIDTH;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	//ife->bandwidth = EIGRP_DEF_IF_BANDWIDTH;
	return	CMD_SUCCESS;
}

static int eigrp_bandwidth_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32 bandwidth;	
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	bandwidth = EigrpPortAtoi(argv[1]);
	if(bandwidth < 1 || bandwidth > 10000000){
		vty_out (vty,"bandwidth value is error.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	ret = EigrpCmdApiBandwidth(FALSE, EigrpPortAtoi(argv[0]), ife->name, bandwidth);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	ifp->bandwidth = bandwidth;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	ife->bandwidth = bandwidth;
	return	CMD_SUCCESS;
}
static int eigrp_no_bandwidth_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiBandwidth(TRUE, EigrpPortAtoi(argv[0]), ife->name, 0);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	ifp->bandwidth = EIGRP_DEF_IF_BANDWIDTH;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	ife->bandwidth = EIGRP_DEF_IF_BANDWIDTH;
	return	CMD_SUCCESS;
}

static int eigrp_delay_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	U32 delay;	
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 2)||(argv[0] == NULL)||(argv[1] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
 	delay = EigrpPortAtoi(argv[1]);
	if(delay < 1 || delay > 10000000){
		vty_out (vty,"bandwidth value is error.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	delay *= 25600;/*25600 is 1ms*/
	ret = EigrpCmdApiDelay(FALSE, EigrpPortAtoi(argv[0]), ife->name, delay);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	ife->delay = delay;
	return	CMD_SUCCESS;
}
static int eigrp_no_delay_cmd(struct vty *vty, int argc, char *argv[])
{    
	int ret;
	EigrpIntf_pt ife;
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
 	struct interface *ifp = vty->index;
 	eigrp_set_current_interface(ifp->ifindex);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if( (argc != 1)||(argv[0] == NULL) )
	{
		vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"value null", VTY_NEWLINE);
		return CMD_WARNING;
	}
 	ife = eigrp_get_current_interface();
	if(ife == NULL)
	{
		vty_out (vty, "Failed to find eigrp interface to setting param.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	ret = EigrpCmdApiDelay(TRUE, EigrpPortAtoi(argv[0]), ife->name, 0);
	if (ret == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue.%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	ife->delay = EIGRP_DEF_IF_DELAY * 25600;
	return	CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_debug_cmd(struct vty *vty, int argc, char *argv[])
{
	int type = -1;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if (strncmp (argv[0], "a", 1) == 0)
	    type = EigrpCmdApiDebugAllByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "p", 1) == 0)
		type = EigrpCmdApiDebugPacketByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "e", 1) == 0)
	    type = EigrpCmdApiDebugEventByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "ta", 2) == 0)
	    type = EigrpCmdApiDebugTaskByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "ti", 2) == 0)
	    type = EigrpCmdApiDebugTimerByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "r", 1) == 0)
	    type = EigrpCmdApiDebugRouteByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "i", 1) == 0)
	    type = EigrpCmdApiDebugInternalByTerm("shell", NULL, 0, EigrpDebug);
	if (type == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
static int eigrp_debug_packet_cmd(struct vty *vty, int argc, char *argv[])
{
	int type = -1;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	if((argc>=1)&&(argv[0]!= NULL))
	{
		if (strncmp (argv[0], "s", 1) == 0)
		{
			if(argc == 1)
				type = EigrpCmdApiDebugPacketSendByTerm("shell", NULL, 0, EigrpDebug);
			else if((argc==2)&&(argv[1]!= NULL))
			{
				if (strncmp (argv[1], "u", 1) == 0)
					type = EigrpCmdApiDebugPacketSendUpdateByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "q", 1) == 0)
					type = EigrpCmdApiDebugPacketSendQueryByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "r", 1) == 0)
					type = EigrpCmdApiDebugPacketSendReplyByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "h", 1) == 0)
					type = EigrpCmdApiDebugPacketSendHelloByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "a", 1) == 0)
					type = EigrpCmdApiDebugPacketSendAckByTerm("shell", NULL, 0, EigrpDebug);
			}
		}
		else if (strncmp (argv[0], "r", 1) == 0)
		{
			if(argc == 1)
				type = EigrpCmdApiDebugPacketRecvByTerm("shell", NULL, 0, EigrpDebug);
			else if((argc==2)&&(argv[1]!= NULL))
			{
				if (strncmp (argv[1], "u", 1) == 0)
					type = EigrpCmdApiDebugPacketRecvUpdateByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "q", 1) == 0)
					type = EigrpCmdApiDebugPacketRecvQueryByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "r", 1) == 0)
					type = EigrpCmdApiDebugPacketRecvReplyByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "h", 1) == 0)
					type = EigrpCmdApiDebugPacketRecvHelloByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "a", 1) == 0)
					type = EigrpCmdApiDebugPacketRecvAckByTerm("shell", NULL, 0, EigrpDebug);
			}				
		}
	}
	if (type == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_debug_detail_cmd(struct vty *vty, int argc, char *argv[])
{
	int type = -1;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if((argc==1)&&(argv[0]!= NULL))
	{
		if (strncmp (argv[0], "u", 1) == 0)
			type = EigrpCmdApiDebugPacketDetailUpdateByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "q", 1) == 0)
			type = EigrpCmdApiDebugPacketDetailQueryByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "r", 1) == 0)
			type = EigrpCmdApiDebugPacketDetailReplyByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "h", 1) == 0)
			type = EigrpCmdApiDebugPacketDetailHelloByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "a", 1) == 0)
			type = EigrpCmdApiDebugPacketDetailAckByTerm("shell", NULL, 0, EigrpDebug);
	}
	if (type == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_no_debug_cmd(struct vty *vty, int argc, char *argv[])
{
	int type = -1;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if (strncmp (argv[0], "a", 1) == 0)
	    type = EigrpCmdApiNoDebugAllByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "p", 1) == 0)
		type = EigrpCmdApiNoDebugPacketByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "e", 1) == 0)
	    type = EigrpCmdApiNoDebugEventByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "ta", 2) == 0)
	    type = EigrpCmdApiNoDebugTaskByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "ti", 2) == 0)
	    type = EigrpCmdApiNoDebugTimerByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "r", 1) == 0)
	    type = EigrpCmdApiNoDebugRouteByTerm("shell", NULL, 0, EigrpDebug);
	else if (strncmp (argv[0], "i", 1) == 0)
	    type = EigrpCmdApiNoDebugInternalByTerm("shell", NULL, 0, EigrpDebug);
	if (type == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
static int eigrp_no_debug_packet_cmd(struct vty *vty, int argc, char *argv[])
{
	int type = -1;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}

	if((argc>=1)&&(argv[0]!= NULL))
	{
		if (strncmp (argv[0], "s", 1) == 0)
		{
			if(argc == 1)
				type = EigrpCmdApiNoDebugPacketSendByTerm("shell", NULL, 0, EigrpDebug);
			else if((argc==2)&&(argv[1]!= NULL))
			{
				if (strncmp (argv[1], "u", 1) == 0)
					type = EigrpCmdApiNoDebugPacketSendUpdateByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "q", 1) == 0)
					type = EigrpCmdApiNoDebugPacketSendQueryByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "r", 1) == 0)
					type = EigrpCmdApiNoDebugPacketSendReplyByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "h", 1) == 0)
					type = EigrpCmdApiNoDebugPacketSendHelloByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "a", 1) == 0)
					type = EigrpCmdApiNoDebugPacketSendAckByTerm("shell", NULL, 0, EigrpDebug);
			}
		}
		else if (strncmp (argv[0], "r", 1) == 0)
		{
			if(argc == 1)
				type = EigrpCmdApiNoDebugPacketRecvByTerm("shell", NULL, 0, EigrpDebug);
			else if((argc==2)&&(argv[1]!= NULL))
			{
				if (strncmp (argv[1], "u", 1) == 0)
					type = EigrpCmdApiNoDebugPacketRecvUpdateByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "q", 1) == 0)
					type = EigrpCmdApiNoDebugPacketRecvQueryByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "r", 1) == 0)
					type = EigrpCmdApiNoDebugPacketRecvReplyByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "h", 1) == 0)
					type = EigrpCmdApiNoDebugPacketRecvHelloByTerm("shell", NULL, 0, EigrpDebug);
				else if (strncmp (argv[1], "a", 1) == 0)
					type = EigrpCmdApiNoDebugPacketRecvAckByTerm("shell", NULL, 0, EigrpDebug);
			}				
		}
	}
	if (type == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_no_debug_detail_cmd(struct vty *vty, int argc, char *argv[])
{
	int type = -1;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if((argc==1)&&(argv[0]!= NULL))
	{
		if (strncmp (argv[0], "u", 1) == 0)
			type = EigrpCmdApiNoDebugPacketDetailUpdateByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "q", 1) == 0)
			type = EigrpCmdApiNoDebugPacketDetailQueryByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "r", 1) == 0)
			type = EigrpCmdApiNoDebugPacketDetailReplyByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "h", 1) == 0)
			type = EigrpCmdApiNoDebugPacketDetailHelloByTerm("shell", NULL, 0, EigrpDebug);
		else if (strncmp (argv[0], "a", 1) == 0)
			type = EigrpCmdApiNoDebugPacketDetailAckByTerm("shell", NULL, 0, EigrpDebug);
	}
	if (type == FAILURE)
	{
		  vty_out (vty, "Failed to send eigrp command to command queue..%s",VTY_NEWLINE);
	      return CMD_WARNING;
	}	
	return	CMD_SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
static int eigrp_show_debugging_cmd(struct vty *vty, int argc, char *argv[])
{
	//EigrpDbgCtrl_st	dbg;
	EigrpDbgCtrl_pt	pDbg;
	U32	flagTmp;
	pDbg	= EigrpUtilDbgCtrlFind("shell", NULL, 0);
	if(!pDbg){
		vty_out (vty, "Failed to Find eigrp debug db.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	vty_out(vty, "EIGRP debugging status:%s", VTY_NEWLINE);

	if(pDbg->flag  == 0xffffffff){ 
		vty_out(vty, "ALL EIGRP debugging is on%s", VTY_NEWLINE); 
		return CMD_SUCCESS;
	}

	if(pDbg->flag & DEBUG_EIGRP_EVENT){
		vty_out(vty, "  EIGRP event debugging is on%s", VTY_NEWLINE);
	}

	if(pDbg->flag & DEBUG_EIGRP_PACKET){
		flagTmp	= DEBUG_EIGRP_PACKET_DETAIL;
		flagTmp	|= DEBUG_EIGRP_ROUTE;
		vty_out(vty, "  EIGRP packet%s debugging is on%s", 
										(pDbg->flag & flagTmp)? " detail" : "", VTY_NEWLINE);
	}else{
		if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND){
			vty_out(vty, "  EIGRP packet send%s debugging is on%s",
											(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL)? " detail" : "",	VTY_NEWLINE);
		}else{
	  		if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_UPDATE){
				vty_out(vty, "  EIGRP packet send update debugging is on%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_QUERY){
				vty_out(vty, "  EIGRP packet send query debugging is on%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_REPLY){
				vty_out(vty, "  EIGRP packet send reply debugging is on%s", VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_HELLO){
				vty_out(vty, "  EIGRP packet send hello debugging is on%s",VTY_NEWLINE);
			}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_ACK){
				vty_out(vty, "  EIGRP packet send ack debugging is on%s",VTY_NEWLINE);
			}
		}
	  	
	  	if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV){
	    		vty_out(vty, "  EIGRP packet receive%s debugging is on%s", 
											(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL)? " detail" : "",VTY_NEWLINE);
	  	}else{
	  	 	if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_UPDATE){
		    		vty_out(vty, "  EIGRP packet receive update debugging is on%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_QUERY){
		    		vty_out(vty, "  EIGRP packet receive query debugging is on%s", VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_REPLY){
		    		vty_out(vty, "  EIGRP packet receive reply debugging is on%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_HELLO){
		    		vty_out(vty, "  EIGRP packet receive hello debugging is on%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_ACK){
		    		vty_out(vty, "  EIGRP packet receive ack debugging is on%s",VTY_NEWLINE);
			}
	  	}
	} 

	if(pDbg->flag & DEBUG_EIGRP_ROUTE){
		vty_out(vty, "  EIGRP Route-Manage debugging is on%s",VTY_NEWLINE);
	}

	if(pDbg->flag & DEBUG_EIGRP_TIMER){
		vty_out(vty, "  EIGRP timer debugging is on%s", VTY_NEWLINE);
	}

	if(pDbg->flag & DEBUG_EIGRP_TASK){
		vty_out(vty, "  EIGRP task debugging is on%s",VTY_NEWLINE);
	}
	return	CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_show_ip_interface_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	//show ip eigrp interfaces
	if(argc==0)
	{
		retVal	= EigrpCmdApiShowIntfByTerm("shell", vty, 0, EigrpVtyOut);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	else if( (argc==1)&&(argv[0]) )
	{

		//show ip eigrp interfaces if
		retVal	= EigrpCmdApiShowIntfIfByTerm("shell", vty, 0, EigrpVtyOut, argv[0]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}		
	}
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_show_ip_interface_detail_cmd(struct vty *vty, int argc, char *argv[])
{

	S32	retVal;
	//show ip eigrp interfaces detail
	retVal	= EigrpCmdApiShowIntfDetailByTerm("shell", vty, 0, EigrpVtyOut);
	if(retVal == FAILURE){
		return CMD_WARNING;
	}
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_show_ip_interface_as_cmd(struct vty *vty, int argc, char *argv[])
{
	//show ip eigrp interfaces as <1-65536> [detail]
	//show ip eigrp interfaces as <1-65536> [NAME] [detail] 
	S32	retVal;
	int as = atoi(argv[0]);
	if(argc==1)
	{
		//show ip eigrp interfaces as
		retVal	= EigrpCmdApiShowIntfAsByTerm("shell", vty, 0, EigrpVtyOut, 2);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	else if((argc==2)&&(argv[1]) )
	{

		//show ip eigrp interfaces as if 
		retVal	= EigrpCmdApiShowIntfAsIfByTerm("shell", vty, 0, EigrpVtyOut, as, argv[1]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_show_ip_interface_as_detail_cmd(struct vty *vty, int argc, char *argv[])
{
	//show ip eigrp interfaces as <1-65536> [detail]
	//show ip eigrp interfaces as <1-65536> [NAME] [detail] 
	S32	retVal;
	int as = atoi(argv[0]);
	if(argc==1)
	{
		//show ip eigrp interfaces as detail
		retVal	= EigrpCmdApiShowIntfAsDetailByTerm("shell", vty, 0, EigrpVtyOut, as);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	else
	{
		//show ip eigrp interfaces as if detail
		retVal	= EigrpCmdApiShowIntfAsIfDetailByTerm("shell", vty, 0, EigrpVtyOut, as, argv[1]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;
}
/******************************************************************************************/
/******************************************************************************************/
static int eigrp_show_ip_neighbor_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	if(argc==0)
	{
		//show ip eigrp neighbor
		retVal	=EigrpCmdApiShowNeiByTerm("shell", vty, 0, EigrpVtyOut);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	else
	{
		retVal	=EigrpCmdApiShowNeiIfByTerm("shell", vty, 0, EigrpVtyOut, argv[0]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;

}
/******************************************************************************************/
static int eigrp_show_ip_neighbor_detail_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	if(argc==0)
	{
		//show ip eigrp neighbor
		retVal	=EigrpCmdApiShowNeiDetailByTerm("shell", vty, 0, EigrpVtyOut);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	else
	{
		retVal	=EigrpCmdApiShowNeiIfDetailByTerm("shell", vty, 0, EigrpVtyOut, argv[0]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;

}
/******************************************************************************************/
static int eigrp_show_ip_neighbor_as_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	int as = atoi(argv[0]);	
	if(argc==1)
	{
		//show ip eigrp neighbor
		retVal	=EigrpCmdApiShowNeiAsByTerm("shell", vty, 0, EigrpVtyOut, as);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	if(argc==2)
	{
		//show ip eigrp neighbor
		retVal	=EigrpCmdApiShowNeiAsIfByTerm("shell", vty, 0, EigrpVtyOut, as, argv[1]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;
}
/******************************************************************************************/
static int eigrp_show_ip_neighbor_as_detail_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	int as = atoi(argv[0]);	
	if(argc==1)
	{
		//show ip eigrp neighbor
		retVal	=EigrpCmdApiShowNeiAsDetailByTerm("shell", vty, 0, EigrpVtyOut, as);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	if(argc==2)
	{
		//show ip eigrp neighbor
		retVal	=EigrpCmdApiShowNeiAsIfDetailByTerm("shell", vty, 0, EigrpVtyOut, as, argv[1]);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;
}
/******************************************************************************************/
/******************************************************************************************/
static int eigrp_show_ip_topology_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	if(argc==0)
	{
		//show ip eigrp topo
		retVal	=EigrpCmdApiShowTopoByTerm("shell", vty, 0, EigrpVtyOut);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	if( (argc==1)&&(argv[0]) )
	{
		//show ip eigrp topo (summary|active|feasible|zero)
		if(strncmp(argv[0], "summary",2)==0)
		{
			retVal	=EigrpCmdApiShowTopoSummaryByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
		else if(strncmp(argv[0], "active",2)==0)
		{
			retVal	=EigrpCmdApiShowTopoActiveByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
		else if(strncmp(argv[0], "feasible",2)==0)
		{
			retVal	=EigrpCmdApiShowTopoFeasibleByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
		else if(strncmp(argv[0], "zero",2)==0)
		{
			retVal	=EigrpCmdApiShowTopoZeroByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
	}
	return CMD_SUCCESS;
}
/******************************************************************************************/
static int eigrp_show_ip_topology_as_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	int as = atoi(argv[0]);	
	if(argc==0)
	{
		//show ip eigrp topo as <1-65536>
		retVal	=EigrpCmdApiShowTopoAsByTerm("shell", vty, 0, EigrpVtyOut, as);
		if(retVal == FAILURE){
			return CMD_WARNING;
		}
	}
	return CMD_SUCCESS;
}
/******************************************************************************************/
/******************************************************************************************/
static int eigrp_show_ip_eigrp_cmd(struct vty *vty, int argc, char *argv[])
{
	S32	retVal;
	if( (argc==1)&&(argv[0]) )
	{
		//show ip eigrp  (protocol|traffic|route|struct)
		if(strncmp(argv[0], "protocol",2)==0)
		{
			retVal	=EigrpCmdApiShowProtoByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
		else if(strncmp(argv[0], "traffic",2)==0)
		{
			retVal	=EigrpCmdApiShowTrafficByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
		else if(strncmp(argv[0], "route",2)==0)
		{
			retVal	=EigrpCmdApiShowRouteByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
		else if(strncmp(argv[0], "struct",2)==0)
		{
			retVal	=EigrpCmdApiShowStructByTerm("shell", vty, 0, EigrpVtyOut);
			if(retVal == FAILURE){
				return CMD_WARNING;
			}
		}
	}
	return CMD_SUCCESS;
}
/************************************************************************************/
static int eigrp_show_interface_detail(struct vty *vty, int argc, char *argv[])
{    
	EigrpIntf_pt	pIntf;
	if( (!gpEigrp)||(!EigrpMaster))
	{
		vty_out (vty, "EIGRP protocol is not running, Failed config.%s",VTY_NEWLINE);
		return CMD_WARNING;
	}
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);

	for(pIntf = gpEigrp->intfLst; pIntf; pIntf = pIntf->next)
	{
		if(pIntf)
		{
			vty_out (vty, "%sInterface %s %s%s",VTY_NEWLINE,pIntf->name,
					BIT_TEST(pIntf->flags, EIGRP_INTF_FLAG_ACTIVE)? "up":"down",VTY_NEWLINE);
			
			vty_out (vty, " index %d metric %d mtu %d bandwidth %d kbps delay %d msec%s",
					pIntf->ifindex,
					pIntf->metric,
					pIntf->mtu,
					pIntf->bandwidth,
					pIntf->delay,
					VTY_NEWLINE);
			
			vty_out (vty, " flag %s %s",zebra_interface_flag_dump (pIntf->flags),VTY_NEWLINE);
			if(pIntf->ipAddr != 0)
			{
				vty_out (vty, " inet %s netmask %s ",
						EigrpUtilIp2Str(pIntf->ipAddr),
						EigrpUtilIp2Str(pIntf->ipMask));
				
				if(BIT_TEST(pIntf->flags, EIGRP_INTF_FLAG_POINT2POINT))
					vty_out (vty, "destnation %s %s",EigrpUtilIp2Str(pIntf->remoteIpAddr),VTY_NEWLINE);
				else
					vty_out (vty, "broadcast %s %s",EigrpUtilIp2Str(pIntf->ipAddr | (~pIntf->ipMask)),VTY_NEWLINE);
			}
		}
	}	
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);
	return CMD_SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
#define FRP_STR	"Fast and Reliable Routing Protocol (FRP)\n"
#define AS_CONFIG_STR "Autonomous system number config\n"

//router frp specific commands
DEFUN (router_frp,
       router_frp_cmd,
       "router frp <1-65536>",
       "Enable a routing process\n"
       FRP_STR
       AS_STR)
{
	return eigrp_router_cmd(vty, argc, argv);
}
DEFUN (no_router_frp,
       no_router_frp_cmd,
       "no router frp <1-65536>",
       NO_STR
       "Enable a routing process\n"
       FRP_STR
       AS_STR)
{
	return eigrp_no_router_cmd(vty, argc, argv);
}
DEFUN (frp_router_id,
       frp_router_id_cmd,
       "router-id A.B.C.D",
       "router-id for the FRP process\n"
       V4NOTATION_STR)
{
  return eigrp_routing_id(vty, argc, argv);
}

DEFUN (no_frp_router_id,
       no_frp_router_id_cmd,
       "no router-id",
       NO_STR
       "router-id for the FRP process\n")
{
  return eigrp_no_routing_id(vty,  argc,  argv);
}

DEFUN (frp_network_mask,
     frp_network_mask_cmd,
     "network A.B.C.D/M",
     "Enable routing on an IP network\n"
     "FRP network prefix\n")
{
	return eigrp_network_cmd(vty, argc, argv);
}

DEFUN(no_frp_network_mask,
     no_frp_network_mask_cmd,
     "no network A.B.C.D/M",
     NO_STR
     "Enable routing on an IP network\n"
     "FRP network prefix\n")
{
	return eigrp_no_network_cmd(vty, argc, argv);
}
/* frpd */
#undef QUAGGA_REDIST_STR_FRPD
#define QUAGGA_REDIST_STR_FRPD \
  "(kernel|connected|static|rip|ospf)"
#undef QUAGGA_REDIST_HELP_STR_FRPD
#define QUAGGA_REDIST_HELP_STR_FRPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" 


DEFUN (frp_redistribute_type,
     frp_redistribute_type_cmd,
     "redistribute " QUAGGA_REDIST_STR_FRPD,
     REDIST_STR
     QUAGGA_REDIST_HELP_STR_FRPD)
{
	return eigrp_redistribute_type_cmd(vty, argc, argv);
}

DEFUN (no_frp_redistribute_type,
     no_frp_redistribute_type_cmd,
     "no redistribute " QUAGGA_REDIST_STR_FRPD,
     NO_STR
     REDIST_STR
     QUAGGA_REDIST_HELP_STR_FRPD)
{
	return eigrp_no_redistribute_type_cmd(vty, argc, argv);
}
#ifdef EIGRP_REDISTRIBUTE_METRIC
DEFUN (frp_redistribute_metric,
     frp_redistribute_metric_cmd,
     "redistribute " QUAGGA_REDIST_STR_FRPD" <1-65536> <1-65536> <1-65536> <1-65536> <1-65536>",
     REDIST_STR
     QUAGGA_REDIST_HELP_STR_FRPD
     "metric\n"
     "metric\n"
     "metric\n"
     "metric\n")
{
	return eigrp_redistribute_metric_cmd(vty, argc, argv);
}

DEFUN (no_frp_redistribute_metric,
     no_frp_redistribute_metric_cmd,
     "no redistribute " QUAGGA_REDIST_STR_FRPD ,
     NO_STR
     REDIST_STR
     QUAGGA_REDIST_HELP_STR_FRPD)
{
	return eigrp_no_redistribute_metric_cmd(vty, argc, argv);
}
#endif//EIGRP_REDISTRIBUTE_METRIC
DEFUN (frp_distance,
     frp_distance_cmd,
     "distance <0-255> <0-255>",
     "Define an administrative distance\n"
     "FRP Administrative interior distance\n"
     "FRP Administrative exterior distance\n")
{
	return eigrp_distance_cmd(vty, argc, argv);
}

DEFUN (no_frp_distance,
     no_frp_distance_cmd,
     "no distance ",
     NO_STR
     "Define an administrative distance\n")
{
	return eigrp_no_distance_cmd(vty, argc, argv);
}
DEFUN (frp_passive,
     frp_passive_cmd,
     "ip frp as <1-65536> passive",
     IP_STR
     FRP_STR
     AS_CONFIG_STR
     AS_STR
     "setting passive type\n")
{
	return eigrp_passive_cmd(vty, argc, argv);
}

DEFUN (no_frp_passive,
     no_frp_passive_cmd,
     "no ip frp as <1-65536> passive",
     NO_STR
     IP_STR
     FRP_STR
     AS_CONFIG_STR
     AS_STR
     "setting passive type\n")
{
	return eigrp_no_passive_cmd(vty, argc, argv);
}

DEFUN (frp_invisible,
     frp_invisible_cmd,
     "ip frp as <1-65536> invisible",
     IP_STR
     FRP_STR
     AS_CONFIG_STR
     AS_STR
     "Invisible to neighbors\n")
{
	return eigrp_invisible_cmd(vty, argc, argv);
}

DEFUN (no_frp_invisible,
     no_frp_invisible_cmd,
     "no ip frp as <1-65536> invisible",
     NO_STR
     IP_STR
     FRP_STR
     AS_CONFIG_STR
     AS_STR
     "Invisible to neighbors\n")
{
	return eigrp_no_invisible_cmd(vty, argc, argv);
}


DEFUN (frp_network_metric,
     frp_network_metric_cmd,
     "network A.B.C.D/M metric <0-4294967295> <1-4294967295>  <1-1500> <0-255> <1-255>",
     "Enable routing on an IP network\n"
     "FRP network prefix\n"
     "set vector metric\n"
     "Delay metric, in 1 millisecond units\n"
     "Bandwidth in Mbits per second\n"
     "MTU\n"
     "Reliability metric where 255 is 100% reliable\n"
     "Effective bandwidth metric (Loading) where 255 is 100% loaded\n")
{

	return eigrp_network_metric_cmd(vty, argc, argv);
}

DEFUN (no_frp_network_metric,
     no_frp_network_metric_cmd,
     "no network A.B.C.D/M metric",
     NO_STR
     "Enable routing on an IP network\n"
     "FRP network prefix\n"
     "Set the metirc to destination network\n"
     )
{
	return eigrp_no_network_metric_cmd(vty, argc, argv);
}

DEFUN (frp_default_route,
	 frp_default_route_cmd,
     "default-information (in|out)",
     "Control distribution of default informtion\n"
     "Distribute in default route\n"
     "Distribute out default route\n")
{

	return eigrp_default_route_cmd(vty, argc, argv);
}

DEFUN (no_frp_default_route,
	 no_frp_default_route_cmd,
     "no default-information (in|out)",
     NO_STR
     "Control distribution of default informtion\n"
     "Distribute in default route\n"
     "Distribute out default route\n")
{
	return eigrp_no_default_route_cmd(vty, argc, argv);
}

DEFUN (frp_metric_weights,
	frp_metric_weights_cmd,
	"metric weights <0-65535> <0-65535> <0-65535> <0-65535> <0-65535>",
	"set metric commands\n"
	"set weights commands\n"
	"k1\n" 
	"k2\n"
	"k3\n"
	"k4\n"
	"k5\n")
{
	return	eigrp_metric_weights_cmd(vty, argc, argv);
}

DEFUN (no_frp_metric_weights,
	no_frp_metric_weights_cmd,
	"no metric weights",
	NO_STR
	"set metric commands\n"
	"set weights commands\n")
{
	return	eigrp_no_metric_weights_cmd(vty, argc, argv);
}
DEFUN (frp_default_metric,
	frp_default_metric_cmd,
	"default-metric <1-4294967295> <0-4294967295> <0-255> <1-255> <1-4294967295>",
	"default metric of redistributed routes\n"
	"Bandwidth in Kbits per second\n"
	"Delay metric, in 10 microsecond units\n"
	"Reliability metric where 255 is 100% reliable\n"
	"Effective bandwidth metric (Loading) where 255 is 100% loaded\n"
	"Maximum Transmission Unit metric of the path\n")
{
	return	eigrp_default_metric_cmd(vty, argc, argv);
}

DEFUN (no_frp_default_metric,
	no_frp_default_metric_cmd,
	"no default-metric",
	NO_STR
	"default metric of redistributed routes\n")
{
	return	eigrp_no_default_metric_cmd(vty, argc, argv);
}

DEFUN (frp_auto_summary,
	frp_auto_summary_cmd,
	"auto-summary",
	"Enable automatic network number summarization\n")
{
	return	eigrp_auto_summary_cmd(vty, argc, argv);
}

DEFUN (no_frp_auto_summary,
	no_frp_auto_summary_cmd,
	"no auto-summary",
	NO_STR
	"Enable automatic network number summarization\n")
{
	return	eigrp_no_auto_summary_cmd(vty, argc, argv);
}

DEFUN (frp_neighber,
	frp_neighber_cmd,
	"neighber A.B.C.D",
	"Specific neighber router\n"
	"Neighber IP Address\n")
{
	return	eigrp_neighber_cmd(vty, argc, argv);
}

DEFUN (no_frp_neighber,
	no_frp_neighber_cmd,
	"no neighber A.B.C.D",
	NO_STR
	"Specific neighber router\n"
	"Neighber IP Address\n")
{
	return	eigrp_no_neighber_cmd(vty, argc, argv);
}

//debug commands
DEFUN (frp_debug,
	frp_debug_cmd,
	"debug frp (packet|event|task|time|all|route|internal) ",
	DEBUG_STR
	FRP_STR
	"FRP packets(all packet type)\n"
	"FRP Event\n"
	"FRP Task\n"
	"FRP Time\n"
	"FRP All packet\n"
	"FRP Route\n"
	"FRP Internal\n")
{
	return	eigrp_debug_cmd(vty, argc, argv);
}

DEFUN (no_frp_debug,
       no_frp_debug_cmd,
       "no debug frp (packet|event|task|time|all|route|internal) ",
       NO_STR
       DEBUG_STR
       FRP_STR
       "FRP packets(all packet type)\n"
       "FRP Event\n"
       "FRP Task\n"
       "FRP Time\n"
       "FRP All packet\n"
       "FRP Route\n"
       "FRP Internal\n")
{
	return	eigrp_no_debug_cmd(vty, argc, argv);
}

DEFUN (frp_packet_debug,
	frp_packet_val_debug_cmd,
	"debug frp packet (send|recv) (update|qurey|reply|hello|ack)",
	DEBUG_STR
	FRP_STR
	"FRP packets(all packet type)\n"
	"Packet sent\n"
	"Packet received\n"
	"Update packet\n"
	"Qurey packet\n"
	"Reply packet\n"
	"Hello packet\n"
	"Ack packet\n")
{
	return	eigrp_debug_packet_cmd(vty, argc, argv);
}

ALIAS (frp_packet_debug,
       frp_packet_debug_cmd,
       "debug frp packet (send|recv)",
       DEBUG_STR
       FRP_STR
       "FRP packets(all packet type)\n"
       "Packet sent\n"
       "Packet received\n")
       
DEFUN (frp_detail_debug,
	frp_detail_debug_cmd,
	"debug frp detail (update|qurey|reply|hello|ack)",
	DEBUG_STR
	FRP_STR
	"Detail Information(all packet type)\n"
	"Update packet\n"
	"Qurey packet\n"
	"Reply packet\n"
	"Hello packet\n"
	"Ack packet\n")
{
	return	eigrp_debug_detail_cmd(vty, argc, argv);
}

DEFUN (no_frp_detail_debug,
	no_frp_detail_debug_cmd,
	"no debug frp detail (update|qurey|reply|hello|ack)",
	NO_STR
	DEBUG_STR
	FRP_STR
	"Detail Information(all packet type)\n"
	"Update packet\n"
	"Qurey packet\n"
	"Reply packet\n"
	"Hello packet\n"
	"Ack packet\n")
{
	return	eigrp_no_debug_detail_cmd(vty, argc, argv);
}
       
DEFUN (no_frp_packet_debug,
	no_frp_packet_val_debug_cmd,
	"no debug frp packet (send|recv) (update|qurey|reply|hello|ack)",
	NO_STR
	DEBUG_STR
	FRP_STR
	"FRP packets(all packet type)\n"
	"Packet sent\n"
	"Packet received\n"
	"Update packet\n"
	"Qurey packet\n"
	"Reply packet\n"
	"Hello packet\n"
	"Ack packet\n")
{
	return	eigrp_no_debug_packet_cmd(vty, argc, argv);
}
ALIAS (no_frp_packet_debug,
       no_frp_packet_debug_cmd,
       "no debug frp packet  (send|recv)",
       NO_STR
       DEBUG_STR
       FRP_STR
       "FRP packets(all packet type)\n"
       "Packet sent\n"
       "Packet received\n")
       
       

//interface command

DEFUN (frp_hello_interval,
	frp_hello_interval_cmd,
	"ip frp as <1-65536> hello-interval <1-65536>",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Time between HELLO packets\n"
	"Value of Seconds\n")
{
	return	eigrp_hello_interval_cmd(vty, argc, argv);
}

DEFUN (no_frp_hello_interval,
	no_frp_hello_interval_cmd,
	"no ip frp as <1-65536> hello-interval",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Time between HELLO packets\n")
{
	return	eigrp_no_hello_interval_cmd(vty, argc, argv);
}
DEFUN (frp_hold_interval,
	frp_hold_interval_cmd,
	"ip frp as <1-65536> hold-interval <1-65536>",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Interval after which a neighbor is declared dead\n"
	"Value of Seconds\n")
{
	return	eigrp_hold_interval_cmd(vty, argc, argv);
}

DEFUN (no_frp_hold_interval,
	no_frp_hold_interval_cmd,
	"no ip frp as <1-65536> hold-interval",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Interval after which a neighbor is declared dead\n")
{
	return	eigrp_no_hold_interval_cmd(vty, argc, argv);
}

DEFUN (frp_summary,
	frp_summary_cmd,
	"ip frp as <1-65536> summary A.B.C.D/M",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Route Table summary\n"
	"Frp network prefix\n")
{
	return	eigrp_summary_cmd(vty, argc, argv);
}

DEFUN (no_frp_summary,
	no_frp_summary_cmd,
	"no ip frp as <1-65536> summary A.B.C.D/M",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Route Table summary\n"
	"Frp network prefix\n")
{
	return	eigrp_no_summary_cmd(vty, argc, argv);
}
DEFUN (frp_split_horizon,
	frp_split_horizon_cmd,
	"ip frp as <1-65536> split-horizon",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Perform split-horizon\n")
{
	return	eigrp_split_horizon_cmd(vty, argc, argv);
}
DEFUN (no_frp_split_horizon,
	no_frp_split_horizon_cmd,
	"no ip frp as <1-65536> split-horizon",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Perform split horizon\n")
{
	return	eigrp_no_split_horizon_cmd(vty, argc, argv);
}
DEFUN (frp_key,
	frp_key_cmd,
	"ip frp as <1-65536> key-string WORD",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Md5 key-string\n"
	"Key string\n")
{
	return	eigrp_key_cmd(vty, argc, argv);
}

DEFUN (no_frp_key,
	no_frp_key_cmd,
	"no ip frp as <1-65536> key-string",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Md5 key-string\n")
{
	return	eigrp_no_key_cmd(vty, argc, argv);
}
DEFUN (frp_key_id,
	frp_key_id_cmd,
	"ip frp as <1-65536> key-id <1-65536>",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Md5 key-id\n"
	"key-id numbers\n")
{
	return	eigrp_key_id_cmd(vty, argc, argv);
}

DEFUN (no_frp_key_id,
	no_frp_key_id_cmd,
	"no ip frp as <1-65536> key-id",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Md5 key-id\n")
{
	return	eigrp_no_key_id_cmd(vty, argc, argv);
}

DEFUN (frp_authmode,
	frp_authmode_cmd,
	"ip frp as <1-65536> authmode",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Md5 authentication mode\n")
{
	return	eigrp_authmode_cmd(vty, argc, argv);
}
DEFUN (no_frp_authmode,
	no_frp_authmode_cmd,
	"no ip frp as <1-65536> authmode",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Md5 authentication mode\n")
{
	return	eigrp_no_authmode_cmd(vty, argc, argv);
}

DEFUN (frp_bandwidth_percent,
	frp_bandwidth_percent_cmd,
	"ip frp as <1-65536> bandwidth-percent <1-100>",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Set bandwidth percent informational parameter\n"
	"Bandwidth value in percent(80%)\n")
{
	return	eigrp_bandwidth_percent_cmd(vty, argc, argv);
}
DEFUN (no_frp_bandwidth_percent,
	no_frp_bandwidth_percent_cmd,
	"no ip frp as <1-65536> bandwidth-percent",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Set bandwidth percent informational parameter\n")
{
	return	eigrp_no_bandwidth_percent_cmd(vty, argc, argv);
}

DEFUN (frp_bandwidth,
	frp_bandwidth_cmd,
	"ip frp as <1-65536> bandwidth <1-10000000>",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Set bandwidth informational parameter\n"
	"Bandwidth value in kbps\n")
{
	return	eigrp_bandwidth_cmd(vty, argc, argv);
}
DEFUN (no_frp_bandwidth,
	no_frp_bandwidth_cmd,
	"no ip frp as <1-65536> bandwidth",
	NO_STR
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Set bandwidth informational parameter\n")
{
	return	eigrp_no_bandwidth_cmd(vty, argc, argv);
}

DEFUN (frp_delay,
	frp_delay_cmd,
	"ip frp as <1-65536> delay <1-888888>",
	IP_STR
	FRP_STR	
	AS_CONFIG_STR
	AS_STR
	"Set delay informational parameter\n"
	"delay value in millisecond\n")
{
	return	eigrp_delay_cmd(vty, argc, argv);
}

DEFUN (no_frp_delay,
		no_frp_delay_cmd,
		"no ip frp as <1-65536> delay",
		NO_STR
		IP_STR
		FRP_STR	
		AS_CONFIG_STR
		AS_STR
		"Set delay informational parameter\n")
{
	return	eigrp_no_delay_cmd(vty, argc, argv);
}

//show command

DEFUN (show_frp_debugging,
	show_frp_debugging_cmd,
	"show debugging frp",
	SHOW_STR
	"Debugging functions (see also 'undebug')\n"
	FRP_STR)
{
	return	eigrp_show_debugging_cmd(vty, argc, argv);
}

DEFUN (show_ip_frp_interface,
	show_ip_frp_interface_cmd,
	"show ip frp interface",
	SHOW_STR
	IP_STR
	FRP_STR
    INTERFACE_STR)
{
	return	eigrp_show_ip_interface_cmd(vty, argc, argv);
}

ALIAS (show_ip_frp_interface,
		show_ip_frp_interface_if_cmd,
		"show ip frp interface IFNAME",
		SHOW_STR
		IP_STR
		FRP_STR
	    INTERFACE_STR
	    IFNAME_STR)


DEFUN (show_ip_frp_interface_detail,
	show_ip_frp_interface_detail_cmd,
	"show ip frp interface detail",
	SHOW_STR
	IP_STR
	FRP_STR
    INTERFACE_STR
    "Inteface detail infromation\n")
{
	return	eigrp_show_ip_interface_detail_cmd(vty, argc, argv);
}

DEFUN (show_ip_frp_interface_as,
	show_ip_frp_interface_as_cmd,
	"show ip frp interface as <1-65536>",
	SHOW_STR
	IP_STR
	FRP_STR
    INTERFACE_STR
	AS_CONFIG_STR
	AS_STR)
{
	return	eigrp_show_ip_interface_as_cmd(vty, argc, argv);
}

ALIAS (show_ip_frp_interface_as,
		show_ip_frp_interface_as_if_cmd,
		"show ip frp interface as <1-65536> IFNAME",
		SHOW_STR
		IP_STR
		FRP_STR
	    INTERFACE_STR
		AS_CONFIG_STR
		AS_STR
		"Interface name")

DEFUN (show_ip_frp_interface_as_detail,
	show_ip_frp_interface_as_detail_cmd,
	"show ip frp interface as <1-65536> detail",
	SHOW_STR
	IP_STR
	FRP_STR
    INTERFACE_STR
	AS_CONFIG_STR
	AS_STR
	"Inteface detail infromation\n")
{
	return	eigrp_show_ip_interface_as_detail_cmd(vty, argc, argv);
}
ALIAS (show_ip_frp_interface_as_detail,
	show_ip_frp_interface_as_if_detail_cmd,
	"show ip frp interface as <1-65536> IFNAME detail",
	SHOW_STR
	IP_STR
	FRP_STR
    INTERFACE_STR
	AS_CONFIG_STR
	AS_STR
    IFNAME_STR
	"Inteface detail infromation\n")


DEFUN (show_ip_frp_neighbor,
	show_ip_frp_neighbor_cmd,
	"show ip frp neighbor",
	SHOW_STR
	IP_STR
	FRP_STR
	"Specify neighbor router\n")
{
	return	eigrp_show_ip_neighbor_cmd(vty, argc, argv);
}

ALIAS(show_ip_frp_neighbor,
	show_ip_frp_neighbor_if_cmd,
	"show ip frp neighbor IFNAME",
	SHOW_STR
	IP_STR
	FRP_STR
	"Specify neighbor router\n"
	IFNAME_STR)

DEFUN (show_ip_frp_neighbor_detail,
	show_ip_frp_neighbor_detail_cmd,
	"show ip frp neighbor IFNAME detail",
	SHOW_STR
	IP_STR
	FRP_STR
	"Specify neighbor router\n"
	IFNAME_STR
	"detail infromation\n")	
{
	return	eigrp_show_ip_neighbor_detail_cmd(vty, argc, argv);
}
DEFUN (show_ip_frp_neighbor_as,
	show_ip_frp_neighbor_as_cmd,
	"show ip frp neighbor as <1-65536>",
	SHOW_STR
	IP_STR
	FRP_STR
	"Specify neighbor router\n"
	AS_CONFIG_STR
	AS_STR)	
{
	return	eigrp_show_ip_neighbor_as_cmd(vty, argc, argv);
}
ALIAS(show_ip_frp_neighbor_as,
	show_ip_frp_neighbor_as_if_cmd,
	"show ip frp neighbor as <1-65536> IFNAME",
	SHOW_STR
	IP_STR
	FRP_STR
	"Specify neighbor router\n"
	AS_CONFIG_STR
	AS_STR
	IFNAME_STR)
	
DEFUN (show_ip_frp_neighbor_as_detail,
	show_ip_frp_neighbor_as_detail_cmd,
	"show ip frp neighbor as <1-65536> IFNAME detail",
	SHOW_STR
	IP_STR
	FRP_STR
	"Specify neighbor router\n"
	AS_CONFIG_STR
	AS_STR
	IFNAME_STR
	"detail information\n")	
{
	return	eigrp_show_ip_neighbor_as_detail_cmd(vty, argc, argv);
}

DEFUN (show_ip_frp_topology,
	show_ip_frp_topology_value_cmd,
	"show ip frp topo (summary|active|feasible|zero)",
	SHOW_STR
	IP_STR
	FRP_STR
	"topology information\n"
	"summary topology information\n"
	"active topology information\n"
	"feasible topology information\n"
	"zero topology information\n")
{
	return	eigrp_show_ip_topology_cmd(vty, argc, argv);
}

ALIAS (show_ip_frp_topology,
	show_ip_frp_topology_cmd,
	"show ip frp topo",
	SHOW_STR
	IP_STR
	FRP_STR
	"topology information\n")

DEFUN (show_ip_frp_topo_as,
	show_ip_frp_topo_as_cmd,
	"show ip frp topo as <1-65536>",
	SHOW_STR
	IP_STR
	FRP_STR
	"topology information\n"
	AS_CONFIG_STR
	AS_STR)
{
	return	eigrp_show_ip_topology_as_cmd(vty, argc, argv);
}

DEFUN (show_ip_frp_proto,
	show_ip_frp_protocol_cmd,
	"show ip frp (protocol|traffic|route|struct)",
	SHOW_STR
	IP_STR
	FRP_STR
	"protocol information\n"
	"traffic information\n"
	"route information\n"
	"struct information\n")
{
	return	eigrp_show_ip_eigrp_cmd(vty, argc, argv);
}

DEFUN (frp_show_interface_detail,
	frp_show_interface_detail_cmd,
	"show ip frp sys interfce",
	SHOW_STR
	IP_STR
	FRP_STR
	"system config"
	INTERFACE_STR)
{
	return	eigrp_show_interface_detail(vty, argc, argv);
}

static int eigrp_config_debug_breif(struct vty *vty)
{
	EigrpDbgCtrl_pt	pDbg;
	U32	flagTmp;
	pDbg	= EigrpUtilDbgCtrlFind("shell", NULL, 0);
	if(!pDbg)
	{
		return 0;
	}
	if(pDbg->flag  == 0xffffffff)
	{ 
		vty_out(vty, "debugging frp all%s", VTY_NEWLINE); 
		return 1;
	}

	if(pDbg->flag & DEBUG_EIGRP_EVENT)
	{
		vty_out(vty, "debugging frp event%s", VTY_NEWLINE);
	}

	if((pDbg->flag & DEBUG_EIGRP_PACKET)==DEBUG_EIGRP_PACKET)
	{
		//flagTmp	= DEBUG_EIGRP_PACKET_DETAIL;
		//flagTmp	|= DEBUG_EIGRP_ROUTE;
		vty_out(vty, "debugging frp packet%s", VTY_NEWLINE);
	}
	else
	{
		if((pDbg->flag & DEBUG_EIGRP_PACKET_SEND)== DEBUG_EIGRP_PACKET_SEND)
		{
			vty_out(vty, "debugging frp packet send%s",VTY_NEWLINE);
		}
		else
		{
	  		if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_UPDATE)
			{
				vty_out(vty, "debugging frp packet send update %s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_QUERY)
			{
				vty_out(vty, "debugging frp packet send query%s",VTY_NEWLINE);
			}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_REPLY)
			{
				vty_out(vty, "debugging frp packet send reply%s", VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_HELLO)
			{
				vty_out(vty, " debugging frp packet send hello%s",VTY_NEWLINE);
			}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_SEND_ACK)
			{
				vty_out(vty, "debugging frp packet send ack%s",VTY_NEWLINE);
			}
		}
	  	if((pDbg->flag & DEBUG_EIGRP_PACKET_RECV)== DEBUG_EIGRP_PACKET_RECV)
		{
	    		vty_out(vty, "debugging frp packet recv%s", VTY_NEWLINE);
	  	}
		else
		{
	  	 	if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_UPDATE)
			{
		    		vty_out(vty, "debugging frp packet recv update %s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_QUERY)
			{
		    		vty_out(vty, "debugging frp packet recv query%s", VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_REPLY)
			{
		    		vty_out(vty, "debugging frp packet recv reply%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_HELLO)
			{
		    		vty_out(vty, "debugging frp packet recv hello%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_RECV_ACK)
			{
		    		vty_out(vty, "debugging frp packet recv ack%s",VTY_NEWLINE);
			}
	  	}
	  	if((pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL)== DEBUG_EIGRP_PACKET_DETAIL)
		{
	    		vty_out(vty, "debugging frp packet detail%s", VTY_NEWLINE);
	  	}
		else
		{
	  	 	if(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL_UPDATE)
			{
		    		vty_out(vty, "debugging frp packet detail update %s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL_QUERY)
			{
		    		vty_out(vty, "debugging frp packet detail query%s", VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL_REPLY)
			{
		    		vty_out(vty, "debugging frp packet detail reply%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL_HELLO)
			{
		    		vty_out(vty, "debugging frp packet detail hello%s",VTY_NEWLINE);
	  	 	}
			if(pDbg->flag & DEBUG_EIGRP_PACKET_DETAIL_ACK)
			{
		    		vty_out(vty, "debugging frp packet detail ack%s",VTY_NEWLINE);
			}
	  	}		
	} 

	if(pDbg->flag & DEBUG_EIGRP_ROUTE){
		vty_out(vty, "debugging frp route%s",VTY_NEWLINE);
	}

	if(pDbg->flag & DEBUG_EIGRP_TIMER){
		vty_out(vty, "debugging frp timer%s", VTY_NEWLINE);
	}

	if(pDbg->flag & DEBUG_EIGRP_TASK){
		vty_out(vty, "debugging frp task%s",VTY_NEWLINE);
	}
	
	return	1;
}

static int eigrp_config_interface_breif(struct vty *vty, struct interface *ifp)
{
	EigrpConfIntf_pt			pConfIntf;
	EigrpConfIntfAuthkey_pt	pAuthKey;
	EigrpConfIntfAuthid_pt		pAuthId;
	EigrpConfIntfAuthMode_pt	pAuthmode;
	EigrpConfIntfPassive_pt		pPassive;
	EigrpConfIntfInvisible_pt	pInvisible;
	EigrpConfIntfBw_pt			pBandwidth;
	EigrpConfIntfBandwidth_pt		bw;
	EigrpConfIntfDelay_pt		delay;
	EigrpConfIntfHello_pt		pHello;
	EigrpConfIntfHold_pt		pHold;
	EigrpConfIntfSplit_pt		pSplit;
	EigrpConfIntfSum_pt		pSummary;

	char *circName = NULL;

	if( (!gpEigrp)||(!gpEigrp->conf))
		return 0;
	
	if(!gpEigrp->conf->pConfIntf){
		return 0;
	}
	circName = strdup(ifp->name);
	if(circName == NULL)
		return 0;
	
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){
		if(EigrpPortStrCmp(pConfIntf->ifName, circName)){
			continue;
		}
		
		for(pAuthKey = pConfIntf->authkey; pAuthKey; pAuthKey = pAuthKey->next){
			vty_out (vty, "  ip frp as %d key %s%s", pAuthKey->asNum, pAuthKey->auth,VTY_NEWLINE);
		}

		for(pAuthId = pConfIntf->authid; pAuthId; pAuthId = pAuthId->next){
			vty_out (vty, "  ip frp as %d key-id %d%s", pAuthId->asNum, pAuthId->authId,VTY_NEWLINE);
		}

		for(pAuthmode = pConfIntf->authmode; pAuthmode; pAuthmode = pAuthmode->next){
			vty_out (vty, "  ip frp as %d authMode%s", pAuthmode->asNum,VTY_NEWLINE);
		}

		for(pPassive = pConfIntf->passive; pPassive; pPassive = pPassive->next){
			if(EIGRP_DEF_PASSIVE == FALSE){
				vty_out (vty, "  ip frp as %d passive-interface%s", pPassive->asNum,VTY_NEWLINE);
			}else{
				vty_out (vty, "  no ip frp as %d passive-interface%s", pPassive->asNum,VTY_NEWLINE);
			}
		}

		for(pInvisible = pConfIntf->invisible; pInvisible; pInvisible = pInvisible->next){
			if(EIGRP_DEF_INVISIBLE == FALSE){
				vty_out (vty, "  ip frp as %d invisible%s", pInvisible->asNum,VTY_NEWLINE);
			}else{
				vty_out (vty, "  no ip frp as %d invisible%s", pInvisible->asNum,VTY_NEWLINE);
			}
		}

		for(pBandwidth = pConfIntf->bandwidth; pBandwidth; pBandwidth = pBandwidth->next){
			vty_out (vty, "  ip frp as %d bandwidth-percent %d%s", 
					pBandwidth->asNum, 
					pBandwidth->bandwidth, 
					VTY_NEWLINE);
		}

		for(bw = pConfIntf->bw; bw; bw = bw->next){
			vty_out (vty, "  ip frp as %d bandwidth %d%s", 
					bw->asNum, 
					bw->bandwidth, 
					VTY_NEWLINE);
		}

		for(delay = pConfIntf->delay; delay; delay = delay->next){
			vty_out (vty, "  ip frp as %d delay %d%s", 
					delay->asNum, 
					delay->delay, 
					VTY_NEWLINE);
		}
		/*hold-time MUST before hello-interval*/
		for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
			vty_out (vty, "  ip frp as %d hold-time %d%s", 
					pHold->asNum, 
					pHold->hold, 
					VTY_NEWLINE);
		}

		/*hello-interval MUST after hold-time*/
		for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
			vty_out (vty, "  ip frp as %d hello-interval %d%s", 
					pHello->asNum, 
					pHello->hello, 
					VTY_NEWLINE);
		}

		for(pSplit = pConfIntf->split; pSplit; pSplit = pSplit->next){
			if(pSplit->split == TRUE){
				vty_out (vty, "  ip frp as %d split-horizon%s", pSplit->asNum,VTY_NEWLINE);
			}else{
				vty_out (vty, "  no ip frp as %d split-horizon%s", pSplit->asNum,VTY_NEWLINE);
			}

		}

		for(pSummary = pConfIntf->summary; pSummary; pSummary = pSummary->next){
			char prefix_str[64];
			memset(prefix_str, 0 , sizeof(prefix_str));
			netmask_str2prefix_str (EigrpUtilIp2Str(pSummary->ipNet), EigrpUtilIp2Str(pSummary->ipMask),prefix_str);
			vty_out (vty, "  ip frp as %d summary %s %s", 
					pSummary->asNum,
					prefix_str,
					VTY_NEWLINE);
		}
	}
	if(circName)
		free(circName);
	return 1;
}
static int config_write_interface (struct vty *vty)
{
  struct listnode *n1, *n2;
  struct interface *ifp;
  int write = 0;
  struct route_node *rn = NULL;

  for (ALL_LIST_ELEMENTS_RO (iflist, n1, ifp))
    {
      if (memcmp (ifp->name, "VLINK", 5) == 0)
    	  continue;

      vty_out (vty, "!%s", VTY_NEWLINE);
      vty_out (vty, "interface %s%s", ifp->name, VTY_NEWLINE);
      if (ifp->desc)
        vty_out (vty, " description %s%s", ifp->desc, VTY_NEWLINE);
      eigrp_config_interface_breif(vty, ifp);
      write++;

    }
  return write;
}
static int eigrp_config_write(struct vty *vty)
{
	EigrpConfAs_pt	pConfAs;
	EigrpConfAsNet_pt	pNet;
	EigrpConfAsNei_pt	pNei;
	
	EigrpPdb_st		*pdb;	
	EigrpDualDdb_st *ddb;
	
	char prefix_str[64];
#ifdef _EIGRP_VLAN_SUPPORT	
	U32 i;
#endif//_EIGRP_VLAN_SUPPORT	
	struct EigrpConfAsRedis_ *pRedis;
	
	if( (!gpEigrp)||(!gpEigrp->conf)||(!gpEigrp->conf->pConfAs) )
		return 0;
	
	for(pConfAs = gpEigrp->conf->pConfAs; pConfAs; pConfAs = pConfAs->next){
		vty_out (vty, "router frp %d%s", pConfAs->asNum,VTY_NEWLINE);
		
		pdb	= EigrpIpFindPdb(pConfAs->asNum);
		if(pdb)
		{
			ddb = pdb->ddb;
			if( (ddb)&&(ddb->routerId) )
			{
				vty_out (vty, "  router-id %s %s", EigrpUtilIp2Str(ddb->routerId),VTY_NEWLINE);
			}
		}
			
		for(pNet = pConfAs->network; pNet; pNet = pNet->next){
			
			memset(prefix_str, 0 , sizeof(prefix_str));
			netmask_str2prefix_str (EigrpUtilIp2Str(pNet->ipNet), EigrpUtilIp2Str(pNet->ipMask),prefix_str);
			vty_out (vty, "  network %s %s", 
					prefix_str,
					VTY_NEWLINE);
			
			//vty_out (vty, "  network %s %s%s", EigrpUtilIp2Str(pNet->ipNet),
			//		EigrpUtilIp2Str(pNet->ipMask),VTY_NEWLINE);
		}

		if(pConfAs->autoSum /*&& *pConfAs->autoSum != EIGRP_DEF_AUTOSUM*/){
			if(*pConfAs->autoSum == TRUE){
				vty_out (vty, "  auto-summary%s", VTY_NEWLINE);
			}else{
				vty_out (vty, "  no auto-summary%s", VTY_NEWLINE);
			}
		}

		if(pConfAs->redis){
			for(pRedis = pConfAs->redis; pRedis; pRedis = pRedis->next){
				vty_out (vty, "  redistribute %s%s", pRedis->protoName,VTY_NEWLINE);
	/** Ŀǰ�����в�֧�֣���ʱ����
				if(pRedis->srcProc){
					cmdByte	+= sprintf(bufTem + cmdByte, " %d", pRedis->srcProc);
				}
				
				if(pRedis->vmetric){
					cmdByte	+= sprintf(bufTem + cmdByte, " metric %d %d %d %d %d",
													pRedis->vmetric->bandwidth,
													pRedis->vmetric->delay,
													pRedis->vmetric->reliability,
													pRedis->vmetric->load,
													pRedis->vmetric->mtu);
				}
				if(pRedis->rtMapName[0]){
					cmdByte	+= sprintf(bufTem + cmdByte, " route-map %s", redis->rtMapName);
				}
	*/
			}
		}
		
		if(pConfAs->defMetric){
			vty_out (vty, "  default-metric %d %d %d %d %d%s", 
					pConfAs->defMetric->bandwidth,
					pConfAs->defMetric->delay,
					pConfAs->defMetric->reliability,
					pConfAs->defMetric->load,
					pConfAs->defMetric->mtu,
					VTY_NEWLINE);
		}

		if(pConfAs->distance){
			vty_out (vty, "  distance %d %d%s", 
					pConfAs->distance->interDis, 
					pConfAs->distance->exterDis,
					VTY_NEWLINE);
		}

		if(pConfAs->metricWeight){
			vty_out (vty, "  metric weights tos %d %d %d %d %d%s", 
					pConfAs->metricWeight->k1, 
					pConfAs->metricWeight->k2, 
					pConfAs->metricWeight->k3, 
					pConfAs->metricWeight->k4, 
					pConfAs->metricWeight->k5,
					VTY_NEWLINE);
		}

		for(pNei = pConfAs->nei; pNei; pNei = pNei->next){
			
			vty_out (vty, "  neighbor %s%s", 
					EigrpUtilIp2Str(pNei->ipAddr),
					VTY_NEWLINE);
		}

		if(pConfAs->defRouteIn/* && *pConfAs->defRouteIn != EIGRP_DEF_DEFROUTE_IN*/){
			if(*pConfAs->defRouteIn == TRUE){
				vty_out (vty, "  default-information in%s", VTY_NEWLINE);
			}else{
				vty_out (vty, "  no default-information in%s", VTY_NEWLINE);
			}
		}

		if(pConfAs->defRouteOut /*&& *pConfAs->defRouteOut != EIGRP_DEF_DEFROUTE_OUT*/){
			if(*pConfAs->defRouteOut == TRUE){
				vty_out (vty, "  default-information out%s", VTY_NEWLINE);
			}else{
				vty_out (vty, "  no default-information out%s", VTY_NEWLINE);
			}
		}

	}
#ifdef _EIGRP_VLAN_SUPPORT
	for(i = 0; i < 15; i++){
		if(gpEigrp->vlanFlag[i]){
			cmdByte	= sprintf(bufTem, "frp u-s-map vlan-id %d\n", i);
			EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
			EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
			*usedLen = *usedLen + cmdByte;
		}
	}
#endif//_EIGRP_VLAN_SUPPORT
#ifdef _DC_	
	cmdByte = DcCmdApiBuildRunConfMode_Router(bufTem);
	if(cmdByte != 0){
		EigrpCmdEnsureSize(ppBuf, bufLen, *usedLen, cmdByte);
		EigrpPortMemCpy((U8 *)(*ppBuf + *usedLen), bufTem, cmdByte);
		*usedLen = *usedLen + cmdByte;
	}
#endif
	return 1; 
}
static struct cmd_node interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
  1,
};
static struct cmd_node frp_node =
{
  FRP_NODE,
  "%s(config-router)# ",
  1
};
/* Debug node. */
static struct cmd_node debug_node =
{
  DEBUG_NODE,
  "",
  1 /* VTYSH */
};
static int  eigrp_vty_init (void)
{
	//��quaggaע����ʾ�ӿ���Ϣ�ĺ�����show run�����ʱ����ʾ�ӿ���Ϣ��
	//zebra_node_func_install(0, INTERFACE_NODE, eigrp_config_interface_breif);
	//��quaggaע����ʾdebug��Ϣ�ĺ�����show run�����ʱ����ʾdebug��Ϣ��
	//zebra_node_func_install(0, DEBUG_NODE, eigrp_config_debug_breif);
#if (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	//if_init();
	//if_add_hook (IF_NEW_HOOK, lldp_interface_new_hook);
	//if_add_hook (IF_DELETE_HOOK, lldp_interface_delete_hook);

	install_node (&interface_node, config_write_interface);
	install_default (INTERFACE_NODE);
	install_node (&debug_node, eigrp_config_debug_breif);
	install_element (CONFIG_NODE, &interface_cmd);
	install_element (CONFIG_NODE, &no_interface_cmd);
	install_element (INTERFACE_NODE, &interface_desc_cmd);
	install_element (INTERFACE_NODE, &no_interface_desc_cmd);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
#endif
	install_element (INTERFACE_NODE, &frp_summary_cmd); 
	install_element (INTERFACE_NODE, &no_frp_summary_cmd); 
	install_element (INTERFACE_NODE, &frp_split_horizon_cmd); 
	install_element (INTERFACE_NODE, &no_frp_split_horizon_cmd); 
	
	install_element (INTERFACE_NODE, &frp_key_cmd); 
	install_element (INTERFACE_NODE, &no_frp_key_cmd); 
	install_element (INTERFACE_NODE, &frp_key_id_cmd); 
	install_element (INTERFACE_NODE, &no_frp_key_id_cmd); 
	
	install_element (INTERFACE_NODE, &frp_authmode_cmd); 
	install_element (INTERFACE_NODE, &no_frp_authmode_cmd); 
	
	install_element (INTERFACE_NODE, &frp_bandwidth_percent_cmd); 
	install_element (INTERFACE_NODE, &no_frp_bandwidth_percent_cmd); 
	install_element (INTERFACE_NODE, &frp_bandwidth_cmd); 
	install_element (INTERFACE_NODE, &no_frp_bandwidth_cmd); 

	install_element (INTERFACE_NODE, &frp_delay_cmd); 
	install_element (INTERFACE_NODE, &no_frp_delay_cmd); 

	install_element (INTERFACE_NODE, &frp_hello_interval_cmd); 
	install_element (INTERFACE_NODE, &no_frp_hello_interval_cmd); 
	install_element (INTERFACE_NODE, &frp_hold_interval_cmd); 
	install_element (INTERFACE_NODE, &no_frp_hold_interval_cmd); 

	install_element (INTERFACE_NODE, &frp_passive_cmd); 
	install_element (INTERFACE_NODE, &no_frp_passive_cmd); 

	install_element (INTERFACE_NODE, &frp_invisible_cmd);  
	install_element (INTERFACE_NODE, &no_frp_invisible_cmd); 

	install_node (&frp_node, eigrp_config_write);
	/* "router FRP" commands. */
	install_element (CONFIG_NODE, &router_frp_cmd);
	install_element (CONFIG_NODE, &no_router_frp_cmd);
	install_default (FRP_NODE);

	/* "FRP" commands. */
	install_element (FRP_NODE, &frp_router_id_cmd);
	install_element (FRP_NODE, &no_frp_router_id_cmd);
	install_element (FRP_NODE, &frp_network_mask_cmd);
	install_element (FRP_NODE, &no_frp_network_mask_cmd);
	install_element (FRP_NODE, &frp_redistribute_type_cmd);
	install_element (FRP_NODE, &no_frp_redistribute_type_cmd);
#ifdef EIGRP_REDISTRIBUTE_METRIC	
	install_element (FRP_NODE, &frp_redistribute_metric_cmd);
	install_element (FRP_NODE, &no_frp_redistribute_metric_cmd);
#endif//EIGRP_REDISTRIBUTE_METRIC	
	install_element (FRP_NODE, &frp_distance_cmd);
	install_element (FRP_NODE, &no_frp_distance_cmd);
	install_element (FRP_NODE, &frp_default_route_cmd);
	install_element (FRP_NODE, &no_frp_default_route_cmd);
	
	install_element (FRP_NODE, &frp_network_metric_cmd);
	install_element (FRP_NODE, &no_frp_network_metric_cmd);
	install_element (FRP_NODE, &frp_metric_weights_cmd);
	install_element (FRP_NODE, &no_frp_metric_weights_cmd);
  
	install_element (FRP_NODE, &frp_default_metric_cmd);
	install_element (FRP_NODE, &no_frp_default_metric_cmd);
    
	install_element (FRP_NODE, &frp_auto_summary_cmd);
	install_element (FRP_NODE, &no_frp_auto_summary_cmd);

	install_element (FRP_NODE, &frp_neighber_cmd);
	install_element (FRP_NODE, &no_frp_neighber_cmd);
	
	install_element (CONFIG_NODE, &frp_debug_cmd);
	install_element (CONFIG_NODE, &no_frp_debug_cmd);
	
	install_element (CONFIG_NODE, &frp_packet_val_debug_cmd);
	install_element (CONFIG_NODE, &frp_packet_debug_cmd);
	
	install_element (CONFIG_NODE, &no_frp_packet_val_debug_cmd);
	install_element (CONFIG_NODE, &no_frp_packet_debug_cmd);
	
	install_element (CONFIG_NODE, &frp_detail_debug_cmd);
	install_element (CONFIG_NODE, &no_frp_detail_debug_cmd);

	install_element (ENABLE_NODE, &show_frp_debugging_cmd);
	
	install_element (ENABLE_NODE, &show_ip_frp_interface_cmd);	
	install_element (ENABLE_NODE, &show_ip_frp_interface_if_cmd);			
	install_element (ENABLE_NODE, &show_ip_frp_interface_detail_cmd);	
	
	install_element (ENABLE_NODE, &show_ip_frp_interface_as_cmd);	
	install_element (ENABLE_NODE, &show_ip_frp_interface_as_if_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_interface_as_detail_cmd);	
	install_element (ENABLE_NODE, &show_ip_frp_interface_as_if_detail_cmd);	

	install_element (ENABLE_NODE, &show_ip_frp_neighbor_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_neighbor_if_cmd);	
	install_element (ENABLE_NODE, &show_ip_frp_neighbor_detail_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_neighbor_as_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_neighbor_as_if_cmd);	
	install_element (ENABLE_NODE, &show_ip_frp_neighbor_as_detail_cmd);	
	
	install_element (ENABLE_NODE, &show_ip_frp_topology_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_topology_value_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_topo_as_cmd);
	install_element (ENABLE_NODE, &show_ip_frp_protocol_cmd);	
	
	install_element (ENABLE_NODE, &frp_show_interface_detail_cmd);
	return 0;
}
/************************************************************************************/
/************************************************************************************/
int eigrp_shell_init()
{
	return eigrp_vty_init();
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/*
zclient?????????(?????????????zclient???
??????????????????????????????????)
*/
int ZebraClientEigrpUtilSched()
{
	return SUCCESS;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
static int zebraEigrpMain_THread()//int ifnum, char *config_file,char *logname, char *zserv_path, int vty_port
{
  struct thread eigrp_thread;
  if( (EigrpMaster)&&(EigrpMaster->master) )
  {
	  taskDelay(sysClkRateGet()*5);
	  zclient_init (EigrpMaster->master, EigrpMaster->zclient, ZEBRA_ROUTE_FRP);
	  while(1)
	  {
			//if(thread_fetch_one (EigrpMaster->master, &zebra_thread, 1000))//
			if(thread_fetch (EigrpMaster->master, &eigrp_thread))
			{
#ifdef _EIGRP_PLAT_MODULE
				//if(gpEigrp->EigrpSem)
				//	EigrpPortSemBTake(gpEigrp->EigrpSem);
#endif//_EIGRP_PLAT_MODULE
				thread_call (&eigrp_thread);
#ifdef _EIGRP_PLAT_MODULE
				//if(gpEigrp->EigrpSem)
				//	EigrpPortSemBGive(gpEigrp->EigrpSem);
#endif//_EIGRP_PLAT_MODULE
			}
	  }
  }
  return 0;
}
#elif (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
static int zebraEigrpMain_THread()
{
  struct thread eigrp_thread;
  sleep(1);
  if( (EigrpMaster)&&(EigrpMaster->master) )
  {
		sleep(1);
		zclient_init (EigrpMaster->zclient, ZEBRA_ROUTE_FRP);
		EigrpMaster->eigrpvty.type = VTY_TERM;
		//EigrpMaster->eigrpvty.obuf = EigrpMaster->zclient->vtyobuf;
		//thread_add_timer_msec (EigrpMaster->master, zebraEigrpMain_THread, EigrpMaster, 10);
		//thread_add_timer(EigrpMaster->master, zebraEigrpMain_THread, EigrpMaster, 1);
		while(thread_fetch (EigrpMaster->master, &eigrp_thread))
		{
			thread_call (&eigrp_thread);
		}
  }
}
#endif// (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
#endif// EIGRP_PLAT_ZEBRA	


int	zebraEigrpZclientInit(int daemon, int num, void *m, void *list)
{
	EigrpMaster = malloc(sizeof(struct ZebraEigrpMaster));
	if(EigrpMaster)
	{
		if(ZebraEigrpCmdInit()==FAILURE)
		{
			free(EigrpMaster);
			return FAILURE;
		}
		EigrpMaster->ZebraSched = zebraEigrpUtilSchedInit();
		if(EigrpMaster->ZebraSched == NULL)
		{
			ZebraEigrpCmdCancel();
			free(EigrpMaster);
			return FAILURE;
		}
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
		//ϵͳ�ӿڱ��ʼ��
	ZebraEigrpIntfInit(num);
#endif
#ifdef EIGRP_PLAT_ZEBRA	
		EigrpMaster->master = m;
		if(EigrpMaster->master == NULL)
			EigrpMaster->master = thread_master_create ();
		if(EigrpMaster->master == NULL)
		{
			ZebraEigrpCmdCancel();
			zebraEigrpUtilSchedClean(EigrpMaster->ZebraSched);
			free(EigrpMaster);
			return FAILURE;
		}
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
		if(list)
			EigrpMaster->iflist = list;
		else
			EigrpMaster->iflist = iflist;
#elif (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
		if_init ();
		//if_add_hook (IF_NEW_HOSUCCESS, ospf_if_new_hook);
		//if_add_hook (IF_DELETE_HOSUCCESS, ospf_if_delete_hook);
		EigrpMaster->iflist = iflist;
		//master = EigrpMaster->master;
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)	
		EigrpMaster->zclient = zclient_new ();
		if(EigrpMaster->zclient == NULL)
		{
			ZebraEigrpCmdCancel();
			zebraEigrpUtilSchedClean(EigrpMaster->ZebraSched);
			EigrpMaster->iflist = NULL;
			free(EigrpMaster);
			return FAILURE;
		}
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)		
		EigrpMaster->zclient->daemon = ZEBRA_ROUTE_FRP;
#endif// (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)		
		EigrpMaster->zclient->router_id_update = eigrp_router_id_update_zebra;
		EigrpMaster->zclient->interface_add = eigrp_interface_add;
		EigrpMaster->zclient->interface_delete = eigrp_interface_delete;
		EigrpMaster->zclient->interface_up = eigrp_interface_state_up;
		EigrpMaster->zclient->interface_down = eigrp_interface_state_down;
		EigrpMaster->zclient->interface_address_add = eigrp_interface_address_add;
		EigrpMaster->zclient->interface_address_delete = eigrp_interface_address_delete;
		EigrpMaster->zclient->ipv4_route_add = eigrp_zebra_read_ipv4;
		EigrpMaster->zclient->ipv4_route_delete = eigrp_zebra_read_ipv4;
#if (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
		eigrp_shell_init();
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)	
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
		//?????????
		eigrp_shell_init();	

		EigrpMaster->zEigrpTaskId = taskSpawn("tEiClient", daemon, 0, 50000,  
				(FUNCPTR)zebraEigrpMain_THread, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)

#endif// EIGRP_PLAT_ZEBRA	
		debug_eigrp_zebra = 3;
		zebraEigrpRoutehandle(zebraEigrpClientRouteHandle);			
		return SUCCESS;
	}
	return FAILURE;
}
/*************************************************************************/
/*************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
#ifdef EIGRP_PLAT_ZEBRA
/****************************************************************************/
int	zebraEigrpMasterPthreadInit(void)//?????????????while ??????��?
{
	pthread_create (&EigrpMaster->pThread, NULL, zebraEigrpMain_THread, NULL);
	return 0;
}
#endif// EIGRP_PLAT_ZEBRA
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
/*************************************************************************/
/*************************************************************************/
int	zebraEigrpMasterInit(int daemon,void *master)
{
	EigrpInit(daemon);
	return SUCCESS;
}
/*************************************************************************/
/*************************************************************************/
static int EigrpIntfShowOne(EigrpIntf_pt pIntf)
{
	if(pIntf)
	{
		printf( "\nInterface %s %s%s",pIntf->name,BIT_TEST(pIntf->flags, EIGRP_INTF_FLAG_ACTIVE)? "up":"down","\n");
		printf( " index %d metric %d mtu %d bandwidth %d kbps delay %d msec%s",
				pIntf->ifindex,
				pIntf->metric,
				pIntf->mtu,
				pIntf->bandwidth,
				pIntf->delay,"\n");
		
		printf( " flag %s %s",zebra_interface_flag_dump (pIntf->flags),"\n");
		if(pIntf->ipAddr != 0)
		{
			printf( " inet %s netmask %s ",
					EigrpUtilIp2Str(pIntf->ipAddr),
					EigrpUtilIp2Str(pIntf->ipMask));
			if(BIT_TEST(pIntf->flags, EIGRP_INTF_FLAG_POINT2POINT))
				printf( "destnation %s \n",EigrpUtilIp2Str(pIntf->remoteIpAddr));
			else
				printf( "broadcast %s \n",EigrpUtilIp2Str(pIntf->ipAddr | (~pIntf->ipMask)));
		}
	}
	return SUCCESS;
}
int	EigrpIntfShow()
{
	EigrpIntf_pt pTem = NULL;
	
	if((!gpEigrp) ||(! gpEigrp->intfLst) )
		return FAILURE;
#if(EIGRP_OS_TYPE==EIGRP_OS_VXWORKS)	
	if(gpEigrp->EigrpSem)
		EigrpPortSemBTake(gpEigrp->EigrpSem);	
#endif	
	printf("interface information\n");
	printf("=================================================================\n");
	for(pTem = gpEigrp->intfLst; pTem; pTem = pTem->next){
		EigrpIntfShowOne(pTem);
	}
	printf("=================================================================\n");
#if(EIGRP_OS_TYPE==EIGRP_OS_VXWORKS)	
	if(gpEigrp->EigrpSem)
		EigrpPortSemBGive(gpEigrp->EigrpSem);	
#endif
	return SUCCESS;
}
/*************************************************************************/
/*************************************************************************/
#endif //_EIGRP_PLAT_MODULE
