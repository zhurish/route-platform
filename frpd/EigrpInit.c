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
extern	unsigned long debug_eigrp_zebra;
/************************************************************************************/
extern	struct ZebraEigrpMaster *EigrpMaster;
/************************************************************************************/
/************************************************************************************/
ResponseCmdTable_st ResponseCmdTable[EIGRP_CMD_TYPE_MAX] =
{
	{EIGRP_CMD_TYPE_GLOBAL,"eigrp clobal cmd ","no eigrp clobal cmd "},
	{EIGRP_CMD_TYPE_INTF,"interface","no interface"},
#ifdef _EIGRP_PLAT_MODULE
	/* ����Router ID ���� ������ */
	{EIGRP_CMD_TYPE_ROUTER_ID,"router-id X.X.X.X ","no router-id X.X.X.X "},
#endif// _EIGRP_PLAT_MODULE	

	{EIGRP_CMD_TYPE_ROUTER,"router frp <1-65535> ","no router frp <1-65535> "},
	{EIGRP_CMD_TYPE_AUTOSUM,"ip frp autosammary","no ip frp autosammary"},
	{EIGRP_CMD_TYPE_DEF_METRIC,"ip frp default metric","no ip frp default metric"},
	{EIGRP_CMD_TYPE_DISTANCE,"ip frp default destance","no ip frp default destance"},
	{EIGRP_CMD_TYPE_WEIGHT,"ip frp default weight","no ip frp default weight"},
	{EIGRP_CMD_TYPE_NETWORK,"network A.B.C.D A.B.C.D ","no network A.B.C.D A.B.C.D "},
	{EIGRP_CMD_TYPE_REDIS,"redistribute ","no redistribute "},
	{EIGRP_CMD_TYPE_NET_METRIC,"set network A.B.C.D A.B.C.D metric <0-4294967295> <1-4294967295>  <1-1500> <0-255> <1-255> ",
		"no set network A.B.C.D A.B.C.D metric "},
	{EIGRP_CMD_TYPE_DEF_ROUTE_IN,"ip frp default redistribute in","no ip frp default redistribute in"},
	{EIGRP_CMD_TYPE_DEF_ROUTE_OUT,"ip frp default redistribute out","no ip frp default redistribute out"},
	
	{EIGRP_CMD_TYPE_INTF_AUTHKEYID,"ip frp auto key-id","no ip frp auto key-id"},
	{EIGRP_CMD_TYPE_INTF_AUTHKEY,"ip frp auto key","no ip frp auto key"},
	{EIGRP_CMD_TYPE_INTF_AUTHMODE,"ip frp auto mode","no ip frp auto mode"},
	{EIGRP_CMD_TYPE_INTF_BANDWIDTH_PERCENT,"ip frp bandwidth precent","no ip frp bandwidth precent"},
	{EIGRP_CMD_TYPE_INTF_BANDWIDTH,"ip frp bandwidth  ","no ip frp bandwidth"},
	{EIGRP_CMD_TYPE_INTF_DELAY,"ip frp delay","no ip frp delay"},
	
#ifdef _EIGRP_UNNUMBERED_SUPPORT
	{EIGRP_CMD_TYPE_INTF_UUAI_PARAM,"no ip frp uai param","no ip frp uai param"},/*cwf 20121225 for set uuai band and delay*/
#endif /* _EIGRP_UNNUMBERED_SUPPORT */
	{EIGRP_CMD_TYPE_INTF_HELLO,"ip frp hello time interval","no ip frp hello time interval"},
	{EIGRP_CMD_TYPE_INTF_HOLD,"ip frp holdtime","no ip frp holdtime"},
	{EIGRP_CMD_TYPE_INTF_SUM,"ip frp sum","no ip frp sum"},
	{EIGRP_CMD_TYPE_INTF_SUMNO,"ip frp sumno","no ip frp sumno"},
	{EIGRP_CMD_TYPE_INTF_SPLIT,"ip frp split","no ip frp split"},
	{EIGRP_CMD_TYPE_INTF_PASSIVE,"ip frp passive","no ip frp passive"},
	{EIGRP_CMD_TYPE_INTF_INVISIBLE,"ip frp invisible","no ip frp invisible"},
	{EIGRP_CMD_TYPE_INTF_OSIN,"EIGRP_CMD_TYPE_INTF_OSIN","no EIGRP_CMD_TYPE_INTF_OSIN"},
	{EIGRP_CMD_TYPE_INTF_OSOUT,"EIGRP_CMD_TYPE_INTF_OSOUT","no EIGRP_CMD_TYPE_INTF_OSOUT"},
#ifdef _EIGRP_UNNUMBERED_SUPPORT	
	{EIGRP_CMD_TYPE_INTF_UAI_P2MP,"EIGRP_CMD_TYPE_INTF_UAI_P2MP","no EIGRP_CMD_TYPE_INTF_UAI_P2MP"},
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */	
#ifdef _EIGRP_VLAN_SUPPORT
	{EIGRP_CMD_TYPE_INTF_UAI_P2MP_VLAN_ID,"EIGRP_CMD_TYPE_INTF_UAI_P2MP_VLAN_ID","no EIGRP_CMD_TYPE_INTF_UAI_P2MP_VLAN_ID"},
#endif /* _EIGRP_VLAN_SUPPORT */
#ifdef _EIGRP_UNNUMBERED_SUPPORT	
	{EIGRP_CMD_TYPE_INTF_UAI_P2MP_NEI,"EIGRP_CMD_TYPE_INTF_UAI_P2MP_NEI","no EIGRP_CMD_TYPE_INTF_UAI_P2MP_NEI"},
#endif	/* _EIGRP_UNNUMBERED_SUPPORT */
	{EIGRP_CMD_TYPE_NEI,"ip frp neighber","no ip frp neighber"},
	
	{EIGRP_CMD_TYPE_SHOW_INTF,"show ip frp interface ","show ip frp interface "},
	{EIGRP_CMD_TYPE_SHOW_INTF_DETAIL,"show ip frp interface detail ","show ip frp interface detail "},
	{EIGRP_CMD_TYPE_SHOW_INTF_SINGLE,"show ip frp interface single ","show ip frp interface single "},
	{EIGRP_CMD_TYPE_SHOW_INTF_AS,"show ip frp interface as ","show ip frp interface as"},
	{EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE,"show ip frp interface as single ","show ip frp interface as single"},
	{EIGRP_CMD_TYPE_SHOW_INTF_AS_DETAIL,"show ip frp interface as detail ","show ip frp interface as detail "},
	{EIGRP_CMD_TYPE_SHOW_INTF_AS_SINGLE_DETAIL,"show ip frp interface as single detail ","show ip frp interface as single detail "},

	{EIGRP_CMD_TYPE_SHOW_NEI,"show ip frp neighbor ","show ip frp neighbor "},
	{EIGRP_CMD_TYPE_SHOW_NEI_AS,"show ip frp neighbor as","show ip frp neighbor as"},
	{EIGRP_CMD_TYPE_SHOW_NEI_DETAIL,"show ip frp neighbor detail","show ip frp neighbor detail"},
	{EIGRP_CMD_TYPE_SHOW_NEI_INTF,"show ip frp neighbor intrface","show ip frp neighbor intrface"},
	{EIGRP_CMD_TYPE_SHOW_NEI_INTF_DETAIL,"show ip frp neighbor intrface detail","show ip frp neighbor intrface detail"},
	{EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF,"show ip frp neighbor as intrface","show ip frp neighbor as intrface"},
	{EIGRP_CMD_TYPE_SHOW_NEI_AS_INTF_DETAIL,"show ip frp neighbor as intrface detail","show ip frp neighbor as intrface detail"},
	{EIGRP_CMD_TYPE_SHOW_NEI_AS_DETAIL,"show ip frp neighbor as detail","show ip frp neighbor as detail"},
	
	{EIGRP_CMD_TYPE_SHOW_TOPO_ALL,"show ip frp topology all","show ip frp topology all"},
	{EIGRP_CMD_TYPE_SHOW_TOPO_SUM,"show ip frp topology sammary","show ip frp topology sammary"},
	{EIGRP_CMD_TYPE_SHOW_TOPO_ACT,"show ip frp topology act","show ip frp topology act"},
	{EIGRP_CMD_TYPE_SHOW_TOPO_FS,"show ip frp topology fs","show ip frp topology fs"},
	{EIGRP_CMD_TYPE_SHOW_TOPO_AS_ALL,"show ip frp topology as","show ip frp topology as"},
	{EIGRP_CMD_TYPE_SHOW_TOPO_AS_ZERO,"show ip frp topology as zero","show ip frp topology zero"},

	{EIGRP_CMD_TYPE_SHOW_TRAFFIC,"show ip frp traffic","show ip frp traffic"},
	{EIGRP_CMD_TYPE_SHOW_PROTOCOL,"show ip frp protocol","show ip frp protocol"},
	{EIGRP_CMD_TYPE_SHOW_ROUTE,"show ip frp route","show ip frp route"},
	{EIGRP_CMD_TYPE_SHOW_STRUCT,"show ip frp struct","show ip frp struct"},
	{EIGRP_CMD_TYPE_SHOW_DEBUG,"show ip frp debug","show ip frp debug"},

	{EIGRP_CMD_TYPE_DBG_SEND_UPDATE,"ip frp debug send update","ip frp debug send update"},
	{EIGRP_CMD_TYPE_DBG_SEND_QUERY,"ip frp debug send query","ip frp debug send query"},
	{EIGRP_CMD_TYPE_DBG_SEND_REPLY,"ip frp debug send reply","ip frp debug send reply"},
	{EIGRP_CMD_TYPE_DBG_SEND_HELLO,"ip frp debug send hello","ip frp debug send hello"},
	{EIGRP_CMD_TYPE_DBG_SEND_ACK	,"ip frp debug send ack","ip frp debug send ack"},
	{EIGRP_CMD_TYPE_DBG_SEND,"ip frp debug send ","ip frp debug send "},
	
	{EIGRP_CMD_TYPE_DBG_RECV_UPDATE,"ip frp debug recv update","no ip frp debug recv update"},
	{EIGRP_CMD_TYPE_DBG_RECV_QUERY,"ip frp debug recv query","no ip frp debug recv query"},
	{EIGRP_CMD_TYPE_DBG_RECV_REPLY,"ip frp debug recv reply","ip frp debug recv reply"},
	{EIGRP_CMD_TYPE_DBG_RECV_HELLO,"ip frp debug recv hello","ip frp debug recv hello"},
	{EIGRP_CMD_TYPE_DBG_RECV_ACK,"ip frp debug recv ack","ip frp debug recv ack"},
	{EIGRP_CMD_TYPE_DBG_RECV,"ip frp debug recv ","ip frp debug recv "},
	
	{EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_UPDATE,"ip frp debug packet update detail","ip frp debug packet update detail"},
	{EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_QUERY,"ip frp debug packet query detail","ip frp debug packet query detail"},
	{EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_REPLY,"ip frp debug packet reply detail","ip frp debug packet reply detail"},
	{EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_HELLO,"ip frp debug packet hello detail","ip frp debug packet hello detail"},
	{EIGRP_CMD_TYPE_DBG_PACKET_DETAIL_ACK,"ip frp debug packet ack detail","ip frp debug packet ack detail"},
	{EIGRP_CMD_TYPE_DBG_PACKET_DETAIL,"ip frp debug packet detail","ip frp debug packet detail"},
	
	{EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_UPDATE,"no ip frp debug packet update detail","no ip frp debug packet update detail"},
	{EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_QUERY,"no ip frp debug packet query detail","no ip frp debug packet query detail"},
	{EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_REPLY,"no ip frp debug packet reply detail","no ip frp debug packet reply detail"},
	{EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_HELLO,"no ip frp debug packet hello detail","no ip frp debug packet hello detail"},
	{EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL_ACK,"no ip frp debug packet ack detail","no ip frp debug packet ack detail"},
	{EIGRP_CMD_TYPE_NO_DBG_PACKET_DETAIL,"no ip frp debug packet detail","no ip frp debug packet detail"},
	
	{EIGRP_CMD_TYPE_DBG_PACKET,"debug ip frp packet ","no debug ip frp packet "},
	{EIGRP_CMD_TYPE_DBG_EVENT,"debug ip frp event ","debug ip frp event "},
	{EIGRP_CMD_TYPE_DBG_TIMER,"debug ip frp timer ","debug ip frp timer "},
	{EIGRP_CMD_TYPE_DBG_TASK,"debug ip frp task ","debug ip frp task "},
	{EIGRP_CMD_TYPE_DBG_ROUTE,"debug ip frp route ","debug ip frp route "},
	{EIGRP_CMD_TYPE_DBG_INTERNAL,"debug ip frp invisible ","debug ip frp invisible "},
	{EIGRP_CMD_TYPE_DBG_ALL,"debug ip frp all ","no debug ip frp all "},
	
	{EIGRP_CMD_TYPE_NO_DBG_SEND_UPDATE,"no debug ip send update","no debug ip send update"},
	{EIGRP_CMD_TYPE_NO_DBG_SEND_QUERY,"no debug ip send query","no debug ip send query"},
	{EIGRP_CMD_TYPE_NO_DBG_SEND_REPLY,"no debug ip send reply","no debug ip send reply"},
	{EIGRP_CMD_TYPE_NO_DBG_SEND_HELLO,"no debug ip send hello","no debug ip send hello"},
	{EIGRP_CMD_TYPE_NO_DBG_SEND_ACK	,"no debug ip send ack","no debug ip send ack"},
	{EIGRP_CMD_TYPE_NO_DBG_SEND,"no debug ip send ","no debug ip send "},
	
	{EIGRP_CMD_TYPE_NO_DBG_RECV_UPDATE,"no debug ip recv update","no debug ip recv update"},
	{EIGRP_CMD_TYPE_NO_DBG_RECV_QUERY,"no debug ip recv query","no debug ip recv query"},
	{EIGRP_CMD_TYPE_NO_DBG_RECV_REPLY,"no debug ip recv reply","no debug ip recv reply"},
	{EIGRP_CMD_TYPE_NO_DBG_RECV_HELLO,"no debug ip recv hello","no debug ip recv hello"},
	{EIGRP_CMD_TYPE_NO_DBG_RECV_ACK	,"no debug ip recv ack","no debug ip recv ack"},
	{EIGRP_CMD_TYPE_NO_DBG_RECV,"no debug ip recv ","no debug ip recv "},
	
	
	{EIGRP_CMD_TYPE_NO_DBG_PACKET,"no debug ip frp packet ","no debug ip frp packet "},
	{EIGRP_CMD_TYPE_NO_DBG_EVENT,"no debug ip frp event ","debug ip frp event "},
	{EIGRP_CMD_TYPE_NO_DBG_TIMER,"no debug ip frp timer ","debug ip frp timer "},
	{EIGRP_CMD_TYPE_NO_DBG_TASK,"no debug ip frp task ","debug ip frp task "},
	{EIGRP_CMD_TYPE_NO_DBG_ROUTE,"no debug ip frp route ","debug ip frp route "},
	{EIGRP_CMD_TYPE_NO_DBG_INTERNAL,"no debug ip frp invisible ","debug ip frp invisible "},
	{EIGRP_CMD_TYPE_NO_DBG_ALL,"no debug ip frp all ","no debug ip frp all "},
		
#ifdef _EIGRP_PLAT_MODULE	
	{EIGRP_CMD_TYPE_MAX,"cmd index is too big","nomal"},

	{ZEBRA_INTERFACE_ADD,"zebra client add interface",""},
	{ZEBRA_INTERFACE_DELETE,"zebra client delete interface","nomal"},
	{ZEBRA_INTERFACE_ADDRESS_ADD,"zebra client add interface address","nomal"},
	{ZEBRA_INTERFACE_ADDRESS_DELETE,"zebra client del interface address","nomal"},
	{ZEBRA_INTERFACE_UP,"zebra client interface up","nomal"},
	{ZEBRA_INTERFACE_DOWN,"zebra client interface down","nomal"},
	
	{ZEBRA_IPV4_ROUTE_ADD,"zebra client add ipv4 route","nomal"},
	{ZEBRA_IPV4_ROUTE_DELETE,"zebra client del ipv4 route","nomal"},
	{ZEBRA_IPV6_ROUTE_ADD,"zebra client add ipv6 route","nomal"},
	{ZEBRA_IPV6_ROUTE_DELETE,"zebra client del ipv6 route","nomal"},
	
	{ZEBRA_REDISTRIBUTE_ADD,"zebra client add redistrbute","nomal"},
	{ZEBRA_REDISTRIBUTE_DELETE,"zebra client del redistrbute","nomal"},
	{ZEBRA_REDISTRIBUTE_DEFAULT_ADD,"zebra client add redistrbute default","nomal"},
	{ZEBRA_REDISTRIBUTE_DEFAULT_DELETE,"zebra client del redistrbute default","nomal"},
	
	{ZEBRA_IPV4_NEXTHOP_LOOKUP,"zebra client ipv4 lookup route","nomal"},
	{ZEBRA_IPV6_NEXTHOP_LOOKUP,"zebra client ipv6 lookup route","nomal"},
	{ZEBRA_IPV4_IMPORT_LOOKUP,"zebra client ipv4 lookup import route","nomal"},
	{ZEBRA_IPV6_IMPORT_LOOKUP,"zebra client ipv6 lookup import route","nomal"},
	
	{ZEBRA_INTERFACE_RENAME,"zebra client interface rename","nomal"},
	
	{ZEBRA_ROUTER_ID_ADD,"zebra client add router id","nomal"},
	{ZEBRA_ROUTER_ID_DELETE,"zebra client del router id","nomal"},
	{ZEBRA_ROUTER_ID_UPDATE,"zebra client update router id","nomal"},
#endif	/* #ifdef _EIGRP_PLAT_MODULE */	
};
/************************************************************************************/
/************************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/************************************************************************************/
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
/************************************************************************************/
static int  ZebraEigrpIntf_Update_from_kernel (EigrpInterface_pt pEigrpIntf);
/************************************************************************************/
static int ZebraEigrpIntfAdd_from_kernel (int ifindex, char *name, struct sockaddr_in *addr, struct sockaddr_in *mask, struct sockaddr_in *dest)
{
	EigrpInterface_pt	pEigrpIntf = NULL;
	if( (EigrpMaster == NULL))
		return;

	pEigrpIntf	= EigrpPortMemMalloc(sizeof(EigrpInterface_st));
	if(pEigrpIntf)
	{
		EigrpPortMemSet(pEigrpIntf, 0, sizeof(EigrpInterface_st));

		pEigrpIntf->next = NULL;
		EigrpPortStrCpy(pEigrpIntf->name, name);
		pEigrpIntf->ifindex	= ifindex;
		//pEigrpIntf->flags;
		//pEigrpIntf->metric;
		//pEigrpIntf->mtu;
		//pEigrpIntf->bandwidth;	/* In unit of kbps */
		//pEigrpIntf->delay;	/*hanbing add 120707*/
		//pEigrpIntf->status;

		pEigrpIntf->ipaddress.ipAddr = NTOHL(addr->sin_addr.s_addr);
		pEigrpIntf->ipaddress.ipMask = NTOHL(mask->sin_addr.s_addr);
		pEigrpIntf->ipaddress.ipDstAddr = NTOHL(dest->sin_addr.s_addr);
		
		pEigrpIntf->ipaddress.intfIndex = ifindex;
		
		ZebraEigrpIntf_Update_from_kernel (pEigrpIntf);
		
		ZebraEigrpIntfAdd(pEigrpIntf);
	}
	return 0;
}
/************************************************************************************/
/************************************************************************************/
static int if_ioctl_call(int code, struct ifreq *ifrp)
{
    int sock;
    int status;
    if ((sock = socket (AF_INET, SOCK_RAW, 0)) < 0)//  SOCK_DGRAM
	return (ERROR);
    status = ioctl (sock, code, (int)ifrp);
    (void)close (sock);
    if (status != 0)
	{
	if (status != ERROR)	/* iosIoctl() can return ERROR */
	    (void)errnoSet (status);
	return (ERROR);
	}
    return (OK);
}
/*******************************************************************************
*
* ifIoctlSet - configure network interface
*
* RETURNS: OK or ERROR
*/
static int if_ioctl_set
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   code,                 /* network interface ioctl function code */
    int   val                   /* value to be changed */
    )
    {
    struct ifreq ifr;
    bzero ((caddr_t) &ifr, sizeof (ifr));
    strncpy (ifr.ifr_name, interfaceName, sizeof (ifr.ifr_name));
    switch ((UINT) code)
	{
	case SIOCSIFFLAGS:
	    ifr.ifr_flags = val;
	    break;
        case SIOCSIFMETRIC:
	    ifr.ifr_metric = val;
	    break;
        default:
	    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
	    ifr.ifr_addr.sa_family = AF_INET;
	    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = val;
	    break;
	}
    return (if_ioctl_call (code, &ifr));
    }
