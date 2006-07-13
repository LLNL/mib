#!/usr/bin/perl -w 
#*****************************************************************************\
#*  $Id: diff.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
#  diff.pl [-d <log_dir>] [-h] [-k] [-p <print>] [-s <scale>] 
# Produce a graph combining:
# o rates observed by mib
# o lwatch-lustre trace
# o datarates calulated from syscall profiles
# Allow for manual adjustment of the coordination between lwatch and syscall
# timing data.

use Getopt::Std;
use Time::Local;

use constant GETOPTS_ARGS => "d:hkp:";
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
my @LastW;
my @LastR;
my $wval;
my $rval;
my $call;
my $png_file;
my $log_dir = "";
my $mib_log = "log";
my $options = "options";
my $lwatch_trace = "lwatch";
my $read_calls = "read.syscall.aves";
my $write_calls = "write.syscall.aves";
my $tmp_file = "/tmp/diff.tmp.$$";
my $gnu_file = "/tmp/diff.$$.gnuplot";
my $keep_files = 0;
my $scale = 5;  # This is just a swag, though not far from correct.  It gets set from timestamps,
                # but some older logs don't have timestamps.

sub usage
{
    print $_[0] . "\n" if (defined($_[0]));
    print "$prog [-d <log_dir>] [-h] [-k] [-p <png_file>\n";
    print "Graph lwatch-lustre trace, mib rates, and datarates from syscall profiles\n";
    print "-d <log_dir>\tLocation of collected info\n";
    print "-h\t\tthis message\n";
    print "-k\t\tKeep (ie.e do not delete) the data and gnuplot tmp files\n";
    print "-p <print>\tSave the graph to the file \"<print>.png\" rather than display it\n";
    exit(0);
}

sub month_num 
{
    my $month_str = $_[0];
    my @month_array = ("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");
    my $num_months = @month_array;
    my $num = $num_months - 1;
    
    while( ($num >= 0) && ($month_array[$num] ne $month_str) )
    {
	$num--;
    }
    return($num);
}

usage() unless(getopts(GETOPTS_ARGS));

($opt_h) && usage();
(! $opt_d) && usage();
$log_dir = $opt_d;
[ -d $log_dir ] or usage();
$log_dir .= '/' if ( rindex($log_dir, "/") != (length($log_dir) - 1)  );

$keep_files = $opt_k if ($opt_k);
$png_file = $opt_p if ($opt_p);
if ( $opt_p and ! ($png_file =~ /\.png$/) )
{
    $png_file = $png_file . ".png";
    print"Graph output to $png_file\n";
}

$options = $log_dir . $options;
$mib_log = $log_dir . $mib_log;
$lwatch_trace = $log_dir . $lwatch_trace;
$read_calls = $log_dir . $read_calls;
$write_calls = $log_dir . $write_calls;

-f $options or die "I did not see the options file $options";
-f $mib_log or die "I did not see the mib log file $mib_log";
-f $lwatch_trace or die "I did not see the lwatch-lustre trace file $lwatch_trace";
-f $read_calls or die "I did not see the read system calls file  $read_calls";
-f $write_calls or die "I did not see the write system calls file $write_calls";

my $mib_write;
my $mib_read;
my $mib_xfer;
my $CNs = `grep cns $options | awk '{print \$3}'`;
my $IONs = `grep ions $options | grep -v iterations | awk '{print \$3}'`;
my $CNs_per_ION = $CNs/$IONs;
my $call_size_string;

open(MIB, "<$mib_log") or die "I could not open the mib log file $mib_log";

