
#define NEED_PACKRECORD 1 

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
	nffile_t				*nffile_r;				/* filehandle to the file that we read data from */
	nffile_t				*nffile_w;				/* filehandle for writing */
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
	master_record_t			*master_record_r;		/* pointer to last read master record */
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
SV * libnf_master_record_to_AV(int handle, master_record_t *rec, extension_map_t *map) {
libnf_instance_t *instance = libnf_instances[handle];
AV *res_array;
//bit_array_t ext;
int i=0;
uint64_t t;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}

	// processing map 
	bit_array_clear(&instance->ext_r);

	i = 0;
    while (map->ex_id[i]) {
		bit_array_set(&instance->ext_r, map->ex_id[i], 1);
		i++;
	}

	res_array = (AV *)sv_2mortal((SV *)newAV());

	i = 0;
	while ( instance->field_list[i] ) {
		SV * sv;

		switch ( instance->field_list[i] ) { 
			case NFL_I_FIRST: 	
					t = rec->first * 1000LL + rec->msec_first;
					sv = uint64_to_SV(t, 1);
					break;
			case NFL_I_LAST: 	
					t = rec->last * 1000LL + rec->msec_last;
					sv = uint64_to_SV(t, 1);
					break;

			case NFL_I_RECEIVED:
					sv = uint64_to_SV(rec->received, 
						bit_array_get(&instance->ext_r, EX_RECEIVED) );
					break;

			case NFL_I_DPKTS:
					sv = uint64_to_SV(rec->dPkts, 1);
					break;
			case NFL_I_DOCTETS:
					sv = uint64_to_SV(rec->dOctets, 1);
					break;

			case NFL_I_OUT_PKTS:
					sv = uint64_to_SV(rec->out_pkts, 
						bit_array_get(&instance->ext_r, EX_OUT_PKG_4) ||
						bit_array_get(&instance->ext_r, EX_OUT_PKG_8) );
					break;
			case NFL_I_OUT_BYTES:
					sv = uint64_to_SV(rec->out_bytes, 
						bit_array_get(&instance->ext_r, EX_OUT_BYTES_4) ||
						bit_array_get(&instance->ext_r, EX_OUT_BYTES_8) );
					break;
			case NFL_I_AGGR_FLOWS:
					sv = uint64_to_SV(rec->aggr_flows, 
						bit_array_get(&instance->ext_r, EX_AGGR_FLOWS_4) ||
						bit_array_get(&instance->ext_r, EX_AGGR_FLOWS_8) );
					break;

			case NFL_I_SRCPORT:
					sv = uint_to_SV(rec->srcport, 1);
					break;
			case NFL_I_DSTPORT:
					sv = uint_to_SV(rec->dstport, 1);
					break;
			case NFL_I_TCP_FLAGS: 	
					sv = uint_to_SV(rec->tcp_flags, 1);
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
			case NFL_I_FLOW_START:
					sv = uint64_to_SV(rec->flow_start, 
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
			case NFL_I_FW_EVENT:
					sv = uint_to_SV(rec->fw_event, 
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
#endif // 

			// END OF NSEL 
		
#ifdef NEL	
			// NEL support
			case NFL_I_NAT_EVENT:
					sv = uint_to_SV(rec->nat_event, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;
			case NFL_I_POST_SRC_PORT:
					sv = uint_to_SV(rec->post_src_port, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;
			case NFL_I_POST_DST_PORT:
					sv = uint_to_SV(rec->post_dst_port, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;
			case NFL_I_INGRESS_VRFID:
					sv = uint_to_SV(rec->ingress_vrfid, 
						bit_array_get(&instance->ext_r, EX_NEL_COMMON) );
					break;
			case NFL_I_NAT_INSIDE:
					sv = ip_addr_to_SV(&rec->nat_inside, rec->nat_flags,
						bit_array_get(&instance->ext_r, EX_NEL_GLOBAL_IP_v4) ||
						bit_array_get(&instance->ext_r, EX_NEL_GLOBAL_IP_v6) );
					break;
			case NFL_I_NAT_OUTSIDE:
					sv = ip_addr_to_SV(&rec->nat_outside, rec->nat_flags,
						bit_array_get(&instance->ext_r, EX_NEL_GLOBAL_IP_v4) ||
						bit_array_get(&instance->ext_r, EX_NEL_GLOBAL_IP_v6) );
					break;

			// END OF NEL 
#endif // NEL 

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

//	InitExtensionMaps(&(instance->extension_map_list));
	instance->extension_map_list = InitExtensionMaps(NEEDS_EXTENSION_LIST);
	i = 1;
	instance->max_num_extensions = 0;
	while ( extension_descriptor[i++].id )
		instance->max_num_extensions++;

	bit_array_init(&instance->ext_r, instance->max_num_extensions + 1);
	bit_array_init(&instance->ext_w, instance->max_num_extensions + 1);

	instance->nffile_w = NULL;
	instance->master_record_r = NULL;

	return handle;
}


int libnf_set_fields(int handle, SV *fields) {
libnf_instance_t *instance = libnf_instances[handle];
I32 last_field = 0;
int i;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
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

	memzero(&instance->master_record_w, sizeof(master_record_t));	// clean rec for write row 
	return 1;
}

                                  
/* returns hashref or NULL if we are et the end of the file */
SV * libnf_read_row(int handle) {
//master_record_t	*master_record;
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

		switch (instance->nffile_r->block_header->id) {
			case Large_BLOCK_Type:
					goto begin;
					break;
			case ExporterRecordType:
					goto begin;
					break;
			case SamplerRecordype:
					goto begin;
					break;
			case ExporterInfoRecordType:
					goto begin;
					break;
			case ExporterStatRecordType:
					goto begin;
					break;
			case SamplerInfoRecordype:
					goto begin;
					break;
		}

		if ( instance->nffile_r->block_header->id == Large_BLOCK_Type ) {
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
	instance->blk_record_remains--;

	if ( instance->flow_record->type == ExtensionMapType ) {
		extension_map_t *map = (extension_map_t *)instance->flow_record;
		//Insert_Extension_Map(&instance->extension_map_list, map);
		Insert_Extension_Map(instance->extension_map_list, map);

		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;

	} else if ( instance->flow_record->type != CommonRecordType ) {
		warn("%s Skip unknown record type %i\n", NFL_LOG, instance->flow_record->type);
		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;
	}

	/* we are sure that record is CommonRecordType */

	map_id = instance->flow_record->ext_map;
	if ( map_id >= MAX_EXTENSION_MAPS ) {
		croak("%s Corrupt data file. Extension map id %u too big.\n", NFL_LOG, instance->flow_record->ext_map);
		return 0;
	}
	if ( instance->extension_map_list->slot[map_id] == NULL ) {
		warn("%s Corrupt data file. Missing extension map %u. Skip record.\n", NFL_LOG, instance->flow_record->ext_map);
		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;
	} 

	instance->processed_records++;
	instance->master_record_r = &(instance->extension_map_list->slot[map_id]->master_record);
	instance->engine->nfrecord = (uint64_t *)instance->master_record_r;

	// changed in 1.6.8 - added exporter info 
//	ExpandRecord_v2( flow_record, extension_map_list.slot[map_id], master_record);
	ExpandRecord_v2( instance->flow_record, instance->extension_map_list->slot[map_id], NULL, instance->master_record_r);

	// Time based filter
	// if no time filter is given, the result is always true
	match  = instance->twin_start && (instance->master_record_r->first < instance->twin_start || 
						instance->master_record_r->last > instance->twin_end) ? 0 : 1;

	// filter netflow record with user supplied filter
	if ( match ) 
		match = (*instance->engine->FilterEngine)(instance->engine);

	if ( match == 0 ) { // record failed to pass all filters
		// increment pointer by number of bytes for netflow record
		instance->flow_record = (common_record_t *)((pointer_addr_t)instance->flow_record + instance->flow_record->size);	
		goto begin;
	}

	// update number of flows matching a given map
	instance->extension_map_list->slot[map_id]->ref_count++;

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
	return libnf_master_record_to_AV(handle, instance->master_record_r, instance->extension_map_list->slot[map_id]->map); 

} /* end of _next fnction */

/* copy row from the instance defined as the source handle to destination */
int libnf_copy_row(int handle, int src_handle) {
libnf_instance_t *instance = libnf_instances[handle];
libnf_instance_t *src_instance = libnf_instances[src_handle];

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
		return 0;
	}

	if (src_instance == NULL ) {
		croak("%s seource handler %d not initialized", NFL_LOG);
		return 0;
	}

	memcpy(&instance->master_record_w, src_instance->master_record_r, sizeof(master_record_t));
	bit_array_copy(&instance->ext_w, &src_instance->ext_r);

	return 1;

}

/* TAG for check_items_map.pl: libnf_write_row */
int libnf_write_row(int handle, SV * arrayref) {
master_record_t *rec;
libnf_instance_t *instance = libnf_instances[handle];
extension_map_t *map;
//bit_array_t ext;
int last_field;
int i, res;
uint64_t t;

	if (instance == NULL ) {
		croak("%s handler %d not initialized", NFL_LOG);
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

	i = 0;
	while ( instance->field_list[i] ) {

		SV * sv = (SV *)(*av_fetch((AV *)SvRV(arrayref), i, 0));

		if (!SvOK(sv)) {	// undef value 
			i++;
			continue;
		}

		switch ( instance->field_list[i] ) { 
			case NFL_I_FIRST: 	
					t = SvU64(sv);
					rec->first = t / 1000LL;
					rec->msec_first = t - rec->first * 1000LL;
					break;
			case NFL_I_LAST: 	
					t = SvU64(sv);
					rec->last = t / 1000LL;
					rec->msec_last = t - rec->last * 1000LL;
					break;

			case NFL_I_RECEIVED:
					rec->received = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_RECEIVED, 1);
					break;

			case NFL_I_DPKTS:
					rec->dPkts = SvU64(sv);
					break;
			case NFL_I_DOCTETS:
					rec->dOctets = SvU64(sv);
					break;

			// EX_OUT_PKG_4 not used 
			case NFL_I_OUT_PKTS:
					rec->out_pkts = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_OUT_PKG_8, 1);
					break;
			// EX_OUT_BYTES_4 not used 
			case NFL_I_OUT_BYTES:
					rec->out_bytes = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_OUT_BYTES_8, 1);
					break;
			// EX_AGGR_FLOWS_4 not used 
			case NFL_I_AGGR_FLOWS:
					rec->aggr_flows = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_AGGR_FLOWS_8, 1);
					break;

			case NFL_I_SRCPORT:
					rec->srcport = SvUV(sv);
					break;
			case NFL_I_DSTPORT:
					rec->dstport = SvUV(sv);
					break;
			case NFL_I_TCP_FLAGS: 	
					rec->tcp_flags = SvUV(sv);
					break;

			// Required extension 1 - IP addresses 
			// NOTE: srcaddr and dst addr do not uses ip_addr_t union/structure 
			// however the structures are compatible so we will pretend 
			// that v6.srcaddr and v6.dst addr points to same structure 
			case NFL_I_SRCADDR: 
					res = SV_to_ip_addr((ip_addr_t *)&rec->v6.srcaddr, sv);
					switch (res) {
						case AF_INET:
							ClearFlag(rec->flags, FLAG_IPV6_ADDR);
							break;
					case AF_INET6:
							SetFlag(rec->flags, FLAG_IPV6_ADDR);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_SRCADDR);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_DSTADDR:
					res = SV_to_ip_addr((ip_addr_t *)&rec->v6.dstaddr, sv);
					switch (res) {
						case AF_INET:
							ClearFlag(rec->flags, FLAG_IPV6_ADDR);
							break;
					case AF_INET6:
							SetFlag(rec->flags, FLAG_IPV6_ADDR);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_DSTADDR);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_IP_NEXTHOP:
					res = SV_to_ip_addr((ip_addr_t *)&rec->ip_nexthop, sv);
					switch (res) {
						case AF_INET:
							ClearFlag(rec->flags, FLAG_IPV6_NH);
							bit_array_set(&instance->ext_w, EX_NEXT_HOP_v4, 1);
							break;
					case AF_INET6:
							SetFlag(rec->flags, FLAG_IPV6_NH);
							bit_array_set(&instance->ext_w, EX_NEXT_HOP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_IP_NEXTHOP);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_SRC_MASK:
					rec->src_mask = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_MULIPLE, 1);
					break;
			case NFL_I_DST_MASK:
					rec->dst_mask = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_MULIPLE, 1);
					break;

			case NFL_I_TOS:
					rec->tos = SvUV(sv);
					break;
			case NFL_I_DST_TOS:
					rec->dst_tos = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_MULIPLE, 1);
					break;

			// EX_AS_2 not used 
			case NFL_I_SRCAS:
					rec->srcas = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_AS_4, 1);
					break;
			case NFL_I_DSTAS:
					rec->dstas = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_AS_4, 1);
					break;

			case NFL_I_BGPNEXTADJACENTAS:
					rec->bgpNextAdjacentAS = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_BGPADJ, 1);
					break;
			case NFL_I_BGPPREVADJACENTAS:
					rec->bgpPrevAdjacentAS = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_BGPADJ, 1);
					break;
			case NFL_I_BGP_NEXTHOP:
					res = SV_to_ip_addr((ip_addr_t *)&rec->bgp_nexthop, sv);
					switch (res) {
						case AF_INET:
							ClearFlag(rec->flags, FLAG_IPV6_NHB);
							bit_array_set(&instance->ext_w, EX_NEXT_HOP_BGP_v4, 1);
							break;
					case AF_INET6:
							SetFlag(rec->flags, FLAG_IPV6_NHB);
							bit_array_set(&instance->ext_w, EX_NEXT_HOP_BGP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_BGP_NEXTHOP);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;

			case NFL_I_PROT: 	
					rec->prot = SvUV(sv);
					break;

			case NFL_I_SRC_VLAN:
					rec->src_vlan = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_VLAN, 1);
					break;
			case NFL_I_DST_VLAN:
					rec->dst_vlan = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_VLAN, 1);
					break;

			case NFL_I_IN_SRC_MAC:
					if (SV_to_mac(&rec->in_src_mac, sv) != 0) {
						warn("%s invalid MAC address %s", NFL_LOG, NFL_T_IN_SRC_MAC);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					bit_array_set(&instance->ext_w, EX_MAC_1, 1);
					break;
			case NFL_I_OUT_DST_MAC:
					if (SV_to_mac(&rec->out_dst_mac, sv) != 0) {
						warn("%s invalid MAC address %s", NFL_LOG, NFL_T_OUT_DST_MAC);
						bit_array_release(&instance->ext_w);
						return 0;
					}
					bit_array_set(&instance->ext_w, EX_MAC_1, 1);
					break;
			case NFL_I_OUT_SRC_MAC:
					if (SV_to_mac(&rec->out_src_mac, sv) != 0) {
						warn("%s invalid MAC address %s", NFL_LOG, NFL_T_OUT_SRC_MAC);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					bit_array_set(&instance->ext_w, EX_MAC_2, 1);
					break;
			case NFL_I_IN_DST_MAC:
					if (SV_to_mac(&rec->in_dst_mac, sv) != 0) {
						warn("%s invalid MAC address %s", NFL_LOG, NFL_T_IN_DST_MAC);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					bit_array_set(&instance->ext_w, EX_MAC_2, 1);
					break;

			case NFL_I_MPLS_LABEL:
					if (SV_to_mpls((char *)&rec->mpls_label, sv) != 0) {
						warn("%s invalid item %s", NFL_LOG, NFL_T_MPLS_LABEL);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					bit_array_set(&instance->ext_w, EX_MPLS, 1);
					break;

			// EX_IO_SNMP_2 not used 
			case NFL_I_INPUT:
					rec->input = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_IO_SNMP_4, 1);
					break;
			case NFL_I_OUTPUT:
					rec->output = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_IO_SNMP_4, 1);
					break;

			case NFL_I_DIR:
					rec->dir = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_MULIPLE, 1);
					break;

			case NFL_I_FWD_STATUS:
					rec->fwd_status = SvUV(sv);
					break;

			case NFL_I_IP_ROUTER:
					res = SV_to_ip_addr((ip_addr_t *)&rec->ip_router, sv);
					switch (res) {
						case AF_INET:
							ClearFlag(rec->flags, FLAG_IPV6_EXP);
							bit_array_set(&instance->ext_w, EX_ROUTER_IP_v4, 1);
							break;
					case AF_INET6:
							SetFlag(rec->flags, FLAG_IPV6_EXP);
							bit_array_set(&instance->ext_w, EX_ROUTER_IP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_IP_ROUTER);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_ENGINE_TYPE:
					rec->engine_type = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_ROUTER_ID, 1);
					break;
			case NFL_I_ENGINE_ID:
					rec->engine_id = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_ROUTER_ID, 1);
					break;

			// NSEL 
