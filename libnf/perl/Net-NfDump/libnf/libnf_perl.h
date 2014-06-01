
/* 
* This .h file is also used to create some parts of libnf documentation 
* the text after the macro pod<double dot> will be placed into separate file libnf.h.pod 
* and then is included into basic libnf documentation 
*/

#include <stdio.h>
#include <stdlib.h>
#include <nffile.h>
#include <nfx.h>
//#include "bit_array.h"


/* multiple use of version for both perl and nfdump so we redefine it */
#define NFL_VERSION VRESION
#undef VERSION


/* string prefix for error and warning outputs */
#define NFL_LOG			"Net::NfDump: "


/*
typedef struct lnf_fields_f {
	int fld_inde;
	char *fld_name;
	char *fld_descr;
} lnf_fields_t;


lnf_fields_t lnf_fields[] = {
#define LNF_FLD_FIRST		10
	LNF_FLD_FIRST, 		"first",
#define LNF_FLD_FIRST		30
	NFL_FLD_LAST,		"last",


*/


#define NFL_ZERO_FIELD		000
/* names of attribudes used in result/input hash */
// pod:=head1 SUPPORTED ITEMS 
// pod:
// pod:  Time items
// pod:  =====================
#define NFL_T_FIRST	 		"first"		// pod:  ## - Timestamp of the first packet seen (in miliseconds)
#define NFL_I_FIRST	 		0x010064
#define NFL_T_LAST	 		"last"		// pod:  ## - Timestamp of the last packet seen (in miliseconds)
#define NFL_I_LAST	 		0x020064
#define NFL_T_RECEIVED		"received"	// pod:  ## - Timestamp regarding when the packet was received by collector 
#define NFL_I_RECEIVED		0x030064
// pod:
// pod:  Statistical items
// pod:  =====================
#define NFL_T_DOCTETS		"bytes"		// pod:  ## - The number of bytes 
#define NFL_I_DOCTETS		0x040064	
#define NFL_T_DPKTS			"pkts"		// pod:  ## - The number of packets 
#define NFL_I_DPKTS			0x050064
#define NFL_T_OUT_BYTES		"outbytes"	// pod:  ## - The number of output bytes 
#define NFL_I_OUT_BYTES		0x060064
#define NFL_T_OUT_PKTS		"outpkts"	// pod:  ## - The number of output packets 
#define NFL_I_OUT_PKTS		0x070064
#define NFL_T_AGGR_FLOWS	"flows"		// pod:  ## - The number of flows (aggregated) 
#define NFL_I_AGGR_FLOWS	0x080064
// pod:
// pod:  Layer 4 information
// pod:  =====================
#define NFL_T_SRCPORT 		"srcport"	// pod:  ## - Source port 
#define NFL_I_SRCPORT 		0x090016
#define NFL_T_DSTPORT 		"dstport"	// pod:  ## - Destination port 
#define NFL_I_DSTPORT 		0x0a0016
#define NFL_T_TCP_FLAGS		"tcpflags"	// pod:  ## - TCP flags  
#define NFL_I_TCP_FLAGS		0x0b0008
// pod:
// pod:  Layer 3 information
// pod:  =====================
#define NFL_T_SRCADDR 		"srcip"		// pod:  ## - Source IP address 
#define NFL_I_SRCADDR 		0x0c00a1
#define NFL_T_DSTADDR		"dstip"		// pod:  ## - Destination IP address 
#define NFL_I_DSTADDR		0x0d00a1
#define NFL_T_IP_NEXTHOP	"nexthop"	// pod:  ## - IP next hop 
#define NFL_I_IP_NEXTHOP	0x0e00a1
#define NFL_T_SRC_MASK		"srcmask"	// pod:  ## - Source mask 
#define NFL_I_SRC_MASK		0x0f0008
#define NFL_T_DST_MASK		"dstmask"	// pod:  ## - Destination mask 
#define NFL_I_DST_MASK		0x100008
#define NFL_T_TOS			"tos"		// pod:  ## - Source type of service 
#define NFL_I_TOS			0x110008
#define NFL_T_DST_TOS		"dsttos"	// pod:  ## - Destination type of service 
#define NFL_I_DST_TOS		0x130008

