
/* 
* This .h file is also used to create some parts of libnf documentation 
* the text after the macro pod<double dot> will be placed into separate file libnf.h.pod 
* and then is included into basic libnf documentation 
*/

#include <stdio.h>
#include <stdlib.h>


/* multiple use of version for both perl and nfdump so we redefine it */
#define NFL_VERSION VRESION
#undef VERSION


/* string prefix for error and warning outputs */
#define NFL_LOG			"Net::NfDump: "


#define NFL_ZERO_FIELD		000
/* names of attribudes used in result/input hash */
// pod:=head1 SUPPORTED ITEMS 
// pod:
// pod:  Time items
// pod:  =====================
#define NFL_T_FIRST	 		"first"		// pod:  ## - Timestamp of first seen packet 
#define NFL_I_FIRST	 		10
#define NFL_T_MSEC_FIRST 	"msecfirst"	// pod:  ## - Number of miliseconds of first seen packet since B<first>  
#define NFL_I_MSEC_FIRST 	20
#define NFL_T_LAST	 		"last"		// pod:  ## - Timestamp of last seen packet 
#define NFL_I_LAST	 		30	
#define NFL_T_MSEC_LAST 	"mseclast"	// pod:  ## - Number of miliseconds of last seen packet since B<last>  
#define NFL_I_MSEC_LAST 	40	
#define NFL_T_RECEIVED		"received"	// pod:  ## - Timestamp when the packet was received by collector 
#define NFL_I_RECEIVED		50	
// pod:
// pod:  Statistical items
// pod:  =====================
#define NFL_T_DOCTETS		"bytes"		// pod:  ## - The number of bytes 
#define NFL_I_DOCTETS		60
#define NFL_T_DPKTS			"pkts"		// pod:  ## - The number of packets 
#define NFL_I_DPKTS			70
#define NFL_T_OUT_BYTES		"outbytes"	// pod:  ## - The number of output bytes 
#define NFL_I_OUT_BYTES		80
#define NFL_T_OUT_PKTS		"outpkts"	// pod:  ## - The number of output packets 
#define NFL_I_OUT_PKTS		90
#define NFL_T_AGGR_FLOWS	"flows"		// pod:  ## - The number of flows (aggregated) 
#define NFL_I_AGGR_FLOWS	100
// pod:
// pod:  Layer 4 information
// pod:  =====================
#define NFL_T_SRCPORT 		"srcport"	// pod:  ## - Source port 
#define NFL_I_SRCPORT 		110	
#define NFL_T_DSTPORT 		"dstport"	// pod:  ## - Destination port 
#define NFL_I_DSTPORT 		120
#define NFL_T_TCP_FLAGS		"tcpflags"	// pod:  ## - TCP flags  
#define NFL_I_TCP_FLAGS		130
// pod:
// pod:  Layer 3 information
// pod:  =====================
#define NFL_T_SRCADDR 		"srcip"		// pod:  ## - Source IP address 
#define NFL_I_SRCADDR 		140
#define NFL_T_DSTADDR		"dstip"		// pod:  ## - Destination IP address 
#define NFL_I_DSTADDR		150
#define NFL_T_IP_NEXTHOP	"nexthop"	// pod:  ## - IP next hop 
#define NFL_I_IP_NEXTHOP	160
#define NFL_T_SRC_MASK		"srcmask"	// pod:  ## - Source mask 
#define NFL_I_SRC_MASK		170
#define NFL_T_DST_MASK		"dstmask"	// pod:  ## - Destination mask 
#define NFL_I_DST_MASK		180
#define NFL_T_TOS			"tos"		// pod:  ## - Source type of service 
#define NFL_I_TOS			190
#define NFL_T_DST_TOS		"dsttos"	// pod:  ## - Destination type of Service 
#define NFL_I_DST_TOS		200

#define NFL_T_SRCAS			"srcas"		// pod:  ## - Source AS number 
#define NFL_I_SRCAS			210	
#define NFL_T_DSTAS			"dstas"		// pod:  ## - Destination AS number 
#define NFL_I_DSTAS			220
#define NFL_T_BGPNEXTADJACENTAS		"nextas"	// pod:  ## - BGP Next AS 
#define NFL_I_BGPNEXTADJACENTAS		230
#define NFL_T_BGPPREVADJACENTAS		"prevas"	// pod:  ## - BGP Previous AS 
#define NFL_I_BGPPREVADJACENTAS		240
#define NFL_T_BGP_NEXTHOP	"bgpnexthop"		// pod:  ## - BGP next hop 
#define NFL_I_BGP_NEXTHOP	250

