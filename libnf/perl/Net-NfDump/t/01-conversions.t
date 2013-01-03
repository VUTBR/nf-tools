
use Test::More tests => 5;
use Net::NfDump qw 'ip2txt txt2ip mac2txt txt2mac mpls2txt txt2mpls';

my $ip = '147.229.3.10';
my $ip6 = '2001:67c:1220:f565::93e5:f0fb';
my $mac = '00:50:56:ac:60:4e';
my $mpls1 = '126-6-0 127-6-0 128-6-0 129-6-0 130-6-0 131-6-1';
my $mpls2 = '126-7-0 127-3-0 128-5-0 129-1-0 130-3-0 131-6-0 126-6-0 127-6-0 128-6-0 129-6-1';


my $ip_bin = txt2ip($ip);
ok ( $ip eq ip2txt($ip_bin) );

my $ip6_bin = txt2ip($ip6);
ok ( $ip6 eq ip2txt($ip6_bin) );

my $mac_bin = txt2mac($mac);
ok ( $mac eq mac2txt($mac_bin) );

my $mpls1_bin = txt2mpls($mpls1);
ok ( $mpls1 eq mpls2txt($mpls1_bin) );

my $mpls2_bin = txt2mpls($mpls2);
ok ( $mpls2 eq mpls2txt($mpls2_bin) );

# testing v4
#	ok( eq_hash(\%rec_v4, $ref) );
