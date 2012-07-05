#!/usr/bin/perl -w

use strict;
use NFlow;

my %STATS;

$flow = NFlow->new(
			fdir => ['/data/netflow/LIST/2012-07-05/16', '/data/netflow/LIST/2012-07-05/17'], 
			filter => 'src net 147.229.0.0/16'
			);  

while ( my $row = $flow->fetch_next() ) {
	my $srcip = $row->{srcip};
	if (!defined($STAS{$srcip}) {
		$STAS{$srcip} = $row->{bytes};
	} else {
		$STAS{$srcip} += $row->{bytes};
	}
} 

foreach (my ($key, $val) = each %STATS ) {
	printf "%s : %d\n", $key, $val;
}

$flow->close();

