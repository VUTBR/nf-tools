#!/usr/bin/perl -w
#

use ExtUtils::testlib;
use Net::NfDump;
use Data::Dumper;

# list of files to read
#my $flist = ["t/data/dump1.nfcap", "t/data/dump2.nfcap"];
my $flist = [ qx "find -L /data/netflow/2012-11-26/01  -type f " ];

chomp(@{$flist});



print Dumper(\$flist);

# instance of source and destination files
#my $flow_src = new Net::NfDump(InputFiles => $flist, Filter => "proto icmp");
my $flow_src = new Net::NfDump(InputFiles => $flist, Filter => "any");
my $flow_dst = new Net::NfDump(OutputFile => "/data/netflow/nfdump.out", Ident => "myident");


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

$flow_src->close();
$flow_dst->close();

my $ret = $flow_src->info("/data/netflow/nfdump.out");
print Dumper(\$ret);