/*******************************************************************************
*
* ifIoctlGet - retrieve information about the network interface
*
* RETURNS: OK or ERROR
*/
static int if_ioctl_get
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   code,                 /* network interface ioctl function code */
    int  *val                   /* where to return result */
    )
    {
    struct ifreq  ifr;
    bzero ((caddr_t) &ifr, sizeof (ifr));
    strncpy (ifr.ifr_name, interfaceName, sizeof (ifr.ifr_name));
    if (if_ioctl_call (code, &ifr) == ERROR)
    	return (ERROR);
    switch ((UINT) code)
	{
	case SIOCGIFFLAGS:
	    *val = ifr.ifr_flags;
	    break;
        case SIOCGIFMETRIC:
	    *val = ifr.ifr_metric;
	    break;
        case SIOCGIFMTU:
	    *val = ifr.ifr_mtu;
	    break;
	default:
	    *val = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
	    break;
	}
    return (OK);
    }
/*******************************************************************************
*
* ifIoctl - network interface ioctl front-end
*
* Used to manipulate the characteristics of network interfaces
* using socket specific ioctl functions SIOCSIFADDR, SIOCSIFBRDADDR, etc.
* ifIoctl() accomplishes this by calling ifIoctlSet() and ifIoctlGet().
*
* RETURNS: OK or ERROR
*
* ERRNO: EOPNOTSUPP
*/
static int _if_ioctl
    (
    char *interfaceName,        /* name of the interface, i.e. ei0 */
    int code,                   /* ioctl function code */
    int arg                     /* some argument */
    )
{
    int status;
    int hostAddr;
    switch ((UINT) code)
	{
	case SIOCSIFBRDADDR:
	    if (strcmp ((char *) arg, "255.255.255.255") == 0)
		{
	        status = if_ioctl_set (interfaceName, code, 0xffffffff);
		break;
		}
	    /* fall through */
	case SIOCAIFADDR:
	case SIOCDIFADDR:
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
	    /* verify Internet address is in correct format */
	    if ((hostAddr = (int) inet_addr ((char *)arg)) == ERROR &&
		(hostAddr = hostGetByName ((char *)arg)) == ERROR)
		{
		return (ERROR);
		}
	    status = if_ioctl_set (interfaceName, code, hostAddr);
	    break;
	case SIOCSIFNETMASK:
        case SIOCSIFFLAGS:
        case SIOCSIFMETRIC:
	    status = if_ioctl_set (interfaceName, code, arg);
	    break;
	case SIOCGIFNETMASK:
	case SIOCGIFFLAGS:
	case SIOCGIFADDR:
	case SIOCGIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFMETRIC:
	case SIOCGIFMTU:
	    status = if_ioctl_get (interfaceName, code, (int *)arg);
	    break;
	default:
	    (void)errnoSet (EOPNOTSUPP); /* not supported operation */
	    status = ERROR;
	    break;
	}
    return (status);
}


