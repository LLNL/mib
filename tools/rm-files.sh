#!/bin/bash
#*****************************************************************************\
#*  $Id: rm-files.sh 1.9 2005/11/30 20:24:57 auselton Exp $
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
# The following semed to work sometimes when I couldn't find "munlink"
# It may not have always worked, though
#UNLINK=/bgl/ion/bin/rm

HOST=`hostname`
DIR=$1
# BLC
#LSTRIPE=/bgl/ion/usr/bin/lstripe
#UNLINK=/bgl/ion/bin/unlink
#OSTs="448"
#CNs_per_ION=128
#HOST=${HOST##bglio}
#OFFSET=1

# ALC
UNLINK=/bin/unlink
OSTs="32"
#HOST=${HOST##alc}
CNs_per_ION=2
OFFSET=69

START=$(( ($HOST - 1)*$CNs_per_ION - $OFFSET ))
END=$(( $START + $CNs_per_ION - 1 - $OFFSET))

[ X"$DIR" != X ] || { echo "The DIR parameter is required"; exit 1; }
[ -d $DIR ] || { echo "Did not see directory $DIR"; exit 1; }


for index in `seq $START $END`
do
#  OST=$(( $index % $OSTs ))
  TARGET=`printf "%s/mibData.%08d" $DIR $index`
  $UNLINK $TARGET >/dev/null 2>&1
#  TARGET=`printf "%s/iorData.%08d.hist.0" $DIR $index`
#  $UNLINK $TARGET >/dev/null 2>&1
#  TARGET=`printf "%s/iorData.%08d.hist.2" $DIR $index`
#  $UNLINK $TARGET >/dev/null 2>&1
done
