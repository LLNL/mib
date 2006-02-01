#!/bin/bash
# rm-files.sh <dir>
#   Given a directory of files laid out for mib or IOR, try to "delete" 
# them based on the task number encoding.  Assume that the script is 
# being run on an ION and only do the 64 or 128 files that would be 
# associated with that ION, perhaps including any system call profile 
# data files.
#   Use this script from the service node (as root) via a command line 
# like the following:
# pdsh -f 128 -R rsh -w bglio[1-1024] "/g/g0/auselton/mibtools/rm-files.sh \
#   /p/gb1/lustre-test/ior/swl"
# If you only have 64k files and have hist.0 and hist.2 profiles as well
# then you can expect it to take nearly 10 minutes.

# It looks like there is no "munlink" 
# but Andreas thinks the regular "unlink" should work
#UNLINK=/bgl/ion/usr/bin/munlink
LSTRIPE=/bgl/ion/usr/bin/lstripe
UNLINK=/bgl/ion/bin/unlink
# The following semed to work sometimes when I couldn't find "munlink"
# It may not have always worked, though
#UNLINK=/bgl/ion/bin/rm

# BLC
OSTs="448"

DIR=$1
[ X"$DIR" != X ] || { echo "The DIR parameter is required"; exit 1; }
[ -d $DIR ] || { echo "Did not see directory $DIR"; exit 1; }

HOST=`hostname`
HOST=${HOST##bglio}
START=$(( ($HOST - 1)*128 ))
END=$(( $START + 127 ))

for index in `seq $START $END`
do
  OST=$(( $index % $OSTs ))
  TARGET=`printf "%s/mibData.%08d" $DIR $index`
  $UNLINK $TARGET >/dev/null 2>&1
#  TARGET=`printf "%s/iorData.%08d.hist.0" $DIR $index`
#  $UNLINK $TARGET >/dev/null 2>&1
#  TARGET=`printf "%s/iorData.%08d.hist.2" $DIR $index`
#  $UNLINK $TARGET >/dev/null 2>&1
done
