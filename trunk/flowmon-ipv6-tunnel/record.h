/* -----------------------------------------------------------------------
*
*    Company: INVEA-TECH a.s.
*
*    Project: Invea FlowMon Exporter
*
*    $Id: record.h 12 2011-06-16 11:24:07Z celeda $
*
* -----------------------------------------------------------------------
*
*    (c) Copyright 2007-2009 INVEA-TECH a.s.
*    All rights reserved.
*
*    Please review the terms of the license agreement before using this
*    file. If you are not an authorized user, please destroy this
*    source code file and notify INVEA-TECH a.s. immediately that you
*    inadvertently received an unauthorized copy.
*
* -----------------------------------------------------------------------
*/

#ifndef FLOWMON_RECORD_H
#define FLOWMON_RECORD_H

#include <stdint.h>
#include <stddef.h>
#ifdef FLOWMON_PLUGIN
	#define PROCESS_MPLS
	#define PROCESS_VLAN
#else
	#include "config.h"
#endif

/**
 * \file record.h
 * \brief Header contains record structure, defines and macros to work with it
 */

/** ALL all records have these filled (start, end, bytes, octets, interface id's, as id's, sampling rate) */
#define FLOWMON_ALL			0x0000000000000000
/** IPV4 - record contains valid ipv4 dst and src */
#define FLOWMON_IPV4		0x0000000000000001
/** IPV6 - record contains valid ipv6 dst snd src  */
#define FLOWMON_IPV6		0x0000000000000002
/** PORTS - record contains valid TCP/UDP/SCTP ports  */
#define FLOWMON_PORTS		0x0000000000000004
/** TCP - record contains TCP valid flags  */
#define FLOWMON_TCP			0x0000000000000008
/** ICMP - record containt ICMP valid type and code  */
#define FLOWMON_ICMP		0x0000000000000010
/** MPLS1 - record contain valid MPLS id at level 1 */
#define FLOWMON_MPLS1		0x0000000000000020
/** MPLS2 - record contain valid MPLS id at level 2 */
#define FLOWMON_MPLS2		0x0000000000000040
/** MPLS3 - record contain valid MPLS id at level 3 */
#define FLOWMON_MPLS3		0x0000000000000080
/** MPLS4 - record contain valid MPLS id at level 4 */
#define FLOWMON_MPLS4		0x0000000000000100
/** MPLS5 - record contain valid MPLS id at level 5 */
#define FLOWMON_MPLS5		0x0000000000000200
/** MPLS6 - record contain valid MPLS id at level 6 */
#define FLOWMON_MPLS6		0x0000000000000400
/** MPLS7 - record contain valid MPLS id at level 7 */
#define FLOWMON_MPLS7		0x0000000000000800
/** MPLS8 - record contain valid MPLS id at level 8 */
#define FLOWMON_MPLS8		0x0000000000001000
/** VLAN1 - record contain valid VLAN tag at level 1 */
#define FLOWMON_VLAN1		0x0000000000002000
/** EXPORT - record must be exported now and don't have to wait for timeouts */
#define FLOWMON_EXPORT		0x0000000000004000

/** \struct ipv4_addr_t
 *  \brief ipv4 addr structure
 *  Union, contains IPv4 address as uint8, uint16 and uint32 arrays.
 */
typedef struct {
	/** Union contains 8,16 and 32 bits array with IPv4 address */
	union {
		/** uint8_t array part */
		uint8_t addr8[4];
		/** uint16_t array part */
		uint16_t addr16[2];
		/** uint32_t array part */
		uint32_t addr32[1];
	} in4_u;
/** Macro for direct acces to uint8 part of ipv4 address */
#define ip4_addr8 in4_u.addr8
/** Macro for direct acces to uint16 part of ipv4 address */
#define ip4_addr16 in4_u.addr16
/** Macro for direct acces to uint32 part of ipv4 address */
#define ip4_addr32 in4_u.addr32
} ipv4_addr_t;		/*ipv4 addr struct */

/** \struct ipv4_t
 *  Structure, contains IPv4 address pair and ToS field
 */
typedef struct {
	/** Source address */
	ipv4_addr_t src;
	/** Destination address */
	ipv4_addr_t dst;
	/** ToS field */
	uint8_t tos;
} ipv4_t;

/** \struct ipv6_addr_t
 *  Union, contains IPv6 address as uint8, uint16, uint32 and uint64 arrays.
 */
