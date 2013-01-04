#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "libnf/libnf.h"

#include "const-c.inc"

/* #include <sys/vfs.h> */

MODULE = Net::NfDump		PACKAGE = Net::NfDump

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


