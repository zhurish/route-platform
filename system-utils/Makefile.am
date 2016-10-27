# Process this file with automake to produce Makefile.in.
# This is for RSVP-TE Daemon.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS)
AM_LDFLAGS = $(PILDFLAGS)

noinst_LIBRARIES = libutils.a
sbin_PROGRAMS = utilsd

libutils_a_SOURCES = utils.c utils_interface.c utils_zebra.c \
	ip_tunnel.c ip_brctl.c ip_vlan.c ip_dhcp.c ip_firewall.c \
	ip_iptables.c
	
noinst_HEADERS = \
	utils.h utils_interface.h ip_tunnel.h ip_vlan.h


utilsd_SOURCES = main.c $(libutils_a_SOURCES)
utilsd_LDADD = ../lib/libzebra.la @LIBCAP@



