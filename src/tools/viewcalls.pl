#!/usr/bin/perl -w 
#*****************************************************************************\
#*  $Id: viewcalls.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
#  viewcalls.pl [-b <begin>] [-c <collumns>] [-e <end>] [-f <table>] [-h] [-k] [-p <print>] [-s] 
# For each CN (or average per ION) show the trace of system call 
# steps versus their time of completion.

use Getopt::Std;

use constant GETOPTS_ARGS => "b:c:e:f:hkp:s";
use vars map { '$opt_' . $_ } split(/:*/, GETOPTS_ARGS);

my $prog = $0;
my $path = ".";
if ($prog =~ /^(.*)\/([^\/]+)$/ )
{
    $path = $1;
    $prog = $2;
}

my $verbose = 0;

my $line;
my @calls;
my $begin = 0;
my $end = -1;
my $col_arg = "0";
my @COLs;
my @Ranges;
my $max_range;
my $data_file = "syscalls.aves";
my $png_file;
my $tmp_file = "/tmp/viewcalls.tmp.$$";
my $gnu_file = "/tmp/viewcalls.$$.gnuplot";
my $show_data = 0;
my $keep_files = 0;

sub usage
{
    print $_[0] . "\n" if (defined($_[0]));
    print "$prog [-c <cols>] [-f <file>] [-h]\n";
    print "Show the trace of system call steps versus their time of completion\n";
    print "-b <begin>\tBegin graph at step <begin> (default 0)\n";
    print "-c <cols>\tUse the given list of collumns, which may be a comma\n";
    print "\t\tseparated list of ranges (eg. 1,3-5,7,9,10-12, default 0)\n";
    print "-e <end>\tEnd graph at step <end> (default last)\n";
    print "-f <file>\tGet table data from <file> (default \"syscalls.ave\")\n";
    print "-h\t\tthis message\n";
    print "-k\t\tKeep (ie.e do not delete) the data and gnuplot tmp files\n";
    print "-p <print>\tSave the graph to the file \"<print>.png\" rather than display it\n";
    print "-s\t\tShow the data as well as plotting it\n";
    exit(0);
}

usage() unless(getopts(GETOPTS_ARGS));

($opt_h) && usage();

$col_arg = $opt_c if ($opt_c);
$data_file = $opt_f if ($opt_f);
$keep_files = $opt_k if ($opt_k);
$show_data = $opt_s if ($opt_s);
$begin = $opt_b if ($opt_b);
$end = $opt_e if ($opt_e);
$png_file = $opt_p if ($opt_p);
if ( $opt_p and ! ($png_file =~ /\.png$/) )
{
    $png_file = $png_file . ".png";
    print"Graph output to $png_file\n";
}

sub set_range
{
    my $range = $_[0];
    my $max;
    
    if(defined($range))
    {
	if($range =~ /^(\d+)$/)
	{
	    $COLs[$1] = 1;
	    $max = $1;
	} 
	elsif ($range =~ /^(\d+)-(\d+)$/)
	{
	    for (my $i = $1; $i <= $2; $i++)
	    {
		$COLs[$i] = 1;
	    }
	    $max = $2;
	}
	else
	{
	    return;
	}
	$max_range = $max if (!defined($max_range));
	$max_range = ($max_range > $max) ? $max_range : $max;
    }
}
	
@Ranges = split /,/, $col_arg;
for $range (@Ranges)
{
    set_range $range;
}

-f $data_file or die "I did not see the data file $data_file";
open(DATA, "<$data_file") or die "I could not open the data file $data_file";
open(TMP, ">$tmp_file") or die "Could not open tmp file $tmp_file";

my $num_cols = 0;
for (my $i = 0; $i <= $max_range; $i++)
{
    $num_cols++ if(defined($COLs[$i]));
}

# This loop reads in the system call timings table, and prints the 
# value from each line that is in the <task>-th position.
my $count = 0;
my $include;
while( defined($line = <DATA>) )
{
    chomp($line);
    @calls = split /\s/, $line;
    $max_range = @calls if (! defined($max_range));
    $include = 0;
    $include = 1 if ( ($count >= $begin) and (($end == -1) or ($count <= $end)));
    printf TMP "%d\t", $count if ($include);
    printf "%d\t", $count if $show_data;
    for (my $i = 0; $i <= $max_range; $i++)
    {
	if(defined($COLs[$i]))
	{
	    if (defined($calls[$i]) and ($calls[$i] != 0))
	    {
		my $val = $calls[$i] ;
		printf TMP "%.6f\t", $val if ($include);
		printf "%.6f\t", $val if $show_data;
	    }
	    else
	    {
		printf TMP "\"\"f\t", $val if ($include);
		printf "\"\"\t", $val if $show_data;
	    }
	}
    }
    $count++;
    print TMP "\n" if ($include);
    print "\n" if $show_data;
}
close(TMP);
close(DATA);

# Now produce a gnuplot script to show the data in the TMP file.
open (GNU, ">$gnu_file") or die "Could not open gnuplot script file $gnu_file for output";
# eps output
#print GNU "set terminal postscript eps\nset size 0.5,0.5\n";
#print GNU "set output \"syscalls.eps\"\n";
# gif output
#print GNU "set terminal gif size 640,480\nset size 1,1\n";
#print GNU "set output \"syscalls.gif\"\n";
# thumbnail gif output
#print GNU "set terminal gif size 640,480\nset size 0.5,0.5\n";
#print GNU "set output \"syscalls.thumb.gif\"\n";
#
if(defined($png_file))
{
# Colour graph output
    print GNU "set terminal png\nset size 1,1\n";
    print GNU "set output \"$png_file\"\n";
}
else
{
# On screen output
    print GNU "set terminal X11\nset size 1,1\n";
}
print GNU "set title \"Syscall Profile for $col_arg in $data_file\"\n";
print GNU "set nokey \n";
print GNU "set xlabel \"system call number\"\n";
print GNU "set ylabel \"time (seconds)\"\n";
print GNU "plot ";
my $col;
for ($i = 1; $i < $num_cols; $i++)
{
    $col = $i + 1;
    print GNU "'$tmp_file' using 1:$col with lines,";
}
$col = $num_cols + 1;
print GNU "'$tmp_file' using 1:$col with lines\n";
close(GNU);
`gnuplot -persist $gnu_file`;

unlink $tmp_file if (! $keep_files);
unlink $gnu_file if (! $keep_files);