#define NFL_T_SRCAS			"srcas"		// pod:  ## - Source AS number 
#define NFL_I_SRCAS			0x140032
#define NFL_T_DSTAS			"dstas"		// pod:  ## - Destination AS number 
#define NFL_I_DSTAS			 0x150032
#define NFL_T_BGPNEXTADJACENTAS		"nextas"	// pod:  ## - BGP Next AS 
#define NFL_I_BGPNEXTADJACENTAS		0x160032
#define NFL_T_BGPPREVADJACENTAS		"prevas"	// pod:  ## - BGP Previous AS 
#define NFL_I_BGPPREVADJACENTAS		0x170032
#define NFL_T_BGP_NEXTHOP	"bgpnexthop"		// pod:  ## - BGP next hop 
#define NFL_I_BGP_NEXTHOP	0x1800a1

#define NFL_T_PROT		 	"proto"		// pod:  ## - IP protocol  
#define NFL_I_PROT		 	0x190008
// pod:
// pod:  Layer 2 information
// pod:  =====================
#define NFL_T_SRC_VLAN		"srcvlan"	// pod:  ## - Source vlan label 
#define NFL_I_SRC_VLAN		0x200016
#define NFL_T_DST_VLAN		"dstvlan"	// pod:  ## - Destination vlan label 
#define NFL_I_DST_VLAN		0x210016
#define NFL_T_IN_SRC_MAC	"insrcmac"	// pod:  ## - In source MAC address 
#define NFL_I_IN_SRC_MAC	0x2200a2
#define NFL_T_OUT_SRC_MAC	"outsrcmac"	// pod:  ## - Out destination MAC address 
#define NFL_I_OUT_SRC_MAC	0x2300a2
#define NFL_T_IN_DST_MAC	"indstmac"	// pod:  ## - In destination MAC address 
#define NFL_I_IN_DST_MAC	0x2400a2
#define NFL_T_OUT_DST_MAC	"outdstmac"	// pod:  ## - Out source MAC address 
#define NFL_I_OUT_DST_MAC	0x2500a2
// pod:
// pod:  MPLS information
// pod:  =====================
#define NFL_T_MPLS_LABEL	"mpls"		// pod:  ## - MPLS labels 
#define NFL_I_MPLS_LABEL	0x2600ab
// pod:
// pod:  Layer 1 information
// pod:  =====================
#define NFL_T_INPUT			"inif"		// pod:  ## - SNMP input interface number 
#define NFL_I_INPUT			0x270016
#define NFL_T_OUTPUT		"outif"		// pod:  ## - SNMP output interface number 
#define NFL_I_OUTPUT		0x280016
#define NFL_T_DIR			"dir"		// pod:  ## - Flow directions ingress/egress 
#define NFL_I_DIR			0x290008
#define NFL_T_FWD_STATUS	"fwd"		// pod:  ## - Forwarding status 
#define NFL_I_FWD_STATUS	0x300008
// pod:
// pod:  Exporter information
// pod:  =====================
#define NFL_T_IP_ROUTER		"router"	// pod:  ## - Exporting router IP 
#define NFL_I_IP_ROUTER		0x3100a1
#define NFL_T_ENGINE_TYPE	"systype"	// pod:  ## - Type of exporter 
#define NFL_I_ENGINE_TYPE	0x320008
#define NFL_T_ENGINE_ID		"sysid"		// pod:  ## - Internal SysID of exporter 
#define NFL_I_ENGINE_ID		0x330008
// pod:
// pod:  NSEL fields, see: http://www.cisco.com/en/US/docs/security/asa/asa81/netflow/netflow.html
// pod:  =====================
#define NFL_T_EVENT_TIME	"eventtime"	// pod:  ## - NSEL The time that the flow was created
#define NFL_I_EVENT_TIME	0x340064
#define NFL_T_CONN_ID		"connid"	// pod:  ## - NSEL An identifier of a unique flow for the device 
#define NFL_I_CONN_ID		0x350032

#define NFL_T_ICMP_CODE		"icmpcode"	// pod:  ## - NSEL ICMP code value 
#define NFL_I_ICMP_CODE		0x360008
#define NFL_T_ICMP_TYPE		"icmptype"	// pod:  ## - NSEL ICMP type value 
#define NFL_I_ICMP_TYPE		0x370008
#define NFL_T_FW_XEVENT		"xevent"	// pod:  ## - NSEL Extended event code
#define NFL_I_FW_XEVENT		 0x380016