#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif//IFNAMSIZ
//��ȡ�ں˽ӿڵ�ַ�����ڳ�ʼ����
static int kernel_get_addr (char *name, struct sockaddr_in *addr, struct sockaddr_in *mask, struct sockaddr_in *dest)
{
  struct ifreq ifreq;
  /* Interface's name and address family. */
  memset ((caddr_t) &ifreq, 0, sizeof (ifreq));
  strncpy (ifreq.ifr_name, name, IFNAMSIZ);
  ifreq.ifr_addr.sa_family = AF_INET;
  /* Interface's address. */
  if (if_ioctl_call (SIOCGIFADDR, &ifreq) == ERROR)
  {
	  printf("error con not get interface address: %s\n",ifreq.ifr_name);
	  return (ERROR);
  }
  memcpy(addr, &ifreq.ifr_addr, sizeof(struct sockaddr_in));

  /* Interface's network mask. */
  if (if_ioctl_call (SIOCGIFNETMASK, &ifreq) == ERROR)
  {
	  printf("error con not get interface address netmask:%s\n",ifreq.ifr_name);
	  return (ERROR);
  }
  memcpy(mask, &ifreq.ifr_addr, sizeof(struct sockaddr_in));

  /* Point to point or borad cast address pointer init. */
  if (if_ioctl_call (SIOCGIFBRDADDR, &ifreq) == ERROR)
  {
	  printf("error con not get interface boardcast address: %s\n",ifreq.ifr_name);
	  return (ERROR);
  }
  memcpy(dest, &ifreq.ifr_addr, sizeof(struct sockaddr_in));

  return OK;
}
/////////////////////////////////////////////////////////////////////////////////////////
static int _if_bandwidth_update (EigrpInterface_pt pEigrpIntf)
{ 
	int index = -1;
	char name[32];
	END_OBJ *   pEndObj;
	M2_INTERFACETBL *m2If;
    memset(name, 0, 32);
    memcpy(name, pEigrpIntf->name, strlen(pEigrpIntf->name)-1);
    index = pEigrpIntf->name[strlen(pEigrpIntf->name)-1]-'0';
	pEndObj = (END_OBJ *)endFindByName(name, index);
	if(pEndObj != NULL)
	{
		m2If = (M2_INTERFACETBL *)&(pEndObj)->mib2Tbl;
		if(m2If)
			pEigrpIntf->bandwidth	= (m2If->ifSpeed /* >> 10 */);
	}
	return 0;
}
static int  ZebraEigrpIntf_Update_from_kernel (EigrpInterface_pt pEigrpIntf)
{
	int if_flag = 0;	
	int ret = -1;
#ifndef ETHERMTU
#define ETHERMTU              (1500)
#endif	
	ret = _if_ioctl (pEigrpIntf->name, SIOCGIFMTU, (int)(&if_flag));
	if(ret==-1)
	{
		pEigrpIntf->mtu = ETHERMTU;
		printf("%s :SIOCGIFMTU and set default MTU:%d ",__func__,ETHERMTU);
	}
	pEigrpIntf->mtu = if_flag;
	ret = _if_ioctl (pEigrpIntf->name, SIOCGIFMETRIC, (int)(&pEigrpIntf->metric));
	if(ret==-1)
	{
		pEigrpIntf->metric = 1;
		printf("%s :SIOCGIFMETRIC and set default metric:1 ",__func__);
	}
	_if_bandwidth_update (pEigrpIntf);
	pEigrpIntf->bandwidth =  (pEigrpIntf->bandwidth *8)/1000;
	
	ret = _if_ioctl (pEigrpIntf->name, SIOCGIFFLAGS, &if_flag);
	if(ret==-1)
	{
		printf("%s :SIOCGIFFLAGS  ",__func__);
	}
	//pEigrpIntf->flags = if_flag;
	
	BIT_RESET(pEigrpIntf->flags, EIGRP_INTF_FLAG_EIGRPED);
	
	if(if_flag & IFF_UP)
		BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_ACTIVE);
	
	if(if_flag & IFF_BROADCAST)
		BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_BROADCAST);
	
	if(if_flag & IFF_POINTOPOINT)
		BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_POINT2POINT);

	if(if_flag & IFF_MULTICAST)
		BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_MULTICAST);

	if(if_flag & IFF_LOOPBACK)
		BIT_SET(pEigrpIntf->flags, EIGRP_INTF_FLAG_LOOPBACK);
	pEigrpIntf->flags = (U32)if_flag;
	return 0;
}
//extern STATUS m2IfTblEntryGet (int search, void * pIfTblEntry);
//��ȡ�ӿ�mac��ַ�ʹ�����Ϣ���ɽӿڳ�ʼ����ʱ�����


