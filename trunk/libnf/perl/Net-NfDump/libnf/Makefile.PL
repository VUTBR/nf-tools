
use ExtUtils::MakeMaker;
$Verbose = 1;
WriteMakefile(
	NAME => 'Net::NfDump::libnf',
#	SKIP => [qw(all static static_lib dynamic dynamic_lib)],
	INC  => '-I ../nfdump -I ../nfdump/bin',
	C => [ 
		'libnf.c',
		'../nfdump/bin/nffile.c', 
		'../nfdump/bin/grammar.c', 
		'../nfdump/bin/nfx.c', 
		'../nfdump/bin/minilzo.c', 
		'../nfdump/bin/nftree.c', 
		'../nfdump/bin/scanner.c', 
		'../nfdump/bin/util.c', 
		'../nfdump/bin/ipconv.c', 
		'../nfdump/bin/flist.c', 
		],	
	CCFLAGS => '-DDEFINE_LIBNF=1',
	clean => { 'FILES' => '*$(LIB_EXT) *$(OBJ_EXT) $(O_FILES)' },
);
#libnf$(LIB_EXT): nfdump $(O_FILES) libnf.h libnf.c bit_array.h 
sub MY::top_targets {
'
NFOBJS = nffile grammar nfx minilzo nftree scanner util ipconv first

all :: static
pure_all :: static
static :: libnf$(LIB_EXT)

libnf$(LIB_EXT): $(O_FILES) libnf.c libnf.h bit_array.h 
		$(CP) *.o ../nfdump/bin/
		./check_items_map.pl 
		./h2pod.pl < libnf.h > libnf.h.pod
		$(CP) ../lib/Net/NfDump.pm ../lib/Net/NfDump.pm.bkp
		cat ../lib/Net/NfDump.pm.bkp libnf.h.pod | ./mergechapter.pl > ../lib/Net/NfDump.pm
		$(AR) cr libnf$(LIB_EXT) $(O_FILES) *$(OBJ_EXT) ../nfdump/bin/*$(OBJ_EXT)
		$(RANLIB) libnf$(LIB_EXT)
';
}


