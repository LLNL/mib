#!/usr/bin/perl -w 
#*****************************************************************************\
#*  $Id: lwatch.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
#  lwatch.pl [-f <file>] [-h] [-k] [-p <print>]
# Produce a graph of the manually recorded lwatch.py readings.

use Time::Local;
use Getopt::Std;

use constant GETOPTS_ARGS => "f:hkp:";
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
my @wvals;
my @rvals;
my $wval;
my $rval;
my $png_file;
my $data_file = "lwatch.log";
my $tmp_file = "/tmp/lwatch.tmp.$$";
my $gnu_file = "/tmp/lwatch.$$.gnuplot";
my $keep_files = 0;
my $count = 0;
my $max;

sub usage
{
    print $_[0] . "\n" if (defined($_[0]));
    print "$prog [-h] [-k] [-p <png_file>\n";
    print "Graph lwatch-lustre observations\n";
    print "-f <file>\tGet lwatch-lustre log data from <file> (default \"lwatch.log\")\n";
    print "-h\t\tthis message\n";
    print "-k\t\tKeep (ie.e do not delete) the data and gnuplot tmp files\n";
    print "-p <print>\tSave the graph to the file \"<print>.png\" rather than display it\n";
    exit(0);
}

sub get_month
{
    my $month_name = $_[0];
    my @month_array = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");
    my $mon = 0;
    
    while( ($mon < 12) and ($month_array[$mon] ne $month_name)  )
    {
	$mon++;
    }
    return $mon;
}

usage() unless(getopts(GETOPTS_ARGS));

($opt_h) && usage();

$data_file = $opt_f if ($opt_f);
$keep_files = $opt_k if ($opt_k);
$png_file = $opt_p if ($opt_p);
if ( $opt_p and ! ($png_file =~ /\.png$/) )
{
    $png_file = $png_file . ".png";
    print"Graph output to $png_file\n";
}

-f $data_file or die "I did not see the data file $data_file";
open(DATA, "<$data_file") or die "I could not open the data file $data_file";

my $check = 0;
my $day_name;
my $month_name;
my $mon;
my $day;
my $hour;
my $minute;
my $second;
my $year;
my $date_string;
my $start_secs;
my $end_secs;
while( defined($line = <DATA>) )
{
    $date_string = "";
    chomp($line);
    if( $line =~ /^(\S{3}) (\S{3}) ([ \d]{2}) (\d{2}):(\d{2}):(\d{2}) (\d{4}) (.*)$/ )
    {
	$day_name = $1;
	$month_name = $2;
	$mon = get_month($month_name);
	$day = $3;
	$hour = $4;
	$minute = $5;
	$second = $6;
	$year = $7;
	$date_string = "$day_name $month_name $day $hour:$minute:$second $year";
	$start_secs = timelocal($second,$minute,$hour,$day,$mon,$year) if ( not defined($start_secs) );
	$end_secs = timelocal($second,$minute,$hour,$day,$mon,$year);
	$line = $8;
    }
    if( $line =~ /^0\.0\s+0\.0$/ )
    {
	$check = 1;
	$rvals[$count] = 0.0;
	$wvals[$count] = 0.0;
	$count++;	
    }
    elsif( $line =~ /^0\s+0$/ )
    {
	$check = 1;
	$rvals[$count] = 0.0;
	$wvals[$count] = 0.0;
	$count++;	
    }
    elsif( $line =~ /^([\d\.]+)\s+([\d\.]+)$/ )
    {
	$rvals[$count] = $1;
	$wvals[$count] = $2;
	if ($check == 1)
	{
	    $check = 0;
	    if($count > 1)
	    {
		if($rvals[$count-2] != 0.0)
		{
		    $rvals[$count - 1] = ($rvals[$count - 2] + $rvals[$count])/2;
		}
		if($wvals[$count-2] != 0.0)
		{
		    $wvals[$count - 1] = ($wvals[$count - 2] + $wvals[$count])/2;
		}
	    }
	}
	$count++;
    }
}
$max = $count;
$elapsed_time = $end_secs - $start_secs if ( defined($start_secs) and defined($end_secs) );
printf "%d seconds elapsed time\n", $elapsed_time if defined($elapsed_time);

open(TMP, ">$tmp_file") or die "Could not open tmp file $tmp_file";
for ($count = 0; $count < $max; $count++)
{
    printf TMP "%d\t%f\n", $count*6, $wvals[$count];
}
print TMP "\n\n";
for ($count = 0; $count < $max; $count++)
{
    printf TMP "%d\t%f\n", $count*6, $rvals[$count];
}

close(TMP);

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
print GNU "set title \"Lwatch.py readings from $data_file\"\n";
print GNU "set nokey \n";
print GNU "set xlabel \"Approximate time in seconds\"\n";
print GNU "set ylabel \"Data rate (MB/s)\"\n";
print GNU "plot '$tmp_file' index 0 using 1:2 title \"write\" with lines,'$tmp_file' index 1 using 1:2 title \"read\" with lines\n";
close(GNU);
`gnuplot -persist $gnu_file`;

unlink $tmp_file if (! $keep_files);
unlink $gnu_file if (! $keep_files);
