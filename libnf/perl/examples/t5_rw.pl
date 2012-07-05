#!/usr/bin/perl -w

use strict;
use NFlow;

my %USERS = ( '147.229.3.10' => '3303', '147.229.3.15' => '123');

$flow1 = NFlow->new(
			fdir => ['/data/netflow/LIST/2012-07-05/16', '/data/netflow/LIST/2012-07-05/17'], 
			filter => 'src net 147.229.0.0/16'
			);  


my (@items) = $flow->get_items();

push(@items, "srcurid");
push(@items, "dsturid");

$flow2 = Flow->new(
			ffile => [ 'output.flow' ],
			items => [ @items ],
			compressed => 0
			);  

while ( my $row = $flow->fetch_next() ) {
	my $srcip = $row->{srcip};

	if (defined($USERS{$srcip})) {
		$row->{srcuserid} = $USERS{$srcip};
	} else {
		$row->{srcuserid} = 0;
	}

	if (defined($USERS{$sdtip})) {
		$row->{dstuserid} = $USERS{$dstip};
	} else {
		$row->{sdtuserid} = 0;
	}

	$flow2->add_rec($row);
	
} 

$flow1->close();
$flow2->close();

