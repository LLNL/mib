#*****************************************************************************\
#*  $Id: mib.spec,v 1.8 2005/11/30 20:24:57 auselton Exp $
#*****************************************************************************
#*  Copyright (C) 2001-2002 The Regents of the University of California.
#*  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
#*  Written by Andrew Uselton <uselton2@llnl.gov>
#*  UCRL-CODE-2006-xxx.
#*  
#*  This file is part of Mib, an MPI-based parallel I/O benchamrk
#*  For details, see <http://www.llnl.gov/linux/mib/>.
#*  
#*  Mib is free software; you can redistribute it and/or modify it under
#*  the terms of the GNU General Public License as published by the Free
#*  Software Foundation; either version 2 of the License, or (at your option)
#*  any later version.
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
# cd $HOME/src/mib
# make $HOME/rpm and its subdirs BUILD, RPMS, SPECS, SOURCES, and SRPMS then
# The source RPM ends up in SRPMS and the binary in RPMS/<arch>
# make $HOME/tmp, so temporary RPM build scripts have a place to live
# Put this spec file in SPECS and %{name}-%{version}.tar.gz in SOURCES
# then build with
# rpmbuild -ba --define "_tmppath $HOME/tmp" --define "_topdir $HOME/rpm/" mib.spec
#

Summary: An MPI-based I/O test for beowulf-style clusters
Name: mib
Version: 1.5
Release: chaos
Copyright: GPL
Group: Applications/System
Source:  %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot

%description
An MPI-based I/O test for beowulf-style clusters.  

%prep
%setup -q

%build
make mpiio

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
#mkdir -p $RPM_BUILD_ROOT/usr/man/man1

install -m 755 mib $RPM_BUILD_ROOT/usr/bin/mib
#install -m 644 mib.1 $RPM_BUILD_ROOT/usr/man/man1/mib.1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README META

/usr/bin/mib
#/usr/man/man1/mib.1


%changelog
* Thu Feb 10 2005 Andrew C. Uselton <auselton@alci> 
- Initial build.


