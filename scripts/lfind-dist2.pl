#!/usr/bin/perl -w
#*****************************************************************************\
#*  $Id: lfind-dist2.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
# lfind-dist2.pl
# Read the table of task number versus obdix index from standard in.
# Note how many distinct obdix entries there are for each group of
# 64 CNs (collectively behind one ION).

#BGL
#my $CNS_PER_ION = 64;
#my $NUM_OSTS = 448;
#BGL VNm
my $CNS_PER_ION = 128;
my $NUM_OSTS = 448;
#uBGL
#my $CNS_PER_ION = 8;
#my $NUM_OSTS = 28;
my $line;
my @obdix;
my @ions;
my $index;
my $cn;
my $ion;
my $max = -1;
my $min;
my $ave = 0;
my $low;
my $high = -1;

while(defined($line=<>))
{
    if ($line =~ /(\d+)\s+(\d+)/)
    {
	$cn = $1;
	$index = $2;
	$ion = int($cn/$CNS_PER_ION);
	if($max < 0)
	{
	    $max = $min = $ion;
	}
	else
	{
	    $max = ($max > $ion) ? $max : $ion;
	    $min = ($min < $ion) ? $min : $ion;
	}
	if( ($cn % $CNS_PER_ION) == 0 )
	{
	    for (my $ost = 0; $ost < $NUM_OSTS; $ost++)
	    {
		$ions[$ion - 1] += $obdix[$ost] if ($ion > 0);
		$obdix[$ost] = 0;
	    }
	    if($ion > 0)
	    {
		$val = $ions[$ion - 1];
		$ave += $val;
		printf "%d\t%d\n", $ion - 1, $val if ($ion > 0);
		if($high < 0)
		{
		    $high = $low = $val;
		}
		else
		{
		    $high = ($high > $val) ? $high : $val;
		    $low = ($low < $val) ? $low : $val;
		}
	    }
	}
	$obdix[$index] = 1;
    }
}
for (my $ost = 0; $ost < $NUM_OSTS; $ost++)
{
    $ions[$ion] += $obdix[$ost];
}
$val = $ions[$ion];
$ave += $val;
printf "%d\t%d\n", $ion, $val;
$high = ($high > $val) ? $high : $val;
$low = ($low < $val) ? $low : $val;

my $size = $max - $min + 1;

$ave /= $size if ( $size > 0 );
printf "Average number of the %d OSTs used by the %d IONs is %.2f\n", $NUM_OSTS, $size, $ave;

my $sdev = 0;

for($ion = $min; $ion <= $max; $ion++)
{
    my $val;

    if (defined($ions[$ion]))
    {
	$val = $ions[$ion];
    }
    else
    {
	$val = 0;
    }
    $sdev += ($val - $ave) * ($val - $ave);
}
$sdev = sqrt($sdev/$size) if ( $size > 0 );
printf "Standard deviation is %.2f, the high value is %d and the low is %d\n", $sdev, $high, $low;
