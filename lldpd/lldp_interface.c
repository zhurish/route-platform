/*
 * lldp_interface.c
 *
 *  Created on: Oct 24, 2016
 *      Author: zhurish
 */

#include <zebra.h>

#include "command.h"
#include "prefix.h"
#include "table.h"
#include "stream.h"
#include "memory.h"
#include "routemap.h"
#include "zclient.h"
#include "log.h"

#include "lldpd.h"
#include "lldp_db.h"
#include "lldp_interface.h"
#include "lldp_packet.h"
#include "lldp-socket.h"


unsigned char lldp_dst_mac[LLDP_DST_MAC_MAX][ETH_ALEN] =
{
	{0x01,0x80,0xC2,0x00,0x00,0x0E},
	{0x01,0x80,0xC2,0x00,0x00,0x03},
	{0x01,0x80,0xC2,0x00,0x00,0x00},
	{0x01,0x00,0x0C,0xCC,0xCC,0xCC},/* Cisco CDP */
};

static int lldp_interface_new_hook (struct interface *ifp)
{
  struct lldp_interface *lifp;
  if(ifp->info)
	  return CMD_SUCCESS;
  lifp = XMALLOC (MTYPE_IF, sizeof(struct lldp_interface));
  if(lifp)
	  memset(lifp, 0, sizeof(struct lldp_interface));
  else
	  return -1;

  ifp->info = lifp;
  if(lifp)
  {
	  lifp->mode = 0;//LLDP_WRITE_MODE;/* 使能状态 */
	  lifp->protocol = LLDP_CDP_TYPE;/* 兼容协议 */
	  lifp->states = 0;/* 接口状态 */
	  lifp->frame = LLDP_FRAME_TYPE;/* LLDP 帧封装格式 */
	  lifp->Changed = 0;/* 本地信息发生变动 */
	  lifp->capabilities = 1;/*  */

	  lifp->lldp_timer = LLDP_TIMER_DEFAULT;/* 定时时间 */
	  lifp->lldp_holdtime = LLDP_TTL_DEFAULT;/* 生存时间 */
	  lifp->lldp_reinit	 = 0;/* 重新初始化时间 */
	  lifp->lldp_fast_count	= 16;/* 快速发送计数 */
	  lifp->lldp_tlv_select	= 0xffffff;//TLV选择
	  lifp->lldp_check_interval = 5;//本地检测周期，检测本地信息是否发生变化


	  lifp->lldpd = lldpd_config;
	  memcpy(lifp->dst_mac, lldp_dst_mac[LLDP_DST_MAC1], ETH_ALEN);

	  //lifp->ibuf = stream_new (LLDP_PACKET_MAX_SIZE);
	  //lifp->obuf = stream_new (LLDP_PACKET_MAX_SIZE);
	  //if(memcmp(ifp->name, "eno16777736",strlen("eno16777736"))==0)
	  //lldp_interface_enable(ifp, LLDP_WRITE_MODE|LLDP_READ_MODE, LLDP_CDP_TYPE);

	  lifp->ifp = ifp;
	  return CMD_SUCCESS;
  }
  return -1;
}

/* Called when interface structure deleted. */
static int lldp_interface_delete_hook (struct interface *ifp)
{
  if(ifp->info)
  {
	  XFREE (MTYPE_IF, ifp->info);
  }
  ifp->info = NULL;
  return CMD_SUCCESS;
}

/***************************************************************************************/
/***************************************************************************************/
DEFUN (lldpd_admin_enable,
		lldpd_admin_enable_cmd,
	    "lldp admin enable",
		LLDP_STR
		"admin status\n"
		"enable lldp\n")
{
	return CMD_SUCCESS;
}
DEFUN (lldpd_enable,
		lldpd_enable_cmd,
	    "lldp enable",
		LLDP_STR
		"enable lldp\n")
{
	struct interface *ifp = NULL;
	ifp = vty->index;
	if(ifp)
		return lldp_interface_enable(ifp);
	return CMD_WARNING;
}
DEFUN (lldpd_receive,
		lldpd_receive_val_cmd,
	    "lldp receive (compliance) (cdp|edp|fdp|sonmp|lldp-med|custom)",
		LLDP_STR
		"lldpd receive enable\n"
		"lldpd compliance witch config\n"
		"cdp protocol enable\n"
		"edp protocol enable\n"
		"fdp protocol enable\n"
		"sonmp protocol enable\n"
		"lldp-med protocol enable\n"
		"custom protocol enable\n")
{
	struct interface *ifp = NULL;
	struct lldp_interface *lifp = NULL;
	ifp = vty->index;
	if(ifp)
		lifp = ifp->info;
	if(lifp == NULL)
		return CMD_WARNING;
	lifp->mode |= LLDP_READ_MODE;/* 使能状态 */
#if 0
	lifp->protocol = LLDP_CDP_TYPE;/* 兼容协议 */
	lifp->states = 0;/* 接口状态 */
	lifp->frame = LLDP_FRAME_TYPE;/* LLDP 帧封装格式 */
	lifp->Changed = 0;/* 本地信息发生变动 */
	lifp->write_timer = LLDP_TIMER_DEFAULT;/* 定时时间 */
	lifp->lldp_ttl = LLDP_TTL_DEFAULT;/* 报文TTL */
	lifp->capabilities = 1;/*  */
	lifp->lldpd = lldpd_config;
#endif
	return lldp_interface_enable_update(ifp);
}
ALIAS (lldpd_receive,
		lldpd_receive_cmd,
	   "lldp receive",
		LLDP_STR
		"lldpd receive enable\n")

