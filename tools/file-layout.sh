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

LSTRIPE=/bgl/ion/usr/bin/lstripe
DD=/bgl/ion/bin/dd

#BGL, VN mode
#OSTs="448"
#CNs_per_ION=128

#half BLC, VN mode
OSTs="224"
CNs_per_ION=128
NUM_DDNS=28
LUNS_per_DDN=16

#uBGL
#OSTs="28"
#CNs_per_ION=16

DIR=$1
[ X"$DIR" != X ] || { echo "The DIR parameter is required"; exit 1; }

HOST=`hostname`
HOST=${HOST##bglio}
START=$(( ($HOST - 1)*$CNs_per_ION ))
END=$(( $START + $CNs_per_ION - 1 ))

#mkdir -p $DIR
#$LSTRIPE $DIR 0 -1 1
#[ -d $DIR ] || { echo "Did not create directory $DIR"; exit 1; }

for index in `seq $START $END`
do
# Regular round-robin relation
  OST=$(( $index % $OSTs ))
# Fancy half-file-system layout, uses all the LUNs on every other DDN
#  DDN=$(( ($index / $LUNS_per_DDN) % ($NUM_DDNS/2) ))
#  DDN_OFF=$(( $index % $LUNS_per_DDN ))
#  OST=$(( $DDN*2*$LUNS_per_DDN + $DDN_OFF ))

  TARGET=`printf "%s/mibData.%08d" $DIR $index`
  $LSTRIPE $TARGET 0 $OST 1
  $DD if=/dev/zero of=$TARGET bs=1048576 count=128 >/dev/null 2>&1
  [ X"$OST" == X"0" ] && echo $TARGET
done
