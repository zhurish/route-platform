/*
 * OLSR Rout(e)ing protocol
 *
 * Copyright (C) 2005        Tudor Golubenco
 *                           Polytechnics University of Bucharest 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public Licenseas published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
 * more details.

 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef _ZEBRA_OLSRD_H
#define _ZEBRA_OLSRD_H

#include "prefix.h"

/* 
   RFC 3626 3.1 Protocol and Port Number
   Port 698 has been assigned by IANA for exclusive usage by
   OLSR protocol
*/
#define OLSR_PORT_DEFAULT      698

/* VTY port number. */
/* 2016年7月3日 15:28:20 zhurish: 修改hell监听端口，原来是2611，这个端口和PIM路由协议重复 */
#define OLSR_VTY_PORT          2615
/* 2016年7月3日 15:28:20  zhurish: 修改hell监听端口，原来是2611，这个端口和PIM路由协议重复 */

/* Default configuration file name. */
#define OLSR_DEFAULT_CONFIG   "olsrd.conf"


/* FIXME: these should be set by configure */
/* 2016年7月3日 15:29:09 zhurish: 屏蔽这两个宏定义，这两个宏定义在config.h定义，由configure.ac文件自动生成 */
//#define PATH_OLSRD_PID "/var/run/olsrd.pid"
//#define OLSR_VTYSH_PATH "/tmp/.olsrd"
/* 2016年7月3日 15:29:09  zhurish: 屏蔽这两个宏定义，这两个宏定义在config.h定义，由configure.ac文件自动生成 */

/* Default OLSR emission intervals (RFC3626). */
#define OLSR_HELLO_INTERVAL_DEFAULT        2.0
#define OLSR_MID_INTERVAL_DEFAULT          5.0
#define OLSR_TC_INTERVAL_DEFAULT           5.0

/* Default OLSR holding intervals (RFC3626). */
#define OLSR_NEIGHB_HOLD_TIME_DEFAULT      6.0
#define OLSR_DUP_HOLD_TIME_DEFAULT        30.0 
#define OLSR_TOP_HOLD_TIME_DEFAULT        15.0 

#define OLSR_MPR_UPDATE_TIME_DEFAULT       1.0 
#define OLSR_RT_UPDATE_TIME_DEFAULT        2.0 

/* UDP receive buffer size */
#define OLSR_UDP_RCV_BUF 41600

/* FIXME: Normal OLSR packet min and max size. */
#define OLSR_PACKET_MINSIZE              16
#define OLSR_PACKET_MAXSIZE            1500 /* max packet size. */
#define OLSR_MSG_HDR_SIZE                12

#define OLSR_WILL_NEVER                   0
#define OLSR_WILL_LOW                     1
#define OLSR_WILL_DEFAULT                 3
#define OLSR_WILL_HIGH                    6
#define OLSR_WILL_ALWAYS                  7

#define OLSR_C_DEFAULT               0.0625 /* 1/16 seconds */

#ifndef TRUE 
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* 2016年7月3日 15:29:35 zhurish: 增加OLSR使用的链表查询等相关宏定义 */
#ifndef LIST_LOOP
#define LIST_LOOP(L,V,N)  for (ALL_LIST_ELEMENTS_RO (L,N,V))
#define stream_get_putp stream_get_endp 
#define stream_forward stream_forward_endp
#define nextnode(X) ((X) = (X)->next)
#define getdata(X) listgetdata(X)
#endif
/* 2016年7月3日 15:29:35  zhurish: 增加OLSR使用的链表查询等相关宏定义 */

/* OLSR master fro system wide configuration and variables */
struct olsr_master
{
  /* OLSR instance list. */
  struct list *olsr;
  
  /* OLSR thread master. */
  struct thread_master *master;
  
  /* Zebra interface list. */
  struct list *iflist;
  
  /* OLSR start time. */
  time_t start_time;

};


/* OLSR instance structure. */
struct olsr
{

  struct in_addr main_addr;	        /* Main router address.                */
  int main_addr_is_set;




  struct list *oiflist;                 /* Olsr interfaces.                    */

  /* Parameters. */
  u_int16_t port;		        /* Olsr port.                          */
  float C;			        /* OLSR Constant.                      */
  u_char will;			        /* Willingness.                        */
  float neighb_hold_time;	        /* Neighbor Hold Time.                 */
  float dup_hold_time;		        /* Duplicate Message Hold Time.        */
  float top_hold_time;		        /* Topology Hold Time.                 */
  float mpr_update_time;	        /* Delay updating mpr amount.          */
  float rt_update_time;	                /* Delay updating routing table amount.*/

  /* Global sequence numbers. */
  u_int16_t msn;		        /* Message Sequence Number.            */
  u_int16_t ansn;		        /* Advertised Neighbor Sequence Number.*/

  /* OLSR sets. */
  struct list *dupset;		        /* Duplicate Message set tuples.       */
  struct list *neighset;	        /* Neighbour set tuples.               */
  struct list *n2hopset;	        /* 2 Hop Neighbour set tuples.         */
  struct list *midset;		        /* Multiple Interface tuples.          */
  struct list *topset;		        /* Topology tuples.                    */

  struct list *advset;		        /* Advertised set of nodes.            */

  struct route_table *networks;         /* OLSR config networks.               */

  struct route_table *table;	        /* The routing table.                  */

  /* Threads. */
  struct thread *t_write;
  struct thread *t_read;
  
  struct thread *t_delay_stc;
  struct thread *t_mpr_update;
  struct thread *t_rt_update;

  int fd;

};


/* Extern variables. */
extern struct olsr_master *olm;
extern struct thread_master *master;
extern struct zebra_privs_t olsrd_privs;


/* Prototypes */
int olsr_network_set (struct olsr *olsr, struct prefix_ipv4 *p);
void olsr_network_run (struct olsr *olsr, struct prefix *p);
void olsr_if_update (struct olsr *olsr);

int
sockopt_broadcast (int sock);

void
olsr_main_addr_update (struct olsr *olsr);

struct olsr* olsr_lookup();
void olsr_add(struct olsr *olsr);
void olsr_delete(struct olsr *olsr);
struct olsr *olsr_get();

void olsr_master_init();

#endif
