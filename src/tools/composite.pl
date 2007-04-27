#!/usr/bin/perl -w 
#*****************************************************************************\
#*  $Id: composite.pl 1.9 2005/11/30 20:24:57 auselton Exp $
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
#
#  composite.pl [-d <log_dir>] [-h] [-k] [-p <print>] [-s <scale>] 
# Produce a graph combining:
# o rates observed by mib
# o lwatch-lustre trace
# o datarates calulated from syscall profiles
# Allow for manual adjustment of the coordination between lwatch and syscall
# timing data.

use Getopt::Std;
use Time::Local;

use constant GETOPTS_ARGS => "d:hkp:v";
use vars map { '$opt_' . $_ } split(/:*/, GETOPTS_ARGS);
my $log_dir = ".";
my $keep_files = 0;
my $png_file;
if ($log_dir =~ /^(.*)\/([^\/]+)$/ )
{
    $png_file = $2;
}
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
    print "$prog [-d <log_dir>] [-h] [-k] [-p <png_file>\n";
    print "Graph lwatch-lustre trace, mib rates, and datarates from syscall profiles\n";
    print "-d <log_dir>\tLocation of collected info\n";
    print "-h\t\tthis message\n";
    print "-k\t\tKeep (ie.e do not delete) the data and gnuplot tmp files\n";
    print "-p <print>\tSave the graph to the file \"<print>.png\" rather than display it\n";
    print "-s <scale>\tScale the lwatch trace by this amount (default 5)\n";
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
if ( defined($png_file) and ! ($png_file =~ /\.png$/) )
{
    $png_file = $png_file . ".png";
    print"Graph output to $png_file\n";
}
$verbose = 1 if ($opt_v);

my $Tasks_per_Node;
my $mib_plot;
my $mib_write;
my $mib_read;
my $mib_xfer;
sub read_log 
{
# Given $log_dir read the file and get the given values    
    my $call_size_string;
    my $mib_log = "log";
    my $line;
    my $use_node_aves;

    $mib_log = $log_dir . $mib_log;
    if ( ! -f $mib_log )
    {
	print "No mib log file $mib_log\n";
	$mib_xfer = 512*1024;
	return 0;
    }

    $use_node_aves = `grep use_node_aves $mib_log | awk '{print \$3}'`;
    if ( !defined($use_node_aves) || ($use_node_aves eq "true") )
    {
	$Tasks = `grep tasks $mib_log | awk '{print \$3}'`;
	$Nodes = `grep nodes $mib_log | awk '{print \$3}'`;
	$Tasks_per_Node = int($Tasks/$Nodes);
    }
    else
    {
	$Tasks_per_Node = 1;
    }
    defined($Tasks_per_Node) or die "Didn't find Tasks_per_Node value from log";
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
    return 0 if ( ! defined($mib_write) or ! defined($mib_read) ) ;
    #print "$mib_read, $mib_write, $mib_xfer\n";
    return 1;
}
# $mib_plot, $mib_write, $mib_read, $mib_xfer, $Tasks_per_Node
#End of read_log

