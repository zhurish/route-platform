/************************************************************************************

	(C) COPYRIGHT 2005-2007 by Beijing Jointbest System Technology Co. Ctd.
						All rights reserved.

	This software is confidential and proprietary to Beijing Jointbest System Technology Co. Ctd.
	No part of this software may be reproduced, stored, transmitted, disclosed or used in any 
	form or by any means other than as expressly provided by the written license agreement
	between Beijing Jointbest System Technology Co. Ctd. and its licensee.

	JOINTBEST MAKES NO OTHER WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT
	LIMITATION WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
	WITH REGARD TO THIS SOFTWARE OR ANY RELATED MATERIALS.

	IN NO EVENT SHALL JOINTBEST BE LIABLE FOR ANY INDIRECT, SPECIAL, OR CONSEQUENTIAL
	DAMAGES IN CONNECTION WITH OR ARISING OUT OF THE USE OF, OR INABILITY TO USE,
	THIS SOFTWARE, WHETHER BASED ON BREACH OF CONTRACT, TORT (INCLUDING 
	NEGLIGENCE), PRODUCT LIABILITY, OR OTHERWISE, AND WHETHER OR NOT IT HAS BEEN
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

					IMPORTANT LIMITATION(S) ON USE

	The use of this software is limited to the Use set 	forth in the written License Agreement 
	between Jointbest and its Licensee. Among other things, the Use of this software may be limited
	to a particular type of Designated Equipment. Before any installation, use or transfer of this 
	software, please consult the written License Agreement or contact Jointbest at the location set 
	forth below in order to confirm that you are engaging in a permissible Use of the software.

	Beijing Jointbest System Technology Co. Ctd.
	1th, 10th Floor, A Building, Changyin Plaza,
	Yongding Road, Haidian District, Beijing, China

	Tel:	+86 10 58896099
	Fax:	+86 10 58895234
	Web: www.jointbest.com

************************************************************************************/

/************************************************************************************

	Module:	EigrpPacket

	Name:	EigrpPacket.c

	Desc:	

	Rev:	
	
***********************************************************************************/

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

#ifdef _DC_
#include	"../Dc/include/dcmain.h"
#include	"../Dc/include/dcsysport.h"
#endif//_DC_

extern	Eigrp_pt	gpEigrp;
/************************************************************************************

	Name:	EigrpSocketInit

	Desc:	This function is to initialize an eigrp socket.
		
	Para: 	max_packetsize	- max received packet length
	
	Ret:		new socket or -1 if failed to create socket
************************************************************************************/

S32	EigrpSocketInit(U32 *max_packetsize)
{
	S32 sock;
	
	EIGRP_FUNC_ENTER(EigrpSocketInit);
	sock = EigrpPortSockCreate();
	if(sock<0) {    
		EIGRP_FUNC_LEAVE(EigrpSocketInit);
		return sock;
	}

	if(!*max_packetsize){
		*max_packetsize = EigrpPortSetMaxPackSize(sock);
	}
	EIGRP_FUNC_LEAVE(EigrpSocketInit);
	
	return sock;
}

/************************************************************************************

	Name:	EigrpDebugPacketType

	Desc:	This function is to tell if the given packet should be displayed.
		
	Para: 	opcode	- the opcode of packet
			direct	- the signal for judging the packet is received packet or sent packet
	
	Ret:		TRUE
************************************************************************************/

U32	EigrpDebugPacketType(S32 opcode, S32 direct)
{
	U32	flag;
	
	EIGRP_FUNC_ENTER(EigrpDebugPacketType);
	if(direct) /*recv*/{
		switch(opcode){
			case EIGRP_OPC_UPDATE:
				flag	= DEBUG_EIGRP_PACKET_RECV_UPDATE;
				break;
			case EIGRP_OPC_QUERY:
				flag	= DEBUG_EIGRP_PACKET_RECV_QUERY;
				break;
			case EIGRP_OPC_REPLY:
				flag	= DEBUG_EIGRP_PACKET_RECV_REPLY;				
				break;
			case EIGRP_OPC_HELLO:
				flag	= DEBUG_EIGRP_PACKET_RECV_HELLO;
				break;
			case EIGRP_OPC_ACK:
				flag	= DEBUG_EIGRP_PACKET_RECV_ACK;
				break;		
			default:
				flag	= 0;
				break;
		}
	}
	else{ /* send */
		switch(opcode){
			case EIGRP_OPC_UPDATE:
				flag	= DEBUG_EIGRP_PACKET_SEND_UPDATE;
				break;
			case EIGRP_OPC_QUERY:
				flag	= DEBUG_EIGRP_PACKET_SEND_QUERY;
				break;
			case EIGRP_OPC_REPLY:
				flag	= DEBUG_EIGRP_PACKET_SEND_REPLY;
				break;
			case EIGRP_OPC_HELLO:
				flag	= DEBUG_EIGRP_PACKET_SEND_HELLO;
				break;
			case EIGRP_OPC_ACK:
				flag	= DEBUG_EIGRP_PACKET_SEND_ACK;
				break;
			default:
				flag	= 0;
				break;
		}
	}
	EIGRP_FUNC_LEAVE(EigrpDebugPacketType);
	return flag;

}
/************************************************************************************

	Name:	EigrpDebugPacketDetailType

	Desc:	This function is to tell if the given packet detail should be displayed.
		
	Para: 	opcode	- the opcode of packet
	
	Ret:		TRUE
************************************************************************************/

U32	EigrpDebugPacketDetailType(S32 opcode)
{
	U32	flag;
	
	EIGRP_FUNC_ENTER(EigrpDebugPacketDetailType);
	switch(opcode){
		case EIGRP_OPC_UPDATE:
			flag	= DEBUG_EIGRP_PACKET_DETAIL_UPDATE;
			break;
		case EIGRP_OPC_QUERY:
			flag	= DEBUG_EIGRP_PACKET_DETAIL_QUERY;
			break;
		case EIGRP_OPC_REPLY:
			flag	= DEBUG_EIGRP_PACKET_DETAIL_REPLY;				
			break;
		case EIGRP_OPC_HELLO:
			flag	= DEBUG_EIGRP_PACKET_DETAIL_HELLO;
			break;
		case EIGRP_OPC_ACK:
			flag	= DEBUG_EIGRP_PACKET_DETAIL_ACK;
			break;		
		default:
			flag	= 0;
			break;
	}
	EIGRP_FUNC_LEAVE(EigrpDebugPacketDetailType);
	return flag;

}

