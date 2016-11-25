#ifndef	_EIGRP_SYSPORT_ROUTER_SNMP_H_
#define	_EIGRP_SYSPORT_ROUTER_SNMP_H_

#include	<stdio.h>
#ifdef __cplusplus 
extern "C"{
#endif

#ifdef INCLUDE_EIGRP_SNMP

/* EIGRP-MIB. */
#define EIGRPMIB 1,3,6,1,2,1,15

/* ZebOS enterprise EIGRP MIB.  This variable is used for register
   EIGRP MIB to SNMP agent under SMUX protocol.  */
#define EIGRPDOID 1,3,6,1,4,1,3317,1,2,11


/* OSPF MIB General Group values. */
#define EIGRP_AS_NO								1
#define EIGRP_AS_SUBNET						2
#define EIGRP_AS_ENABLENET						3
#define EIGRP_AS_AUTO_FOUND_NEIGHBORS		4
#define EIGRP_AS_ROW_STATUS					5

/* SNMP value hack. */
#define COUNTER     ASN_COUNTER
#define INTEGER     ASN_INTEGER
#define GAUGE       ASN_GAUGE
#define TIMETICKS   ASN_TIMETICKS
#define IPADDRESS   ASN_IPADDRESS
#define STRING      ASN_OCTET_STR

#endif//INCLUDE_EIGRP_SNMP

#ifdef __cplusplus
}
#endif

#endif	/** _EIGRP_SYSPORT_ROUTER_SNMP_H_ **/
