#!/bin/bash
#*****************************************************************************\
#*  $Id: mib-test.sh 1.9 2005/11/30 20:24:57 auselton Exp $
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

CLUSTER=`nodeattr -v cluster`
case $CLUSTER in
    adev) 
	FS="ti1"
	ONE_NODE="adev2"
	;;
    tdev)
	FS="ti2"
	ONE_NODE="tdev3"
	;;
    alc) 
	FS="ga2"
	ONE_NODE="alc68"
	;;
    *)
	echo "I need to know what File System to test on this cluster"
	;;
esac

[ X"$HOME" == X ] && { echo "You want to set \$HOME before proceeding"; exit 1; }
[ -w $HOME ] || { echo "You will want \$HOME set to something you have permissions in"; exit 1; }
TOOLS=/var/lustredata/scripts
TARGET="-t /p/${FS}/lustre-test/mib/crunchy"
NO_FILES="-t /p/${FS}/lustre-test/mib/nofiles"
MIB=`which mib`
SRUN="srun --core=light"
#N.B. This is not distributed with mib:
LWATCH="$HOME/bin/lwatch.py"
FS_MONITOR="http://ilci:50538"
COMPOSITE=$TOOLS/composite.pl
PROFILE_DIR="$HOME/tmp/testing"

echo "===>test a version request"
$MIB -V

echo "===>test a help request"
$MIB -h

echo "===>test a normal one node run using MPI"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE

echo "===>do the same thing quietly"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m

echo "===>just use the defaults"
$SRUN -N1 -n1 $MIB $TARGET

echo "===>and verbosely"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHEIP

echo "===>Just write"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -W

echo "===>Just read"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -R

echo "===>test random reads, byte granularity"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -b

echo "===>test random reads, 4k granularity"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -b4k

echo "===>Remove the file(s) at the end"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -r

echo "===>Create new file(s)"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -n

echo "===>Create new file(s) in place of existing ones"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -n

echo "===>Try to read where there are no files, This should fail"
[ -d $NO_FILES ] && { echo "The \$NO_FILES target $NO_FILES needs to be removed before this test will work"; exit 1 }
if mkdir $NO_FILES
    then 
    $SRUN -N1 -n1 $MIB $NO_FILES -L60 -l1024 -s 4m -SHE -R
#clean up the core files, if any
    rm -f *core
else
    echo "Looks like we do not have permissiion to create the \$NO_FILES target $NO_FILES"
    exit 1
fi

echo "===>Forget to give target, should fail"
$SRUN -N1 -n1 $MIB -L60 -l1024 -s 4m -SHE
#clean up the core files, if any
rm -f *core

echo "===>test suppressing MPI"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -M

echo "===>test without SLURM"
pdsh -w $ONE_NODE $MIB $TARGET -L60 -l1024 -s 4m -SHE -M

echo "===>Check that it figures out there's no MPI even without being told"
#(i.e. no -M)
pdsh -w $ONE_NODE $MIB $TARGET -L60 -l1024 -s 4m -SHE

echo "===>run locally to NFS home directory, this one should fail"
$MIB -t "." -L60 -l512 -s 512k -SHE
#clean up the core files, if any
rm -f *core

echo "===>run locally to NFS home directory, force it to do even though it's"
echo "a bad idea"
$MIB -t "." -L60 -l512 -s 512k -SHE -F

echo "===>test with two tasks per node"
$SRUN -N1 -n2 $MIB $TARGET -L60 -l1024 -s 4m -SHE

echo "===>test with more nodes"
# Modify this to include as many nodes as you can
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE

[ -d $PROFILE_DIR -a -w $PROFILE_DIR ] || { echo "The is no $PROFILE_DIR for profiling output so we will just quit here"; exit 1; }
echo "===>test profiling"
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE -p $PROFILE_DIR/profile

echo "===>test node averaging behavior"
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE -p $PROFILE_DIR/profile -a

[ -f $LWATCH -a -x $LWATCH ] || { echo "There is no $LWATCH so we will just quit here"; exit 1; }
echo "===>test composite file generation"
nohup $LWATCH -f $FS_MONITOR > $PROFILE_DIR/lwatch 2>&1 &
LWATCH_PID=$!
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE -p $PROFILE_DIR/profile | tee -a $PROFILE_DIR/log
sleep 5
kill -9 $LWATCH_PID
echo "===>run this as non-root so gnuplot can get at X:"
echo "$COMPOSITE -d $PROFILE_DIR"
echo
echo "===>Done"
