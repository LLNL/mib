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
	$(INSTALL) src/mib-serial		$(DESTDIR)$(lustre_bin)/
	$(mkinstalldirs)			$(DESTDIR)$(mandir)
	$(INSTALL) src/mib.1 -m 644		$(DESTDIR)$(mandir)/
	$(mkinstalldirs)			$(DESTDIR)$(lustre_scripts)
	$(INSTALL) src/tools/composite.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/diff.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/file-layout.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/hullo-lustre.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfind-dist1.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfind-dist2.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfind-dist3.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfinding.sh	$(DESTDIR)$(lustre_scripts)/
#	$(INSTALL) src/tools/lwatch.pl		$(DESTDIR)$(lustre_scripts)/
#	$(INSTALL) src/tools/lwatch.py		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/mib-test.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/mib-test.transcript $(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/mib.sh		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/profile.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/rm-files.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/viewcalls.pl	$(DESTDIR)$(lustre_scripts)/

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

