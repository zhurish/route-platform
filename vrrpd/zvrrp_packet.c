/*******************************************************************************
  Copyright (C), 2001-2004, CETC7.
  �ļ���:  vrrpd.c
  ����:     (W0039)
  �汾:    1.0.0          
  ����:    2005-08-30
  ����:    VRRPģ���ʵ���ļ�
  �����б�:  
    1. -------
  �޸���ʷ:         
    1.����:
      ����:
      �汾:
      �޸�:
    2.-------
*******************************************************************************/
#include "zvrrpd.h"
#include "zvrrp_if.h"
#include "zvrrp_packet.h"
#include "zvrrp_sched.h"

#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
#include "net/ethernet.h"
#include "linux/if_packet.h"
#include "netinet/if_ether.h"
#include "netinet/ip_icmp.h"

extern struct zebra_privs_t vrrp_privs;
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)

/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_send_pkt( vrrp_rt *vsrv, char *buffer, int buflen );
static int zvrrp_free_arp_check(const char * m, int vsrvidx);
static int zvrrp_arp_response(const char * m, int vsrvidx);
static int zvrrp_arp_request(const char * m);
static int zvrrp_arppkt_handle(const char *ethbuf, int ethlen);
static int zvrrp_icmppkt_handle(const char *ethbuf, int ethlen);
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_sending_pkt( vrrp_rt *vsrv, char *buffer, int buflen);
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_vip_debug(const char *ip, int num)
{
	int i = 0;
	struct in_addr ip_src;
	long *ipaddress = (long *)ip;
	for(i = 0; i < num; i++)
	{
		ip_src.s_addr = *ipaddress;
		ZVRRP_DEBUG("  virtual ip %d  %s\n",i,inet_ntoa (ip_src));
	}
	return OK;
}
static int zvrrp_vip_auto_debug(int type, const char *text, int size)
{
	char authentification[VRRP_AUTH_LEN];
	if(type == VRRP_AUTH_NONE)
		return OK;
	if(type == VRRP_AUTH_PASS)
	{
		memcpy(authentification, text, VRRP_MIN(VRRP_AUTH_LEN,size));
		ZVRRP_DEBUG("  authentification password sample text:%s\n",authentification);
	}
	return OK;
}
static int zvrrp_packet_debug(const char *packet, int size)
{
	int  type,ver,offset;
	vrrp_pkt *pkt = (vrrp_pkt *)packet;
	ver = (pkt->vers_type)>>4;
	type = (pkt->vers_type)&0x0f;
	ZVRRP_DEBUG(" version %d type %d\n",ver,type);
	ZVRRP_DEBUG("  virtual router id %d\n",pkt->vrid);
	ZVRRP_DEBUG("  priority %d\n",pkt->priority);
	ZVRRP_DEBUG("  address counter %d\n",pkt->naddr);
	ZVRRP_DEBUG("  authentification type %s\n",(pkt->auth_type==VRRP_AUTH_NONE)? "NO Authentication":"Reserved");
	ZVRRP_DEBUG("  advertissement interval %d\n",pkt->adver_int);
	ZVRRP_DEBUG("  checksum 0x%x\n",ntohs(pkt->chksum));
	if(pkt->naddr > 0)
		zvrrp_vip_debug((const char *)(packet + sizeof(vrrp_pkt)), pkt->naddr);
	offset = sizeof(vrrp_pkt) + pkt->naddr * 4;
	zvrrp_vip_auto_debug(pkt->auth_type, packet + offset, size - offset);
	return OK;
}
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************
  ����: vrrp_vsrv_find_by_pkt
  ����: ���ҵ�ǰ���ĵĽ��ձ�����
  ����: iph: ��ǰ�յ���VRRP����
  ���: ��
  ����: ���ʵĽ��ձ����飬�Ҳ���ʱ����NULL
  ����: 