/************************************************************************************

	Name:	EigrpRecv

	Desc:	This function is to receive eigrp packet through eigrp socket.
		
	Para: 	NONE
	
	Ret:		0	for success to  receive
			-1	for fail to receive
************************************************************************************/
#ifdef _DC_
S32	EigrpPktCopyFromDc(U32 size, U8 *msgbuf, void *pCirc)  /*add by zm 130108*/
{
	EigrpFirstPkt_pt firstPkt;

	EIGRP_FUNC_ENTER(EigrpPktCopyFromDc);
	
	FPKT_LOCK_TAKE(gpEigrp->firstPktLock);
		
	EIGRP_FUNC_ENTER(EigrpRecvforFirst);
	
	if(gpEigrp->firstPktCnt > 200){			
		FPKT_LOCK_GIVE(gpEigrp->firstPktLock);
		EIGRP_FUNC_LEAVE(EigrpPktCopyFromDc);
		return -1;
	}
	firstPkt = (EigrpFirstPkt_pt)(gpEigrp->firstPktbuf + gpEigrp->firstPktCnt * sizeof(struct EigrpFirstPkt_));

	firstPkt->pktLen = size;
	EigrpPortMemCpy(firstPkt->pktBuf, msgbuf, firstPkt->pktLen);
	firstPkt->pCirc = pCirc;

	gpEigrp->firstPktCnt++;
	
#if 0
	printf("EigrpRecvforFirst:1\n");
	EigrpProcPkt(size, msgbuf, pCirc);
	EIGRP_FUNC_LEAVE(EigrpRecvforFirst);
#endif
	FPKT_LOCK_GIVE(gpEigrp->firstPktLock);

	EIGRP_FUNC_LEAVE(EigrpPktCopyFromDc);
	return 0;
}

void	EigrpProcDcPkt()  /*zhangming 130108*/
{
	EigrpFirstPkt_pt firstPkt;
	U32 cnt = 0;
	S32 retVal;
	EIGRP_FUNC_ENTER(EigrpProcDcPkt);
	FPKT_LOCK_TAKE(gpEigrp->firstPktLock);
	if(gpEigrp->firstPktCnt <= 0){			
		FPKT_LOCK_GIVE(gpEigrp->firstPktLock);
		EIGRP_FUNC_LEAVE(EigrpProcDcPkt);
		return;
	}else{
		for(cnt = 0; cnt < gpEigrp->firstPktCnt; cnt++){
			firstPkt = (EigrpFirstPkt_pt)(gpEigrp->firstPktbuf + cnt * sizeof(struct EigrpFirstPkt_));
			retVal = EigrpProcPkt(firstPkt->pktLen, firstPkt->pktBuf, firstPkt->pCirc);
		}
		gpEigrp->firstPktCnt = 0;
		EigrpPortMemSet(gpEigrp->firstPktbuf, 0, sizeof(gpEigrp->firstPktbuf));
	}
#if 0
	printf("EigrpRecvforFirst:1\n");
	EigrpProcPkt(size, msgbuf, pCirc);
	EIGRP_FUNC_LEAVE(EigrpRecvforFirst);
	return 0;
#endif
	FPKT_LOCK_GIVE(gpEigrp->firstPktLock);
	EIGRP_FUNC_LEAVE(EigrpProcDcPkt);
	return;
}
#endif//#ifdef _DC_

S32 EigrpProcPkt(U32 size, U8 *msgbuf, void *pCirc)
{
	U32				source;
	EigrpPktHdr_st	*eigrph;
	EigrpPdb_st		*pdb;
	struct EigrpIpHdr_	*ip;
	EigrpIntf_pt		pEigrpIntf;
	S32				ipHdrLen, retVal;
//#ifdef EIGRP_PACKET_UDP
	struct EigrpSockIn_ peer, local;
//#endif//EIGRP_PACKET_UDP
	U32				flag, opcode;
	U32				srcAddr;
	U16				srcPort;

#ifdef EIGRP_PACKET_UDP
	eigrph			= (EigrpPktHdr_st*)((U8 *)msgbuf);
	source			= srcAddr;	
	peer.sin_addr.s_addr	= HTONL(srcAddr);
#else//EIGRP_PACKET_UDP
	ip				= (struct EigrpIpHdr_*) msgbuf;
	ipHdrLen			= ip->hdrLen * 4 ;
	eigrph			= (EigrpPktHdr_st*)((U8 *) ip + ipHdrLen);
	source			= NTOHL(ip->srcIp); 
	peer.sin_addr.s_addr	= ip->srcIp;
	local.sin_addr.s_addr	= ip->dstIp;
#endif//EIGRP_PACKET_UDP

	/* Check is this packet comming from myself? */
	if(EigrpIntfFindByAddr(source)){
		EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP: Ignore packet comes from myself, source :%s size =%d\n",
					EigrpUtilIp2Str(peer.sin_addr.s_addr), size);
		EIGRP_FUNC_LEAVE(EigrpRecv);
		return -1;
	}
#if	(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
	{
		if(EigrpPortCircIsUnnumbered(pCirc)){
			EigrpPortSetUnnumberedIf(pCirc, source);
			/* tigerwh 120611 changed begin */
			pEigrpIntf = EigrpIntfFindByPeer(source);
			if(pEigrpIntf && pEigrpIntf->sysCirc == pCirc && 
					(pEigrpIntf->remoteIpAddr!= source || pEigrpIntf->updateFinished != TRUE)){
				retVal	= EigrpPortSetUnnumberedIf(pCirc, source);
				if(retVal == FAILURE){
					EIGRP_FUNC_LEAVE(EigrpRecv);
					return -1;
				}
				pEigrpIntf	= EigrpIntfFindByPeer(source);
				if(!pEigrpIntf){
					EIGRP_FUNC_LEAVE(EigrpRecv);
					return -1;
				}
				EigrpIpIpAddressChange(pEigrpIntf, 0, 0, TRUE);
				pEigrpIntf->updateFinished	= TRUE;
			}
			/* tigerwh 120611 changed end */
		}
		pEigrpIntf = EigrpIntfFindByPeer(source);	/* tigerwh 120611 changed */
		if(!pEigrpIntf){/* tigerwh 120611 changed add "|| pEigrpIntf->sysCirc != pCirc" */
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EIGRP: Unknown income interface, from: %s, \tsize =%d\n",
						EigrpUtilIp2Str(peer.sin_addr.s_addr), size);
			EIGRP_FUNC_LEAVE(EigrpRecv);
			return -1;
		}
	}
#elif	(EIGRP_OS_TYPE == EIGRP_OS_PIL)
	{
		if(EigrpPortCircIsUnnumbered(pCirc)){
			EigrpPortSetUnnumberedIf(pCirc, source);
		}
		pEigrpIntf = EigrpIntfFindByPeer(source);
		if(!pEigrpIntf || pEigrpIntf->sysCirc != pCirc){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP: Unknown income interface, from: %s, \tsize =%d\n", 
						EigrpUtilIp2Str(peer.sin_addr.s_addr), size);
			EIGRP_FUNC_LEAVE(EigrpRecv);
			return -1;
		}
	}
