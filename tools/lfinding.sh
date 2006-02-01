#!/bin/bash
# lfinding.sh <dir>
# Go through the given directory and run lfind against all the listed
# data files.  Print task and obdix for each file.  This script
# naively assumes the files really are stripecount=1.  

# on an ION:
LFIND=/bgl/ion/usr/bin/lfind
HOSTNAME=`hostname`
ION=${HOSTNAME##bglio}
ION=$(($ION-1))
START=$(($ION*128))
END=$((START+127))
# but normally
#LFIND=/usr/bin/lfind

DIR=$1
[ -d $DIR ] || { echo "I can not access directory: $DIR" ; exit 1 ; }
#for file in $DIR/mibData.????????
for index in `seq $START $END`
do
  file=`printf "%s/mibData.%08d" $DIR $index`
  task=${file##*mibData.}
#  ostix=`$LFIND $file  | tail -2 | head -1 | /bin/awk '{print $1}'`
  ostix=`$LFIND $file  | tail -2 | head -1 `
done
