#include "zvrrpd.h"
#include "zvrrp_if.h"
#include "zvrrp_packet.h"
#include "zvrrp_sched.h"
#include "zvrrp_zebra.h"


/*******************************************************************************/
/*******************************************************************************/
struct zvrrp_master *gVrrpMatser = NULL;
/*******************************************************************************/
/*******************************************************************************/
const char * zvrrpOperStateStr[] = 
{
		"Unknow",
		"Initalize",
		"Backup",
		"Master",
		"Unknow",
};
const char * zvrrpOperAdminStateStr[] = 
{
		"Unknow",
		"UP",
		"DOWN",
		"Unknow",
};
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
struct zvrrp_master *zvrrp_master_lookup(void)
{
    return gVrrpMatser;
}
/*******************************************************************************/
/****************************************************************
 NAME    : vrrp_vsrv_init            00/02/06 09:18:02
 AIM    :
 REMARK    :
****************************************************************/
static void zvrrp_vsrv_init( vrrp_rt *vsrv )
{
    int no = 0;
    no = vsrv->no;    
    memset( vsrv, 0, sizeof(*vsrv) );
    vsrv->no          = no;
    vsrv->state       = zvrrpOperState_initialize;
    vsrv->priority    = VRRP_PRIO_DFL;
    vsrv->oldpriority = VRRP_PRIO_DFL;
    vsrv->adver_int   = VRRP_ADVER_DFL*VRRP_TIMER_HZ;
    vsrv->preempt     = VRRP_PREEMPT_DFL;
    vsrv->adminState  = zvrrpOperAdminState_down;
    vsrv->zvrrp_master = gVrrpMatser;
    //vsrv->rowStatus   = zvrrpOperRowStatus_notReady;
}
/*******************************************************************************/
vrrp_rt * zvrrp_vsrv_lookup( int vrid )
{
    vrrp_rt * vsrv = NULL;
    if( (vrid >0)&&(vrid < VRRP_VSRV_SIZE_MAX)&&(gVrrpMatser) )
    {
    	vsrv = &(gVrrpMatser->gVrrp_vsrv[vrid]);
    	return vsrv;
    }
    return NULL;
}
/****************************************************************
 NAME    : zvrrp_goto_master            00/02/07 00:15:26
 AIM    :
 REMARK    : called when the state is now MASTER
****************************************************************/
static int zvrrp_goto_master_state( vrrp_rt *vsrv )
{
    int    i;

    /* ����Ӳ���ӿڽ���Ŀ��MACΪ����MAC�ı��� */


    /* Ϊ����IP���·�� */


    /* send an advertisement */
    zvrrp_send_adv( vsrv, vsrv->priority );
    /* send gratuitous arp for each virtual ip */
    for( i = 0; i < vsrv->naddr; i++ )
        send_gratuitous_arp( vsrv, vsrv->vaddr[i].addr, 1 );
    /* init the struct */
    VRRP_TIMER_SET( vsrv->adver_timer, vsrv->adver_int );
    vsrv->state = zvrrpOperState_master;
    vsrv->staBecomeMaster++;
    return OK;
}