/////////////////////////////////////////////////////////////////////////////////////////
//�ӿ������ʼ�������ڳ�ʼ��
static int _kernel_list_ioctl(int num)
{
	  int n,ret;
	  char name[32];
	  struct sockaddr_in addr, mask, dest;

	  if( (EigrpMaster == NULL))
			return;
	  printf("******************************** kernel init ********************************\n");
	  printf("%-12s %-6s %-16s %-20s %-16s\n","name", "index","address","netmask","broadcast");
	  for (n = 1; n <= num; n++)
	  {
		  memset(name, 0, sizeof(name)); 
		  if( (if_indextoname (n, name) != NULL)&&(memcmp(name, "lo0", 3)!=0) )
		  {
			ret = kernel_get_addr (name, &addr, &mask, &dest);//��ȡ��ַ��Ϣ
			if(ret == OK)
			{
				ZebraEigrpIntfAdd_from_kernel (n, name, &addr, &mask, &dest);
				printf("%-12s %-6d %-16s %-20s %-16s\n",name, n, 
						inet_ntoa(addr.sin_addr),
						inet_ntoa(mask.sin_addr),
						inet_ntoa(dest.sin_addr));
			}
		  }
	  }
	  printf("*****************************************************************************\n");  
	  return 0; 
}

int kernel_list_init(int num)
{
	return _kernel_list_ioctl(num);
}
#endif//#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
/*************************************************************************/
#ifdef EIGRP_PLAT_ZEBRA	
static int eigrp_start_up(void *p, void *y)
{
	debug_eigrp_zebra = 3;
	/*Eigrp�ͻ��˳�ʼ��*/
	zebraEigrpZclientInit(200, 6, NULL, iflist);
	//Eigrp�����̳�ʼ�� 
	zebraEigrpMasterInit(200, NULL);
	EigrpMaster->asnum = 1;		
}
/*************************************************************************/
#endif//EIGRP_PLAT_ZEBRA
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_LINUX)
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#include "getopt.h"
#include "sigevent.h"

