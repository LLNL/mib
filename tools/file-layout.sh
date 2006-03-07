#!/bin/bash
# file-layout.sh <dir>
# Pre-create Lustre target files such that they are evenly layed out 
# accross OSTs and each ION has 64 distinct OSSs as targets.  Split
# the task up so that all the IONs can participate in the creation.
# N.B. Until CFS puts out a repaired lstripe binary you have to do this
# on blc-mds-gb2a:/tmp/auselton, where gb1 is mounted.  
# 2006-01-17 the lstripe binary for the IONs works fine, as does the 
# one on the FENS.
#
# This script should be invoked on the IONs.  Do so by using "pdsh" 
# from the service node:
# bglsn> pdsh -R rsh -w bglio[1-1024] \
#      /g/g0/auselton/mibtools/file-layout.sh <dir>
#


#BGL, VN mode
#LSTRIPE=/bgl/ion/usr/bin/lstripe
#DD=/bgl/ion/bin/dd
#OSTs="448"
#CNs_per_ION=128

#half BLC, VN mode
#OSTs="224"
#CNs_per_ION=128
#NUM_DDNS=28
#LUNS_per_DDN=16
#
#uBGL
#OSTs="28"
#CNs_per_ION=16

# ALC
CLUSTER=`nodeattr -v cluster`
[ X"$CLUSTER" == X ] && fail "Didn't get CLUSTER: $CLUSTER"
CLUSTER_BIN=$HOME/$CLUSTER
LSTRIPE=/usr/bin/lstripe
DD=/bin/dd
UNLINK=/bin/unlink
TOUCH=/bin/touch
LFIND=/usr/bin/lfind
# ga2
#OSTs=32
# ga1
#OSTs=64
# ti1
OSTs="4"
TASKS="32"
MB_per_FILE="2048"
FIRST_CLIENT="7"
 
DIR=$1
[ X"$DIR" != X ] || { echo "The DIR parameter is required"; exit 1; }

HOST=`hostname`
HOST=${HOST##$CLUSTER}
START=$(( $HOST - $FIRST_CLIENT ))
#END=$TASKS
END="5"

for index in `seq $START $OSTs $END`
do
  OST=$(( $HOST - $FIRST_CLIENT ))
  TARGET=`printf "%s/mibData.%08d" $DIR $index`
  echo "$LSTRIPE $TARGET 0 $OST 1"
#  $UNLINK $TARGET
#  $TOUCH $TARGET
  $LSTRIPE $TARGET 0 $OST 1
  $DD if=/dev/zero of=$TARGET bs=1048576 count=$MB_per_FILE >/dev/null 2>&1
done
