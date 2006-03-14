#!/bin/bash
#*****************************************************************************\
#*  $Id: mest.sh 1.9 2005/11/30 20:24:57 auselton Exp $
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

BLOCKS_IN_USE="/usr/local/bin/blocks_in_use"
LWATCH="/g/g0/auselton/bin/lwatch.py"
export PYTHONPATH="/g/g0/auselton/src/xwatch"

DATE=""
MAP_STYLE=""
CLUSTER=""
CLUSTER_DIR=""
LOG=""
USER=""
BLOCK=""
MPS=""
LIES=""

function fail () {
    local MSG=""
    [ X"$1" == X ] || MSG=$1
    echo "$0 FAILED: $MSG"
    [ X"$LOG" != X -a -f $LOG ] && cat $LOG
    exit 1
}

function get_date () {
    date
    DATE=`date '+%Y%m%d%H%M%S'`
}

function get_map_style () {
    MAP_STYLE=$1
    [ -z $MAP_STYLE ] && { echo -n "Sure you want to go without a mapfile?  (You may enter it now if you like.):" ; read MAP_STYLE ; }
    [ X"${MAP_STYLE:0:1}" == X"y" -o  X"${MAP_STYLE:0:1}" == X"Y"  ] && MAP_STYLE=""
}

function get_cluster () {
    CLUSTER=`nodeattr -v cluster`
    [ X"$CLUSTER" == X ] && fail "Didn't get CLUSTER: $CLUSTER"
    case $CLUSTER in
	bgl) FS_MONITOR="http://blci:50538"
	    ;;
	ubgl) FS_MONITOR="http://levii:50538"
	    ;;
	*) fail "I need to know the fs monitor (lmtd)"
	    ;;
    esac
}

function set_log () {
    LOG_DIR="/g/g0/auselton/testing/${DATE}"
    mkdir -p $LOG_DIR
    LOG="$LOG_DIR/log"
    LWATCH_LOG="$LOG_DIR/lwatch"
# This is the first time we've been able to send anything to the log
    echo "o DATE = $DATE" > $LOG
    [ X"$LOG" != X -a -f $LOG ] || fail "Didn't get LOG: $LOG"
    echo "o MAP_STYLE = $MAP_STYLE" >> $LOG
    echo "o CLUSTER = $CLUSTER" >> $LOG
    echo "o LOG = $LOG" >> $LOG
    chown -R auselton.auselton $LOG_DIR
}

function get_user () {
    USER=`whoami`
    [ X"$USER" == X ] && fail "Didn't get USER: $USER"
    echo "o USER = $USER" >> $LOG
}

function get_block () {
    BLOCK=`$BLOCKS_IN_USE | grep "$USER" | head -1 | awk '{print $1}'`
    [ X"$BLOCK" == X ] && fail "Didn't get BLOCK: $BLOCK"
    echo "o BLOCK = $BLOCK" >> $LOG
}

function get_mps () {
# I discovered a recent change to smap that makes 
    SMAP=`smap -c -Db | grep "$BLOCK" | head -1 `
    SMAP=${SMAP//no part/no_part/}
    MPS=`echo $SMAP | awk '{print $7}'`
    [ X"$MPS" == X ] && fail "Didn't get MPS: $MPS"
    echo "o MPS = $MPS" >> $LOG
}

function get_fen_kernel () {
    FEN_KERNEL=`uname -r`
    [ X"$FEN_KERNEL" == X ] && fail "Didn't get FEN_KERNEL: $FEN_KERNEL"
    echo "o FEN_KERNEL = $FEN_KERNEL" >> $LOG
}

function get_sn_info () {
    if [ X"$LIES" == X ] 
	then
	echo "Visiting Service Node"
	SN_INFO=`ssh ${SUDO_USER}@${CLUSTER}sn /g/g0/auselton/bgl/sn_info.sh $ION $BLOCK`
	[ X"$SN_INFO" == X"FAILED" ] && fail "Didn't get SN_INFO: $SN_INFO"
	PSETS=`echo $SN_INFO | awk '{print $1}'`
	SN_KERNEL=`echo $SN_INFO | awk '{print $2}'`
#    SN_KERNEL=2.6.5-7.191-pseries64
#    ION_KERNEL=2.4.19-20llnl
	echo "o PSETS = $PSETS" >> $LOG
	echo "o SN_KERNEL = $SN_KERNEL" >> $LOG
	echo "Back from Service Node"
    fi
}

function get_mib_version () {
    MIB_VERSION=`ls -l /g/g0/auselton/bgl/mib | awk '{print $11}'`
    echo "o IOR = $IOR_VERSION" 
    if [ X"$LIES" == X ]
	then
	echo "o fsych is on" >> $LOG
	echo "o extents,mballoc = enabled" >>$LOG
    fi
}

LIES=$2

get_date
get_map_style $1
get_cluster
set_log
get_user
get_block
get_mps
get_fen_kernel
#get_sn_info
#get_mib_version
case $CLUSTER in
    bgl)
	ION="bglio1"
	;;
    ubgl)
	ION="ubglio1"
	;;
