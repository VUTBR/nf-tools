
use Test::More tests => 2;
use Net::NfDump;

my %rec_v4 = (
            'protocol' => '6',
            'bytes' => '291',
            'input' => '2',
            'dstas' => '0',
            'insrcmac' => '00:1c:2e:92:03:80',
            'dstport' => '10050',
            'outbytes' => '291',
            'flows' => '1',
            'pkts' => '5',
            'msec_last' => '748',
            'outdstmac' => '00:50:56:bf:a2:88',
            'first' => '1355439616',
            'dstip' => '10.255.5.6',
            'srcip' => '147.229.3.135',
            'srcas' => '0',
            'last' => '1355439616',
            'outpkts' => '5',
            'output' => '0',
            'iprouter' => '10.255.5.6',
            'enginetype' => '0',
            'srcport' => '53008',
            'msec_first' => '747',
            'engineid' => '0',
			'tcp_flags' => '27'
          );

my %rec_v6 = (
            'protocol' => '6',
            'bytes' => '144',
            'input' => '2',
            'dstas' => '0',
            'insrcmac' => '00:1c:2e:92:03:80',
            'dstport' => '57435',
            'outbytes' => '144',
            'flows' => '1',
            'pkts' => '2',
            'msec_last' => '173',
            'outdstmac' => '38:22:d6:e5:94:88',
            'first' => '1355439610',
            'dstip' => '2001:67c:1220:c1a2:297f:d8d6:8a71:bac8',
            'srcip' => '2a00:bdc0:3:102:2:0:402:831',
            'srcas' => '0',
            'last' => '1355439610',
            'outpkts' => '2',
            'output' => '0',
            'iprouter' => '10.255.5.6',
            'enginetype' => '0',
            'srcport' => '80',
            'msec_first' => '173',
            'engineid' => '0',
			'tcp_flags' => '16'
          );


# testing v4
my $flow_v4 = new Net::NfDump(OutputFile => "t/record_v4.out" );
$flow_v4->create();
$flow_v4->write(\%rec_v4);
$flow_v4->close();


my $flow_v4i = new Net::NfDump(InputFiles => [ "t/record_v4.out" ] );
$flow_v4i->query();
while ($ref = $flow_v4i->read()) {
	ok( eq_hash(\%rec_v4, $ref) );
}

# testing v6
my $flow_v6 = new Net::NfDump(OutputFile => "t/record_v6.out" );
$flow_v6->create();
$flow_v6->write(\%rec_v6);
$flow_v6->close();

my $flow_v6i = new Net::NfDump(InputFiles => [ "t/record_v6.out" ] );
$flow_v6i->query();
while ($ref = $flow_v6i->read()) {
	ok( eq_hash(\%rec_v6, $ref) );
}


