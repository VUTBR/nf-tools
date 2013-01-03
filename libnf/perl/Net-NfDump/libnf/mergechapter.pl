#!/usr/bin/perl -w 
#
# Merges chapters from stdin. If any chapter defined by ^=head1 <chapter name>$ is definned multiple times them 
# the chapter is replaced by the last occurance of the chapter. The position of the chapter 
# will betaken from the firs occurance of the chapter. 

use strict; 

my %CHAPTERS_CONTENT;
my @CHAPTERS_ORDER;
my $chapter = "";

push(@CHAPTERS_ORDER, $chapter);

while (<STDIN>) {
	# =head1 NAME
	if (/^=head1 .+$/) {
		$chapter = $_;
		chomp $chapter;
		$CHAPTERS_CONTENT{$chapter} = "";
		push(@CHAPTERS_ORDER, $chapter);
	}

	$CHAPTERS_CONTENT{$chapter} .= $_;
}

foreach (@CHAPTERS_ORDER) {

	if (defined($CHAPTERS_CONTENT{$_})) {
		print $CHAPTERS_CONTENT{$_};
		delete($CHAPTERS_CONTENT{$_});
	}

}

