#!/bin/bash
# hello_lustre.sh
#   Verify the correct operation of the Lustre file system using 
# mib with sensible default values.  
#   To use this script begin by insuring that the TARGET_DIR is set the 
# way you want it.  For now, this script will only work  on BGL and uBGL.  
#   Run this script.  Mib will spend 200 seconds or a bit more and produce 
# a single line of output.  That output may be interpreted as follows:
#                     date  tasks  xfer  call  time      write       read
#                                       limit limit       MB/s       MB/s
# ------------------------ ------ ----- ----- ----- ---------- ----------
# Mon Jan  9 13:33:42 2006    256  512k   256   100     629.38     374.98
#
# N.B. Mib will only produce the last line.
#   The "time limit" value of 100 is the number of seconds the test
# spends on each of the write test and the read test, thus a total of 200 
# seconds.  The script echoes the mib output to stdout and appends it
# to the file "log" in the cwd.  Finally, the script reproduces the most 
# recent 5 tests (i.e. it tails the last 5 lines of the "log" file).  An 
# examination of those tests may reveal problems with a file system even 
# when it is working.  
#
# Keep in mind that mib requires all its files to be precreated.   If the 
# target directory doesn't already have all the needed files, mib will 
# fail.
#

PROG=$0
PROG=${PROG##*/}

usage()
{
    [ -z $1 ] || echo "$1"
    echo "$PROG"
    echo "This script invokes mib with default values.  If it runs"
    echo "to completion the Lustre file system is probably in good"
    echo "shape.  An examination of the data rates of the current "
    echo "and previous runs may reveal slowness even in a working"
    echo "file system.  Currently this script only knows about BGL"
    echo "and uBGL."
    echo 
    exit
}

[ X"$1" == X"-h" ] && usage
[ X"$1" == X"--help" ] && usage

CLUSTER=`nodeattr -v cluster`
case $CLUSTER in
    bgl) TARGET_DIR="/p/gb1/lustre-test/mib/skippy"
	;;
    ubgl) TARGET_DIR="/p/gbtest/lustre-test/mib/smooth"
	;;
    *) usage
	;;
esac
cd $TARGET_DIR

MPIRUN="/usr/local/bin/mpirun"
MIB="/g/g0/auselton/bgl/mib"
TEST_DIR=`pwd`
VN_MODE=" -mode VN"

$MPIRUN -env BGL_APP_L1_WRITE_THROUGH=1 -cwd $TEST_DIR $VN_MODE -exe $MIB | tee -a log

echo
echo "Recent results:"
echo
tail -5 log
