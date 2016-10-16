## Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS)
AM_LDFLAGS = $(PILDFLAGS)


noinst_LIBRARIES = libolsr.a
sbin_PROGRAMS = olsrd

libolsr_a_SOURCES = \
	olsrd.c olsr_zebra.c olsr_interface.c olsr_vty.c \
	olsr_packet.c olsr_linkset.c olsr_time.c olsr_neigh.c \
	olsr_dup.c olsr_mpr.c olsr_debug.c olsr_route.c

noinst_HEADERS = \
	olsrd.h olsr_zebra.h olsrd_version.h olsr_interface.h olsr_vty.h \
	olsr_packet.h olsr_linkset.h olsr_time.h olsr_neigh.h \
	olsr_dup.h olsr_mpr.h olsr_debug.h olsr_route.h

olsrd_SOURCES = \
	olsr_main.c $(libolsr_a_SOURCES)

olsrd_LDADD = ../lib/libzebra.la @LIBCAP@

man_MANS = olsrd.8

examplesdir = $(exampledir)
dist_examples_DATA = olsrd.conf.sample