#elif   (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
	{
		pEigrpIntf = EigrpIntfFindByPeer(source);
		if(!pEigrpIntf){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV, "EIGRP: Unknown income interface, from: %s, \tsize =%d\n",
						EigrpUtilIp2Str(NTOHL(peer.sin_addr.s_addr)), size);
			EIGRP_FUNC_LEAVE(EigrpRecv);
			return -1;
		}
	}
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
		pdb = EigrpIpFindPdb(NTOHL(eigrph->asystem));

		/* no such asysterm then next packet */
		if(!pdb){
			EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP: Got unknown AS:%d EIGRP packet, from: %s, \tsize =%d\n",
						NTOHL(eigrph->asystem), EigrpUtilIp2Str(NTOHL(peer.sin_addr.s_addr)), size);
			EIGRP_FUNC_LEAVE(EigrpRecv);
			return -1;
		}

#ifndef EIGRP_PACKET_UDP
	size -= ipHdrLen;
#endif//EIGRP_PACKET_UDP

	if(size <= 0){
		EIGRP_FUNC_LEAVE(EigrpRecv);
		return -1;
	}
	opcode	= eigrph->opcode;
	if(eigrph->opcode == EIGRP_OPC_HELLO && eigrph->ack){
		opcode	= EIGRP_OPC_ACK;
	}
	flag	= EigrpDebugPacketType(opcode, 1);
	flag |= DEBUG_EIGRP_INTERNAL;
	
#ifdef EIGRP_PACKET_UDP
	EIGRP_TRC(flag,"EIGRP-RECV: %-6s PACKET \t%s->%s \tsize =%d\n",
						EigrpOpercodeItoa(eigrph->opcode),
						EigrpUtilIp2Str(NTOHL(peer.sin_addr.s_addr)),
						"local-addr",
						size);
#else//EIGRP_PACKET_UDP
	EIGRP_TRC(flag,"EIGRP-RECV: %-6s PACKET \t%s->%s \tsize =%d\n",
						EigrpOpercodeItoa(eigrph->opcode),
						EigrpUtilIp2Str(NTOHL(peer.sin_addr.s_addr)),
						EigrpUtilIp2Str(NTOHL(local.sin_addr.s_addr)),
						size);
#endif//EIGRP_PACKET_UDP

	flag = EigrpDebugPacketDetailType(opcode);
	EigrpDebugPacketDetail(flag, eigrph, size);

		/* process hello packet, do it and then next packet */
		/* currently we always return FALSE except the peer is going down */
		if(EigrpUpdatePeerHoldtimer(pdb->ddb, &source, pEigrpIntf)){
			EIGRP_FUNC_LEAVE(EigrpRecv);
			return -1;
		}
	EigrpReceivedPktProcess(pdb, eigrph, size, &source, pEigrpIntf);
	EIGRP_FUNC_LEAVE(EigrpRecv);
	return 0;
}

S32	EigrpRecv()
{
//	U32				source;
//	EigrpPktHdr_st	*eigrph;
//	EigrpPdb_st		*pdb;
//	struct EigrpIpHdr_	*ip;
	void				*pCirc = NULL;        
//	EigrpIntf_pt		pEigrpIntf;
	S32				size, retVal;
#ifdef EIGRP_PACKET_UDP	
	struct EigrpSockIn_ peer, local;
	U32				srcAddr;
	U16				srcPort;
#endif//#ifdef EIGRP_PACKET_UDP	
//	U32				flag, opcode;

	EIGRP_FUNC_ENTER(EigrpRecv);
	/* keep reading */
	EigrpEventReadAdd();

#ifdef EIGRP_PACKET_UDP
	retVal = EigrpPortRecvIpPacket(gpEigrp->socket, &size, gpEigrp->recvBuf, &srcAddr, &srcPort, &pCirc);
#else//EIGRP_PACKET_UDP
	retVal = EigrpPortRecvIpPacket(gpEigrp->socket, &size, gpEigrp->recvBuf, &pCirc);
#endif
	if(retVal == SUCCESS){
		EigrpProcPkt(size, gpEigrp->recvBuf, pCirc);
	}
	EIGRP_FUNC_LEAVE(EigrpRecv);

	return 0;
}

/************************************************************************************

	Name:	EigrpPrintDest

	Desc:	This function is to print dest ip address with the length limit.
		
	Para: 	len		- ip address length (with out zero byte)
			dest		- pointer to the destination ip address
	
	Ret:		pointer to the ip address string (with the zero byte)
************************************************************************************/

S8 *EigrpPrintDest(S8 len, U8 *dest)
{
	static S8	address[16];
	S8		temp[8];
	S32		i;

	EIGRP_FUNC_ENTER(EigrpPrintDest);
	address[0] = '\0';
	for(i= 0; i<4; i++){
		if(i<len){
			sprintf_s(temp, sizeof(temp), "%d.", dest[i]);
		}else{ 	
			sprintf_s(temp, sizeof(temp), "%d.",0);
		}
		EigrpPortStrCat(address,temp);
	}
	address[EigrpPortStrLen(address)] = '\0';
	EIGRP_FUNC_LEAVE(EigrpPrintDest);
	
	return address;
}

/************************************************************************************

	Name:	EigrpDebugPacketDetail

	Desc:	This function is display eigrp packet details.
		
	Para: 	eigrph	- pointer to eigrp packet header
			size		- length of the eigrp packet
	
	Ret:		NONE
************************************************************************************/

