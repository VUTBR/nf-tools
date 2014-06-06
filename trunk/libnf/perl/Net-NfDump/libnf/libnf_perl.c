
#define NEED_PACKRECORD 1 

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "libnf.h"
#include "libnf_perl.h"

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

/* defining macros for storing numbers, 64 bit numbers and strings into hash */
#define HV_STORE_NV(r,k,v) (void)hv_store(r, k, strlen(k), newSVnv(v), 0)
#define HV_STORE_U64V(r,k,v) (void)hv_store(r, k, strlen(k), newSVu64(v), 0)
#define HV_STORE_PV(r,k,v) (void)hv_store(r, k, strlen(k), newSVpvn(v, strlen(v)), 0)


/* hash parameters */
//#define NumPrealloc 128000

//#define AGGR_SIZE 7

//#define STRINGSIZE 10240
//static char data_string[STRINGSIZE];

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
	//extension_map_list_t 	extension_map_list;		/* nfdup structure containig extmap */
	extension_map_list_t 	*extension_map_list;		/* nfdup structure containig extmap */
	libnf_map_list_t		*map_list;				/* libnf structure that holds maps */
	int 					max_num_extensions;		/* mamimum number of extensions */
	libnf_file_list_t		*files;					/* list of files to read */
//	nffile_t				*nffile_r;				/* filehandle to the file that we read data from */
//	nffile_t				*nffile_w;				/* filehandle for writing */
	lnf_file_t				*lnf_nffile_r;			/* filehandle for reading */
	lnf_file_t				*lnf_nffile_w;			/* filehandle for wirting */
	int 					blk_record_remains; 	/* counter of processed rows in a signle block */
	FilterEngine_data_t		*engine;
	common_record_t			*flow_record;
	int						*field_list;
	int						field_last;
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
//	master_record_t			*master_record_r;		/* pointer to last read master record */
	master_record_t			master_record_w;		/* pointer to master record that will be stored */
	bit_array_t				ext_r;					/* extension bit array for read and write */
	bit_array_t				ext_w;
} libnf_instance_t;


/* array of initalized instances */
libnf_instance_t *libnf_instances[NFL_MAX_INSTANCES] = { NULL };

/* Global Variables */
extern extension_descriptor_t extension_descriptor[];

// compare at most 16 chars
#define MAXMODELEN	16	

#define STRINGSIZE 10240
#define IP_STRING_LEN (INET6_ADDRSTRLEN)

#include "nfdump_inline.c"
#include "nffile_inline.c"
//#include "nf_common.c"


/***********************************************************************
*                                                                      *
* functions and macros for converting data types to perl's SV and back *
*                                                                      *
************************************************************************/

/* cinverts unsigned integer 32b. to SV */
static inline SV * uint_to_SV(uint32_t n, int is_defined) {

	if (!is_defined) 
		return newSV(0);

	return newSVuv(n);
}

/* converts unsigned integer 64b. to SV */
static inline SV * uint64_to_SV(uint64_t n, int is_defined) {

	if (!is_defined) 
		return newSV(0);

	return newSVu64(n);
}

/* converts mpls array to SV */
static inline SV * mpls_to_SV(char *mpls, int is_defined) {

	if (!is_defined) 
		return newSV(0);

	return newSVpvn(mpls, sizeof(((struct master_record_s *)0)->mpls_label));
}

/* converts SV to MPLS string   */
/* returns 0 if conversion was not succesfull */
static inline int SV_to_mpls(char *a, SV * sv) {
STRLEN len;
char *s;

	s = SvPV(sv, len);

	if ( len != sizeof(((struct master_record_s *)0)->mpls_label) ) 
		return -1;

	memcpy(a, s, sizeof(((struct master_record_s *)0)->mpls_label) );
	
	return 0;
}


/* IPv4 or IPv6 address to SV */
static inline SV * ip_addr_to_SV(ip_addr_t *a, int is6, int is_defined) {
char s[IP_STRING_LEN];
int len = 0;

	if (!is_defined) 
		return newSV(0);

	if ( is6 ) { // IPv6
		uint64_t ip[2];

		ip[0] = htonll(a->v6[0]);
		ip[1] = htonll(a->v6[1]);
		len = sizeof(a->v6);
		memcpy(s, ip, len);
	} else {    // IPv4
		uint32_t ip;

		//ip = a->v4;
		ip = htonl(a->v4);
		len = sizeof(a->v4);
		memcpy(s, &ip, len);
	}

	return  newSVpvn(s, len);
}