#define NFL_T_PROT		 	"proto"		// pod:  ## - IP protocol  
#define NFL_I_PROT		 	260
// pod:
// pod:  Layer 2 information
// pod:  =====================
#define NFL_T_SRC_VLAN		"srcvlan"	// pod:  ## - Source vlan label 
#define NFL_I_SRC_VLAN		270
#define NFL_T_DST_VLAN		"dstvlan"	// pod:  ## - Destination vlan label 
#define NFL_I_DST_VLAN		280
#define NFL_T_IN_SRC_MAC	"insrcmac"	// pod:  ## - In source MAC address 
#define NFL_I_IN_SRC_MAC	290
#define NFL_T_OUT_SRC_MAC	"outsrcmac"	// pod:  ## - Out destination MAC address 
#define NFL_I_OUT_SRC_MAC	300
#define NFL_T_IN_DST_MAC	"indstmac"	// pod:  ## - In destintation MAC address 
#define NFL_I_IN_DST_MAC	310
#define NFL_T_OUT_DST_MAC	"outdstmac"	// pod:  ## - Out source MAC address 
#define NFL_I_OUT_DST_MAC	320
// pod:
// pod:  MPLS information
// pod:  =====================
#define NFL_T_MPLS_LABEL	"mpls"		// pod:  ## - MPLS labels 
#define NFL_I_MPLS_LABEL	330
// pod:
// pod:  Layer 1 information
// pod:  =====================
#define NFL_T_INPUT			"inif"		// pod:  ## - SNMP input interface number 
#define NFL_I_INPUT			340
#define NFL_T_OUTPUT		"outif"		// pod:  ## - SNMP output interface number 
#define NFL_I_OUTPUT		350
#define NFL_T_DIR			"dir"		// pod:  ## - Flow directions ingress/egress 
#define NFL_I_DIR			360
#define NFL_T_FWD_STATUS	"fwd"		// pod:  ## - Forwarding status 
#define NFL_I_FWD_STATUS	370
// pod:
// pod:  Exporter information
// pod:  =====================
#define NFL_T_IP_ROUTER		"router"	// pod:  ## - Exporting router IP 
#define NFL_I_IP_ROUTER		380
#define NFL_T_ENGINE_TYPE	"systype"	// pod:  ## - Type of exporter 
#define NFL_I_ENGINE_TYPE	390
#define NFL_T_ENGINE_ID		"sysid"		// pod:  ## - Internal SysID of exporter 
#define NFL_I_ENGINE_ID		400
// pod:
// pod:  Extra/special fields
// pod:  =====================
#define NFL_T_CLIENT_NW_DELAY_USEC	"clientdelay"	// pod:  ## - nprobe latency client_nw_delay_usec 
#define NFL_I_CLIENT_NW_DELAY_USEC	410
#define NFL_T_SERVER_NW_DELAY_USEC	"serverdelay"	// pod:  ## - nprobe latency server_nw_delay_usec
#define NFL_I_SERVER_NW_DELAY_USEC	420
#define NFL_T_APPL_LATENCY_USEC		"appllatency"	// pod:  ## - nprobe latency appl_latency_usec
#define NFL_I_APPL_LATENCY_USEC		430
// pod:
//


/* the maximim number of fields requested from the client */
#define NFL_MAX_FIELDS 256

/* the maxumim naumber of instances (objects) that can be used in code */
#define NFL_MAX_INSTANCES 512


/* return eroror codes */
#define NFL_NO_FREE_INSTANCES -1;

/* extend NF_XX codes with code idicates thet we already to read the next record */
#define NF_OK      1


/* function prototypes */
SV * libnf_file_info(char *file);
SV * libnf_instance_info(int handle);
int libnf_init(void);
int libnf_set_fields(int handle, SV *fields);
int libnf_read_files(int handle, char *filter, int window_start, int window_end, SV *files);
int libnf_create_file(int handle, char *filename, int compressed, int anonymized, char *ident);
SV * libnf_read_row(int handle);
int libnf_copy_row(int handle, int src_handle);
int libnf_write_row(int handle, SV * arrayref);
void libnf_finish(int handle);


