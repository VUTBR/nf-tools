
use Test::More tests => 1;;
open(STDOUT, ">&STDERR");

use Net::NfDump qw ':all';

my %data =( 

	'version' => '1',
	'ident' => 'none',
	'blocks' => '1',
	'catalog' => '0',
	'anonymized' => '0',
	'compressed' => '1',
	'sequence_failures' => '0',

	'first' => '1354046356360',
	'last' => '1354046668173',

	'flows' => '8170',
	'flows_tcp' => '14',
	'flows_udp' => '8092',
	'flows_icmp' => '63',
	'flows_other' => '1',

	'bytes' => '2641163',
	'bytes_tcp' => '10611',
	'bytes_udp' => '2613720',
	'bytes_icmp' => '16792',
	'bytes_other' => '40',

	'packets' => '8534',
	'packets_tcp' => '73',
	'packets_udp' => '8346',
	'packets_icmp' => '114',
	'packets_other' => '1',
	);

# we will use the output file from the previous test 

my $info = file_info("t/data2.tmp");

ok( eq_hash($info, \%data) );