/* converts SV to  IP addres (ip_addr_t) */
/* returns AF_INET or AF_INET6 based of the address type */
static inline int SV_to_ip_addr(ip_addr_t *a, SV * sv) {
uint64_t ip6[2];
uint32_t ip4;
char *s;
STRLEN len;

	s = SvPV(sv, len);

	if ( len == sizeof(ip4) )  {
		memcpy(&ip4, s, sizeof(ip4));
		//a->v4 = ip4;
		a->v4 = ntohl(ip4);
		return AF_INET;
	} else if ( len == sizeof(ip6) ) {
		memcpy(ip6, s, sizeof(ip6));
		a->v6[0] = ntohll(ip6[0]);
		a->v6[1] = ntohll(ip6[1]);
		return AF_INET6;
	} else {
		return -1;
	}
}

/* converts MAC address to SV */
static inline SV * mac_to_SV(uint8_t *a, int is_defined) {
char s[sizeof(uint64_t)];
int i;

	if (!is_defined) 
		return newSV(0);

	s[0] = 0;

	for ( i=0; i<6; i++ ) {
		s[5 - i] = a[i] & 0xFF;
    }

	return newSVpvn(s, 6);
}

/* converts SV to MAC addres and store to uint64_t   */
/* returns 0 if conversion was not succesfull */
static inline int SV_to_mac(uint64_t *a, SV * sv) {
uint8_t *mac = (uint8_t *)a;
char *s;
int i; 
STRLEN len;

	s = SvPV(sv, len);

	if ( len != 6 ) 
		return -1;
		
	for (i = 0; i < 6; i++) {
		mac[5 - i] = s[i];
	}
	
	mac[6] = 0x0;
	mac[7] = 0x0;

	return 0;
}

/*
************************************************************************
*                                                                      *
* end of convertion functions                                          *
*                                                                      *
************************************************************************
*/


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
		HV_STORE_U64V(res, "flows", nffile->stat_record->numflows);
		HV_STORE_U64V(res, "bytes", nffile->stat_record->numbytes);
		HV_STORE_U64V(res, "packets", nffile->stat_record->numpackets);
		
		HV_STORE_U64V(res, "flows_tcp", nffile->stat_record->numflows_tcp);
		HV_STORE_U64V(res, "bytes_tcp", nffile->stat_record->numbytes_tcp);
		HV_STORE_U64V(res, "packets_tcp", nffile->stat_record->numpackets_tcp);

		HV_STORE_U64V(res, "flows_udp", nffile->stat_record->numflows_udp);
		HV_STORE_U64V(res, "bytes_udp", nffile->stat_record->numbytes_udp);
		HV_STORE_U64V(res, "packets_udp", nffile->stat_record->numpackets_udp);

		HV_STORE_U64V(res, "flows_icmp", nffile->stat_record->numflows_icmp);
		HV_STORE_U64V(res, "bytes_icmp", nffile->stat_record->numbytes_icmp);
		HV_STORE_U64V(res, "packets_icmp", nffile->stat_record->numpackets_icmp);

		HV_STORE_U64V(res, "flows_other", nffile->stat_record->numflows_other);
		HV_STORE_U64V(res, "bytes_other", nffile->stat_record->numbytes_other);
		HV_STORE_U64V(res, "packets_other", nffile->stat_record->numpackets_other);

		HV_STORE_U64V(res, "first", nffile->stat_record->first_seen * 1000LL + nffile->stat_record->msec_first);
		HV_STORE_U64V(res, "last", nffile->stat_record->last_seen * 1000LL + nffile->stat_record->msec_last);
