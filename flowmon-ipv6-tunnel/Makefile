# -----------------------------------------------------------------------
# Written by: Pavel Celeda
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