while(defined($line = <MIB>))
{
    chomp($line);
    if( $line =~ /^\s*call_size\s*=\s*(\d+)\s*MiB\s*$/)
    {
	$mib_xfer = $1 * 1024 * 1024;
    }
    elsif( $line =~ /^\s*call_size\s*=\s*(\d+)\s*kiB\s*$/)
    {
	$mib_xfer = $1 * 1024;
    }
    elsif( $line =~ /^\s*call_size\s*=\s*(\d+)\s*bytes\s*$/)
    {
	$mib_xfer = $1;
    }
    elsif( $line =~ /^\s*call_size\s*=\s*(\d+)\s*$/)
    {
	$mib_xfer = $1;
    }
    elsif( $line =~ /^Aggregate Write Rate\s*=\s*([\.\d]+)\s+MB\/s\s*$/)
    {
	$mib_write = $1;
    }
    elsif( $line =~ /^Aggregate Read Rate\s*=\s*([\.\d]+)\s+MB\/s\s*$/)
    {
	$mib_read = $1;
    }
    elsif( $line =~ /^Aggregate Write Rate\s*=\s*([\.\d]+)\s*$/)
    {
	$mib_write = $1;
    }
    elsif( $line =~ /^Aggregate Read Rate\s*=\s*([\.\d]+)\s*$/)
    {
	$mib_read = $1;
    }
    elsif( $line =~ /(\d+)\s+([\dkM]+)\s+(\d+)\s+(\d+)\s+([\.\d]+)\s+([\.\d]+)$/)
    {
	$call_size_string = $2;
	$mib_write = $5;
	$mib_read  = $6;
	if ( $call_size_string =~ /^(\d+)k$/)
	{
	    $mib_xfer = $1 *1024;
	} 
	elsif ( $call_size_string =~ /^(\d+)M$/)
	{
	    $mib_xfer = $1 * 1024 * 1024;
	}
	else
	{
	    $mib_xfer = $call_size_string;
	}
    }
}
close(MIB);
defined($mib_xfer) or die "Didn't find mib xfer size";
defined($mib_write) or die "Didn't find mib write rate";
defined($mib_read) or die "Didn't find mib read rate";
#print "$mib_read, $mib_write, $mib_xfer\n";

open(LWATCH, "<$lwatch_trace") or die "I could not open the lwatch trace file $lwatch_trace";

