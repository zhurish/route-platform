/*
 * ip_brctl.h
 *
 *  Created on: Oct 22, 2016
 *      Author: zhurish
 */

#ifndef SYSTEM_UTILS_IP_BRCTL_H_
#define SYSTEM_UTILS_IP_BRCTL_H_


#ifdef HAVE_UTILS_BRCTL

/* if_mode */

//#define ZEBRA_INTERFACE_BRIDGE (1<<3)

//#define HAVE_UTILS_BRCTL_MAX	32

extern void utils_bridge_cmd_init (void);
extern int bridge_interface_config_write (struct vty *vty, struct interface *ifp);
extern int no_bridge_interface(struct vty *vty, const char *ifname);
extern int ip_bridge_startup_config(struct utils_interface *uifp);
extern int ip_bridge_port_startup_config(struct utils_interface *uifp);

#endif


#endif /* SYSTEM_UTILS_IP_BRCTL_H_ */