/****************************************************************
 NAME    : zvrrp_leave_master            00/02/07 00:15:26
 AIM    :
 REMARK    : called when the state is no more MASTER
****************************************************************/
static int zvrrp_leave_master_state( vrrp_rt *vsrv, int advF )
{
    //int i;
    //ushort_t vlanid = 0;
    struct sockaddr_in inaddr;
    //ulong_t idlen, vid, swId, ulValueLen;
    //STATUS rc;

    bzero((char *)&inaddr, sizeof(struct sockaddr_in));
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    inaddr.sin_len = sizeof(struct sockaddr_in);
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
    inaddr.sin_family = AF_INET;
    
    /* ����Ӳ���ӿڲ��ٽ���Ŀ��MACΪ����MAC�ı��� */    
  
    /* ɾ������IP·�� */

    /* if we stop vrrpd, warn the other routers to speed up the recovery */
    if( advF ){
        zvrrp_send_adv( vsrv, VRRP_PRIO_STOP );
        vsrv->staPriZeroPktsSent++;
    }
    return OK;
}
/****************************************************************
 NAME    : zvrrp_on_init_state                00/02/07 00:15:26
 AIM    :
 REMARK    : rfc2338.6.4.1
****************************************************************/
static int zvrrp_on_init_state( vrrp_rt *vsrv )
{
    int delay = 0;
    //int rc = OK;
    //ulong_t idx = 0;
    //ulong_t vlen;
    
    if (!vsrv || vsrv->adminState != zvrrpOperAdminState_up)
    {
        return -1;
    }

    if( vsrv->priority == VRRP_PRIO_OWNER)
    {
    	zvrrp_goto_master_state( vsrv );
    }
    else
    {
        delay = 3*vsrv->adver_int + VRRP_TIMER_SKEW(vsrv) +
                        (vsrv->preempt ? vsrv->delay : 0);
        VRRP_TIMER_SET( vsrv->ms_down_timer, delay );
        vsrv->state = zvrrpOperState_backup;

        //Vrrp_VsrpStateChangeCallback(vsrv);
    }
    return OK;
}
/****************************************************************
 NAME    : zvrrp_on_backup_state                00/02/07 00:15:26
 AIM    :
 REMARK    : rfc2338.6.4.2
****************************************************************/
static int zvrrp_on_backup_state( vrrp_rt *vsrv, char * pPkt )
{
    struct ip * iph = NULL;
    vrrp_pkt * hd = NULL;
    int delay = 0;
    
    if (vsrv->adminState != zvrrpOperAdminState_up)
    {
        vsrv->state = zvrrpOperState_initialize;
        return OK;
    }

    /* �ӿ�IP��ַ�ѱ�ɾ�� */
    //if (!vsrv->vif.ipaddr)
    if (!vsrv->vif.address)
    {
        VRRP_TIMER_CLR( vsrv->ms_down_timer );
        vsrv->adminState = zvrrpOperAdminState_down;
        vsrv->state = zvrrpOperState_initialize;
        return OK;
    }

    /* �յ�VRRP���� */
    iph = (struct ip *)pPkt;
    hd = (vrrp_pkt *)((char *)iph + (iph->ip_hl<<2));
    
    if ( hd->priority == 0 )
    {
        delay = VRRP_TIMER_SKEW(vsrv);
        delay = (delay == 0) ? 1: delay;
        VRRP_TIMER_SET( vsrv->ms_down_timer, delay );
        vsrv->staPriZeroPktsRcvd++;
    }
    else if( !vsrv->preempt || hd->priority > vsrv->priority ||
            (hd->priority == vsrv->priority &&
            //ntohl(iph->ip_src.s_addr) > vsrv->vif.ipaddr))
            ntohl(iph->ip_src.s_addr) > ntohl(vsrv->vif.address->u.prefix4.s_addr))) 		
    {
        delay = 3*vsrv->adver_int + VRRP_TIMER_SKEW(vsrv) +
                        (vsrv->preempt ? vsrv->delay : 0);
        VRRP_TIMER_SET( vsrv->ms_down_timer, delay );
    }
    return OK;
}
/****************************************************************
 NAME    : zvrrp_on_master_state                00/02/07 00:15:26
 AIM    :
 REMARK    : rfc2338.6.4.3
****************************************************************/
static int zvrrp_on_master_state( vrrp_rt *vsrv, char * pPkt )
{    
    struct ip * iph = NULL;
    vrrp_pkt * hd = NULL;
    int delay = 0;
    
    if (vsrv->adminState != zvrrpOperAdminState_up)
    {
        vsrv->state = zvrrpOperState_initialize;
        return OK;
    }
    
    /* �ӿ�IP��ַ�ѱ�ɾ�� */
    //if (!vsrv->vif.ipaddr)
    if(!vsrv->vif.address)
    {
        VRRP_TIMER_CLR( vsrv->adver_timer );
        zvrrp_leave_master_state( vsrv, 0 );
        vsrv->adminState = zvrrpOperAdminState_down;
        vsrv->state = zvrrpOperState_initialize;
        return OK;
    }

    /* �յ�VRRP���� */
    iph = (struct ip *)pPkt;
    hd = (vrrp_pkt *)((char *)iph + (iph->ip_hl<<2));
    
    if( hd->priority == 0 )
    {
        zvrrp_send_adv( vsrv, vsrv->priority );
        VRRP_TIMER_SET(vsrv->adver_timer,vsrv->adver_int);
        vsrv->staPriZeroPktsRcvd++;
    }
    else if( hd->priority > vsrv->priority ||
            (hd->priority == vsrv->priority &&
            //ntohl(iph->ip_src.s_addr) > vsrv->vif.ipaddr) )
            ntohl(iph->ip_src.s_addr) > ntohl(vsrv->vif.address->u.prefix4.s_addr)) ) 		
    	
    {
        delay = 3*vsrv->adver_int + VRRP_TIMER_SKEW(vsrv) +
                        (vsrv->preempt ? vsrv->delay : 0);
        VRRP_TIMER_SET( vsrv->ms_down_timer, delay );
        VRRP_TIMER_CLR( vsrv->adver_timer );
        zvrrp_leave_master_state( vsrv, 0 );
        vsrv->state = zvrrpOperState_backup;
    }
    return OK;
}
/*******************************************************************************
  ����: zvrrp_timer
  ����: VRRP���Ľ���������ں���
  ����: ��
  ���: ��
  ����: ��
  ����: 
*******************************************************************************/
static int zvrrp_timer_for_one(vrrp_rt *pVsrv)
{
	int j = 0;
	/* �����ӽӿ��Ƿ�down */
	if (pVsrv->priority < VRRP_PRIO_OWNER)
	{
		pVsrv->priority = pVsrv->oldpriority;
		for (j = 0; j < pVsrv->niftrack; j++)
		{
			// IfIsDown ����: TRUE: �ӿ�down
	        // FALSE: �ӿڷ�down
			/*
			if (IfIsDown(pVsrv->iftrack[j]))
			{
				pVsrv->priority -= pVsrv->pritrack[j];
				if (pVsrv->priority < 1)
				{
					pVsrv->priority = 1;
					break;
				}
			}
			*/
		}
	}

	switch( pVsrv->state )
	{
		case zvrrpOperState_initialize:
			zvrrp_on_init_state( pVsrv );
			break;
	                
		case zvrrpOperState_backup:
			if( VRRP_TIMER_EXPIRED(pVsrv->ms_down_timer) )
			{
				zvrrp_goto_master_state( pVsrv );
			}
			break;
	                
		case zvrrpOperState_master:
			if( VRRP_TIMER_EXPIRED(pVsrv->adver_timer) )
			{
				zvrrp_send_adv( pVsrv, pVsrv->priority );
				pVsrv->adver_timer = pVsrv->adver_int;
			}
			break;
	}
	return OK;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_check_enable(vrrp_rt *pVsrv)
{
	int ret = 0;
	char vhwaddr[6] = {0,0,0,0,0,0}; /* ����MAC��ַ */
	if(pVsrv->used && pVsrv->enable && pVsrv->vrid && pVsrv->naddr && pVsrv->vif.sock)
		ret = 1;
	if(pVsrv->vif.ifp && memcmp(vhwaddr, pVsrv->vif.hwaddr, 6))
		ret++;
	if(ret == 2)
		return 1;
	return 0;
}
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_timer_thread(void *m)
{
    vrrp_rt *pVsrv = NULL;
#ifdef ZVRRPD_ON_ROUTTING     
    pVsrv = THREAD_ARG(((struct thread *)m));
#else
    pVsrv = (vrrp_rt *)m;
#endif

	if ( (pVsrv)&&(zvrrp_check_enable(pVsrv))&&(pVsrv->vif.address)&&(pVsrv->vif.ifp) )
	{
			zvrrp_timer_for_one(pVsrv);
	}
	//
#ifdef ZVRRPD_ON_ROUTTING
	pVsrv->vif.zvrrp_timer = thread_add_timer_msec(pVsrv->zvrrp_master->master, zvrrp_timer_thread, pVsrv, VRRP_TIMER_HZ*1000);
#else
	zvrrputilschedaddtimer(pVsrv->zvrrp_master->master, zvrrp_timer_thread, pVsrv, 1000);
#endif// ZVRRPD_ON_ROUTTING
	return OK;
}
/*******************************************************************************/
/*******************************************************************************/
int zvrrp_handle_on_state(vrrp_rt *pVsrv, const char *buff)
{	
	if( !pVsrv ||(!pVsrv->vif.address))
		return ERROR;
    switch( pVsrv->state )
    {
        case zvrrpOperState_initialize:
        	zvrrp_on_init_state( pVsrv );
            break;
        case zvrrpOperState_backup:
        	zvrrp_on_backup_state( pVsrv, (char *)buff );
            break;
        case zvrrpOperState_master:
        	zvrrp_on_master_state( pVsrv, (char *)buff );
            break;
        default:
            break;
    }
    return OK;
}
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
static int zvrrp_vsrv_create(vrrp_rt *vsrv, int vrid, int opcode)
{
    vsrv->vrid = vrid;
    //vsrv->vif.ipaddr = ipaddr;
    //vsrv->vif.netmask = ipaddr;
    vsrv->vhwaddr[0] = 0x00;
    vsrv->vhwaddr[1] = 0x00;
    vsrv->vhwaddr[2] = 0x5E;
    vsrv->vhwaddr[3] = 0x00;
    vsrv->vhwaddr[4] = 0x01;
    vsrv->vhwaddr[5] = vrid;
    vsrv->adminState = zvrrpOperAdminState_up;
    return OK;
}
/*******************************************************************************/
/*******************************************************************************
  ����: Vrrp_VsrvDelete
  ����: Ϊĳ��������ɾ����������
  ����: vsrv: ���������ݽṹָ��
  ���: ��
  ����: OK: �ɹ���ERROR: ʧ��
  ����: 
*******************************************************************************/
static int zvrrp_vsrv_delete( vrrp_rt * vsrv )
{
    int rc = OK;
    int i;
    struct sockaddr_in inaddr;

    /* ���������������״̬ */
    if (vsrv->state == zvrrpOperState_master)
    {
    	zvrrp_leave_master_state(vsrv, 1);
        vsrv->adminState = zvrrpOperAdminState_down;
        vsrv->state = zvrrpOperState_initialize;
    }

    /* �������IPΪ�ӿ�IP��ʹ�ýӿ�MAC�������ARP */

    for (i = 0; i < vsrv->naddr; i++)
    {
        inaddr.sin_addr.s_addr = vsrv->vaddr[i].addr;
        // ���ݵ�ַ �ж��Ƿ������ϵͳ�ӿڱ�
        //if (gbsdifa_ifwithaddr((struct sockaddr *)&inaddr))
        {//zhurish �������ARP����
        	//zhurish gbsdarp_freeArp_send(inaddr.sin_addr.s_addr, gbsdifUnitToIfp((vsrv->vif.ifindex & 0x00000FFF) - 1));
        }
    }

    return rc;
}
/*******************************************************************************/
static void zvrrp_vsrv_free( vrrp_rt * vsrv )
{
    if (vsrv)
    {
        zvrrp_vsrv_init(vsrv);
    }
}
/*******************************************************************************/
/****************************************************************
 NAME    : vrrp_add_ipaddr            00/02/06 09:24:08
 AIM    :Ϊ��������������IP��ַ
 REMARK    :
****************************************************************/
static int vrrp_add_ipaddr( vrrp_rt *vsrv, uint32_t ipaddr )
{
	if(vsrv->naddr == VRRP_VIP_NUMS_MAX )//8����������Χ
		return ERROR;
	vsrv->vaddr[vsrv->naddr].addr = ipaddr;
    vsrv->naddr++;
    /* Ϊ����������������IPʱ�������ARP */
    if (vsrv->state == zvrrpOperState_master)
    {
        send_gratuitous_arp( vsrv, ipaddr, 1 );
    }
    return OK;
}
/****************************************************************
 NAME    : vrrp_del_ipaddr            00/02/06 09:24:08
 AIM    :Ϊ������ɾ������IP��ַ
 REMARK    :
****************************************************************/
static int vrrp_del_ipaddr( vrrp_rt *vsrv, uint32_t vipaddr )
{
	int i,pos = -1;
	if(vsrv->naddr == 0)
		return OK;
	for (i = 0; i < vsrv->naddr; i++)
	{
		if (vsrv->vaddr[i].addr == vipaddr)
		{
			pos = i;
			break;
		}
	}
	if (pos < 0)
	{
		return VRRP_CFGERR_VIPNOTEXIST;
	}
	for (i = pos; i < vsrv->naddr-1; i++)
	{
		vsrv->vaddr[i].addr = vsrv->vaddr[i+1].addr;
	}
	vsrv->naddr--;  
    return OK;
}
/*******************************************************************************/
int vrrp_mach_ipaddr( vrrp_rt *vsrv, uint32_t vipaddr )
{
	int i;
	if(vsrv->naddr == 0)
		return ERROR;
	for (i = 0; i < vsrv->naddr; i++)
	{
		if (vsrv->vaddr[i].addr == vipaddr)
		{
			return OK;
		}
	}
    return ERROR;
}
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************
  ����: zvrrp_vsrv_config
  ����: ���ñ�����
  ����: 
        vrid: ������VRID
        vipaddr: ����������IP��ַ
        opcode: ������: VRRP_OPCODE_ADDIP: ��ӱ�����IP 
                        VRRP_OPCODE_DELIP: ɾ��������IP 
                        VRRP_OPCODE_DELVR: ɾ��������
  ���: ��
  ����: �ɹ�ʱ����0��ʧ��ʱ���ش�����
  ����: ������:  ERROR: ��������
                 VRRP_CFGERR_IFWRONG: �ӿڲ����ڻ�δ������ȷ��IP��ַ
                 VRRP_CFGERR_SUBNETDIFF: ������IP��ӿ�IP������ͬһ����
                 VRRP_CFGERR_MAXVSRV: ��������Ŀ�Ѵﵽ���ֵ
                 VRRP_CFGERR_MAXVIP: ������IP��ַ��Ŀ�Ѵﵽ���ֵ
                 VRRP_CFGERR_VSRVNOTEXIST: ָ�������鲻����
                 VRRP_CFGERR_VIPNOTEXIST: ָ��������IP������
*******************************************************************************/
static int zvrrp_vsrv_config(struct zvrrp_master *vrrp, int vrid, long ipaddress, int opcode)
{
	int rc = ERROR;
    vrrp_rt * vsrv = NULL;

    vsrv = zvrrp_vsrv_lookup(vrid);
    if(vsrv == NULL)
    	return VRRP_CFGERR_VSRVNOTEXIST;
    if (vsrv)
    {
        switch (opcode)
        {
        case zvrrp_opcode_add_vrs:
            if (vsrv->used == 0)//��������
            {
                zvrrp_vsrv_init(vsrv);
                vsrv->used = 1;
                zvrrp_vsrv_create(vsrv, vrid, opcode);
                rc = OK;
                vrrp->vrid = vrid;
                break;
            }
            rc = OK;
            vrrp->vrid = vrid;
            break;
        case zvrrp_opcode_add_ip:
            //Ϊ���ñ�������������IP��ַ
            {
                if (ipaddress)
                {
                	rc = vrrp_add_ipaddr(vsrv, ipaddress);
                }
            }
            break;
        case zvrrp_opcode_del_ip:
        	 if (ipaddress)
        	 {
        		 rc = vrrp_del_ipaddr(vsrv, ipaddress);
        	 }
        	 break;
        	 
        case zvrrp_opcode_del_vrs:
        	zvrrp_vsrv_delete(vsrv);
            zvrrp_vsrv_free(vsrv);
            vrrp->vrid = -1;
            break;
        default:
            rc = ERROR;
            break;
        }        	
    }
    return rc;
}
/********************************************************************************/
static int mac_printf_debug(char *mac, int len)
{
		int i=0;
		for(i=0;i<len;i++)
			printf(" 0x%x",(unsigned char)mac[i]);
		printf(" \n");
		return OK;
}
/********************************************************************************/
static int zvrrp_interface_config(vrrp_rt * vsrv, char *ifname, int opcode)
{
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif	
	int ret = 0;
	struct interface *ifp = NULL;
	ifp = zif_lookup_by_name (ifname);
	if(ifp == NULL)
		return ERROR;
	//接口已经存在
	if(opcode == zvrrp_opcode_add_interface)
	{
		if(zvrrp_if_lookup_by_ifindex(ifp->ifindex))
			return ERROR;
		//vsrv->vif.ifindex = ifp->ifindex;
		vsrv->vif.address = zif_prefix_get_by_name (ifp);
		vsrv->vif.ifp = ifp;
		//printf("mac size:%d\n",ifp->sdl.sdl_alen);
		//mac_printf_debug(ifp->sdl.sdl_data, ifp->sdl.sdl_alen);
		ret = zvrrp_socket_init(ifp->ifindex);
#ifdef ZVRRPD_ON_ROUTTING
		if(ret)
		{
			vsrv->vif.zvrrp_timer = thread_add_timer_msec(vsrv->zvrrp_master->master, zvrrp_timer_thread, vsrv, VRRP_TIMER_HZ*1000);
			//vsrv->vif.zvrrp_timer = thread_add_timer_msec(vsrv->master->master, zvrrp_timer_thread, vsrv, VRRP_TIMER_HZ*1000);
		    vsrv->vif.zvrrp_read = thread_add_read(vsrv->zvrrp_master->master,zvrrp_reading_thread, vsrv, vsrv->vif.sock);
		}
#ifdef HAVE_STRUCT_SOCKADDR_DL
		memcpy(vsrv->vif.hwaddr, ifp->sdl.sdl_data, MIN(6, ifp->sdl.sdl_alen));
#else
		memcpy(vsrv->vif.hwaddr, ifp->hw_addr, MIN(6, ifp->hw_addr_len));
#endif
#else
		if(ret)
		{
			zvrrputilschedaddtimer(vsrv->zvrrp_master->master, zvrrp_timer_thread, pVsrv, 1000);
			zvrrputilschedaddread(vsrv->zvrrp_master->master, zvrrp_reading_thread, vsrv, vsrv->vif.sock);
		}
		memcpy(vsrv->vif.hwaddr, ifp->hw_addr, MIN(6, ifp->hw_addr_len));
#endif

	}
	else
	{
		//接口不存在
		if(!zvrrp_if_lookup_by_ifindex(ifp->ifindex))
			return ERROR;
#ifdef ZVRRPD_ON_ROUTTING
		if(vsrv->vif.zvrrp_read)
			thread_cancel (vsrv->vif.zvrrp_read);
		if(vsrv->vif.zvrrp_timer)
			thread_cancel (vsrv->vif.zvrrp_timer);
#else
		zvrrputilschedcancel(vsrv->master->master, vsrv->master->master->sched_timer);
		zvrrputilschedcancel(vsrv->master->master, vsrv->master->master->sched_read);
#endif
		if(vsrv->vif.sock)
			close(vsrv->vif.sock);
		memset(&vsrv->vif, 0, sizeof(vrrp_if));
		vsrv->vif.address = NULL;
	}
	return OK;
}
/********************************************************************************/
/*******************************************************************************
  ����: zvrrp_vsrv_interface_config
  ����: ���ñ�����
  ����: 
        vrid: ������VRID
        ifname: ����������IP��ַ
        opcode: ������: VRRP_OPCODE_ADDIP: ��ӱ�����IP 
                        VRRP_OPCODE_DELIP: ɾ��������IP 
*******************************************************************************/
static int zvrrp_vsrv_interface_config(struct zvrrp_master *vrrp, int vrid, char *ifname, int opcode)
{
	int rc = ERROR;
    vrrp_rt * vsrv = NULL;
    vsrv = zvrrp_vsrv_lookup(vrid);
    if(vsrv == NULL)
    	return VRRP_CFGERR_VSRVNOTEXIST;
    if (vsrv)
    {
        switch (opcode)
        {
        case zvrrp_opcode_add_interface:
        	rc = zvrrp_interface_config(vsrv, ifname, opcode);
            break;
        case zvrrp_opcode_del_interface:
        	rc = zvrrp_interface_config(vsrv, ifname, opcode);
        	break;
        	 
        default:
            rc = ERROR;
            break;
        }        	
    }
    return rc;
}
/************************************************************************************/
/************************************************************************************/
static int zvrrp_write_config_one(vrrp_rt * vsrv)
{
	int i = 0;
	struct in_addr ip_src;
    if( (vsrv == NULL) )
    	return ERROR;
    zvrrp_show(vsrv,"router vrrp %d",vsrv->vrid);
    zvrrp_show(vsrv," priority %d",vsrv->priority);
    for(i = 0; i < vsrv->naddr; i++)
    {
    	ip_src.s_addr = htonl(vsrv->vaddr[i].addr);
    	zvrrp_show(vsrv," virtual-ip %s",inet_ntoa (ip_src));
    }
    zvrrp_show(vsrv," advertisement-interval %d",vsrv->adver_int);
    //zvrrp_show(vsrv," state %s",zvrrpOperStateStr[vsrv->state]);
    //zvrrp_show(vsrv," adminState %s",zvrrpOperAdminStateStr[vsrv->adminState]);
    //zvrrp_show(vsrv," interface %s",if_indextoname (vsrv->vif.ifindex, NULL));
    zvrrp_show(vsrv," interface %s",vsrv->vif.ifp->name);
    //ip_src.s_addr = htonl(vsrv->vif.ipaddr);
	//zvrrp_show(vsrv," networks %s",inet_ntoa (ip_src));
    return OK;                                           
}
/************************************************************************************/
/************************************************************************************/
static int zvrrp_show_one(vrrp_rt * vsrv)
{
	int i = 0;
	char vmac[32];
	struct in_addr ip_src;
    if( (vsrv == NULL) )
    	return ERROR;
    zvrrp_show(vsrv,"VRID <%d> %s",vsrv->vrid,zvrrpOperAdminStateStr[vsrv->adminState]);
    zvrrp_show(vsrv," State		:%s(Interface %s)",zvrrpOperStateStr[vsrv->state],if_is_up(vsrv->vif.ifp)? "UP":"DOWN");
    for(i = 0; i < vsrv->naddr; i++)
    {
    	ip_src.s_addr = htonl(vsrv->vaddr[i].addr);
    	zvrrp_show(vsrv," Virtual-ip	:%s %s",inet_ntoa (ip_src),vsrv->nowner? "(IP owner)":"");
    }
    zvrrp_show(vsrv," Interface	:%s",vsrv->vif.ifp->name);
    memset(vmac, 0 , sizeof(vmac));
    sprintf(vmac, "%02x-%02x-%02x-%02x-%02x-%02x",(int)(unsigned char)vsrv->vhwaddr[0],
                                                  (int)(unsigned char)vsrv->vhwaddr[1],
                                                  (int)(unsigned char)vsrv->vhwaddr[2],
                                                  (int)(unsigned char)vsrv->vhwaddr[3],
                                                  (int)(unsigned char)vsrv->vhwaddr[4],
                                                  (int)(unsigned char)vsrv->vhwaddr[5]);
    zvrrp_show(vsrv," VMAC		:%s",vmac);
    zvrrp_show(vsrv," Advt timer	:%d msecond(s)",vsrv->adver_int);
    zvrrp_show(vsrv," Priority	:%d",vsrv->priority);
    zvrrp_show(vsrv," Preempt	:%s",vsrv->preempt? "TRUE":"FALSE");
    zvrrp_show(vsrv," Preempt delay	:%d second(s)",vsrv->delay);
    zvrrp_show(vsrv," ping		:%s",gVrrpMatser->ping_enable? "enable":"disable");
    
    ip_src.s_addr = htonl(vsrv->ms_router_id);
    zvrrp_show(vsrv," Master router id	:%s",vsrv->ms_router_id? inet_ntoa (ip_src):"Unknown");
    if(vsrv->ms_priority == 0)
    {
    	zvrrp_show(vsrv," Master Priority	:Unknown");
    }
    else
    {
    	zvrrp_show(vsrv," Master Priority	:%d",vsrv->ms_priority);
    }
    if(vsrv->ms_advt_timer == 0)
    {
    	zvrrp_show(vsrv," Master advt times	:Unknown");
    }
    else
    {
    	zvrrp_show(vsrv," Master advt times	:%d",vsrv->ms_advt_timer);
    }
    if(vsrv->ms_down_timer == 0)
    {
    	zvrrp_show(vsrv," Master down timer	:Unknown");
    }
    else
    {
    	zvrrp_show(vsrv," Master down timer	:%d",vsrv->ms_down_timer);
    }
    zvrrp_show(vsrv," Learn master mode	:%s",vsrv->ms_learn? "TRUE":"FALSE");
    return OK;                                           
}
/************************************************************************************/
/************************************************************************************/
int zvrrp_vsrv_show(int vrid)
{
    int      i;
    vrrp_rt *pVsrv = NULL;
    if(vrid != 0)
    {
    	pVsrv = zvrrp_vsrv_lookup(vrid);
    	if ( (pVsrv)&&(pVsrv->used == 1) )
    	{
    		zvrrp_show_one(pVsrv);
    	}
    	else
    		zvrrp_show(pVsrv,"VRID <%d> is not configure",vrid);
    	return OK;
    }
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
        pVsrv = zvrrp_vsrv_lookup(i);

        if ( (pVsrv)&&(pVsrv->used == 1) )
        {
        	zvrrp_show_one(pVsrv);
        }
    }        
    return OK;
}
/************************************************************************************/
/************************************************************************************/
static int zvrrp_cmd_config_process(struct zvrrp_master *vrrp)
{
	int ret = -1;
	vrrp_rt * vsrv = NULL;
	zvrrp_opcode *opcode = (zvrrp_opcode *)(vrrp->opcode);
	if((opcode == NULL)&&(opcode->cmd))
		return ERROR;
	switch(opcode->cmd)
	{
	case zvrrp_opcode_add_vrs://�������ݷ���
		ret = zvrrp_vsrv_config(vrrp, opcode->vrid, 0, zvrrp_opcode_add_vrs);
		break;
	case zvrrp_opcode_del_vrs:
		ret = zvrrp_vsrv_config(vrrp, opcode->vrid, 0, zvrrp_opcode_del_vrs);
		break;
	case zvrrp_opcode_enable_vrs://ʹ�ܱ�����
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->enable = 1;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_disable_vrs:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->enable = 0;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_enable_ping://ʹ��ping����
		vrrp->ping_enable = 1;
		ret = OK;
		break;
	case zvrrp_opcode_disable_ping:
		vrrp->ping_enable = 0;
		ret = OK;
		break;    
	case zvrrp_opcode_add_ip://Ϊ�������������IP��ַ
		ret = zvrrp_vsrv_config(vrrp, opcode->vrid, opcode->ipaddress, zvrrp_opcode_add_ip);
		break;
	case zvrrp_opcode_del_ip:
		ret = zvrrp_vsrv_config(vrrp, opcode->vrid, opcode->ipaddress, zvrrp_opcode_del_ip);
		break;
	case zvrrp_opcode_set_pri://���ñ��������ȼ�
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->oldpriority = vsrv->priority;
	    	vsrv->priority = opcode->value;     /* priority value */
	    	//vsrv->oldpriority;  /* old priority value */    
	    	//vsrv->runpriority;  /* run priority value */
	    	ret = OK;
	    }
		break;	
	case zvrrp_opcode_unset_pri:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	//vsrv->oldpriority = vsrv->priority;
	    	vsrv->priority = vsrv->oldpriority;     /* priority value */
	    	//vsrv->oldpriority;  /* old priority value */    
	    	//vsrv->runpriority;  /* run priority value */
	    	ret = OK;
	    }
		break; 
	case zvrrp_opcode_set_interval://���ñ�����helloʱ����
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->adver_int = opcode->value;
	    	vsrv->adver_timer = opcode->value;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_unset_interval:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->adver_int = VRRP_ADVER_DFL * VRRP_TIMER_HZ;
	    	vsrv->adver_timer = VRRP_ADVER_DFL * VRRP_TIMER_HZ;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_set_interval_msec:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->adver_int = opcode->value;
	    	vsrv->adver_timer = opcode->value;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_unset_interval_msec:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->adver_int = VRRP_ADVER_DFL * VRRP_TIMER_HZ;
	    	vsrv->adver_timer = VRRP_ADVER_DFL * VRRP_TIMER_HZ;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_set_learn_master://���ñ�����ѧϰ����
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->ms_learn = 1;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_unset_learn_master:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->ms_learn = 0;
	    	ret = OK;
	    }
		break;  
	case zvrrp_opcode_set_preempt:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->preempt = 1;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_unset_preempt:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->preempt = 0;
	    	ret = OK;
	    }
		break;  
	case zvrrp_opcode_set_preempt_delay:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->delay = opcode->value;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_unset_preempt_delay:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->delay = 0;//opcode->value;
	    	ret = OK;
	    }
		break;   
	case zvrrp_opcode_add_interface://Ϊ��������ӽӿ�
		ret = zvrrp_vsrv_interface_config(vrrp, opcode->vrid, opcode->ifname, zvrrp_opcode_add_interface);
		break;
	case zvrrp_opcode_del_interface:
		ret = zvrrp_vsrv_interface_config(vrrp, opcode->vrid, opcode->ifname, zvrrp_opcode_del_interface);
		break;
	case zvrrp_opcode_set_track://���ñ������ؽӿ�
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->niftrack = opcode->value;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_unset_track:
	    vsrv = zvrrp_vsrv_lookup(opcode->vrid);
	    if(vsrv)
	    {
	    	vsrv->niftrack = VRRP_IF_TRACK_MAX;//opcode->value;
	    	ret = OK;
	    }
		break;
	case zvrrp_opcode_show://��ʾ��������Ϣ
		zvrrp_vsrv_show(opcode->vrid);
		ret = OK;
		break;
	default:
		break;
	}
	opcode->respone = ret;
	return ret;
}
/************************************************************************************/
int zvrrp_cmd_config(void *m)
{
	int ret = -1;
#ifdef ZVRRPD_ON_ROUTTING     
    struct zvrrp_master *vrrp = (struct zvrrp_master *)m;
    vrrp->opcode->respone = 10;
    zvrrp_cmd_config_process(vrrp);
#else//ZVRRPD_ON_ROUTTING
    struct zvrrp_master *vrrp = (struct zvrrp_master *)m;
    vrrp->opcode->respone = 10;
    zvrrpsemtake(vrrp->SemMutexId, -1);
	zvrrputilschedcmdadd((zvrrp_sched_master *)(vrrp->master), zvrrp_cmd_config_process, vrrp);//zvrrp_cmd_config_process(void *m)
	zvrrpsemgive(vrrp->SemMutexId);
#endif// ZVRRPD_ON_ROUTTING	

	while(vrrp->opcode->respone == 10)
		zvrrpsleep(10);
	
	if(vrrp->opcode)
	{
		ret = vrrp->opcode->respone;
		if(vrrp->opcode->param1)
			free(vrrp->opcode->param1);
		if(vrrp->opcode->param2)
			free(vrrp->opcode->param2);
		if(vrrp->opcode->param3)
			free(vrrp->opcode->param3);	
	}
	return ret;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
//û��·��ƽ̨�����µ����¼�
#ifndef ZVRRPD_ON_ROUTTING
static int zvrrp_main_sched(struct zvrrp_master *vrrp)
{
	while(vrrp->init)
	{
		zvrrpsemtake(vrrp->SemMutexId, -1);
		
		zvrrputilsched((zvrrp_sched_master *)(vrrp->master), ((zvrrp_sched_master *)(vrrp->master))->sched_cmd);
		zvrrputilsched((zvrrp_sched_master *)(vrrp->master), ((zvrrp_sched_master *)(vrrp->master))->sched_func);
		//zvrrp_reading_thread(vrrp);
		zvrrputilsched((zvrrp_sched_master *)(vrrp->master), ((zvrrp_sched_master *)(vrrp->master))->sched_timer);
		zvrrputilsched((zvrrp_sched_master *)(vrrp->master), ((zvrrp_sched_master *)(vrrp->master))->sched_write);
		zvrrputilsched((zvrrp_sched_master *)(vrrp->master), ((zvrrp_sched_master *)(vrrp->master))->sched_read);
		zvrrputilsched((zvrrp_sched_master *)(vrrp->master), ((zvrrp_sched_master *)(vrrp->master))->sched_event);
		
		zvrrpsemgive(vrrp->SemMutexId);
	}
	return OK;
}
/************************************************************************************/
/************************************************************************************/
int zvrrp_main(void *vrrp)
{
	return zvrrp_main_sched((struct zvrrp_master *)vrrp);
}
/*******************************************************************************/
/*******************************************************************************/
#else//ZVRRPD_ON_ROUTTING 
/*******************************************************************************/
/****************************************************************************/
/*******************************************************************************/
int zvrrp_main(void *vrrp)
{
	struct zvrrp_master * zvrrp_daemon_master= (struct zvrrp_master *)vrrp;
	struct thread zvrrp_thread;//zebra�ػ����̵��̱߳�ʶ
#if (ZVRRPD_OS_TYPE == ZVRRPD_ON_VXWORKS)
	zvrrp_daemon_master->zclient->daemon = ZEBRA_ROUTE_VRRP;
	zclient_init (zvrrp_daemon_master->master, zvrrp_daemon_master->zclient, ZEBRA_ROUTE_VRRP);	
#endif//(ZVRRPD_OS_TYPE == ZVRRPD_ON_VXWORKS)
	while (thread_fetch (zvrrp_daemon_master->master, &zvrrp_thread))
		thread_call (&zvrrp_thread);	
}
/*******************************************************************************/
/*******************************************************************************/
#endif//ZVRRPD_ON_ROUTTING
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
//��ʼ������
int zvrrp_master_init(int pri, void *m)
{
	int i = 0;
	int ret = 0;
	if(gVrrpMatser)
		return OK;
	gVrrpMatser = malloc(sizeof(struct zvrrp_master));
	if(gVrrpMatser == NULL)
		return ERROR;
	memset(gVrrpMatser, 0, sizeof(struct zvrrp_master));

    /* ��ʼ��glob_vsrv���� */
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
    	gVrrpMatser->gVrrp_vsrv[i].no = i;
        zvrrp_vsrv_init(&gVrrpMatser->gVrrp_vsrv[i]);
    }
	//��ʼ���ڲ�ʹ�õ������������ݽṹ
	ret = zvrrp_cmd_init(gVrrpMatser);
	if(ret == ERROR)
	{
		free(gVrrpMatser);
		return ERROR;
	}
#ifndef ZVRRPD_ON_ROUTTING
	//���������������̵߳������ݽṹ
    gVrrpMatser->SemMutexId = zvrrpsemcreate(1);
    CHECK_VALID(gVrrpMatser->SemMutexId, ERROR);   
    gVrrpMatser->master = zvrrputilschedinit();
#else//ZVRRPD_ON_ROUTTING   
    if(m)
    	gVrrpMatser->master = m;
    else
    	gVrrpMatser->master = thread_master_create(); 
#endif// ZVRRPD_ON_ROUTTING 	
	
    if(gVrrpMatser->master == NULL)
    {
#ifndef ZVRRPD_ON_ROUTTING    	
    	zvrrpsemdelete(gVrrpMatser->SemMutexId);
#endif// ZVRRPD_ON_ROUTTING  
    	zvrrp_cmd_uninit(gVrrpMatser);
    	free(gVrrpMatser);
    	return ERROR;
    }

    vrrp_interface_init();

#ifdef ZVRRPD_ON_ROUTTING     
    //��ʼ���ͻ������ݽṹ
    ret = zvrrp_zclient_init(gVrrpMatser);
    if(ret == ERROR)
    {
#ifndef ZVRRPD_ON_ROUTTING
    	zvrrpsemdelete(gVrrpMatser->SemMutexId);
    	zvrrp_cmd_uninit(gVrrpMatser);
    	free(gVrrpMatser);
#else//ZVRRPD_ON_ROUTTING
    	zvrrp_cmd_uninit(gVrrpMatser);
    	thread_master_free ((struct thread_master *)gVrrpMatser->master);
    	free(gVrrpMatser);
#endif// ZVRRPD_ON_ROUTTING
    	return ERROR;
    }
#endif// ZVRRPD_ON_ROUTTING     
    
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
    gVrrpMatser->init = pri;
    gVrrpMatser->task = taskSpawn("zvrrp",
				pri,
                0,
                2048*10,
                (FUNCPTR)zvrrp_main, 
                gVrrpMatser, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)   
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX)
    gVrrpMatser->init = pri;
    //zvrrp_main(gVrrpMatser);
