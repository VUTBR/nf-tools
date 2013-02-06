
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


/* names of attribudes used in result/input hash */
// pod:=head1 SUPPORTED ITEMS 
// pod:
// pod:=head2 Time items
// pod:
#define NFL_T_FIRST	 		"first"		// pod:## - Timestamp of first seen packet E<10>
#define NFL_I_FIRST	 		01
#define NFL_T_MSEC_FIRST 	"msecfirst"	// pod:## - Number of miliseconds of first seen packet since B<first>  E<10>
#define NFL_I_MSEC_FIRST 	02
#define NFL_T_LAST	 		"last"		// pod:## - Timestamp of last seen packet E<10>
#define NFL_I_LAST	 		03	
#define NFL_T_MSEC_LAST 	"mseclast"	// pod:## - Number of miliseconds of last seen packet since B<last>  E<10>
#define NFL_I_MSEC_LAST 	04	
#define NFL_T_RECEIVED		"received"	// pod:## - Timestamp when the packet was received by collector E<10>
#define NFL_I_RECEIVED		05	
// pod:
// pod:=head2 Statistical items
// pod:
#define NFL_T_DOCTETS		"bytes"		// pod:## - The number of bytes E<10>
#define NFL_I_DOCTETS		06
#define NFL_T_DPKTS			"pkts"		// pod:## - The number of packets E<10>
#define NFL_I_DPKTS			07
#define NFL_T_OUT_BYTES		"outbytes"	// pod:## - The number of output bytes  E<10>
#define NFL_I_OUT_BYTES		08
#define NFL_T_OUT_PKTS		"outpkts"	// pod:## - The number of output packets  E<10>
#define NFL_I_OUT_PKTS		09
#define NFL_T_AGGR_FLOWS	"flows"		// pod:## - The number of flows (aggregated) E<10>
#define NFL_I_AGGR_FLOWS	10
// pod:
// pod:=head2 Layer 4 information
// pod:
#define NFL_T_SRCPORT 		"srcport"	// pod:## - Source port E<10>
#define NFL_I_SRCPORT 		11	
#define NFL_T_DSTPORT 		"dstport"	// pod:## - Destination port E<10>
#define NFL_I_DSTPORT 		12
#define NFL_T_TCP_FLAGS		"tcpflags"	// pod:## - TCP flags  E<10>
#define NFL_I_TCP_FLAGS		13
// pod:
// pod:=head2 Layer 3 information
// pod:
#define NFL_T_SRCADDR 		"srcip"		// pod:## - Source IP address E<10>
#define NFL_I_SRCADDR 		14
#define NFL_T_DSTADDR		"dstip"		// pod:## - Destination IP address E<10>
#define NFL_I_DSTADDR		15
#define NFL_T_IP_NEXTHOP	"nexthop"	// pod:## - IP next hop E<10>
#define NFL_I_IP_NEXTHOP	16
#define NFL_T_SRC_MASK		"srcmask"	// pod:## - Source mask E<10>
#define NFL_I_SRC_MASK		17
#define NFL_T_DST_MASK		"dstmask"	// pod:## - Destination mask E<10>
#define NFL_I_DST_MASK		18
#define NFL_T_TOS			"tos"		// pod:## - Source type of service E<10>
#define NFL_I_TOS			19
#define NFL_T_DST_TOS		"dsttos"	// pod:## - Destination type of Service E<10>
#define NFL_I_DST_TOS		20

#define NFL_T_SRCAS			"srcas"		// pod:## - Source AS number E<10>
#define NFL_I_SRCAS			21	
#define NFL_T_DSTAS			"dstas"		// pod:## - Destination AS number E<10>
#define NFL_I_DSTAS			22
#define NFL_T_BGPNEXTADJACENTAS		"nextas"	// pod:## - BGP Next AS E<10>
#define NFL_I_BGPNEXTADJACENTAS		23
#define NFL_T_BGPPREVADJACENTAS		"prevas"	// pod:## - BGP Previous AS E<10>
#define NFL_I_BGPPREVADJACENTAS		24
#define NFL_T_BGP_NEXTHOP	"bgpnexthop"		// pod:## - BGP next hop E<10>
#define NFL_I_BGP_NEXTHOP	25