struct thread_master *master;
static const char *pid_file = PATH_FRPD_PID;
static char config_default[] = SYSCONFDIR FRP_DEFAULT_CONFIG;
/*************************************************************************/
/*************************************************************************/
zebra_capabilities_t _caps_p [] = 
{
  ZCAP_NET_RAW,
  ZCAP_BIND,
  ZCAP_NET_ADMIN,
};
/*************************************************************************/
struct zebra_privs_t eigrp_privs =
{
#if defined(QUAGGA_USER) && defined(QUAGGA_GROUP)
  .user = QUAGGA_USER,
  .group = QUAGGA_GROUP,
#endif
#if defined(VTY_GROUP)
  .vty_group = VTY_GROUP,
#endif
  .caps_p = _caps_p,
  .cap_num_p = array_size(_caps_p),
  .cap_num_i = 0
};
/*************************************************************************/
/*************************************************************************/
struct option longopts[] =
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "pid_file",    required_argument, NULL, 'i'},
  { "socket",      required_argument, NULL, 'z'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_addr",    required_argument, NULL, 'A'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "user",        required_argument, NULL, 'u'},
  { "group",       required_argument, NULL, 'g'},
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};
/*************************************************************************/
/*************************************************************************/
/* Help information display. */
static void __attribute__ ((noreturn))
usage (char *progname, int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages LLDPD.\n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-i, --pid_file     Set process identifier file name\n\
-z, --socket       Set path of zebra socket\n\
-A, --vty_addr     Set vty's bind address\n\
-P, --vty_port     Set vty's port number\n\
-u, --user         User to run as\n\
-g, --group        Group to run as\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
\n\
Report bugs to %s\n", progname, ZEBRA_BUG_ADDRESS);
    }
  exit (status);
}

/* SIGHUP handler. */
static void 
sighup (void)
{
  zlog_info ( "SIGHUP received");
}

/* SIGINT / SIGTERM handler. */
static void
sigint (void)
{
  zlog_notice ("Terminating on signal");
  EigrpFree();
}

/* SIGUSR1 handler. */
static void
sigusr1 (void)
{
  zlog_rotate (NULL);
}

struct quagga_signal_t eigrp_signals[] =
{
  {
    .signal = SIGHUP,
    .handler = &sighup,
  },
  {
    .signal = SIGUSR1,
    .handler = &sigusr1,
  },  
  {
    .signal = SIGINT,
    .handler = &sigint,
  },
  {
    .signal = SIGTERM,
    .handler = &sigint,
  },
};
/*************************************************************************/
int main (int argc, char **argv)
{
  char *p;
  char *vty_addr = NULL;
  int vty_port = FRP_VTY_PORT;
  int daemon_mode = 0;
  char *config_file = NULL;
  char *progname;
  int ret = 0;

  /* Set umask before anything for security */
  umask (0027);
  /* get program name */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  while (1)
  {
      int opt;
      opt = getopt_long (argc, argv, "df:i:z:hA:P:u:g:av", longopts, 0);
      if (opt == EOF)
    	  break;

      switch (opt)
      {
      case 0:
    	  break;
      case 'd':
    	  daemon_mode = 1;
    	  break;
      case 'f':
    	  config_file = optarg;
    	  break;
      case 'A':
    	  vty_addr = optarg;
    	  break;
      case 'i':
          pid_file = optarg;
          break;
      case 'z':
    	  zclient_serv_path_set (optarg);
    	  break;
      case 'P':
          if (strcmp(optarg, "0") == 0)
          {
              vty_port = 0;
              break;
          }
          vty_port = atoi (optarg);
          if (vty_port <= 0 || vty_port > 0xffff)
            vty_port = FRP_VTY_PORT;
          break;
      case 'u':
    	  eigrp_privs.user = optarg;
    	  break;
      case 'g':
    	  eigrp_privs.group = optarg;
    	  break;
      case 'v':
    	  print_version (progname);
    	  exit (0);
    	  break;
      case 'h':
    	  usage (progname, 0);
    	  break;
      default:
    	  usage (progname, 1);
    	  break;
	}
    }
  /* Invoked by a priviledged user? -- endo. */
  if (geteuid () != 0)
    {
      errno = EPERM;
      perror (progname);
      exit (1);
    }

  if (daemon_mode && daemon (0, 0) < 0)
    {
      printf("FRPD daemon failed: %s", strerror(errno));
      exit (1);
    } 
  zlog_default = openzlog (progname, ZLOG_FRP,
			   LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);

  master = thread_master_create ();
  if(master == NULL)
    return 0;

  /* Library inits. */
  zprivs_init (&eigrp_privs);
  signal_init (master, array_size(eigrp_signals), eigrp_signals);

  cmd_init (1);
  vty_init (master);
  memory_init ();
  
  ret = zebraEigrpZclientInit(daemon_mode, 16, master, NULL);
  if(ret == FAILURE)
	  return 0;
  /* Get configuration file. */
  vty_read_config (config_file, config_default);

  /* Change to the daemon program. */
  if (daemon_mode && daemon (0, 0) < 0)
    {
      zlog_err("LLDPd daemon failed: %s", strerror(errno));
      exit (1);
    }
  /* Process id file create. */
  pid_output (pid_file);
  /* Create VTY socket */
  vty_serv_sock (vty_addr, vty_port, FRP_VTYSH_PATH);
  /* Print banner. */
  zlog_notice ("FRPd %s starting: vty@%d", QUAGGA_VERSION, vty_port);

  //Eigrp�����̳�ʼ�� 
  zebraEigrpMasterInit(daemon_mode, NULL);

  /* Not reached. */
  return (0);
}
/*************************************************************************/
#endif//(EIGRP_OS_TYPE == EIGRP_OS_LINUX)
/*************************************************************************/
/*************************************************************************/
#endif //_EIGRP_PLAT_MODULE
/*************************************************************************/
/*************************************************************************/
/*************************************************************************/
#if (EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)	
/*************************************************************************/
/*************************************************************************/
#ifdef _EIGRP_PLAT_MODULE_DEBUG
/* only test  */
int frp_id()
{
	zebraEigrpCmdApiRouterId(0, EigrpMaster->asnum, tickGet()+rand());
//	EigrpIpSetRouterId(ddb);		/* Just in case. */	
}