void	EigrpDebugPacketDetail(U32 flag, EigrpPktHdr_st *eigrph, S32 size)
{
	EigrpGenTlv_st			*tlv;
	EigrpIpExtern_st			*ipe;        
	EigrpIpMetric_st			*ipm;        
	EigrpParamTlv_st		*ipp;
	EigrpVerTlv_st		*ipv;
	EigrpNextMultiSeq_st	*ipn;

	S16	len,	type;

	EIGRP_FUNC_ENTER(EigrpDebugPacketDetail);
	/*  we do not print in this function */
	if(size < sizeof(EigrpPktHdr_st)){
		EIGRP_FUNC_LEAVE(EigrpDebugPacketDetail);
		return;
	}

	EIGRP_TRC(flag, "\n\rEigrp Packet Header" \
				"\n\r\tVersion=%d   Opcode =%s          Checksum=%x" \
				"\n\r\tFlags=%x  Sequence =%d    ACK=%d     AS=%d",
				eigrph->version,
				EigrpOpercodeItoa(eigrph->opcode),
				eigrph->checksum,
				eigrph->flags,
				eigrph->sequence,
				eigrph->ack,
				eigrph->asystem);

	tlv	= (EigrpGenTlv_st*)((U8 *)eigrph + sizeof(EigrpPktHdr_st));
	size	-= sizeof(EigrpPktHdr_st);

	switch(eigrph->opcode){
		case EIGRP_OPC_QUERY:
		case EIGRP_OPC_REPLY:
		case EIGRP_OPC_UPDATE:
		case EIGRP_OPC_HELLO:
			break;
			
		default:
			EIGRP_FUNC_LEAVE(EigrpDebugPacketDetail);
			return;
	}

	while(size > 0){
		len	= NTOHS(tlv->length);

/* tigerwh 120517
#if	(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
*/
	/* tigerwh just to avoid packet error! */
		if(len == 0){
			return;
		}
/*
#endif
*/
		type	= NTOHS(tlv->type) ;
		if(len < sizeof(EigrpGenTlv_st)){
			tlv	= (EigrpGenTlv_st*)((U8 *)tlv + len);
			size	-= len; 
			continue;
		}

		if(type ==EIGRP_PARA){
			ipp = (EigrpParamTlv_st *) tlv;
			EIGRP_TRC(flag, "\rTLV\r\n\tType =Parametes \tLength=%d " \
							"\n\r\tk1=%d  k2=%d  k3=%d  k4=%d " \
							"\r\n\tk5=%d  Reserved= 0x%x        Hold Time =%d\n",
							len,
							ipp->k1, ipp->k2, ipp->k3, ipp->k4,
							ipp->k5, ipp->pad, ipp->holdTime);
		}
		if(type ==EIGRP_SEQUENCE){
			S8	*addr_ptr;
			S32	length, len0;
			
			EIGRP_TRC(flag, "\r\tTLV\r\n\tType =Sequence \tLength=%d ", len);
			addr_ptr = (S8 *)(tlv + 1);
			length = len - sizeof(EigrpGenTlv_st);
			while(length>0){   
				EIGRP_TRC(flag, "\r\tLen=%d \tAddress= 0x%x", *addr_ptr, *(U32*)(addr_ptr + 1));
				len0		= *addr_ptr + 1;
				length	-= len0;
				addr_ptr	+= len0;
			}
		}
		if(type ==EIGRP_SW_VERSION){
			ipv = (EigrpVerTlv_st *) tlv;
			EIGRP_TRC(flag, 
						"\rTLV\r\n\tType =Software Version \tLength=%d " \
						"\r\n\tvos_majorver =%d    minVer =%d " \
						"\r\n\teigrp_majorrev=%d  eigrpMinVer=%d ", 
						len,
						ipv->version.majVer, ipv->version.minVer,
						ipv->version.eigrpMajVer, ipv->version.eigrpMinVer);
		}
		if(type ==EIGRP_NEXT_MCAST_SEQ){
			ipn = (EigrpNextMultiSeq_st *) tlv;
			EIGRP_TRC(flag,
						"\rTLV\r\n\tType =Next MCast Sequence \tLength=%d " \
						"\r\n\tseq_num=%d ", len, ipn->seqNum);

		}else if(type ==EIGRP_METRIC){
			EIGRP_TRC(flag, 	"\rTLV\r\n\tType =Internal \tLength=%d ", len);
			ipm = (EigrpIpMetric_st *) tlv;
			EIGRP_TRC(flag, 
						"\r\tNextHop= 0x%x \n\r\tDelay=%d \n\r\tBandthwidth=%d " \
						"\r\n\tMTU=%d     HopCount=%d",
						ipm->nexthop, *(U32*)(ipm->delay), *(U32*)(ipm->bandwidth),
						(*(U32*)(ipm->mtu))/256, (*(U32*)(ipm->mtu))%256);
			EIGRP_TRC(flag, 
						"\r\tReliablility=%d load=%d      Reserved= 0x%x" \
						"\r\n\tMask=%d Destination=%s",
						ipm->reliability, ipm->load, *(U16*)(ipm->mreserved),
						ipm->intDest[0].mask, EigrpPrintDest(ipm->intDest[0].mask, ipm->intDest[0].number));
		}else if(type ==EIGRP_EXTERNAL){
			EIGRP_TRC(flag, "\rTLV\r\n\tType =External \tLength=%d ", len);
			ipe = (EigrpIpExtern_st *) tlv;
			EIGRP_TRC(flag, 
						"\r\tNextHop= 0x%x \n\r\tOriginating Router = 0x%x  \n\r\tOriginating Asystem=%d",
						ipe->nexthop, ipe->routerId, ipe->asystem);
			EIGRP_TRC(flag, 
						"\r\tArbitrary Tag= 0x%x\n\r\tExternal Protocol Metric= 0x%x" \
						"\r\n\t Reserved= 0x%x    External Protocol ID= 0x%x Flags= 0x%x",
						ipe->tag, ipe->metric, ipe->ereserved, ipe->protocol, ipe->flag);
			EIGRP_TRC(flag, "\r\tDelay=%d \n\r\tBandthwidth=%d" \
						"\r\n\tMTU=%d HopCount=%d",
						*(U32*)(ipe->delay), *(U32*)(ipe->bandwidth),
						(*(U32*)(ipe->mtu))/256, (*(U32*)(ipe->mtu))%256);
			EIGRP_TRC(flag, 
						"\r\tReliablility=%d load=%d         Reserved= 0x%x" \
						"\r\n\tMask=%d     Destination=%s",
						ipe->reliability, ipe->load, *(U16*)(ipe->mreserved),
						ipe->extDest[0].mask, 
						EigrpPrintDest(ipe->extDest[0].mask, ipe->extDest[0].number));
		}
		size	-= len; 
		tlv	= (EigrpGenTlv_st*)((U8 *)tlv + len);
	}
	EIGRP_FUNC_LEAVE(EigrpDebugPacketDetail);

	return;
}

/************************************************************************************

	Name:	EigrpPacketData

	Desc:	This function is to return a pointer to the data field in an EIGRP packet, given a 
			pointer to the packet itself.
		
	Para: 	eigrph	- pointer to the EIGRP packet 
	
	Ret:		pointer to the data field int an EIGRP packet
************************************************************************************/

void *EigrpPacketData(EigrpPktHdr_st *eigrph)
{
	return((S8 *)eigrph + EIGRP_HEADER_BYTES);
}

/************************************************************************************

	Name:	EigrpAddressDecode

	Desc:	This function is to convert address/mask pair from packet into internal format.  Return
			number of significant octets.
			
	Para: 	ipd		- pointer to the address in packet format
 			am		- pointer to the address in internal format 
	
	Ret:		length of mask. If failed, return -1
************************************************************************************/

S32	EigrpAddressDecode(EigrpIpMpDecode_st *ipd, EigrpNetEntry_st *am)
{
	S32 bytes;
	U32	nboAddr;

	EIGRP_FUNC_ENTER(EigrpAddressDecode);
	bytes = (ipd->mask + 7) / 8;
	if(bytes > sizeof(U32)){
		EIGRP_FUNC_LEAVE(EigrpAddressDecode);
		return( -1);
	}
	am->address = 0L;

	if(ipd->mask){
		nboAddr	= 0;
		EigrpPortMemCpy((U8 *) &nboAddr, (void *)ipd->number, (U32) bytes);

		am->address	= NTOHL(nboAddr);
		am->mask	= 0xffffffff << (32 - ipd->mask);
	}else{
		am->mask = 0;
		EIGRP_FUNC_LEAVE(EigrpAddressDecode);
		return(1);
	}
	EIGRP_FUNC_LEAVE(EigrpAddressDecode);
	
	return(bytes);
}

