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
	my $dstip = $row->{dstcip};

	$row->{srcip} = $dstip;
	$row->{dstip} = $srcip;

	$row->store();
	
} 


$flow->close();