int time_debug = 0;
int timetest(void *pdunp, U32 param)
{
	char * pst = (char *)param;
	if(time_debug)
		printf("timetest interuppt:%s\n",pst);
	//zebraEigrpUtilSchedAdd(EigrpMaster->ZebraSched, frp_Show, NULL, 0);
}
int frp_network_mask(char *ip, int mask)
{
	U32	net;
	//if(gpEigrp->EigrpSem)
	//	EigrpPortSemBTake(gpEigrp->EigrpSem);
	EigrpUtilConvertStr2Ipv4(&net, ip);
	//EigrpUtilConvertStr2Ipv4Mask(&mask, m);
	EigrpCmdApiNetwork(0, EigrpMaster->asnum, net ,mask);
	//if(gpEigrp->EigrpSem)
	//	EigrpPortSemBGive(gpEigrp->EigrpSem);
	return 0;
}

/*
zebraTaskMemory "tEigrp"
ts "tEigrp"
tr "tEigrp"
*/	


//EigrpPortTaskStart() 
void frp_init()
{
	debug_eigrp_zebra = 3;
	/*Eigrp�ͻ��˳�ʼ��*/
#ifdef EIGRP_PLAT_ZEBRA		
	zebraEigrpZclientInit(200, 6, NULL, iflist);
#else
	zebraEigrpZclientInit(200, 6, NULL, NULL);
#endif//#ifdef EIGRP_PLAT_ZEBRA	
#if	(EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	//����ʹ�õĽӿڱ��ʼ��
	//sys_if_init_test();
	printf("sys_if_init_test\n");
	taskDelay(sysClkRateGet());
#endif
	//Eigrp�����̳�ʼ�� 
	zebraEigrpMasterInit(200, NULL);
	//EigrpMaster->asnum = 1;
	//����·��ID	
	//frp_id();
	printf("EigrpInit\n");
	//taskDelay(sysClkRateGet());
	//����Eigrp·��Э��
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)	
	EigrpCmdApiRouterEigrp(0, EigrpMaster->asnum);
#endif	
	printf("EigrpCmdApiRouterEigrp\n");
	taskDelay(sysClkRateGet());
	
	//EigrpCmdApiMetricWeights(0,asNum,0,0,1,0,0);
	printf("EigrpCmdApiMetricWeights\n");


	//����·���طֲ�
	//EigrpCmdApiRedistType�����ĵ����ڶ���������ָ���̱��֮��ģ�����ָ·�ɿ�����Ĭ�ϲ�ʹ�ã�Ĭ�� Ϊ0
	//��Ҫ�����ģ�ʹ�� EigrpCmdApiRedistTypeMetric ����
#if (EIGRP_PLAT_TYPE == EIGRP_PLAT_BSD)
	EigrpCmdApiRedistType(0, EigrpMaster->asnum,"static", 0,0);	//���طֲ������ھ�̬·���طֲ���EIGRP��
	EigrpCmdApiRedistType(0, EigrpMaster->asnum,"connect", 0,0);
	EigrpCmdApiRedistType(0, EigrpMaster->asnum,"rip", 0,0);
	EigrpCmdApiRedistType(0, EigrpMaster->asnum,"ospf", 0,0);
#endif
	//printf("��ʼ��EIGRP���\n");
	//EigrpCmdApiRouteTypeAtoi
	//EIGRP_ROUTE_EIGRP
	EigrpPortTimerCreate(timetest, "adajhdajhdgjdhaj", 3);
}

int frp_addRoute_test()
{
	printf("%s\n",__func__);

#ifdef EIGRP_PLAT_ZEBRA	
	//��quaggaע����������
	zebra_node_func_install(0, CONFIG_NODE, eigrp_start_up);
#endif//EIGRP_PLAT_ZEBRA	

//	frp_init();
}
#endif//(EIGRP_OS_TYPE == EIGRP_OS_VXWORKS)
/*
 * 
 *  select: timeout:S_objLib_OBJ_UNAVAILABLE
 select: timeout:S_objLib_OBJ_UNAVAILABLE
 select: timeout:S_objLib_OBJ_UNAVAILABLE
 */

