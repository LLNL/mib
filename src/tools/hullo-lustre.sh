#!/bin/bash
#*****************************************************************************\
#*  $Id: hullo-lustre 1.9 2005/11/30 20:24:57 auselton Exp $
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
# hullo-lustre [<test_dir>]
#   Run a quick validation test to a Lustre directory.  The test directory
# is optional if this script already knows about it, otherwise it's 
# required.  The test should run for about two minutes and produce a single
# line of new output.  For comparisson's sake the last five lines, including
# the current one, of previous results are also printed.
#   Invoke this test with a command like the following:
# srun -N8 -n16 -p ltest /g/g0/auselton/bin/hullo-lustre \
#     /p/ga2/lustre-test/mib/crunchy

PROG=$(basename $0)
MIB=/usr/bin/mib

usage()
{
    if [ X"$SLURM_PROCID" == X"0" ]
	then
	[ X"$1" == X ] || echo "$1"
	echo "$PROG <test dir>"
	echo "$PROG -h: Print this messages and exit"
	echo "<test dir> is the directory where you want to run the test"
	echo "  It is optional if this script already knows where to do its I/O"
	echo 
    fi
    exit 0
}

[ X"$SLURM_PROCID" == X ] && { echo "$PROG is supposed to be srun"; exit 1; }
[ X"$1" == X"-h" ] && usage

CLUSTER=`nodeattr -v cluster`
[ X"$CLUSTER" == X ] && usage "Didn't get CLUSTER: $CLUSTER"

TEST_DIR=$1

if [ X"$TEST_DIR" == X ]
    then
    case $CLUSTER in
	adev) TEST_DIR="/p/ti1/lustre-test/mib/crunchy"
	    MIB=/home/auselton/$CLUSTER/mib
	    ;;
	alc) TEST_DIR="/p/ga2/lustre-test/mib/crunchy"
	    MIB=/g/g0/auselton/$CLUSTER/mib-1.8
	    ;;
	*) usage 
	    ;;
    esac
fi

[ X"$TEST_DIR" == X -o ! -d $TEST_DIR -o ! -w $TEST_DIR ] && usage "There is a problem with the test directory"

cd $TEST_DIR

[ ! -f previous.db ] && [ X"$SLURM_PROCID" == X"0" ] && touch previous.db

$MIB | tee -a previous.db

[ X"$SLURM_PROCID" != X"0" ] || { echo;echo; tail -5 previous.db; }

exit 0

