#use 5.010001;
use 5.000001;
use ExtUtils::MakeMaker;
use POSIX;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    NAME              => 'Net::NfDump',
    VERSION_FROM      => 'lib/Net/NfDump.pm', # finds $VERSION
	# test whether the platform supports 32bit interger or not 
	(ULONG_MAX == 0xFFFFFFFF ?
	    (PREREQ_PM         => { Socket6 => 0.15, strict => 0, Math::Int64 => 0.09 }) :
	    (PREREQ_PM         => { Socket6 => 0.15, strict => 0 }) 
	), 
    LICENSE           => 'perl',
    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM  => 'lib/Net/NfDump.pm', # retrieve abstract from module
       AUTHOR         => 'Tomas Podermanski <tpoder@cis.vutbr.cz>') : ()
	),
    LIBS              => ['-lresolv'], # e.g., '-lm'
    DEFINE            => '', # e.g., '-DHAVE_SOMETHING'
    INC               => '-I.', # e.g., '-I. -I/usr/include/other'
	# Un-comment this if you add C files to link with later:
    OBJECT            => '$(O_FILES)', # link all the C files too
    MYEXTLIB          => 'libnf/libnf$(LIB_EXT)',
    NORECURS          => 0,
    clean             => {FILES => "nfdump/ nfdump t/*.tmp" },
    realclean         => {FILES => "nfdump-*.tar.gz"},
);
if  (eval {require ExtUtils::Constant; 1}) {
  # If you edit these definitions to change the constants used by this module,
  # you will need to use the generated const-c.inc and const-xs.inc
  # files to replace their "fallback" counterparts before distributing your
  # changes.
  my @names = (qw());
  ExtUtils::Constant::WriteConstants(
                                     NAME         => 'Net::NfDump',
                                     NAMES        => \@names,
                                     DEFAULT_TYPE => 'IV',
                                     C_FILE       => 'const-c.inc',
                                     XS_FILE      => 'const-xs.inc',
                                  );

}
else {
  use File::Copy;
  use File::Spec;
  foreach my $file ('const-c.inc', 'const-xs.inc') {
    my $fallback = File::Spec->catfile('fallback', $file);
    copy ($fallback, $file) or die "Can't copy $fallback to $file: $!";
  }
}


sub MY::post_constants {
'
VERSION_MACRO = LIBNF_VERSION
DEFINE_VERSION = -D$(VERSION_MACRO)=\"$(VERSION)\"
';
}

sub MY::postamble {
'
NFDUMP		= nfdump-1.6.10p1
NFDUMP_MD5	= a3da6716b47ac7a0cae6ccd07c2e3a6d
NFDUMP_SRC	= $(NFDUMP).tar.gz
NFDUMP_URL	= http://downloads.sourceforge.net/project/nfdump/stable/$(NFDUMP)/$(NFDUMP_SRC)

$(NFDUMP).tar.gz:
		@echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		@echo "!!                                                     !!"
		@echo "!!  If automatic download fails please get nfdump      !!"
		@echo "!!  sources manually                                   !!"
		@echo "!!                                                     !!"
		@echo "!!  VERSION: $(NFDUMP)                            !!"
		@echo "!!  URL:     http://sourceforge.net/projects/nfdump/   !!"
		@echo "!!  FILE:    $(NFDUMP).tar.gz                     !!"
		@echo "!!                                                     !!"
		@echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		wget -nv $(NFDUMP_URL) || curl -L -o $(NFDUMP_SRC) $(NFDUMP_URL)


nfdump: $(NFDUMP).tar.gz 
		./md5sum.sh $(NFDUMP_MD5) $(NFDUMP).tar.gz 
		tar xzf $(NFDUMP).tar.gz
		$(MV) -f $(NFDUMP) nfdump 
		# apply patches in beta code 
		cd nfdump && patch -p1 < ../nfdump-bugs.patch && cd ..
		# nfdump gzip contains object files, so we rid them off
		$(RM_F) nfdump/bin/nfdump
		$(RM_F) nfdump/bin/*.o
		cd nfdump && export CFLAGS=-fPIC && ./configure --enable-nsel --enable-nel && cd bin && make nfdump
		$(RM_F) nfdump/bin/*.o

$(MYEXTLIB): nfdump libnf/Makefile
		cd libnf && $(MAKE) $(PASSTHRU)
		#cd libnf && $(MAKE) 
';
}

