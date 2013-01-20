 

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "libnf.h"
#include "bit_array.h"

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


#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif


/* Ignore Math-Int64 on 64 bit platform */
/* #define MATH_INT64_NATIVE 1 */

#if MATH_INT64_NATIVE
#undef newSVu64
#define newSVu64 newSVuv
#undef SvU64 
#define SvU64 SvUV
#endif


/* hash parameters */
#define NumPrealloc 128000

#define AGGR_SIZE 7

#define STRINGSIZE 10240
static char data_string[STRINGSIZE];

/* list of maps used in file taht we create */
typedef struct libnf_file_list_s {
	char			 			*filename;
	struct libnf_file_list_s 	*next;
} libnf_file_list_t;

/* list of maps used in file taht we create */
typedef struct libnf_map_list_s {
	bit_array_t 				bit_array;
	extension_map_t 			*map;
	struct libnf_map_list_s 	*next;
} libnf_map_list_t;


/* structure that bears all data related to one instance */
typedef struct libnf_instance_s {
	extension_map_list_t 	extension_map_list;		/* nfdup structure containig extmap */
	libnf_map_list_t		*map_list;				/* libnf structure that holds maps */
	int 					max_num_extensions;		/* mamimum number of extensions */
	libnf_file_list_t		*files;					/* list of files to read */
	nffile_t				*nffile_r;				/* filehandle to the file that we read data from */
	nffile_t				*nffile_w;				/* filehandle for writing */
	int 					blk_record_remains; 	/* counter of processed rows in a signle block */
	FilterEngine_data_t		*engine;
	common_record_t			*flow_record;
//	stat_record_t			stat_record;
	uint64_t				processed_bytes;		/* read statistics */
	uint64_t				total_files;
	uint64_t				processed_files;
	uint64_t				processed_blocks;
	uint64_t				skipped_blocks;
	uint64_t				processed_records;
	char 					*current_filename;		/* currently processed file name */
	uint64_t				current_processed_blocks;
//	uint32_t				is_anonymized;
	time_t 					t_first_flow, t_last_flow;
	time_t					twin_start, twin_end;
} libnf_instance_t;


/* array of initalized instances */
libnf_instance_t *libnf_instances[NFL_MAX_INSTANCES] = { NULL };


/* last used handle identifier, starts with 0 */
//int libnf_last_instance = 1;		



/* Global Variables */
//FilterEngine_data_t	*Engine;

//extern char	*FilterFilename;
//extern uint32_t loopcnt;
extern extension_descriptor_t extension_descriptor[];

/* Local Variables */
//const char *nfdump_version = VERSION;
/*
static uint64_t total_bytes;
static uint32_t total_flows;
static uint32_t skipped_blocks;
static uint32_t	is_anonymized;
static time_t 	t_first_flow, t_last_flow;
static time_t	twin_start, twin_end;
static char		Ident[IDENTLEN];
*/

//common_record_t 	*flow_record;
//nffile_t			*nffile_r;
//int					map_initalized = 0;	/* defines whether the map was already initialized (for writing file) */


//int hash_hit = 0; 
//int hash_miss = 0;
//int hash_skip = 0;

//extension_map_list_t extension_map_list;
//static extension_info_t extension_info;



// compare at most 16 chars
#define MAXMODELEN	16	

#define STRINGSIZE 10240
#define IP_STRING_LEN (INET6_ADDRSTRLEN)

#include "nfdump_inline.c"
#include "nffile_inline.c"
#include "nf_common.c"

/* compare key in hashref */
#define CMP_STR(k, v) strnEQ(k, v, strlen(v))

/* defining macros for storing numbers, 64 bit numbers and strings into hash */
#define HV_STORE_NV(r,k,v) (void)hv_store(r, k, strlen(k), newSVnv(v), 0)
#define HV_STORE_U64V(r,k,v) (void)hv_store(r, k, strlen(k), newSVu64v(v), 0)
#define HV_STORE_PV(r,k,v) (void)hv_store(r, k, strlen(k), newSVpvn(v, strlen(v)), 0)

/* function to store IPv4 or IPv6 address into hash */
void static inline HV_STORE_AV(HV *r, char *k, ip_addr_t *a, int is6) {
char s[IP_STRING_LEN];

	//s[0] = 0;
	int len = 0;

	if ( is6 ) { // IPv6
		uint64_t ip[2];

		ip[0] = htonll(a->v6[0]);
		ip[1] = htonll(a->v6[1]);
		//ip[0] = a->v6[0];
		//ip[1] = a->v6[1];
		len = sizeof(a->v6);
		memcpy(s, ip, len);
	} else {    // IPv4
		uint32_t ip;

		//ip = htonl(a->v4);
		ip = a->v4;
//		ip = a->v4;
		len = sizeof(a->v4);
		memcpy(s, &ip, len);
	}

	(void)hv_store(r, k, strlen(k), newSVpvn(s, len), 0);
}

/* function to store MAC address into hash */
void static inline HV_STORE_MV(HV *r,char *k, uint8_t *a) {
char s[MAX_STRING_LENGTH];
int i;

	s[0] = 0;

	for ( i=0; i<6; i++ ) {
		s[5 - i] = a[i] & 0xFF;
    }
//	snprintf(s, MAX_STRING_LENGTH-1 ,"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
//	s[MAX_STRING_LENGTH-1] = '\0';

	(void)hv_store(r, k, strlen(k), newSVpvn(s, 6), 0);
}