*******************************************************************************/
vrrp_rt * zvrrp_vsrv_find_by_pkt_id(struct ip * iph)
{
    int ihl = 0;
    vrrp_rt * vsrv = NULL;    
    vrrp_pkt * hd = NULL;
    //struct interface * vif = NULL;
    //long ipaddr = 0;
    //ipaddr = ntohl(iph->ip_src.s_addr);
    if( (gVrrpMatser == NULL) )
    	return NULL;
    
    CHECK_VALID(iph, NULL);
    ihl = iph->ip_hl << 2;
    hd  = (vrrp_pkt *)((char *)iph + ihl);

    vsrv = zvrrp_vsrv_lookup(hd->vrid);
    if(vsrv == NULL)
    	return NULL;
    if( (vsrv->used == 1)&&(vsrv->vif.address) )
    {
    	//����IP����ͷԴIP��ַ���ұ�����
    	struct prefix src;
    	src.family = AF_INET;
    	src.prefixlen = IPV4_MAX_PREFIXLEN;
    	src.prefixlen = prefix_blen (vsrv->vif.address);
    	src.u.prefix4.s_addr = iph->ip_src.s_addr;
    	
    	if(prefix_match (&src, vsrv->vif.address))
    		return vsrv;
    	
    	//if( (ipaddr & vsrv->vif.netmask)==(vsrv->vif.ipaddr & vsrv->vif.netmask) )
    	//	return vsrv;
    	//vsrv->vif.ipaddr = ipaddr;      /* the master address of the interface */
    	//vsrv->vif.netmask;      /* the primary address of the interface */
    	//vsrv->vif.ifindex;      /* �ӿ���������ӦMIB����rfc2233:ifindex */
    	//���ݽӿ����� ifindex�ж�IP����ͷԴIP��ַ�Ƿ���ڱ�����Ľӿڵ�IP��ַ��һ������
    	return NULL;
    }
    else
    	return NULL;
}
vrrp_rt * zvrrp_vsrv_find_by_pkt_ip(struct ip * iph)
{
    vrrp_rt *pVsrv = NULL;
    int      i;
    /* ����ƥ��ı����� */
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
        pVsrv = zvrrp_vsrv_lookup(i);

        if ( (pVsrv)&&(pVsrv->used == 1)&&(pVsrv->vif.address) )
        {
        	if(vrrp_mach_ipaddr(pVsrv, htonl(iph->ip_dst.s_addr))==OK)
        		return pVsrv;

        }
    } 
    return NULL;
}
/*******************************************************************************/
int zvrrp_local_myself_detection(const long srcip)
{
    vrrp_rt *pVsrv = NULL;
    int      i;
    /* ����ƥ��ı����� */
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
        pVsrv = zvrrp_vsrv_lookup(i);

        if ( (pVsrv)&&(pVsrv->used == 1)&&(pVsrv->vif.address) )
        {
        	if(srcip == ntohl(pVsrv->vif.address->u.prefix4.s_addr))
        		return OK;
        	//if(srcip == pVsrv->vif.ipaddr)
        	//	return OK;
        }
    } 
    return ERROR;
}
/*******************************************************************************/
/*******************************************************************************/
/****************************************************************
 NAME    : vrrp_in_csum                00/05/10 20:12:20
 AIM    : compute a IP checksum
 REMARK    : from kuznet's iputils
****************************************************************/
static u_short zvrrp_in_csum( u_short *addr, int len, u_short csum)
{
    register int nleft = len;
    const u_short *w = addr;
    register u_short answer;
    register int sum = csum;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1)
        sum += htons(*(u_char *)w << 8);

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff);    /* add hi 16 to low 16 */
    sum += (sum >> 16);            /* add carry */
    answer = ~sum;                /* truncate to 16 bits */
    return (answer);
}

/****************************************************************
 NAME    : vrrp_hd_len                00/02/02 15:16:23
 AIM    : return the vrrp header size in byte
 REMARK    :
****************************************************************/
static int zvrrp_hd_len( vrrp_pkt *pkt )
{
    return sizeof( vrrp_pkt ) + pkt->naddr*sizeof(uint32_t)
                    + VRRP_AUTH_LEN;
}

/****************************************************************
 NAME    : vrrp_hd_len2                00/02/02 15:16:23
 AIM    : return the vrrp header size in byte
 REMARK    :
****************************************************************/
static int zvrrp_hd_len2( vrrp_rt *rt )
{
    return sizeof( vrrp_pkt ) + rt->naddr*sizeof(uint32_t)
                    + VRRP_AUTH_LEN;
}

/****************************************************************
 NAME    : vrrp_dlthd_len            00/02/02 15:16:23
 AIM    : return the vrrp header size in byte
 REMARK    :
****************************************************************/
static int zvrrp_dlt_len( vrrp_rt *rt )
{
    return sizeof(struct ether_header);    /* hardcoded for ethernet */
}

/****************************************************************
 NAME    : vrrp_iphdr_len            00/02/02 15:16:23
 AIM    : return the ip  header size in byte
 REMARK    :
****************************************************************/
static int zvrrp_iphdr_len( vrrp_rt *vsrv )
{
    return sizeof( struct ip );
}
/***********************************************************************/
static int zvrrp_ethhdr_chk( const char *buf )
{
	struct	ether_header *eth = (struct	ether_header *)buf;
	//struct ip *iph = (struct ip *)(buf + sizeof(struct	ether_header));
	//u_char	ether_dhost[ETHER_ADDR_LEN];
	//u_char	ether_shost[ETHER_ADDR_LEN];
    return ntohs(eth->ether_type);
}
static int zvrrp_iphdr_get( struct ip *iph )
{
	if(zvrrp_local_myself_detection(ntohl(iph->ip_src.s_addr))==OK)
		return ERROR;		
    return iph->ip_p;
}
/***********************************************************************/
static int zvrrp_iphdr_chk( struct ip *iph )
{
	//if(iph->ip_v != 4)	
	//iph->ip_hl;      
	//iph->ip_tos;
	//iph->ip_len;
	//iph->ip_id;
	//iph->ip_off;
	//iph->ip_ttl;	
	if(iph->ip_ttl != VRRP_IP_TTL)
	{
		return 1;
	}
	if(iph->ip_p != IPPROTO_VRRP)
	{
		ZVRRP_DEBUG_LOG("invalid protocol. %d and expect %d"
		                        , (iph->ip_p), IPPROTO_VRRP);
		return 1;
	}
	if(iph->ip_dst.s_addr != htonl(INADDR_VRRP_GROUP))
	{
		ZVRRP_DEBUG_LOG("invalid dest ip. 0x%x and expect 0x%x"
				                        , ntohl(iph->ip_dst.s_addr), INADDR_VRRP_GROUP);
		return 1;
	}
	//iph->ip_sum;
	//printf("\n");
    return 0;
}
/****************************************************************
 NAME    : vrrp_in_chk                00/02/02 12:54:54
 AIM    : check a incoming packet. return 0 if the pkt is valid, != 0 else
 REMARK    : rfc2338.7.1
****************************************************************/
static int zvrrp_in_chk( struct ip *iph )
{
    int        ihl = iph->ip_hl << 2;
    vrrp_pkt * hd  = (vrrp_pkt *)((char *)iph + ihl);
    
    /* MUST verify the VRRP version */
    if( (hd->vers_type >> 4) != VRRP_VERSION )
    {
    	ZVRRP_DEBUG_LOG("invalid version. %d and expect %d"
                        , (hd->vers_type >> 4), VRRP_VERSION);
        //gVrrp_VersionErrors++;
        return 1;
    }
    /* WORK: MUST verify the VRRP checksum */
    if( zvrrp_in_csum( (u_short*)hd, zvrrp_hd_len(hd), 0) )
    {
    	ZVRRP_DEBUG_LOG("Invalid vrrp checksum");
        //gVrrp_ChecksumErrors++;
        return 1;
    }
    
    /* MUST verify that the VRID is valid on the receiving interface */
    /* �ں��������н�һ����� */
    if (0 == hd->vrid)
    {
    	ZVRRP_DEBUG_LOG("invalid vrid %d.", hd->vrid);
        //gVrrp_VrIdErrors++;
        return 1;
    }

    return 0;
}

