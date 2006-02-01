#!/usr/bin/perl -w
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


	
