/*
 * ip_vlan.h
 *
 *  Created on: Oct 21, 2016
 *      Author: zhurish
 */

#ifndef SYSTEM_UTILS_IP_VLAN_H_
#define SYSTEM_UTILS_IP_VLAN_H_

#ifdef HAVE_UTILS_VLAN


#define VLAN_PARENT_NAME	"enp0s25"
#define VLAN_DEV_PROC	"/proc/net/vlan/"


extern int ip_vlan_startup_config(struct utils_interface *uifp);
extern void utils_vlan_cmd_init (void);
extern int vlan_interface_config_write (struct vty *vty, struct interface *ifp);
extern int no_vlan_interface(struct vty *vty, const char *ifname);

#endif

#endif /* SYSTEM_UTILS_IP_VLAN_H_ */