#ifdef NSEL
			case NFL_I_FLOW_START:
					rec->flow_start = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_COMMON, 1);
					break;
			case NFL_I_CONN_ID:
					rec->conn_id = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_COMMON, 1);
					break;
			case NFL_I_ICMP_CODE:
					rec->icmp_code = SvUV(sv); 
					bit_array_set(&instance->ext_w, EX_NSEL_COMMON, 1);
					break;
			case NFL_I_ICMP_TYPE:
					rec->icmp_type = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_COMMON, 1);
					break;
			case NFL_I_FW_EVENT:
					rec->fw_event = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_COMMON, 1);
					break;
			case NFL_I_FW_XEVENT:
					rec->fw_xevent = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_COMMON, 1);
					break;
			case NFL_I_XLATE_SRC_IP:
					res = SV_to_ip_addr((ip_addr_t *)&rec->xlate_src_ip, sv);
					switch (res) {
						case AF_INET:
							rec->xlate_flags = 0;
							bit_array_set(&instance->ext_w, EX_NSEL_XLATE_IP_v4, 1);
							break;
					case AF_INET6:
							rec->xlate_flags = 1;
							bit_array_set(&instance->ext_w, EX_NSEL_XLATE_IP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_XLATE_SRC_IP);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_XLATE_DST_IP:
					res = SV_to_ip_addr((ip_addr_t *)&rec->xlate_dst_ip, sv);
					switch (res) {
						case AF_INET:
							rec->xlate_flags = 0;
							bit_array_set(&instance->ext_w, EX_NSEL_XLATE_IP_v4, 1);
							break;
					case AF_INET6:
							rec->xlate_flags = 1;
							bit_array_set(&instance->ext_w, EX_NSEL_XLATE_IP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_XLATE_DST_IP);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_XLATE_SRC_PORT:
					rec->xlate_src_port = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_XLATE_PORTS, 1);
					break;
			case NFL_I_XLATE_DST_PORT:
					rec->xlate_dst_port = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_XLATE_PORTS, 1);
					break;
			case NFL_I_INGRESS_ACL_ID:
					rec->ingress_acl_id[0] = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_ACL, 1);
					break;
			case NFL_I_INGRESS_ACE_ID:
					rec->ingress_acl_id[1] = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_ACL, 1);
					break;
			case NFL_I_INGRESS_XACE_ID:
					rec->ingress_acl_id[2] = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_ACL, 1);
					break;
			case NFL_I_EGRESS_ACL_ID:
					rec->egress_acl_id[0] = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_ACL, 1);
					break;
			case NFL_I_EGRESS_ACE_ID:
					rec->egress_acl_id[1] = SvUV(sv); 
					bit_array_set(&instance->ext_w, EX_NSEL_ACL, 1);
					break;
			case NFL_I_EGRESS_XACE_ID:
					rec->egress_acl_id[2] = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NSEL_ACL, 1);
					break;
			case NFL_I_USERNAME: {
						STRLEN len;
						char *s;

						s = SvPV(sv, len);

						if ( len > sizeof(rec->username) -  1 ) {
							len = sizeof(rec->username) - 1;
						}

						memcpy(rec->username, s, len );
						rec->username[len] = '\0';		// to be sure 
						if ( len < sizeof(((struct tpl_ext_42_s *)0)->username) - 1 ) {
	 						bit_array_set(&instance->ext_w, EX_NSEL_USER, 1);
						} else {
		 					bit_array_set(&instance->ext_w, EX_NSEL_USER_MAX, 1);
						}
					}
					break;
