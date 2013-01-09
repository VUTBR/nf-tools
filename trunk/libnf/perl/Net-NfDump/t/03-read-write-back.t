
use Test::More tests => 1;
use Net::NfDump qw ':all';

open(STDOUT, ">&STDERR");


# 
my ($floww, $flowr);
$flowr = new Net::NfDump(InputFiles => [ "t/data1" ] );
$floww = new Net::NfDump(OutputFile => "t/data1.tmp" );

use Data::Dumper;

while ( my $raw = $flowr->fetchrow_hashref() ) {
	
	my $plain = row2txt($raw);
	my $raw2 = txt2row($plain);
	$floww->storerow_hashref($raw2);
}

$flowr->finish();
$floww->finish();

system("nfdump/bin/nfdump -q -r t/data1 -o raw | grep -v size > t/data1.txt");
system("nfdump/bin/nfdump -q -r t/data1.tmp -o raw | grep -v size > t/data1.tmp.txt");

system("diff t/data1.txt t/data1.tmp.txt");

ok( $? == 0 );