/* converts perl's SV with string representation of IP addres to  ip_addr_t */
inline int Sv_addr(ip_addr_t *a, SV * sv) {
uint64_t ip6[2];
uint32_t ip4;
char *s;
STRLEN len;

	s = SvPV(sv, len);

	if ( len == sizeof(ip4) )  {
		memcpy(&ip4, s, sizeof(ip4));
		//a->v4 = ntohl(ip4);
		a->v4 = ip4;
//		a->v4 = ip4;
		return AF_INET;
	} else {
		memcpy(ip6, s, sizeof(ip6));
		a->v6[0] = ntohll(ip6[0]);
		a->v6[1] = ntohll(ip6[1]);
		//a->v6[0] = ip6[0];
		//a->v6[1] = ip6[1];
		return AF_INET6;
	}

	return -1;
}

/* converts perl's SV with string representation of MAC addres and store to uint64_t   */
/* returns 0 if conversion was not succesfull */
inline int Sv_mac(uint64_t *a, SV * sv) {
uint8_t *mac = (uint8_t *)a;
char *s;
int i; 
STRLEN len;

	s = SvPV(sv, len);

	if ( len != 6 ) 
		return -1;
		
	for (i = 0; i < 6; i++) {
//		long b = strtol(s+(3*i), (char **) NULL, 16);
		mac[5 - i] = s[i];
	}
	
	mac[6] = 0x0;
	mac[7] = 0x0;

	return 0;
}

/* returns the information about file get from file header */
SV * libnf_file_info(char *file) {
HV *res;
nffile_t *nffile = NULL;

	res = (HV *)sv_2mortal((SV *)newHV());

	nffile = OpenFile((char *)file, nffile);
	if ( nffile == NULL ) {
		return NULL;
	}

	HV_STORE_NV(res, "version", nffile->file_header->version);
	HV_STORE_NV(res, "blocks", nffile->file_header->NumBlocks);
	HV_STORE_NV(res, "compressed", nffile->file_header->flags & FLAG_COMPRESSED);
	HV_STORE_NV(res, "anonymized", nffile->file_header->flags & FLAG_ANONYMIZED);
	HV_STORE_NV(res, "catalog", nffile->file_header->flags & FLAG_CATALOG);
	HV_STORE_PV(res, "ident", nffile->file_header->ident);

	if (nffile->stat_record != NULL) {
		HV_STORE_NV(res, "flows", nffile->stat_record->numflows);
		HV_STORE_NV(res, "bytes", nffile->stat_record->numbytes);
		HV_STORE_NV(res, "packets", nffile->stat_record->numpackets);
		
		HV_STORE_NV(res, "flows_tcp", nffile->stat_record->numflows_tcp);
		HV_STORE_NV(res, "bytes_tcp", nffile->stat_record->numbytes_tcp);
		HV_STORE_NV(res, "packets_tcp", nffile->stat_record->numpackets_tcp);

		HV_STORE_NV(res, "flows_udp", nffile->stat_record->numflows_udp);
		HV_STORE_NV(res, "bytes_udp", nffile->stat_record->numbytes_udp);
		HV_STORE_NV(res, "packets_udp", nffile->stat_record->numpackets_udp);

		HV_STORE_NV(res, "flows_icmp", nffile->stat_record->numflows_icmp);
		HV_STORE_NV(res, "bytes_icmp", nffile->stat_record->numbytes_icmp);
		HV_STORE_NV(res, "packets_icmp", nffile->stat_record->numpackets_icmp);

		HV_STORE_NV(res, "flows_other", nffile->stat_record->numflows_other);
		HV_STORE_NV(res, "bytes_other", nffile->stat_record->numbytes_other);
		HV_STORE_NV(res, "packets_other", nffile->stat_record->numpackets_other);

		HV_STORE_NV(res, "first", nffile->stat_record->first_seen);
		HV_STORE_NV(res, "last", nffile->stat_record->last_seen);
		HV_STORE_NV(res, "msec_first", nffile->stat_record->msec_first);
		HV_STORE_NV(res, "msec_last", nffile->stat_record->msec_last);

		HV_STORE_NV(res, "sequence_failures", nffile->stat_record->sequence_failure);
	}

	CloseFile(nffile);
	DisposeFile(nffile);
	
	
	return newRV((SV *)res);
}

/* returns the information about instance */
SV * libnf_instance_info(int handle) {
libnf_instance_t *instance = libnf_instances[handle];
HV *res;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}

	res = (HV *)sv_2mortal((SV *)newHV());

	if ( instance->current_filename != NULL && instance->nffile_r != NULL ) {
		int nblocs = instance->nffile_r->file_header->NumBlocks;
		HV_STORE_PV(res, "current_filename", instance->current_filename);
		HV_STORE_NV(res, "current_processed_blocks", instance->current_processed_blocks);
		HV_STORE_NV(res, "current_total_blocks", nblocs);
	}
	HV_STORE_NV(res, "total_files", instance->total_files);
	HV_STORE_NV(res, "processed_files", instance->processed_files);
	HV_STORE_NV(res, "processed_blocks", instance->processed_blocks);
	HV_STORE_NV(res, "processed_bytes", instance->processed_bytes);
	HV_STORE_NV(res, "processed_records", instance->processed_records);

	return newRV((SV *)res);
}



