/*
 * lldpd.h
 *
 *  Created on: Oct 25, 2016
 *      Author: zhurish
 */

#ifndef LLDPD_LLDPD_H_
#define LLDPD_LLDPD_H_

#define LLDP_DEBUG
//#define LLDP_DEBUG_TEST

#define LLDP_VTY_PORT 			2617
#define LLDP_DEFAULT_CONFIG 	"lldpd.conf"

#define LLDP_STR	"lldp config\n"

#define LLDP_TIMER_DEFAULT		5//60

#define LLDP_TTL_DEFAULT		120

#define LLDP_PACKET_MAX_SIZE	ZEBRA_MAX_PACKET_SIZ

#define LLDP_SNAP_FRAME_HEAD 	{0XAA, 0XAA, 0X03, 0X00, 0X00, 0X00, 0X88, 0XCC}


struct lldpd
{
	//char *system_name;
	//char *system_desc;
	//char *mgt_address;
	int enable;
};
extern struct thread_master *master;
extern struct lldpd *lldpd_config;


//lldp_zebra.c
extern void lldp_zclient_init (void);

//lldpd.c
extern int lldp_config_init(void);
extern int lldp_timer(struct thread *thread);
extern int lldp_interface_enable(struct interface *ifp);
extern int lldp_interface_enable_update(struct interface *ifp);

//lldp_write.c
extern int lldp_write(struct thread *thread);
//lldp_read.c
extern int lldp_read(struct thread *thread);


#ifdef LLDP_DEBUG
#define LLDP_DEBUG_LOG(format, args...)	\
	fprintf(stderr, "%s:",__func__); \
	fprintf(stderr, format, ##args); \
	fprintf(stderr, "\n");
#else
#define LLDP_DEBUG_LOG(format, args...)
#endif


#endif /* LLDPD_LLDPD_H_ */
