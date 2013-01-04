
#use Test::More tests => 2;
use Test::More;
use Net::NfDump qw ':all';

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
	'nextas' => '1234569',
	'prevas' => '12345622',
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

# prepare v6 structure - same as V4 but address changed to v6
$DS{'v6_txt'} = { %{$DS{'v4_txt'}} };
$DS{'v6_txt'}->{'srcip'} ='2001:67c:1220:f565::93e5:f0fb';
$DS{'v6_txt'}->{'dstip'} ='2001:abc:1220:f565::93e5:f0fb';
$DS{'v6_txt'}->{'nexthop'} ='2001:67c:1220:f565::1';
$DS{'v6_txt'}->{'bgpnexthop'} ='2001:67c:1220:f565::1';
$DS{'v6_txt'}->{'router'} ='2001:67c:1220:f565::10';

$DS{'v4_raw'} = txt2row( $DS{'v4_txt'} );
$DS{'v6_raw'} = txt2row( $DS{'v6_txt'} );


# testing v4
my ($floww, $flowr);
$floww = new Net::NfDump(OutputFile => "t/v4_rec.tmp" );
$floww->storerow_hashref( $DS{'v4_raw'} );
$floww->finish();


$flowr = new Net::NfDump(InputFiles => [ "t/v4_rec.tmp" ] );
while ( my $row = $flowr->fetchrow_hashref() )  {
	ok( eq_hash( $DS{'v4_raw'}, $row) );
	ok( eq_hash( $DS{'v4_txt'}, row2txt($row)) );
}

# testing v6
$floww = new Net::NfDump(OutputFile => "t/v6_rec.tmp" );
$floww->storerow_hashref( $DS{'v6_raw'} );
$floww->finish();


$flowr = new Net::NfDump(InputFiles => [ "t/v6_rec.tmp" ] );
while ( my $row = $flowr->fetchrow_hashref() )  {
	ok( eq_hash( $DS{'v6_raw'}, $row) );
	ok( eq_hash( $DS{'v6_txt'}, row2txt($row)) );
}

done_testing();

# testing performance 
diag "";
diag "Testing performance, it will take while...";
my $recs = 1000000;

my $rec = $DS{'v4_raw'} ;
my $flow = new Net::NfDump(OutputFile => "t/v6_rec.tmp" );
my $tm1 = time();
for (my $x = 0 ; $x < $recs; $x++) {
	$flow->storerow_hashref( $rec );
}
$flow->finish();

my $tm2 = time() - $tm1;
diag sprintf("PERFORMACE v6, written %d recs in %d secs (%.3f/sec)", $recs, $tm2, $recs/$tm2);


