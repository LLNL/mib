#!/usr/bin/perl -w
#*****************************************************************************\
#*  $Id: lfind-dist1.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
# lfind-dist1.pl
# Read the table of task number versus obdix index from standard in.
# Note how many times obdix is mentioned for each obdix.  Print
# a summary of the obdix distribution.

my $line;
my @obdix;
my $index;
my $max = -1;
my $min;
my $ions = 0;

while(defined($line=<>))
{
    if ($line =~ /(\d+)\s+(\d+)/)
    {
	$ions++;
	$index = $2;
	if($max < 0)
	{
	    $max = $min = $index;
	}
	else
	{
	    $max = ($max > $index) ? $max : $index;
	    $min = ($min < $index) ? $min : $index;
	}
	if (defined($obdix[$index]))
	{
	    $obdix[$index]++;
	}
	else
	{
	    $obdix[$index] = 1;
	}
    }
}

my $ave = 0;
my $size = $max - $min + 1;

for($index = $min; $index <= $max; $index++)
{
    my $val;
    if (defined($obdix[$index]))
    {
	$val = $obdix[$index];
    }
    else
    {
	$val = 0;
    }
    $ave += $val;
    printf "%d\t%d\n", $index, $val;
}
$ave /= $size if ( $size > 0 );
printf "Average for %d IONs distributed over %d OSTs is %.2f\n", $ions, $size, $ave;

my $sdev = 0;

for($index = $min; $index <= $max; $index++)
{
    my $val;
    if (defined($obdix[$index]))
    {
	$val = $obdix[$index];
    }
    else
    {
	$val = 0;
    }
    $sdev += ($val - $ave) * ($val - $ave);
}
$sdev = sqrt($sdev/$size) if ( $size > 0 );
printf "Standard deviation is %.2f\n", $sdev;


	