//		HV_STORE_NV(res, "msec_first", nffile->stat_record->msec_first);
//		HV_STORE_NV(res, "msec_last", nffile->stat_record->msec_last);

		HV_STORE_U64V(res, "sequence_failures", nffile->stat_record->sequence_failure);
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
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}

	res = (HV *)sv_2mortal((SV *)newHV());

	if ( instance->current_filename != NULL && instance->lnf_nffile_r->nffile != NULL ) {
		int nblocs = instance->lnf_nffile_r->nffile->file_header->NumBlocks;
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
//SV * libnf_master_record_to_AV(int handle, master_record_t *rec, extension_map_t *map) {
SV * libnf_master_record_to_AV(int handle, lnf_rec_t *lnf_rec) {
libnf_instance_t *instance = libnf_instances[handle];
AV *res_array;
master_record_t *rec = lnf_rec->master_record;
//extension_map_t *map = rec->map_ref;
//bit_array_t ext;
int i=0;
uint64_t t;
int ret;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}
	
	// processing map 
	/*

	bit_array_clear(&instance->ext_r);

	i = 0;
    while (map->ex_id[i]) {
		bit_array_set(&instance->ext_r, map->ex_id[i], 1);
		i++;
	}
	*/


	bit_array_copy(&instance->ext_r, lnf_rec->extensions_arr);

	res_array = (AV *)sv_2mortal((SV *)newAV());


	i = 0;
	while ( instance->field_list[i] ) {
		SV * sv;
		int field = instance->field_list[i];

		switch ( instance->field_list[i] ) { 
			case NFL_I_FIRST: 	
			case NFL_I_LAST: 	
			case NFL_I_RECEIVED:
			case NFL_I_DPKTS:
			case NFL_I_DOCTETS:
			case NFL_I_OUT_PKTS:
			case NFL_I_OUT_BYTES:
			case NFL_I_AGGR_FLOWS:
			case NFL_I_SRCPORT:
			case NFL_I_DSTPORT:
			case NFL_I_TCP_FLAGS: 	


		switch (LNF_GET_TYPE(field)) {
			case LNF_UINT8:
			case LNF_UINT16:
			case LNF_UINT32: {
                uint64_t t32 = 0;
                ret = lnf_item_get(lnf_rec, field, (void *)&t32);
				sv = uint_to_SV(t32, ret == LNF_OK);
                break;
            }
            case LNF_UINT64: {
                uint64_t t64 = 0;
                ret = lnf_item_get(lnf_rec, field, (void *)&t64);
				sv = uint64_to_SV(t64, ret == LNF_OK);
                break;
			}

		} /* switch */

			break;


			// Required extension 1 - IP addresses 
			// NOTE: srcaddr and dst addr do not uses ip_addr_t union/structure 
			// however the structures are compatible so we will pretend 
			// that v6.srcaddr and v6.dst addr points to same structure 
			case NFL_I_SRCADDR:
					sv = ip_addr_to_SV((ip_addr_t *)&rec->v6.srcaddr, 
						rec->flags & FLAG_IPV6_ADDR, 1);
					break;
			case NFL_I_DSTADDR:
					sv = ip_addr_to_SV((ip_addr_t *)&rec->v6.dstaddr, 
						rec->flags & FLAG_IPV6_ADDR, 1);
					break;
			case NFL_I_IP_NEXTHOP:
					sv = ip_addr_to_SV(&rec->ip_nexthop, rec->flags & FLAG_IPV6_NH,
						bit_array_get(&instance->ext_r, EX_NEXT_HOP_v4) ||
						bit_array_get(&instance->ext_r, EX_NEXT_HOP_v6) );
					break;
			case NFL_I_SRC_MASK:
					sv = uint_to_SV(rec->src_mask, 
						bit_array_get(&instance->ext_r, EX_MULIPLE) );
					break;
			case NFL_I_DST_MASK:
					sv = uint_to_SV(rec->dst_mask, 
						bit_array_get(&instance->ext_r, EX_MULIPLE) );
					break;

			case NFL_I_TOS:
					sv = uint_to_SV(rec->tos, 1);
					break;
			case NFL_I_DST_TOS:
					sv = uint_to_SV(rec->dst_tos, 
						bit_array_get(&instance->ext_r, EX_MULIPLE) );
					break;

			case NFL_I_SRCAS:
					sv = uint_to_SV(rec->srcas, 
						bit_array_get(&instance->ext_r, EX_AS_2) ||
						bit_array_get(&instance->ext_r, EX_AS_4) );
					break;
			case NFL_I_DSTAS:
					sv = uint_to_SV(rec->dstas, 
						bit_array_get(&instance->ext_r, EX_AS_2) ||
						bit_array_get(&instance->ext_r, EX_AS_4) );
					break;

			case NFL_I_BGPNEXTADJACENTAS:
					sv = uint_to_SV(rec->bgpNextAdjacentAS, 
						bit_array_get(&instance->ext_r, EX_BGPADJ) );
					break;
			case NFL_I_BGPPREVADJACENTAS:
					sv = uint_to_SV(rec->bgpPrevAdjacentAS, 
						bit_array_get(&instance->ext_r, EX_BGPADJ) );
					break;
			case NFL_I_BGP_NEXTHOP:
					sv = ip_addr_to_SV(&rec->bgp_nexthop, rec->flags & FLAG_IPV6_NHB,
						bit_array_get(&instance->ext_r, EX_NEXT_HOP_BGP_v4) ||
						bit_array_get(&instance->ext_r, EX_NEXT_HOP_BGP_v6) );
					break;

			case NFL_I_PROT: 	
					sv = uint_to_SV(rec->prot, 1);
					break;

			case NFL_I_SRC_VLAN:
					sv = uint_to_SV(rec->src_vlan, 
						bit_array_get(&instance->ext_r, EX_VLAN) );
					break;
			case NFL_I_DST_VLAN:
					sv = uint_to_SV(rec->dst_vlan, 
						bit_array_get(&instance->ext_r, EX_VLAN) );
					break;

			case NFL_I_IN_SRC_MAC:
					sv = mac_to_SV((u_int8_t *)&rec->in_src_mac,
						bit_array_get(&instance->ext_r, EX_MAC_1) );
					break;
			case NFL_I_OUT_DST_MAC:
					sv = mac_to_SV((u_int8_t *)&rec->out_dst_mac,
						bit_array_get(&instance->ext_r, EX_MAC_1) );
					break;
			case NFL_I_OUT_SRC_MAC:
					sv = mac_to_SV((u_int8_t *)&rec->out_src_mac,
						bit_array_get(&instance->ext_r, EX_MAC_2) );
					break;
			case NFL_I_IN_DST_MAC:
					sv = mac_to_SV((u_int8_t *)&rec->in_dst_mac,
						bit_array_get(&instance->ext_r, EX_MAC_2) );
					break;
 
			case NFL_I_MPLS_LABEL:
					sv = mpls_to_SV((char *)&rec->mpls_label, 
						bit_array_get(&instance->ext_r, EX_MPLS) );
					break;

			case NFL_I_INPUT:
					sv = uint_to_SV(rec->input, 
						bit_array_get(&instance->ext_r, EX_IO_SNMP_2) ||
						bit_array_get(&instance->ext_r, EX_IO_SNMP_4) );
					break;
			case NFL_I_OUTPUT:
					sv = uint_to_SV(rec->output, 
						bit_array_get(&instance->ext_r, EX_IO_SNMP_2) ||
						bit_array_get(&instance->ext_r, EX_IO_SNMP_4) );

					break;
			case NFL_I_DIR:
					sv = uint_to_SV(rec->dir, 
						bit_array_get(&instance->ext_r, EX_MULIPLE) );
					break;

			case NFL_I_FWD_STATUS:
					sv = uint_to_SV(rec->fwd_status, 1);
					break;


			case NFL_I_IP_ROUTER:
					sv = ip_addr_to_SV(&rec->ip_router, rec->flags & FLAG_IPV6_EXP,
						bit_array_get(&instance->ext_r, EX_ROUTER_IP_v4) ||
						bit_array_get(&instance->ext_r, EX_ROUTER_IP_v6) );
					break;
			case NFL_I_ENGINE_TYPE:
					sv = uint_to_SV(rec->engine_type, 
						bit_array_get(&instance->ext_r, EX_ROUTER_ID) );
					break;
			case NFL_I_ENGINE_ID:
					sv = uint_to_SV(rec->engine_id, 
						bit_array_get(&instance->ext_r, EX_ROUTER_ID) );
					break;

			// NSEL 
#ifdef NSEL
			case NFL_I_EVENT_TIME:
					sv = uint64_to_SV(rec->event_time, 
						bit_array_get(&instance->ext_r, EX_NSEL_COMMON) );
					break;
			case NFL_I_CONN_ID:
					sv = uint_to_SV(rec->conn_id, 
						bit_array_get(&instance->ext_r, EX_NSEL_COMMON) );
					break;
			case NFL_I_ICMP_CODE:
					sv = uint_to_SV(rec->icmp_code, 
						bit_array_get(&instance->ext_r, EX_NSEL_COMMON) );
					break;
			case NFL_I_ICMP_TYPE:
					sv = uint_to_SV(rec->icmp_type, 
						bit_array_get(&instance->ext_r, EX_NSEL_COMMON) );
					break;
			case NFL_I_FW_XEVENT:
					sv = uint_to_SV(rec->fw_xevent, 
						bit_array_get(&instance->ext_r, EX_NSEL_COMMON) );
					break;
			case NFL_I_XLATE_SRC_IP:
					sv = ip_addr_to_SV(&rec->xlate_src_ip, rec->xlate_flags,
						bit_array_get(&instance->ext_r, EX_NSEL_XLATE_IP_v4) ||
						bit_array_get(&instance->ext_r, EX_NSEL_XLATE_IP_v6) );
					break;
			case NFL_I_XLATE_DST_IP:
					sv = ip_addr_to_SV(&rec->xlate_dst_ip, rec->xlate_flags,
						bit_array_get(&instance->ext_r, EX_NSEL_XLATE_IP_v4) ||
						bit_array_get(&instance->ext_r, EX_NSEL_XLATE_IP_v6) );
					break;
			case NFL_I_XLATE_SRC_PORT:
					sv = uint_to_SV(rec->xlate_src_port, 
						bit_array_get(&instance->ext_r, EX_NSEL_XLATE_PORTS) );
					break;
			case NFL_I_XLATE_DST_PORT:
					sv = uint_to_SV(rec->xlate_dst_port, 
						bit_array_get(&instance->ext_r, EX_NSEL_XLATE_PORTS) );
					break;
			case NFL_I_INGRESS_ACL_ID:
					sv = uint_to_SV(rec->ingress_acl_id[0], 
						bit_array_get(&instance->ext_r, EX_NSEL_ACL) );
					break;
			case NFL_I_INGRESS_ACE_ID:
					sv = uint_to_SV(rec->ingress_acl_id[1], 
						bit_array_get(&instance->ext_r, EX_NSEL_ACL) );
					break;
			case NFL_I_INGRESS_XACE_ID:
					sv = uint_to_SV(rec->ingress_acl_id[2], 
						bit_array_get(&instance->ext_r, EX_NSEL_ACL) );
					break;
			case NFL_I_EGRESS_ACL_ID:
					sv = uint_to_SV(rec->egress_acl_id[0], 
						bit_array_get(&instance->ext_r, EX_NSEL_ACL) );
					break;
			case NFL_I_EGRESS_ACE_ID:
					sv = uint_to_SV(rec->egress_acl_id[1], 
						bit_array_get(&instance->ext_r, EX_NSEL_ACL) );
					break;
			case NFL_I_EGRESS_XACE_ID:
					sv = uint_to_SV(rec->egress_acl_id[2], 
						bit_array_get(&instance->ext_r, EX_NSEL_ACL) );
					break;
			case NFL_I_USERNAME:
					if ( bit_array_get(&instance->ext_r, EX_NSEL_USER) ||
	 					 bit_array_get(&instance->ext_r, EX_NSEL_USER_MAX ) )  {
						sv = newSVpvn(rec->username, strlen(rec->username));
					} else {
						sv =  newSV(0);
					}
					break;

			// END OF NSEL 
		
			// NEL support
			case NFL_I_INGRESS_VRFID:
					sv = uint_to_SV(rec->ingress_vrfid, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;
			case NFL_I_EVENT_FLAG:
					sv = uint_to_SV(rec->event_flag, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;
			case NFL_I_EGRESS_VRFID:
					sv = uint_to_SV(rec->egress_vrfid, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;

			// END OF NEL 
#endif // NEL 

			// EX_PORT_BLOCK_ALLOC - added 2014-04-19
			case NFL_I_BLOCK_START:
					sv = uint_to_SV(rec->block_start, 
						bit_array_get(&instance->ext_r, EX_PORT_BLOCK_ALLOC) );
					break;
			case NFL_I_BLOCK_END:
					sv = uint_to_SV(rec->block_end, 
						bit_array_get(&instance->ext_r, EX_PORT_BLOCK_ALLOC) );
					break;
			case NFL_I_BLOCK_STEP:
					sv = uint_to_SV(rec->block_step, 
						bit_array_get(&instance->ext_r, EX_PORT_BLOCK_ALLOC) );
					break;
			case NFL_I_BLOCK_SIZE:
					sv = uint_to_SV(rec->block_size, 
						bit_array_get(&instance->ext_r, EX_PORT_BLOCK_ALLOC) );
					break;

			// extra fileds
			case NFL_I_CLIENT_NW_DELAY_USEC:
					sv = uint64_to_SV(rec->client_nw_delay_usec, 
						bit_array_get(&instance->ext_r, EX_LATENCY) );
					break;
			case NFL_I_SERVER_NW_DELAY_USEC:
					sv = uint64_to_SV(rec->server_nw_delay_usec, 
						bit_array_get(&instance->ext_r, EX_LATENCY) );
					break;
			case NFL_I_APPL_LATENCY_USEC:
					sv = uint64_to_SV(rec->appl_latency_usec, 
						bit_array_get(&instance->ext_r, EX_LATENCY) );
					break;

			default:
					croak("%s Unknown ID in %s !!", NFL_LOG, __FUNCTION__);
					break;
		}

		i++;
		av_push(res_array, sv);	
	}

 
	return newRV((SV *)res_array);
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


	//Insert_Extension_Map(&instance->extension_map_list, map); 
	Insert_Extension_Map(instance->extension_map_list, map); 
	AppendToBuffer(instance->lnf_nffile_w->nffile, (void *)map, map->size);

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
			croak("%s no free handles available, max instances %d reached", NFL_LOG, NFL_MAX_INSTANCES);
			return 0;	
		}
	}

	instance = malloc(sizeof(libnf_instance_t));
	memset(instance, 0, sizeof(libnf_instance_t));

	if (instance == NULL) {
		croak("%s can not allocate memory for instance:", NFL_LOG );
		return 0;
	}

	instance->map_list = NULL;

	libnf_instances[handle] = instance;

//	InitExtensionMaps(&(instance->extension_map_list));
	instance->extension_map_list = InitExtensionMaps(NEEDS_EXTENSION_LIST);
	i = 1;
	instance->max_num_extensions = 0;
	while ( extension_descriptor[i++].id )
		instance->max_num_extensions++;

	bit_array_init(&instance->ext_r, instance->max_num_extensions + 1);
	bit_array_init(&instance->ext_w, instance->max_num_extensions + 1);

	instance->lnf_nffile_w = NULL;
//	instance->master_record_r = NULL;

	return handle;
}


int libnf_set_fields(int handle, SV *fields) {
libnf_instance_t *instance = libnf_instances[handle];
I32 last_field = 0;
int i;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}

	if ((!SvROK(fields))
		|| (SvTYPE(SvRV(fields)) != SVt_PVAV) 
		|| ((last_field = av_len((AV *)SvRV(fields))) < 0)) {
			croak("%s can not determine the list of fields", NFL_LOG);
			return 0;
	}

	// release memory allocated before	
	if (instance->field_list != NULL) {
		free(instance->field_list);
	}

	// last_field contains the highet index of array ! - not number of items 
	instance->field_list = malloc(sizeof(int) * (last_field + 2));

	if (instance->field_list == NULL) {
		croak("%s can not allocate memory in %s", NFL_LOG, __FUNCTION__);
		return 0;
	}

	for (i = 0; i <= last_field; i++) {
		int field = SvIV(*av_fetch((AV *)SvRV(fields), i, 0));

		if (field != 0 || field > NFL_MAX_FIELDS) {	
			instance->field_list[i] = field;
		} else {
			warn("%s ivalid itemd ID", NFL_LOG);
		}
	}
	instance->field_list[i++] = NFL_ZERO_FIELD;
	instance->field_last = last_field;
	return 1;
}