DEFUN (lldpd_transmit,
		lldpd_transmit_val_cmd,
	    "lldp transmit (compliance) (cdp|edp|fdp|sonmp|lldp-med|custom)",
		LLDP_STR
		"lldpd transmit enable\n"
		"lldpd compliance witch config\n"
		"cdp protocol enable\n"
		"edp protocol enable\n"
		"fdp protocol enable\n"
		"sonmp protocol enable\n"
		"lldp-med protocol enable\n"
		"custom protocol enable\n")
{
	struct interface *ifp = NULL;
	struct lldp_interface *lifp = NULL;
	ifp = vty->index;
	if(ifp)
		lifp = ifp->info;
	if(lifp == NULL)
		return CMD_WARNING;
	lifp->mode |= LLDP_WRITE_MODE;/* 使能状态 */
#if 0
	lifp->protocol = LLDP_CDP_TYPE;/* 兼容协议 */
	lifp->states = 0;/* 接口状态 */
	lifp->frame = LLDP_FRAME_TYPE;/* LLDP 帧封装格式 */
	lifp->Changed = 0;/* 本地信息发生变动 */
	lifp->write_timer = LLDP_TIMER_DEFAULT;/* 定时时间 */
	lifp->lldp_ttl = LLDP_TTL_DEFAULT;/* 报文TTL */
	lifp->capabilities = 1;/*  */
	lifp->lldpd = lldpd_config;
#endif
	return lldp_interface_enable_update(ifp);
}
ALIAS (lldpd_transmit,
		lldpd_transmit_cmd,
	   "lldp transmit",
		LLDP_STR
		"lldpd transmit enable\n")

static int lldp_interface_config_write (struct vty *vty)
{
  char proto[32];
  struct listnode *node;
  struct interface *ifp;
  struct lldp_interface *lifp;

  for (ALL_LIST_ELEMENTS_RO (iflist, node, ifp))
    {
      vty_out (vty, "interface %s%s", ifp->name, VTY_NEWLINE);

      if (ifp->desc)
    	  vty_out (vty, " description %s%s", ifp->desc,VTY_NEWLINE);
      lifp = ifp->info;
      if(lifp == NULL)
    	  continue;
      switch(lifp->protocol)
      {
      case LLDP_CDP_TYPE:/* Cisco Discovery Protocol */
    	  sprintf(proto,"%s","cdp");
    	  break;
      case LLDP_EDP_TYPE:/* Extreme Discovery Protocol */
	  	  sprintf(proto,"%s","edp");
	  	  break;
      case LLDP_FDP_TYPE:/* Foundry Discovery Protocol */
	  	  sprintf(proto,"%s","fdp");
	  	  break;
      case LLDP_DOT1_TYPE:/* Dot1 extension (VLAN stuff) */
    	  sprintf(proto,"%s","dot1");
    	  break;
      case LLDP_DOT3_TYPE:/* Dot3 extension (PHY stuff) */
    	  sprintf(proto,"%s","dot3");
    	  break;
      case LLDP_SONMP_TYPE:/*  */
    	  sprintf(proto,"%s","sonmp");
    	  break;
      case LLDP_MED_TYPE:/* LLDP-MED extension */
    	  sprintf(proto,"%s","med");
    	  break;
      case LLDP_CUSTOM_TYPE:/* Custom TLV support */
    	  sprintf(proto,"%s","custom");
    	  break;
      }
      if(lifp->mode & LLDP_DISABLE)
    	  vty_out (vty, " lldp disable %s", VTY_NEWLINE);
      if(lifp->mode & LLDP_READ_MODE)
    	  vty_out (vty, " lldp receive protocol %s%s", proto,VTY_NEWLINE);
      if(lifp->mode & LLDP_WRITE_MODE)
    	  vty_out (vty, " lldp transmit protocol %s%s", proto,VTY_NEWLINE);
      if(lifp->mode & LLDP_MED_MODE)
    	  vty_out (vty, " lldp  med %s",VTY_NEWLINE);

      vty_out (vty, "!%s", VTY_NEWLINE);
    }
  return CMD_SUCCESS;
}

static struct cmd_node interface_node =
{
  INTERFACE_NODE,
  "%s(config-if)# ",
  1,
};

int lldp_interface_init(void)
{
	if_init();
	if_add_hook (IF_NEW_HOOK, lldp_interface_new_hook);
	if_add_hook (IF_DELETE_HOOK, lldp_interface_delete_hook);

	install_node (&interface_node, lldp_interface_config_write);

	install_default (INTERFACE_NODE);
	install_element (CONFIG_NODE, &interface_cmd);
	install_element (CONFIG_NODE, &no_interface_cmd);
	install_element (INTERFACE_NODE, &interface_desc_cmd);
	install_element (INTERFACE_NODE, &no_interface_desc_cmd);

	install_element (CONFIG_NODE, &lldpd_admin_enable_cmd);
	install_element (INTERFACE_NODE, &lldpd_enable_cmd);
	install_element (INTERFACE_NODE, &lldpd_transmit_cmd);
	install_element (INTERFACE_NODE, &lldpd_transmit_val_cmd);
	install_element (INTERFACE_NODE, &lldpd_receive_cmd);
	install_element (INTERFACE_NODE, &lldpd_receive_val_cmd);

	return 0;
}