#endif 
			// END OF NSEL 
			
			// NEL support
#ifdef NEL
			case NFL_I_NAT_EVENT:
					rec->nat_event = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NEL_COMMON, 1);
					break;
			case NFL_I_POST_SRC_PORT:
					rec->post_src_port = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NEL_COMMON, 1);
					break;
			case NFL_I_POST_DST_PORT:
					rec->post_dst_port = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NEL_COMMON, 1);
					break;
			case NFL_I_INGRESS_VRFID:
					rec->ingress_vrfid = SvUV(sv);
					bit_array_set(&instance->ext_w, EX_NEL_COMMON, 1);
					break;
			case NFL_I_NAT_INSIDE:
					res = SV_to_ip_addr((ip_addr_t *)&rec->nat_inside, sv);
					switch (res) {
						case AF_INET:
							rec->nat_flags = 0;
							bit_array_set(&instance->ext_w, EX_NEL_GLOBAL_IP_v4, 1);
							break;
					case AF_INET6:
							rec->nat_flags = 1;
							bit_array_set(&instance->ext_w, EX_NEL_GLOBAL_IP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_NAT_INSIDE);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
			case NFL_I_NAT_OUTSIDE:
					res = SV_to_ip_addr((ip_addr_t *)&rec->nat_outside, sv);
					switch (res) {
						case AF_INET:
							rec->nat_flags = 0;
							bit_array_set(&instance->ext_w, EX_NEL_GLOBAL_IP_v4, 1);
							break;
					case AF_INET6:
							rec->nat_flags = 1;
							bit_array_set(&instance->ext_w, EX_NEL_GLOBAL_IP_v6, 1);
							break;
					default: 
						warn("%s invalid value for %s", NFL_LOG, NFL_T_NAT_OUTSIDE);
						bit_array_clear(&instance->ext_w);
						return 0;
					}
					break;
#endif 

			// END OF NEL 


			case NFL_I_CLIENT_NW_DELAY_USEC:
					rec->client_nw_delay_usec = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_LATENCY, 1);
					break;
			case NFL_I_SERVER_NW_DELAY_USEC:
					rec->server_nw_delay_usec = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_LATENCY, 1);
					break;
			case NFL_I_APPL_LATENCY_USEC:
					rec->appl_latency_usec = SvU64(sv);
					bit_array_set(&instance->ext_w, EX_LATENCY, 1);
					break;

			default:
					croak("%s Unknown ID (%d) in %s !!", NFL_LOG, instance->field_list[i], __FUNCTION__);
					break;
		}

		i++;
	}

	map = libnf_lookup_map(instance, &instance->ext_w);

	rec->map_ref = map;
	rec->ext_map = map->map_id;
	rec->type = CommonRecordType;

	bit_array_clear(&instance->ext_w);
/*
	{
		char *s;
		PrintExtensionMap(map);
		VerifyExtensionMap(map);
		format_file_block_record(&rec, &s, 0);
		printf("WRITE: (%p) \n%s\n", map, s);
	}
*/
	UpdateStat(instance->nffile_w->stat_record, rec);

	PackRecord(rec, instance->nffile_w);

	memzero(rec, sizeof(master_record_t));	// clean rec for next row 

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
	
	bit_array_release(&instance->ext_r);
	bit_array_release(&instance->ext_w);

	PackExtensionMapList(instance->extension_map_list);
	FreeExtensionMaps(instance->extension_map_list);
	free(instance); 
	libnf_instances[handle] = NULL;
	//return stat_record;
	return ;

} // End of process_data_finish


