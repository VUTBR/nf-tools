
use Test::More tests => 8;
use Net::NfDump qw ':all';
use Data::Dumper;

#open(STDOUT, ">&STDERR");

my %DS;		# data sets

$DS{'v4_basic_txt'} = {
	'first' => '1355439616',
	'msecfirst' => '747',
	'last' => '1355439616',
	'mseclast' => '748',
	
	'bytes' => '291',
	'pkts' => '5',

	'srcport' => '53008',
	'dstport' => '10050',
	'tcpflags' => '27',

	'srcip' => '147.229.3.135',
	'dstip' => '10.255.5.6',
	'nexthop' => '10.255.5.1',

	'proto' => '6',
};

$DS{'v4_txt'} = {
	'first' => '1355439616',
	'msecfirst' => '747',
	'last' => '1355439616',
	'mseclast' => '748',
	'received' => '22341355439617',
	
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

#	'clientdelay' => '100', 
#	'serverdelay' => '200',
#	'appllatency' => '300'

};

# prepare v6 structure - same as V4 but address changed to v6
$DS{'v6_txt'} = { %{$DS{'v4_txt'}} };
$DS{'v6_txt'}->{'srcip'} ='2001:67c:1220:f565::93e5:f0fb';
$DS{'v6_txt'}->{'dstip'} ='2001:abc:1220:f565::93e5:f0fb';
$DS{'v6_txt'}->{'nexthop'} ='2001:67c:1220:f565::1';
$DS{'v6_txt'}->{'bgpnexthop'} ='2001:67c:1220:f565::1';
$DS{'v6_txt'}->{'router'} ='2001:67c:1220:f565::10';

$DS{'v4_raw'} = txt2flow( $DS{'v4_txt'} );
$DS{'v4_basic_raw'} = txt2flow( $DS{'v4_basic_txt'} );
$DS{'v6_raw'} = txt2flow( $DS{'v6_txt'} );


# testing v4
my ($floww, $flowr);
$floww = new Net::NfDump(OutputFile => "t/v4_rec.tmp" );
$floww->storerow_hashref( $DS{'v4_raw'} );
$floww->storerow_hashref( $DS{'v4_raw'} );
$floww->finish();

$flowr = new Net::NfDump(InputFiles => [ "t/v4_rec.tmp" ] );
while ( my $row = $flowr->fetchrow_hashref() )  {
#	diag Dumper(row2txt($row));
#	diag Dumper($DS{'v4_txt'});
	ok( eq_hash( $DS{'v4_raw'}, $row) );
	ok( eq_hash( $DS{'v4_txt'}, flow2txt($row)) );
}

# testing v6
$floww = new Net::NfDump(OutputFile => "t/v6_rec.tmp" );
$floww->storerow_hashref( $DS{'v6_raw'} );
$floww->storerow_hashref( $DS{'v6_raw'} );
$floww->finish();


$flowr = new Net::NfDump(InputFiles => [ "t/v6_rec.tmp" ] );
while ( my $row = $flowr->fetchrow_hashref() )  {
	ok( eq_hash( $DS{'v6_raw'}, $row) );
	ok( eq_hash( $DS{'v6_txt'}, flow2txt($row)) );
}


# testing performance 
diag "";
diag "Testing performance, it will take while...";
my $recs = 1000000;

my %tests = ( 'v4_basic_raw' => 'basic items', 'v4_raw' => 'all items' );

while (my ($key, $val) = each %tests ) {
	my $rec = $DS{$key} ;
	my $flow = new Net::NfDump(OutputFile => "t/flow_$key.tmp" );
	my $tm1 = time();
	for (my $x = 0 ; $x < $recs; $x++) {
		$flow->storerow_hashref( $rec );
	}
	$flow->finish();

	my $tm2 = time() - $tm1;
	diag sprintf("Write performance %s, written %d recs in %d secs (%.3f/sec)", $val, $recs, $tm2, $recs/$tm2);
}


while (my ($key, $val) = each %tests ) {
	my $flow = new Net::NfDump(InputFiles => [ "t/flow_$key.tmp" ] );
	my $tm1 = time();
	my $cnt = 0;
	$flow->query();
	while ( $row = $flow->fetchrow_hashref() )  {
		$cnt++ if ($row);
	}
	$flow->finish();

	my $tm2 = time() - $tm1;
	diag sprintf("Read performance %s, read %d recs in %d secs (%.3f/sec)", $val, $cnt, $tm2, $recs/$tm2);
}

