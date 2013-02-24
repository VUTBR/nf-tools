
use Test::More tests => 16;
use Net::NfDump qw ':all';
use Data::Dumper;

open(STDOUT, ">&STDERR");

require "t/ds.pl";

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
$flowr->finish();

# testing fetchrow_array
$floww = new Net::NfDump(OutputFile => "t/v6_rec2.tmp", Fields => ['srcip', 'dstip'] );
$floww->storerow_array($DS{'v6_raw'}->{'srcip'}, $DS{'v6_raw'}->{'dstip'});
$floww->storerow_array($DS{'v6_raw'}->{'srcip'}, $DS{'v6_raw'}->{'dstip'});
$floww->finish();

$flowr = new Net::NfDump(InputFiles => [ "t/v6_rec2.tmp" ], Fields => ['srcip', 'dstip'] );
while ( my ($srcip, $dstip) = $flowr->fetchrow_array() ) {
	ok( ip2txt($srcip), $DS{'v6_txt'}->{'srcip'} );
	ok( ip2txt($dstip), $DS{'v6_txt'}->{'dstip'} );
}
$flowr->finish();


# testing fields as a string 
$flowr = new Net::NfDump(InputFiles => "t/v6_rec2.tmp" , Fields => 'srcip, dstip' );
while ( my ($srcip, $dstip) = $flowr->fetchrow_array() ) {
	ok( ip2txt($srcip), $DS{'v6_txt'}->{'srcip'} );
	ok( ip2txt($dstip), $DS{'v6_txt'}->{'dstip'} );
}
$flowr->finish();

