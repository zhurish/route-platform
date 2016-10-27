/*
 * ip_tunnel.h
 *
 *  Created on: Oct 16, 2016
 *      Author: zhurish
 */

#ifndef SYSTEM_UTILS_IP_TUNNEL_H_
#define SYSTEM_UTILS_IP_TUNNEL_H_

#ifdef HAVE_UTILS_TUNNEL

/* mode */
#define TUNNEL_GRE	1
#define TUNNEL_IPIP	2
#define TUNNEL_VTI	3
#define TUNNEL_SIT	4


/* active */
#define TUNNEL_ACTIVE	1



extern int utils_tunnel_cmd_init (void);
extern int tunnel_interface_config_write (struct vty *vty, struct interface *ifp);
extern int no_tunnel_interface(struct vty *vty, const char *ifname);
#endif

#endif /* SYSTEM_UTILS_IP_TUNNEL_H_ */