/* converts master_record to perl structures (hashref) */
/* TAG for check_items_map.pl: libnf_master_record_to_SV */
SV * libnf_master_record_to_SV(master_record_t *rec, extension_map_t *map) {
HV *res;
int i=0;

	res = (HV *)sv_2mortal((SV *)newHV());

	// time fields
	HV_STORE_NV(res, NFL_FIRST, rec->first);
	HV_STORE_NV(res, NFL_MSEC_FIRST, rec->msec_first);
	HV_STORE_NV(res, NFL_LAST, rec->last);
	HV_STORE_NV(res, NFL_MSEC_LAST, rec->msec_last);

	HV_STORE_NV(res, NFL_PROT, rec->prot);
	HV_STORE_NV(res, NFL_TCP_FLAGS, rec->tcp_flags);
	
	// Required extension 1 - IP addresses 
	// NOTE: srcaddr and dst addr do not uses ip_addr_t union/structure 
	// however the structures are simmilar so we will pretend 
	// that v6.srcaddr and v6.dst addr points to same structure 
	HV_STORE_AV(res, NFL_SRCADDR, (ip_addr_t *)&rec->v6.srcaddr, rec->flags & FLAG_IPV6_ADDR);
	HV_STORE_AV(res, NFL_DSTADDR, (ip_addr_t *)&rec->v6.dstaddr, rec->flags & FLAG_IPV6_ADDR);

	HV_STORE_NV(res, NFL_SRCPORT, rec->srcport);
	HV_STORE_NV(res, NFL_DSTPORT, rec->dstport);

	// Required extension 2,3 - counters
	HV_STORE_NV(res, NFL_DPKTS, rec->dPkts);
	HV_STORE_NV(res, NFL_DOCTETS, rec->dOctets);

	HV_STORE_NV(res, NFL_FWD_STATUS, rec->fwd_status);
	HV_STORE_NV(res, NFL_TOS, rec->tos);

//	String_DstAddr(rec, s);
//	hv_store(res, "dstaddr", strlen("dstaddr"), newSVpvn(s, strlen(s)), 0);
		
    while (map->ex_id[i]) {
   	    switch(map->ex_id[i++]) {
			case 1:
			case 2:
			case 3:
				break;
			case EX_IO_SNMP_2: 
			case EX_IO_SNMP_4: 
				HV_STORE_NV(res, NFL_INPUT, rec->input);
				HV_STORE_NV(res, NFL_OUTPUT, rec->output);
				break;
			case EX_AS_2: 
			case EX_AS_4: 
				HV_STORE_NV(res, NFL_SRCAS, rec->srcas);
				HV_STORE_NV(res, NFL_DSTAS, rec->dstas);
				break;
			case EX_MULIPLE: 
				HV_STORE_NV(res, NFL_DST_TOS, rec->dst_tos);
				HV_STORE_NV(res, NFL_DIR, rec->dir);
				HV_STORE_NV(res, NFL_SRC_MASK, rec->src_mask);
				HV_STORE_NV(res, NFL_DST_MASK, rec->dst_mask);
				break;
			case EX_NEXT_HOP_v4:
			case EX_NEXT_HOP_v6:
				HV_STORE_AV(res, NFL_IP_NEXTHOP, (ip_addr_t *)&rec->ip_nexthop, rec->flags & FLAG_IPV6_NH);
				break;
			case EX_NEXT_HOP_BGP_v4:
			case EX_NEXT_HOP_BGP_v6:
				HV_STORE_AV(res, NFL_BGP_NEXTHOP, (ip_addr_t *)&rec->bgp_nexthop, rec->flags & FLAG_IPV6_NH);
				break;
			case EX_VLAN: 
				HV_STORE_NV(res, NFL_SRC_VLAN, rec->src_vlan);
				HV_STORE_NV(res, NFL_DST_VLAN, rec->dst_vlan);
				break;
			case EX_OUT_PKG_4:
			case EX_OUT_PKG_8:
				HV_STORE_NV(res, NFL_OUT_PKTS, rec->out_pkts);
				break;
			case EX_OUT_BYTES_4:
			case EX_OUT_BYTES_8:
				HV_STORE_NV(res, NFL_OUT_BYTES, rec->out_bytes);
				break;
			case EX_AGGR_FLOWS_4:
			case EX_AGGR_FLOWS_8:
				HV_STORE_NV(res, NFL_AGGR_FLOWS, rec->aggr_flows);
				break;
			case EX_MAC_1:
				HV_STORE_MV(res, NFL_IN_SRC_MAC, (u_int8_t *)&rec->in_src_mac);
				HV_STORE_MV(res, NFL_OUT_DST_MAC, (u_int8_t *)&rec->out_dst_mac);
				break;
			case EX_MAC_2:
				HV_STORE_MV(res, NFL_OUT_SRC_MAC, (u_int8_t *)&rec->out_src_mac);
				HV_STORE_MV(res, NFL_IN_DST_MAC, (u_int8_t *)&rec->in_dst_mac);
				break;
			case EX_MPLS:
				(void)hv_store(res, NFL_MPLS_LABEL, strlen(NFL_MPLS_LABEL), 
						newSVpvn((char *)rec->mpls_label, sizeof(rec->mpls_label)), 0);
				break;
			case EX_ROUTER_IP_v4:
			case EX_ROUTER_IP_v6:
				HV_STORE_AV(res, NFL_IP_ROUTER, (ip_addr_t *)&rec->ip_router, rec->flags & FLAG_IPV6_EXP);
				break;
			case EX_ROUTER_ID:
				HV_STORE_NV(res, NFL_ENGINE_TYPE, rec->engine_type);
				HV_STORE_NV(res, NFL_ENGINE_ID, rec->engine_id);
				break;
			case EX_BGPADJ:
				HV_STORE_NV(res, NFL_BGPNEXTADJACENTAS, rec->bgpNextAdjacentAS);
				HV_STORE_NV(res, NFL_BGPPREVADJACENTAS, rec->bgpPrevAdjacentAS);
				break;
			case EX_LATENCY:
				HV_STORE_NV(res, NFL_CLIENT_NW_DELAY_USEC, rec->client_nw_delay_usec);
				HV_STORE_NV(res, NFL_SERVER_NW_DELAY_USEC, rec->server_nw_delay_usec);
				HV_STORE_NV(res, NFL_APPL_LATENCY_USEC, rec->appl_latency_usec);
				break;
			case EX_RECEIVED:
				HV_STORE_NV(res, NFL_RECEIVED, rec->received);

		}
//			printf("  %d Index %3i, ext %3u = %s\n", i, extension_descriptor[id].user_index, id, extension_descriptor[id].description );
	}

/*
		flow_record_to_csv(master_record, &string, 0);
		if ( string ) {
				printf("%s\n", string);
		}
*/
	return newRV((SV *)res);
}