#endif// (ZVRRPD_OS_TYPE==ZVRRPD_ON_LINUX) 
    return OK;
}

int zvrrp_master_uninit(void)
{
	int i = 0;
	if(gVrrpMatser == NULL)
		return OK;
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
    	if(gVrrpMatser->gVrrp_vsrv[i].used)
    	{
#ifdef ZVRRPD_ON_ROUTTING
    		if(gVrrpMatser->gVrrp_vsrv[i].vif.zvrrp_read)
    			thread_cancel (gVrrpMatser->gVrrp_vsrv[i].vif.zvrrp_read);
    		if(gVrrpMatser->gVrrp_vsrv[i].vif.zvrrp_timer)
    			thread_cancel (gVrrpMatser->gVrrp_vsrv[i].vif.zvrrp_timer);
    		//if(gVrrpMatser->gVrrp_vsrv[i].vif.sock)
    		//	close(gVrrpMatser->gVrrp_vsrv[i].vif.sock);
#else
    		if(gVrrpMatser->SemMutexId)
    			zvrrpsemdelete(gVrrpMatser->SemMutexId);
    		zvrrputilschedcancel(gVrrpMatser->master, gVrrpMatser->master->sched_timer);
    		zvrrputilschedcancel(gVrrpMatser->master, gVrrpMatser->master->sched_read);
#endif
    		if(gVrrpMatser->gVrrp_vsrv[i].vif.sock)
    			close(gVrrpMatser->gVrrp_vsrv[i].vif.sock);
			zvrrp_vsrv_delete(&gVrrpMatser->gVrrp_vsrv[i]);
			zvrrp_vsrv_free(&gVrrpMatser->gVrrp_vsrv[i]);
    	}
    }

	zvrrp_cmd_uninit(gVrrpMatser);
	zvrrp_zclient_uninit(gVrrpMatser);

#ifdef ZVRRPD_ON_ROUTTING
#if (ZVRRPD_OS_TYPE==ZVRRPD_ON_VXWORKS)
	if(gVrrpMatser->task)
		taskDelete(gVrrpMatser->task);
#endif

	thread_master_free ((struct thread_master *)gVrrpMatser->master);

#else//ZVRRPD_ON_ROUTTING
	if(zvrrp->master)
		zvrrputilschedclean(zvrrp->master);
	if(zvrrp->SemMutexId)
		zvrrpsemdelete(zvrrp->SemMutexId);
#endif// ZVRRPD_ON_ROUTTING

	free(gVrrpMatser);
    return OK;
}



