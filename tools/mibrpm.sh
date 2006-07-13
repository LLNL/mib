#!/bin/bash
#*****************************************************************************\
#*  $Id: mibrpm.sh 1.9 2005/11/30 20:24:57 auselton Exp $
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
# mibrpm.sh 
#   Automate the steps to build a mib rpm.

PROG=$(basename $0)

usage ()
{
	[ X"$1" == X ] || { echo "$1"; echo; } 
	echo "$PROG"
	exit 1
}

if [ X"$ARCH" == X ]
    then
    ARCH=`uname -m`
    [ X"$ARCH" == X ] && usage "Failed to detect architecture"
    [ "$ARCH" == "i686" -o "$ARCH" == "i586" -o "$ARCH" == "i486" ] && ARCH="i386"
fi
export ARCH
# We want to get /var/lustredata mounted on bgldev and ubgl, but it's 
# not there yet.
if [ X"$ARCH" != X"ppc64" -a X"$ARCH" != X"bgl" ]
then 
    INSTALL_TARGET="/var/lustredata"
    [ -d $INSTALL_TARGET ] || { echo "The install target $INSTALL_TARGET does not exist.  Are you sure you want to build here?"; exit 1; }
fi
mkdir -p $HOME/rmp/BUILD
mkdir -p $HOME/rmp/RPMS/$ARCH
mkdir -p $HOME/rmp/RPMS/pseries64
mkdir -p $HOME/rmp/SOURCES
mkdir -p $HOME/rmp/SPECS
mkdir -p $HOME/rmp/SRPMS
mkdir -p $HOME/rmp/TMP

cd $HOME/tmp
rm -rf mib
svn co https://eris.llnl.gov/svn/chaos/private/mib/trunk mib
cd mib
if ./configure 
then
    echo "configured"
else
    usage "config failed"
fi

VERSION=`grep VERSION META | cut -d':' -f2`
VERSION=${VERSION// /}
[ X"$VERSION" != X ] ||  usage "The META file did not yield a version"
cd ..
mkdir -p mib-$VERSION/aux
mkdir -p mib-$VERSION/tools
mkdir -p mib-$VERSION/doc/sample
cp mib/* mib-$VERSION/
rm mib-$VERSION/*.in
rm mib-$VERSION/configure
rm mib-$VERSION/META
cp mib/aux/* mib-$VERSION/aux
cp mib/tools/* mib-$VERSION/tools
cp mib/doc/* mib-$VERSION/doc
rm mib-$VERSION/doc/README
cp mib/doc/sample/* mib-$VERSION/doc/sample
rm mib-$VERSION/tools/mibrpm.sh
rm mib-$VERSION/tools/Makefile.in
rm mib-$VERSION/doc/Makefile.in
rm -rf mib
tar cvfz $HOME/rpm/SOURCES/mib-$VERSION.tar.gz mib-$VERSION
cp mib-$VERSION/mib.spec $HOME/rpm/SPECS
rm -rf mib-$VERSION
cd $HOME/rpm/SPECS
mkdir -p $HOME/rpm/TMP/mib-$VERSION
echo "rpmbuild -ba --define \"_tmppath $HOME/rpm/TMP/mib-$VERSION\" --define \"_topdir $HOME/rpm\" mib.spec "
rpmbuild -ba --define "_tmppath $HOME/rpm/TMP/mib-$VERSION" --define "_topdir $HOME/rpm" mib.spec 
