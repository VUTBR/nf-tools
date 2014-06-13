#!/usr/bin/perl -w 
#
# Converts stdin and try to find macro pod:. The text after 
# that macro ies expected to be pod document. 
#
# Symbol ## after the pod macro will be replaced with the value 
# of the macro define by #define. 

use strict; 

my $macro = undef;

while (<STDIN>) {
	# #define NFL_SRCPORT     "srcport"   // pod: Source port
	my ($p1, $p2) = split(/pod\:/);

	# check whether $p1 contains macro definition 
	if ($p1 =~ /#define\s+\w+\s+(.+?)\s+/) {
		$macro = $1;
	}

	if ( defined($macro) && defined($p2) ) {
		$macro =~ s/\"(.+)\"/$1/;
		$macro =~ s/(.+);/$1/;
		$p2 =~ s/##/$macro/g;
	}

	printf $p2 if defined ($p2);

}

