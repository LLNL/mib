#*****************************************************************************\
#*  $Id: mib.spec.in,v 1.8 2005/11/30 20:24:57 auselton Exp $
#*****************************************************************************
#*  Copyright (C) 2001-2002 The Regents of the University of California.
#*  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#*  Written by Andrew Uselton <uselton2@llnl.gov>
#*  UCRL-CODE-222725
#*  
#*  This file is part of Mib, an MPI-based parallel I/O benchamrk
#*  For details, see <http://www.llnl.gov/linux/mib/>.
#*  
#*  Mib is free software; you can redistribute it and/or modify it under
#*  the terms of the GNU General Public License (as published by the Free
#*  Software Foundation version 2, dated June 1991.
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
#
# Sources in, for instance, $HOME/src/mib, so 
# See the hints in the script $HOME/src/mib/tools/mibrpm.sh 
#

Summary: An MPI-based I/O test for beowulf-style clusters
Name: mib
Version: 1.9.8
Release: 1chaos
Copyright: GPL
Group: Applications/System
Source:  %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
Prefix: /
%description
An MPI-based I/O test for beowulf-style clusters.  

%package tools
Summary: Tools to assist with mib 
Group: Applications/System
%description tools 
Shell, Perl, and Python scripts to assist with analyzing mib results
	and setting up for mib tests.

%pre tools
# /var/lustredata is intended to be a separate large partition dedicated
# to the results of auto-test operation.  This check was to prevent 
# setting up a lustredta directory when ther eis no partition.
# We've decided to just go ahead and install and hope the mib-tools user
# doesn't do anything stupid with auto-test output, like fill up the
# root partition.
#[ -d $INSTALLPREFIX/var/lustredata ] || { echo "no lustredata directory"; exit 1; }


%prep
%setup -q

%build
make
make doc

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=$RPM_BUILD_ROOT make -e install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc /usr/share/doc/%{name}-%{version}

/usr/bin/mib
/usr/man/man1/*

%files tools
%defattr(-,root,root)
/var/lustredata/scripts/*



%changelog
* Wed Aug  2 2006 Andrew C. Uselton <auselton@alci> 
- Allow mib-tools to be installed even if lustredata directory is missing.
* Fri Mar 31 2006 Andrew C. Uselton <auselton@alci> 
- Don't install hullo-lustre except as a regular old tool
* Mon Mar 27 2006 Andrew C. Uselton <auselton@alci> 
- Fixed build system to do documentation correctly
* Thu Mar 9 2006 Andrew C. Uselton <auselton@alci> 
- Substantial changes, relocated to subversion, rewritten from scratch
* Thu Feb 10 2005 Andrew C. Uselton <auselton@alci> 
- Initial build.


