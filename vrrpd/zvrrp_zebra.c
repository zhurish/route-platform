#include "zvrrpd.h"
#include "zvrrp_sched.h"


#ifdef ZVRRPD_ON_ROUTTING
//����zebra��Ԫ����route id��Ϣ
static int zvrrp_router_id_update_zebra (int command, struct zclient *zclient,
			     zebra_size_t length)
{
  struct prefix router_id;
  zebra_router_id_update_read(zclient->ibuf,&router_id);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
/* Inteface addition message from zebra. //��ӽӿ���Ϣ */
static int zvrrp_interface_add (int command, struct zclient *zclient, zebra_size_t length)
{
  struct interface *ifp;
  ifp = zebra_interface_add_read (zclient->ibuf);
  if(ifp == NULL)
	  return 0;		
  zlog_debug ("Zebra: %s",__func__);
  return 0;
}
/****************************************************************************/
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
//�ӿ�ɾ��
static int zvrrp_interface_delete (int command, struct zclient *zclient,
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
	  zlog_debug ("Zebra: got delete of %s, but interface is still up",
               ifp->name);
  zlog_debug ("Zebra: %s",__func__);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
//�ӿ�״̬UP�¼�
static int zvrrp_interface_state_up (int command, struct zclient *zclient,
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
      
      return 0;
    }
  zebra_interface_if_set_value (zclient->ibuf, ifp);
  zlog_debug ("Zebra: %s",__func__);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
//�ӿ�down�¼�
static int zvrrp_interface_state_down (int command, struct zclient *zclient,
                           zebra_size_t length)
{
  struct interface *ifp;
//  struct route_node *node;
  ifp = zebra_interface_state_read (zclient->ibuf);
  if (ifp == NULL)
    return 0;
  zlog_debug ("Zebra: %s",__func__);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
//�ӿڵ�ַ�����¼�
static int zvrrp_interface_address_add (int command, struct zclient *zclient,
                            zebra_size_t length)
{
  //char buf[128];
  struct connected *c;
  //struct prefix p;
  c = zebra_interface_address_read (command, zclient->ibuf);
  if (c == NULL)
    return 0;
  zlog_debug ("Zebra: %s",__func__);
  return 0;
}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
//�ӿڵ�ַɾ���¼�
static int zvrrp_interface_address_delete (int command, struct zclient *zclient, zebra_size_t length)
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
			  ;//zebraEigrpCmdInterfaceAddress(1, EigrpMaster->asnum, ifindex, 0, 0, 0);
		  zlog_debug("Eigrp: can't faind interface address,so use to get interface index\n");
		  return 0;
	  }
	  ifp = c->ifp;
	  p = *c->address;
	  //p.prefixlen = IPV4_MAX_PREFIXLEN;
	  //if_rtflag_unset(&p, ZEBRA_ROUTE_VRRP);
	  zlog_debug ("Zebra: %s",__func__);
	  return 0;
}
/****************************************************************************/
/****************************************************************************/
static int zvrrp_router_vrrp(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_vrrp(1, atoi(argv[0]));
	if(ret == OK)
	{
		  vty->node = VRRP_NODE;
		  return CMD_SUCCESS;	
	}
	vty_out (vty, "%% create vrrp error%s", VTY_NEWLINE);
	return CMD_WARNING;
}
static int zvrrp_no_router_vrrp(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_vrrp(0, atoi(argv[0]));
	if(ret == OK)
	{
		  vty->node = CONFIG_NODE;
		  return CMD_SUCCESS;	
	}
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_enable_vrrp(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_enable(1, gVrrpMatser->opcode->vrid);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_enable_vrrp(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_enable(0, gVrrpMatser->opcode->vrid);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_ip(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_vip(1, gVrrpMatser->opcode->vrid, ntohl(inet_addr(argv[0])));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_ip(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_vip(0, gVrrpMatser->opcode->vrid, ntohl(inet_addr(argv[0])));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_interface(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_interface(1, gVrrpMatser->opcode->vrid, argv[0]);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_interface(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_interface(0, gVrrpMatser->opcode->vrid, argv[0]);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_adev_time(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_set_adev_time(1, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_adev_time(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_set_adev_time(0, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_adev_mtime(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_set_adev_mtime(1, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_adev_mtime(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_set_adev_mtime(0, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_priority(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_priority(1, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_priority(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}
	ret = zvrrp_cmd_priority(0, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_ping(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_ping(1);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_ping(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_ping(0);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_learn(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_learn_master(1, gVrrpMatser->opcode->vrid);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_learn(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_learn_master(0, gVrrpMatser->opcode->vrid);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_preempt(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_preempt(1, gVrrpMatser->opcode->vrid);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_preempt(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	ret = zvrrp_cmd_preempt(0, gVrrpMatser->opcode->vrid);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_preempt_delay(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}	
	ret = zvrrp_cmd_preempt_delay(1, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_preempt_delay(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}	
	ret = zvrrp_cmd_preempt_delay(0, gVrrpMatser->opcode->vrid, atoi(argv[0]));
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
static int zvrrp_vrrp_track(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc < 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}	
	if(argc == 2)
		ret = zvrrp_cmd_track(1, gVrrpMatser->opcode->vrid, 0, argv[0], atoi(argv[1]));
	else
		ret = zvrrp_cmd_track(1, gVrrpMatser->opcode->vrid, 0, argv[0], 0);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
static int zvrrp_no_vrrp_track(struct vty *vty, int argc, char *argv[])
{
	int ret = ERROR;
	if( (argc != 1)||(argv[0]==NULL))
	{
			vty_out (vty, "%% Invalid %s value %s",argv[0]? argv[0]:"argv is null", VTY_NEWLINE);
			return CMD_WARNING;
	}	
	ret = zvrrp_cmd_track(0, gVrrpMatser->opcode->vrid, 0, argv[0], 0);
	if(ret == OK)
		return CMD_SUCCESS;	
	return CMD_WARNING;
}
/****************************************************************************/
/************************************************************************************/
/****************************************************************************/
static int zvrrp_vty_show_one(struct vty *vty, vrrp_rt * vsrv)
{
	int i = 0;
	char vmac[32];
	struct in_addr ip_src;
	extern const char * zvrrpOperStateStr[];
	extern const char * zvrrpOperAdminStateStr[];
	
    if( (vsrv == NULL) )
    	return ERROR;
    vty_out (vty, "VRID <%d> %s%s",vsrv->vrid, zvrrpOperAdminStateStr[vsrv->adminState], VTY_NEWLINE);
    if(vsrv->vif.ifp)
    	vty_out (vty, " State		:%s(Interface %s)%s", zvrrpOperStateStr[vsrv->state], if_is_up(vsrv->vif.ifp)? "UP":"DOWN", VTY_NEWLINE);
    else
    	vty_out (vty, " State		:%s(Interface Unknown)%s", zvrrpOperStateStr[vsrv->state], VTY_NEWLINE);
    for(i = 0; i < vsrv->naddr; i++)
    {
    	ip_src.s_addr = htonl(vsrv->vaddr[i].addr);
    	vty_out (vty, " Virtual-ip	:%s %s%s",inet_ntoa (ip_src),vsrv->nowner? "(IP owner)":"", VTY_NEWLINE);
    }
    if(vsrv->vif.ifp)
    	vty_out (vty, " Interface	:%s%s",vsrv->vif.ifp->name, VTY_NEWLINE); 
   
    memset(vmac, 0 , sizeof(vmac));
    sprintf(vmac, "%02x-%02x-%02x-%02x-%02x-%02x",(int)(unsigned char)vsrv->vhwaddr[0],
                                                  (int)(unsigned char)vsrv->vhwaddr[1],
                                                  (int)(unsigned char)vsrv->vhwaddr[2],
                                                  (int)(unsigned char)vsrv->vhwaddr[3],
                                                  (int)(unsigned char)vsrv->vhwaddr[4],
                                                  (int)(unsigned char)vsrv->vhwaddr[5]);
    vty_out (vty, " VMAC		:%s%s",vmac, VTY_NEWLINE);   
    vty_out (vty, " Advt timer	:%d second(s)%s",vsrv->adver_int, VTY_NEWLINE);
    vty_out (vty, " Priority	:%d%s",vsrv->priority, VTY_NEWLINE);
    vty_out (vty, " Preempt	:%s%s",vsrv->preempt? "TRUE":"FALSE", VTY_NEWLINE); 
    vty_out (vty, " Preempt delay	:%d second(s)%s",vsrv->delay, VTY_NEWLINE);    
    vty_out (vty, " ping		:%s%s",gVrrpMatser->ping_enable? "enable":"disable", VTY_NEWLINE); 
    
    ip_src.s_addr = htonl(vsrv->ms_router_id);
    vty_out (vty, " Master router id	:%s%s",vsrv->ms_router_id? inet_ntoa (ip_src):"Unknown", VTY_NEWLINE);
    if(vsrv->ms_priority == 0)
    {
    	vty_out (vty, " Master Priority	:Unknown%s", VTY_NEWLINE);
    }
    else
    {
    	vty_out (vty, " Master Priority	:%d%s",vsrv->ms_priority, VTY_NEWLINE);
    }
    if(vsrv->ms_advt_timer == 0)
    {
    	vty_out (vty, " Master advt times	:Unknown%s", VTY_NEWLINE);
    }
    else
    {
    	vty_out (vty, " Master advt times	:%d%s",vsrv->ms_advt_timer, VTY_NEWLINE);
    }
    if(vsrv->ms_down_timer == 0)
    {
    	vty_out (vty, " Master down timer	:Unknown%s", VTY_NEWLINE);
    }
    else
    {
    	vty_out (vty, " Master down timer	:%d%s",vsrv->ms_down_timer, VTY_NEWLINE);
    }
    vty_out (vty, " Learn master mode	:%s%s",vsrv->ms_learn? "TRUE":"FALSE", VTY_NEWLINE);    
    return OK;                                           
}
/************************************************************************************/
/****************************************************************************/
static int zvrrp_vrrp_show(struct vty *vty, int argc, char *argv[])
{
    int      i;
    int vrid = 0;
    vrrp_rt *pVsrv = NULL;
	if( (argc == 1)&&(argv[0])&&(atoi(argv[0])>0) )
		vrid = atoi(argv[0]);
    if(vrid != 0)
    {
    	pVsrv = zvrrp_vsrv_lookup(vrid);
    	if ( (pVsrv)&&(pVsrv->used == 1) )
    	{
    		zvrrp_vty_show_one(vty, pVsrv);
    	}
    	else
    		vty_out (vty,"VRID <%d> is not configure%s",vrid,VTY_NEWLINE);
    	return CMD_SUCCESS;
    }
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
        pVsrv = zvrrp_vsrv_lookup(i);

        if ( (pVsrv)&&(pVsrv->used == 1) )
        {
        	zvrrp_vty_show_one(vty, pVsrv);
        }
    } 	
	return CMD_SUCCESS;
}
/****************************************************************************/
/****************************************************************************/
//���ƽ̨��ֱ�Ӱ�װ�����������
/****************************************************************************/
/****************************************************************************/
DEFUN (vrrp_router,
	vrrp_router_cmd,
	"router vrrp <1-254>",
    "Enable a routing process\n"
    "vrrp routing protocol\n"
	"Autonomous system number config\n")
{
	//vty->node = VRRP_NODE;
	return	zvrrp_router_vrrp(vty, argc, argv);
}
DEFUN (vrrp_no_router,
	vrrp_no_router_cmd,
	"no router vrrp <1-254>",
	NO_STR
    "Enable a routing process\n"
    "vrrp routing protocol\n"
	"Autonomous system number config\n")
{
	//vty->node = VRRP_NODE;
	return	zvrrp_no_router_vrrp(vty, argc, argv);
}
DEFUN (vrrp_enable,
	vrrp_enable_cmd,
	"enable",
    "Enable a routing process\n")
{
	return	zvrrp_enable_vrrp(vty, argc, argv);
}
DEFUN (vrrp_no_enable,
	vrrp_no_enable_cmd,
	"no enable",
	NO_STR
    "Enable a routing process\n")
{
	return	zvrrp_no_enable_vrrp(vty, argc, argv);
}
DEFUN (vrrp_add_ip,
	vrrp_add_ip_cmd,
	"virtual-ip A.B.C.D",
	"Virtual IP information\n"
	"Specify by IPv4 address(e.g. 0.0.0.0)\n")
{
	return	zvrrp_vrrp_ip(vty, argc, argv);
}
DEFUN (vrrp_no_add_ip,
	vrrp_no_add_ip_cmd,
	"no virtual-ip A.B.C.D",
	NO_STR
	"Virtual IP information\n"
	"Specify by IPv4 address(e.g. 0.0.0.0)\n")
{
	return	zvrrp_no_vrrp_ip(vty, argc, argv);
}
DEFUN (vrrp_add_interface,
	vrrp_interface_cmd,
	"interface IFNAME",
	INTERFACE_STR
	IFNAME_STR)
{
	return	zvrrp_vrrp_interface(vty, argc, argv);
}
DEFUN (vrrp_no_interface,
	vrrp_no_interface_cmd,
	"no interface IFNAME",
	NO_STR
	INTERFACE_STR
	IFNAME_STR)
{
	return	zvrrp_no_vrrp_interface(vty, argc, argv);
}
DEFUN (vrrp_adev_time,
	vrrp_adev_time_cmd,
	"advertisement-interval <1-255>",
	"Minimum interval between sending VRRP advertisement\n"
	"time in seconds\n")
{
	return	zvrrp_vrrp_adev_time(vty, argc, argv);
}
DEFUN (vrrp_no_adev_time,
	vrrp_no_adev_time_cmd,
	"no advertisement-interval",
	NO_STR
	"Minimum interval between sending VRRP advertisement\n")
{
	return	zvrrp_no_vrrp_adev_time(vty, argc, argv);
}
DEFUN (vrrp_adev_mtime,
	vrrp_adev_mtime_cmd,
	"advertisement-interval msec <1-255>",
	"Minimum interval between sending VRRP advertisement\n"
	"advertisement time in msec\n"
	"time in msec seconds\n")
{
	return	zvrrp_vrrp_adev_mtime(vty, argc, argv);
}
DEFUN (vrrp_no_adev_mtime,
	vrrp_no_adev_mtime_cmd,
	"no advertisement-interval msec",
	NO_STR
	"Minimum interval between sending VRRP advertisement\n"
	"advertisement time in msec\n")
{
	return	zvrrp_no_vrrp_adev_mtime(vty, argc, argv);
}

DEFUN (vrrp_priority,
	vrrp_priority_cmd,
	"priority <1-255>",
	"priority\n"
	"priority value\n")
{
	return	zvrrp_vrrp_priority(vty, argc, argv);
}
DEFUN (vrrp_no_priority,
	vrrp_no_priority_cmd,
	"no priority",
	NO_STR
	"priority\n")
{
	return	zvrrp_no_vrrp_priority(vty, argc, argv);
}

DEFUN (vrrp_ping,
	vrrp_ping_cmd,
	"ping-enable",
	"enable ping sevice\n")
{
	return	zvrrp_vrrp_ping(vty, argc, argv);
}
DEFUN (vrrp_no_ping,
	vrrp_no_ping_cmd,
	"no ping-enable",
	NO_STR
	"enable ping sevice\n")
{
	return	zvrrp_no_vrrp_ping(vty, argc, argv);
}
DEFUN (vrrp_learn,
	vrrp_learn_cmd,
	"learnmaster-mode (true|false)",
	"enable learn master param mode\n")
{
	if(argv[0])
	{
		if(memcmp(argv[0], "tr", 2)==0)
			return	zvrrp_vrrp_learn(vty, argc, argv);
		else if(memcmp(argv[0], "fa", 2)==0)
			return	zvrrp_no_vrrp_learn(vty, argc, argv);
	}	
	return CMD_WARNING;
}

DEFUN (vrrp_preempt,
	vrrp_preempt_cmd,
	"preempt-mode (true|false)",
	"enable preempt mode\n")
{
	if(argv[0])
	{
		if(memcmp(argv[0], "tr", 2)==0)
			return	zvrrp_vrrp_preempt(vty, argc, argv);
		else if(memcmp(argv[0], "fa", 2)==0)
			return	zvrrp_no_vrrp_preempt(vty, argc, argv);
	}
	return CMD_WARNING;
}

DEFUN (vrrp_preempt_delay,
	vrrp_preempt_delay_cmd,
	"preempt-delay <1-255>",
	"preempt mode\n"
	"preempt delay time\n")
{
	return	zvrrp_vrrp_preempt_delay(vty, argc, argv);
}
DEFUN (vrrp_no_preempt_delay,
	vrrp_no_preempt_delay_cmd,
	"no preempt-delay",
	NO_STR
	"preempt mode\n")
{
	return	zvrrp_no_vrrp_preempt_delay(vty, argc, argv);
}
//vrrp 2 track eth0 20
DEFUN (vrrp_track_interface,
	vrrp_track_interface_cmd,
	"track interface IFNAME",
	"track mode\n"
	INTERFACE_STR
	IFNAME_STR)
{
	return	zvrrp_vrrp_track(vty, argc, argv);
}

ALIAS (vrrp_track_interface,
		vrrp_track_interface_decrement_cmd,
		"track interface IFNAME decrement <1-255>",
		"track mode\n"
		INTERFACE_STR
		IFNAME_STR
		"decrement\n"
		"priority value\n")

DEFUN (vrrp_no_track_interface,
	vrrp_no_track_interface_cmd,
	"no track interface IFNAME",
	NO_STR
	"track mode\n"	
	INTERFACE_STR
	IFNAME_STR)
{
	return	zvrrp_no_vrrp_track(vty, argc, argv);
}

DEFUN (vrrp_show,
	vrrp_show_val_cmd,
	"show ip vrrp <1-255>",
	SHOW_STR
	IP_STR
    "vrrp routing protocol\n"
	"Autonomous system number config\n")
{
	return	zvrrp_vrrp_show(vty, argc, argv);
}
ALIAS (vrrp_show,
	vrrp_show_cmd,
	"show ip vrrp",
	SHOW_STR
	IP_STR
    "vrrp routing protocol\n")
/****************************************************************************/
/************************************************************************************/
static int zvrrp_write_config_one(vrrp_rt * vsrv, struct vty *vty)
{
	int i = 0;
	struct in_addr ip_src;
    if( (vsrv == NULL) )
    	return ERROR;
    vty_out (vty, "router vrrp %d%s", vsrv->vrid,VTY_NEWLINE);
    if(vsrv->enable == 1)
     vty_out (vty, " enable%s",VTY_NEWLINE);
    if(vsrv->priority != VRRP_PRIO_DFL)
     vty_out (vty, " priority %d%s",vsrv->priority,VTY_NEWLINE);
    for(i = 0; i < vsrv->naddr; i++)
    {
    	ip_src.s_addr = htonl(vsrv->vaddr[i].addr);
    	vty_out (vty, " virtual-ip %s%s",inet_ntoa (ip_src),VTY_NEWLINE);
    }
    if(vsrv->adver_int != (VRRP_ADVER_DFL*VRRP_TIMER_HZ))
    	vty_out (vty, " advertisement-interval %d%s",vsrv->adver_int,VTY_NEWLINE);
    if(vsrv->preempt == VRRP_PREEMPT_DFL)
    	vty_out (vty, " preempt-mode true%s",VTY_NEWLINE);
    else
    	vty_out (vty, " preempt-mode false%s",VTY_NEWLINE);
    if(vsrv->ms_learn == VRRP_PREEMPT_DFL)
    	vty_out (vty, " leanmaster-mode true%s",VTY_NEWLINE);
    else
    	vty_out (vty, " leanmaster-mode false%s",VTY_NEWLINE);
    
    if(vsrv->vif.ifp)
    	vty_out (vty, " interface %s%s",vsrv->vif.ifp->name, VTY_NEWLINE);    
    //ip_src.s_addr = htonl(vsrv->vif.ipaddr);
	//ZVRRP_SHOW(" networks %s",inet_ntoa (ip_src));
    for(i = 0; i < vsrv->niftrack; i++)
    {
        if(vsrv->pritrack[i] == VRRP_PRI_TRACK)
        	vty_out (vty, " track interface %s%s",ifindex2ifname(vsrv->iftrack[i]), VTY_NEWLINE);
        else
        	vty_out (vty, " track interface %s decrement %d %s",ifindex2ifname(vsrv->iftrack[i]), vsrv->pritrack[i], VTY_NEWLINE);
    }
    return OK;                                           
}
/************************************************************************************/
static int zvrrp_config_write(struct vty *vty)
{
    int      i;
    vrrp_rt *pVsrv = NULL;
    for (i = 0; i < VRRP_VSRV_SIZE_MAX; i++)
    {
        pVsrv = zvrrp_vsrv_lookup(i);

        if ( (pVsrv)&&(pVsrv->used == 1) )
        {
        	zvrrp_write_config_one(pVsrv, vty);
        }
    }        
    return 1;
}
/****************************************************************************/
static struct cmd_node vrrp_node =
{
  VRRP_NODE,
  "%s(config-router)# ",
  1
};
static int zvrrp_vtycmd_init(void)
{
	//��quaggaע����ʾ�ӿ���Ϣ�ĺ�����show run�����ʱ����ʾ�ӿ���Ϣ��
	//zebra_node_func_install(0, INTERFACE_NODE, eigrp_config_interface_breif);
	//��quaggaע����ʾdebug��Ϣ�ĺ�����show run�����ʱ����ʾdebug��Ϣ��
	//zebra_node_func_install(0, DEBUG_NODE, eigrp_config_debug_breif); 
	install_node (&vrrp_node, zvrrp_config_write);
	/* "router vrrp" commands. */
	install_element (CONFIG_NODE, &vrrp_router_cmd);
	install_element (CONFIG_NODE, &vrrp_no_router_cmd);
	install_default (VRRP_NODE);
	install_element (VRRP_NODE, &vrrp_enable_cmd);
	install_element (VRRP_NODE, &vrrp_no_enable_cmd);  
	install_element (VRRP_NODE, &vrrp_add_ip_cmd);
	install_element (VRRP_NODE, &vrrp_no_add_ip_cmd);
	install_element (VRRP_NODE, &vrrp_interface_cmd);
	install_element (VRRP_NODE, &vrrp_no_interface_cmd);
	install_element (VRRP_NODE, &vrrp_adev_time_cmd);
	install_element (VRRP_NODE, &vrrp_no_adev_time_cmd);  
	install_element (VRRP_NODE, &vrrp_adev_mtime_cmd);
	install_element (VRRP_NODE, &vrrp_no_adev_mtime_cmd);
	install_element (VRRP_NODE, &vrrp_priority_cmd);
	install_element (VRRP_NODE, &vrrp_no_priority_cmd);
	install_element (VRRP_NODE, &vrrp_ping_cmd);
	install_element (VRRP_NODE, &vrrp_no_ping_cmd);  
	install_element (VRRP_NODE, &vrrp_learn_cmd);
	install_element (VRRP_NODE, &vrrp_preempt_cmd);

	install_element (VRRP_NODE, &vrrp_preempt_delay_cmd);
	install_element (VRRP_NODE, &vrrp_no_preempt_delay_cmd);  
	install_element (VRRP_NODE, &vrrp_track_interface_cmd);
	install_element (VRRP_NODE, &vrrp_track_interface_decrement_cmd);
	install_element (VRRP_NODE, &vrrp_no_track_interface_cmd);
	install_element (ENABLE_NODE, &vrrp_show_cmd);
	install_element (ENABLE_NODE, &vrrp_show_val_cmd);	
	return OK;
}
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/****************************************************************************/
/****************************************************************************/
int zvrrp_zclient_init(struct zvrrp_master * zvrrp)
{
	if(zvrrp == NULL)
		return ERROR;
	zvrrp->zclient = zclient_new ();
	zvrrp->zclient->router_id_update = zvrrp_router_id_update_zebra;
	zvrrp->zclient->interface_add = zvrrp_interface_add;
	zvrrp->zclient->interface_delete = zvrrp_interface_delete;
	zvrrp->zclient->interface_up = zvrrp_interface_state_up;
	zvrrp->zclient->interface_down = zvrrp_interface_state_down;
	zvrrp->zclient->interface_address_add = zvrrp_interface_address_add;
	zvrrp->zclient->interface_address_delete = zvrrp_interface_address_delete;
	zvrrp->zclient->ipv4_route_add = NULL;
	zvrrp->zclient->ipv4_route_delete = NULL;
#if (ZVRRPD_OS_TYPE == ZVRRPD_ON_LINUX)
	//sleep(1);
	zclient_init (gVrrpMatser->zclient, ZEBRA_ROUTE_VRRP);
	//EigrpMaster->eigrpvty.type = VTY_TERM;
	//EigrpMaster->eigrpvty.obuf = EigrpMaster->zclient->vtyobuf;	
#endif//(ZVRRPD_OS_TYPE == ZVRRPD_ON_LINUX)
	zvrrp_vtycmd_init();
	return OK;
}
/****************************************************************************/
int zvrrp_zclient_uninit(struct zvrrp_master * zvrrp)
{
	if( (zvrrp == NULL)||(zvrrp->zclient == NULL) )
		return ERROR;
	zclient_stop (zvrrp->zclient);
	zclient_free (zvrrp->zclient);
	return OK;
}
/****************************************************************************/
/****************************************************************************/
#endif// ZVRRPD_ON_ROUTTING
