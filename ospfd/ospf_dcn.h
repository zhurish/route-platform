/*
 * This is an implementation of draft-katz-yeung-ospf-traffic-06.txt
 * Copyright (C) 2001 KDD R&D Laboratories, Inc.
 * http://www.kddlabs.co.jp/
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 * 
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _ZEBRA_OSPF_DCN_H
#define _ZEBRA_OSPF_DCN_H



/*
 * Following section defines TLV body parts.
 */

#define CPE_MANULFACTORY  "JIUBO"  //"GlobalTechnology Co., Ltd"
//#define CPE_DEVICEMODEL   "GT-MSAP-PTAP240-SC320"
#define OSPF_DCN_LINK_IF_NAME	ospf_dcn_link_name() ? ospf_dcn_link_name():"dtl0"


//#define GT_DCN_DEBUG
//#define OSPF_DCN_ROUTE_ID//if router id is useing in DCN module mey be defined this

#define	OSPF_ADDR_LOOPBACK (oxe0001)

#define OSPF_LEGAL_DCN_INSTANCE (0X00FFEE00)
#define	MAX_LEGAL_DCN_INSTANCE_NUM (0xffff)


/*
 * Opaque LSA's link state ID for Traffic Engineering is
 * structured as follows.
 *
 *        24       16        8        0
 * +--------+--------+--------+--------+
 * |    1   |  MBZ   |........|........|
 * +--------+--------+--------+--------+
 * |<-Type->|<Resv'd>|<-- Instance --->|
 *
 *
 * Type:      IANA has assigned '1' for Traffic Engineering.
 * MBZ:       Reserved, must be set to zero.
 * Instance:  User may select an arbitrary 16-bit value.
 *
 */
/*
 *        24       16        8        0
 * +--------+--------+--------+--------+ ---
 * |   LS age        |Options |   10   |  A
 * +--------+--------+--------+--------+  |
 * |    1   |   0    |    Instance     |  |
 * +--------+--------+--------+--------+  |
 * |        Advertising router         |  |  Standard (Opaque) LSA header;
 * +--------+--------+--------+--------+  |  Only type-10 is used.
 * |        LS sequence number         |  |
 * +--------+--------+--------+--------+  |
 * |   LS checksum   |     Length      |  V
 * +--------+--------+--------+--------+ ---
 * |      Type       |     Length      |  A
 * +--------+--------+--------+--------+  |  TLV part for TE; Values might be
 * |              Values ...           |  V  structured as a set of sub-TLVs.
 * +--------+--------+--------+--------+ ---
 */
 
/*
 * Following section defines TLV (tag, length, value) structures,
 * used for Traffic Engineering.
 */
struct dcn_tlv_header
{
  u_int16_t	type;			/* DCN_TLV_XXX (see below) */
  u_int16_t	length;			/* Value portion only, in octets */
};

#define DCN_TLV_HDR_SIZE		(sizeof (struct dcn_tlv_header))

#define DCN_TLV_BODY_SIZE(tlvh)		(ROUNDUP (ntohs ((tlvh)->length), sizeof (u_int32_t)))

#define DCN_TLV_SIZE(tlvh)		(DCN_TLV_HDR_SIZE + DCN_TLV_BODY_SIZE(tlvh))

#define DCN_TLV_HDR_TOP(lsah)	(struct dcn_tlv_header *)((char *)(lsah) + OSPF_LSA_HEADER_SIZE)

#define DCN_TLV_HDR_NEXT(tlvh)	(struct dcn_tlv_header *)((char *)(tlvh) + DCN_TLV_SIZE(tlvh))


/*
 * Following section defines TLV body parts.
 */
#ifdef OSPF_DCN_ROUTE_ID
/* Router Address TLV *//* Mandatory */
#define	DCN_TLV_ROUTER_ADDR		1
struct dcn_tlv_router_addr
{
  struct dcn_tlv_header	header;		/* Value length is 4 octets. */
  struct in_addr	value;
};
#endif /*OSPF_DCN_ROUTE_ID*/

/* Link TLV */
#define	DCN_TLV_LINK			2
struct dcn_tlv_link
{
  struct dcn_tlv_header	header;
  /* A set of link-sub-TLVs will follow. */
};

/* Link Type Sub-TLV */
/* ManulFactory */
#define	DCN_LINK_SUBTLV_MANUL		    0x8000
#define MAX_MANUL_LENGTH            4*8
struct dcn_link_subtlv_manu
{
  struct dcn_tlv_header	header;		    /* Value length is 4*16 octets. */
  u_char 	value[MAX_MANUL_LENGTH];		/* string */
};

/* Link Sub-TLV: DEVMODEL */
#define	DCN_LINK_SUBTLV_DEVMODEL 		0x8001
#define MAX_MODEL_LENGTH            4*8
struct dcn_link_subtlv_devmodel
{
  struct dcn_tlv_header	header;		    /* Value length is 4*8 octets. */
  u_char 	value[MAX_MODEL_LENGTH];		/* string */
};


