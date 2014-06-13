
use ExtUtils::MakeMaker;
$Verbose = 1;
WriteMakefile(
	NAME => 'Net::NfDump::libnf',
    VERSION_FROM => '../lib/Net/NfDump.pm',
#	SKIP => [qw(all static static_lib dynamic dynamic_lib)],
	INC  => '-I ../libnf/include',
	C => [ 
		'libnf_perl.c',
		],	
#	CCFLAGS => '-DNSEL=1 -D__SUNPRO_C=1 ', # see note 
#	CCFLAGS => '-D__SUNPRO_C=1', # see note 
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
#NFOBJS = nffile grammar nfx minilzo nftree scanner util ipconv first

all :: static
pure_all :: static
static :: libnf_perl$(LIB_EXT)

#libnf$(LIB_EXT): $(O_FILES) libnf.c libnf.h bit_array.h bit_array.c libnf_perl.h libnf_perl.c
libnf_perl$(LIB_EXT): $(O_FILES) libnf_perl.c 
		./h2pod.pl < libnf_perl.h > libnf_perl.h.pod
		$(CP) ../lib/Net/NfDump.pm ../lib/Net/NfDump.pm.bkp
		cat ../lib/Net/NfDump.pm.bkp libnf_perl.h.pod | ./mergechapter.pl > ../lib/Net/NfDump.pm
		$(RM_F) ../lib/Net/NfDump.pm.bkp
		echo "XXX"
		$(AR) cr libnf_perl$(LIB_EXT) $(O_FILES)
#		$(RANLIB) libnf_perl$(LIB_EXT)
		echo "XXX2"

libnf_perl$(LIB_EXT): 
		../libnf/src/libnf-info | ./info2fields.pl > ../lib/Net/NfDump/Fields.pm 
		$(AR) x ../libnf/src/.libs/libnf.a
		$(AR) cr libnf_perl$(LIB_EXT) *.o
		$(RANLIB) libnf_perl$(LIB_EXT)
';
}


