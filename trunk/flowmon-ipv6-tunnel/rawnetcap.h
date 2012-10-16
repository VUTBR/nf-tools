/* -----------------------------------------------------------------------
*
*    Company: INVEA-TECH a.s.
*
*    Project: Invea FlowMon Exporter
*
*    $Id: rawnetcap.h 12 2011-06-16 11:24:07Z celeda $
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

#ifndef RAWNETCAP_H
#define RAWNETCAP_H
#include <stdint.h>
#include <stddef.h>
#include <poll.h>

typedef struct{
	int fd;
	struct pollfd pfd;
	int cap_len;
	int queue_size;
	unsigned char *queue;
	int	queue_pointer;
	int queue_ready;
	int sampling;
}rawnetcap_t;

#define ALIGN8(what) (((what) + ((8)-1)) & (~((8)-1)))

typedef struct{
	u_int32_t tv_sec;
	u_int32_t tv_nsec;
	u_int16_t data_len;
	u_int16_t real_len;
	u_int16_t interface_id;
	u_int16_t fill_up;
}__attribute__((__packed__))packet_head_t;



rawnetcap_t *rawnetcap_init(char *device_list, int transparent, int sampling, int snaplen);
int rawnetcap_get_version(rawnetcap_t *cap);
packet_head_t *rawnetcap_get_next(rawnetcap_t *cap, int timeout);

#endif
