## Process this file with automake to produce Makefile.in.

SUBDIRS = lib @ZEBRA@ @BGPD@ @RIPD@ @RIPNGD@ @OSPFD@ @OSPF6D@ @BABELD@ \
         @ISISD@ @PIMD@ @WATCHQUAGGA@ @VTYSH@ @OSPFCLIENT@ @DOC@ m4 @pkgsrcdir@ \
         redhat @SOLARIS@ tests @FPMD@  @OLSRD@ @ICRPD@ @FRPD@ @AODVD@ @VRRPD@ \
         @LLDPD@  @VPND@ @L2TPD@ @BFDD@ @LDPD@ @RSVPD@  @MPLSD@ @UTILS@ @IMISHD@

DIST_SUBDIRS = lib zebra bgpd ripd ripngd ospfd ospf6d babeld \
	  isisd watchquagga vtysh ospfclient doc m4 pkgsrc redhat tests \
	  solaris pimd fpm olsrd icrpd frpd adovd vrrpd lldpd \
      vpnd vpnd/l2tp bfdd ldpd rsvpd mplsd system-utils imi-cli

EXTRA_DIST = aclocal.m4 SERVICES TODO REPORTING-BUGS INSTALL.quagga.txt \
	update-autotools \
	vtysh/Makefile.in vtysh/Makefile.am imi-cli/Makefile.in imi-cli/Makefile.am \
	tools/rrcheck.pl tools/rrlookup.pl tools/zc.pl \
	tools/zebra.el tools/multiple-bgpd.sh \
	fpm/fpm.h

if HAVE_LATEX

HACKING.pdf: HACKING.tex
	$(LATEXMK) -pdf $<

clean-local:
	-$(LATEXMK) -C HACKING.tex

endif

ACLOCAL_AMFLAGS = -I m4
