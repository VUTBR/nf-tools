
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
#define NFL_FIRST	 	"first"		// pod:## - Timestamp of first seen packet E<10>
#define NFL_MSEC_FIRST 	"msecfirst"	// pod:## - Number of miliseconds of first seen packet since B<first>  E<10>
#define NFL_LAST	 	"last"		// pod:## - Timestamp of last seen packet E<10>
#define NFL_MSEC_LAST 	"mseclast"	// pod:## - Number of miliseconds of last seen packet since B<last>  E<10>
#define NFL_RECEIVED	"received"	// pod:## - Timestamp when the packet was received by collector E<10>
// pod:
// pod:=head2 Statistical items
// pod:
#define NFL_DOCTETS		"bytes"		// pod:## - The number of bytes E<10>
#define NFL_DPKTS		"pkts"		// pod:## - The number of packets E<10>
#define NFL_OUT_BYTES	"outbytes"	// pod:## - The number of output bytes  E<10>
#define NFL_OUT_PKTS	"outpkts"	// pod:## - The number of output packets  E<10>
#define NFL_AGGR_FLOWS	"flows"		// pod:## - The number of flows (aggregated) E<10>
// pod:
// pod:=head2 Layer 4 information
// pod:
#define NFL_SRCPORT 	"srcport"	// pod:## - Source port E<10>
#define NFL_DSTPORT 	"dstport"	// pod:## - Destination port E<10>
#define NFL_TCP_FLAGS	"tcpflags"	// pod:## - TCP flags  E<10>
// pod:
// pod:=head2 Layer 3 information
// pod:
#define NFL_SRCADDR 	"srcip"		// pod:## - Source IP address E<10>
#define NFL_DSTADDR		"dstip"		// pod:## - Destination IP address E<10>
#define NFL_IP_NEXTHOP	"nexthop"	// pod:## - IP next hop E<10>
#define NFL_SRC_MASK	"srcmask"	// pod:## - Source mask E<10>
#define NFL_DST_MASK	"dstmask"	// pod:## - Destination mask E<10>
#define NFL_TOS			"tos"		// pod:## - Source type of service E<10>
#define NFL_DST_TOS		"dsttos"	// pod:## - Destination type of Service E<10>

#define NFL_SRCAS		"srcas"		// pod:## - Source AS number E<10>
#define NFL_DSTAS		"dstas"		// pod:## - Destination AS number E<10>
#define NFL_BGPNEXTADJACENTAS	"nextas"	// pod:## - BGP Next AS E<10>
#define NFL_BGPPREVADJACENTAS	"prevas"	// pod:## - BGP Previous AS E<10>
#define NFL_BGP_NEXTHOP	"bgpnexthop"		// pod:## - BGP next hop E<10>

#define NFL_PROT	 	"proto"		// pod:## - IP protocol  E<10>
// pod:
// pod:=head2 Layer 2 information
// pod:
#define NFL_SRC_VLAN	"srcvlan"	// pod:## - Source vlan label E<10>
#define NFL_DST_VLAN	"dstvlan"	// pod:## - Destination vlan label E<10>
#define NFL_IN_SRC_MAC	"insrcmac"	// pod:## - In source MAC address E<10>
#define NFL_OUT_SRC_MAC	"outsrcmac"	// pod:## - Out destination MAC address E<10>
#define NFL_IN_DST_MAC	"indstmac"	// pod:## - In destintation MAC address E<10>
#define NFL_OUT_DST_MAC	"outdstmac"	// pod:## - Out source MAC address E<10>
// pod:
// pod:=head2 MPLS information
// pod:
#define NFL_MPLS_LABEL	"mpls"		// pod:## - MPLS labels E<10>
// pod:
// pod:=head2 Layer 1 information
// pod:
#define NFL_INPUT		"inif"		// pod:## - SNMP input interface number E<10>
#define NFL_OUTPUT		"outif"		// pod:## - SNMP output interface number E<10>
#define NFL_DIR			"dir"		// pod:## - Flow directions ingress/egress E<10>
#define NFL_FWD_STATUS	"fwd"		// pod:## - Forwarding status E<10>
// pod:
// pod:=head2 Exporter information
// pod:
#define NFL_IP_ROUTER	"router"	// pod:## - Exporting router IP E<10>
#define NFL_ENGINE_TYPE	"systype"	// pod:## - Type of exporter E<10>
#define NFL_ENGINE_ID	"sysid"		// pod:## - Internal SysID of exporter E<10>
// pod:
// pod:=head2 Extra/special fields
// pod:
#define NFL_CLIENT_NW_DELAY_USEC	"clientdelay"	// pod:## - nprobe latency client_nw_delay_usec E<10>
#define NFL_SERVER_NW_DELAY_USEC	"serverdelay"	// pod:## - nprobe latency server_nw_delay_usec E<10>
#define NFL_APPL_LATENCY_USEC		"appllatency"	// pod:## - nprobe latency appl_latency_usec E<10>

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


