
use Test::More tests => 4;
use Net::NfDump qw ':all';
use Data::Dumper;

open(STDOUT, ">&STDERR");

require "t/ds.pl";

# preapre dataset 
$floww = new Net::NfDump(OutputFile => "t/agg_dataset.tmp" );
my %row = %{$DS{'v4_txt'}};
for (my $i = 0; $i < 10000; $i++) {
	$floww->storerow_hashref( txt2flow(\%row) );
}
$floww->finish();


$flowr = new Net::NfDump(InputFiles => [ "t/agg_dataset.tmp" ], Aggreg => "srcip,dstport,bytes");
$flowr->query();
while ( my $row = $flowr->fetchrow_hashref() )  {
#	diag Dumper($DS{'v4_nel_nsel_txt'});
	diag Dumper(flow2txt($row));
}



