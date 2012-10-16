/* -----------------------------------------------------------------------
*
*    Company: INVEA-TECH a.s.
*
*    Project: Invea FlowMon Exporter
*
*    $Id: plugin.h 12 2011-06-16 11:24:07Z celeda $
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <stdint.h>
#include <stddef.h>
#include "record.h"

/**
 * \file plugin.h
 * \brief Header contains extrenal plugins struture functions and defines
 */


/**
 * \struct plugin_input_ops_t
 * \brief input plugin API functions
 */
typedef struct{
	/** init function */
	void			*(*init)			(char *params, int sampling);
	/** help function which printf help to stdout */
	void			(*help)				();
	/** returns nonfinished flow */
	uint64_t		(*get_flow)			(void *plugin_private, flow_record_t * record);
	/** returns complete flow */
	int				(*get_final_flow)	(void *plugin_private, flow_record_t * record);
	/** returns packet (including ETHERNET part) */
	unsigned char	*(*get_packet)		(void * plugin_private, struct timeval *tv, unsigned int *len);
	/** params which are passed to input function */
	char *params;
} plugin_input_ops_t;

/**
 * \struct plugin_ops_t
 * \brief base plugin operations structure
 */
typedef struct{
	/** input API functions. Only one input plugin at one time is allowed */
	plugin_input_ops_t input;
}plugin_ops_t;

mpls_t *record_mpls(flow_record_t * record);
vlan_t *record_vlan(flow_record_t * record);

/** \def PLUGIN_INIT
 define flow ok bit which must be set for identifying valid flow, to use in plugin code */
#define FLOW_OK_BIT 0x8000000000000000ULL

/** \def PLUGIN_INIT
 define init function prototype, to use in plugin code */
#define PLUGIN_INIT void *init (char *params, int sampling)
/** \def PLUGIN_HELP
 define print help function prototype, to use in plugin code */
#define PLUGIN_HELP void help ()
/** \def PLUGIN_GET_FLOW
 define get_flow prototype, to use in plugin code */
#define PLUGIN_GET_FLOW uint64_t get_flow (void *plugin_private, flow_record_t * record)
/** \def PLUGIN_GET_FINAL_FLOW
 define get_final_flow prototype, to use in plugin code */
#define PLUGIN_GET_FINAL_FLOW int get_final_flow (void *plugin_private, flow_record_t * record)
/** \def PLUGIN_GET_PACKET
 define get_packet prototype, to use in plugin code */
#define PLUGIN_GET_PACKET unsigned char *get_packet (void * plugin_private, struct timeval *tv, unsigned int *len)

#endif				/*FLOWMON_RECORD_H */