int libnf_read_files(int handle, char *filter, int window_start, int window_end, SV *files) {
libnf_instance_t *instance = libnf_instances[handle];

libnf_file_list_t	*pfile;
I32 numfiles = 0;
int i;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
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
	instance->twin_start = window_start;
	instance->twin_end = window_end;

	for (i = 0; i <= numfiles; i++) {
		STRLEN l;
		char * file = SvPV(*av_fetch((AV *)SvRV(files), i, 0), l);
		

		pfile->filename = malloc(l + 1);
		strcpy((char *)(pfile->filename), file);
		
		pfile->next = malloc(sizeof(libnf_file_list_t));
		pfile = pfile->next;
		pfile->filename = NULL;

	}
//	instance->nffile_r = NULL;
	instance->lnf_nffile_r = NULL;

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
int flags = 0;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}


	/* the file was already opened */
	if (instance->lnf_nffile_w != NULL) {
		croak("%s file handler was opened before", NFL_LOG);
		return 0;
	}

	/* writing file */
	flags |= LNF_WRITE;
	flags |= compressed ? LNF_COMP  : 0x0;
	flags |= anonymized ? LNF_ANON  : 0x0;

    instance->lnf_nffile_w = lnf_open(filename, flags , ident);
//    instance->nffile_w = OpenNewFile(filename, NULL, compressed, anonymized, ident);
    if ( !instance->lnf_nffile_w ) {
		warn("%s cannot open file %s", NFL_LOG, filename);
		return 0;
    }

	memzero(&instance->master_record_w, sizeof(master_record_t));	// clean rec for write row 
	return 1;
}

