#*****************************************************************************\
#*  $Id: Makefile.in,v 1.9 2005/11/30 20:24:57 auselton Exp $
#*****************************************************************************
#*  Copyright (C) 2006 The Regents of the University of California.
#*  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#*  Written by Andrew Uselton <uselton2@llnl.gov>
#*  UCRL-CODE-2006-xxx.
#*  
#*  This file is part of Mib, an MPI-based parallel I/O benchamrk
#*  For details, see <http://www.llnl.gov/linux/mib/>.
#*  
#*  Mib is free software; you can redistribute it and/or modify it under
#*  the terms of the GNU General Public License (as published by the Free
#*  Software Foundation) version 2, dated June 1991.
#*  
#*  Mib is distributed in the hope that it will be useful, but WITHOUT 
#*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
#*  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
#*  for more details.
#*  
#*  You should have received a copy of the GNU General Public License along
#*  with Mib; if not, write to the Free Software Foundation, Inc.,
#*  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
#*****************************************************************************/

PROJECT       := mib
VERSION       := $(shell awk '/[Vv]ersion:/ {print $$2}' META)
RELEASE       := $(shell awk '/[Rr]elease:/ {print $$2}' META)
SVNURL        := https://eris.llnl.gov/svn/lustre-utils/$(PROJECT)
TRUNKURL      := $(SVNURL)/trunk
TAGURL        := $(SVNURL)/tags/$(PROJECT)-$(VERSION)-$(RELEASE)

SHELL=   /bin/sh
MAKE=    /usr/bin/make
CC = %CC%
INSTALL= /usr/bin/install -c
mkinstalldirs=	 $(SHELL) ./aux/mkinstalldirs

MPI_CCFLAGS = -DUSE_STDARG -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDARG_H=1 -DUSE_STDARG=1 -DMALLOC_RET_VOID=1 
CCFLAGS = ${MPI_CCFLAGS}  -Wextra -g -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -rdynamic
# I don't think it turned out to need the following:
LDFLAGS = #-Wl,-rpath-link -Wl,/usr/lib/mpi/mpi_intel/lib 
LIBDIRS = #-L/usr/lib/mpi/mpi_intel/lib 
LIBS = %LIBS%
MPI_INCDIR = -I/usr/lib/mpi/include
INCDIR = ${MPI_INCDIR}
DEBUG = # Put 'DEBUG="-DDEBUG_CODE=1"' on the command line if you want 
	# debugging, or you can put it here if you really want to be sure
	# and always have the debugging on.  N.B. I don't think it even
	# works right now.

lustre		= /usr/lustre
lustre_bin	= ${lustre}/bin
lustre_man	= /usr/man/man1
lustre_scripts	= ${lustre}/scripts

all: 
	$(MAKE) -C src

install:
	$(mkinstalldirs) 			$(DESTDIR)$(lustre_bin)
	$(INSTALL) src/mib			$(DESTDIR)$(lustre_bin)/
	$(mkinstalldirs)			$(DESTDIR)$(lustre_man)
	$(INSTALL) src/mib.1 -m 644		$(DESTDIR)$(lustre_man)/
	$(mkinstalldirs)			$(DESTDIR)$(lustre_scripts)
	$(INSTALL) src/tools/composite.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/diff.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/file-layout.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/hullo-lustre.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfind-dist1.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfind-dist2.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfind-dist3.pl	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lfinding.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lwatch.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/lwatch.py		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/mib-test.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/mib-test.transcript $(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/mib.sh		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/profile.pl		$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/rm-files.sh	$(DESTDIR)$(lustre_scripts)/
	$(INSTALL) src/tools/viewcalls.pl	$(DESTDIR)$(lustre_scripts)/

clean: 
	rm -rf $(PROJECT)+chaos
	rm -f *.rpm
	$(MAKE) -C src clean
	rm -f *.o mib *~

apply-patches quilt: $(PROJECT)+chaos

$(PROJECT)+chaos: patches/series
	make clean
	@scripts/quilt.sh $(QUILTFLAGS)

check-vars:
	@echo "Release:  $(PROJECT)-$(VERSION)-$(RELEASE)"
	@echo "Trunk:    $(TRUNKURL)"
	@echo "Tag:      $(TAGURL)"

rpms-working:
	make clean
	@scripts/build --snapshot .

rpms-trunk: check-vars
	@scripts/build --snapshot $(TRUNKURL)

rpms-release: check-vars
	@scripts/build --nosnapshot $(TAGURL)

tagrel:
	svn copy $(TRUNKURL) $(TAGURL)