typedef struct {
	/** Union contains 8,16,32 and 64 bits array with IPv6 address */
	union {
		/** uint8_t array part */
		uint8_t addr8[16];
		/** uint16_t array part */
		uint16_t addr16[8];
		/** uint32_t array part */
		uint32_t addr32[4];
		/** uint64_t array part */
		uint64_t addr64[2];
	} in6_u;
/** Macro for direct acces to uint8 part of ipv6 address */
#define ip6_addr8 in6_u.addr8
/** Macro for direct acces to uint16 part of ipv6 address */
#define ip6_addr16 in6_u.addr16
/** Macro for direct acces to uint32 part of ipv6 address */
#define ip6_addr32 in6_u.addr32
/** Macro for direct acces to uint64 part of ipv6 address */
#define ip6_addr64 in6_u.addr64
} ipv6_addr_t;		/*ipv6 addr struct */

/** \struct ipv6_t
 *  Structure, contains IPv6 address pair and ToS field
 */
typedef struct {
	/** Source address */
	ipv6_addr_t src;
	/** Destination address */
	ipv6_addr_t dst;
	/** Flow label (currently unused) */
	uint32_t flow_label;
} ipv6_t;

/** \struct l3_t
 *  Structure, contains IPv4 or IPv6 address pairs and L3 protocol number
 */
typedef struct {
	/** union contains ipv4 or ipv6 address pair */
	union {
#ifndef NO_IPV6
		/** ipv6 address pair */
		ipv6_t ipv6;
#endif
		/** ipv4 address pair */
		ipv4_t ipv4;
	} addr;
	/** L3 protocol number */
	uint8_t protocol;
} l3_t;

/** \struct l4_t
 *  Structure, contains L4 values
 */
typedef struct {
	/** L4 protocol number */
	uint8_t protocol;
	/** Source port (check FLOWMON_PORTS if valid)*/
	uint16_t src_port;
	/** Destination port (check FLOWMON_PORTS if valid)*/
	uint16_t dst_port;
	/** TCP flags (check FLOWMON_TCP if valid)*/
	uint8_t tcp_flags;
	/** ICMP type and code (check FLOWMON_ICMP if valid)*/
	uint16_t icmp_type_code;
} l4_t;

/** \def VLAN_MAX
 *  \brief Maximum space for vlan id in vlan_t
 */
#define VLAN_MAX 4
/** \struct vlan_t
 *  Structure, contains vlan ID's
 */
typedef struct {
	/** array with vlan ID's */
	uint16_t id[VLAN_MAX];
	/** count of present vlan ID's */
	uint16_t count;
} vlan_t;

/** \def MPLS_MAX
 *  \brief Maximum space for MPLS id's in mpls_t
 */
#define MPLS_MAX 8
/** \struct mpls_t
 *  Structure, contains mpls ID's
 */
typedef struct {
	/** array with mpls IS's */
	uint32_t id[MPLS_MAX];
	/** count of present mpls ID's */
	uint16_t count;
} mpls_t;

/** \struct flow_record_t
 *  Structure, contains complete record passed througth all exporter parts
 * all data are stored in host byte order except network internal data like addresses
 */
typedef struct {
	/** binary map with ones filled at valid parts. See FLOWMON_* defines, which parts are defined. Must be at first place !!!*/
	uint32_t validity_map;
	/** packet source sampling rate (if flow sampling is set, multiplied by it's value) */
	uint16_t sampling_rate;
	/** flow start time-stamp (in ms as UNIX time) */
	uint64_t flow_start;
	/** flow end timestamp (in ms as UNIX time) */
	uint64_t flow_end;
	/** flow packet count (upper allowed value is 0xFFFFF0 due to internal limits) */
	uint32_t flow_packets;
	/** flow byte count (upper allowed value is 0xFFFF0000 due to internal limits) */
	uint32_t flow_octets;
	/** flow source physical port id */
	uint16_t flow_input_interface_id;
	/** flow destination physical port id */
	uint16_t flow_output_interface_id;
	/** src_as id */
	uint32_t src_as;
	/** dst_as id */
	uint32_t dst_as;
	/** Layer 3 data (stored in network byte order at input into exporter
	    and converted to host byte order at output from flow queue) */
	l3_t l3;
	/** Layer 4 data (stored in network byte order
	    and converted to host byte order at output from flow queue) */
	l4_t l4;
#ifdef PROCESS_VLAN
	/** vlan data (not allready available). 
	    Must use record_vlan(flow_record_t*) from plugin.h to get pointer to vlan_t* or NULL if not present. */
	vlan_t vlan;
#endif
#ifdef PROCESS_MPLS
	/** mpls data (not allready available). 
	    Must use record_mpls(flow_record_t*) from plugin.h to get pointer to mpls_t* or NULL if not present. */
	mpls_t mpls;
#endif
} flow_record_t;

/**
 * Macro for zero whole record.
 * Not recommended to use.
 * Record entering into get_flow is allready zeroed
 */
#define ZERO_RECORD(r) memset(r,0,sizeof(flow_record_t));

void record_print(int level, flow_record_t * record);
uint64_t record_process_packet(unsigned char *packet, int len, flow_record_t *record);

#endif				/*FLOWMON_RECORD_H */
