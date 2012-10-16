IPv6 TUNNEL MONITORING PLUG-IN

Author: Matej Gregr <igregr@fit.vutbr.cz>

Thank Martin Elich and Pavel Celeda for help with initial plugin development.

DESCRIPTION:
IPv6 tunnel monitoring adapted for BUT network. Plugin is able to monitor IPv4, native IPv6,
Teredo, ISATAP, 6to4 and AYIYA traffic.


REQUIREMENTS:
- flowmonexp version 2.51.07 and newer with compile time options: -DPROCESS_VLAN -DPLUGIN -DPLUGIN_INPUT
- tested with flowmonexp version 2.64.02

EXAMPLE OF USAGE:
flowmonexp -u flowmon -d -p v9 -E/home/flowmon/flowmon_tunnel_ipv6.so,eth2  -T template 127.0.0.1:9901

EXPORTED FLOW DATA

INPUT_SNMP          Input interface
OUTPUT_SNMP         tunnel type:    1 - native
                                    2 - Teredo
                                    3 - ISATAP
                                    4 - 6to4
                                    5 - AYIYA
                                    6 - other proto 41 traffic
FIRST_SWITCHED      Timestamp of flow beginning
LAST_SWITCHED       Timestamp of flow beginning
IN_BYTES, OUT_BYTES Bytes count transfered by tunneled flow or IPv4 traffic
IN_PKTS, OUT_PKTS   Packet count transfered by tunneled flow or IPv4 traffic
IP_PROTOCOL_VERSION IP version
PROTOCOL            L4 protocol
IPV6_SRC_ADDR       Source IPv6 address.
IPV6_DST_ADDR       Destination IPv6 address.
L4_SRC_PORT         Source port number
L4_DST_PORT         Destination port
TCP_FLAGS           TCP flags
SRC_AS32            Source IP address of IPv4 envelope. Unset for IPv4 traffic
DST_AS32            Destination IP address of IPv4 envelope. Unset for IPv4 traffic
ICMP_TYPE_CODE      ICMP type and code
VLAN_ID             VLAN
