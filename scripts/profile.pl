#!/usr/bin/perl -w 
#*****************************************************************************\
#*  $Id: profile.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
#  profile.pl [-h] [-k] [-p <print>] [-v]
# Produce a graph showing:
# The minimum and maximum time of completion of each transfer accross all 
# nodes.

use Getopt::Std;
use Time::Local;

use constant GETOPTS_ARGS => "hkp:v";
use vars map { '$opt_' . $_ } split(/:*/, GETOPTS_ARGS);
my $keep_files = 0;
my $png_file;
my $verbose = 0;

my $prog = $0;
my $path = ".";
if ($prog =~ /^(.*)\/([^\/]+)$/ )
{
    $path = $1;
    $prog = $2;
}

sub usage
{
    print $_[0] . "\n" if (defined($_[0]));
    print "$prog [-h] [-k] [-p <png_file> [-v]\n";
    print "Graph syscall profiles completion time (range) vs step\n";
    print "-h\t\tthis message\n";
    print "-k\t\tKeep (ie.e do not delete) the data and gnuplot tmp files\n";
    print "-p <print>\tSave the graph to the file \"<print>.png\" rather than display it\n";
    print "-v\tverbose\n";
    exit(0);
}

usage() unless(getopts(GETOPTS_ARGS));

($opt_h) && usage();
$keep_files = $opt_k if ($opt_k);
$png_file = $opt_p if ($opt_p);
if ( defined($png_file) and ! ($png_file =~ /\.png$/) )
{
    $png_file = $png_file . ".png";
    print"Graph output to $png_file\n";
}
$verbose = 1 if ($opt_v);


my $tmp_file = "/tmp/composite.tmp.$$";
open(TMP, ">$tmp_file") or die "Could not open tmp file $tmp_file";
sub read_data
{
    my $call = 0;
    my $aggregate = 0;
    my $line = <>;
    my $first;
    my $last;
    # The first line is before the first system call
    if( ! defined($line) )
    {
	printf "No data found\n";
	exit(1);
    }
    my @calls = split /\s/, $line;
    my $num_nodes = @calls - 1;
    for (my $index = 0; $index <= $num_nodes; $index++)
    {
	if ( defined($calls[$index]) and ($calls[$index] != 0) )
	{
	    $first = $calls[$index] if ( ! defined($first));
	    $first = ($first < $calls[$index]) ? $first : $calls[$index];
	}
    }
    if ( ! defined($first) )
    {
	printf "No data found in call number %d\n", $call;
	exit(1);
    }
    $call++;
    while( defined($line = <>) )
    {
	my $min;
	my $max;
	chomp($line);

	@calls = split /\s/, $line;

	for (my $index = 0; $index <= $num_nodes; $index++)
	{
	    if ( defined($calls[$index]) and ($calls[$index] != 0) )
	    {
		$min = $calls[$index] if ( ! defined($min));
		$min = ($min < $calls[$index]) ? $min : $calls[$index];
		$max = $calls[$index] if ( ! defined($max) );
		$max = ($max > $calls[$index]) ? $max : $calls[$index];
		$aggregate++ if ( $call > 0 );
	    }
	}
	# $min and $max are either both defined or neither defined
	if ( ! (defined($min) && defined($max)) )
	{
	    printf "No data found in call number %d\n", $call;
	    exit(1);
	}
	else
	{
	    printf TMP "%d\t%d\t%d\n", $call, $min, $max;
	}
	# At this point both $min and $max are defined
	$last = $max if (! defined($last));
	$last = ($last > $max) ? $last : $max;
	$call++;
    }
    printf "%d aggregate system calls in %d seconds\n", $aggregate, $last - $first;
    close(TMP);
}
# $syscall_run, $syscall_start
# End of reads





my $gnu_file = "/tmp/profile.$$.gnuplot";
my $do_plot = 0;
sub write_gnuplot_file
{

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
    print GNU "set title \"Profile of system call data\n";
    print GNU "set xlabel \"Approximate time in seconds\"\n";
    print GNU "set ylabel \"System call number\"\n";
    print GNU "plot '$tmp_file' using 2:1 title \"earliest\" with lines,'$tmp_file' using 3:1 title \"latest\" with lines\n";
    close(GNU);
    return 1;
}

read_data;
$do_plot = write_gnuplot_file;
`gnuplot -persist $gnu_file` if ($do_plot);

unlink $tmp_file if (! $keep_files);
unlink $gnu_file if (! $keep_files);
    