/****************************************************************
 NAME    : vrrp_in_chk2                00/02/02 12:54:54
 AIM    : check a incoming packet. return 0 if the pkt is valid, != 0 else
 REMARK    : rfc2338.7.1
****************************************************************/
static int zvrrp_in_chk2( vrrp_rt *vsrv, struct ip *iph )
{
    int        ihl = iph->ip_hl << 2;
    vrrp_pkt * hd  = (vrrp_pkt *)((char *)iph + ihl);
    
    /* MUST verify that the IP TTL is 255 */
    if( iph->ip_ttl != VRRP_IP_TTL ) 
    {
    	ZVRRP_DEBUG_LOG("invalid ttl. %d and expect %d", iph->ip_ttl, VRRP_IP_TTL);
        vsrv->staIpTtlErrors++;
        return 1;
    }
    
    /* MUST verify that the received packet length is greater than or
    ** equal to the VRRP header */
    /* iph->ip_len��¼��IP���ĵĳ�����IP��ʱ�ѱ���ȥIP�ײ��ĳ��� */
    if( ntohs(iph->ip_len) <= sizeof(vrrp_pkt) )
    {
    	ZVRRP_DEBUG_LOG("ip payload too short. %d and expect at least %d"
                        , ntohs(iph->ip_len), sizeof(vrrp_pkt));
        vsrv->staPktsLenErrors++;
        return 1;
    }
    
    /* MUST verify the VRRP type */
    if( (hd->vers_type & 0x0F) != VRRP_PKT_ADVERT )
    {
    	ZVRRP_DEBUG_LOG("invalid vrrp type. %d and expect %d"
                        , (hd->vers_type & 0x0F), VRRP_PKT_ADVERT);
        vsrv->staInvTypePktsRcvd++;
        return 1;
    }
    
    /* Auth Type must be 0 */
    if( 0 != hd->auth_type )
    {        
    	ZVRRP_DEBUG_LOG("receive a %d auth, expecting 0!", hd->auth_type);
        if (VRRP_IS_BAD_AUTH_TYPE(hd->auth_type))
        {
            vsrv->staInvAuthType++;
            //gVrrp_TrapAuthErrType = EvrrpTrapAuthErrorType_invalidAuthType;
        }
        else
        {
            vsrv->staAuthTypeMismatch++; 
            //gVrrp_TrapAuthErrType = EvrrpTrapAuthErrorType_authTypeMismatch;
        }
        /* ����TRAP */
        //Vrrp_TrapAuthFailureSend();
        return 1;
    }
    
    /* MUST verify that the VRID is valid on the receiving interface */
    if( vsrv->vrid != hd->vrid || VRRP_PRIO_OWNER == vsrv->priority )
    {
    	ZVRRP_DEBUG_LOG("receive VRID %d priority %d", hd->vrid,vsrv->priority );
        return 1;
    }

    /* MAY verify that the IP address(es) associated with the VRID are
    ** valid */
    /* WORK: currently we don't */

    /* MUST verify that the Adver Interval in the packet is the same as
    ** the locally configured for this virtual router */
    if( vsrv->adver_int/VRRP_TIMER_HZ != hd->adver_int )
    {
    	ZVRRP_DEBUG_LOG("advertissement interval mismatch mine=%d rcved=%d"
                        , vsrv->adver_int/VRRP_TIMER_HZ, hd->adver_int);
        vsrv->staAdverIntErrors++;
        return 1;
    }

    return 0;
}
/****************************************************************
 NAME    : vrrp_build_dlt            00/02/02 14:39:18
 AIM    :
 REMARK    : rfc2338.7.3
****************************************************************/
static void zvrrp_build_dlt( vrrp_rt *vsrv, char *buffer, int buflen )
{
    /* hardcoded for ethernet */
    struct ether_header *    eth = (struct ether_header *)buffer;
    /* destination address --rfc1122.6.4*/
    eth->ether_dhost[0]    = 0x01;
    eth->ether_dhost[1]    = 0x00;
    eth->ether_dhost[2]    = 0x5E;
    eth->ether_dhost[3]    = (INADDR_VRRP_GROUP >> 16) & 0x7F;
    eth->ether_dhost[4]    = (INADDR_VRRP_GROUP >>  8) & 0xFF;
    eth->ether_dhost[5]    =  INADDR_VRRP_GROUP        & 0xFF;
    /* source address --rfc2338.7.3 */
    memcpy( eth->ether_shost, vsrv->vhwaddr, sizeof(vsrv->vhwaddr));
    /* type */
    eth->ether_type        = htons( ETHERTYPE_IP );
}

