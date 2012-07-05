#!/usr/bin/perl -w

use strict;
use NFlow;

my %STATS;

$flow = NFlow->new(
			fdir => ['/data/netflow/LIST/2012-07-05/16', '/data/netflow/LIST/2012-07-05/17'], 
			dilter => 'src net 147.229.0.0/16'
			aggregate => "srcip,dstip" 
			);  

while ( my $row = $flow->fetch_next() ) {
	printf "%s -> %s : %d %d %d\n", $row->{srcip}, $row->{dstip}, $row->{bytes}, $row->{pkts}, $row->{flows};
} 

$flow->close();