extension_map_t * libnf_lookup_map( libnf_instance_t *instance, bit_array_t *ext ) {
extension_map_t *map; 
libnf_map_list_t *map_list;
int i = 0;
int is_set = 0;
int id = 0;
int map_id = 0;

	// find whether the template already exist 
	map_id = 0;
	map_list = instance->map_list; 
	if (map_list == NULL) {
		// first map 
		map_list =  malloc(sizeof(libnf_map_list_t));
		instance->map_list = map_list;
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
		map_list->next = malloc(sizeof(libnf_map_list_t));
		map_list = map_list->next;
	}
	
	// allocate memory potentially for all extensions 
	map = malloc(sizeof(extension_map_t) + (instance->max_num_extensions + 1) * sizeof(uint16_t));

	map_list->map = map;
	map_list->next = NULL;

	bit_array_init(&map_list->bit_array, instance->max_num_extensions + 1);
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

	Insert_Extension_Map(&instance->extension_map_list, map); 
//	PackExtensionMapList(&instance->extension_map_list);
//	PrintExtensionMap(map);
	AppendToBuffer(instance->nffile_w, (void *)map, map->size);

	return map;
}




int libnf_init(void) {
int handle = 1;
libnf_instance_t *instance;
//char *filter = NULL;
int i;

	/* find the first free handler and assign to array of open handlers/instances */
	while (libnf_instances[handle] != NULL) {
		handle++;
		if (handle >= NFL_MAX_INSTANCES - 1) {
			croak("% no free handles available, max instances %d reached", NFL_LOG, NFL_MAX_INSTANCES);
			return 0;	
		}
	}

	instance = malloc(sizeof(libnf_instance_t));
	memset(instance, 0, sizeof(libnf_instance_t));

	if (instance == NULL) {
		croak("% can not allocate memory for instance:", NFL_LOG );
		return 0;
	}

	instance->map_list = NULL;

	libnf_instances[handle] = instance;

//	Ident[0] = '\0';

//	SetStat_DefaultOrder("flows");
/*
	for ( i=0; i<AGGR_SIZE; AggregateMasks[i++] = 0 ) ;
*/

	/* initialize ExtensionMaps */
//	SetupExtensionDescriptors("");
	InitExtensionMaps(&(instance->extension_map_list));
	i = 1;
	instance->max_num_extensions = 0;
	while ( extension_descriptor[i++].id )
		instance->max_num_extensions++;



	//SetupInputFileSequence(NULL, "/root/src/nfdump/Net-NfDump/t/data/dump1.nfcap", NULL);
//	SetupInputFileSequence(NULL, "/root/src/nfdump/Net-NfDump/t/data/dump2.nfcap", NULL);

//	print_header = format_file_block_header;
	//print_record = flow_record_to_csv;
//	print_record =libnf_row_callback; 

/*	
	filter = "any";

	instance->engine = CompileFilter(filter);
	if ( !instance->engine ) 
		exit(254);

*/
//	SetLimits(element_stat || aggregate || flow_stat, packet_limit_string, byte_limit_string);

	//nfprof_start(&profile_data);

	//ffile_r = NULL;

	// Get the first file handle
	/*
	nffile_r = GetNextFile(NULL, twin_start, twin_end);
	if ( !nffile_r ) {
		LogError("GetNextFile() error in %s line %d: %s\n", __FILE__, __LINE__, strerror(errno) );
		return 0;
	}
	if ( nffile_r == EMPTY_LIST ) {
		LogError("Empty file list. No files to process\n");
		return 0;
	}
	*/

	// preset time window of all processed flows to the stat record in first flow file
	/*
	t_first_flow = nffile_r->stat_record->first_seen;
	t_last_flow  = nffile_r->stat_record->last_seen;
	*/

	// store infos away for later use
	// although multiple files may be processed, it is assumed that all 
	// have the same settings
	/*
	is_anonymized = IP_ANONYMIZED(nffile_r);
	strncpy(Ident, nffile_r->file_header->ident, IDENTLEN);
	Ident[IDENTLEN-1] = '\0';
	*/

//	blk_record_remains = 0;

	instance->nffile_w = NULL;

/*
	while ( libnf_fetchrow_hashref() == NF_OK ) { 
		printf(""); 
	};
*/

//	libnf_finish();

	//nfprof_end(&profile_data, total_flows);

//	Dispose_FlowTable();
//	Dispose_StatTable();

	return handle;
}