#define NFL_T_PROT		 	"proto"		// pod:## - IP protocol  E<10>
#define NFL_I_PROT		 	26
// pod:
// pod:=head2 Layer 2 information
// pod:
#define NFL_T_SRC_VLAN		"srcvlan"	// pod:## - Source vlan label E<10>
#define NFL_I_SRC_VLAN		27
#define NFL_T_DST_VLAN		"dstvlan"	// pod:## - Destination vlan label E<10>
#define NFL_I_DST_VLAN		28
#define NFL_T_IN_SRC_MAC	"insrcmac"	// pod:## - In source MAC address E<10>
#define NFL_I_IN_SRC_MAC	29
#define NFL_T_OUT_SRC_MAC	"outsrcmac"	// pod:## - Out destination MAC address E<10>
#define NFL_I_OUT_SRC_MAC	30
#define NFL_T_IN_DST_MAC	"indstmac"	// pod:## - In destintation MAC address E<10>
#define NFL_I_IN_DST_MAC	31
#define NFL_T_OUT_DST_MAC	"outdstmac"	// pod:## - Out source MAC address E<10>
#define NFL_I_OUT_DST_MAC	32
// pod:
// pod:=head2 MPLS information
// pod:
#define NFL_T_MPLS_LABEL	"mpls"		// pod:## - MPLS labels E<10>
#define NFL_I_MPLS_LABEL	33
// pod:
// pod:=head2 Layer 1 information
// pod:
#define NFL_T_INPUT			"inif"		// pod:## - SNMP input interface number E<10>
#define NFL_I_INPUT			34
#define NFL_T_OUTPUT		"outif"		// pod:## - SNMP output interface number E<10>
#define NFL_I_OUTPUT		35
#define NFL_T_DIR			"dir"		// pod:## - Flow directions ingress/egress E<10>
#define NFL_I_DIR			36
#define NFL_T_FWD_STATUS	"fwd"		// pod:## - Forwarding status E<10>
#define NFL_I_FWD_STATUS	37
// pod:
// pod:=head2 Exporter information
// pod:
#define NFL_T_IP_ROUTER		"router"	// pod:## - Exporting router IP E<10>
#define NFL_I_IP_ROUTER		38
#define NFL_T_ENGINE_TYPE	"systype"	// pod:## - Type of exporter E<10>
#define NFL_I_ENGINE_TYPE	39
#define NFL_T_ENGINE_ID		"sysid"		// pod:## - Internal SysID of exporter E<10>
#define NFL_I_ENGINE_ID		40
// pod:
// pod:=head2 Extra/special fields
// pod:
#define NFL_T_CLIENT_NW_DELAY_USEC	"clientdelay"	// pod:## - nprobe latency client_nw_delay_usec E<10>
#define NFL_I_CLIENT_NW_DELAY_USEC	41
#define NFL_T_SERVER_NW_DELAY_USEC	"serverdelay"	// pod:## - nprobe latency server_nw_delay_usec E<10>
#define NFL_I_SERVER_NW_DELAY_USEC	42
#define NFL_T_APPL_LATENCY_USEC		"appllatency"	// pod:## - nprobe latency appl_latency_usec E<10>
#define NFL_I_APPL_LATENCY_USEC		43
// pod:


//
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
int libnf_read_files(int handle, char *filter, int window_start, int window_end, SV *files);
int libnf_create_file(int handle, char *filename, int compressed, int anonymized, char *ident);
SV * libnf_read_row(int handle);
int libnf_write_row(int handle, HV * hashref);
void libnf_finish(int handle);


