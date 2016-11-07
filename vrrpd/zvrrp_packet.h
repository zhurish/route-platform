#ifndef __ZEBRA_VRRPD_PKT_H__
#define __ZEBRA_VRRPD_PKT_H__

#ifdef __cplusplus
extern "C" {
#endif 

#define MAC_ETH_LEN		sizeof(struct ether_header)


#pragma pack(1)
typedef struct {    /* rfc2338.5.1 */
    uint8_t        vers_type;    /* 0-3=type, 4-7=version */
    uint8_t        vrid;         /* virtual router id */
    uint8_t        priority;     /* router priority */
    uint8_t        naddr;        /* address counter */
    uint8_t        auth_type;    /* authentification type */
    uint8_t        adver_int;    /* advertissement interval(in sec) */
    uint16_t       chksum;       /* checksum (ip-like one) */
/* here <naddr> ip addresses */
/* here authentification infos */
} vrrp_pkt;
#pragma pack(0)


extern int zvrrp_socket_init(int pvoid);


extern int zvrrp_reading_thread(void *p);

//检测是不是自己发送的数据包
extern int zvrrp_local_myself_detection(const long srcip);
extern vrrp_rt * zvrrp_vsrv_find_by_pkt_id(struct ip * iph);
extern vrrp_rt * zvrrp_vsrv_find_by_pkt_ip(struct ip * iph);
extern int zvrrp_handle_on_state(vrrp_rt *pVsrv, const char *buff);
extern int zvrrp_send_adv( vrrp_rt *vsrv, int prio );

extern int send_gratuitous_arp( vrrp_rt *vsrv, int addr, int vAddrF );






#ifdef __cplusplus
}
#endif 

#endif    /* __ZEBRA_VRRPD_PKT_H__ */