#define NFL_T_XLATE_SRC_IP		"xsrcip"	// pod:  ## - NSEL Mapped source IPv4 address 
#define NFL_I_XLATE_SRC_IP		0x3900a1
#define NFL_T_XLATE_DST_IP		"xdstip"	// pod:  ## - NSEL Mapped destination IPv4 address 
#define NFL_I_XLATE_DST_IP		0x4000a1
#define NFL_T_XLATE_SRC_PORT	"xsrcport"	// pod:  ## - NSEL Mapped source port 
#define NFL_I_XLATE_SRC_PORT	0x410016
#define NFL_T_XLATE_DST_PORT	"xdstport"	// pod:  ## - NSEL Mapped destination port 
#define NFL_I_XLATE_DST_PORT	0x420016

// pod: NSEL The input ACL that permitted or denied the flow
#define NFL_T_INGRESS_ACL_ID	"iacl"	// pod:  ## - Hash value or ID of the ACL name
#define NFL_I_INGRESS_ACL_ID	0x430032
#define NFL_T_INGRESS_ACE_ID	"iace"	// pod:  ## - Hash value or ID of the ACL name 
#define NFL_I_INGRESS_ACE_ID	0x440032
#define NFL_T_INGRESS_XACE_ID	"ixace"	// pod:  ## - Hash value or ID of an extended ACE configuration 
#define NFL_I_INGRESS_XACE_ID	0x450032
// pod: NSEL The output ACL that permitted or denied a flow  
#define NFL_T_EGRESS_ACL_ID		"eacl"	// pod:  ## - Hash value or ID of the ACL name
#define NFL_I_EGRESS_ACL_ID		0x460032
#define NFL_T_EGRESS_ACE_ID		"eace"	// pod:  ## - Hash value or ID of the ACL name
#define NFL_I_EGRESS_ACE_ID		0x470032
#define NFL_T_EGRESS_XACE_ID	"exace"	// pod:  ## - Hash value or ID of an extended ACE configuration
#define NFL_I_EGRESS_XACE_ID	0x480032
#define NFL_T_USERNAME			"username"	// pod:  ## - NSEL username
#define NFL_I_USERNAME			0x4900AA
// pod:
// pod:  NEL (NetFlow Event Logging) fields
// pod:  =====================
#define NFL_T_INGRESS_VRFID		"ingressvrfid"	// pod:  ## - NEL NAT ingress vrf id 
#define NFL_I_INGRESS_VRFID		0x500032
#define NFL_T_EVENT_FLAG		"eventflag"		// pod:  ## -  NAT event flag (always set to 1 by nfdump)
#define NFL_I_EVENT_FLAG		0x510008
#define NFL_T_EGRESS_VRFID		"egressvrfid"	// pod:  ## -  NAT egress VRF ID
#define NFL_I_EGRESS_VRFID		0x520032
// pod:
// pod:  NEL Port Block Allocation (added 2014-04-19)
// pod:  =====================
#define NFL_T_BLOCK_START		"blockstart"	// pod:  ## -  NAT pool block start
#define NFL_I_BLOCK_START		0x530016
#define NFL_T_BLOCK_END			"blockend"		// pod:  ## -  NAT pool block end 
#define NFL_I_BLOCK_END			0x540016
#define NFL_T_BLOCK_STEP		"blockstep"		// pod:  ## -  NAT pool block step
#define NFL_I_BLOCK_STEP		0x550016
#define NFL_T_BLOCK_SIZE		"blocksize"		// pod:  ## -  NAT pool block size
#define NFL_I_BLOCK_SIZE		0x560016
// pod:
// pod:  Extra/special fields
// pod:  =====================
#define NFL_T_CLIENT_NW_DELAY_USEC	"cl"	// pod:  ## - nprobe latency client_nw_delay_usec 
#define NFL_I_CLIENT_NW_DELAY_USEC  0x570064
#define NFL_T_SERVER_NW_DELAY_USEC	"sl"	// pod:  ## - nprobe latency server_nw_delay_usec
#define NFL_I_SERVER_NW_DELAY_USEC	0x580064
#define NFL_T_APPL_LATENCY_USEC		"al"	// pod:  ## - nprobe latency appl_latency_usec
#define NFL_I_APPL_LATENCY_USEC		0x590064
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


/* perl - function prototypes */
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