#define MAX_IP_STRING_LENGTH        16

/* Link Sub-TLV: MAC */
#define	DCN_LINK_SUBTLV_MAC 		    0x8002
#define MAX_MAC_LENGTH              6
struct dcn_link_subtlv_mac
{
  struct dcn_tlv_header	header;		    /* Value length is 4*8 octets. */
  struct {
      u_char 	value[MAX_MAC_LENGTH];	/* string */
      u_char	padding[2];
  } mac;
};

/* Link Sub-TLV: NEID *//* Optional */
#define	DCN_LINK_SUBTLV_NEID		    0x8003
struct dcn_link_subtlv_neid
{
  struct dcn_tlv_header	header;		/* Value length is 4  octets. */
  struct in_addr	value[1];      	/* Local NEID . */
};

/* Link Sub-TLV: Local Interface IP Address *//* Optional */
#define	DCN_LINK_SUBTLV_IPADDR		  0x8004
struct dcn_link_subtlv_ipaddr
{
  struct dcn_tlv_header	header;		/* Value length is 4  octets. */
  struct in_addr	value[1];	      /* Local IP address(es). */
};

#ifdef HAVE_IPV6
/* Link Sub-TLV: NE IPV6 *//* Optional */
#define	DCN_LINK_SUBTLV_IPADDRV6		0x8005
struct dcn_link_subtlv_ipaddrv6
{
  struct dcn_tlv_header	header;		/* Value length is 4 x 2 octets. */
  struct in6_addr	value[1];	      /* Local IP address(es). */
};
#endif /* HAVE_IPV6 */

/* Here are "non-official" architechtual constants. */
#define DCN_MINIMUM_BANDWIDTH	1.0	/* Reasonable? *//* XXX */








struct ospf_dcn
{
  enum status{disabled = 0, enabled = 1} status;

  /* List elements are zebra-interfaces (ifp), not ospf-interfaces (oi). */
  struct list *iflist;
#ifdef OSPF_DCN_ROUTE_ID
  /* Store Router-TLV in network byte order. */
  struct dcn_tlv_router_addr router_addr;
#endif /*OSPF_DCN_ROUTE_ID*/  
};

struct dcn_link
{
  /*
   * According to DCN (draft) specification, 24-bit Opaque-ID field
   * is subdivided into 8-bit "unused" field and 16-bit "instance" field.
   * In this implementation, each Link-TLV has its own instance.
   */
  u_int32_t instance;

  /* Reference pointer to a Zebra-interface. */
  struct interface *ifp;

  /* Area info in which this DCN link belongs to. */
  struct ospf_area *area;
  struct in_addr ip_address;
  /* Flags to manage this link parameters. */

  int install_lsa;//标识是否已经安装本的LSA
  int isInitialed;  /*标识接口是否已经初始化完毕*/
  int enable;  /*标识接口是否启动DCN功能*/
  
  /* Store Link-TLV in network byte order. */
  struct dcn_tlv_link link_header;
  struct dcn_link_subtlv_manu lv_manul;
  struct dcn_link_subtlv_devmodel lv_devmodel;
  struct dcn_link_subtlv_mac lv_mac;
  struct dcn_link_subtlv_neid lv_neid;
  struct dcn_link_subtlv_ipaddr lv_ipaddr;
#ifdef HAVE_IPV6  
  struct dcn_link_subtlv_ipaddrv6 lv_ipaddrv6;
#endif /* HAVE_IPV6 */  
};

/* 保存连接端MAC和IP地址信息 */
struct dcn_link_nbr
{
#define OSPF_DCN_NBR_MAC_SIZE	(64)	
	char nbrMac[OSPF_DCN_NBR_MAC_SIZE];
	struct in_addr nbrIp;	
	int initialed;
};

extern int ospf_dcn_debug;

#define IS_OSPF_DCN_EVENT_DEBUG	(ospf_dcn_debug & 0x01)
#define IS_OSPF_DCN_LSA_DEBUG	(ospf_dcn_debug & 0x02)
#define IS_OSPF_DCN_LSA_D_DEBUG	(ospf_dcn_debug & 0x04)
#define IS_OSPF_DCN_LINK_DEBUG	(ospf_dcn_debug & 0x08)



extern int ospf_dcn_init (void);
extern void ospf_dcn_term (void);



#ifdef GT_DCN_DEBUG
#define GT_ZLOG_DEBUG	printf
#define GT_DEBUG	printf
#define GT_TLV_DEBUG	printf
#else
#define GT_ZLOG_DEBUG(__fmt__, __args__...)
#define GT_DEBUG	printf
#define GT_TLV_DEBUG(__fmt__, __args__...)
#endif



#endif /* _ZEBRA_OSPF_DCN_H */