int libnf_read_files(int handle, char *filter, int window_start, int window_end, SV *files) {
libnf_instance_t *instance = libnf_instances[handle];

libnf_file_list_t	*pfile;
I32 numfiles = 0;
int i;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}

	/* copy files to the instance structure */
	if ((!SvROK(files))
		|| (SvTYPE(SvRV(files)) != SVt_PVAV) 
		|| ((numfiles = av_len((AV *)SvRV(files))) < 0)) {
			croak("%s can not determine the list of files", NFL_LOG);
			return 0;
	}

	pfile = malloc(sizeof(libnf_file_list_t));
	pfile->next = NULL;
	pfile->filename = NULL;
	instance->files = pfile;
	instance->total_files = numfiles + 1;

	for (i = 0; i <= numfiles; i++) {
		STRLEN l;
		char * file = SvPV(*av_fetch((AV *)SvRV(files), i, 0), l);
		

		pfile->filename = malloc(l + 1);
		strcpy((char *)(pfile->filename), file);
		
		pfile->next = malloc(sizeof(libnf_file_list_t));
		pfile = pfile->next;
		pfile->filename = NULL;

	}
	instance->nffile_r = NULL;

	/* set filter */
	if (filter == NULL || strcmp(filter, "") == 0) {
		filter = "any";
	}

	instance->engine = CompileFilter(filter);
	if ( !instance->engine ) {
		croak("%s can not setup filter (%s)", NFL_LOG, filter);
		return 0;
	}
	
	return 1;
}


int libnf_create_file(int handle, char *filename, int compressed, int anonymized, char *ident) {
libnf_instance_t *instance = libnf_instances[handle];

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}


	/* the file was already opened */
	if (instance->nffile_w != NULL) {
		croak("%s file handler was opened before", NFL_LOG);
		return 0;
	}

	/* writing file */
    instance->nffile_w = OpenNewFile(filename, NULL, compressed, anonymized, ident);
    if ( !instance->nffile_w ) {
		warn("%s cannot open file %s", NFL_LOG, filename);
		return 0;
    }

	return 1;
}

                                  
/* returns hashref or NULL if we are et the end of the file */
SV * libnf_read_row(int handle) {
master_record_t	*master_record;
libnf_instance_t *instance = libnf_instances[handle];
int ret;
int match;
uint32_t map_id;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}

#ifdef COMPAT15
int	v1_map_done = 0;
#endif

begin:

	if (instance->blk_record_remains == 0) {
	/* all records in block have been processed, we are going to load nex block */

		// get next data block from file
		if (instance->nffile_r) {
			ret = ReadBlock(instance->nffile_r);
			instance->processed_blocks++;
			instance->current_processed_blocks++;
		} else {	
			ret = NF_EOF;		/* the firt file in the list */
		}

		switch (ret) {
			case NF_CORRUPT:
				LogError("Skip corrupt data file '%s'\n",GetCurrentFilename());
				exit(1);
			case NF_ERROR:
				LogError("Read error in file '%s': %s\n",GetCurrentFilename(), strerror(errno) );
				exit(1);
				// fall through - get next file in chain
			case NF_EOF: {
				libnf_file_list_t *next;

				//nffile_t *next = GetNextFile(nffile_r, twin_start, twin_end);
				CloseFile(instance->nffile_r);
				if (instance->files->filename == NULL) {	// the end of the list 
					free(instance->files);
					instance->files = NULL;		
					return NULL;
				}
				instance->nffile_r = OpenFile((char *)instance->files->filename, instance->nffile_r);
				instance->processed_files++;
				instance->current_processed_blocks = 0;

				next = instance->files->next;

				/* prepare instance->files to nex unread file */
				if (instance->current_filename != NULL) {
					free(instance->current_filename);
				}
				instance->current_filename = instance->files->filename;
				free(instance->files);
				instance->files = next;

				if ( instance->nffile_r == NULL ) {
					croak("%s can not read file %s", NFL_LOG, instance->files->filename);
					return NULL;
				}
				goto begin;
			}

			default:
				// successfully read block
				instance->processed_bytes += ret;
		}

		if ( instance->nffile_r->block_header->id == Large_BLOCK_Type ) {
			// skip
			//printf("Xstat block skipped ...\n");
			goto begin;
		}

		if ( instance->nffile_r->block_header->id != DATA_BLOCK_TYPE_2 ) {
			if ( instance->nffile_r->block_header->id == DATA_BLOCK_TYPE_1 ) {
				croak("%s Can't process nfdump 1.5.x block type 1. Block skipped.", NFL_LOG);
				return NULL;
			} else {
				croak("%s Can't process block type %u. Block skipped.", NFL_LOG, instance->nffile_r->block_header->id);
				return NULL;
			}
			instance->skipped_blocks++;
			goto begin;
		}

		instance->flow_record = instance->nffile_r->buff_ptr;
		instance->blk_record_remains = instance->nffile_r->block_header->NumRecords;

	} 

	/* there are some records to process - we are going continue reading next record */

//	printf("BLK: %d\n", blk_record_remains);
	instance->blk_record_remains--;

	if ( instance->flow_record->type == ExtensionMapType ) {
		extension_map_t *map = (extension_map_t *)instance->flow_record;
		Insert_Extension_Map(&instance->extension_map_list, map);

		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;

	} else if ( instance->flow_record->type != CommonRecordType ) {
		warn("%s Skip unknown record type %i\n", NFL_LOG, instance->flow_record->type);
		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;
	}

	/* we are sure that record is CommonRecordType */

