# -----------------------------------------------------------------------
#
#    Company: INVEA-TECH a.s.
#
#    Project: Invea Ethernet Monitoring Plugin
#
#    $Id: Makefile 12 2011-06-16 11:24:07Z celeda $
#
# -----------------------------------------------------------------------
#
#    (c) Copyright 2010 INVEA-TECH a.s.
#    All rights reserved.
#
#    Please review the terms of the license agreement before using this
#    file. If you are not an authorized user, please destroy this
#    source code file and notify INVEA-TECH a.s. immediately that you
#    inadvertently received an unauthorized copy.
#
# -----------------------------------------------------------------------

PLUGIN = flowmon_tunnel_ipv6
PLUGIN_OBJS = tunnel_ipv6.o crc/pg_crc.o

#DEBUG = -DDEBUG
CPPFLAGS = -DFLOWMON_PLUGIN 
CFLAGS = -Wall -g -fPIC -pedantic -std=gnu99 -O2 $(DEBUG)
LDFLAGS = -lpcap -lm

all: $(PLUGIN).so 

$(PLUGIN).so: $(PLUGIN_OBJS)
	gcc -shared -o $(PLUGIN).so $(LDFLAGS) $(PLUGIN_OBJS)

clean:
	-rm $(PLUGIN_OBJS) $(PLUGIN).so