/* returns hashref or NULL if we are et the end of the file */
SV * libnf_read_row(int handle) {
//master_record_t	*master_record;
libnf_instance_t *instance = libnf_instances[handle];
int ret;
int match;
//uint32_t map_id;
lnf_rec_t lnf_rec;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}

#ifdef COMPAT15
int	v1_map_done = 0;
#endif

begin:
		// get next data block from file
		if (instance->lnf_nffile_r) {
			ret = lnf_read(instance->lnf_nffile_r, &lnf_rec);
		} else {
			ret = NF_EOF;		/* the firt file in the list */
		}

		switch (ret) {
			case LNF_ERR_CORRUPT:
				LogError("Skip corrupt data file '%s'\n",GetCurrentFilename());
				exit(1);
			case LNF_ERR_READ:
				LogError("Read error in file '%s': %s\n",GetCurrentFilename(), strerror(errno) );
				exit(1);
				// fall through - get next file in chain
			case LNF_EOF: {
				libnf_file_list_t *next;

				//CloseFile(instance->nffile_r);
				lnf_close(instance->lnf_nffile_r);
				instance->lnf_nffile_r = NULL;
				if (instance->files->filename == NULL) {	// the end of the list 
					free(instance->files);
					instance->files = NULL;		
					return NULL;
				}
				//instance->nffile_r = OpenFile((char *)instance->files->filename, instance->nffile_r);
				instance->lnf_nffile_r = lnf_open((char *)instance->files->filename, LNF_READ, NULL);
				instance->processed_files++;

				next = instance->files->next;

				/* prepare instance->files to nex unread file */
				if (instance->current_filename != NULL) {
					free(instance->current_filename);
				}

				instance->current_filename = instance->files->filename;
				free(instance->files);
				instance->files = next;

				if ( instance->lnf_nffile_r == NULL ) {
					croak("%s can not read file %s", NFL_LOG, instance->files->filename);
					return NULL;
				}
				goto begin;
			}

			default:
				// successfully read block
				instance->processed_bytes += ret;
	}

	// Time based filter
	// if no time filter is given, the result is always true
	match  = instance->twin_start && (lnf_rec.master_record->first < instance->twin_start || 
						lnf_rec.master_record->last > instance->twin_end) ? 0 : 1;

	// filter netflow record with user supplied filter
	instance->engine->nfrecord = (uint64_t *)lnf_rec.master_record;
	if ( match ) 
		match = (*instance->engine->FilterEngine)(instance->engine);

	if ( match == 0 ) { // record failed to pass all filters
		goto begin;
	}

	/* the record seems OK. We prepare hash reference with items */
	//return libnf_master_record_to_AV(handle, lnf_rec.master_record, lnf_rec.master_record->map_ref); 
	return libnf_master_record_to_AV(handle, &lnf_rec); 

} /* end of _next fnction */
                                  