/************************************************************************************

	Name:	EigrpMetricDecode

	Desc:	This function is to pull metric out of packet and salami it into the infotype structure.
		
	Para: 	im		- pointer to the metric in internal format
			metric	- pointer to the metric in packet format
	
	Ret:		NONE
************************************************************************************/

void	EigrpMetricDecode(EigrpVmetric_st *im, EigrpVmetricDecode_st *metric)
{
	U32 mtu;

	EIGRP_FUNC_ENTER(EigrpMetricDecode);
	EigrpPortMemCpy((void *) & im->bandwidth, (void *) metric->bandwidth,  4);
	im->bandwidth = NTOHL(im->bandwidth);
	EigrpPortMemCpy((void *) & im->delay, (void *) metric->delay, 4);
	im->delay = NTOHL(im->delay);

	if(NTOHS(0x1234) == 0x1234){
		EigrpPortMemCpy((void *) & mtu, (void *) metric->mtu, 3);  /* ABC -> ABC0 */
		im->mtu = mtu >> 8;             /* ABC0 -> 0ABC */
	}else{
		/* Intel CPU */
		/* net-byte-order diff with host-byte-order */
		EigrpPortMemCpy((void *) & mtu, (void *) metric->mtu, 3);  /* ABC -> ABC0 */
		im->mtu = mtu << 8;             /* ABC0 -> 0ABC */
		im->mtu = NTOHL(im->mtu);     /* 0ABC -> CBA0 */
	}

	im->hopcount	= metric->hopcount;
	im->reliability	= metric->reliability;
	im->load		= metric->load;
	EIGRP_FUNC_LEAVE(EigrpMetricDecode);

	return;
}

/************************************************************************************

	Name:	EigrpProcRoute

	Desc:	This function is to grab metric, compute composite metric, get destinations/masks	
			from packet and fire off call to dual.  
		
	Para: 	ipd		- pointer to the address in packet format
			limit		- pointer to the end of the packet
			route	- pointer to the route which contains route parameters
			metric	- vector	metric of received packet
			ddb		- pointer to the dual descriptor block 
			pkt_type	- the type of the received packet
			ignore_tlv	- the sign by which we can judge if the TLV should be ignored
			address		- the ip address of route next hop
			peer		- the peer from which the packet received
			extd		- pointer to the extened data
	
	Ret:		TRUE	for success
			FALSE 	for if we run out of memory, or the packet has bogus data.
************************************************************************************/

S32	EigrpProcRoute(EigrpIpMpDecode_st *ipd, S8 *limit, EigrpDualNewRt_st *route, EigrpVmetricDecode_st *metric,
									EigrpDualDdb_st *ddb, U32 pkt_type, S32 ignore_tlv, U32 address, 
									EigrpDualPeer_st *peer, void *extd)
{
	EigrpPdb_st		*pdb;
	S32				length;
	EigrpExtData_st	*data;
	U16				rtType;
	U32				source, bandwidth, delay;
	EigrpIntf_pt		pEigrpIntf;
	EigrpIntfAddr_pt	pAddr;
	EigrpDualNdb_st	*dndb;
	EigrpDualRdb_st	*drdb;

	EIGRP_FUNC_ENTER(EigrpProcRoute);
	pEigrpIntf = EigrpDualIdb(route->idb);
	if(!pEigrpIntf){
		EIGRP_ASSERT(0);
		EIGRP_FUNC_LEAVE(EigrpProcRoute);
		return FALSE;
	}

	pdb		= ddb->pdb;
	source	= peer->source;

	EigrpMetricDecode(&route->vecMetric, metric);

	while(ipd < (EigrpIpMpDecode_st *) limit){
		route->flagIgnore	= ignore_tlv;
		route->nextHop	= (address == 0) ? source : address;
		length = EigrpAddressDecode(ipd, &route->dest);

		if(length == -1){        
			EIGRP_FUNC_LEAVE(EigrpProcRoute);
			return FALSE;
		}

#if 0/*zhenxl_del_20130125*/
		/*zhenxl_20130121*/
		dndb = EigrpDualNdbLookup(ddb, &route->dest);
		if(dndb){
			drdb = dndb->rdb;
			while(drdb){	
				if(drdb->iidb == route->idb){
					if(drdb->vecMetric.hopcount == 1){/*Direct connetion link.*/
						if(route->vecMetric.hopcount > 0){
							EIGRP_FUNC_LEAVE(EigrpProcRoute);
							return TRUE;
						}
					}
				}
				drdb = drdb->next;
			}
		}
#endif

		/* tigerwh add 120527 begin */
		if(route->dest.address == EigrpPortGetRouterId() && 
				route->dest.mask == 0xffffffff && 
				route->vecMetric.delay != 0xffffffff){
			ipd = (EigrpIpMpDecode_st *)((S8 *) ipd + length + 1);
			continue;
		}
		/* tigerwh add 120527 end */

		/*
		*  We should not receive invalid network address as 255.0.0.0             */
		if(! EIGRP_INET_CLASS_VALID_BYTE(route->dest.address)){
			EIGRP_FUNC_LEAVE(EigrpProcRoute);
			return TRUE;
		}
		if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT)){
			pAddr	= EigrpPortGetFirstAddr(pEigrpIntf->sysCirc);
			if(!pAddr){	
				EIGRP_FUNC_LEAVE(EigrpProcRoute);
				return FALSE;
			}
			if(pAddr->ipDstAddr == route->dest.address){
				EigrpPortMemFree(pAddr);
				EIGRP_FUNC_LEAVE(EigrpProcRoute);
				return TRUE;
			}
			EigrpPortMemFree(pAddr);
		}

		route->metric = EigrpComputeMetric(ddb, &route->vecMetric, &route->dest,
													route->idb, &route->succMetric);

		_EIGRP_DEBUG("EigrpProcRoute: Compute %s/%d %s metric=%d\n",
					EigrpUtilIp2Str(route->dest.address),
					EigrpUtilMask2Len(route->dest.mask),
					EigrpUtilIp2Str(route->nextHop),
					route->metric);
		route->opaqueFlag = 0;

		/* Copy the external data for external destinations. */
		if(route->flagExt){
			if(!extd){	
				EigrpPortAssert(0,"");
				EIGRP_FUNC_LEAVE(EigrpProcRoute);
				return FALSE;
			}
			data = (EigrpExtData_st *) EigrpUtilChunkMalloc(ddb->extDataChunk);
			if(!data){
				EIGRP_FUNC_LEAVE(EigrpProcRoute);
				return FALSE;
			}
			EigrpPortMemCpy((void *) data, (void *) extd, sizeof(EigrpExtData_st));

			/* If route is marked as a candidate default, consider performing input filtering on it. */
			if(data->flag & EIGRP_INFO_CD){
				if(pdb->cdIn == FALSE){       
					data->flag &= ~EIGRP_INFO_CD;
				}
			}
			route->data = data;
		}else{			/* Not external */
			route->data = NULL;

			if(peer->peerVer.majVer){
				/* We can't trust the flag bit information from older routers. The flag bit 
				  * information was carved out of a reserved field which older routers neglected
				  * to initialize to zero. So only trust the flag bits if we know that our neighbor
				  * is running new enough code that it can tell us what software version it is
				  * running. */
				route->opaqueFlag = EIGRP_METRIC_TYPE_FLAGS(metric);
				/* If route is marked as a candidate default, consider performing input filtering 
				  * on it. */
				if(route->opaqueFlag & EIGRP_INFO_CD){
					if(pdb->cdIn  == FALSE){
						route->opaqueFlag &= ~EIGRP_INFO_CD;
					}
				}
			}
		}

		/* At this point check to see if the ignore flag should be set in the topology table.  Only
		  * consider distance less than EIGRP_MAX_DISTANCE and must pass input filtering. */
		if(!route->flagIgnore){
			rtType = (route->flagExt) ? EIGRP_RT_TYPE_EXT : EIGRP_RT_TYPE_INT;

			route->flagIgnore = FALSE ;

			if(route->dest.mask == 0){
				if(pdb->rtInDef == FALSE){
					route->flagIgnore = TRUE;
				}
			}

			if(route->flagIgnore == FALSE){
				if(EigrpPortPermitIncoming(route->dest.address & route->dest.mask, 
												route->dest.mask, route->idb) != TRUE){
					route->flagIgnore = TRUE ;
				}
			}
		}

		EigrpPortMemCpy((void *) & bandwidth, (void *) metric->bandwidth, 4);
		EigrpPortMemCpy((void *) & delay, (void *) metric->delay, 4);
		EIGRP_TRC(DEBUG_EIGRP_ROUTE,"IP-EIGRP RECV: %s %s M %u - %u %u SM %u - %u %u from %s\n",
										(route->data) ? "Ext" : "Int",
										EigrpIpPrintNetwork(&route->dest),
										route->metric,
										route->vecMetric.bandwidth, route->vecMetric.delay,
										route->succMetric,
										NTOHL(bandwidth), NTOHL(delay), 
										EigrpIpPrintAddr(&route->nextHop));

		EigrpDualRecvPacket(ddb, peer, route, pkt_type);
		ipd = (EigrpIpMpDecode_st *)((S8 *) ipd + length + 1);
	} /* end of main while */
	EIGRP_FUNC_LEAVE(EigrpProcRoute);
	
	return TRUE;
}