/****************************************************************
 NAME    : vrrp_build_ip                00/02/02 14:39:18
 AIM    : build a ip packet
 REMARK    :
****************************************************************/
static void zvrrp_build_ip( vrrp_rt *vsrv, char *buffer, int buflen )
{
    struct ip * iph = (struct ip *)(buffer);
    iph->ip_hl      = 5;
    iph->ip_v       = 4;
    iph->ip_tos     = 0;
    iph->ip_len     = iph->ip_hl*4 + zvrrp_hd_len2( vsrv );
    iph->ip_len     = htons(iph->ip_len);
    iph->ip_id      = 0;//zhurish htons(gbsdip_id++);
    iph->ip_off     = 0;
    iph->ip_ttl     = VRRP_IP_TTL;
    iph->ip_p       = IPPROTO_VRRP;
    //iph->ip_src.s_addr = htonl(vsrv->vif.ipaddr);
    iph->ip_src.s_addr = (vsrv->vif.address->u.prefix4.s_addr);    
    iph->ip_dst.s_addr = htonl(INADDR_VRRP_GROUP);
    /* checksum must be done last */
    iph->ip_sum = 0;
    iph->ip_sum = zvrrp_in_csum( (u_short*)iph, iph->ip_hl*4, 0 );
}

/****************************************************************
 NAME    : vrrp_build_vrrp            00/02/02 14:39:18
 AIM    :
 REMARK    :
****************************************************************/
static int zvrrp_build_vrrp( vrrp_rt *vsrv, int prio, char *buffer, int buflen )
{
    int    i;
    vrrp_pkt *hd    = (vrrp_pkt *)buffer;
    uint32_t *iparr    = (uint32_t *)((char *)hd+sizeof(*hd));
    
    hd->vers_type    = (VRRP_VERSION<<4) | VRRP_PKT_ADVERT;
    hd->vrid    = vsrv->vrid;
    hd->priority    = prio;
    hd->naddr    = vsrv->naddr;
    hd->auth_type    = 0;
    hd->adver_int    = vsrv->adver_int/VRRP_TIMER_HZ;
    /* copy the ip addresses */
    for( i = 0; i < vsrv->naddr; i++ ){
        iparr[i] = htonl(vsrv->vaddr[i].addr);
    }
    hd->chksum    = 0;
    hd->chksum    = zvrrp_in_csum( (u_short*)hd, zvrrp_hd_len2(vsrv), 0);
    return(0);
}
/****************************************************************
 NAME    : vrrp_build_pkt                00/02/02 13:33:32
 AIM    : build a advertissement packet
 REMARK    :
****************************************************************/
static void zvrrp_build_pkt( vrrp_rt *vsrv, int prio, char *buffer, int buflen )
{
    zvrrp_build_dlt( vsrv, buffer, buflen );
    buffer += zvrrp_dlt_len(vsrv);
    buflen -= zvrrp_dlt_len(vsrv);
    /* build the ip header */
    zvrrp_build_ip( vsrv, buffer, buflen );
    buffer += zvrrp_iphdr_len(vsrv);
    buflen -= zvrrp_iphdr_len(vsrv);
    /* build the vrrp header */
    zvrrp_build_vrrp( vsrv, prio, buffer, buflen );
}
/****************************************************************
 NAME    : vrrp_send_adv                00/02/06 16:31:24
 AIM    :
 REMARK    :
****************************************************************/
int zvrrp_send_adv( vrrp_rt *vsrv, int prio )
{
    int    buflen, ret;
    char *    buffer;
    if(!vsrv || !vsrv->vif.address)
    	return ERROR;
    /* alloc the memory */
    buflen = zvrrp_dlt_len(vsrv) + zvrrp_iphdr_len(vsrv) + zvrrp_hd_len2(vsrv);
    buffer = malloc( buflen + 4 );
    //buffer = memalign( 4, buflen + 4 );
    assert( buffer );
    /* build the packet  */
    zvrrp_build_pkt( vsrv, prio, buffer + 2, buflen );
    /* send it */
    ret = zvrrp_send_pkt( vsrv, buffer + 2, buflen );
    /* build the memory */
    //zvrrp_packet_debug(buffer + 2 + 14 + 20, buflen - 14 - 20);
    free( buffer );
    return ret;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_pkt_handle(struct ip *buff, int len)
{  
    vrrp_rt * pVsrv = NULL;

    if(len > 0 && zvrrp_iphdr_chk( (struct ip *)buff))
    	len = 0;
        
    if( len > 0 && zvrrp_in_chk( (struct ip *)buff ) )
    {
    	ZVRRP_DEBUG_LOG("bogus packet!");
    	len = 0;
    }
    if (len <= 0)
    {
    	return ERROR;
    }

    /* ȷ�����Ľ����� */
    pVsrv = zvrrp_vsrv_find_by_pkt_id((struct ip *)buff);
    if (!pVsrv || pVsrv->adminState != zvrrpOperAdminState_up )
    {
    	//ZVRRP_DEBUG_LOG("adminState is not up\n");
    	return ERROR;
    }
    pVsrv->staAdverRcvd++;

    /* ������鱨�ĵĺϷ��� */
    if (zvrrp_in_chk2(pVsrv, (struct ip *)buff))
    {
    	return ERROR;
    }
    return zvrrp_handle_on_state(pVsrv, (const char *)buff);	
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_ippkt_handle(const char *ethbuf, int ethlen)
{  
	int ret = -1;
    int pkt_type = 0;
    struct ip *iph = (struct ip *)(ethbuf + MAC_ETH_LEN);
	pkt_type = zvrrp_iphdr_get(iph);
	if(pkt_type == ERROR)
		return ERROR;

	switch(pkt_type)
	{
	case IPPROTO_ICMP:/* control message protocol */
		zvrrp_icmppkt_handle(ethbuf, ethlen);
		break;
	case IPPROTO_IP:/* dummy for IP */
	//case IPPROTO_HOPOPTS==IPPROTO_IP:/* IP6 hop-by-hop options */
	case IPPROTO_IGMP:/* group mgmt protocol */
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	case IPPROTO_IPV4:/* IPv4 encapsulation */
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	//case IPPROTO_IPIP==IPPROTO_IPV4	/* for compatibility */
	case IPPROTO_TCP:/* tcp */
	case IPPROTO_UDP:/* user datagram protocol */
	case IPPROTO_IPV6:/* IP6 header */
	case IPPROTO_RSVP:/* resource reservation */
	case IPPROTO_GRE:/* General Routing Encap. */
	case IPPROTO_ESP:/* IP6 Encap Sec. Payload */
	case IPPROTO_AH:/* IP6 Auth Header */
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	case IPPROTO_MOBILE:/* IP Mobility */
	case IPPROTO_ICMPV6:/* ICMP6 */
	case IPPROTO_OSPFIGP:/* OSPFIGP */
	case IPPROTO_PIM:/* Protocol Independent Mcast */
	case IPPROTO_L2TP:/* L2TP   */    
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	case IPPROTO_MH:/* IPv6 Mobility Header */
		break;
	case IPPROTO_VRRP:
		ret = zvrrp_pkt_handle(iph, ethlen - MAC_ETH_LEN);
		break;
	default:
		break;
	}
	return ret;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_ethpkt_handle(const char *ethbuf, int ethlen)
{  
	int ret = -1;
    int pkt_type = 0;
   
    pkt_type = zvrrp_ethhdr_chk(ethbuf);   
    switch(pkt_type)
    {
    case ETHERTYPE_PUP:/* PUP protocol */
    	break;
    case ETHERTYPE_IP:/* IP protocol */
    	ret = zvrrp_ippkt_handle(ethbuf, ethlen);
    	break;
    case ETHERTYPE_ARP:/* Addr. resolution protocol */
    	zvrrp_arppkt_handle(ethbuf, ethlen);
    	break;
    case ETHERTYPE_REVARP:/* reverse Addr. resolution protocol */
    	break;
    case ETHERTYPE_VLAN:/* IEEE 802.1Q VLAN tagging */
    	break;
    case ETHERTYPE_IPV6:/* IPv6 */
    	break;
    case ETHERTYPE_LOOPBACK:/* used to test interfaces */
    	break;
    default:
    	break;
    }  
    return ret;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_arppkt_handle(const char *ethbuf, int ethlen)
{
	int ret = 0;
	int vsrvidx = 0;
	ret = zvrrp_arp_request((char *)(ethbuf + MAC_ETH_LEN));
	if(ret != ERROR)//master�ڵ���Ӧarp����
	{
		vsrvidx = ret;
		ret = zvrrp_free_arp_check((char *)(ethbuf + MAC_ETH_LEN), vsrvidx);
		if(!ret)
			zvrrp_arp_response((char *)(ethbuf + MAC_ETH_LEN), vsrvidx);
	}
	return OK;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_icmppkt_handle(const char *ethbuf, int ethlen)
{
    int iplen;
    vrrp_rt *vsrv = NULL;
    struct icmp *icmphdr;
    struct ip *iph = (struct ip *)(ethbuf + MAC_ETH_LEN);
    iplen = (iph->ip_hl<<2);
    icmphdr = (struct icmp *)(ethbuf + MAC_ETH_LEN + iplen);
	
	if(icmphdr->icmp_type == ICMP_ECHO)
	{
		//if(icmphdr->icmp_code == 0)
	    /* ȷ�����Ľ����� */
	    vsrv = zvrrp_vsrv_find_by_pkt_ip(iph);
	    //master�ڵ���ӦICMP����
	    if( (!vsrv)||(vsrv->state != zvrrpOperState_master) )
	    	return OK;
	    if (vsrv->adminState != zvrrpOperAdminState_up)
	    {
	    	//ZVRRP_DEBUG_LOG("%s:adminState is not up\n",__func__);
	    	return ERROR;
	    }
		printf("ICMP:request :%s\n",inet_ntoa(iph->ip_dst));
		//printf_msg_hex((unsigned char *)ethbuf, ethlen);
#if (ZVRRPD_OS_TYPE	== ZVRRPD_ON_VXWORKS)
		bswap((char *)ethbuf, (char *)(ethbuf + 6), 6);//����mac��ַ
		bswap((char *)&iph->ip_dst.s_addr, (char *)&iph->ip_src.s_addr, 4);//����ip��ַ
#endif
		iph->ip_sum = 0;
		iph->ip_sum = zvrrp_in_csum( (u_short*)iph, iplen, 0 );//У��IP
	    
	    icmphdr->icmp_type = ICMP_ECHOREPLY;
	    icmphdr->icmp_code = 0;
	    icmphdr->icmp_cksum = 0;
	    icmphdr->icmp_cksum = zvrrp_in_csum( (u_short*)icmphdr, ethlen - MAC_ETH_LEN - iplen, 0 );

	    return zvrrp_send_pkt(vsrv, (char *)ethbuf, ethlen);
	}
	return ERROR;
}
/*******************************************************************************/
/****************************************************************
 NAME    : vrrp_send_pkt                00/02/06 16:37:10
 AIM    :
 REMARK    :������̫��֡
****************************************************************/
static int zvrrp_send_pkt( vrrp_rt *vsrv, char *buffer, int buflen )
{
	return zvrrp_sending_pkt(vsrv, buffer, buflen);
}
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/****************************************************************
 NAME    : send_gratuitous_arp            00/05/11 11:56:30
 AIM    :
 REMARK    : rfc0826
    : WORK: ugly because heavily hardcoded for ethernet
****************************************************************/
int send_gratuitous_arp( vrrp_rt *vsrv, int addr, int vAddrF )
{
    char   buf[sizeof(struct ether_arp) + MAC_ETH_LEN ];
    char   buflen = sizeof(struct ether_arp) + MAC_ETH_LEN;
    struct ether_header *eth = (struct ether_header *)buf;
    struct ether_arp *arph;
    char   *hwaddr;
    int    hwlen = 6;
    
    if( (!vsrv)||(vsrv->vif.address == NULL))
    	return ERROR;
    
    arph = (struct ether_arp *)(buf+zvrrp_dlt_len(vsrv));
    hwaddr = vAddrF ? vsrv->vhwaddr : vsrv->vif.hwaddr;


    /* hardcoded for ethernet */
    memset( eth->ether_dhost, 0xFF, hwlen );
    memcpy( eth->ether_shost, hwaddr, hwlen );
    eth->ether_type = htons(ETHERTYPE_ARP);

    /* build the arp payload */
    bzero( (char *)arph, sizeof( *arph ) );
    arph->arp_hrd    = htons(ARPHRD_ETHER);
    arph->arp_pro    = htons(ETHERTYPE_IP);
    arph->arp_hln    = 6;
    arph->arp_pln    = 4;
    arph->arp_op     = htons(ARPOP_REQUEST);
    memcpy( arph->arp_sha, hwaddr, hwlen );
    memcpy( arph->arp_spa, &addr, sizeof(addr) );
    memcpy( arph->arp_tpa, &addr, sizeof(addr) );
    return zvrrp_send_pkt( vsrv, buf, buflen );
}

/*******************************************************************************
  ����: zvrrp_free_arp_check
  ����: ���ARP�����Ƿ�Ϊ����IP���ARP����
  ����: m: ARP������
        vsrvidx: ����������+1
  ���: ��
  ����: �Ƿ���1�����Ƿ���0
  ����: 
*******************************************************************************/
static int zvrrp_free_arp_check(const char * m, int vsrvidx)
{
    vrrp_rt * vsrv;
    struct ether_arp *ea;
    vsrv = zvrrp_vsrv_lookup(vsrvidx);
    if( (!vsrv)||(vsrv->vif.address == NULL))
    	return ERROR;
    ea = (struct ether_arp *)m;
    if (!memcmp(ea->arp_spa, ea->arp_tpa, 4) && !memcmp(ea->arp_sha, vsrv->vhwaddr, 6))
    {
        return 1;
    }
    return 0;
}
/*******************************************************************************
  ����: zvrrp_arp_response
  ����: ʹ������MAC��ӦARP����
  ����: m: ARP������
        vsrvidx: ����������+1
  ���: ��
  ����: ��
  ����: 
*******************************************************************************/
static int zvrrp_arp_response(const char * m, int vsrvidx)
{   
    char   buf[sizeof(struct ether_arp)+MAC_ETH_LEN];
    char   buflen = sizeof(struct ether_arp)+MAC_ETH_LEN;
    vrrp_rt * vsrv;
    struct ether_header *eth;
    struct ether_arp *arph, *ea;
    char   *hwaddr;
    int    hwlen = 6;
    vsrv = zvrrp_vsrv_lookup(vsrvidx);
    if( (!vsrv)||(vsrv->vif.address == NULL))
    	return ERROR;
    eth = (struct ether_header *)buf;
    arph = (struct ether_arp *)(buf+zvrrp_dlt_len(vsrv));
    hwaddr = vsrv->vhwaddr;

    ea = (struct ether_arp *)m;
    
    printf(" arp request mac(net_virtual_arpx) for %d.%d.%d.%d \n",ea->arp_tpa[0],ea->arp_tpa[1],ea->arp_tpa[2],ea->arp_tpa[3]);
    
    /* hardcoded for ethernet */
    memcpy( eth->ether_dhost, ea->arp_sha, hwlen );
    memcpy( eth->ether_shost, hwaddr, hwlen );
    eth->ether_type = htons(ETHERTYPE_ARP);

    /* build the arp payload */
    bzero( (char *)arph, sizeof( *arph ) );
    arph->arp_hrd    = htons(ARPHRD_ETHER);
    arph->arp_pro    = htons(ETHERTYPE_IP);
    arph->arp_hln    = 6;
    arph->arp_pln    = 4;
    arph->arp_op     = htons(ARPOP_REPLY);
    memcpy( arph->arp_sha, hwaddr, hwlen );
    memcpy( arph->arp_tha, ea->arp_sha, hwlen);
    memcpy( arph->arp_spa, ea->arp_tpa, sizeof(ea->arp_tpa) );
    memcpy( arph->arp_tpa, ea->arp_spa, sizeof(ea->arp_spa) );
    return zvrrp_send_pkt( vsrv, buf, buflen );
}
/*******************************************************************************/
static int zvrrp_arp_request(const char * m)
{
	int i = 0,ret = -1;
	long *ipaddress;
    struct ether_arp *arph;
    vrrp_rt * vsrv = NULL;
    arph = (struct ether_arp *)m;   
    ipaddress = (long *)arph->arp_tpa;
    
	//printf(" arp request mac(net_virtual_arpx) for %d.%d.%d.%d \n",arph->arp_tpa[0],arph->arp_tpa[1],arph->arp_tpa[2],arph->arp_tpa[3]);

    for( i = 0; i < VRRP_VSRV_SIZE_MAX; i++ )
    {
    	vsrv = zvrrp_vsrv_lookup(i);
    	if(vsrv && vsrv->used && vsrv->vif.address)
    	{
    		ret = vrrp_mach_ipaddr(vsrv, ntohl((*ipaddress)));
    		if(ret == OK)
    		{
    		    if(vsrv->state == zvrrpOperState_master) //master�ڵ���Ӧarp����
    		    	return i;
    		}
    	}
    }
    return ERROR;
}
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
int zvrrp_socket_init(int pvoid)
{
	int sock = -1;
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
	char *name;
	vrrp_if * vif;
	struct ifreq ifr;
	struct sockaddr_ll ll;
	if ( vrrp_privs.change (ZPRIVS_RAISE) )
	    zlog_err ("%s: could not raise privs, %s",__func__,safe_strerror (errno) );
	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if (sock < 0)
	{
		if (vrrp_privs.change (ZPRIVS_LOWER))
			zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
		return -1;
	}
	memset(&ifr, 0, sizeof(ifr));
	name = ifindex2ifname(pvoid);
	if(name == NULL)
	{
		if (vrrp_privs.change (ZPRIVS_LOWER))
			zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
		close(sock);
		return -1;
	}
	strcpy(ifr.ifr_name, name);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
	{
		if (vrrp_privs.change (ZPRIVS_LOWER))
			zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
		ZVRRP_DEBUG_LOG( "%s: ioctl[SIOCGIFHWADDR]: %s",__func__, strerror(errno));
		close(sock);
		return -1;
	}
	memset(&ll, 0, sizeof(ll));
	ll.sll_family = PF_PACKET;
	ll.sll_halen = ETH_ALEN;
	ll.sll_ifindex = pvoid;
	ll.sll_protocol = htons(ETH_P_IP);
	memcpy(ll.sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	if (bind(sock, (struct sockaddr *) &ll, sizeof(ll)) < 0)
	{
		if (vrrp_privs.change (ZPRIVS_LOWER))
			zlog_err ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
		ZVRRP_DEBUG_LOG( "%s: bind[PF_PACKET]: %s",__func__, strerror(errno));
		close(sock);
		return -1;
	}

	if (vrrp_privs.change (ZPRIVS_LOWER))
	{
		ZVRRP_DEBUG_LOG ("%s: could not lower privs, %s",__func__,safe_strerror (errno) );
	}
	vif = zvrrp_if_lookup_by_ifindex(pvoid);
	if(vif == NULL)
	{
		ZVRRP_DEBUG_LOG( "%s: bind[PF_PACKET]: %s",__func__, strerror(errno));
		close(sock);
		return -1;
	}
	memcpy(vif->hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#ifdef ZVRRPD_ON_ROUTTING
#ifdef HAVE_STRUCT_SOCKADDR_DL
	if(memcmp(vif->hwaddr, vif->ifp->sdl.sdl_data, ETH_ALEN) != 0)
		memcpy(vif->ifp->sdl.sdl_data, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#else
	if(memcmp(vif->hwaddr, vif->ifp->hw_addr, ETH_ALEN) != 0)
		memcpy(vif->ifp->hw_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
#endif
#endif
	ZVRRP_DEBUG_LOG("%s:%s get mac %02x:%02x:%02x:%02x:%02x:%02x\n",__func__,name,
			vif->hwaddr[0],vif->hwaddr[1],vif->hwaddr[2],
			vif->hwaddr[3],vif->hwaddr[4],vif->hwaddr[5]);
	vif->sock = sock;
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	sock = socket(AF_PACKET, SOCK_RAW, htons(ETHERTYPE_IP));//
	if(sock <= 0)
	{
		ZVRRP_DEBUG_LOG("ERROR PACKET RAW socket create :%s",strerror(errno));
		return ERROR;
	}
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	return sock;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_sending_pkt( vrrp_rt *vsrv, char *buffer, int buflen)
{
	int ret = -1;
	struct sockaddr_ll sll;
    struct ether_header *    eth = (struct ether_header *)buffer;
    
	if( (!gVrrpMatser)||(!vsrv->vif.ifp)||(!vsrv->vif.address) )
	{
		return ERROR;
	}
#ifdef HAVE_STRUCT_SOCKADDR_DL_SDL_LEN
	sll.sll_len	= sizeof(struct sockaddr_ll);        /* Length of this structure */
#endif
	sll.sll_family	= AF_PACKET;     /* Always AF_PACKET */
	sll.sll_protocol = 0;  
	//sll.sll_ifindex	= vsrv->vif.ifindex;    /* Interface index, must not be zero */
	sll.sll_ifindex	= vsrv->vif.ifp->ifindex;
	sll.sll_hatype	= 0;     
	sll.sll_pkttype = 0;  
	sll.sll_halen	= ETH_ALEN;      /* Length of link layer address */
	/*
	ll.sll_family = AF_PACKET;
	ll.sll_ifindex = ifp->ifindex;
	ll.sll_protocol = htons(ETH_P_8021Q|ETH_P_LLDP);
	ll.sll_halen = ETH_ALEN;
	memcpy(ll.sll_addr, lifp->own_mac, ETH_ALEN);
	errno = 0;
	 */
	//memcpy(sll.sll_addr, vsrv->vhwaddr, 6);    /* Link layer address */
	memcpy(sll.sll_addr, eth->ether_dhost, 6);
	//memcpy(sll.sll_addr, eth->ether_shost, 6);
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
	ret = sendto(vsrv->vif.sock, (caddr_t)buffer, buflen, 0, (struct sockaddr *)&sll, sizeof(sll));
#endif
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	ret = sendto(gVrrpMatser->sock, (caddr_t)buffer, buflen, 0, (struct sockaddr *)&sll, sizeof(sll));
#endif
	if(ret < 0)
	{
		ZVRRP_DEBUG_LOG("ERROR PACKET RAW socket send :%s",strerror(errno));
		return ERROR;
	}
	return ret;	
}
/*******************************************************************************/
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
static int zvrrp_reading(vrrp_rt *vsrv)
#endif
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
static int zvrrp_reading(struct zvrrp_master *vrrp)
#endif
{
    int len = 0;
    char ethbuf[2048];	
    /* ��ȡVRRP���� */
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
    len = read(vsrv->vif.sock, ethbuf, sizeof(ethbuf));
#endif
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
    len = read(vrrp->sock, ethbuf, sizeof(ethbuf));
#endif
    if(len < 14)
    	return ERROR;
    //��⵱ǰ��ʲô���ݰ�
    return zvrrp_ethpkt_handle((const char *)ethbuf, len);
}
/*******************************************************************************/
/*******************************************************************************
  ����: zvrrp_reading_thread
  ����: VRRP���Ľ���������ں���
  ����: ��
  ���: ��
  ����: ��
  ����: 
*******************************************************************************/
int zvrrp_reading_thread(void *p)
{
#ifndef ZVRRPD_ON_ROUTTING	
    int cs = 0;
    fd_set readfds;
    struct timeval timer_val;
    timer_val.tv_sec = 0;
    timer_val.tv_usec = 1000;
    struct timeval *timer_wait = &timer_val;
    struct zvrrp_master *vrrp = (struct zvrrp_master *)p;
    
    FD_ZERO(&readfds);
    FD_SET(vrrp->sock, &readfds);
    cs = select(vrrp->sock + 1, &readfds, NULL, NULL, timer_wait);
    if (cs <= 0)
    {
    	return ERROR;
    }
    if(FD_ISSET(vrrp->sock, &readfds))
    	return zvrrp_reading(vrrp);
    return ERROR;
#else//ZVRRPD_ON_ROUTTING  
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
    vrrp_rt *vsrv = THREAD_ARG(((struct thread *)p));

    vsrv->vif.zvrrp_read = thread_add_read(gVrrpMatser->master,
    		zvrrp_reading_thread, vsrv, vsrv->vif.sock);
    return zvrrp_reading(vsrv);
#endif
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
    struct zvrrp_master *vrrp = THREAD_ARG(((struct thread *)p));
    vrrp->zvrrp_read = thread_add_read(vrrp->master, zvrrp_reading_thread, vrrp, vrrp->sock);  
    return zvrrp_reading(vrrp);
#endif
#endif//ZVRRPD_ON_ROUTTING    
}
/*******************************************************************************/
/*******************************************************************************/
int zvrrp_reading_task(void *p)
{
#ifndef ZVRRPD_ON_ROUTTING	
    int cs = 0;
    int len = 0;
    fd_set readfds;
    char ethbuf[2048];
    struct timeval timer_val;
    timer_val.tv_sec = 0;
    timer_val.tv_usec = 10000;
    struct timeval *timer_wait = NULL;//&timer_val;
    struct zvrrp_master *master = (struct zvrrp_master *)p;
    
    //zvrrputilschedadd(master->master, zvrrp_reading_thread, master, 0);
    for (;;)
    {
        /* �ȴ�VRRP���� */
    	//zvrrpsemtake(master->SemMutexId, -1);
        FD_ZERO(&readfds);
        FD_SET(master->sock, &readfds);
        cs = select(master->sock + 1, &readfds, NULL, NULL, timer_wait);
        if (cs <= 0)
        {
        	//zvrrpsemgive(master->SemMutexId);
        	continue;
        }
        /* ��ȡVRRP���� */
        len = read(master->sock, ethbuf, sizeof(ethbuf));
        
        if(len < 14)
        {
        	//zvrrpsemgive(master->SemMutexId);
        	continue;
        }
        //��⵱ǰ��ʲô���ݰ�
        zvrrp_ethpkt_handle((const char *)ethbuf, len);
        
        //zvrrpsemgive(master->SemMutexId);
    }
#else//ZVRRPD_ON_ROUTTING  
#endif//ZVRRPD_ON_ROUTTING    
}
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