/* copy row from the instance defined as the source handle to destination */
int libnf_copy_row(int handle, int src_handle) {
libnf_instance_t *instance = libnf_instances[handle];
libnf_instance_t *src_instance = libnf_instances[src_handle];

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}

	if (src_instance == NULL ) {
		croak("%s seource handler %d not initialized", NFL_LOG, handle);
		return 0;
	}

	memcpy(&instance->master_record_w, src_instance->lnf_nffile_r->master_record, sizeof(master_record_t));
	bit_array_copy(&instance->ext_w, &src_instance->ext_r);

	return 1;

}

/* TAG for check_items_map.pl: libnf_write_row */
int libnf_write_row(int handle, SV * arrayref) {
master_record_t *rec;
libnf_instance_t *instance = libnf_instances[handle];
//extension_map_t *map;
//bit_array_t ext;
int last_field;
int i;
//res;
lnf_ip_t tip;

int field, ret;
lnf_rec_t lnf_rec;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return 0;
	}

	if ((!SvROK(arrayref))
		|| (SvTYPE(SvRV(arrayref)) != SVt_PVAV) 
		|| ((last_field = av_len((AV *)SvRV(arrayref))) < 0)) {
			croak("%s can not determine fields to store", NFL_LOG);
			return 0;
	}


	if (last_field != instance->field_last) {
		croak("%s number of fields do not match", NFL_LOG);
		return 0;
	}

	rec = &instance->master_record_w;

	// rec + array
	lnf_rec.master_record = rec;
	lnf_rec.extensions_arr = &instance->ext_w;


	i = 0;
	while ( instance->field_list[i] ) {

		SV * sv = (SV *)(*av_fetch((AV *)SvRV(arrayref), i, 0));

		if (!SvOK(sv)) {	// undef value 
			i++;
			continue;
		}

		field = instance->field_list[i];


		switch (LNF_GET_TYPE(field)) {
			case LNF_UINT8:
			case LNF_UINT16:
			case LNF_UINT32: {
				uint32_t t32 = SvUV(sv);
				ret = lnf_item_set(&lnf_rec, field, (void *)&t32);
				break;
			}
			case LNF_UINT64: {
				uint64_t t64 = SvU64(sv);
				ret = lnf_item_set(&lnf_rec, field, (void *)&t64);
				break;
			}
			case LNF_ADDR: {
				char *s;
				STRLEN len;

			    s = SvPV(sv, len);

			    if ( len == sizeof(tip.data[3]) )  {
			        memset(&tip, 0x0, sizeof(tip));
			        memcpy(&tip.data[3], s, sizeof(tip.data[3]));
				} else if (len == sizeof(tip) ) {
			        memcpy(&tip, s, sizeof(tip));
				} else {
					warn("%s invalid IP address value for %d", NFL_LOG, field);
					return 0;
				}

				ret = lnf_item_set(&lnf_rec, field, (void *)&tip);
				break;
			}
			case LNF_MAC: {
				char *s;
				STRLEN len;

				s = SvPV(sv, len);

				if ( len != sizeof(lnf_mac_t) ) {
					warn("%s invalid MAC address value for %d", NFL_LOG, field);
					return 0;
				}

				ret = lnf_item_set(&lnf_rec, field, (void *)s);
				break;
			}

			case LNF_MPLS: {
				char *s;
				STRLEN len;

				s = SvPV(sv, len);

				if ( len != sizeof(lnf_mpls_t) ) {
					warn("%s invalid MPLS stack value for %d", NFL_LOG, field);
					return 0;
				}

				ret = lnf_item_set(&lnf_rec, field, (void *)s);
				break;
			}

			case LNF_STRING: {
				char buf[1024];
				STRLEN len;
				char *s;

				s = SvPV(sv, len);	
				if (len > sizeof(buf) - 1) {
					len = sizeof(buf) - 1;
				}
				memcpy(buf, s, len);
				buf[len] = '\0';

				ret = lnf_item_set(&lnf_rec, field, (void *)buf);
				break;
			}

			default:
				croak("%s Unknown ID (%d) in %s !!", NFL_LOG, field, __FUNCTION__);
		}
		if (ret != LNF_OK) {
			croak("%s Error when processing field %d, error code: %d !!", NFL_LOG, field, ret);
		}
		i++;
	}

	lnf_write(instance->lnf_nffile_w, &lnf_rec);

	bit_array_clear(&instance->ext_w);
	memzero(rec, sizeof(master_record_t));	// clean rec for next row 

	return 1;
}

void libnf_finish(int handle) {
libnf_instance_t *instance = libnf_instances[handle];

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG, handle);
		return;
	}

	if (instance->lnf_nffile_w) {
		lnf_close(instance->lnf_nffile_w);
		instance->lnf_nffile_w = NULL;
	}

	if (instance->lnf_nffile_r) {	
		lnf_close(instance->lnf_nffile_r);
		instance->lnf_nffile_r = NULL;
	}

	//release list of extensions map
	// TODO 
	
	bit_array_release(&instance->ext_r);
	bit_array_release(&instance->ext_w);

/*
	PackExtensionMapList(instance->extension_map_list);
	FreeExtensionMaps(instance->extension_map_list);
*/
	free(instance); 
	libnf_instances[handle] = NULL;
	//return stat_record;
	return ;

} // End of process_data_finish


