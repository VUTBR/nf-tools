#!/usr/bin/perl -w

use strict;
use NFlow;

my %STATS;

$flow = NFlow->new(
			ffile => ['nfcapd.201207051600', 'nfcapd.201207051600'], 
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

$flow->clode();

