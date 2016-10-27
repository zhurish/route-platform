/*
 * utils.h
 *
 *  Created on: Oct 16, 2016
 *      Author: zhurish
 */

#ifndef SYSTEM_UTILS_UTILS_H_
#define SYSTEM_UTILS_UTILS_H_


#define HAVE_UTILS_TUNNEL 1
#define HAVE_UTILS_BRCTL 1
//#define HAVE_UTILS_DHCPD 1
//#define HAVE_UTILS_FIREWALL 1
//#define HAVE_UTILS_IPTABLES 1
#define HAVE_UTILS_VLAN 1


#ifdef HAVE_UTILS_TUNNEL
#define TUNNEL_TABLE_MAX	32
#define TUNNEL_MTU_DEFAULT	1476
#define TUNNEL_TTL_DEFAULT	255
#define TUNNEL_DEBUG
#endif






#define UTILS_DEBUG
#define UTILS_DEFAULT_CONFIG    "utils.conf"
#define UTILS_VTY_PORT	2621



extern struct thread_master *master;
extern struct zebra_privs_t utils_privs;


extern int super_system(const char *cmd);

extern void utils_zclient_init (void);







#ifdef UTILS_DEBUG
#define UTILS_DEBUG_LOG(format, args...)	\
	fprintf(stderr, "%s:",__func__); \
	fprintf(stderr, format, ##args); \
	fprintf(stderr, "\n");
#else
#define UTILS_DEBUG_LOG(format, args...)
#endif

#endif /* SYSTEM_UTILS_UTILS_H_ */
