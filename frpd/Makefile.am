## Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS) -g
AM_LDFLAGS = $(PILDFLAGS)

noinst_LIBRARIES = libfrp.a
sbin_PROGRAMS = frpd

libfrp_a_SOURCES = EigrpUtil.c Eigrpd.c EigrpDual.c \
	EigrpIp.c EigrpMain.c EigrpCmd.c EigrpPacket.c \
	EigrpIntf.c EigrpSysPort.c EigrpZebra.c \
	EigrpSnmp.c EigrpSysPortRouterSnmp.c

noinst_HEADERS = include/EigrpPreDefine.h include/EigrpDef.h include/EigrpDual.h \
	include/Eigrpd.h include/EigrpIp.h include/EigrpMain.h include/EigrpCmd.h include/EigrpIntf.h \
	include/EigrpPacket.h include/EigrpSysPort.h  include/EigrpZebra.h \
	include/EigrpUtil.h include/EigrpSnmp.h include/EigrpSysPortRouterSnmp.h

frpd_SOURCES = EigrpInit.c $(libfrp_a_SOURCES)

frpd_LDADD = ../lib/libzebra.la @LIBCAP@ -lm