/************************************************************************************

	Name:	EigrpRoutesDecode

	Desc:	This function is to transform update, query or reply packet into internal data structures.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- the peer from which the packet received
			tlv		- pointer to the first TLV
			bytes	- length of packet
			packet_type	- type of packet
	
	Ret:		NONE
************************************************************************************/

void	EigrpRoutesDecode(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer,
								EigrpGenTlv_st *tlv, S32 bytes, U32 packet_type)
{
	EigrpPdb_st		*pdb;
	S8				*limit;
	U16				length;
	EigrpDualNewRt_st	route;
	EigrpIpExtern_st	*ipe;        /* external route tvl */
	EigrpIpMetric_st	*ipm;        /* internal route tvl */
	S32				ignore_tlv;

	EIGRP_FUNC_ENTER(EigrpRoutesDecode);
	EigrpUtilMemZero((void *) & route, sizeof(EigrpDualNewRt_st));
	pdb				= ddb->pdb;
	route.infoSrc		= peer->source;
	route.idb			= peer->iidb;
	route.origin		= EIGRP_ORG_EIGRP;
	route.rtType		= EIGRP_ROUTE_EIGRP ;

	EIGRP_TRC(DEBUG_EIGRP_ROUTE,"IP-EIGRP: Processing incoming %s packet from %s\n",
		EigrpOpercodeItoa((S32)(packet_type)), EigrpIpPrintAddr(&peer->source));
	while(bytes > 0){
		/* Let's inject a little sanity checking.  This is the same case as default in the switch statement. */
		length = NTOHS(tlv->length);
#if	(EIGRP_OS_TYPE==EIGRP_OS_LINUX)
	/* tigerwh just to avoid packet error! */
		if(length == 0){
			return;
		}
#endif

		if(length < sizeof(EigrpGenTlv_st)){
			EigrpDualFinishUpdateSend(ddb);
			EIGRP_FUNC_LEAVE(EigrpRoutesDecode);
			return;
		}

		limit = (S8 *) tlv + length;
		switch(NTOHS(tlv->type)){
			case EIGRP_METRIC:                   /* Internal Route TLV */
				_EIGRP_DEBUG("%s:EIGRP_METRIC\n",__func__);
				ipm = (EigrpIpMetric_st *) tlv;
				route.flagExt = FALSE;
				route.data = NULL;

				/* We stroe IP with net-byte-order in all routing protocols, so NTOHL() is not 
				  * right, delete it!*/
				if(!EigrpProcRoute(ipm->intDest, limit, &route,
											(EigrpVmetricDecode_st *)(&(ipm->delay)), ddb,
											packet_type , FALSE,
											ipm->nexthop,
											peer, NULL)){
					EigrpDualFinishUpdateSend(ddb);
					EIGRP_FUNC_LEAVE(EigrpRoutesDecode);
					return;
				}
				break;

			case EIGRP_EXTERNAL:
				_EIGRP_DEBUG("%s:EIGRP_EXTERNAL\n",__func__);
				route.flagExt = TRUE;
				ipe = (EigrpIpExtern_st *) tlv;
				route.metric	= ipe->metric;/** tigerwh 100321 added **/

				/* If we are the source of this advertisement then reject it. */
				ignore_tlv = (NTOHL(ipe->routerId) == ddb->routerId);/* ignore route broadcasted from my self */
				if(ignore_tlv == TRUE)														
					EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP-RECV: Get Peer RouteId is same of local\n");
				_EIGRP_DEBUG("%s: ddb->routerId %x ipe->routerId %x\n",__func__, ddb->routerId,NTOHL(ipe->routerId) );
				if(ignore_tlv == TRUE && packet_type == EIGRP_OPC_UPDATE){/*zhenxl_20130114 Ignore UPDATE of external route which origined from my self*/
					bytes -= length;
					tlv = (EigrpGenTlv_st *)((S8 *) tlv + length);
					continue;
				}
				route.originRouter = NTOHL(ipe->routerId);/*zhenxl_20130116 Record the origin router of this external route.*/

				/* We stroe IP with net-byte-order in all routing protocols, so NTOHL() is not 
				  * right, delete it! */
				if(!EigrpProcRoute(ipe->extDest, limit, &route,
											(EigrpVmetricDecode_st *)(&(ipe->delay)), ddb,
											packet_type , ignore_tlv,
											ipe->nexthop,
											peer, &ipe->routerId)){
					EigrpDualFinishUpdateSend(ddb);
					EIGRP_FUNC_LEAVE(EigrpRoutesDecode);
					return;
				}
				break;

			default:
				/* Skip the unknown TLV. */
				EIGRP_TRC(DEBUG_EIGRP_PACKET_RECV,"EIGRP-RECV: Get an unknown TVL\n");
				break;
		}
		bytes	-= length;
		tlv		= (EigrpGenTlv_st *)((S8 *) tlv + length);
	}
	EIGRP_TRC(DEBUG_EIGRP_ROUTE,"IP-EIGRP: Processing incoming %s packet end\n",
				EigrpOpercodeItoa((S32)(packet_type)));
	/* Finish up any Updates we felt it necessary to send. */
	EigrpDualFinishUpdateSend(ddb);
	EIGRP_FUNC_LEAVE(EigrpRoutesDecode);

	return;
}