//	int match;
	map_id = instance->flow_record->ext_map;
	if ( map_id >= MAX_EXTENSION_MAPS ) {
		croak("%s Corrupt data file. Extension map id %u too big.\n", NFL_LOG, instance->flow_record->ext_map);
		return 0;
	}
	if ( instance->extension_map_list.slot[map_id] == NULL ) {
		warn("%s Corrupt data file. Missing extension map %u. Skip record.\n", NFL_LOG, instance->flow_record->ext_map);
		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		//continue;
		goto begin;
	} 

	instance->processed_records++;
	master_record = &(instance->extension_map_list.slot[map_id]->master_record);
	instance->engine->nfrecord = (uint64_t *)master_record;

	// changed in 1.6.8 - added exporter info 
//	ExpandRecord_v2( flow_record, extension_map_list.slot[map_id], master_record);
	ExpandRecord_v2( instance->flow_record, instance->extension_map_list.slot[map_id], NULL, master_record);

	// Time based filter
	// if no time filter is given, the result is always true
	match  = instance->twin_start && (master_record->first < instance->twin_start || 
						master_record->last > instance->twin_end) ? 0 : 1;
//	match &= limitflows ? stat_record.numflows < limitflows : 1;

	// filter netflow record with user supplied filter
	if ( match ) 
		match = (*instance->engine->FilterEngine)(instance->engine);

	if ( match == 0 ) { // record failed to pass all filters
		// increment pointer by number of bytes for netflow record
		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;
	}

	// update number of flows matching a given map
	instance->extension_map_list.slot[map_id]->ref_count++;

	// Advance pointer by number of bytes for netflow record
	instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	

/*
	{
		char *s;
		PrintExtensionMap(instance->extension_map_list.slot[map_id]->map);
		format_file_block_record(master_record, &s, 0);
		printf("READ: %s\n", s);
	}
*/

	/* the record seems OK. We prepare hash reference with items */
	return libnf_master_record_to_SV(master_record, instance->extension_map_list.slot[map_id]->map); 

} /* end of _next fnction */


