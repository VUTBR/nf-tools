
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
//
#define NFL_FIRST	 	"first"			
#define NFL_MSEC_FIRST 	"msecfirst"
#define NFL_LAST	 	"last"
#define NFL_MSEC_LAST 	"mseclast"
#define NFL_RECEIVED	"received"	// pod: Flow received time in ms

#define NFL_DOCTETS		"bytes"
#define NFL_DPKTS		"pkts"		
#define NFL_OUT_BYTES	"outbytes"
#define NFL_OUT_PKTS	"outpkts"
#define NFL_AGGR_FLOWS	"flows"

#define NFL_SRCPORT 	"srcport"	// pod:=item ## Source port
#define NFL_DSTPORT 	"dstport"	// pod: Destination port
#define NFL_TCP_FLAGS	"tcpflags"	// pod: TCP flags 

#define NFL_SRCADDR 	"srcip"		// pod: Source IP address
#define NFL_DSTADDR		"dstip"		// pod: Destination IP address
#define NFL_IP_NEXTHOP	"nexthop"	// pod: IP next hop
#define NFL_SRC_MASK	"srcmask"	// pod: Source mask
#define NFL_DST_MASK	"dstmask"	// pod: Destination mask
#define NFL_TOS			"tos"		// pod: Source type of service
#define NFL_DST_TOS		"dsttos"	// pod: Destination type of Service

#define NFL_SRCAS		"srcas"		// pod: Source AS number
#define NFL_DSTAS		"dstas"		// pod: Destination AS number
#define NFL_BGPNEXTADJACENTAS	"nextas"	// pod: BGP Next AS
#define NFL_BGPPREVADJACENTAS	"prevas"	// pod: BGP Previous AS
#define NFL_BGP_NEXTHOP	"bgpnexthop"		// pod: BGP next hop

#define NFL_PROT	 	"proto"		// pod: IP protocol 


#define NFL_SRC_VLAN	"srcvlan"	// pod: Source vlan label
#define NFL_DST_VLAN	"dstvlan"	// pod: Destination vlan label
#define NFL_IN_SRC_MAC	"insrcmac"	// pod: In source MAC address
#define NFL_OUT_SRC_MAC	"outsrcmac"	// pod: Out destination MAC address
#define NFL_IN_DST_MAC	"indstmac"	// pod: In destintation MAC address
#define NFL_OUT_DST_MAC	"outdstmac"	// pod: Out source MAC address

#define NFL_MPLS_LABEL	"mpls"		// pod: MPLS label

#define NFL_INPUT		"inif"		// pod: SNMP input interface number
#define NFL_OUTPUT		"outif"		// pod: SNMP output interface number
#define NFL_DIR			"dir"		// pod: Flow directions ingress/egress
#define NFL_FWD_STATUS	"fwd"		// pod: Forwarding status

#define NFL_IP_ROUTER	"router"	// pod: Exporting router IP
#define NFL_ENGINE_TYPE	"systype"	// pod: Type of exporter
#define NFL_ENGINE_ID	"sysid"		// pod: Internal SysID of exporter


#define NFL_CLIENT_NW_DELAY_USEC	"clientdelay"	// pod: nprobe latency client_nw_delay_usec
#define NFL_SERVER_NW_DELAY_USEC	"serverdelay"	// pod: nprobe latency server_nw_delay_usec
#define NFL_APPL_LATENCY_USEC		"appllatency"	// pod: nprobe latency appl_latency_usec

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


