PROJECT       := mib
VERSION       := $(shell awk '/[Vv]ersion:/ {print $$2}' META)
RELEASE       := $(shell awk '/[Rr]elease:/ {print $$2}' META)
SVNURL        := https://eris.llnl.gov/svn/lustre-utils/$(PROJECT)
BUILDURL      := https://eris.llnl.gov/svn/buildfarm/trunk/build
TRUNKURL      := $(SVNURL)/trunk
TAGURL        := $(SVNURL)/tags/$(PROJECT)-$(VERSION)-$(RELEASE)

SHELL=   /bin/sh
MAKE=    /usr/bin/make
INSTALL= /usr/bin/install
mkinstalldirs=	 $(SHELL) ./aux/mkinstalldirs -m 755

lustre		= /usr/lustre
lustre_bin	= ${lustre}/bin
mandir 		= /usr/share/man/man1
lustre_scripts	= ${lustre}/scripts

all: 
	$(MAKE) -C src

install:
	$(mkinstalldirs) 			$(DESTDIR)$(lustre_bin)
	$(INSTALL) src/mib			$(DESTDIR)$(lustre_bin)/
	$(mkinstalldirs)			$(DESTDIR)$(mandir)
	$(INSTALL) doc/mib.1 -m 644		$(DESTDIR)$(mandir)/
	$(mkinstalldirs)			$(DESTDIR)$(lustre_scripts)
	$(INSTALL) scripts/composite.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/diff.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/file-layout.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/hullo-lustre.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/lfind-dist1.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/lfind-dist2.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/lfind-dist3.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/lfinding.sh	$(DESTDIR)$(lustre_scripts)/
#	$(INSTALL) scripts/lwatch.pl		$(DESTDIR)$(lustre_scripts)/
#	$(INSTALL) scripts/lwatch.py		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/mib-test.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/mib-test.transcript $(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/mib.sh		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/profile.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/rm-files.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) scripts/viewcalls.pl	$(DESTDIR)$(lustre_scripts)/

clean: 
	rm -f *.rpm
	$(MAKE) -C src clean
distclean: clean
	rm -f build
	rm -f *.rpm *.bz2

rpms-working: build
	make clean
	./build --snapshot .
rpms-trunk: build
	./build --snapshot $(TRUNKURL)
rpms-release: build
	./build --nosnapshot $(TAGURL)
tagrel:
	svn copy $(TRUNKURL) $(TAGURL)
build:
	svn cat $(BUILDURL) >$@
	chmod +x $@