my $count = 0;
my $max;
my $check = 0;
my $day_name;
my $month_name;
my $day;
my $hour;
my $minute;
my $second;
my $year;
my $start_time;
my $end_time;
my $time_stamp;
while( defined($line = <LWATCH>) )
{
    chomp($line);
    if( $line =~ /^(\S{3}) (\S{3}) ([ \d]{2}) (\d{2}):(\d{2}):(\d{2}) (\d{4}) (.*)$/ )
    {
	$day_name = $1;
	$month_name = $2;
	$day = $3;
	$hour = $4;
	$minute = $5;
	$second = $6;
	$year = $7;
	$line = $8;
	$time_stamp = timelocal($second, $minute, $hour, $day, month_num($month_name), $year);
	$start_time = $time_stamp if (not defined($start_time));
	$end_time = $time_stamp;
    }
    if( $line =~ /^\s*0\.0\s+0\.0\s*$/ )
    {
	$check = 1;
	$rvals[$count] = 0.0;
	$wvals[$count] = 0.0;
	$count++;	
    }
    elsif( $line =~ /^\s*0\s+0\s*$/ )
    {
	$check = 1;
	$rvals[$count] = 0.0;
	$wvals[$count] = 0.0;
	$count++;	
    }
    elsif( $line =~ /^\s*([\d\.]+)\s+([\d\.]+)\s*$/ )
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
close(LWATCH);
$max = $count;
$scale = ($end_time - $start_time)/$max if ( ($max > 0) and defined($start_time) and defined($end_time) );
printf "max time for lwatch is %.0f\n", $max * $scale if ($verbose);

my $max_write_run = 0;
my $max_read_run = 0;
my $write_run = 0;
my $read_run = 0;
my $start_write = $max - 1;
my $start_read = $max - 1;
my $i;
for ($i = $max - 2; $i  >= 0; $i--)
{
    if($wvals[$i+1] > 0)
    {
	$write_run++;
    }
    else
    {
	$start_write = ($max_write_run > $write_run) ? $start_write : $i + 1;
	$max_write_run = ($max_write_run > $write_run) ? $max_write_run : $write_run;
	$write_run = 0;
    }
    if($rvals[$i+1] > 0)
    {
	$read_run++;
    }
    else
    {
	$start_read = ($max_read_run > $read_run) ? $start_read : $i + 1;
	$max_read_run = ($max_read_run > $read_run) ? $max_read_run : $read_run;
	$read_run = 0;
    }
}
my $lwatch_start_write = ($max_write_run > $write_run) ? $start_write : 0;
my $lwatch_start_read = ($max_read_run > $read_run) ? $start_read : 0;
my $lwatch_max = $max;

my $end = $lwatch_start_read;
while ( (defined($rvals[$end + 1]) and defined($rvals[$end + 2])) and 
	 ! ( ($rvals[$end + 1] == 0) and ($rvals[$end + 2] == 0) ) )
{
    $end++;
}
my $lwatch_end_read = $end;
printf "lwatch: write start = %.0f, read start = %.0f, end read = %.0f\n", $lwatch_start_write*$scale, $lwatch_start_read*$scale , $lwatch_end_read*$scale if ($verbose);

open(WRITE, "<$write_calls") or die "I could not open the write system calls file $write_calls";


# The first row of values is before the first system call, so don't
# count them in.
$line = <WRITE>;
chomp($line);
@calls = split /\s/, $line;
my $start = $calls[0];
my $max_range = @calls - 1;
# This loop reads in the system call timings table, and notes the 
# time each syscall completes, by incrementing that second's count.
$end = 0;
my @Write;
$call = 1;
while( defined($line = <WRITE>) )
{
    chomp($line);
    @calls = split /\s/, $line;
    for ($i = 0; $i <= $max_range; $i++)
    {
	if ( defined($calls[$i]) and ($calls[$i] != 0) )
	{
	    my $val = int($calls[$i]);
	    $LastW[$i] = $call;
	    $end = ($end > $val) ? $end : $val;
	    if (!defined($Write[$val]))
	    {
		$Write[$val] = 1;
	    }
	    else
	    {
		$Write[$val]++;
	    }
	}
    }
    $call++;
}
close(WRITE);
$sum = 0;
for (my $i = 0; $i <= $max_range; $i++)
{
    $sum += $LastW[$i] if (defined($LastW[$i]));
}
printf("system call size = %d\n", $mib_xfer);
printf("CNs_per_ION = %d\n", $CNs_per_ION);
printf("Aggregate number of write system calls = %d\n", $sum);
printf("Start time = %d\n", $start);
printf("End time = %d\n", $end);
printf("Write rate: %f MB/s\n", ($sum*$mib_xfer*$CNs_per_ION)/(1024*1024*($end - $start)));

my $write_max = $end;
printf "max time for write syscalls is %d\n", $write_max if ($verbose);

open(READ, "<$read_calls") or die "I could not open the read system calls file $read_calls";


# The first row of values is before the first system call, so don't
# count them in.
$line = <READ>;
chomp($line);
@calls = split /\s/, $line;
$start = $calls[0];
$max_range = @calls - 1;
# This loop reads in the system call timings table, and notes the 
# time each syscall completes, by incrementing that second's count.
$end = 0;
my @Read;
$call = 1;
while( defined($line = <READ>) )
{
    chomp($line);
    @calls = split /\s/, $line;
    for ($i = 0; $i <= $max_range; $i++)
    {
	if ( defined($calls[$i]) and ($calls[$i] != 0) )
	{
	    my $val = int($calls[$i]);
	    $LastR[$i] = $call;
	    $end = ($end > $val) ? $end : $val;
	    if (!defined($Read[$val]))
	    {
		$Read[$val] = 1;
	    }
	    else
	    {
		$Read[$val]++;
	    }
	}
    }
    $call++;
}
close(READ);
my $sum = 0;
for (my $i = 0; $i <= $max_range; $i++)
{
    $sum += $LastR[$i] if (defined($LastR[$i]));
}
printf("Aggregate number of read system calls = %d\n", $sum);
printf("Start time = %d\n", $start);
printf("End time = %d\n", $end);
printf("Read rate:  %f MB/s\n", ($sum*$mib_xfer*$CNs_per_ION)/(1024*1024*($end - $start)));

my $read_max = $end;
printf "max time for read syscalls is %d\n", $read_max if ($verbose);


$max = ($read_max > $write_max) ? $read_max : $write_max;
$max_write_run = 0;
$max_read_run = 0;
$start_write = $max - 1;
$start_read = $max - 1;
for ($i = $max - 2; $i >= 0; $i--)
{
    $Write[$i+1] = 0 if(! defined($Write[$i+1]));
    if($Write[$i+1] > 0)
    {
	$write_run++;
    }
    else
    {
	$start_write = ($max_write_run > $write_run) ? $start_write : $i + 1;
	$max_write_run = ($max_write_run > $write_run) ? $max_write_run : $write_run;
	$write_run = 0;
    }
    $Read[$i+1] = 0 if(! defined($Read[$i+1]));
    if($Read[$i+1] > 0)
    {
	$read_run++;
    }
    else
    {
	$start_read = ($max_read_run > $read_run) ? $start_read : $i + 1;
	$max_read_run = ($max_read_run > $read_run) ? $max_read_run : $read_run;
	$read_run = 0;
    }
}
my $syscall_start_write = ($max_write_run > $write_run) ? $start_write : 0;
my $syscall_start_read = ($max_read_run > $read_run) ? $start_read : 0;

$end = $syscall_start_read;
while ( (defined($Read[$end + 1]) and defined($Read[$end + 2])) and 
	 ! ( ($Read[$end + 1] == 0) and ($Read[$end + 2] == 0) ) )
{
    $end++;
}
my $syscall_end_read = $end;
printf "syscall: write start = %d, read start = %d, read end = %d\n", $syscall_start_write, $syscall_start_read, $syscall_end_read if ($verbose);

my $syscall_xw_read_end = int(($lwatch_end_read - $lwatch_start_write)*$scale + $syscall_start_write);
$max = ($syscall_end_read > $syscall_xw_read_end) ? $syscall_end_read : $syscall_xw_read_end;
my $xw_min = int($lwatch_start_write - ($syscall_start_write/$scale));
my $xw_max = int(($max - $syscall_start_write)/$scale + $lwatch_start_write);

$xw_min = 0 if ($xw_min < 0);
open(TMP, ">$tmp_file") or die "Could not open tmp file $tmp_file";
for ($count = $xw_min; $count < $xw_max; $count++)
{
    $wvals[$count] = 0 if (! defined($wvals[$count]));
    $rvals[$count] = 0 if (! defined($rvals[$count]));
    my $xt = ($count - $lwatch_start_write)*$scale + $syscall_start_write;
}

printf TMP "\n\n";

$max_yval = 1;
$read_sum = 0;
for ($t = 0; $t < $syscall_end_read; $t++)
{
    $lwatch_count = int((($t - $syscall_start_write)/$scale) + $lwatch_start_write);
    $Read[$t] = 0 if (!defined($Read[$t]));
    $read_diff = $rvals[$lwatch_count] - $Read[$t]*($CNs_per_ION*$mib_xfer)/(1024*1024);
    $read_sum += $read_diff;
    $max_yval = (abs($read_diff) > $max_yval) ? abs($read_diff) : $max_yval;
    $Write[$t] = 0 if (!defined($Write[$t]));
    $write_diff = $wvals[$lwatch_count] - $Write[$t]*($CNs_per_ION*$mib_xfer)/(1024*1024);
    $write_sum += $write_diff;
    $max_yval = (abs($write_diff) > $max_yval) ? abs($write_diff) : $max_yval;
    printf TMP "%d\t%f\t%f\n", $t, $write_diff, $read_diff;
}
close(TMP);

printf "Integral of the difference for writes: %f and reads: %f\n", $write_sum, $read_sum;

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
print GNU "set title \"diff readings from $log_dir\"\n";
print GNU "set xlabel \"Approximate time in seconds\"\n";
print GNU "set ylabel \"Data rate difference (MB/s)\"\n";
print GNU "set yrange [-$max_yval:$max_yval]\n";
print GNU "plot '$tmp_file' index 0 using 1:2 title \"write diff\" with lines,'$tmp_file' index 0 using 1:3 title \"read diff\" with lines\n";
close(GNU);
`gnuplot -persist $gnu_file`;

unlink $tmp_file if (! $keep_files);
unlink $gnu_file if (! $keep_files);