void showNeigh(int show_detail)
{
	EigrpPdb_st	*pdb;
	EigrpDualDdb_st	*ddb;
	EigrpDualPeer_st	*peer;
	S32	banner;
	U32	holdTime, t, t1, t2, t3, seq_number;
	EigrpDbgCtrl_st	dbg;

	static const S8 peer_banner[]	={
		" H       Address     Interface   Hold    Uptime     SRTT    RTO    Q   Seq"
		"\r\n                                  (sec)              (ms)          Cnt  Num"
	};


	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		ddb = pdb->ddb;

		/* ÿ�����̱��� */
		printf("-----------------------------------------------------\n");
		printf("\r\nIP-EIGRP %s for process %d :\r\n", "neighbors", ddb->asystem);

		banner = FALSE;
		/* ÿ����ʾһ���ھӵ���Ϣ */
		for(peer = ddb->peerLst; peer; peer = peer->nextPeer){
	   		 /* ��ʾ��ͷ,���е����Ƶ�λ */
			if(!banner){
				printf("%s", peer_banner);
				banner	= TRUE;
			}

			holdTime = EigrpUtilMgdTimerLeftSleeping(&peer->holdingTimer) / EIGRP_MSEC_PER_SEC;

			t	= EigrpPortGetTimeSec() - peer->uptime;
			if(t > EIGRP_SEC_PER_DAY){
				t1 = t / EIGRP_SEC_PER_DAY;
				t2 = (t - t1 * EIGRP_SEC_PER_DAY) / EIGRP_SEC_PER_HOUR;
				t3 = (t - t1 * EIGRP_SEC_PER_DAY - t2 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
			}else{
				t1 = t / EIGRP_SEC_PER_HOUR;
				t2 = (t - t1 * EIGRP_SEC_PER_HOUR) / EIGRP_SEC_PER_MIN;
				t3 = t % EIGRP_SEC_PER_MIN;
			}
			seq_number = peer->lastSeqNo;
			if(seq_number >= 10000){
				seq_number %= 10000;
			}
			printf("\r\n%3d %14s %12s    %-3d   %2d:%2d:%2d    %-4d   %-5d   %-2d  %d",
						peer->peerHandle,
						(*ddb->printAddress)(&peer->source),
						peer->iidb->idb ? (S8 *)peer->iidb->idb->name : "Null0",
						holdTime, t1, t2, t3, peer->srtt, peer->rto,
						peer->xmitQue[EIGRP_UNRELIABLE_QUEUE]->count +
						peer->xmitQue[EIGRP_RELIABLE_QUEUE]->count,
						seq_number);

			if(show_detail ==  TRUE){
				if(peer->lastStartupSerNo){
					printf("\r\n   Last startup serial %d",peer->lastStartupSerNo);
				}

				printf("\r\n   Version %d.%d/%d.%d, ",peer->peerVer.majVer,peer->peerVer.minVer,peer->peerVer.eigrpMajVer,
							peer->peerVer.eigrpMinVer);

				printf("Retrans: %d, Retries: %d",peer->retransCnt, peer->retryCnt);

				if(peer->flagNeedInit){
					printf(", Waiting for Init");
				}

				if(peer->flagNeedInitAck){
					printf(", Waiting for Init Ack");
				}

				if(EIGRP_TIMER_RUNNING(peer->reInitStart)){
					printf(", reinit for %d sec",EIGRP_ELAPSED_TIME(peer->reInitStart));
				}

			}

		} 
		printf("\r\n");
		printf("-----------------------------------------------------\n");
		banner = FALSE;
	}
	printf("\r\n");
}

void	showTopo()
{
	EigrpDualNdb_st	*dndb = NULL, *next_dndb = NULL ;
	EigrpDualRdb_st	*drdb = NULL, *next_drdb = NULL;
	EigrpHandle_st	reply_status;
	EigrpDualPeer_st	*peer;
	EigrpIntf_pt		pEigrpIntf;
	EigrpPdb_st  		*pdb;
	EigrpDualDdb_st 	*ddb;
	
	S32	i, banner, active, connectFlag, redistributed, rStatic, summary, FSonly, display_drdb;
	U32	handlesize, handle_num, usedLen;

	static const S8 topo_banner[]	={
		"\n\rCodes: P - Passive, A - Active, U - Update, Q - Query, R - Reply, "
		"\r\n       r - Reply status\n\r"
	};

	usedLen	= 0;
	EigrpUtilMemZero((void *) &reply_status, (U32)(sizeof(reply_status)));

	for(pdb = (EigrpPdb_st *)gpEigrp->protoQue->head; pdb; pdb = pdb->next){
		ddb = pdb->ddb;
		/* ÿ�����̱��� */
		printf("-----------------------------------------------------\n");
		printf("\nIP-EIGRP %s for process %d :\n", " Topology Table", ddb->asystem);

		/* �������Բ�ͬ�Ĳ�������ʾ���˱�ĸ���·����Ϣ*/

		/* ����һ��Ӧ��״̬�� */
		handlesize = EIGRP_HANDLE_MALLOC_SIZE(ddb->handle.arraySize);
		if(handlesize){
			reply_status.array = EigrpPortMemMalloc(handlesize);
		}
		if(handlesize && !reply_status.array){
			return ;
		}
		if(handlesize && reply_status.array){
			EigrpUtilMemZero((void *)reply_status.array, handlesize);
		}
		/* ѭ����ʾ������Ҫ��ʾ�����˱��� */
		banner = FALSE;

		for(i = 0; i < EIGRP_NET_HASH_LEN; i++){
			for(dndb = ddb->topo[ i ]; dndb; dndb = dndb->next){
				/* ����ÿһ��dndb */
				/* 1.��ʾdndb��Ϣ */
				active = EigrpDualDndbActive(dndb);
				reply_status.used		= dndb->replyStatus.used;
				reply_status.arraySize	= dndb->replyStatus.arraySize;
				if(active){
					EigrpPortMemCpy((U8 *)reply_status.array, 	(U8 *) dndb->replyStatus.array,
									(U32)(EIGRP_HANDLE_MALLOC_SIZE(reply_status.arraySize)));
				}
				FSonly = FALSE;

				/* ���˱��˵���� */
				if(!banner){
					printf("%s",topo_banner);
					banner	= TRUE;
				}
			
				printf("\r\n%s %s, %d %successors, FD is ", (active) ? "A" : "P",
											(*ddb->printNet)(&dndb->dest), dndb->succNum, dndb->succ ? "S" : "s");
				if(active){
					if(dndb->oldMetric == (U32) - 1){
						printf("Inaccessible");
					}else{
						printf("%u", dndb->oldMetric);
					}
				}else{
					if(!dndb->rdb || dndb->rdb->metric == (U32) - 1){
						printf("Inaccessible");
					}else{
						printf( "%u", dndb->rdb->metric);
					}
				}

				printf( "%s", EigrpPrintSendFlag(dndb->sndFlag));

				printf( ", serno %d", dndb->xmitThread.serNo);
				if(dndb->xmitThread.refCnt){
					printf(", refCnt %d",
					dndb->xmitThread.refCnt);
				}
				if(dndb->xmitThread.anchor){
					printf(", anchored");
				}
			

				/* Indicate number of outstanding replies. */
				if(reply_status.used != 0){
					printf("\r\n    %d replies", reply_status.used);
				}
				if(active){
					/* ��ʾ��ʱ����Active���岢�����Ӧ������ʾ�Ѿ�����Active�೤ʱ�� */
					printf(", active : %d(from %u)", EIGRP_ELAPSED_TIME(dndb->activeTime),
						                    dndb->activeTime);
					printf(", query-origin: %s", EigrpDualQueryOrigin2str(dndb->origin));
				}

				/* ���һ��1: ���dndb��Ϣ */
				/* ���,����ָ��ָ�ص�������ͷ.������ɵȴ�,�ȴ�ʱ���ܻ��ں���dual_dndbdelete()��ɾ��
				  *dndb */

				/* 2.����ÿһ��Drdb */
				for(drdb = dndb->rdb; drdb; drdb = drdb->next){
					/* ֻ��show ip eigrp top all ʱ����ʾ��FS��·�� */
					display_drdb = !(FSonly && (drdb->succMetric >= dndb->oldMetric));
					/* mem_lock(drdb); */
					/* ��ʾ��һ����Metric */
					connectFlag	= (drdb->origin == EIGRP_ORG_CONNECTED);
					redistributed	= (drdb->origin == EIGRP_ORG_REDISTRIBUTED);
					rStatic			= (drdb->origin == EIGRP_ORG_RSTATIC);
					summary		= (drdb->origin == EIGRP_ORG_SUMMARY);

					if(display_drdb){
						printf("\r\n       %s via %s",(dndb->succ == drdb) ? "*" : " ",summary ? "Summary" : connectFlag ? "Connected" :
						redistributed ? "Redistributed" :rStatic ? "Rstatic" :	(*ddb->printAddress)(&drdb->nextHop));

						if(drdb->succMetric == EIGRP_METRIC_INACCESS){
							printf( " (Infinity/Infinity)");
						}else if(drdb->metric == EIGRP_METRIC_INACCESS){
							printf(" (Infinity/%u)", drdb->succMetric);
						}else if(!connectFlag){
							printf( " (%u/%u)", drdb->metric,drdb->succMetric);
						}

						/* Ӧ��״̬��� */
						if(active && (drdb->handle != EIGRP_NO_PEER_HANDLE) &&
							EigrpTestHandle(ddb, &reply_status, drdb->handle)){
							printf(", r");
							EigrpClearHandle(ddb, &reply_status, drdb->handle);
						}

						/* update, query, reply ���ķ��ͱ��*/
						printf("%s", EigrpPrintSendFlag(drdb->sndFlag));

						/* ��һ���Ľӿ� */
						if(drdb->iidb){
							pEigrpIntf	= EigrpDualIdb(drdb->iidb);
 							printf(", %s", pEigrpIntf ? (S8 *)pEigrpIntf->name : "Null0");
						}

						/* ������ */
						if(drdb->thread.serNo){
							printf(", serno %d", drdb->thread.serNo);
						}
						if(drdb->thread.anchor){
							printf(", anchored");
						}
					}  /* if(display_drdb) */
				} /* for drdb loop */

				/* 3.��ʾdndbӦ����Ϣ */
				/* ��ʾ���ڵȴ���Щ�ھӵ�Ӧ��, ǰ���Ѿ���ʾ��Ӧ����λ�Ĳ�����ʾ */
				if(active && reply_status.used){
					printf("\r\n    Remaining replies:");
					for(handle_num = 0;(handle_num < EIGRP_CELL_TO_HANDLE(reply_status.arraySize));handle_num++){
						if(EigrpTestHandle(ddb, &reply_status, handle_num)){
							peer = EigrpHandleToPeer(ddb, handle_num);
							if(peer){
								printf( "\r\n         via %s, r, %s",(*ddb->printAddress)(&peer->source),peer->iidb->idb ? (S8 *)peer->iidb->idb->name : "Null0");
							}else{
								printf("\r\n         via unallocated handle %d",handle_num);
							}
							EigrpClearHandle(ddb, &reply_status, handle_num);
						}
					}
				}

			} /* for dndb loop */
		} /* for topology */

		if(reply_status.array){
			EigrpPortMemFree(reply_status.array);
			reply_status.array = (U32*) 0;
		}
		
		printf("\r\n");
		printf("-----------------------------------------------------\n");
	}
}

