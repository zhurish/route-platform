# Process this file with automake to produce Makefile.in.
# This is for RSVP-TE Daemon.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS)
AM_LDFLAGS = $(PILDFLAGS)

noinst_LIBRARIES = librsvpd.a
sbin_PROGRAMS = rsvpd

librsvpd_a_SOURCES = \
	jhash.c avl.c label_map.c labelmgr.c mpls_xc.c ternary.c \
	rsvpd.c rsvp_zebra.c rsvp_network.c rsvp_packet.c rsvp_if.c \
	rsvp_vty.c rsvp_lsp.c rsvp_obj.c rsvp_state.c rsvp_route.c \
	rsvp_tunnel.c

noinst_HEADERS = \
	jhash.h avl.h label_map.h labelmgr.h mpls_xc.h ternary.h \
	rsvpd.h rsvp_zebra.h rsvp_network.h rsvp_packet.h rsvp_if.h \
	rsvp_vty.h rsvp_lsp.h rsvp_obj.h rsvp_state.h rsvp_route.h \
	rsvp_tunnel.h

rsvpd_SOURCES = rsvp_main.c $(librsvpd_a_SOURCES)

rsvpd_LDADD = ../lib/libzebra.la @LIBCAP@

examplesdir = $(exampledir)
dist_examples_DATA = rsvpd.conf

