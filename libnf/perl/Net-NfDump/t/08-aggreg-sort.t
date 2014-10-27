
use Test::More tests => 2;
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
my $numrows = 0;
while ( my $row = $flowr->fetchrow_hashref() )  {
	$row = flow2txt($row);
	$numrows++ if ($row->{'srcip'} eq '147.229.3.135' && $row->{'bytes'} eq '2910000');
#	diag Dumper($row);
}

ok($numrows == 1);


$flowr = new Net::NfDump(InputFiles => [ "t/agg_dataset.tmp" ], Aggreg => "srcip/24/0,bytes");
$flowr->query();
$numrows = 0;
while ( my $row = $flowr->fetchrow_hashref() )  {
	$row = flow2txt($row);
	$numrows++ if ($row->{'srcip'} eq '147.229.3.0' && $row->{'bytes'} eq '2910000');
	diag Dumper($row);
}

ok($numrows == 1);

