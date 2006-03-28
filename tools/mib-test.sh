#!/bin/bash

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
	echo "I need to know hat File System to test on this cluster"
	;;
esac

BIN="$HOME/bin"
TOOLS=/var/lustredata/scripts
TARGET="-t /p/${FS}/lustre-test/mib/crunchy"
NO_FILES="-t /p/${FS}/lustre-test/mib/nofiles"
MIB=`which mib`
SRUN="srun --core=light"
#N.B. This is not distributed with mib:
LWATCH="$BIN/lwatch.py"
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

echo "===>Try to read where there are no files, this should fail"
#Don't use a shell variable in the following.  A typo could lead to 
#disaster
pdsh -w $ONE_NODE rm -f /p/ti1/lustre-test/mib/nofiles/*
$SRUN -N1 -n1 $MIB $NO_FILES -L60 -l1024 -s 4m -SHE -R
#clean up the core files
rm -f *core

echo "===>Forget to give target, should fail"
$SRUN -N1 -n1 $MIB -L60 -l1024 -s 4m -SHE
#clean up the core files
rm -f *core

echo "===>test suppressing MPI"
$SRUN -N1 -n1 $MIB $TARGET -L60 -l1024 -s 4m -SHE -M

echo "===>test without SLURM"
pdsh -w $ONE_NODE $MIB $TARGET -L60 -l1024 -s 4m -SHE -M

echo "===>Check that it figures out there's no MPI even without being told"
#(no -M)
pdsh -w $ONE_NODE $MIB $TARGET -L60 -l1024 -s 4m -SHE

echo "===>run locally to NFS home directory, this one should fail"
$MIB -t "." -L60 -l512 -s 512k -SHE
#clean up the core files (are there any?)
rm -f *core

echo "===>run locally to NFS home directory, force it to do even though it's"
echo "a bad idea"
$MIB -t "." -L60 -l512 -s 512k -SHE -F

echo "===>test with two tasks per node"
$SRUN -N1 -n2 $MIB $TARGET -L60 -l1024 -s 4m -SHE

echo "===>test with more nodes"
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE

echo "===>test profiling"
#Don't use a shell variable in the following.  A typo could lead to 
#disaster
rm -rf tmp/testing
mkdir -p $PROFILE_DIR
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE -p $PROFILE_DIR/profile

echo "===>test node averaging behavior"
$SRUN -N2 -n4 $MIB $TARGET -L60 -l1024 -s 4m -SHE -p $PROFILE_DIR/profile -a

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