void showConfIntf(int icrpAsNum)
{
	EigrpConfIntf_pt		pConfIntf;
	EigrpConfIntfHello_pt	pHello;
	EigrpConfIntfHold_pt	pHold;
	EigrpConfIntfBandwidth_pt		pBandwidth;
	EigrpConfIntfBw_pt		pBw;
	EigrpConfIntfDelay_pt		pDelay;
	EigrpConfIntfSplit_pt	pSplit;
	EigrpConfIntfSum_pt	pSum;

	printf("%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s%-10s\n", "ifName", "isSei", "helloInt", "helloHold", "bandwidth", "bw", "delay", "split", "summary");
	for(pConfIntf = gpEigrp->conf->pConfIntf; pConfIntf; pConfIntf = pConfIntf->next){

		pHello = NULL;
		if(pConfIntf){
			for(pHello = pConfIntf->hello; pHello; pHello = pHello->next){
				if(pHello->asNum == icrpAsNum){
					break;
				}
			}
		}

		pHold	= NULL;
		if(pConfIntf){
			for(pHold = pConfIntf->hold; pHold; pHold = pHold->next){
				if(pHold->asNum == icrpAsNum){
					break;
				}
			}
		}

		pBandwidth = NULL;
		if(pConfIntf){
			for(pBandwidth = pConfIntf->bandwidth; pBandwidth; pBandwidth = pBandwidth->next){
				if(pBandwidth->asNum == icrpAsNum){
					break;
				}
			}
		}

		pBw = NULL;
		if(pConfIntf){
			for(pBw = pConfIntf->bw; pBw; pBw = pBw->next){
				if(pBw->asNum == icrpAsNum){
					break;
				}
			}
		}

		pDelay = NULL;
		if(pConfIntf){
			for(pDelay = pConfIntf->delay; pDelay; pDelay = pDelay->next){
				if(pDelay->asNum == icrpAsNum){
					break;
				}
			}
		}

		pSplit= NULL;
		if(pConfIntf){
			for(pSplit = pConfIntf->split; pSplit; pSplit = pSplit->next){
				if(pSplit->asNum == icrpAsNum){
					break;
				}
			}
		}

		pSum= NULL;
		if(pConfIntf){
			for(pSum = pConfIntf->delay; pSum; pSum = pSum->next){
				if(pSum->asNum == icrpAsNum){
					break;
				}
			}
		}
		
		printf("%-10s", pConfIntf->ifName);
		printf("%-10d", pConfIntf->IsSei);

		if(pHello)
			printf("%-10ld", pHello->hello);
		else 
			printf("%-10s", "-1");
		if(pHold)
			printf("%-10ld", pHold->hold);
		else 
			printf("%-10s", "-1");
		if(pBandwidth)
			printf("%-10ld", pBandwidth->bandwidth);
		else 
			printf("%-10s", "-1");
		if(pBw)
			printf("%-10ld", pBw->bandwidth);
		else 
			printf("%-10s", "-1");
		if(pDelay)
			printf("%-10ld", pDelay->delay);
		else 
			printf("%-10s", "-1");
		if(pSplit)
			printf("%-10ld", pSplit->split);
		else 
			printf("%-10s", "-1");

		if(pSum){
			printf("%-10x/%-10x", pSum->ipNet, pSum->ipMask);
		}else 
			printf("%-10s", "-1");

		printf("\n");
	}
}
#endif//_EIGRP_PLAT_MODULE_DEBUG
