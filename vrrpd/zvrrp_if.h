#ifndef __ZEBRA_VRRPD_IF_H__
#define __ZEBRA_VRRPD_IF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef ZVRRPD_ON_ROUTTING 

#define INTERFACE_NAMSIZ 32
#define INTERFACE_HWADDR_MAX 32
#define IPV4_MAX_PREFIXLEN 32;
/* Interface structure */
struct interface 
{
	struct interface *next;
	struct interface *prev;	
	char name[INTERFACE_NAMSIZ + 1];
	unsigned int ifindex;
	/* Interface flags. */
	uint64_t flags;
	/* Hardware address. */
	unsigned short hw_type;
	u_char hw_addr[INTERFACE_HWADDR_MAX];
	int hw_addr_len;
	/* Connected address list. */
	void *connected;
};

/* IPv4 and IPv6 unified prefix structure. */
struct prefix
{
  u_char family;
  u_char prefixlen;
  union 
  {
    u_char prefix;
    struct in_addr prefix4;
#ifdef HAVE_IPV6
    struct in6_addr prefix6;
#endif /* HAVE_IPV6 */
    u_char val[8];
  } u __attribute__ ((aligned (8)));
};
#endif//ZVRRPD_ON_ROUTTING

typedef struct vrrp_if_ {    /* parameters per interface -- rfc2338.6.1.1 */
	struct interface *ifp;//�����ַ��ָ��Ľӿ�
	struct prefix *address;
    char        hwaddr[6];    /* WORK: lame hardcoded for ethernet !!!! */
	int sock;
#ifdef ZVRRPD_ON_ROUTTING
	struct thread *zvrrp_timer;
	struct thread *zvrrp_read;
#endif
} vrrp_if;

extern struct interface * zif_lookup_by_name(const char *ifname);
extern struct prefix *zif_prefix_get_by_name (struct interface *ifp);


extern vrrp_if * zvrrp_if_lookup_by_ifindex(int ifindex);
extern int vrrp_interface_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif    /* __ZEBRA_VRRPD_IF_H__ */

