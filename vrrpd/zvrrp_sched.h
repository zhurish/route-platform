#ifndef __ZEBRA_VRRPD_SCHED_H__
#define __ZEBRA_VRRPD_SCHED_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (ZVRRPD_OS_TYPE == ZVRRPD_ON_LINUX)
#define	zvrrpsleep(mSec) usleep(mSec)
#elif (ZVRRPD_OS_TYPE == ZVRRPD_ON_VXWORKS)
//#define	zvrrpsleep(mSec) taskDelay(mSec)
#define	zvrrpsleep(mSec) \
	{\
		struct timespec rqtp;\
		rqtp.tv_sec	= mSec / 1000;\
		rqtp.tv_nsec	= (mSec % 1000) * 1000;\
		nanosleep(&rqtp, NULL);\
	}
#endif//

#ifndef ZVRRPD_ON_ROUTTING

#define ZVRRPD_SCHED_CMD	1
#define ZVRRPD_SCHED_FUNC	2
#define ZVRRPD_SCHED_TIME	3
#define ZVRRPD_SCHED_WRITE	4
#define ZVRRPD_SCHED_READ	5
#define ZVRRPD_SCHED_EVENT	6

#define ZVRRPD_SCHED_MAX	16

typedef	struct	zvrrp_sched_
{
	int		used;
	int 	type;
	int 	cmd;
	int		(*func)(void *);
	void	*data;
	int		value;
	struct timeval next_timer;
}zvrrp_sched;

typedef	struct	zvrrp_sched_master_
{
	void *master;
	zvrrp_sched sched_cmd[ZVRRPD_SCHED_MAX];	
	zvrrp_sched sched_func[ZVRRPD_SCHED_MAX];	
	zvrrp_sched sched_timer[ZVRRPD_SCHED_MAX];
	zvrrp_sched sched_write[ZVRRPD_SCHED_MAX];
	zvrrp_sched sched_read[ZVRRPD_SCHED_MAX];
	zvrrp_sched sched_event[ZVRRPD_SCHED_MAX];	
	
}zvrrp_sched_master;



extern int zvrrp_gettimeofday(struct timeval *tv, int opt);

extern void *zvrrpsemcreate(int full);
extern int	zvrrpsemgive(void	*pSem);
extern int	zvrrpsemtake(void *pSem, int timeout);
extern int	zvrrpsemdelete(void *pSem);

extern zvrrp_sched_master * zvrrputilschedinit(void);
extern int zvrrputilschedclean(zvrrp_sched_master * master);
extern int zvrrputilschedcancel(zvrrp_sched_master * master, zvrrp_sched * pSched);
extern int zvrrputilschedadd(zvrrp_sched_master * master, int (*func)(void *), void *param, int size);
extern int zvrrputilschedaddread(zvrrp_sched_master * master, int (*func)(void *), void *param, int size);
extern int zvrrputilschedaddtimer(zvrrp_sched_master * master, int (*func)(void *), void *param, int mtimer);
extern int zvrrputilschedcmdadd(zvrrp_sched_master * master, int (*func)(void *), void *param);

extern int zvrrputilsched(zvrrp_sched_master *master, zvrrp_sched * sched);

#endif//ZVRRPD_ON_ROUTTING

extern int zvrrp_cmd_init(struct zvrrp_master * zvrrp);
extern int zvrrp_cmd_uninit(struct zvrrp_master * zvrrp);
//配置命令
extern int zvrrp_cmd_vrrp(int type, int vrid);//创建备份组
extern int zvrrp_cmd_enable(int type, int vrid);//使能备份组
extern int zvrrp_cmd_vip(int type, int vrid, long ip);//为备份组增加虚拟IP地址
extern int zvrrp_cmd_interface(int type, int vrid, char *ifname);//把接口加入备份组
extern int zvrrp_cmd_set_adev_time(int type, int vrid, int sec);//为备份组设置定时器
extern int zvrrp_cmd_set_adev_mtime(int type, int vrid, int msec);//为备份组设置定时器
extern int zvrrp_cmd_priority(int type, int vrid, int pri);//为备份组设置定优先级
extern int zvrrp_cmd_ping(int type);//使能ping功能
extern int zvrrp_cmd_learn_master(int type, int vrid);//为备份组使能学习功能
extern int zvrrp_cmd_preempt(int type, int vrid);//配置是否使能抢占模式
extern int zvrrp_cmd_preempt_delay(int type, int vrid, int value);//配置Master路由器Down时候backup路由器抢占延时时间
extern int zvrrp_cmd_track(int type, int vrid, int id, char *ifname, int vlaue);
extern int zvrrp_cmd_show(int vrid);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif    /* __ZEBRA_VRRPD_SCHED_H__ */

