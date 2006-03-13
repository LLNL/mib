#!/bin/bash
# mibrpm.sh 
#   Automate the steps to build a mib rpm.

INSTALL_TARGET="/var/lustredata"
[ -d $INSTALL_TARGET ] || { echo "The install target $INSTALL_TARGET does not exist.  Are you sure you want to build here?"; exit 1; }
mkdir -p $HOME/rmp/BUILD
mkdir -p $HOME/rmp/RPMS/i386
mkdir -p $HOME/rmp/RPMS/ia_64
mkdir -p $HOME/rmp/RPMS/x86_64
mkdir -p $HOME/rmp/RPMS/pseries64
mkdir -p $HOME/rmp/SOURCES
mkdir -p $HOME/rmp/SPECS
mkdir -p $HOME/rmp/SRPMS
mkdir -p $HOME/rmp/TMP

cd $HOME/tmp
rm -rf mib
svn co https://eris.llnl.gov/svn/chaos/private/mib/trunk mib
cd mib
./configure
VERSION=`grep VERSION META | cut -d':' -f2`
VERSION=${VERSION// /}
[ X"$VERSION" != X ] ||  usage "The META file did not yield a version"
cd ..
mkdir -p mib-$VERSION/aux
mkdir -p mib-$VERSION/mib-parms
mkdir -p mib-$VERSION/tools
cp mib/* mib-$VERSION/
rm mib-$VERSION/*.in
rm mib-$VERSION/configure
rm mib-$VERSION/META
cp mib/aux/* mib-$VERSION/aux
cp mib/mib-parms/* mib-$VERSION/mib-parms
cp mib/tools/* mib-$VERSION/tools
rm mib-$VERSION/tools/mibrpm.sh
rm -rf mib
tar cvfz $HOME/rpm/SOURCES/mib-$VERSION.tar.gz mib-$VERSION
cp mib-$VERSION/mib.spec $HOME/rpm/SPECS
rm -rf mib-$VERSION
cd $HOME/rpm/SPECS
mkdir -p $HOME/rpm/TMP/mib-$VERSION
rpmbuild -ba --define "_tmppath $HOME/rpm/TMP/mib-$VERSION" --define "_topdir $HOME/rpm" mib.spec 
