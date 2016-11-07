## Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS) -g
AM_LDFLAGS = $(PILDFLAGS)


sbin_PROGRAMS = vrrpd

vrrpd_SOURCES = zvrrpd_main.c zvrrpd.c zvrrp_if.c zvrrp_packet.c zvrrp_sched.c zvrrp_zebra.c



noinst_HEADERS = zvrrpd.h zvrrp_if.h zvrrp_packet.h zvrrp_sched.h zvrrp_zebra.h

vrrpd_LDADD = ../lib/libzebra.la @LIBCAP@