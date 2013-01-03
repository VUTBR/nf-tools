
use Test::More tests => 2;
use Net::NfDump;

my %DS;		# data sets

$DS{'v4_txt'} = {
	'first' => '1355439616',
	'msecfirst' => '747',
	'last' => '1355439616',
	'mseclast' => '748',
	'received' => '1355439617',
	
	'bytes' => '291',
	'pkts' => '5',
	'outbytes' => '291',
	'outpkts' => '5',
	'flows' => '1',

	'srcport' => '53008',
	'dstport' => '10050',
	'tcpflags' => '27',

	'srcip' => '147.229.3.135',
	'dstip' => '10.255.5.6',
	'nexthop' => '10.255.5.1',
	'srcmask' => '24',
	'dstmask' => '32',
	'tos' => '7',
	'dsttos' => '8',

	'dstas' => '635789',
	'srcas' => '1234568',
	'next' => '1234569',
	'prev' => '1234569',
	'bgpnexthop' => '10.255.5.1',

	'proto' => '6',

	'insrcmac' => '00:1c:2e:92:03:80',
	'outdstmac' => '00:50:56:bf:a2:88',
	'indstmac' => '00:1c:2e:92:04:80',
	'outsrcmac' => '00:50:56:bf:a3:88',
	'srcvlan' => '10',
	'dstvlan' => '20',

	'mpls' => '336-6-0 123-6-0 3337-6-1',

	'inif' => '2',
	'outif' => '1',
	'dir' => '0',
	'fwd' => '1',

	'router' => '10.255.5.6',
   	'sysid' => '0',
   	'systype' => '0',

};

#$DS{'v4_raw'} = 


# testing v4
my $flow_v4 = new Net::NfDump(InputFiles => [ "t/record_v4" ] );
$flow_v4->query();

while ($ref = $flow_v4->read()) {
	ok( eq_hash(\%rec_v4, $ref) );
}

# testing v6
my $flow_v6 = new Net::NfDump(InputFiles => [ "t/record_v6" ] );
$flow_v6->query();
while ($ref = $flow_v6->read()) {
	use Data::Dumper;

	diag Dumper(\$ref);
	ok( eq_hash(\%rec_v6, $ref) );
}


