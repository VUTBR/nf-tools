

#include <stdio.h>
#include <stdlib.h>

/* multiple use of version for both perl and nfdump */
#define NFL_VERSION VRESION
#undef VERSION

#define NFL_LOG			"Net::NfDump: "

/* names of attribudes used in result/input hash */
#define NFL_FIRST	 	"first"
#define NFL_MSEC_FIRST 	"msec_first"
#define NFL_LAST	 	"last"
#define NFL_MSEC_LAST 	"msec_last"
#define NFL_SRCADDR 	"srcip"
#define NFL_DSTADDR		"dstip"
#define NFL_DPKTS		"pkts"
#define NFL_DOCTETS		"bytes"
#define NFL_AGGR_FLOWS	"flows"
#define NFL_INPUT		"input"
#define NFL_OUTPUT		"output"
#define NFL_SRCAS		"srcas"
#define NFL_DSTAS		"dstas"
#define NFL_DST_TOS		"dsttos"
#define NFL_DIR			"dir"
#define NFL_SRC_MASK	"srcmask"
#define NFL_DST_MASK	"dstmask"
#define NFL_IP_NEXTHOP	"nexthop"
#define NFL_BGP_NEXTHOP	"bgpnexthop"
#define NFL_SRC_VLAN	"srcvlan"
#define NFL_DST_VLAN	"dstvlan"
#define NFL_OUT_PKTS	"outpkts"
#define NFL_OUT_BYTES	"outbytes"
#define NFL_IN_SRC_MAC	"insrcmac"
#define NFL_OUT_SRC_MAC	"outsrcmac"
#define NFL_IN_DST_MAC	"indstmac"
#define NFL_OUT_DST_MAC	"outdstmac"
#define NFL_IP_ROUTER	"iprouter"
#define NFL_ENGINE_TYPE	"enginetype"
#define NFL_ENGINE_ID	"engineid"

#define NFL_RAW			"_raw_"

/* the maxumim naumber of instances (objects) that can be used in code */
#define NFL_MAX_INSTANCES 512

/* return eroror codes */
#define NFL_NO_FREE_INSTANCES -1;

/* extend NF_XX codes with code idicates thet we already to read the next record */
#define NF_OK      1


int libnf_init(void);
int libnf_read_files(int handle, char *filter, int window_start, int window_end, SV *files);
int libnf_create_file(int handle, char *filename, int compressed, int anonymized, char *ident);
SV * libnf_read_row(int handle);
int libnf_write_row(int handle, HV * hashref);
void libnf_finish(int handle);


