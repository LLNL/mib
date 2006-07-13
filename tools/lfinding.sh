#!/bin/bash
#*****************************************************************************\
#*  $Id: lfinding.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
# lfinding.sh <dir>
# Go through the given directory and run lfind against all the listed
# data files.  Print task and obdix for each file.  This script
# naively assumes the files really are stripecount=1.  

# on an ION:
#LFIND=/bgl/ion/usr/bin/lfind
#HOSTNAME=`hostname`
#ION=${HOSTNAME##bglio}
#ION=$(($ION-1))
#START=$(($ION*128))
#END=$((START+127))
# but normally
LFIND=/usr/bin/lfind
START=0
END=1024

DIR=$1
[ -d $DIR ] || { echo "I can not access directory: $DIR" ; exit 1 ; }
#for file in $DIR/mibData.????????
for index in `seq $START $END`
do
  file=`printf "%s/mibData.%08d" $DIR $index`
  task=${file##*mibData.}
#  ostix=`$LFIND $file  | tail -2 | head -1 | /bin/awk '{print $1}'`
  ostix=`$LFIND $file  | tail -2 | head -1 `
  echo "$task: $ostix"
done