/************************************************************************************

	Name:	EigrpProbeDecode

	Desc:	This function is to process an incoming probe packet.
		
	Para: 	ddb		- pointer to the dual descriptor block 
			peer		- the peer from which the packet received
			eigrp	- pointer to the eigrp packet header
	
	Ret:		NONE
************************************************************************************/

void	EigrpProbeDecode(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer, EigrpPktHdr_st *eigrp)
{
	return;
}

/************************************************************************************

	Name:	EigrpReceivedPktProcess

	Desc:	This function is the main eigrp routing process.  One per major network.
		
	Para: 	pdb		- pointer to the process descriptor block
			eigrph	- pointer to the received packet header
			source	- pointer to the packet source ip address
			pEigrpIntf	- ther logical interface on which the packet is recvived
	
	Ret:		NONE
************************************************************************************/

void	EigrpReceivedPktProcess(EigrpPdb_st *pdb, EigrpPktHdr_st *eigrph, S32 size, U32 *source, EigrpIntf_pt pEigrpIntf)
{
	EigrpDualDdb_st	*ddb;
	EigrpDualPeer_st	*peer;
	EigrpGenTlv_st	*tlv;

	ddb = pdb->ddb;

	EIGRP_FUNC_ENTER(EigrpReceivedPktProcess);
	if(BIT_TEST(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE)){
		/* Process fixed header. */
		peer = (EigrpDualPeer_st *)EigrpHeadProc(ddb, eigrph, size, source, pEigrpIntf);

		/* Switch on packet type. Routines must check version number. */
		if(peer){
			switch(eigrph->opcode){
				case EIGRP_OPC_QUERY:
				case EIGRP_OPC_REPLY:
				case EIGRP_OPC_UPDATE:
					tlv = EigrpPacketData(eigrph);
					EigrpRoutesDecode(ddb, peer,  tlv, size - sizeof(EigrpPktHdr_st), eigrph->opcode);
					break;

				case EIGRP_OPC_PROBE:
					EigrpProbeDecode(ddb, peer, eigrph);
					break;

				default:
					break;
			}
		}

		EigrpDualFinishUpdateSend(ddb);
	}
	EIGRP_FUNC_LEAVE(EigrpReceivedPktProcess);

	return;
}

/************************************************************************************

	Name:	EigrpMetricEncode

	Desc:	This function is to fill in the contents of a packet metric block from a DNDB.
 
 			If "unreachable" is TRUE, the route is marked as being unreachable.
		
	Para: 	ddb		- pointer to the dual descriptor block
			dndb	- pointer to the network entry
			iidb		- pointer to vector metric 
			unreachable	- the sign by which we judge the route as being unreachable
			connect	- the sign by which we judge if the hop count if zero
	Ret:		
************************************************************************************/

void	EigrpMetricEncode(EigrpDualDdb_st *ddb, EigrpDualNdb_st *dndb,
							EigrpIdb_st *iidb, EigrpVmetricDecode_st *metric,
							S32 unreachable, S32 connect)
{
	EigrpVmetric_st	*vecmetric;
	U32	mtu, delay, offset;

	EIGRP_FUNC_ENTER(EigrpMetricEncode);
	vecmetric = &dndb->vecMetric;

	/* Mark the route as unreachable if asked. */
	if(unreachable){
		delay = EIGRP_METRIC_INACCESS;
	}else{
		delay = vecmetric->delay;
	}

	if(ddb->mmetricFudge){
		offset = (*ddb->mmetricFudge)(ddb, &dndb->dest, iidb, EIGRP_OFFSET_OUT);
		if(offset > EIGRP_METRIC_INACCESS - delay){
			delay = EIGRP_METRIC_INACCESS;
		}else{
			delay += offset;
		}
	}
	EigrpPortMemSet((U8 *)metric, 0, sizeof(EigrpVmetricDecode_st));
	*(U32*) metric->bandwidth	= HTONL(vecmetric->bandwidth);
	*(U32*) metric->delay			= HTONL(delay);

	if(NTOHS(0x1234) == 0x1234){
		/* Motorola CPU */
		/* net-byte-order same with host-byte-order */
		mtu = (vecmetric->mtu) & 0x0000ffff;	/* 0ABC -> 00BC */
		mtu = mtu << 8;						/* 00BC -> 0BC0 */
		EigrpPortMemCpy((void *) metric->mtu, (void *) & mtu, 3);	/* 0BC0 -> 0BC(ABC) */
	}else{
		/* net-byte-order diff with host-byte-order */
		mtu = (vecmetric->mtu) & 0x0000ffff;	/* CBA0 -> CB00 */
		mtu = HTONL(mtu);					/* CB00 -> 00BC */
		mtu = mtu >> 8;						/* 00BC -> 0BC0 */
		EigrpPortMemCpy((void *) metric->mtu, (void *) & mtu, 3);        /* 0BC0 -> 0BC(ABC) */
	}

	metric->hopcount	= connect ? 0 : vecmetric->hopcount;
	metric->reliability	= vecmetric->reliability;
	metric->load		= vecmetric->load;
	EIGRP_FUNC_LEAVE(EigrpMetricEncode);

	return;
}

/************************************************************************************

	Name:	EigrpAddItem

	Desc:	This function is to build an item from the supplied DNDB and add it to the packet.
 			
  			Note that we do not put multiple destinations into a TLV any more.
			This could still be done as an optimization to make the packets smaller.
			Maybe Mikel will do this.
			
	Para:	ddb		- pointer to dual descriptor block 
			iidb		- eigrp interface on which the packet to be sent
			dndb	- network entry which need to be encoded
			packet_ptr	- pointer to packet
			bytes_left	- the length of the left bytes
			unreachable	- the sign by which we judge the dndb as being unreachable
			opcode	- operation code of packet
			
	Ret:		length of the item, or 0 if there wasn't enough room to
			add the item.
************************************************************************************/