my $lwatch_plot;
my $lwatch_scale;
my $lwatch_peak = 0;
my $lwatch_start_write;
my $lwatch_end_write;
my $lwatch_start_read;
my $lwatch_end_read;
my $lwatch_max;
my @lwatch_W;
my @lwatch_R;
# It's not clear that this really needs the large scope.
# How is it being used?
sub read_lwatch
{
    my $lwatch_trace = "lwatch";

    $lwatch_trace = $log_dir . $lwatch_trace;
    if ( ! -f $lwatch_trace)
    {
	print "No lwatch lustre trace file $lwatch_trace\n";
	$lwatch_start_write = 0;
	$lwatch_end_read = 0;
	$lwatch_scale = 0;
	return 0;
    }

    open(LWATCH, "<$lwatch_trace") or die "I could not open the lwatch trace file $lwatch_trace";
    
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
    my $line;
    my $index = 0;
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
	    $lwatch_R[$index] = 0.0;
	    $lwatch_W[$index] = 0.0;
	    $index++;	
	}
	elsif( $line =~ /^\s*0\s+0\s*$/ )
	{
	    $check = 1;
	    $lwatch_R[$index] = 0.0;
	    $lwatch_W[$index] = 0.0;
	    $index++;	
	}
	elsif( $line =~ /^\s*([\d\.]+)\s+([\d\.]+)\s*$/ )
	{
	    $lwatch_R[$index] = $1;
	    $lwatch_W[$index] = $2;
	    if ($check == 1)
	    {
		$check = 0;
		if($index > 1)
		{
		    if($lwatch_R[$index-2] != 0.0)
		    {
			$lwatch_R[$index - 1] = ($lwatch_R[$index - 2] + $lwatch_R[$index])/2;
		    }
		    if($lwatch_W[$index-2] != 0.0)
		    {
			$lwatch_W[$index - 1] = ($lwatch_W[$index - 2] + $lwatch_W[$index])/2;
		    }
		}
	    }
	    $index++;
	}
    }
    close(LWATCH);
    $lwatch_max = $index;
    $lwatch_scale = ($end_time - $start_time)/$lwatch_max if ( ($lwatch_max > 0) and defined($start_time) and defined($end_time) );
    printf "lwatch: count = %d, scale = %.0f\n", $lwatch_max, $lwatch_scale if ($verbose);
    printf "\tmax time = %.0f\n", $lwatch_max * $lwatch_scale if ($verbose);
    
    my $run = 0;
    my $max_run = 0;
    my $start_write = $lwatch_max - 1;
    for (my $index = $lwatch_max - 2; $index  >= 0; $index--)
    {
	if($lwatch_W[$index+1] > 0)
	{
	    $run++;
	}
	else
	{
	    $start_write = ($max_run > $run) ? $start_write : $index + 1;
	    $max_run = ($max_run > $run) ? $max_run : $run;
	    $run = 0;
	}
    }
    $start_write = ($max_run > $run) ? $start_write : 1;

    $run = 0;
    $max_run = 0;
    my $start_read = $lwatch_max - 1;
    for (my $index = $lwatch_max - 2; $index  >= 0; $index--)
    {
	if($lwatch_R[$index+1] > 0)
	{
	    $run++;
	}
	else
	{
	    $start_read = ($max_run > $run) ? $start_read : $index + 1;
	    $max_run = ($max_run > $run) ? $max_run : $run;
	    $run = 0;
	}
    }
    $start_read = ($max_run > $run) ? $start_read : 1;

# Note that the write value is used later (outside this scope), but
# the read value is not.
    $lwatch_start_write = $start_write;
    $lwatch_start_read = $start_read;

    my $end = $lwatch_start_write;
    while ( (defined($lwatch_W[$end + 1]) and defined($lwatch_W[$end + 2])) and 
	    ! ( ($lwatch_W[$end + 1] == 0) and ($lwatch_W[$end + 2] == 0) ) )
    {
	$end++;
    }
    $lwatch_end_write = $end;
    $end = $lwatch_start_read;
    while ( (defined($lwatch_R[$end + 1]) and defined($lwatch_R[$end + 2])) and 
	    ! ( ($lwatch_R[$end + 1] == 0) and ($lwatch_R[$end + 2] == 0) ) )
    {
	$end++;
    }
    $lwatch_end_read = $end;
    printf "\twrites: start = %.0f, end = %.0f\n", $lwatch_start_write, $lwatch_end_write if ($verbose);
    printf "\treads: start = %.0f, end = %.0f\n", $lwatch_start_read, $lwatch_end_read if ($verbose);

# Note the peak value in case we can't scale based on the log's report.
# Make a stab at the aggregate write and read rates.
    my $sum = 0;
    $run = $lwatch_end_write - $lwatch_start_write;
    my $period = $lwatch_scale*$run;
    if( $period > 0 )
    {
	for(my $index = $lwatch_start_write; $index <= $lwatch_end_write; $index++)
	{
	    my $greater = ($lwatch_W[$index] > $lwatch_R[$index]) ? $lwatch_W[$index] : $lwatch_R[$index];
	    
	    $lwatch_peak = $greater if ( ! defined($lwatch_peak) );
	    $lwatch_peak = ($lwatch_peak > $greater) ? $lwatch_peak : $greater;

	    $sum += $lwatch_W[$index];
	}
	printf "\taggregate write rate = %f\n", ($sum*$lwatch_scale)/$period if ($verbose);
    }
    $run = $lwatch_end_read - $lwatch_start_read;
    $period = $lwatch_scale*$run;
    $sum = 0;
    if( $period > 0 )
    {
	for(my $index = $lwatch_start_read; $index <= $lwatch_end_read; $index++)
	{
	    my $greater = ($lwatch_W[$index] > $lwatch_R[$index]) ? $lwatch_W[$index] : $lwatch_R[$index];
	    
	    $lwatch_peak = $greater if ( ! defined($lwatch_peak) );
	    $lwatch_peak = ($lwatch_peak > $greater) ? $lwatch_peak : $greater;

	    $sum += $lwatch_R[$index];
	}
	printf "\taggregate read rate  = %f\n", ($sum*$lwatch_scale)/$period if ($verbose);
    }
    return 0 if ($lwatch_peak == 0);
    return 1;
}
# $lwatch_plot, $lwatch_scale, $lwatch_start_write,  
# $lwatch_end_read, @lwatch_R, @lwatch_W, $lwatch_peak
# End of read_lwatch


