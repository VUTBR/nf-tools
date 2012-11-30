#!/usr/bin/perl -w
#

use ExtUtils::testlib;
use Net::NfDump;
use Data::Dumper;

# list of files to read
my $flist = ["t/data/dump1.nfcap", "t/data/dump2.nfcap"];

# instance of source and destination files
my $flow_src = new Net::NfDump(InputFiles => $flist, Filter => "proto icmp");
my $flow_dst = new Net::NfDump(OutputFile => "out", Ident => "myident");

# statistics counters
my $bytes = 0;
my $flows = 0;
my $pkts = 0;

# exec query 
$flow_src->query();

while ($ref = $flow_src->read()) {

#	print Dumper(\$ref);

	# count statistics
	$bytes += $ref->{'bytes'};
	$pkts += $ref->{'pkts'};
	$flows += $ref->{'flows'};

	# wite data to output file
	$flow_dst->write($ref);
}

printf "bytes=$bytes, pkts=$pkts, flows=$flows\n";



