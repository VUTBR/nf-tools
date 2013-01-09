#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "libnf/libnf.h"

#include "const-c.inc"

/* #include <sys/vfs.h> */

#define MATH_INT64_NATIVE_IF_AVAILABLE 1
#include "perl_math_int64.h"

MODULE = Net::NfDump		PACKAGE = Net::NfDump

BOOT:
	PERL_MATH_INT64_LOAD_OR_CROAK;

INCLUDE: const-xs.inc


SV * 
libnf_file_info(file)
	char * file


int 
libnf_init()


SV * 
libnf_instance_info(handle)
	int handle


int 
libnf_read_files(handle, filter, window_start, window_end, files)
	int handle
	char *filter
	int window_start
	int window_end
	SV * files


SV *
libnf_read_row(handle)
	int handle


int 
libnf_create_file(handle, filename, compressed, anonymized, ident)
	int handle 
	char *filename
	int compressed
	int anonymized
	char *ident


int 
libnf_write_row(handle, hashref)
	int handle
	HV * hashref


void 
libnf_finish(handle)
	int handle


