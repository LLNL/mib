#!/usr/bin/perl -w
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

 
