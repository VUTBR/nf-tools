
use Test::More;

plan tests => 8;

use Net::NfDump qw ':all';
use Data::Dumper;
#open(STDOUT, ">&STDERR");
our %DS;

require "t/ds.pl";

system("libnf/examples/lnf_ex01_writer -r 10 -n 50000 ");

# get result wiyh single thread 
system("./libnf/bin/nfdumpp -r ./test-file.tmp -T 1 -A srcip -O bytes > t/threads-reference.txt");


for (my $i = 2; $i < 10; $i++) {

	system("./libnf/bin/nfdumpp -r ./test-file.tmp -T $i -A srcip -O bytes > t/threads-res-$i.txt");

	system("diff t/threads-reference.txt t/threads-res-$i.txt");

    ok( $? == 0 );
}