U32	EigrpAddItem(EigrpDualDdb_st *ddb, EigrpIdb_st *iidb, EigrpDualNdb_st *dndb, void *packet_ptr,
						U32 bytes_left, S32 unreachable, S8 opcode, EigrpDualPeer_st *peer/*add_zhenxl for INCLUDE_SATELLITE_RESTRICT*/)
{
	U32				item_length;
	EigrpDualRdb_st		*drdb;
	U32				dest_size;
	S32				external;
	EigrpIpMetric_st	*internal_route;
	EigrpIpExtern_st	*external_route;
	EigrpIpMpDecode_st	*dest;
	U32				origin, nboAddr;
	U32				flags;
	S8				*pMetric;
	EIGRP_VALID_CHECK_VOID();

	EIGRP_FUNC_ENTER(EigrpAddItem);
	dest_size	= 1 + EigrpIpBytesInMask(dndb->dest.mask);
	flags		= 0;
	external	= EigrpDualRouteIsExternal(dndb);

	/* Bail if there's no room for the item.  (There better be!) */
	if(external){
		item_length = EIGRP_EXTERN_TYPE_SIZE + dest_size;
	}else{
		item_length = EIGRP_METRIC_TYPE_SIZE + dest_size;
	}
	if(item_length > bytes_left){
		EIGRP_FUNC_LEAVE(EigrpAddItem);
		return(0);
	}

	drdb = dndb->rdb;
	EIGRP_TRC(DEBUG_EIGRP_ROUTE,"IP-EIGRP SEND: %s %s %s metric %u - %u %u on %s\n",
						EigrpOpercodeItoa(opcode),
						(external ? "Ext" : "Int"),
						EigrpIpPrintNetwork(&dndb->dest), dndb->metric,
						dndb->vecMetric.bandwidth,
						dndb->vecMetric.delay,
						(iidb ? (S8 *)iidb->idb->name : ""));

	/* Build the item depending on the type. */
	if(external){			/* External route */
		external_route			= (EigrpIpExtern_st *)packet_ptr;
		dest						= external_route->extDest;
		external_route->type		= HTONS(EIGRP_EXTERNAL);
		external_route->length		= HTONS((U16)item_length);	
		external_route->nexthop		= 0;
		if(drdb){			/* It has to be there, but we check */
			EigrpPortMemCpy((void *) & external_route->routerId, (void *) drdb->extData, sizeof(EigrpExtData_st));
		}else{
			EIGRP_ASSERT(0);
			EIGRP_FUNC_LEAVE(EigrpAddItem);
			return(0);
		}

		/* If route is marked as a candidate default, consider performing output filtering on it. */
		if(external_route->flag & EIGRP_INFO_CD){
			if(ddb->pdb->cdOut == FALSE){
				external_route->flag &= ~EIGRP_INFO_CD;
			}
		}
#if 1
		if(external_route->protocol != EIGRP_PROTO_STATIC){/*add_zhenxl_20100908*/
			EigrpVmetric_st	saveMetric;
			saveMetric = dndb->vecMetric;
			dndb->vecMetric = ddb->pdb->vMetricDef;
			pMetric = external_route->delay;
			EigrpMetricEncode(ddb, dndb, iidb, (EigrpVmetricDecode_st *)pMetric, 
								unreachable, FALSE);
			dndb->vecMetric = saveMetric;
		}else{
			EigrpMetricEncode(ddb, dndb, iidb, (EigrpVmetricDecode_st *)&(external_route->delay), 
								unreachable, FALSE);
		}
#else// 1
		*(U32 *)&(external_route->delay)	= 256000; /* tigerwh 100831*/	
#endif// 1
#ifdef INCLUDE_SATELLITE_RESTRICT
		if(peer && peer->rtt >= EIGRP_DEF_SATELLITE_RTT){
			*(U32 *)&(external_route->delay) = EIGRP_METRIC_INACCESS;
		}
#endif//INCLUDE_SATELLITE_RESTRICT
	}else{				/* Internal route */
		/* If route is marked as a candidate default, consider performing output filtering on
		  * it. */
		internal_route	= packet_ptr;
		dest			= internal_route->intDest;
		if(drdb){
			flags		= drdb->opaqueFlag;
			origin	= drdb->origin;
		}else{
			flags		= 0;
			origin	= EIGRP_ORG_EIGRP;
		}
		if(flags & EIGRP_INFO_CD){
			if(ddb->pdb->cdOut  == FALSE){
				flags &= ~EIGRP_INFO_CD;
			}
		}
		internal_route->type		= HTONS(EIGRP_METRIC);
		internal_route->length		= HTONS((U16)item_length);	
		internal_route->nexthop	= 0;
		pMetric = internal_route->delay;
		EigrpMetricEncode(ddb, dndb, iidb, (EigrpVmetricDecode_st *)pMetric,
								unreachable, (S32)(origin == EIGRP_ORG_CONNECTED));
#ifdef INCLUDE_SATELLITE_RESTRICT
		if(peer && peer->rtt >= EIGRP_DEF_SATELLITE_RTT){
			*(U32 *)&(internal_route->delay) = EIGRP_METRIC_INACCESS;
		}
#endif//INCLUDE_SATELLITE_RESTRICT
		EIGRP_METRIC_TYPE_FLAGS(pMetric) = flags;
	}

	dest->mask = EigrpIpBitsInMask(dndb->dest.mask);
	
	nboAddr	= HTONL(dndb->dest.address);		
	EigrpPortMemCpy((void *) dest->number, (U8 *) &nboAddr, dest_size - 1);
	EIGRP_FUNC_LEAVE(EigrpAddItem);
	
	return(item_length);
}

/************************************************************************************

	Name:	EigrpBuildPacket

	Desc:	This function is to build a packet, given a pktDesc.
			Returns a pointer to the packet header, or NULL.
 
 			This routine is called by the transport when it comes across a packet
			that was enqueued earlier by EigrpEnqueuePak.  It is now time to transmit
			that packet, so we are being called to build the packet with up-to-date
			data.
			
	Para: 	ddb		- pointer to dual descriptor block 
			peer		- unused
			pktDesc	- pointer to the structure which contain the information of packet
			suppressed_packet		- unused
			
	Ret:		pointer to the new packet header
************************************************************************************/

EigrpPktHdr_st *EigrpBuildPacket(EigrpDualDdb_st *ddb, EigrpDualPeer_st *peer,
										EigrpPackDesc_st *pktDesc,      S32 *suppressed_packet)
{
	EigrpProbe_st	*probe;
	EigrpPktHdr_st	*eigrph;
	U32				probesize;

	EIGRP_FUNC_ENTER(EigrpBuildPacket);
	probesize  = (U32)sizeof(EigrpProbe_st);

	/* Select based on opcode. */
	switch(pktDesc->opcode){
		case EIGRP_OPC_PROBE:
			eigrph = EigrpAllocPktbuff(probesize);
			if(eigrph){
				probe = (EigrpProbe_st *)EigrpPacketData(eigrph);
				probe->sequence = HTONL(pktDesc->serNoStart);
			}
			pktDesc->length = probesize;
			break;
			
		default:
			eigrph = NULL;
		break;
	}
	EIGRP_FUNC_LEAVE(EigrpBuildPacket);
	
	return(eigrph);
}
