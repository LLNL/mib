#!/usr/bin/perl -w
# lfind-dist3.pl
# Read the table of task number versus obdix index from standard in.
# Note how many distinct OSSs there are for each group of 64 CNs 
# (collectively behind one ION).  N.B. There is more than one obdix 
# per OSS, and that mapping is available, among other places, from lfind.
# I'll put that info in BGL/gb1.lfind.osts and call for it inline.

my $CNS_PER_ION = 64;
my $NUM_OSSS = 224;
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
my $gb1_lfind_osts = "/home/auselton/BGL/gb1.lfind.osts";
my @OST;
my @HOST;

open (OSTS, "<$gb1_lfind_osts") or die "Could not get OST lfind info";
while(defined($line=<OSTS>))
{
    if($line =~ /(\d+): OST_blc(\d+)_/)
    {
	$OST[$1] = $2;
    }
}
close(OSTS);

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
	    for (my $oss = 0; $oss < $NUM_OSSS; $oss++)
	    {
		$ions[$ion - 1] += $OSS[$oss] if ($ion > 0);
		$OSS[$oss] = 0;
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
	$OSS[$OST[$index]] = 1;
    }
}
for (my $oss = 0; $oss < $NUM_OSSS; $oss++)
{
    $ions[$ion] += $OSS[$oss];
}
$val = $ions[$ion];
$ave += $val;
printf "%d\t%d\n", $ion, $val;
$high = ($high > $val) ? $high : $val;
$low = ($low < $val) ? $low : $val;

my $size = $max - $min + 1;

$ave /= $size if ( $size > 0 );
printf "Average number of the %d OSSs used by the %d IONs is %.2f\n", $NUM_OSSS, $size, $ave;

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
