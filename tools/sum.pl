#!/usr/bin/perl -w
#*****************************************************************************\
#*  $Id: sum.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
# Sum the contents of STDIN, one number per line.  Print statistics.


my $verbose = 0;
my $sum = 0;
my $count = 0;
my @ar;
my $ave;
my $sdev = 0;

while ( $line = <> )
{
    chomp($line);
    if ( $line =~ /(\d+\.?\d*)/ )
    {
	push(@ar, $1);
	$sum += $1;
	$count++;
    }
}
if ($count == 0)
{
    print "No Data\n";
    exit;
}
else
{
    $ave = $sum/$count;
}
my $val;
foreach $val (@ar)
{
    $sdev += ($ave - $val) * ($ave - $val);
}
$sdev = sqrt($sdev/$count);
printf "count total        ave    sdev\n" if $verbose;
printf "----- --------- --------- ----\n" if $verbose;
if ( ($ave < 1) and ($ave > -1) )
{
    printf "%3d   %8.2f %6.4f %6.4f\n", $count, $sum, $ave, $sdev;
}
else
{
    printf "%3d   %8.2f %8.2f %6.2f\n", $count, $sum, $ave, $sdev;
}

 
