
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

/* Global Variables */
extern extension_descriptor_t extension_descriptor[];

#define FLOW_RECORD_NEXT(x) x = (common_record_t *)((pointer_addr_t)x + x->size)


/* open existing nfdump file and prepare for reading records */
/* only simple wrapper to nfdump function */
lnf_file_t * lnf_open(char * filename, unsigned int flags, char * ident) {
	int i;
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

	lnf_file->lnf_map_list = NULL;

	i = 1;
	lnf_file->max_num_extensions = 0;
	while ( extension_descriptor[i++].id )
		lnf_file->max_num_extensions++;

	bit_array_init(&lnf_file->extensions_arr, lnf_file->max_num_extensions + 1);	

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

	PackExtensionMapList(lnf_file->extension_map_list);
	FreeExtensionMaps(lnf_file->extension_map_list);

	free(lnf_file);
}

/* return next record in file */
/* status of read and fill pre-prepared structure lnf_rec */
int lnf_read(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {

//master_record_t	*master_record;
int ret;
uint32_t map_id;
extension_map_t *map;
int i;

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
				/* just skip */
				FLOW_RECORD_NEXT(lnf_file->flow_record);	
				goto begin;
				break;
		case ExtensionMapType: 
				map = (extension_map_t *)lnf_file->flow_record;
				//Insert_Extension_Map(&instance->extension_map_list, map);
				Insert_Extension_Map(lnf_file->extension_map_list, map);
				FLOW_RECORD_NEXT(lnf_file->flow_record);	
				goto begin;
				break;
			
		case CommonRecordV0Type:
		case CommonRecordType:
				/* data record type - go ahead */
				break;

		default:
				FLOW_RECORD_NEXT(lnf_file->flow_record);	
				return LNF_ERR_UNKREC;

	}

	/* we are sure that record is CommonRecordType */
	map_id = lnf_file->flow_record->ext_map;
	if ( map_id >= MAX_EXTENSION_MAPS ) {
		FLOW_RECORD_NEXT(lnf_file->flow_record);	
		return LNF_ERR_EXTMAPB;
	}
	if ( lnf_file->extension_map_list->slot[map_id] == NULL ) {
		FLOW_RECORD_NEXT(lnf_file->flow_record);	
		return LNF_ERR_EXTMAPM;
	} 

	lnf_file->processed_records++;

	lnf_file->master_record = &(lnf_file->extension_map_list->slot[map_id]->master_record);
//	lnf_rec->extension_map = lnf_file->extension_map_list->slot[map_id]->map;
	lnf_rec->master_record = lnf_file->master_record;
//	lnf_file->engine->nfrecord = (uint64_t *)lnf_file->master_record_r;

	// changed in 1.6.8 - added exporter info 
//	ExpandRecord_v2( flow_record, extension_map_list.slot[map_id], master_record);
	ExpandRecord_v2(lnf_file->flow_record, lnf_file->extension_map_list->slot[map_id], NULL, lnf_rec->master_record);

	// update number of flows matching a given map
	lnf_file->extension_map_list->slot[map_id]->ref_count++;

	// Move pointer by number of bytes for netflow record
	FLOW_RECORD_NEXT(lnf_file->flow_record);	
/*
	{
		char *s;
		PrintExtensionMap(instance->extension_map_list.slot[map_id]->map);
		format_file_block_record(master_record, &s, 0);
		printf("READ: %s\n", s);
	}
*/

	// processing map 
	bit_array_clear(&lnf_file->extensions_arr);

	i = 0;
	while (lnf_rec->master_record->map_ref->ex_id[i]) {
		bit_array_set(&lnf_file->extensions_arr, lnf_rec->master_record->map_ref->ex_id[i], 1);
		i++;
	}

	lnf_rec->extensions_arr = &(lnf_file->extensions_arr);

	/* the record seems OK. We prepare hash reference with items */
	lnf_file->lnf_rec = lnf_rec; /* XXX temporary */

	return LNF_OK;

} /* end of _readfnction */


extension_map_t * lnf_lookup_map(lnf_file_t *lnf_file, bit_array_t *ext ) {
extension_map_t *map; 
lnf_map_list_t *map_list;
int i = 0;
int is_set = 0;
int id = 0;
int map_id = 0;

	// find whether the template already exist 
	map_id = 0;

	map_list = lnf_file->lnf_map_list; 
	if (map_list == NULL) {
		// first map 
		map_list =  malloc(sizeof(lnf_map_list_t));
		lnf_file->lnf_map_list = map_list;
	} else {
		if (bit_array_cmp(&(map_list->bit_array), ext) == 0) {
			return map_list->map;
		}
		map_id++;
		while (map_list->next != NULL ) {
			if (bit_array_cmp(&(map_list->bit_array), ext) == 0) {
				return map_list->map;
			} else {
				map_id++;
				map_list = map_list->next;
			}
		}
		map_list->next = malloc(sizeof(lnf_map_list_t));
		map_list = map_list->next;
	}
	
	// allocate memory potentially for all extensions 
	map = malloc(sizeof(extension_map_t) + (lnf_file->max_num_extensions + 1) * sizeof(uint16_t));

	map_list->map = map;
	map_list->next = NULL;

	bit_array_init(&map_list->bit_array, lnf_file->max_num_extensions + 1);
	bit_array_copy(&map_list->bit_array, ext);

	map->type   = ExtensionMapType;
	map->map_id = map_id; 
			
	// set extension map according the bits set in ext structure 
	id = 0;
	i = 0;
	while ( (is_set = bit_array_get(ext, id)) != -1 ) {
//		fprintf(stderr, "i: %d, bit %d, val: %d\n", i, id, is_set);
		if (is_set) 
			map->ex_id[i++]  = id;
		id++;
	}
	map->ex_id[i++] = 0;

	// determine size and align 32bits
	map->size = sizeof(extension_map_t) + ( i - 1 ) * sizeof(uint16_t);
	if (( map->size & 0x3 ) != 0 ) {
		map->size += (4 - ( map->size & 0x3 ));
	}

	map->extension_size = 0;
	i=0;
	while (map->ex_id[i]) {
		int id = map->ex_id[i];
		map->extension_size += extension_descriptor[id].size;
		i++;
	}

	//Insert_Extension_Map(&instance->extension_map_list, map); 
	Insert_Extension_Map(lnf_file->extension_map_list, map); 
	AppendToBuffer(lnf_file->nffile, (void *)map, map->size);

	return map;
}



/* return next record in file */
/* status of read and fill pre-prepared structure lnf_rec */
int lnf_write(lnf_file_t *lnf_file, lnf_rec_t *lnf_rec) {
extension_map_t *map;

	/* lookup and add map into file it it is nescessary */
	map = lnf_lookup_map(lnf_file, lnf_rec->extensions_arr);

	if (map == NULL) {
		return LNF_ERR_WRITE;
	}

	lnf_rec->master_record->map_ref = map;
	lnf_rec->master_record->ext_map = map->map_id;
	lnf_rec->master_record->type = CommonRecordType;

	UpdateStat(lnf_file->nffile->stat_record, lnf_rec->master_record);

	PackRecord(lnf_rec->master_record, lnf_file->nffile);

	return LNF_OK;
}