my $plot_write;
my @Write;
my @LastW;
my $write_max;
my $write_min;
my $syscall_start_write;
my $syscall_write_run;
my $data_written = 0;
sub read_writes
{
    my $write_calls = "profile.write";
    my $line;
    my $call;

    $write_calls = $log_dir . $write_calls;
    if ( ! -f $write_calls ) 
    {
	print "No write system calls file $write_calls\n";
	$syscall_write_run = 0;
	$syscall_start_write = 0;
	return 0;
    }
    
    open(WRITE, "<$write_calls") or die "I could not open the write system calls file $write_calls";
    
    
# The first row of values is before the first system call, so don't
# count them in.
    $line = <WRITE>;
    if ( ! defined($line) ) 
    {
	print "Empty write system calls file $write_calls\n";
	return 0;
    }
    chomp($line);
    @calls = split /\s/, $line;
    my $start = $calls[0];
    my $max_range = @calls - 1;
# This loop reads in the system call timings table, and notes the 
# time each syscall completes, by incrementing that second's count.
    $end = 0;
    $call = 1;
    while( defined($line = <WRITE>) )
    {
	chomp($line);
	@calls = split /\s/, $line;
	for (my $index = 0; $index <= $max_range; $index++)
	{
	    if ( defined($calls[$index]) and ($calls[$index] != 0) )
	    {
		my $val = int($calls[$index]);
		$LastW[$index] = $call;
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
    for (my $index = 0; $index <= $max_range; $index++)
    {
	$sum += $LastW[$index] if (defined($LastW[$index]));
    }
    $data_written = $sum*$mib_xfer*$Tasks_per_Node/(1024*1024);
    printf("Tasks_per_Node = %d\n", $Tasks_per_Node) if ($verbose);
    printf("Aggregate number of write system calls = %d\n", $sum) if ($verbose);
    printf("Start time = %d\n", $start) if ($verbose);
    printf("End time = %d\n", $end) if ($verbose);
    printf("Write rate: %f MB/s\n", ($sum*$mib_xfer*$Tasks_per_Node)/(1024*1024*($end - $start))) if ($verbose);
    
    $write_min = $start;
    $write_max = $end;
    printf "write syscalls from %d to %d\n", $write_min, $write_max if ($verbose);
    $max = $write_max;
    my $max_write_run = 0;
    my $write_run = 0;
    my $start_write = $max - 1;
    for (my $index = $max - 2; $index >= 0; $index--)
    {
	$Write[$index+1] = 0 if(! defined($Write[$index+1]));
	if($Write[$index+1] > 0)
	{
	    $write_run++;
	}
	else
	{
	    $start_write = ($max_write_run > $write_run) ? $start_write : $index + 1;
	    $max_write_run = ($max_write_run > $write_run) ? $max_write_run : $write_run;
	    $write_run = 0;
	}
    }
    $syscall_start_write = ($max_write_run > $write_run) ? $start_write : 1;
    $syscall_write_run = ($max_write_run > $write_run) ? $max_write_run : $write_run;
    return 1;
}
# $syscall_write_run, $syscall_start_write
# End of read_writes


my $plot_read;
my @Read;
my @LastR;
my $read_min;
my $read_max;
my $syscall_start_read;
my $syscall_read_run;
sub read_reads
{    
    my $read_calls = "profile.read";
    my $line;
    my $call;
    $read_calls = $log_dir . $read_calls;
    if ( ! -f $read_calls)
    {
	print "No read system calls file  $read_calls\n";
	return 0;
    }
    open(READ, "<$read_calls") or die "I could not open the read system calls file $read_calls";
    
    
# The first row of values is before the first system call, so don't
# count them in.
    $line = <READ>;
    if ( ! defined($line) )
    {
	print "Empty read system calls file  $read_calls\n";
	return 0;
    }	
    chomp($line);
    @calls = split /\s/, $line;
    $start = $calls[0];
    $max_range = @calls - 1;
# This loop reads in the system call timings table, and notes the 
# time each syscall completes, by incrementing that second's count.
    $end = 0;
    $call = 1;
    while( defined($line = <READ>) )
    {
	chomp($line);
	@calls = split /\s/, $line;
	for (my $index = 0; $index <= $max_range; $index++)
	{
	    if ( defined($calls[$index]) and ($calls[$index] != 0) )
	    {
		my $val = int($calls[$index]);
		$LastR[$index] = $call;
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
    for (my $index = 0; $index <= $max_range; $index++)
    {
	$sum += $LastR[$index] if (defined($LastR[$index]));
    }
    printf("Aggregate number of read system calls = %d\n", $sum) if ($verbose);
    printf("Start time = %d\n", $start) if ($verbose);
    printf("End time = %d\n", $end) if ($verbose);
    printf("Read rate:  %f MB/s\n", ($sum*$mib_xfer*$Tasks_per_Node)/(1024*1024*($end - $start))) if ($verbose);

    $read_min = $start;
    $read_max = $end;
    printf "max time for read syscalls is %d\n", $read_max if ($verbose);
    
    
    $max = ($read_max > $write_max) ? $read_max : $write_max;
    my $max_read_run = 0;
    my $start_read = $max - 1;
    my $read_run = 0;
    for (my $index = $max - 2; $index >= 0; $index--)
    {
	$Read[$index+1] = 0 if(! defined($Read[$index+1]));
	if($Read[$index+1] > 0)
	{
	    $read_run++;
	}
	else
	{
	    $start_read = ($max_read_run > $read_run) ? $start_read : $index + 1;
	    $max_read_run = ($max_read_run > $read_run) ? $max_read_run : $read_run;
	    $read_run = 0;
	}
    }
    $syscall_start_read = ($max_read_run > $read_run) ? $start_read : 1;
    $syscall_read_run = ($max_read_run > $read_run) ? $max_read_run : $read_run;
    return 1;
}
# $syscall_write_run
# End of read_reads

my $syscall_end_read;
my $xw_min = 0;
my $xw_max = 0;
sub find_bounds
{
    my $syscall_xw_read_end = int(($lwatch_end_read - $lwatch_start_write)*$lwatch_scale + $syscall_start_write);
    if ( $plot_read)
    {
	$end = $syscall_start_read;
	while ( (defined($Read[$end + 1]) and defined($Read[$end + 2])) and 
		! ( ($Read[$end + 1] == 0) and ($Read[$end + 2] == 0) ) )
	{
	    $end++;
	}
	$syscall_end_read = $end;
    }
    else
    {
	$syscall_start_read = $syscall_xw_read_end;
	$syscall_end_read = $syscall_xw_read_end;
    }
    printf "syscall: write start = %d, read start = %d, read end = %d\n", $syscall_start_write, $syscall_start_read, $syscall_end_read if ($verbose);
    
# This does not fairly produce bounds when only lwatch data is available
    $max = ($syscall_end_read > $syscall_xw_read_end) ? $syscall_end_read : $syscall_xw_read_end;
    $xw_min = int($lwatch_start_write - ($syscall_start_write/$lwatch_scale));
    $xw_max = int(($max - $syscall_start_write)/$lwatch_scale + $lwatch_start_write);
    printf "lwatch bounds: min = %d, max = %d\n", $xw_min, $xw_max if ($verbose);
}
# End of find_bounds

my $mib_index;
my $lwatch_index;
my $write_index;
my $read_index;
my $tmp_file = "/tmp/composite.tmp.$$";
sub write_data_file
{
    my $index = 0;
    my $write;
    my $read;
    my $xw_index;
    my $write_min = $syscall_start_write;
    my $write_max = $syscall_start_write;
    my $xt;
    $write_max += $data_written/$mib_write if (defined($mib_write));

    $xw_min = 0 if ($xw_min < 0);
    open(TMP, ">$tmp_file") or die "Could not open tmp file $tmp_file";
    for ($xw_index = $xw_min; $xw_index < $xw_max; $xw_index++)
    {
	$lwatch_W[$xw_index] = 0 if (! defined($lwatch_W[$xw_index]));
	$lwatch_R[$xw_index] = 0 if (! defined($lwatch_R[$xw_index]));
	$xt = ($xw_index - $lwatch_start_write)*$lwatch_scale + $syscall_start_write;
	printf TMP "%.0f\t%f\t%f\n", $xt, $lwatch_W[$xw_index], $lwatch_R[$xw_index];
    }
    $xt = ($xw_max - $lwatch_start_write)*$lwatch_scale + $syscall_start_write;
    printf TMP "%.0f\t%f\t%f\n", $xt, 0, 0;
    $lwatch_index = $index++;
    printf TMP "\n\n";
    
    for ($t = 0; $t < $syscall_end_read; $t++)
    {
	$Write[$t] = 0 if (!defined($Write[$t]));
	$write = $Write[$t]*($Tasks_per_Node*$mib_xfer)/(1024*1024);
	printf TMP "%d\t%f\n", $t, $write;
    }
    printf TMP "%d\t%f\n", $syscall_end_read, 0;
    $write_index = $index++;
    printf TMP "\n\n";
    
    for ($t = 0; $t < $syscall_end_read; $t++)
    {
	$Read[$t] = 0 if (!defined($Read[$t]));
	$read = $Read[$t]*($Tasks_per_Node*$mib_xfer)/(1024*1024);
	printf TMP "%d\t%f\n", $t, $read;
    }
    printf TMP "%d\t%f\n", $syscall_end_read, 0;
    $read_index = $index++;
    printf TMP "\n\n";
    
    for ($t = 0; $t < $syscall_end_read; $t++)
    {
	$read  = 0;
	$read = $mib_read if ( defined($mib_read) && ($t > $read_min) && ($t < $read_max) );
	$write = 0;
	$write = $mib_write if ( defined($mib_write) && ($t > $write_min) && ($t < $write_max) );
	printf TMP "%d\t%f\t%f\n", $t, $write, $read;
    }
    printf TMP "%d\t%f\t%f\n", $syscall_end_read, 0, 0;
    $mib_index = $index;
    close(TMP);
}

my $gnu_file = "/tmp/composite.$$.gnuplot";
my $do_plot = 0;
sub write_gnuplot_file
{
    my $plot_com_mib = "";
    my $plot_com_lwatch = "";
    my $plot_com_write = "";
    my $plot_com_read = "";
    my $need_comma = 0;

    if ( $mib_plot )
    {
	$plot_com_mib = "'$tmp_file' index $mib_index using 1:2 title \"mib write\" with lines,'$tmp_file' index $mib_index using 1:3 title \"mib read\" with lines";
	$need_comma = 1;
	$do_plot = 1;
	print "Will plot mib data.\n" if ($verbose);
    }
    if ( $lwatch_plot )
    {
	$plot_com_lwatch = "'$tmp_file' index $lwatch_index using 1:2 title \"lwatch write\" with lines,'$tmp_file' index 0 using 1:3 title \"lwatch read\" with lines";
	$plot_com_lwatch = ",$plot_com_lwatch" if ($need_comma);
	$need_comma = 1;
	$do_plot = 1;
	print "Will print lwatch data.\n" if ($verbose);
    }
    if ( $plot_write )
    {
	$plot_com_write = "'$tmp_file' index $write_index using 1:2 title \"syscall write\" with lines";
	$plot_com_write = ",$plot_com_write" if ($need_comma);
	$need_comma = 1;
	$do_plot = 1;
	print "Will plot write syscall data\n" if ($verbose);
    }
    if ( $plot_read )
    {
	$plot_com_read = "'$tmp_file' index $read_index using 1:2 title \"syscall read\" with lines";
	$plot_com_read = ",$plot_com_read" if ($need_comma);
	$do_plot = 1;
	print "Will plot read syscall data\n" if ($verbose);
    }
    return 0 if ( ! $do_plot);

# Sometimes the early spike in write rate make the rest of the graph 
# disappear.  Try limiting that.
    my $yrange = 1;
    my $tmp;
    if ( defined($mib_write) )
    {
	$tmp = $mib_write;
    }
    else
    {
	$tmp = $lwatch_peak;
    }

    if (defined ($tmp))
    {
	while ($tmp > 1)
	{
	    $tmp /= 2;
	    $yrange *= 2;
	}
	$yrange *= 2;
    }

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
    print GNU "set title \"composite readings from $log_dir\"\n";
    print GNU "set xlabel \"Approximate time in seconds\"\n";
    print GNU "set ylabel \"Data rate (MB/s)\"\n";
#print GNU "set xrange [800:1400]\n";
    print GNU "set yrange [0:$yrange]\n" if (defined($yrange));
    print GNU "plot $plot_com_mib $plot_com_lwatch $plot_com_write $plot_com_read\n";
    close(GNU);
    return 1;
}

$mib_plot = read_log;
$lwatch_plot = read_lwatch;
$plot_write = read_writes;
$plot_read = read_reads;
find_bounds;
write_data_file;
$do_plot = write_gnuplot_file;
`gnuplot -persist $gnu_file` if ($do_plot);

unlink $tmp_file if (! $keep_files);
unlink $gnu_file if (! $keep_files);
    
