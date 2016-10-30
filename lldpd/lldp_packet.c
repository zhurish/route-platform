/*
 * lldp_packet.c
 *
 *  Created on: Oct 25, 2016
 *      Author: zhurish
 */

#include <zebra.h>

#include <lib/version.h>
#include "thread.h"
#include "command.h"
#include "prefix.h"
#include "plist.h"
#include "if.h"
#include "stream.h"
#include "log.h"
#include "memory.h"

//#include <net/if_arp.h>
#include <linux/if_ether.h>
//#include <linux/if_packet.h>

#include "lldpd.h"
#include "lldp_db.h"
#include "lldp_interface.h"
#include "lldp_packet.h"
#include "lldp-socket.h"


static int lldp_constr_ltv (struct stream *s, int type, unsigned char * TLV, int TLVlen)
{
	char buf[512];
	unsigned char *value = (unsigned char *)(buf + sizeof(struct lldp_tlv));
	struct lldp_tlv *tlv = (struct lldp_tlv *)buf;
    if( ( type > 127 ) || ( ( TLVlen > 0 ) && ( NULL == TLV ) ) )
    {
        return -1;
    }
	LLDP_TLV_HDR(tlv, type, TLVlen);
	memcpy(value, TLV, TLVlen);
	stream_put(s, buf, TLVlen + sizeof(struct lldp_tlv));
	return (TLVlen + sizeof(struct lldp_tlv));
}

static int lldp_constr_exp_ltv (struct stream *s, int orgtype, int subtype, unsigned char * TLV, int TLVlen)
{
	char buf[512];
	unsigned char *value = (unsigned char *)(buf + sizeof(struct lldp_exp_tlv));
	struct lldp_exp_tlv *tlv = (struct lldp_exp_tlv *)buf;
    if(( ( TLVlen > 0 ) && ( NULL == TLV ) ) )
    {
        return -1;
    }
	LLDP_TLV_HDR(tlv, LLDP_ORG_TLV_ID, TLVlen+4);
	if(orgtype == LLDP_8021_ORG)
	{
		tlv->identifer[0] = 0x00;
		tlv->identifer[1] = 0x80;
		tlv->identifer[2] = 0xc2;
	}
	else if(orgtype == LLDP_8023_ORG)
	{
		tlv->identifer[0] = 0x00;
		tlv->identifer[1] = 0x12;
		tlv->identifer[2] = 0x0f;
	}
	tlv->subtype = subtype;

	memcpy(value, TLV, TLVlen);
	stream_put(s, buf, TLVlen + sizeof(struct lldp_exp_tlv));
	return (TLVlen + sizeof(struct lldp_exp_tlv));
}

static int lldp_constr_end_ltv(struct stream *s)
{
	char buf[64];
	struct lldp_tlv *tlv = (struct lldp_tlv *)buf;
	LLDP_TLV_HDR(tlv, LDDP_END, 0);
	stream_put(s, buf, sizeof(struct lldp_tlv));
	return (sizeof(struct lldp_tlv));
}