/* TAG for check_items_map.pl: libnf_write_row */
int libnf_write_row(int handle, HV * hashref) {
master_record_t rec;
libnf_instance_t *instance = libnf_instances[handle];
extension_map_t *map;
char *key;
int len;
STRLEN svlen;
SV * sv;
bit_array_t ext;


	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}

	memzero(&rec, sizeof(rec));


	/* check the hashref */
	hv_iterinit(hashref);

	// detect the maximum number of extensions 
		
	bit_array_init(&ext, instance->max_num_extensions + 1);

	/* go through handled hashref and enable extensions according the key values */	
	while ( (sv = hv_iternextsv(hashref, &key, (I32*) &len)) != NULL ) {
//		fprintf(stderr, "%s : %s\n", key, SvPV(sv, len));

		// time items
		if ( CMP_STR(key, NFL_FIRST)) {
			rec.first = SvUV(sv);
		} else if ( CMP_STR(key, NFL_MSEC_FIRST)) {
			rec.msec_first = SvUV(sv);
		} else if ( CMP_STR(key, NFL_LAST)) {
			rec.last = SvUV(sv);
		} else if ( CMP_STR(key, NFL_MSEC_LAST)) {
			rec.msec_last = SvUV(sv);
 
		// preotocol + tcp_flags + fwd_status 
		} else if ( CMP_STR(key, NFL_PROT)) {
			rec.prot = SvUV(sv);
		} else if ( CMP_STR(key, NFL_TCP_FLAGS) ) {
			rec.tcp_flags = SvUV(sv);
		} else if ( CMP_STR(key, NFL_FWD_STATUS) ) {
			rec.fwd_status = SvUV(sv);
		} else if ( CMP_STR(key, NFL_TOS) ) {
			rec.tos = SvUV(sv);

		// BASIC ITEMS SRC/DST ADDR/PORTS
		} else if ( CMP_STR(key, NFL_SRCADDR)) {
			int res = Sv_addr((ip_addr_t *)&rec.v6.srcaddr, sv);
			switch (res) {
				case AF_INET:
					ClearFlag(rec.flags, FLAG_IPV6_ADDR);
					break;
				case AF_INET6:
					SetFlag(rec.flags, FLAG_IPV6_ADDR);
					break;
				default: 
					warn("%s invalid value for %s", NFL_LOG, NFL_SRCADDR);
					return 0;
			}
		} else if ( CMP_STR(key, NFL_DSTADDR)) {
			int res = Sv_addr((ip_addr_t *)&rec.v6.dstaddr, sv);
			switch (res) {
				case AF_INET:
					ClearFlag(rec.flags, FLAG_IPV6_ADDR);
					break;
				case AF_INET6:
					SetFlag(rec.flags, FLAG_IPV6_ADDR);
					break;
				default: 
					warn("%s invalid value for %s", NFL_LOG, NFL_DSTADDR);
					return 0;
			}

		// aggregated flows 
		// EX_AGGR_FLOWS_4 is nod used, we always use 32b. version
		} else if ( CMP_STR(key, NFL_AGGR_FLOWS) ) {
			rec.aggr_flows = SvU64(sv);
			bit_array_set(&ext, EX_AGGR_FLOWS_8, 1);

		// src/dst port
		} else if ( CMP_STR(key, NFL_SRCPORT)) {
			rec.srcport = SvUV(sv);
		} else if ( CMP_STR(key, NFL_DSTPORT)) {
			rec.dstport = SvUV(sv);

		// bytes, packets
		} else if ( CMP_STR(key, NFL_DPKTS)) {
			rec.dPkts = SvU64(sv);
		} else if ( CMP_STR(key, NFL_DOCTETS)) {
			rec.dOctets = SvU64(sv);

		// INPUT + OUTPUT interface 
		// EX_IO_SNMP_2 is nod used, we always use 32b. version
		} else if ( CMP_STR(key, NFL_INPUT) ) {
			rec.input = SvUV(sv);
			bit_array_set(&ext, EX_IO_SNMP_4, 1);
		} else if ( CMP_STR(key, NFL_OUTPUT) ) {
			rec.output = SvUV(sv);
			bit_array_set(&ext, EX_IO_SNMP_4, 1);

		// AS numbers
		// EX_AS_2 is nod used, we always use 32b. version
		} else if ( CMP_STR(key, NFL_SRCAS) ) {
			rec.srcas = SvUV(sv);
			bit_array_set(&ext, EX_AS_4, 1);
		} else if ( CMP_STR(key, NFL_DSTAS) ) {
			rec.dstas = SvUV(sv);
			bit_array_set(&ext, EX_AS_4, 1);

		// bgp AdjacentAS, EX_BGPADJ
		} else if ( CMP_STR(key, NFL_BGPNEXTADJACENTAS) ) {
			rec.bgpNextAdjacentAS = SvUV(sv);
			bit_array_set(&ext, EX_BGPADJ, 1);
		} else if ( CMP_STR(key, NFL_BGPPREVADJACENTAS) ) {
			rec.bgpPrevAdjacentAS = SvUV(sv);
			bit_array_set(&ext, EX_BGPADJ, 1);

		// DST TOS, DIRECTION, MASKS
		} else if ( CMP_STR(key, NFL_DST_TOS) ) {
			rec.dst_tos = SvUV(sv);
			bit_array_set(&ext, EX_MULIPLE, 1);
		} else if ( CMP_STR(key, NFL_DIR) ) {
			rec.dir = SvUV(sv);
			bit_array_set(&ext, EX_MULIPLE, 1);
		} else if ( CMP_STR(key, NFL_SRC_MASK) ) {
			rec.src_mask = SvUV(sv);
			bit_array_set(&ext, EX_MULIPLE, 1);
		} else if ( CMP_STR(key, NFL_DST_MASK) ) {
			rec.dst_mask = SvUV(sv);
			bit_array_set(&ext, EX_MULIPLE, 1);

		// NEXT HOP
		} else if ( CMP_STR(key, NFL_IP_NEXTHOP)) {
			int res = Sv_addr(&rec.ip_nexthop, sv);
			switch (res) {
				case AF_INET:
					ClearFlag(rec.flags, FLAG_IPV6_NH);
					bit_array_set(&ext, EX_NEXT_HOP_v4, 1);
					bit_array_set(&ext, EX_NEXT_HOP_v6, 0);
					break;
				case AF_INET6:
					SetFlag(rec.flags, FLAG_IPV6_NH);
					bit_array_set(&ext, EX_NEXT_HOP_v4, 0);
					bit_array_set(&ext, EX_NEXT_HOP_v6, 1);
					break;
				default: 
					warn("%s invalid value for %s", NFL_LOG, NFL_IP_NEXTHOP);
					return 0;
			}
	
		// BGP NETX HOP	
		} else if ( CMP_STR(key, NFL_BGP_NEXTHOP)) {
			int res = Sv_addr(&rec.bgp_nexthop, sv);
			switch (res) {
				case AF_INET:
					ClearFlag(rec.flags, FLAG_IPV6_NHB);
					bit_array_set(&ext, EX_NEXT_HOP_BGP_v4, 1);
					bit_array_set(&ext, EX_NEXT_HOP_BGP_v6, 0);
					break;
				case AF_INET6:
					SetFlag(rec.flags, FLAG_IPV6_NHB);
					bit_array_set(&ext, EX_NEXT_HOP_BGP_v4, 0);
					bit_array_set(&ext, EX_NEXT_HOP_BGP_v6, 1);
					break;
				default: 
					warn("%s invalid value for %s", NFL_LOG, NFL_BGP_NEXTHOP);
					return 0;
			}

		// VLANS
		} else if ( CMP_STR(key, NFL_SRC_VLAN)) {
			rec.src_vlan = SvUV(sv);
			bit_array_set(&ext, EX_VLAN, 1);
		} else if ( CMP_STR(key, NFL_DST_VLAN)) {
			rec.dst_vlan = SvUV(sv);
			bit_array_set(&ext, EX_VLAN, 1);

		// OUTPUT CONTERS
		// EX_OUT_PKG_4, EX_OUT_BYTES_4 is nod used, we always use 32b. version
		} else if ( CMP_STR(key, NFL_OUT_PKTS)) {
			rec.out_pkts = SvU64(sv);
			bit_array_set(&ext, EX_OUT_PKG_8, 1);
		} else if ( CMP_STR(key, NFL_OUT_BYTES)) {
			rec.out_bytes = SvU64(sv);
			bit_array_set(&ext, EX_OUT_BYTES_8, 1);

		// MAC ADDRESSES
		} else if ( CMP_STR(key, NFL_IN_SRC_MAC) ) {
			if (Sv_mac(&rec.in_src_mac, sv) != 0) {
				warn("%s invalid MAC address %s (%s)", NFL_LOG, NFL_IN_SRC_MAC, SvPV(sv, svlen));
				return 0;
			}
			bit_array_set(&ext, EX_MAC_1, 1);
		} else if ( CMP_STR(key, NFL_OUT_DST_MAC) ) {
			if (Sv_mac(&rec.out_dst_mac, sv) != 0) {
				warn("%s invalid MAC address %s (%s)", NFL_LOG, NFL_OUT_DST_MAC, SvPV(sv, svlen));
				return 0;
			}
			bit_array_set(&ext, EX_MAC_1, 1);
		} else if ( CMP_STR(key, NFL_IN_DST_MAC) ) { 
			if (Sv_mac(&rec.in_dst_mac, sv) != 0) {
				warn("%s invalid MAC address %s (%s)", NFL_LOG, NFL_IN_DST_MAC, SvPV(sv, svlen));
				return 0;
			}
			bit_array_set(&ext, EX_MAC_2, 1);
		} else if ( CMP_STR(key, NFL_OUT_SRC_MAC) ) {
			if (Sv_mac(&rec.out_src_mac, sv) != 0) {
				warn("%s invalid MAC address %s (%s)", NFL_LOG, NFL_OUT_SRC_MAC, SvPV(sv, svlen));
				return 0;
			}
			bit_array_set(&ext, EX_MAC_2, 1);


		// MPLS LABELS
		} else if ( CMP_STR(key, NFL_MPLS_LABEL)) {
			STRLEN len;
			char *s;
			s = SvPV(sv, len);
			if ( len != sizeof(rec.mpls_label) ) {
				warn("%s Invalid size of %s field (size: %lu, expected: %lu)", NFL_LOG, 
								NFL_MPLS_LABEL, len, sizeof(rec.mpls_label));
				return 0;
			}
            memcpy(&rec.mpls_label, s, sizeof(rec.mpls_label));
			bit_array_set(&ext, EX_MPLS, 1);


		// ROUTER/EXPORTER INFORMATION
		} else if ( CMP_STR(key, NFL_IP_ROUTER)) {
			int res = Sv_addr(&rec.ip_router, sv);
			switch (res) {
				case AF_INET:
					ClearFlag(rec.flags, FLAG_IPV6_EXP);
					bit_array_set(&ext, EX_ROUTER_IP_v4, 1);
					bit_array_set(&ext, EX_ROUTER_IP_v6, 0);
					break;
				case AF_INET6:
					SetFlag(rec.flags, FLAG_IPV6_EXP);
					bit_array_set(&ext, EX_ROUTER_IP_v4, 0);
					bit_array_set(&ext, EX_ROUTER_IP_v6, 1);
					break;
				default: 
					warn("%s invalid notation for %s (%s)", NFL_LOG, NFL_IP_ROUTER, SvPV(sv, svlen));
					return 0;
			}
		} else if ( CMP_STR(key, NFL_ENGINE_TYPE) ) {
			rec.engine_type = SvUV(sv);
			bit_array_set(&ext, EX_ROUTER_ID, 1);
		} else if ( CMP_STR(key, NFL_ENGINE_ID) ) {
			rec.engine_id = SvUV(sv);
			bit_array_set(&ext, EX_ROUTER_ID, 1);
		 

		// nprobe extensions
		/* neot enabled yet because PackRecord function doesn't contain 
 		* the code for EX_LATENCY. The issue was send to P. Haah
 		* so the future version might solvethe problem 
		} else if ( CMP_STR(key, NFL_CLIENT_NW_DELAY_USEC) ) {
			rec.client_nw_delay_usec = SvU64(sv);
			bit_array_set(&ext, EX_LATENCY, 1);
		} else if ( CMP_STR(key, NFL_SERVER_NW_DELAY_USEC) ) {
			rec.server_nw_delay_usec = SvU64(sv);
			bit_array_set(&ext, EX_LATENCY, 1);
		} else if ( CMP_STR(key, NFL_APPL_LATENCY_USEC) ) {
			rec.appl_latency_usec = SvU64(sv);
			bit_array_set(&ext, EX_LATENCY, 1);
		*/

		// EX_RECEIVED
		} else if ( CMP_STR(key, NFL_RECEIVED) ) {
			rec.received = SvU64(sv);
			bit_array_set(&ext, EX_RECEIVED, 1);
		} 
		else {	
			warn("%s invalid item %s", NFL_LOG, key);
			return 0;
		}

//		SvREFCNT_dec(sv); 	/* free memory */
	}  // while 

