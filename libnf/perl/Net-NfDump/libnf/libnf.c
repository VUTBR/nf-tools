
#define NEED_PACKRECORD 1 

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

//#include "bit_array.h"

#define MATH_INT64_NATIVE_IF_AVAILABLE 1
#include "../perl_math_int64.h"

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <netinet/in.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "nffile.h"
#include "nfx.h"
#include "nfnet.h"
#include "bookkeeper.h"
#include "nfxstat.h"
#include "nf_common.h"
#include "rbtree.h"
#include "nftree.h"
#include "nfprof.h"
#include "nfdump.h"
#include "nflowcache.h"
#include "nfstat.h"
#include "nfexport.h"
#include "ipconv.h"
#include "util.h"
#include "flist.h"

#include "libnf.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>


/* open existing nfdump file and prepare for reading records */
/* only simple wrapper to nfdump function */
lnf_nffile_t * lnf_open_file(char * filename, unsigned int flags, char * ident) {

	lnf_nffile_t *lnf_nffile;

	lnf_nffile = malloc(sizeof(lnf_nffile_t));
	
	if (lnf_nffile == NULL) {
		return NULL;
	}

	lnf_nffile->flags = flags;
	/* open file in either read only or write only mode */
	if (flags & LNF_WRITE) {


		lnf_nffile->nffile = OpenNewFile(filename, NULL, flags & LNF_COMP, 
								flags & LNF_ANON, ident);

	} else {

		lnf_nffile->nffile = OpenFile(filename, NULL);

	}

	if (lnf_nffile->nffile == NULL) {
		return NULL;
	}


	return lnf_nffile;
}

/* close file handler and release related structures */
void lnf_close_file(lnf_nffile_t *lnf_nffile) {

	if (lnf_nffile == NULL || lnf_nffile->nffile == NULL) {
		return ;
	}

	if (lnf_nffile->flags & LNF_WRITE) {

		// write the last records in buffer
		if (lnf_nffile->nffile->block_header->NumRecords ) {
			if ( WriteBlock(lnf_nffile->nffile) <= 0 ) {
				fprintf(stderr, "Failed to write output buffer: '%s'" , strerror(errno));
			}
		}
		CloseUpdateFile(lnf_nffile->nffile, NULL );

	} else {
		CloseFile(lnf_nffile->nffile);
	}

	DisposeFile(lnf_nffile->nffile);

	free(lnf_nffile);
}


