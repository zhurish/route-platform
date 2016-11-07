#ifndef __ZEBRA_VRRPD_ZEBRA_H__
#define __ZEBRA_VRRPD_ZEBRA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ZVRRPD_ON_ROUTTING
extern int zvrrp_zclient_init(struct zvrrp_master * zvrrp);
extern int zvrrp_zclient_uninit(struct zvrrp_master * zvrrp);
#endif// ZVRRPD_ON_ROUTTING



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif    /* __ZEBRA_VRRPD_ZEBRA_H__ */

