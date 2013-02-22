
use ExtUtils::MakeMaker;
$Verbose = 1;
WriteMakefile(
	NAME => 'Net::NfDump::libnf',
    VERSION_FROM => '../lib/Net/NfDump.pm',
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
		'../nfdump/bin/nf_common.c', 
		],	
	CCFLAGS => '-DNSEL=1 -D__SUNPRO_C=1', # see note 
	clean => { 'FILES' => '*$(LIB_EXT) *$(OBJ_EXT) $(O_FILES)' },
);

# NOTE: in nfdump source code function format_number is defined as inline.
# With compiler with defined __SUNPRO_C it also adds extern.
# However some platforms (OSX, FreeBSD 9.x) requires extern inline 
# to make function available in the dinamic library, howevet it 
# produces some warnings on some platforms (Linux) 

sub MY::post_constants {
'
VERSION_MACRO = LIBNF_VERSION
DEFINE_VERSION = -D$(VERSION_MACRO)=\"$(VERSION)\"
';
}


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
		./h2fields.pl < libnf.h > ../lib/Net/NfDump/Fields.pm 
		$(CP) ../lib/Net/NfDump.pm ../lib/Net/NfDump.pm.bkp
		cat ../lib/Net/NfDump.pm.bkp libnf.h.pod | ./mergechapter.pl > ../lib/Net/NfDump.pm
		$(RM_F) ../lib/Net/NfDump.pm.bkp
		$(AR) cr libnf$(LIB_EXT) $(O_FILES) 
		$(RANLIB) libnf$(LIB_EXT)
';
}


