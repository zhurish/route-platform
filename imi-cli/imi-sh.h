/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifndef IMISH_H
#define IMISH_H

#ifdef IMISH_IMI_MODULE



#define IMISH_ZEBRA  0x01
#define IMISH_RIPD   0x02
#define IMISH_RIPNGD 0x04
#define IMISH_OSPFD  0x08
#define IMISH_OSPF6D 0x10
#define IMISH_BGPD   0x20
#define IMISH_ISISD  0x40
#define IMISH_BABELD  0x80
#define IMISH_PIMD   0x100
#define IMISH_OLSRD   0x200
#define IMISH_VRRPD   0x400
#define IMISH_LLDPD   0x800
#define IMISH_HSLSD   0x1000
#define IMISH_FRPD   0x2000
#define IMISH_VPND   0x4000
#define IMISH_NSMD  IMISH_ZEBRA


#define IMISH_ALL	  IMISH_ZEBRA|IMISH_RIPD|IMISH_RIPNGD|IMISH_OSPFD|IMISH_OSPF6D|IMISH_BGPD|\
	IMISH_ISISD|IMISH_BABELD|IMISH_PIMD|IMISH_OLSRD|IMISH_HSLSD|IMISH_FRPD|IMISH_VRRPD|IMISH_LLDPD|IMISH_VPND
#define IMISH_RMAP	  IMISH_ZEBRA|IMISH_RIPD|IMISH_RIPNGD|IMISH_OSPFD|IMISH_OSPF6D|IMISH_BGPD|IMISH_BABELD
#define IMISH_INTERFACE	  IMISH_ZEBRA|IMISH_RIPD|IMISH_RIPNGD|IMISH_OSPFD|IMISH_OSPF6D|IMISH_ISISD|\
	IMISH_BABELD|IMISH_PIMD|IMISH_OLSRD|IMISH_HSLSD|IMISH_FRPD|IMISH_VRRPD|IMISH_LLDPD|IMISH_VPND

/* imish local configuration file. */
#define IMISH_DEFAULT_CONFIG "startup-config.conf"
/* imish local OEM information file. */
#define IMISH_OEM_DEFAULT "oem.bin"
/* imish local default password */
#define IMISH_PASSWORD_DEFAULT "imish"

//imi-sh.c
extern void imish_module_init (void);
extern void imish_module_exit (void);
extern int imish_module_connect_all (const char *optional_daemon_name);
extern int imish_module_client_put (struct vty *vty, const char *buf, int out);
//ִ��ĳ���ͻ���·�ɵ�����
extern int imish_module_execute (struct vty *vty, const char *line, int out);
extern int imish_module_client_execute(int client, const char *cmd, struct vty *vty, int out);
//imi-sh_config.c
extern void imish_module_config_init (void);
extern void imish_module_config_exit (void);
extern void imish_module_config_parse (char *);
extern void imish_module_config_format (const char *format, ...);
extern void imish_module_config_show (struct vty *vty);
extern int imish_read_config (char *config_default);//��ȡ�����ļ���ִ������
//imi-sh_cmd.c
extern void imish_module_init_cmd (void);


extern struct thread_master *master;

#endif /* IMISH_IMI_MODULE */


#endif /* IMISH_H */
