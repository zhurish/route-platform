/* Auto-generated from route_types.txt by . */
/* Do not edit! */

#ifndef _QUAGGA_ROUTE_TYPES_H
#define _QUAGGA_ROUTE_TYPES_H

/* Zebra route's types. */
#define ZEBRA_ROUTE_SYSTEM               0
#define ZEBRA_ROUTE_KERNEL               1
#define ZEBRA_ROUTE_CONNECT              2
#define ZEBRA_ROUTE_STATIC               3
#define ZEBRA_ROUTE_RIP                  4
#define ZEBRA_ROUTE_RIPNG                5
#define ZEBRA_ROUTE_OSPF                 6
#define ZEBRA_ROUTE_OSPF6                7
#define ZEBRA_ROUTE_ISIS                 8
#define ZEBRA_ROUTE_BGP                  9
#define ZEBRA_ROUTE_PIM                  10
#define ZEBRA_ROUTE_HSLS                 11
#define ZEBRA_ROUTE_OLSR                 12
#define ZEBRA_ROUTE_BABEL                13
#define ZEBRA_ROUTE_ICRP                 14
#define ZEBRA_ROUTE_FRP                  15
#define ZEBRA_ROUTE_AODV                 16
#define ZEBRA_ROUTE_VRRP                 17
#define ZEBRA_ROUTE_LLDP                 18
#define ZEBRA_ROUTE_VPN                  19
#define ZEBRA_ROUTE_BFD                  20
#define ZEBRA_ROUTE_LDP                  21
#define ZEBRA_ROUTE_RSVP                 22
#define ZEBRA_ROUTE_MPLS                 23
#define ZEBRA_ROUTE_UTILS                24
#define ZEBRA_ROUTE_MANAGE               25
#define ZEBRA_ROUTE_SWITCH               26
#define ZEBRA_ROUTE_MAX                  27

#define SHOW_ROUTE_V4_HEADER \
  "Codes: K - kernel route, C - connected, S - static, R - RIP,%s" \
  "       O - OSPF, I - IS-IS, B - BGP, P - PIM, o - OLSR, A - Babel,%s" \
  "       i - ICRP, F - FRP, a - AODV,%s" \
  "       > - selected route, * - FIB route%s%s", \
  VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE
#define SHOW_ROUTE_V6_HEADER \
  "Codes: K - kernel route, C - connected, S - static, R - RIPng,%s" \
  "       O - OSPFv6, I - IS-IS, B - BGP, A - Babel,%s" \
  "       > - selected route, * - FIB route%s%s", \
  VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE

/* aodvd */
#define QUAGGA_REDIST_STR_AODVD \
  "(kernel|connected|static|rip|ospf|isis|bgp|pim|olsr|babel|icrp|frp)"
#define QUAGGA_REDIST_HELP_STR_AODVD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n"

/* babeld */
#define QUAGGA_REDIST_STR_BABELD \
  "(kernel|connected|static|rip|ripng|ospf|ospf6|isis|bgp|pim|olsr|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_BABELD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP_REDIST_STR_BABELD \
  "(kernel|connected|static|rip|ospf|isis|bgp|pim|olsr|icrp|frp|aodv)"
#define QUAGGA_IP_REDIST_HELP_STR_BABELD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP6_REDIST_STR_BABELD \
  "(kernel|connected|static|ripng|ospf6|isis|bgp)"
#define QUAGGA_IP6_REDIST_HELP_STR_BABELD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n"

