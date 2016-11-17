/*
 * imi-sh_client.c
 *
 *  Created on: Nov 13, 2016
 *      Author: zhurish
 */

#include <zebra.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <lib/version.h>
#include "command.h"
#include "memory.h"
#include "thread.h"
#include "log.h"
#include "vty.h"
#include "bgpd/bgp_vty.h"

#ifdef IMISH_IMI_MODULE

#include "imi-cli/imi-sh.h"

/* BABELD */
DEFUNSH (IMISH_BABELD,
	 router_babeld,
	 router_babeld_cmd,
	 "router babel",
	 ROUTER_STR
	 "Babel")
{
  vty->node = BABEL_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BABELD,
	 imish_exit_babeld,
	 imish_exit_babeld_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_babeld,
       imish_quit_babeld_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

/* BGP */
DEFUNSH (IMISH_BGPD,
	 router_bgp,
	 router_bgp_cmd,
	 "router bgp " CMD_AS_RANGE,
	 ROUTER_STR
	 BGP_STR
	 AS_STR)
{
  vty->node = BGP_NODE;
  return CMD_SUCCESS;
}

ALIAS_SH (IMISH_BGPD,
	  router_bgp,
	  router_bgp_view_cmd,
	  "router bgp " CMD_AS_RANGE " view WORD",
	  ROUTER_STR
	  BGP_STR
	  AS_STR
	  "BGP view\n"
	  "view name\n")

DEFUNSH (IMISH_BGPD,
	 address_family_vpnv4,
	 address_family_vpnv4_cmd,
	 "address-family vpnv4",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_VPNV4_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_vpnv4_unicast,
	 address_family_vpnv4_unicast_cmd,
	 "address-family vpnv4 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_VPNV4_NODE;
  return CMD_SUCCESS;
}
/*
DEFUNSH (IMISH_BGPD,
	 address_family_vpnv6,
	 address_family_vpnv6_cmd,
	 "address-family vpnv6",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_VPNV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_vpnv6_unicast,
	 address_family_vpnv6_unicast_cmd,
	 "address-family vpnv6 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_VPNV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_encap,
	 address_family_encap_cmd,
	 "address-family encap",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_ENCAP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_encapv4,
	 address_family_encapv4_cmd,
	 "address-family encapv4",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_ENCAP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_encapv6,
	 address_family_encapv6_cmd,
	 "address-family encapv6",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_ENCAPV6_NODE;
  return CMD_SUCCESS;
}
*/
DEFUNSH (IMISH_BGPD,
	 address_family_ipv4_unicast,
	 address_family_ipv4_unicast_cmd,
	 "address-family ipv4 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV4_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv4_multicast,
	 address_family_ipv4_multicast_cmd,
	 "address-family ipv4 multicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV4M_NODE;
  return CMD_SUCCESS;
}
#ifdef HAVE_IPV6
DEFUNSH (IMISH_BGPD,
	 address_family_ipv6,
	 address_family_ipv6_cmd,
	 "address-family ipv6",
	 "Enter Address Family command mode\n"
	 "Address family\n")
{
  vty->node = BGP_IPV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv6_unicast,
	 address_family_ipv6_unicast_cmd,
	 "address-family ipv6 unicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV6_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_BGPD,
	 address_family_ipv6_multicast,
	 address_family_ipv6_multicast_cmd,
	 "address-family ipv6 multicast",
	 "Enter Address Family command mode\n"
	 "Address family\n"
	 "Address Family Modifier\n")
{
  vty->node = BGP_IPV6M_NODE;
  return CMD_SUCCESS;
}
#endif

DEFUNSH (IMISH_BGPD,
	 exit_address_family,
	 exit_address_family_cmd,
	 "exit-address-family",
	 "Exit from Address Family configuration mode\n")
{
  if (vty->node == BGP_IPV4_NODE
      || vty->node == BGP_IPV4M_NODE
      || vty->node == BGP_VPNV4_NODE
      //|| vty->node == BGP_VPNV6_NODE
      //|| vty->node == BGP_ENCAP_NODE
      //|| vty->node == BGP_ENCAPV6_NODE
      || vty->node == BGP_IPV6_NODE
      || vty->node == BGP_IPV6M_NODE)
    vty->node = BGP_NODE;
  return CMD_SUCCESS;
}

ALIAS (exit_address_family,
		quit_address_family_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

/* RIP */
DEFUNSH (IMISH_RIPD,
	 router_ripd,
	 router_ripd_cmd,
	 "router rip",
	 ROUTER_STR
	 "RIP")
{
  vty->node = RIP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RIPD,
	 key_chain,
	 key_chain_cmd,
	 "key chain WORD",
	 "Authentication key management\n"
	 "Key-chain management\n"
	 "Key-chain name\n")
{
  vty->node = KEYCHAIN_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RIPD,
	 key,
	 key_cmd,
	 "key <0-2147483647>",
	 "Configure a key\n"
	 "Key identifier number\n")
{
  vty->node = KEYCHAIN_KEY_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RIPD,
	 imish_exit_ripd,
	 imish_exit_ripd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ripd,
       imish_quit_ripd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")


/* OSPFD */
DEFUNSH (IMISH_OSPFD,
	 router_ospfd,
	 router_ospfd_cmd,
	 "router ospf",
	 "Enable a routing process\n"
	 "Start OSPF configuration\n")
{
  vty->node = OSPF_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_OSPFD,
	 imish_exit_ospfd,
	 imish_exit_ospfd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ospfd,
       imish_quit_ospfd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

#ifdef HAVE_IPV6
/* RIPNGD */
DEFUNSH (IMISH_RIPNGD,
	 router_ripngd,
	 router_ripngd_cmd,
	 "router ripng",
	 ROUTER_STR
	 "RIPng")
{
  vty->node = RIPNG_NODE;
  return CMD_SUCCESS;
}
DEFUNSH (IMISH_RIPNGD,
	 imish_exit_ripngd,
	 imish_exit_ripngd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ripngd,
       imish_quit_ripngd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

/* OSPF6D */
DEFUNSH (IMISH_OSPF6D,
	 router_ospf6d,
	 router_ospf6d_cmd,
	 "router ospf6",
	 OSPF6_ROUTER_STR
	 OSPF6_STR)
{
  vty->node = OSPF6_NODE;
  return CMD_SUCCESS;
}
DEFUNSH (IMISH_RIPNGD,
	 imish_exit_ospf6d,
	 imish_exit_ospf6d_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ospf6d,
       imish_quit_ospf6d_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")

#endif

/* ISISD */
DEFUNSH (IMISH_ISISD,
	 router_isisd,
	 router_isisd_cmd,
	 "router isis WORD",
	 ROUTER_STR
	 "ISO IS-IS\n"
	 "ISO Routing area tag")
{
  vty->node = ISIS_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ISISD,
	 imish_exit_isisd,
	 imish_exit_isisd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_isisd,
       imish_quit_isisd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")


/* PIMD */


#ifdef HAVE_EXPAND_ROUTE_PLATFORM
/* OLSRD */
#ifdef OLSR_VTYSH_PATH
DEFUNSH (IMISH_OLSRD,
	 router_olsrd,
	 router_olsrd_cmd,
	 "router olsr",
	 ROUTER_STR
   "Start OLSR configuration\n")
{
  vty->node = OLSR_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_OLSRD,
	 imish_exit_olsrd,
	 imish_exit_olsrd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_olsrd,
       imish_quit_olsrd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* FRPD */
#ifdef FRP_VTYSH_PATH
DEFUNSH (IMISH_FRPD,
	 router_frpd,
	 router_frpd_cmd,
	 "router frp",
	 ROUTER_STR
   "Start FRP configuration\n")
{
  vty->node = FRP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_FRPD,
	 imish_exit_frpd,
	 imish_exit_frpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_frpd,
       imish_quit_frpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif
/* ICRPD */
#ifdef ICRP_VTYSH_PATH
DEFUNSH (IMISH_ICRPD,
	 router_icrpd,
	 router_icrpd_cmd,
	 "router icrp",
	 ROUTER_STR
   "Start ICRP configuration\n")
{
  vty->node = ICRP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_ICRPD,
	 imish_exit_icrpd,
	 imish_exit_icrpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_icrpd,
       imish_quit_icrpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* LDPD */
#ifdef LDP_VTYSH_PATH
DEFUNSH (IMISH_LDPD,
	 router_ldpd,
	 router_ldpd_cmd,
	 "router ldp",
	 ROUTER_STR
   "Start LDP configuration\n")
{
  vty->node = LDP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_LDPD,
	 imish_exit_ldpd,
	 imish_exit_ldpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_ldpd,
       imish_quit_ldpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* LLDPD */
#ifdef LLDP_VTYSH_PATH
DEFUNSH (IMISH_LLDPD,
	 router_lldpd,
	 router_lldpd_cmd,
	 "router lldp",
	 ROUTER_STR
   "Start LDP configuration\n")
{
  vty->node = LLDP_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_LLDPD,
	 imish_exit_lldpd,
	 imish_exit_lldpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_lldpd,
       imish_quit_lldpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* MPLSD */
#ifdef MPLS_VTYSH_PATH
DEFUNSH (IMISH_MPLSD,
	 router_mplsd,
	 router_mplsd_cmd,
	 "router mpls",
	 ROUTER_STR
   "Start MPLS configuration\n")
{
  vty->node = MPLS_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_MPLSD,
	 imish_exit_mplsd,
	 imish_exit_mplsd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_mplsd,
       imish_quit_mplsd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* RSVPD */
#ifdef RSVP_VTYSH_PATH
DEFUNSH (IMISH_RSVPD,
	 router_rsvpd,
	 router_rsvpd_cmd,
	 "router rsvp",
	 ROUTER_STR
   "Start RSVP configuration\n")
{
  vty->node = MPLS_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_RSVPD,
	 imish_exit_rsvpd,
	 imish_exit_rsvpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_rsvpd,
       imish_quit_rsvpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* VPND */
#ifdef VPN_VTYSH_PATH
DEFUNSH (IMISH_VPND,
	 router_vpnd,
	 router_vpnd_cmd,
	 "router vpn",
	 ROUTER_STR
   "Start VPN configuration\n")
{
  //vty->node = VPN_NODE;
  return CMD_SUCCESS;
}

DEFUNSH (IMISH_VPND,
	 imish_exit_vpnd,
	 imish_exit_vpnd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_vpnd,
       imish_quit_vpnd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

/* VRRPD */
#ifdef VRRP_VTYSH_PATH
DEFUNSH (IMISH_VRRPD,
	 router_vrrpd,
	 router_vrrpd_cmd,
	 "router vrrp  <1-254>",
	 ROUTER_STR
     "Start VRRP configuration\n"
	 "Autonomous system number config\n")
{
  vty->node = VRRP_NODE;
  return CMD_SUCCESS;
}
/*
DEFUNSH (IMISH_VRRPD,
	 no_router_vrrpd,
	 no_router_vrrpd_cmd,
	 "no router vrrp  <1-254>",
	 NO_STR
	 ROUTER_STR
     "Start VRRP configuration\n"
	 "Autonomous system number config\n")
{
  vty->node = VRRP_NODE;
  return CMD_SUCCESS;
}
*/
DEFUNSH (IMISH_VRRPD,
	 imish_exit_vrrpd,
	 imish_exit_vrrpd_cmd,
	 "exit",
	 "Exit current mode and down to previous mode\n")
{
  return imish_exit (vty);
}

ALIAS (imish_exit_vrrpd,
       imish_quit_vrrpd_cmd,
       "quit",
       "Exit current mode and down to previous mode\n")
#endif

#endif
















#ifdef HAVE_SYSTEM_UTILS
/* tunnel */
DEFSH(IMISH_UTILSD,
		imish_ip_tunnel_src_cmd,
	    "ip tunnel source A.B.C.D",
		IP_STR
	    "tunnel Interface\n"
		"Select source ip address\n"
		"IP address information\n")

DEFSH(IMISH_UTILSD,
		imish_ip_tunnel_dest_cmd,
	    "ip tunnel remote A.B.C.D",
		IP_STR
	    "tunnel Interface\n"
		"Select remote ip address\n"
		"IP address information\n")

DEFSH(IMISH_UTILSD,
		imish_ip_tunnel_ttl_cmd,
	    "ip tunnel ttl <16-255>",
		IP_STR
	    "tunnel Interface\n"
		"configure ttl\n"
		"ttl value\n")

DEFSH(IMISH_UTILSD,
		imish_no_ip_tunnel_ttl_cmd,
	    "no ip tunnel ttl",
		NO_STR
		IP_STR
	    "tunnel Interface\n"
		"configure ttl\n")

DEFSH(IMISH_UTILSD,
		imish_ip_tunnel_mtu_cmd,
	    "ip tunnel mtu <64-1500>",
		IP_STR
	    "tunnel Interface\n"
		"configure mtu\n"
		"mtu value\n")

DEFSH(IMISH_UTILSD,
		imish_no_ip_tunnel_mtu_cmd,
	    "no ip tunnel mtu",
		NO_STR
		IP_STR
	    "tunnel Interface\n"
		"configure mtu\n")

DEFSH(IMISH_UTILSD,
		imish_ip_tunnel_mode_cmd,
	    "ip tunnel mode (gre|ipip|vti|sit)",
		IP_STR
	    "tunnel Interface\n"
		"configure mode\n"
		"gre tunnel mode\n"
		"ipip tunnel mode\n"
		"vti tunnel mode\n"
		"sit tunnel mode\n")

DEFSH(IMISH_UTILSD,
		imish_interface_tunnel_cmd,
	    "interface tunnel <0-32>",
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")

DEFSH(IMISH_UTILSD,
		imish_no_interface_tunnel_cmd,
	    "no interface tunnel <0-32>",
		NO_STR
	    "Select an interface to configure\n"
	    "tunnel Interface\n"
		"tunnel num \n")

DEFSH(IMISH_UTILSD,
		imish_show_tunnel_interface_cmd,
	    "show tunnel interface [NAME]",
		SHOW_STR
	    "tunnel Interface\n"
		INTERFACE_STR
		"Interface Name\n")

/* brigde interface ctl */
DEFSH(IMISH_UTILSD,
		imish_ip_bridge_cmd,
		"interface bridge <0-32>",
		INTERFACE_STR
	    "bridge Interface\n"
		"Interface name num\n")

DEFSH(IMISH_UTILSD,
		imish_no_ip_bridge_cmd,
		"no interface bridge <0-32>",//"ip bridge NAME",
		NO_STR
		INTERFACE_STR
	    "bridge Interface\n"
		"Interface name num\n")

/* 网桥接口下命令 */
DEFSH(IMISH_UTILSD,
		imish_ip_bridge_add_sub_cmd,
	    "ip bridge sub NAME",
		IP_STR
	    "bridge Interface\n"
		"sub bridge Interface\n"
		"Interface Name\n")

DEFSH(IMISH_UTILSD,
		imish_no_ip_bridge_add_sub_cmd,
	    "no ip bridge sub NAME",
		NO_STR
		IP_STR
	    "bridge Interface\n"
		"sub bridge Interface\n"
		"Interface Name\n")

//在非网桥接口下的命令
DEFSH(IMISH_UTILSD,
		imish_ip_bridge_on_cmd,
	    "ip bridge on NAME",
		IP_STR
	    "bridge Interface\n"
		"bridge on Interface\n"
		"bridge Interface Name\n")

DEFSH(IMISH_UTILSD,
		imish_no_ip_bridge_on_cmd,
	    "no ip bridge on NAME",
		NO_STR
		IP_STR
	    "bridge Interface\n"
		"bridge on Interface\n"
		"Interface Name\n")

DEFSH(IMISH_UTILSD,
		imish_ip_bridge_stp_on_cmd,
	    "ip bridge stp on",
		IP_STR
	    "bridge Interface\n"
		"stp on bridge Interface\n"
		"open stp\n")

DEFSH(IMISH_UTILSD,
		imish_no_ip_bridge_stp_on_cmd,
	    "ip bridge stp off",
		IP_STR
	    "bridge Interface\n"
		"stp on bridge Interface\n"
		"close stp\n")


DEFSH(IMISH_UTILSD,
		imish_show_bridge_interface_cmd,
	    "show bridge interface [NAME]",
		SHOW_STR
	    "bridge Interface\n"
		INTERFACE_STR
		"Interface Name\n")

DEFSH(IMISH_UTILSD,
		imish_show_bridge_table_cmd,
	    "show bridge-table",
		SHOW_STR
	    "bridge Interface\n"
		INTERFACE_STR
		"Interface Name\n")

/* vlan */
DEFSH(IMISH_UTILSD,
		imish_interface_vlan_cmd,
		"interface vlan <0-32>",
		INTERFACE_STR
		"vlan Interface\n"
		"Interface name num\n")

DEFSH(IMISH_UTILSD,
		imish_no_interface_vlan_cmd,
		"no interface vlan <0-32>",
		NO_STR
		INTERFACE_STR
		"vlan Interface\n"
		"Interface name num\n")

DEFSH(IMISH_UTILSD,
		imish_ip_vlan_skb_qos_cmd,
		"ip vlan (egress|ingress) skb-priority <0-7> qos <0-7>",
		IP_STR
		"vlan Interface\n"
		"outbound  config\n"
		"inbound  config\n"
		"skb priority\n"
		"skb priority value\n"
		"qos priority\n"
		"802.1q priority value\n")


DEFSH(IMISH_UTILSD,
		imish_no_ip_vlan_skb_qos_cmd,
		"no ip vlan (egress|ingress) skb-priority <0-7>",
		NO_STR
		IP_STR
		"vlan Interface\n"
		"outbound  config\n"
		"inbound  config\n"
		"skb priority\n"
		"skb priority value\n")


DEFSH(IMISH_UTILSD,
		imish_show_vlan_interface_cmd,
		"show vlan interface [NAME]",
		SHOW_STR
		"vlan Interface\n"
		INTERFACE_STR
		"Interface name\n")

DEFSH(IMISH_UTILSD,
		imish_base_vlan_interface_cmd,
		"base vlan [NAME]",
		"base Interface\n"
		"vlan Interface\n"
		"Interface name\n")

static int imish_utils_cmd_init (void)
{
	/* tunnel */
	install_element (ENABLE_NODE, &imish_show_tunnel_interface_cmd);
	install_element (CONFIG_NODE, &imish_interface_tunnel_cmd);
	install_element (CONFIG_NODE, &imish_no_interface_tunnel_cmd);
	install_element (INTERFACE_NODE, &imish_ip_tunnel_mode_cmd);
	install_element (INTERFACE_NODE, &imish_ip_tunnel_src_cmd);
	install_element (INTERFACE_NODE, &imish_ip_tunnel_dest_cmd);
	install_element (INTERFACE_NODE, &imish_ip_tunnel_ttl_cmd);
	install_element (INTERFACE_NODE, &imish_no_ip_tunnel_ttl_cmd);
	install_element (INTERFACE_NODE, &imish_ip_tunnel_mtu_cmd);
	install_element (INTERFACE_NODE, &imish_no_ip_tunnel_mtu_cmd);
	/* bridge */
	install_element (ENABLE_NODE, &imish_show_bridge_interface_cmd);
	install_element (ENABLE_NODE, &imish_show_bridge_table_cmd);

	install_element (CONFIG_NODE, &imish_ip_bridge_cmd);
	install_element (CONFIG_NODE, &imish_no_ip_bridge_cmd);

	install_element (INTERFACE_NODE, &imish_ip_bridge_add_sub_cmd);
	install_element (INTERFACE_NODE, &imish_no_ip_bridge_add_sub_cmd);
	install_element (INTERFACE_NODE, &imish_ip_bridge_on_cmd);
	install_element (INTERFACE_NODE, &imish_no_ip_bridge_on_cmd);

	install_element (INTERFACE_NODE, &imish_ip_bridge_stp_on_cmd);
	install_element (INTERFACE_NODE, &imish_no_ip_bridge_stp_on_cmd);

	/* vlan */
	install_element (CONFIG_NODE, &imish_interface_vlan_cmd);
	install_element (CONFIG_NODE, &imish_no_interface_vlan_cmd);
	install_element (ENABLE_NODE, &imish_show_vlan_interface_cmd);

	//install_element (INTERFACE_NODE, &ip_vlan_header_flag_cmd);
	install_element (CONFIG_NODE, &imish_base_vlan_interface_cmd);
	install_element (INTERFACE_NODE, &imish_ip_vlan_skb_qos_cmd);
	install_element (INTERFACE_NODE, &imish_no_ip_vlan_skb_qos_cmd);
	return 0;
}
#endif /*HAVE_SYSTEM_UTILS*/


void imish_module_client_cmd_init (void)
{
	/*BABELD*/
	install_element (CONFIG_NODE, &router_babeld_cmd);
	install_element (BABEL_NODE, &imish_exit_babeld_cmd);
	install_element (BABEL_NODE, &imish_quit_babeld_cmd);

	/*BGPD*/
	install_element (CONFIG_NODE, &router_bgp_cmd);
	install_element (CONFIG_NODE, &router_bgp_view_cmd);
	install_element (BGP_NODE, &address_family_vpnv4_cmd);
	install_element (BGP_NODE, &address_family_vpnv4_unicast_cmd);
	//install_element (BGP_NODE, &address_family_vpnv6_cmd);
	//install_element (BGP_NODE, &address_family_vpnv6_unicast_cmd);
	//install_element (BGP_NODE, &address_family_encap_cmd);
	//install_element (BGP_NODE, &address_family_encapv6_cmd);
	install_element (BGP_NODE, &address_family_ipv4_unicast_cmd);
	install_element (BGP_NODE, &address_family_ipv4_multicast_cmd);
#ifdef HAVE_IPV6
	install_element (BGP_NODE, &address_family_ipv6_cmd);
	install_element (BGP_NODE, &address_family_ipv6_unicast_cmd);
#endif
	install_element (BGP_VPNV4_NODE, &exit_address_family_cmd);
	//install_element (BGP_VPNV6_NODE, &exit_address_family_cmd);
	//install_element (BGP_ENCAP_NODE, &exit_address_family_cmd);
	//install_element (BGP_ENCAPV6_NODE, &exit_address_family_cmd);

	install_element (BGP_IPV4_NODE, &exit_address_family_cmd);
	install_element (BGP_IPV4M_NODE, &exit_address_family_cmd);
	install_element (BGP_IPV6_NODE, &exit_address_family_cmd);
	install_element (BGP_IPV6M_NODE, &exit_address_family_cmd);

	/*RIPD*/
	install_element (CONFIG_NODE, &router_ripd_cmd);
	install_element (CONFIG_NODE, &key_chain_cmd);
	install_element (KEYCHAIN_NODE, &key_chain_cmd);
	install_element (KEYCHAIN_KEY_NODE, &key_chain_cmd);
	install_element (KEYCHAIN_NODE, &key_cmd);
	//install_element (KEYCHAIN_NODE, &vtysh_end_all_cmd);
	//install_element (KEYCHAIN_KEY_NODE, &vtysh_end_all_cmd);
	install_element (KEYCHAIN_NODE, &imish_exit_ripd_cmd);
	install_element (KEYCHAIN_NODE, &imish_quit_ripd_cmd);
	install_element (KEYCHAIN_KEY_NODE, &imish_exit_ripd_cmd);
	install_element (KEYCHAIN_KEY_NODE, &imish_quit_ripd_cmd);
	install_element (RIP_NODE, &imish_exit_ripd_cmd);
	install_element (RIP_NODE, &imish_quit_ripd_cmd);

	/*OSPFD*/
	install_element (CONFIG_NODE, &router_ospfd_cmd);
	install_element (OSPF_NODE, &imish_exit_ospfd_cmd);
	install_element (OSPF_NODE, &imish_quit_ospfd_cmd);

#ifdef HAVE_IPV6
	/*RIPNGD*/
	install_element (CONFIG_NODE, &router_ripngd_cmd);
	install_element (RIPNG_NODE, &imish_exit_ripngd_cmd);
	install_element (RIPNG_NODE, &imish_quit_ripngd_cmd);
	/*OSPF6D*/
	install_element (CONFIG_NODE, &router_ospf6d_cmd);
	install_element (OSPF6_NODE, &imish_exit_ospf6d_cmd);
	install_element (OSPF6_NODE, &imish_quit_ospf6d_cmd);
#endif
	/*ISISD*/
	install_element (CONFIG_NODE, &router_isisd_cmd);
	install_element (ISIS_NODE, &imish_exit_isisd_cmd);
	install_element (ISIS_NODE, &imish_quit_isisd_cmd);

	/* 2016年7月2日 22:08:58 zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_EXPAND_ROUTE_PLATFORM
#ifdef HSLS_VTYSH_PATH

#endif
#ifdef OLSR_VTYSH_PATH
	install_element (CONFIG_NODE, &router_olsrd_cmd);
	install_element (OLSR_NODE, &imish_exit_olsrd_cmd);
	install_element (OLSR_NODE, &imish_quit_olsrd_cmd);
#endif
#ifdef ICRP_VTYSH_PATH
	install_element (CONFIG_NODE, &router_icrpd_cmd);
	install_element (ICRP_NODE, &imish_exit_icrpd_cmd);
	install_element (ICRP_NODE, &imish_quit_icrpd_cmd);
#endif
#ifdef FRP_VTYSH_PATH
	install_element (CONFIG_NODE, &router_frpd_cmd);
	install_element (FRP_NODE, &imish_exit_frpd_cmd);
	install_element (FRP_NODE, &imish_quit_frpd_cmd);
#endif
#ifdef VRRP_VTYSH_PATH
	install_element (CONFIG_NODE, &router_vrrpd_cmd);
	install_element (VRRP_NODE, &imish_exit_vrrpd_cmd);
	install_element (VRRP_NODE, &imish_quit_vrrpd_cmd);
#endif
#ifdef LLDP_VTYSH_PATH
	install_element (CONFIG_NODE, &router_lldpd_cmd);
	install_element (LLDP_NODE, &imish_exit_lldpd_cmd);
	install_element (LLDP_NODE, &imish_quit_lldpd_cmd);
#endif
#ifdef VPN_VTYSH_PATH
//	install_element (CONFIG_NODE, &router_olsrd_cmd);
//	install_element (OLSR_NODE, &imish_exit_olsrd_cmd);
//	install_element (OLSR_NODE, &imish_quit_olsrd_cmd);
#endif
#ifdef LDP_VTYSH_PATH
	install_element (CONFIG_NODE, &router_ldpd_cmd);
	install_element (LDP_NODE, &imish_exit_ldpd_cmd);
	install_element (LDP_NODE, &imish_quit_ldpd_cmd);
#endif
#ifdef RSVP_VTYSH_PATH
	install_element (CONFIG_NODE, &router_rsvpd_cmd);
	install_element (RSVP_NODE, &imish_exit_rsvpd_cmd);
	install_element (RSVP_NODE, &imish_quit_rsvpd_cmd);
#endif
#ifdef MPLS_VTYSH_PATH
	install_element (CONFIG_NODE, &router_mplsd_cmd);
	install_element (MPLS_NODE, &imish_exit_mplsd_cmd);
	install_element (MPLS_NODE, &imish_quit_mplsd_cmd);
#endif
#endif /* HAVE_EXPAND_ROUTE_PLATFORM*/
	/* 2016年7月2日 22:08:58  zhurish: 扩展路由协议增加命令 */
#ifdef HAVE_SYSTEM_UTILS
	imish_utils_cmd_init ();
#endif /* HAVE_SYSTEM_UTILS*/
}
#endif/* IMISH_IMI_MODULE*/
