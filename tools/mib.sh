#!/bin/bash
#*****************************************************************************\
#*  $Id: mib.sh 1.9 2005/11/30 20:24:57 auselton Exp $
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
# mib.sh <srun_opts> -- <mib_opts>
# mib.sh <srun_opts> <full_path_to_mib> <mib_opts>
# 
# Run a mib test with all the logging goodies going as well.  This will create
# a timestamp labelled log directory in the $CLUSTER directory in your home 
# directory.  That log directory will have a copy of the parameters file, if 
# any, named "options".   There will be an "lwatch.log" trace of the output
#  from the lmtd server monitoring daemons (if the lwatch.py script exists).  
# There will also be a transcript of the test run in a file named "log".
# The one curve ball is that the -d option now points to the parameters file 
# rather than the log directory.  

PROG=${0##*/}
CMD_LINE="$*"
USER="auselton"
HOME="/g/g0/$USER"
BIN="$HOME/bin"
CLUSTER=
CLUSTER_BIN=

LWATCH="$BIN/lwatch.py"
HAVE_LWATCH="yes"
[ -x $LWATCH ] || HAVE_LWATCH="no"
# If the xwatch-lustre bits aren't installed you may need to include
# them locally along with this hint were to find them:
#export PYTHONPATH="$HOME/src/xwatch"

usage()
{
    [ -z $1 ] || echo "$1"
    echo "$PROG <srun_opts> -- <mib_opts>"
    echo "or"
    echo "$PROG <srun_opts><full_path_to_mib> <mib_opts>"
    echo "N.B. Use the -d arguement only to point to an optional parameters file,"
    echo "since this script will create the actual log directory."
    echo 
    exit
}

function get_date () {
    date
    DATE=`date '+%Y%m%d%H%M%S'`
}

function get_cluster () {
    CLUSTER=`nodeattr -v cluster`
    [ X"$CLUSTER" == X ] && usage "Didn't get CLUSTER: $CLUSTER"
    CLUSTER_BIN=$HOME/$CLUSTER
    MIB="$CLUSTER_BIN/mib-1.9-try"
}

function set_log () {
    LOG_DIR="$HOME/$CLUSTER/$DATE"
    mkdir -p $LOG_DIR
    LOG="$LOG_DIR/log"
    [ "$HAVE_LWATCH" == "no" ] || LWATCH_LOG="$LOG_DIR/lwatch"
# This is the first time we've been able to send anything to the log
    touch $LOG
    [ X"$LOG" != X -a -f $LOG ] || usage "Didn't get LOG: $LOG"
    echo "o DATE = $DATE" > $LOG
    echo "o CLUSTER = $CLUSTER" >> $LOG
    echo "o LOG = $LOG" >> $LOG
    chown -R $USER.$USER $LOG_DIR
    chmod go+r $LOG_DIR
}

get_date
get_cluster
set_log

# The following is usefull when there are multiple file systems
#TESTDIR=`grep testdir $CLUSTER_BIN/$PARMS | awk '{print $3}'`
#FS=${TESTDIR##/p/}
#FS=${FS%%/*}
case $CLUSTER in
    alc) FS_MONITOR="http://alci:50539"
#	[ X"$FS" == X"ga1" ] && FS_MONITOR="http://alci:50538"
	MPI_COM="srun --core=light "
	;;
    adev) FS_MONITOR="http://ilci:50538"
#	[ X"$FS" == X"ga1" ] && FS_MONITOR="http://alci:50538"
	MPI_COM="srun --core=light "
	;;
    *) usage "Edit \"mib.sh\" to list the fs monitor (lmtd) and MPI command for this cluster"
	;;
esac

# This script wants to always generate profiles and envirnmental info
HAVE_E="no"
for opt in "$*"
do
  [ "$opt" != "-E" ] || HAVE_E="true" 
#  [ "$opt" != "-p" ] || { echo "You do not want to set an explicit profile option here.  The script does it for you"; exit 1; } 
done
CMD_LINE="${CMD_LINE/--/$MIB} -p $LOG_DIR/profile"
[ "$HAVE_E" == "true" ] || CMD_LINE="$CMD_LINE -E"

# We shouldn't actually start lwatch.py until the job gets its resource,
# but that seems hard.
if [ "$HAVE_LWATCH" == "yes" ]
then
    nohup $LWATCH -f $FS_MONITOR > $LWATCH_LOG 2>&1 &
    LWATCH_PID=$!
fi

echo "$MPI_COM $CMD_LINE"
echo "tail -f $LOG"

$MPI_COM $CMD_LINE | tee -a $LOG

chown -R $USER.$USER $LOG_DIR
chmod -R go+r $LOG_DIR
rm -f $LOG~

if [ "$HAVE_LWATCH" == "yes" ]
then
    sleep 5
    kill -9 $LWATCH_PID
fi
echo "$BIN/composite.pl -d $LOG_DIR"


