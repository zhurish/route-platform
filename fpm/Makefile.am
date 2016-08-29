## Process this file with automake to produce Makefile.in.

AM_CPPFLAGS = -I.. -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"
INSTALL_SDATA=@INSTALL@ -m 600

AM_CFLAGS = $(PICFLAGS)
AM_LDFLAGS = $(PILDFLAGS)


noinst_LIBRARIES = libfpm.a
sbin_PROGRAMS = fpmd

libfpm_a_SOURCES = fpm_socket.c fpm_handle.c

noinst_HEADERS = fpm_socket.h fpm.h

fpmd_SOURCES = fpm_main.c $(libfpm_a_SOURCES)

fpmd_LDADD = ../lib/libzebra.la @LIBCAP@



