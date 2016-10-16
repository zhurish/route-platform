# Process this file with automake to produce Makefile.in.
# This is for RSVP-TE Daemon.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

LCF=-DDEBUG_PPPD -DTRUST_PPPD_TO_DIE -DSANITY -DLINUX -DUSE_KERNEL -DIP_ALLOCATION -fPIC

AM_CFLAGS = $(PICFLAGS) $(LCF)
AM_LDFLAGS = $(PILDFLAGS)

sbin_PROGRAMS = l2tpd l2tpctl

l2tpd_SOURCES = \
	xl2tpd.c pty.c misc.c control.c avp.c call.c network.c \
	avpsend.c scheduler.c file.c aaa.c md5.c
	
l2tpctl_SOURCES =	xl2tpd-control.c

noinst_HEADERS = \
	l2tp.h avp.h misc.h control.h call.h scheduler.h file.h aaa.h md5.h

l2tpd_LDADD = -lm

examplesdir = $(exampledir)
dist_examples_DATA = l2tpd.conf.sample l2tp-secrets.sample

