
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

#include "nfdump_inline.c"
#include "nffile_inline.c"


/* open existing nfdump file and prepare for reading records */
/* only simple wrapper to nfdump function */
lnf_file_t * lnf_open(char * filename, unsigned int flags, char * ident) {

	lnf_file_t *lnf_file;

	lnf_file = malloc(sizeof(lnf_file_t));
	
	if (lnf_file == NULL) {
		return NULL;
	}

	lnf_file->flags = flags;
	/* open file in either read only or write only mode */
	if (flags & LNF_WRITE) {

		lnf_file->nffile = OpenNewFile(filename, NULL, flags & LNF_COMP, 
								flags & LNF_ANON, ident);

	} else {

		lnf_file->nffile = OpenFile(filename, NULL);

	}

	if (lnf_file->nffile == NULL) {
		return NULL;
	}


	lnf_file->blk_record_remains = 0;
	lnf_file->extension_map_list = InitExtensionMaps(NEEDS_EXTENSION_LIST);

	return lnf_file;
}

/* close file handler and release related structures */
void lnf_close(lnf_file_t *lnf_file) {

	if (lnf_file == NULL || lnf_file->nffile == NULL) {
		return ;
	}

	if (lnf_file->flags & LNF_WRITE) {

		// write the last records in buffer
		if (lnf_file->nffile->block_header->NumRecords ) {
			if ( WriteBlock(lnf_file->nffile) <= 0 ) {
				fprintf(stderr, "Failed to write output buffer: '%s'" , strerror(errno));
			}
		}
		CloseUpdateFile(lnf_file->nffile, NULL );

	} else {
		CloseFile(lnf_file->nffile);
	}

	DisposeFile(lnf_file->nffile);

	free(lnf_file);
}

/* return next record in file */
/* return pointer to record data structure */
/* returns NULL if some error ocurs */
int lnf_read(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

//master_record_t	*master_record;
int ret;
uint32_t map_id;

#ifdef COMPAT15
int	v1_map_done = 0;
#endif

begin:

	if (lnf_file->blk_record_remains == 0) {
	/* all records in block have been processed, we are going to load nex block */

		// get next data block from file
		if (lnf_file->nffile) {
			ret = ReadBlock(lnf_file->nffile);
			lnf_file->processed_blocks++;
			lnf_file->current_processed_blocks++;
		} else {	
			ret = NF_EOF;		/* the firt file in the list */
		}

		switch (ret) {
			case NF_CORRUPT:
				return LNF_ERR_CORRUPT;
			case NF_ERROR:
				return LNF_ERR_READ;
			case NF_EOF: 
				return LNF_EOF;
			default:
				// successfully read block
				lnf_file->processed_bytes += ret;
		}

		/* block types to be skipped  -> goto begin */
		/* block types that are unknown -> return */
		switch (lnf_file->nffile->block_header->id) {
			case DATA_BLOCK_TYPE_1:		/* old record type - nfdump 1.5 */
					lnf_file->skipped_blocks++;
					goto begin;
					return LNF_ERR_COMPAT15;
					break;
			case DATA_BLOCK_TYPE_2:		/* common record type - normally processed */
					break;
			case Large_BLOCK_Type:
					lnf_file->skipped_blocks++;
					goto begin;
					break;
			default: 
					lnf_file->skipped_blocks++;
					return LNF_ERR_UNKBLOCK;
		}

		lnf_file->flow_record = lnf_file->nffile->buff_ptr;
		lnf_file->blk_record_remains = lnf_file->nffile->block_header->NumRecords;
	} /* reading block */

	/* there are some records to process - we are going continue reading next record */
	lnf_file->blk_record_remains--;

	switch (lnf_file->flow_record->type) {
		case ExporterRecordType:
		case SamplerRecordype:
		case ExporterInfoRecordType:
		case ExporterStatRecordType:
		case SamplerInfoRecordype:
				lnf_file->flow_record = (common_record_t *)((pointer_addr_t)lnf_file->flow_record + lnf_file->flow_record->size);	
				goto begin;
				break;
		case ExtensionMapType: {
		
				extension_map_t *map = (extension_map_t *)lnf_file->flow_record;
				//Insert_Extension_Map(&instance->extension_map_list, map);
				Insert_Extension_Map(lnf_file->extension_map_list, map);

				lnf_file->flow_record = (common_record_t *)((pointer_addr_t)lnf_file->flow_record + lnf_file->flow_record->size);	
				goto begin;
				}
				break;
			
		case CommonRecordV0Type:
		case CommonRecordType:
				/* data record type - go ahead */
				break;

		default:
				lnf_file->flow_record = (common_record_t *)((pointer_addr_t)lnf_file->flow_record + lnf_file->flow_record->size);	
				return LNF_ERR_UNKREC;

	}

	/* we are sure that record is CommonRecordType */
	map_id = lnf_file->flow_record->ext_map;
	if ( map_id >= MAX_EXTENSION_MAPS ) {
		lnf_file->flow_record = (common_record_t *)((pointer_addr_t)lnf_file->flow_record + lnf_file->flow_record->size);	
		return LNF_ERR_EXTMAPB;
	}
	if ( lnf_file->extension_map_list->slot[map_id] == NULL ) {
		lnf_file->flow_record = (common_record_t *)((pointer_addr_t)lnf_file->flow_record + lnf_file->flow_record->size);	
		return LNF_ERR_EXTMAPM;
	} 

	lnf_file->processed_records++;

	lnf_file->master_record = &(lnf_file->extension_map_list->slot[map_id]->master_record);
	lnf_rec->extension_map = lnf_file->extension_map_list->slot[map_id]->map;
	lnf_rec->master_record = lnf_file->master_record;
//	lnf_file->engine->nfrecord = (uint64_t *)lnf_file->master_record_r;

	// changed in 1.6.8 - added exporter info 
//	ExpandRecord_v2( flow_record, extension_map_list.slot[map_id], master_record);
	ExpandRecord_v2(lnf_file->flow_record, lnf_file->extension_map_list->slot[map_id], NULL, lnf_rec->master_record);

	// update number of flows matching a given map
	lnf_file->extension_map_list->slot[map_id]->ref_count++;

	// Advance pointer by number of bytes for netflow record
	lnf_file->flow_record = (common_record_t *)((pointer_addr_t)lnf_file->flow_record + lnf_file->flow_record->size);	

/*
	{
		char *s;
		PrintExtensionMap(instance->extension_map_list.slot[map_id]->map);
		format_file_block_record(master_record, &s, 0);
		printf("READ: %s\n", s);
	}
*/
	/* the record seems OK. We prepare hash reference with items */
	lnf_file->lnf_rec = lnf_rec; /* XXX temporary */

	return LNF_OK;

} /* end of _readfnction */