esac
if [ X"$LIES" == X ] 
    then
    echo "o OST Kernel = 2.6.9-10patched.tunable_backoff" >>$LOG
    echo "o [root@blci ~]# pdsh -w blc[1-224] cat /proc/sys/net/ipv4/tcp_retries2 | dshbak -c" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o blc[1-224]" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o  150" >> $LOG
    echo "o [root@blci ~]# pdsh -w blc[1-224] cat /proc/sys/net/ipv4/tcp_rto* | dshbak -c" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o blc[1-224]" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o  3000" >> $LOG
    echo "o  3000" >> $LOG
    echo "o  100" >> $LOG
    echo "o [root@blci ~]# pdsh -w blc[1-224] "cat /proc/sys/socknal/timeout" | dshbak -c" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o blc[1-224]" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o  50" >> $LOG
    echo "o [root@blci ~]# pdsh -w blc[1-224] "cat /proc/sys/socknal/keepalive_*" | dshbak -c" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o blc[1-224]" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o  100" >> $LOG
    echo "o  30" >> $LOG
    echo "o  2" >> $LOG
    echo "o [root@blci ~]# pdsh -w blc[1-224] "cat /proc/sys/lustre/timeout" | dshbak -c" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o blc[1-224]" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o  300" >> $LOG
    echo "o [root@blci ~]# pdsh -w blc[1-224] "cat /proc/sys/lustre/ldlm_timeout" | dshbak -c" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o blc[1-224]" >> $LOG
    echo "o ----------------" >> $LOG
    echo "o  20" >> $LOG
    echo -n "o driver = " >> $LOG
    ls -l /bgl/BlueLight/ppcfloor >>$LOG
    echo "o CIOD retry settings = rdsd" >>$LOG
    echo "o Lustre = 1.4.3.6" >>$LOG
    echo "o patch for bz#7138" >>$LOG
    echo "o Server keepalive(idle, intrvl, count) = (60, 15, 16)" >>$LOG
    echo "o max_dirty_mb = 32" >>$LOG
    echo "o disabled prefetch on the DDNs" >>$LOG
    echo "" >>$LOG
    echo "Purpose" >>$LOG
    echo "" >>$LOG
    echo "" >>$LOG
    echo "Results:" >>$LOG
    echo "" >>$LOG
    echo "" >>$LOG
    echo "-----------------------------------------------------------------------" >>$LOG
    echo "" >>$LOG
    [ X"$EDITOR" == X ] || $EDITOR $LOG
fi
echo "/g/g0/auselton/bgl/mib.sh $MPS $CLUSTER $MAP_STYLE >> $LOG 2>&1"
echo "tail -f $LOG"

nohup $LWATCH -f $FS_MONITOR > $LWATCH_LOG 2>&1 &
LWATCH_PID=$!

echo "/g/g0/auselton/bgl/mib.sh $MPS $CLUSTER $LOG $MAP_STYLE" >>$LOG 2>&1
/g/g0/auselton/bgl/mib.sh $MPS $CLUSTER $LOG $MAP_STYLE >>$LOG 2>&1

kill -9 $LWATCH_PID
rm $LOG~

echo "/g/g0/auselton/mibtools/composite.pl -d $LOG_DIR"
