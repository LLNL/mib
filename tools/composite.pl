#!/usr/bin/perl -w 
#  composite.pl [-d <log_dir>] [-h] [-k] [-p <print>] [-s <scale>] 
# Produce a graph combining:
# o rates observed by mib
# o lwatch-lustre trace
# o datarates calulated from syscall profiles
# Allow for manual adjustment of the coordination between lwatch and syscall
# timing data.

use Getopt::Std;
use Time::Local;

use constant GETOPTS_ARGS => "d:hkp:s:";
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
my $wval;
my $rval;
my $call;
my $png_file;
my $log_dir = "";
my $tmp_file = "/tmp/composite.tmp.$$";
my $gnu_file = "/tmp/composite.$$.gnuplot";
my $keep_files = 0;
my $r_off = 0;
my $w_off = 0;
my $scale = 5;
my $plot_mib;
my $plot_lwatch;
my $plot_write;
my $plot_read;

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
if ( $opt_p and ! ($png_file =~ /\.png$/) )
{
    $png_file = $png_file . ".png";
    print"Graph output to $png_file\n";
}
$r_off = $opt_r if ($opt_r);
$w_off = $opt_w if ($opt_w);
$scale = $opt_s if ($opt_s);

# If we don't get an options file set the default here.
my $CNs_per_ION = 64;
sub read_options
{
# Given the value of $log_dir read in the needed values:
    my $options = "options";
    my $CNs;
    my $IONs;
    
    $options = $log_dir . $options;
    if ( ! -f $options )
    {
	print "I did not see the options file $options\n";
	return;
    }
    $CNs = `grep cns $options | awk '{print \$3}'`;
    $IONs = `grep ions $options | grep -v iterations | awk '{print \$3}'`;
    $CNs_per_ION = $CNs/$IONs;
    defined($CNs_per_ION) or die "Didn't find options CNs_per_ION value";
    return;
}

# If we don't get a log file set the defaults here.
my $mib_write = 0;
my $mib_read = 0;
my $mib_xfer = 512*1024;
sub read_log 
{
# Given $log_dir read the file and get the given values    
    my $call_size_string;
    my $mib_log = "log";

    $mib_log = $log_dir . $mib_log;
    if ( ! -f $mib_log )
    {
	print "I did not see the mib log file $mib_log\n";
	return 0;
    }

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
    return 1;
}
# End of read_log

my $count = 0;
# It's not clear that this really needs the large scope.
# How is it being used?
my $write_run = 0;
my $read_run = 0;
# These two do appear later, but not the corrsponding read, (resp. write) value
my $lwatch_start_write = 0;
my $lwatch_end_read;
my @wvals;
my @rvals;
sub read_lwatch
{
    my $lwatch_trace = "lwatch";

    $lwatch_trace = $log_dir . $lwatch_trace;
    if ( ! -f $lwatch_trace)
    {
	print "I did not see the lwatch-lustre trace file $lwatch_trace\n";
	return 0;
    }

    open(LWATCH, "<$lwatch_trace") or die "I could not open the lwatch trace file $lwatch_trace";
    
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
# Note that the wrtite value is used later (outside this scope), but
# the read value is not.
    $lwatch_start_write = ($max_write_run > $write_run) ? $start_write : 0;
    my $lwatch_start_read = ($max_read_run > $read_run) ? $start_read : 0;
    my $lwatch_max = $max;

    my $end = $lwatch_start_read;
    while ( (defined($rvals[$end + 1]) and defined($rvals[$end + 2])) and 
	    ! ( ($rvals[$end + 1] == 0) and ($rvals[$end + 2] == 0) ) )
    {
	$end++;
    }
    $lwatch_end_read = $end;
    printf "lwatch: count = %d, write start = %.0f, write_run = %f, end read = %.0f, read_run = %f\n", $count, $lwatch_start_write*$scale, $write_run, $lwatch_end_read*$scale, $read_run if ($verbose);
    return 1;
}
# $count, $lwatch_start_write, $write_run, lwatch_end_read, $read_run
# End of read_lwatch


my @Write;
my @LastW;
my $write_max;
my $write_min;
sub read_writes
{
    my $write_calls = "write.syscall.aves";

    $write_calls = $log_dir . $write_calls;
    if ( ! -f $write_calls ) 
    {
	print "I did not see the write system calls file $write_calls\n";
	return 0;
    }
    
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
    printf("CNs_per_ION = %d\n", $CNs_per_ION);
    printf("Aggregate number of write system calls = %d\n", $sum);
    printf("Start time = %d\n", $start);
    printf("End time = %d\n", $end);
    printf("Write rate: %f MB/s\n", ($sum*$mib_xfer*$CNs_per_ION)/(1024*1024*($end - $start)));
    
    $write_min = $start;
    $write_max = $end;
    printf "write syscalls from %d to %d\n", $write_min, $write_max if ($verbose);
    return 1;
}
# End of read_writes


my @Read;
my @LastR;
my $read_min;
my $read_max;
sub read_reads
{    
    my $read_calls = "read.syscall.aves";
    $read_calls = $log_dir . $read_calls;
    if ( ! -f $read_calls)
    {
	print "I did not see the read system calls file  $read_calls\n";
	return 0;
    }
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

    $read_min = $start;
    $read_max = $end;
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
    return 1;
}
# End of read_reads

