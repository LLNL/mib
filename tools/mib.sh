#!/bin/bash
#*****************************************************************************\
#*  $Id: mib.sh 1.9 2005/11/30 20:24:57 auselton Exp $
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

usage()
{
    [ -z $1 ] || echo "$1"
    echo "$0 <MPs> <cluster> <log> <map_syle>"
    echo "<MPs> is how many midplanes the test is running on (required)"
    echo "<cluster> is the cluster being used (required)"
    echo "<date> is a date and time stamp for distinguishing this test (required)"
    echo "<map_style> determines which mapfile to use, if any (none by default)"
    echo 
    exit
}

MPS=$1
[ X"$MPS" == X ] && usage "Missing <mps> value" 

CLUSTER=$2
[ X"$CLUSTER" == X ] && usage "Missing <cluster> value"

LOG=$3
[ X"$LOG" == X ] && usage "Missing <log> value"
LOG_DIR=${LOG%%/log}

VN_MODE=""
MAP_STYLE=$4
[ X"$MAP_STYLE" == X"f128" ] && VN_MODE=" -mode VN"

BGL_BIN="/g/g0/auselton/bgl"
TEST_DIR="/g/g0/auselton/testing"
MPIRUN="/usr/local/bin/mpirun"
MIB="$BGL_BIN/mib"

MAP=""
[ X"$MAP_STYLE" != X ] && MAP="$BGL_BIN/mapfiles/$CLUSTER.${MPS}mps.${MAP_STYLE}.map"
[ X"$MAP" != X -a ! -f "$MAP" ] && usage "MAP=$MAP"
[ X"$MAP" != X ] && MAP=" -mapfile $MAP "

PARMS="${MPS}mps.${CLUSTER}.mib"
[ -f "$BGL_BIN/mib-parms/$PARMS" ] || usage "PARMS=$PARMS"
[ X"$MAP_STYLE" == X"f128" ] && PARMS="$PARMS.vnm"

cp $BGL_BIN/mib-parms/$PARMS $LOG_DIR/options
MIB_ARGS="-d $LOG_DIR "

echo "$MPIRUN -env BGL_APP_L1_WRITE_THROUGH=1 -cwd $TEST_DIR $VN_MODE -exe $MIB $MAP -args \"$MIB_ARGS\""

$MPIRUN -env BGL_APP_L1_WRITE_THROUGH=1 -cwd $TEST_DIR $VN_MODE -exe $MIB $MAP -args "$MIB_ARGS"