//	printf("REFCNT: %d\n", SvREFCNT(hashref));
//	SvREFCNT_dec(hashref); 	/* free memory */

	map = libnf_lookup_map(instance, &ext);
	bit_array_release(&ext);

	rec.map_ref = map;
	rec.ext_map = map->map_id;
	rec.type = CommonRecordType;

/*
	{
		char *s;
		PrintExtensionMap(map);
		VerifyExtensionMap(map);
		format_file_block_record(&rec, &s, 0);
		printf("WRITE: (%p) \n%s\n", map, s);
	}
*/
	UpdateStat(instance->nffile_w->stat_record, &rec);

	PackRecord(&rec, instance->nffile_w);

	return 1;

}

void libnf_finish(int handle) {
libnf_instance_t *instance = libnf_instances[handle];

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return;
	}

	if (instance->nffile_w) {
		// write the last records in buffer
 		if ( instance->nffile_w->block_header->NumRecords ) {
			if ( WriteBlock(instance->nffile_w) <= 0 ) {
				fprintf(stderr, "Failed to write output buffer: '%s'" , strerror(errno));
			}
		}
//		CloseFile(instance->nffile_w);
		CloseUpdateFile(instance->nffile_w, NULL );
		DisposeFile(instance->nffile_w);
	}

	if (instance->nffile_r) {	
		CloseFile(instance->nffile_r);
		DisposeFile(instance->nffile_r);
	}

	//release list of extensions map
	// TODO 
	

	PackExtensionMapList(&instance->extension_map_list);
	FreeExtensionMaps(&instance->extension_map_list);
	free(instance); 
	libnf_instances[handle] = NULL;
	//return stat_record;
	return ;

} // End of process_data_finish


