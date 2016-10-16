/* lib/version.h.  Generated from version.h.in by configure.
 *
 * Quagga version
 * Copyright (C) 1997, 1999 Kunihiro Ishiguro
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

#ifndef _ZEBRA_VERSION_H
#define _ZEBRA_VERSION_H

#ifdef GIT_VERSION
#include "gitversion.h"
#endif

#ifndef GIT_SUFFIX
#define GIT_SUFFIX ""
#endif
#ifndef GIT_INFO
#define GIT_INFO ""
#endif

#define QUAGGA_PROGNAME   "Quagga"

#define QUAGGA_VERSION     "0.99.24" GIT_SUFFIX

#define ZEBRA_BUG_ADDRESS "https://bugzilla.quagga.net"

#define QUAGGA_URL "http://www.quagga.net"

#define QUAGGA_COPYRIGHT "Copyright 1996-2005 Kunihiro Ishiguro, et al."

#define QUAGGA_CONFIG_ARGS "--prefix=/home/zhurish/workspace/route-platform/app/rt --localstatedir=/home/zhurish/workspace/route-platform/app/rt/var --build=i686-pc-linux-gnu --host=i686-pc-linux-gnu CC=gcc --enable-user=zhurish --enable-group=zhurish --enable-tcp-zebra --enable-zebra --disable-vtysh --enable-watchquagga --disable-babeld --disable-ripd --disable-pimd --disable-ospf6d --disable-ripngd --enable-ospfd --enable-ospfapi --enable-ospfclient --enable-ospf-dcn --enable-opaque-lsa --enable-ospf-te --enable-ldpd --enable-rsvpd --disable-bgpd --disable-isisd --disable-isis-topology --enable-vpnd --enable-fpm --disable-rtadv --disable-irdp --enable-configfile-mask=0664 --enable-logfile-mask=0664 --enable-imi=65530 LIBS=-lpthread"

pid_t pid_output (const char *);

#ifndef HAVE_DAEMON
int daemon(int, int);
#endif

#endif /* _ZEBRA_VERSION_H */
