#!/usr/bin/perl -w
#

use ExtUtils::testlib;
use Net::NfDump;
use Data::Dumper;
use Socket qw( AF_INET );
use Socket6 qw( inet_ntop inet_pton AF_INET6 );


# instance of source and destination files
my $flow = new Net::NfDump(InputFiles => [ "t/record_v4" ], RawData => 1 );
#my $flow = new Net::NfDump(InputFiles => [ "../t/data1" ], Filter => "ipv4" );

# exec query 
$flow->query();

while ($ref = $flow->read()) {

	print Dumper(\$ref);

	my $addr = inet_pton(AF_INET, "10.255.5.6");

	printf "a0: %s, %d\n", unpack("h*", $ref->{'dstip'}), length($ref->{'dstip'});
	printf "a1: %s, %d\n", unpack("h*", $addr), length($addr);
	printf "A1: %s\n", unpack("H*", $addr);

	last;

}

$flow->close();



