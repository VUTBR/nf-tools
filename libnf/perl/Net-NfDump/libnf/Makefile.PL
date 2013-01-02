
use ExtUtils::MakeMaker;
$Verbose = 1;
WriteMakefile(
	NAME => 'Net::NfDump::libnf',
#	SKIP => [qw(all static static_lib dynamic dynamic_lib)],
	INC  => '-I ../nfdump -I ../nfdump/bin',
	clean => {'FILES' => 'libnf$(LIB_EXT)'},
);
sub MY::top_targets {
'
all :: static
pure_all :: static
static :: libnf$(LIB_EXT)
libnf$(LIB_EXT): $(O_FILES) 
		./check_items_map.pl 
		./h2pod.pl < libnf.h > libnf.h.pod
		cp ../lib/Net/NfDump.pm ../lib/Net/NfDump.pm.bkp
		cat ../lib/Net/NfDump.pm.bkp libnf.h.pod | ./mergechapter.pl > ../lib/Net/NfDump.pm
		$(AR) cr libnf$(LIB_EXT) $(O_FILES) ../nfdump/bin/*$(OBJ_EXT)
		$(RANLIB) libnf$(LIB_EXT)
';
}