static int lldp_constr_management_address_ltv(struct lldp_interface *lifp, struct stream *s)
{
	int len = 0,offset = 0;
	char buf[LLDP_SUB_TLV_STR_MAX_LEN];
	struct lldp_mgt_address *mgt_address = (struct lldp_mgt_address *)buf;
	struct lldp_mgt_if *mgt_if;
	struct mgt_address mgt;

	lldp_ststem_mgt_address(&mgt);

	memset(buf, 0, LLDP_SUB_TLV_STR_MAX_LEN);
	mgt_address->addr_len = mgt.addr_len;
	mgt_address->subtype = mgt.subtype;

	if(mgt.addr_len)
		memcpy(mgt_address->value, &mgt.value, mgt.addr_len-1);

	offset = sizeof(mgt_address->addr_len) + mgt.addr_len;
	mgt_if = (struct lldp_mgt_if *)(buf + offset);

	mgt_if->ifsubtype = mgt.ifsubtype;
	mgt_if->idindex = (mgt.idindex);
	mgt_if->oidlen = mgt.oidlen;
	if(mgt.oidlen)
		memcpy(mgt_if->oid, &mgt.oid, mgt.oidlen);

	len = offset + sizeof(mgt_if->ifsubtype) + sizeof(mgt_if->idindex) +
			sizeof(mgt_if->oidlen) + mgt_if->oidlen;

	lldp_constr_ltv(lifp->obuf, LLDP_MGT_ADDR_ID, (unsigned char *)buf, len);/*系统管理地址*/
	return len;
}
static int lldp_constr_system_ltv(struct lldp_interface *lifp, struct stream *s)
{
	char *value = NULL;
	struct system_caability capability;
	value = lldp_ststem_name();
	if(value)
		lldp_constr_ltv(lifp->obuf, LLDP_SYSTEM_NAME_ID, (unsigned char *)value, strlen(value));/*系统名称*/
	value = lldp_ststem_description();
	if(value)
		lldp_constr_ltv(lifp->obuf, LLDP_SYSTEM_DESC_ID, (unsigned char *)value, strlen(value));/*系统描述*/

	lldp_system_caability(&capability);
	lldp_constr_ltv(lifp->obuf, LLDP_SYSTEM_CAPA_ID, (unsigned char *)&capability, sizeof(capability));

	lldp_constr_management_address_ltv(lifp, lifp->obuf);
	return 0;
}
static int lldp_constr_org_ltv(struct lldp_interface *lifp, struct stream *s)
{
	int len;
	struct tlv_vlan vlan;
	struct tlv_vlan_name v_name;
	struct tlv_vlan_proto v_proto;

	len = lldp_port_vlan_id(lifp->ifp, &vlan);
	lldp_constr_exp_ltv (lifp->obuf, LLDP_8021_ORG, LLDP_PVID_TYPE, (unsigned char *)&vlan, len);

	len = lldp_port_vlan_name(lifp->ifp, &v_name);
	lldp_constr_exp_ltv (lifp->obuf, LLDP_8021_ORG, LLDP_VLAN_NAME_TYPE, (unsigned char *)&v_name, len);

	len = lldp_port_vlan_protocol(lifp->ifp, &v_proto);
	lldp_constr_exp_ltv (lifp->obuf, LLDP_8021_ORG, LLDP_PROTOCOL_ID_TYPE, (unsigned char *)&v_proto, len);
	return 0;
}
static int lldp_make_ltv(struct interface *ifp)
{
	int len = 0;
	unsigned short hold_time;
	struct lldp_sub_head head;
	struct lldp_interface *lifp = (struct lldp_interface *)ifp->info;
	if(lifp == NULL)
		return -1;

	//memset(&head, 0, sizeof(head));
	//head.subtype = LLDP_SUB_MAC_ADDR_ID;
	//memcpy(head.value, lifp->own_mac, ETH_ALEN);
	//len = 1 + ETH_ALEN;
	len = lldp_chassis_id(ifp, (struct tlv_sub_head *)&head);
	lldp_constr_ltv (lifp->obuf, LLDP_CHASSIS_ID,  (unsigned char *)&head, len);

	//memset(&head, 0, sizeof(head));
	//head.subtype = LLDP_SUB_PORT_IF_NAME_ID;
	//sprintf((char * restrict)head.value,"%s",ifindex2ifname(ifp->ifindex));
	//len = LLDP_SUB_TLV_HDR(head);
	len = lldp_port_id(ifp, (struct tlv_sub_head *)&head);
	lldp_constr_ltv (lifp->obuf, LLDP_PORT_ID,  (unsigned char *)&head, len);

	hold_time = htons(lifp->lldp_holdtime);
	lldp_constr_ltv (lifp->obuf, LLDP_TTL_ID, (unsigned char *)&hold_time, sizeof(lifp->lldp_holdtime));

	if(ifp->desc)
		lldp_constr_ltv (lifp->obuf, LLDP_PORT_DESC_ID, (unsigned char *)ifp->desc, strlen(ifp->desc));
#if 0
	lldp_constr_ltv(lifp->obuf, LLDP_SYSTEM_CAPA_ID, (unsigned char *)&capability, sizeof(capability));
	lldp_constr_ltv(lifp->obuf, LLDP_SYSTEM_NAME_ID, (unsigned char *)lifp->lldpd->system_name, strlen(lifp->lldpd->system_name));/*系统名称*/
	lldp_constr_ltv(lifp->obuf, LLDP_SYSTEM_DESC_ID, (unsigned char *)lifp->lldpd->system_desc, strlen(lifp->lldpd->system_desc));/*系统描述*/
	lldp_constr_management_address_ltv(lifp, lifp->obuf);
#endif
	lldp_constr_system_ltv(lifp, lifp->obuf);

	lldp_constr_org_ltv(lifp, lifp->obuf);

	return 0;
}


int lldp_head_format (struct interface *ifp, struct stream *obuf)
{
	int len = 0;
	unsigned char l2_hdr[64];
	struct	ethhdr *eth_hdr = (struct ethhdr *)l2_hdr;
	struct lldp_interface *lifp = NULL;
	if(ifp ==  NULL || obuf == NULL)
		return -1;
	lifp = ifp->info;
	if(lifp ==  NULL)
		return -1;
	memcpy(eth_hdr->h_dest, lifp->dst_mac, ETH_ALEN);
	memcpy(eth_hdr->h_source, lifp->own_mac, ETH_ALEN);
	if(lifp->frame == SNAP_FRAME_TYPE)
	{
		char *lh = (char *)(l2_hdr + (2 * ETH_ALEN) );
		char snap_hdr[8] = LLDP_SNAP_FRAME_HEAD;
		memcpy(lh, snap_hdr, 8);
		len = 2 * ETH_ALEN + 8;
	}
	else //if(lifp->frame == LLDP_FRAME_TYPE)
	{
		eth_hdr->h_proto = htons(ETH_P_LLDP);
		len = sizeof(struct	ethhdr);
	}
	stream_put(obuf, l2_hdr, len);
	return len;
}

int lldp_make_lldp_pdu(struct interface *ifp)
{
	struct lldp_interface *lifp =  (struct lldp_interface *)ifp->info;
	stream_reset(lifp->obuf);
	lldp_head_format (ifp, lifp->obuf);
	lldp_make_ltv(ifp);
	lldp_constr_end_ltv(lifp->obuf);
	return 0;
}
