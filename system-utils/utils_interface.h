/*
 * utils_interface.h
 *
 *  Created on: Oct 16, 2016
 *      Author: zhurish
 */

#ifndef SYSTEM_UTILS_UTILS_INTERFACE_H_
#define SYSTEM_UTILS_UTILS_INTERFACE_H_

struct utils_interface
{
	int ifindex;//接口索引

	char name[INTERFACE_NAMSIZ + 1];//接口名称
	/* Interface flags. */
	uint64_t flags;//接口flags

	int type;//接口类型，tunnel，vlan，bridge

#ifdef HAVE_UTILS_BRCTL
	int br_mode;//接口状态，网桥，桥接接口
	char br_name[INTERFACE_NAMSIZ + 1];//桥接接口的网桥名称
	int br_stp;//网桥生成树
	int br_stp_state;//生成树状态
	int max_age;
	int hello_time;
	int forward_delay;
	//
#define BR_PORT_MAX	32
	int br_ifindex[BR_PORT_MAX];
#endif
	//int message_age_timer_value;
	//int forward_delay_timer_value;
	//int hold_timer_value;
#ifdef HAVE_UTILS_TUNNEL
	int tun_index;//tunnel接口的隧道ID编号
	int tun_mode;//隧道模式
	int tun_ttl;//change: ip tunnel change tunnel0 ttl
	int tun_mtu;//change: ip link set dev tunnel0 mtu 1400
    struct in_addr source;//ip tunnel change tunnel1 local 192.168.122.1
    struct in_addr remote;//change: ip tunnel change tunnel1 remote 19.1.1.1
    int active;//隧道接口是否激活
#endif
#ifdef HAVE_UTILS_VLAN
#define SKB_QOS_MAX	8
    char vlan_base_name[INTERFACE_NAMSIZ + 1];//vlan接口的父接口
	int vlanid;//vlan id
	int egress_vlan_qos[SKB_QOS_MAX];//出口skb的优先级到qos的映射
	int ingress_vlan_qos[SKB_QOS_MAX];//入口skb的优先级到qos的映射
	//unsigned int flag; /* Matches vlan_dev_priv flags */
#endif
	struct vty *vty;
	struct interface *ifp;
};

/*type*/
#define UTILS_IF_BRIDGE (1)
#define UTILS_IF_TUNNEL (2)
#define UTILS_IF_VLAN (3)




extern int if_get_ifindex (struct utils_interface *ifp);
extern int utils_interface_status (struct utils_interface *ifp, int value);

extern struct utils_interface * utils_interface_lookup_by_name (const char *name);
extern struct utils_interface * utils_interface_lookup_by_ifindex (int index);
extern struct utils_interface *utils_interface_get_by_name (const char *name);
extern struct utils_interface * utils_interface_lookup (const char *name, int index);
extern struct utils_interface * utils_interface_create (const char *name);
extern void utils_interface_delete (struct utils_interface *ifp);
extern int show_utils_interface(struct vty *vty, struct interface *ifp);

extern int utils_interface_init(void);

#endif /* SYSTEM_UTILS_UTILS_INTERFACE_H_ */