my $syscall_start_write;
my $syscall_end_read;
my $xw_min = 0;
my $xw_max = 0;
sub find_bounds
{
    $syscall_start_write = ($max_write_run > $write_run) ? $start_write : 0;
    my $syscall_start_read = ($max_read_run > $read_run) ? $start_read : 0;
    
    $end = $syscall_start_read;
    while ( (defined($Read[$end + 1]) and defined($Read[$end + 2])) and 
	    ! ( ($Read[$end + 1] == 0) and ($Read[$end + 2] == 0) ) )
    {
	$end++;
    }
    $syscall_end_read = $end;
    printf "syscall: write start = %d, read start = %d, read end = %d\n", $syscall_start_write, $syscall_start_read, $syscall_end_read if ($verbose);
    
    if(defined($lwatch_end_read))
    {
	my $syscall_xw_read_end = int(($lwatch_end_read - $lwatch_start_write)*$scale + $syscall_start_write);
	$max = ($syscall_end_read > $syscall_xw_read_end) ? $syscall_end_read : $syscall_xw_read_end;
    }
    else
    {
	$max = $syscall_end_read;
    }
    $xw_min = int($lwatch_start_write - ($syscall_start_write/$scale));
    $xw_max = int(($max - $syscall_start_write)/$scale + $lwatch_start_write);
}
# End of find_bounds

my $mib_index;
my $lwatch_index;
my $write_index;
my $read_index;
sub write_data_file
{
    my $index = 0;
    my $write;
    my $read;

    $xw_min = 0 if ($xw_min < 0);
    open(TMP, ">$tmp_file") or die "Could not open tmp file $tmp_file";
    for ($count = $xw_min; $count < $xw_max; $count++)
    {
	$wvals[$count] = 0 if (! defined($wvals[$count]));
	$rvals[$count] = 0 if (! defined($rvals[$count]));
	my $xt = ($count - $lwatch_start_write)*$scale + $syscall_start_write;
	printf TMP "%.0f\t%f\t%f\n", $xt, $wvals[$count], $rvals[$count];
    }
    $lwatch_index = $index++;
    printf TMP "\n\n";
    
    for ($t = 0; $t < $syscall_end_read; $t++)
    {
	$Write[$t] = 0 if (!defined($Write[$t]));
	$write = $Write[$t]*($CNs_per_ION*$mib_xfer)/(1024*1024);
	printf TMP "%d\t%f\n", $t, $write;
    }
    $write_index = $index++;
    printf TMP "\n\n";
    
    for ($t = 0; $t < $syscall_end_read; $t++)
    {
	$Read[$t] = 0 if (!defined($Read[$t]));
	$read = $Read[$t]*($CNs_per_ION*$mib_xfer)/(1024*1024);
	printf TMP "%d\t%f\n", $t, $read;
    }
    $read_index = $index++;
    printf TMP "\n\n";
    
    for ($t = 0; $t < $syscall_end_read; $t++)
    {
	$read  = 0;
	$write = 0;
	$read = $mib_read if ( ($t > $read_min) && ($t < $read_max) );
	$write = $mib_write if ( ($t > $write_min) && ($t < $write_max) );
	printf TMP "%d\t%f\t%f\n", $t, $write, $read;
    }
    $mib_index = $index;
    close(TMP);
}

sub write_gnuplot_file
{
    my $plot_com_mib = "";
    my $plot_com_lwatch = "";
    my $plot_com_write = "";
    my $plot_com_read = "";
    my $need_comma = 0;

    if ( $plot_mib )
    {
	$plot_com_mib = "'$tmp_file' index $mib_index using 1:2 title \"mib write\" with lines,'$tmp_file' index $mib_index using 1:3 title \"mib read\" with lines";
	$need_comma = 1;
    }
    if ( $plot_lwatch )
    {
	$plot_com_lwatch = "'$tmp_file' index $lwatch_index using 1:2 title \"lwatch write\" with lines,'$tmp_file' index 0 using 1:3 title \"lwatch read\" with lines";
	$plot_com_lwatch = ",$plot_com_lwatch" if ($need_comma);
	$need_comma = 1;
    }
    if ( $plot_write )
    {
	$plot_com_write = "'$tmp_file' index $write_index using 1:2 title \"syscall write\" with lines";
	$plot_com_write = ",$plot_com_write" if ($need_comma);
	$need_comma = 1;
    }
    if ( $plot_read )
    {
	$plot_com_read = "'$tmp_file' index $read_index using 1:2 title \"syscall read\" with lines";
	$plot_com_read = ",$plot_com_read" if ($need_comma);
    }

# Sometimes the early spike in write rate make the rest of the graph 
# disappear.  Try limiting that.
    my $yrange = 1;
    my $tmp = $mib_write;

    while ($tmp > 1)
    {
	$tmp /= 2;
	$yrange *= 2;
    }
    $yrange *= 2;
    
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
    print GNU "set yrange [0:$yrange]\n";
    print GNU "plot $plot_com_mib $plot_com_lwatch $plot_com_write $plot_com_read\n";
    close(GNU);
}

read_options;
$plot_mib = read_log;
$plot_lwatch = read_lwatch;
$plot_write = read_writes;
$plot_read = read_reads;
find_bounds;
write_data_file;
write_gnuplot_file;
`gnuplot -persist $gnu_file`;

unlink $tmp_file if (! $keep_files);
unlink $gnu_file if (! $keep_files);
    