/* bgpd */
#define QUAGGA_REDIST_STR_BGPD \
  "(kernel|connected|static|rip|ripng|ospf|ospf6|isis|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_BGPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP_REDIST_STR_BGPD \
  "(kernel|connected|static|rip|ospf|isis|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_IP_REDIST_HELP_STR_BGPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP6_REDIST_STR_BGPD \
  "(kernel|connected|static|ripng|ospf6|isis|babel)"
#define QUAGGA_IP6_REDIST_HELP_STR_BGPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Babel routing protocol (Babel)\n"

/* frpd */
#define QUAGGA_REDIST_STR_FRPD \
  "(kernel|connected|static|rip|ospf|isis|bgp|pim|olsr|babel|icrp|aodv)"
#define QUAGGA_REDIST_HELP_STR_FRPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"

/* icrpd */
#define QUAGGA_REDIST_STR_ICRPD \
  "(kernel|connected|static|rip|ospf|isis|bgp|pim|olsr|babel|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_ICRPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"

/* isisd */
#define QUAGGA_REDIST_STR_ISISD \
  "(kernel|connected|static|rip|ripng|ospf|ospf6|bgp|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_ISISD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP_REDIST_STR_ISISD \
  "(kernel|connected|static|rip|ospf|bgp|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_IP_REDIST_HELP_STR_ISISD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP6_REDIST_STR_ISISD \
  "(kernel|connected|static|ripng|ospf6|bgp|babel)"
#define QUAGGA_IP6_REDIST_HELP_STR_ISISD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Babel routing protocol (Babel)\n"

/* olsrd */
#define QUAGGA_REDIST_STR_OLSRD \
  "(kernel|connected|static|rip|ospf|isis|bgp|pim|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_OLSRD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"

/* ospf6d */
#define QUAGGA_REDIST_STR_OSPF6D \
  "(kernel|connected|static|ripng|isis|bgp|babel)"
#define QUAGGA_REDIST_HELP_STR_OSPF6D \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Babel routing protocol (Babel)\n"

/* ospfd */
#define QUAGGA_REDIST_STR_OSPFD \
  "(kernel|connected|static|rip|isis|bgp|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_OSPFD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"

/* pimd */
#define QUAGGA_REDIST_STR_PIMD \
  "(kernel|connected|static|rip|ospf|isis|bgp|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_PIMD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"

/* ripd */
#define QUAGGA_REDIST_STR_RIPD \
  "(kernel|connected|static|ospf|isis|bgp|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_RIPD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"

/* ripngd */
#define QUAGGA_REDIST_STR_RIPNGD \
  "(kernel|connected|static|ospf6|isis|bgp|babel)"
#define QUAGGA_REDIST_HELP_STR_RIPNGD \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Babel routing protocol (Babel)\n"

/* zebra */
#define QUAGGA_REDIST_STR_ZEBRA \
  "(kernel|connected|static|rip|ripng|ospf|ospf6|isis|bgp|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_REDIST_HELP_STR_ZEBRA \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP_REDIST_STR_ZEBRA \
  "(kernel|connected|static|rip|ospf|isis|bgp|pim|olsr|babel|icrp|frp|aodv)"
#define QUAGGA_IP_REDIST_HELP_STR_ZEBRA \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol (RIP)\n" \
  "Open Shortest Path First (OSPFv2)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Protocol Independent Multicast (PIM)\n" \
  "Optimised Link State Routing (OLSR)\n" \
  "Babel routing protocol (Babel)\n" \
  "Internet Based On Node Contact Time Routing Protocol  (ICRP)\n" \
  "Fast and Reliable Routing Protocol  (FRP)\n" \
  "AD HOT MANET Routing Protocol  (AODV)\n"
#define QUAGGA_IP6_REDIST_STR_ZEBRA \
  "(kernel|connected|static|ripng|ospf6|isis|bgp|babel)"
#define QUAGGA_IP6_REDIST_HELP_STR_ZEBRA \
  "Kernel routes (not installed via the zebra RIB)\n" \
  "Connected routes (directly attached subnet or host)\n" \
  "Statically configured routes\n" \
  "Routing Information Protocol next-generation (IPv6) (RIPng)\n" \
  "Open Shortest Path First (IPv6) (OSPFv3)\n" \
  "Intermediate System to Intermediate System (IS-IS)\n" \
  "Border Gateway Protocol (BGP)\n" \
  "Babel routing protocol (Babel)\n"


#ifdef QUAGGA_DEFINE_DESC_TABLE

struct zebra_desc_table
{
  unsigned int type;
  const char *string;
  char chr;
};

#define DESC_ENTRY(T,S,C) [(T)] = { (T), (S), (C) }
static const struct zebra_desc_table route_types[] = {
  DESC_ENTRY	(ZEBRA_ROUTE_SYSTEM,	 "system",	'X' ),
  DESC_ENTRY	(ZEBRA_ROUTE_KERNEL,	 "kernel",	'K' ),
  DESC_ENTRY	(ZEBRA_ROUTE_CONNECT,	 "connected",	'C' ),
  DESC_ENTRY	(ZEBRA_ROUTE_STATIC,	 "static",	'S' ),
  DESC_ENTRY	(ZEBRA_ROUTE_RIP,	 "rip",	'R' ),
  DESC_ENTRY	(ZEBRA_ROUTE_RIPNG,	 "ripng",	'R' ),
  DESC_ENTRY	(ZEBRA_ROUTE_OSPF,	 "ospf",	'O' ),
  DESC_ENTRY	(ZEBRA_ROUTE_OSPF6,	 "ospf6",	'O' ),
  DESC_ENTRY	(ZEBRA_ROUTE_ISIS,	 "isis",	'I' ),
  DESC_ENTRY	(ZEBRA_ROUTE_BGP,	 "bgp",	'B' ),
  DESC_ENTRY	(ZEBRA_ROUTE_PIM,	 "pim",	'P' ),
  DESC_ENTRY	(ZEBRA_ROUTE_HSLS,	 "hsls",	'H' ),
  DESC_ENTRY	(ZEBRA_ROUTE_OLSR,	 "olsr",	'o' ),
  DESC_ENTRY	(ZEBRA_ROUTE_BABEL,	 "babel",	'A' ),
  DESC_ENTRY	(ZEBRA_ROUTE_ICRP,	 "icrp",	'i' ),
  DESC_ENTRY	(ZEBRA_ROUTE_FRP,	 "frp",	'F' ),
  DESC_ENTRY	(ZEBRA_ROUTE_AODV,	 "aodv",	'a' ),
  DESC_ENTRY	(ZEBRA_ROUTE_VRRP,	 "vrrp",	'v' ),
  DESC_ENTRY	(ZEBRA_ROUTE_LLDP,	 "lldp",	'L' ),
  DESC_ENTRY	(ZEBRA_ROUTE_VPN,	 "vpn",	'N' ),
  DESC_ENTRY	(ZEBRA_ROUTE_BFD,	 "bfd",	'B' ),
  DESC_ENTRY	(ZEBRA_ROUTE_LDP,	 "ldp",	'l' ),
  DESC_ENTRY	(ZEBRA_ROUTE_RSVP,	 "rsvp",	'V' ),
  DESC_ENTRY	(ZEBRA_ROUTE_MPLS,	 "mpls",	'M' ),
  DESC_ENTRY	(ZEBRA_ROUTE_UTILS,	 "utils",	'u' ),
  DESC_ENTRY	(ZEBRA_ROUTE_MANAGE,	 "manage",	'm' ),
  DESC_ENTRY	(ZEBRA_ROUTE_SWITCH,	 "switch",	's' ),
};
#undef DESC_ENTRY

#endif /* QUAGGA_DEFINE_DESC_TABLE */

#endif /* _QUAGGA_ROUTE_TYPES_H */
