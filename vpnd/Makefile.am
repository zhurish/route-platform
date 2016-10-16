# Process this file with automake to produce Makefile.in.
# This is for RSVP-TE Daemon.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS)
AM_LDFLAGS = $(PILDFLAGS)

noinst_LIBRARIES = libvpnd.a
sbin_PROGRAMS = vpnd
libvpnd_a_SOURCES = ip_tunnel.c vpn_zebra.c

noinst_HEADERS = l2tp/l2tp.h

vpnd_SOURCES = main.c $(libvpnd_a_SOURCES)
vpnd_LDADD = ../lib/libzebra.la @LIBCAP@



