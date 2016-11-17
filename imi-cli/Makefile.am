## Process this file with Automake to create Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_builddir)/lib
DEFS = @DEFS@ -DSYSCONFDIR=\"$(sysconfdir)/\"

LIBS = @LIBS@ @CURSES@ @LIBPAM@

AM_CFLAGS = $(PICFLAGS)
AM_LDFLAGS = $(PILDFLAGS)
bin_PROGRAMS = imish

imish_SOURCES = imi-sh.c imi-sh_client.c imi-sh_config.c imi-sh_kernel.c imi-sh_log.c imi-sh_main.c 
noinst_HEADERS = imi-sh.h imi-sh_kernel.h imi-sh_log.h

nodist_imish_SOURCES = imi-sh_cmd.c
CLEANFILES = imi-sh_cmd.c

imish_LDADD = ../lib/libzebra.la @LIBCAP@ @LIBREADLINE@

examplesdir = $(exampledir)
dist_examples_DATA = imi-sh.conf.sample

if OLSRD
EXT_OLSR = $(top_srcdir)/olsrd/*.c
endif

if FRPD
EXT_FRP = $(top_srcdir)/frpd/*.c
endif
if VRRPD
EXT_VRRP = $(top_srcdir)/vrrpd/*.c
endif 

if LLDPD
EXT_LLDP = $(top_srcdir)/lldpd/*.c
endif 

EXT_SRC=$(EXT_OLSR) $(EXT_FRP) $(EXT_VRRP) $(EXT_LLDP)

EXTRA_DIST = extract.pl

imish_cmd_FILES = $(top_srcdir)/bgpd/*.c $(top_srcdir)/isisd/*.c \
		  $(top_srcdir)/ospfd/*.c $(top_srcdir)/ospf6d/*.c \
		  $(top_srcdir)/ripd/*.c $(top_srcdir)/ripngd/*.c \
		  $(top_srcdir)/babeld/*.c \
		  $(top_srcdir)/pimd/pim_cmd.c $(EXT_SRC) \
		  $(top_srcdir)/lib/keychain.c $(top_srcdir)/lib/routemap.c \
		  $(top_srcdir)/lib/filter.c $(top_srcdir)/lib/plist.c \
		  $(top_srcdir)/lib/distribute.c $(top_srcdir)/lib/if_rmap.c \
		  $(top_srcdir)/lib/vty.c \
		  $(top_srcdir)/zebra/debug.c \
		  $(top_srcdir)/zebra/interface.c \
		  $(top_srcdir)/zebra/irdp_interface.c \
		  $(top_srcdir)/zebra/rtadv.c $(top_srcdir)/zebra/zebra_vty.c \
		  $(top_srcdir)/zebra/zserv.c $(top_srcdir)/zebra/router-id.c \
		  $(top_srcdir)/zebra/zebra_routemap.c \
	      $(top_srcdir)/zebra/zebra_fpm.c 

imi-sh_cmd.c: $(imish_cmd_FILES)
	./$(EXTRA_DIST) $(imish_cmd_FILES) > imi-sh_cmd.c
	
