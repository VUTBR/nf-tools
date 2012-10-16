
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pcap.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <math.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>

#include "record.h"
#include "plugin.h"
#include "rawnetcap.h"

#include "crc/pg_crc.h"

#define PCAP_SIZE 1500
#define SNAP_SIZE 1000 /* Doesn't work - must be set manually /sys/module/rawnetcap/parameters/packet_size */

#define RAWNETCAP_TIMEOUT 50 /* RAWNETCAP timeout in microseconds */

/* some possibly missing transport protocols */
#ifndef IPPROTO_DCCP
#define IPPROTO_DCCP    33         /* Datagram Congestion Control Protocol.  */
#endif




typedef struct {
    pcap_t *pcap;
    rawnetcap_t *rawnetcap;
    int rawnetcap_offset;
    int offline;
    int input_interface;
} plugin_private_t;

PLUGIN_HELP {
    printf("IPv6 tunnel decapsulation plugin for the FlowMon exporter.\n");
    printf("\tUsage: -E flowmon_tunnel_ipv6.so,network_device\n");
    printf("\tExample: -E flowmon_tunnel_ipv6.so,eth0\n");
}

/* Print captured packet in hex - for debugging*/
void print_packet(unsigned char *packet, uint16_t len)
{
    for (uint16_t x = 0; x <= len; x++) {
        fprintf(stderr, "%x ",(*(uint8_t*) (packet + x)));
    }
    fprintf(stderr, "\n");
    return;
}


int parse_ipv6(unsigned char *ipv6_packet, flow_record_t *record, uint16_t buf_len, uint16_t offset)
{
    uint16_t hophop_len = 0;

    if ((offset + sizeof(struct ip6_hdr)) > buf_len) { 
        return 0; 
    }
    struct ip6_hdr *ip6 = (struct ip6_hdr*) ipv6_packet;
    /* IP version 6 */
    record->l3.protocol = 6;
    /* Transport protocol  */
    record->l4.protocol = ip6->ip6_nxt;
    /* byte count 
     * Byte count for IPv4 is length of IP header + payload. Plugin counts
     * only IPv6 header length and payload of IPv6 packet to provide similar
     * results. However it means, that  IPv4, UDP and Teredo headers are
     * not counted.
     */
    record->flow_octets = ntohs(ip6->ip6_plen) + 40;
    /* IPv6 address */

    record->l3.addr.ipv6.src.ip6_addr32[0] = ntohl(ip6->ip6_src.s6_addr32[3]);
    record->l3.addr.ipv6.src.ip6_addr32[1] = ntohl(ip6->ip6_src.s6_addr32[2]);
    record->l3.addr.ipv6.src.ip6_addr32[2] = ntohl(ip6->ip6_src.s6_addr32[1]);
    record->l3.addr.ipv6.src.ip6_addr32[3] = ntohl(ip6->ip6_src.s6_addr32[0]);
    record->l3.addr.ipv6.dst.ip6_addr32[0] = ntohl(ip6->ip6_dst.s6_addr32[3]);
    record->l3.addr.ipv6.dst.ip6_addr32[1] = ntohl(ip6->ip6_dst.s6_addr32[2]);
    record->l3.addr.ipv6.dst.ip6_addr32[2] = ntohl(ip6->ip6_dst.s6_addr32[1]);
    record->l3.addr.ipv6.dst.ip6_addr32[3] = ntohl(ip6->ip6_dst.s6_addr32[0]);
    
    record->validity_map |= FLOWMON_IPV6;
    
    /* move to next header payload */
    
    if ((offset + 40) > buf_len) { 
        return 0; 
    }
    ipv6_packet += 40;    
    offset += 40;

    if (record->l4.protocol == IPPROTO_HOPOPTS) {
        if ((offset + 2) > buf_len) { 
            return 0;
        }
        record->l4.protocol = *(uint8_t*) ipv6_packet;
        hophop_len = 8 + (*(uint8_t*)(ipv6_packet +1))*8; 
        if ((offset + hophop_len) > buf_len) { 
            return 0; 
        }
        ipv6_packet += hophop_len;
        offset += hophop_len;
    }

    while (record->l4.protocol == IPPROTO_FRAGMENT) {
        if ((offset + 8) > buf_len) { 
            return 0; 
        }
        record->l4.protocol = *(uint8_t*) ipv6_packet;
        ipv6_packet += 8;
        offset += 8;
    }

    switch (record->l4.protocol) {
        case IPPROTO_TCP:
        case IPPROTO_UDP:
        case IPPROTO_SCTP:
        case IPPROTO_DCCP:
        /*
         * TCP like headers - srcport starts at begining of the header followed
         * by dstport, both as 2B
         */
            
            if ((offset + 4) > buf_len) { 
                return 0; 
            }
            record->l4.src_port = ntohs(*(uint16_t*) ipv6_packet);
            record->l4.dst_port = ntohs(*(uint16_t*) (ipv6_packet + 2));
            record->validity_map |= FLOWMON_PORTS;
            record->l4.tcp_flags = 0;
            if (record->l4.protocol == IPPROTO_TCP) {
                if ((offset + 13) > buf_len) { 
                    return 0; 
                }
                record->l4.tcp_flags = *(uint8_t*) (ipv6_packet + 13);
                record->validity_map |= FLOWMON_TCP;
            }
            break;

        case IPPROTO_ICMP:
        case IPPROTO_ICMPV6:
        /*
         * ICMP like headers - code and type in first 2 bytes
         */
            record->l4.src_port = 0;
            record->l4.dst_port = 0;

            if ((offset + 2) > buf_len) { 
                return 0; 
            }
            record->l4.icmp_type_code = ntohs(*(uint16_t*) ipv6_packet);
            record->validity_map |= FLOWMON_ICMP;
            break;
    }
    return 1;
}

/* use internal exporter func to parse packet */
uint64_t internal_parse(plugin_private_t *dev, unsigned char *buf, uint16_t buf_len, flow_record_t *record) 
{
    
    uint64_t ret = 0;
    if (dev->rawnetcap) {
        if ((ret = record_process_packet(buf + dev->rawnetcap_offset, buf_len, record)) == 0) {
            return (0);
        }        
    } else {
        if ((ret = record_process_packet(buf, buf_len, record)) == 0) {
            return (0);
        }    
    }
    record->flow_input_interface_id = dev->input_interface;
    return ret;
}



PLUGIN_INIT {
    plugin_private_t *retval;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct ifreq ifr;
    int sock;
    
    openlog("FlowMon IPv6 tunnel plugin", LOG_CONS | LOG_PERROR, LOG_USER);

    if (!params) {
        syslog(LOG_ERR,"Missing plugin parameters.");
        help();
        return (NULL);
    }

    retval = malloc(sizeof(plugin_private_t));
    if (!retval) {
        syslog(LOG_ERR, "Allocating memory failed.");
        return (NULL);
    }
    /* try to initialize RAWNETCAP */
    if ((retval->rawnetcap = rawnetcap_init(params, 0, sampling, SNAP_SIZE)) == NULL) {
        /* switch to PCAP */
        syslog(LOG_WARNING, "Unable to initialize RAWNETCAP, switching to PCAP.\n");

        /* open PCAP on specified device */
        if (access(params, R_OK) == 0) {
            retval->pcap = pcap_open_offline(params, errbuf);
            retval->offline = 1;
        } else {
            retval->pcap = pcap_open_live(params, SNAP_SIZE, 1, 0, errbuf);
            retval->offline = 0;
        }
        if (retval->pcap == NULL) {
            syslog(LOG_ERR, "Unable to init pcap: %s\n", errbuf);
            free(retval);
            return (NULL);
        }
    } else {
        /* run in RAWNETCAP mode */
        /* set offset of the packet data after rawnetcap header */
        if (rawnetcap_get_version(retval->rawnetcap) > 2) {
            /* version 3 and later returns packet data including first 12B
             * with MAC adresses
             */
            retval->rawnetcap_offset = 0;
        } else {
            /* version 2 and earlier returns packet data without first 12B
             * usually containing MAC adresses
             */
            retval->rawnetcap_offset = -12;
        }
    }

    /* get input interface ID */
    if (retval->offline == 0) {
        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, params, IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = 0;
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
            if (ioctl(sock, SIOCGIFINDEX, &ifr) >= 0) {
                retval->input_interface = ifr.ifr_ifindex;
            } else {
                syslog(LOG_ERR,	"Unable to get input interface information.\n");
                free(retval);
                return (NULL);
            }
            close(sock);
        } else {
            syslog(LOG_ERR, "Unable to prepare socket.\n");
            free(retval);
            return (NULL);
        }
    } else {
        retval->input_interface = 0;
    }   
    syslog(LOG_NOTICE, "IPv6 plugin succesfully initialized in %s mode\n on device %s with ID %d\n",
            retval->rawnetcap ? "RAWNETCAP" : retval->offline ? "PCAP offline" : "PCAP live",
            params, retval->input_interface);

    return (retval);
}

PLUGIN_GET_FLOW {
    plugin_private_t *dev = (plugin_private_t*) plugin_private;
    struct pcap_pkthdr hdr;
    unsigned char *buf; /* PCAP packet */
    packet_head_t *head; /* RAWNETCAP head */
    uint64_t ret;
//    struct ethhdr *eth;
    struct ip *ip;
    uint16_t ethtype, offset, teredo_header, udp_payload_len, udp_src_port, udp_dst_port, ipv6_len, teredo_header_len;
    uint16_t vlan_id;
    uint8_t aulen, idlen;
    uint8_t ayiya_id_len, ayiya_sig_len, ayiya_next_hdr;
    uint32_t ayiya_epoch_time;
    uint32_t time; // needed for compare with ayiya_epoch_time
    vlan_t *vrecord;

    int buf_len = 0; 
    offset = 0;
    record->src_as = 0;
    record->dst_as = 0;

    record->flow_input_interface_id = dev->input_interface;

    if (dev->rawnetcap) {
        /* RAWNETCAP mode */
        /* get the packet */
        if ((head = rawnetcap_get_next(dev->rawnetcap, RAWNETCAP_TIMEOUT)) == NULL) {
            return (0);
        }
        buf = (unsigned char*) head + sizeof(packet_head_t);
        buf_len = head->data_len;

        /* set time into flow record */
        record->flow_end = record->flow_start = (uint64_t) head->tv_sec * 1000
                                            + (uint64_t) head->tv_nsec / 1000000;
        time = head->tv_sec;
    
    } else {
        /* PCAP mode */
        /* get the packet */
        if ((buf = (unsigned char*) pcap_next(dev->pcap, &hdr)) == NULL) {
            return (0);
        }
        buf_len = hdr.caplen;
        offset += 12;
        record->flow_end = record->flow_start = (uint64_t) hdr.ts.tv_sec * 1000
                                            + (uint64_t) hdr.ts.tv_usec / 1000;
        time = hdr.ts.tv_sec;
    }

    /* get pointer to vlan */
    vrecord = record_vlan(record);
    if (vrecord != NULL) {
        vrecord->count = 1;
        vrecord->id[0] = 0;
    } else {
        syslog(LOG_ERR, "Getting vlan pointer failed\n");
        return 0;
    }
    
    //  eth = (struct ethhdr*) buf;
    if (buf_len > 2) {
        ethtype = ntohs(*(uint16_t *)(buf+offset));
        offset += 2;  // move to the beginning of ethernet payload
    } else {
        syslog(LOG_ERR, "1 parsing packet failed\n");
        return 0;
    }

    /* VLAN header
     *  16 bits  3 bits 1 bit  12 bits
     * +--------+------+-----+--------+
     * | TPID   | PCP  | CFI |   VID  |
     * +--------+------+-----+--------+
     */
    if (ethtype == ETH_P_8021Q) {
        if ((offset + 4) > buf_len) { 
            return 0;
        }
        vlan_id = ntohs(*(uint16_t *)(buf + offset));
        vrecord->id[0] += (vlan_id&0x0FFF);
        ethtype = ntohs(*(uint16_t *)(buf + offset + 2));
        offset += 4;
        record->validity_map |= FLOWMON_VLAN1;
    }

    if (ethtype == ETH_P_IP) {
        if ((offset + sizeof(struct ip)) > buf_len) { 
            return internal_parse(dev, buf, buf_len, record);
        }
        ip = (struct ip*) (buf + offset);
        if (ip->ip_p == IPPROTO_UDP) {
            offset += 4*ip->ip_hl; //move to the udp header
            
            if ((offset + 4) > buf_len) { 
                return internal_parse(dev, buf, buf_len, record);
            }
            udp_payload_len = ntohs(*(uint16_t*) (buf + offset + 4)) - 8;
            udp_src_port = ntohs(*(uint16_t*) (buf + offset)) ;
            udp_dst_port = ntohs(*(uint16_t*) (buf + offset + 2));

            offset += 8; //move to the udp payload
            if ((offset) > buf_len) {
                return internal_parse(dev, buf, buf_len, record);
            }

            /* AYIYA encapsulation */

            /* 0        3        7                 15                23               31
             * +--------+--------+--------+--------+--------+--------+----------------+
             * |  IDLen | IDType | SigLen |HshMeth |AutMeth | OpCode |   Next Header  |
             * +----------------------------------------------------------------------+
             * |                   Epoch time                                         |
             * +----------------------------------------------------------------------+
             * |                   Identity                                           |
             * +----------------------------------------------------------------------+
             * |                   Signature                                          |
             * +----------------------------------------------------------------------+
             */

            if ((udp_src_port == 5072) || (udp_dst_port == 5072)) {
                if ((offset + 8) > buf_len) {
                    return internal_parse(dev, buf, buf_len, record);
                }
                ayiya_id_len = (uint8_t)pow(2,((*(uint8_t*)(buf + offset)) >> 4));                
                ayiya_sig_len = 4 * ((*(uint8_t*)(buf + offset + 1)) >> 4);
                ayiya_next_hdr = (*(uint8_t*)(buf + offset + 3));
                ayiya_epoch_time = ntohl((*(uint32_t*)(buf + offset + 4)));
                offset += 8 + ayiya_id_len + ayiya_sig_len;
                if ((offset) > buf_len) {
                    return internal_parse(dev, buf, buf_len, record);
                }
                if (abs(ayiya_epoch_time - time) > 120) { //difference in time is qreater than allowed - probably not ayiya packet                        
                    return internal_parse(dev, buf, buf_len, record);
                }
                if ((ret = parse_ipv6(buf + offset, record, buf_len, offset)) == 0) {
                    return internal_parse(dev, buf, buf_len, record);
                }
                    record->flow_output_interface_id = 5;
                   
                    record->src_as = ntohl((uint32_t) ip->ip_src.s_addr);
                    record->dst_as = ntohl((uint32_t) ip->ip_dst.s_addr);
                   
                    INIT_CRC64(ret);
                    UPDATE_CRC64(ret, &record->l3.addr.ipv6.src.ip6_addr64[0], sizeof (record->l3.addr.ipv6.src.ip6_addr64[0]));
                    UPDATE_CRC64(ret, &record->l3.addr.ipv6.dst.ip6_addr64[0], sizeof (record->l3.addr.ipv6.dst.ip6_addr64[0]));
                    UPDATE_CRC64(ret, &record->l4.protocol, sizeof (record->l4.protocol));
                    UPDATE_CRC64(ret, &record->l4.src_port, sizeof (record->l4.src_port));
                    UPDATE_CRC64(ret, &record->l4.dst_port, sizeof (record->l4.dst_port));
                    FIN_CRC64(ret);
            
                    /* set packet count to 1 */
                    record->flow_packets = 1;
                    return(ret | FLOW_OK_BIT);                    
            }

            /* Teredo Authentication is indicated by 0x00 0x01
                +------+-----+----------------+-------------+
                | IPv4 | UDP | Authentication | IPv6 packet |
                +------+-----+----------------+-------------+
                +------+-----+----------------+--------+-------------+
                | IPv4 | UDP | Authentication | Origin | IPv6 packet |
                +------+-----+----------------+--------+-------------+
            */

            teredo_header = ntohs(*(uint16_t *)(buf + offset));
            teredo_header_len = 0;
            if (teredo_header == 0x0001) {
                if ((offset + 3) > buf_len) { 
                    return internal_parse(dev, buf, buf_len, record);
                }
                idlen = *(uint8_t*)(buf + offset + 2);
                aulen = *(uint8_t*)(buf + offset + 3);
                /* type + id-len + au-len + nonce + confirmation byte */
                teredo_header_len = 4 + idlen + aulen + 8 + 1;
                offset += teredo_header_len; // should be at the begining of ipv6 packet
                if ((offset) > buf_len) { 
                    return internal_parse(dev, buf, buf_len, record);
                }
                teredo_header = ntohs(*(uint16_t *)(buf + offset));
            }

            if (teredo_header == 0x0000) {
                teredo_header_len += 8;
                offset += 8; // should be at the begining of ipv6 packet
                if ((offset) > buf_len) {
                    return internal_parse(dev, buf, buf_len, record);
                }
                teredo_header = ntohs(*(uint16_t *)(buf + offset));
            }
            
            /* nothing to unpack
               +------+-----+-------------+
               | IPv4 | UDP | IPv6 packet |
               +------+-----+-------------+
            */
             
            if (((teredo_header >> 12) == 6)) {
                if ((offset + 4) > buf_len) { 
                    return internal_parse(dev, buf, buf_len, record);
                }
                ipv6_len = ntohs(*(uint16_t *)(buf + offset + 4));
                if ((udp_payload_len - teredo_header_len) == (ipv6_len + 40)) {
                    //fprintf(stderr, "udp\n");
                    if ((ret = parse_ipv6(buf + offset, record, buf_len, offset)) == 0) { 
                        return internal_parse(dev, buf, buf_len, record);
                    } 
                    if ((udp_src_port == 3544) || (udp_dst_port == 3544)
                        || (record->l3.addr.ipv6.src.ip6_addr32[3] == 0x20010000)
                        || (record->l3.addr.ipv6.dst.ip6_addr32[3] == 0x20010000)) {
    
                        record->flow_output_interface_id = 2;
                    
                        record->src_as = ntohl((uint32_t) ip->ip_src.s_addr);
                        record->dst_as = ntohl((uint32_t) ip->ip_dst.s_addr);
                        
                        INIT_CRC64(ret);
                        UPDATE_CRC64(ret, &record->l3.addr.ipv6.src.ip6_addr64[0], sizeof (record->l3.addr.ipv6.src.ip6_addr64[0]));
                        UPDATE_CRC64(ret, &record->l3.addr.ipv6.dst.ip6_addr64[0], sizeof (record->l3.addr.ipv6.dst.ip6_addr64[0]));
                        UPDATE_CRC64(ret, &record->l4.protocol, sizeof (record->l4.protocol));
                        UPDATE_CRC64(ret, &record->l4.src_port, sizeof (record->l4.src_port));
                        UPDATE_CRC64(ret, &record->l4.dst_port, sizeof (record->l4.dst_port));
                        FIN_CRC64(ret);
            
                        /* set packet count to 1 */
                        record->flow_packets = 1;
                        return(ret | FLOW_OK_BIT);
                    } 
                } 
            } 
        } else 
         if (ip->ip_p == 41) {
            offset += 4*ip->ip_hl;
            if ((ret = parse_ipv6(buf + offset, record, buf_len, offset)) == 0) {
                return internal_parse(dev, buf, buf_len, record);
            }    
            //parse_ipv6(buf + offset, record, buf_len, offset);            
            record->src_as = ntohl((uint32_t) ip->ip_src.s_addr);
            record->dst_as = ntohl((uint32_t) ip->ip_dst.s_addr);
            /* 6to4 src */
            if (record->l3.addr.ipv6.src.ip6_addr16[7] == 0x2002) {
                record->flow_output_interface_id = 4;
            
            /* ISATAP - address is in format ...5efe:IPv4_address */
            } 
            else if (record->l3.addr.ipv6.src.ip6_addr16[2] == 0x5efe) {
                    record->flow_output_interface_id = 3;
                 }
            /* 6to4 dst */
            else if (record->l3.addr.ipv6.dst.ip6_addr16[7] == 0x2002) {
                    record->flow_output_interface_id = 4;
                 }
            /* ISATAP dst */
            else if (record->l3.addr.ipv6.dst.ip6_addr16[2] == 0x5efe) {
                    record->flow_output_interface_id = 3;
            /* 3227017985 = 192.88.99.1 = 6to4 relay anycast address */
            } else if ((record->src_as == 3227017985) || (record->dst_as == 3227017985)) {
                    record->flow_output_interface_id = 4;
            } else {
                    record->flow_output_interface_id = 6;
            }
        
            INIT_CRC64(ret);
            UPDATE_CRC64(ret, &record->l3.addr.ipv6.src.ip6_addr64[0], sizeof (record->l3.addr.ipv6.src.ip6_addr64[0]));
            UPDATE_CRC64(ret, &record->l3.addr.ipv6.dst.ip6_addr64[0], sizeof (record->l3.addr.ipv6.dst.ip6_addr64[0]));
            UPDATE_CRC64(ret, &record->l4.protocol, sizeof (record->l4.protocol));
            UPDATE_CRC64(ret, &record->l4.src_port, sizeof (record->l4.src_port));
            UPDATE_CRC64(ret, &record->l4.dst_port, sizeof (record->l4.dst_port));

            FIN_CRC64(ret);

            record->flow_packets = 1;
            return(ret | FLOW_OK_BIT);
        }
    } else if (ethtype == ETH_P_IPV6) {
        if ((ret = parse_ipv6(buf + offset, record, buf_len, offset)) == 0) { 
            return internal_parse(dev, buf, buf_len, record);
        }
        record->flow_output_interface_id = 1;

        INIT_CRC64(ret);
        UPDATE_CRC64(ret, &record->l3.addr.ipv6.src.ip6_addr64[0], sizeof (record->l3.addr.ipv6.src.ip6_addr64[0]));
        UPDATE_CRC64(ret, &record->l3.addr.ipv6.dst.ip6_addr64[0], sizeof (record->l3.addr.ipv6.dst.ip6_addr64[0]));
        UPDATE_CRC64(ret, &record->l4.protocol, sizeof (record->l4.protocol));
        UPDATE_CRC64(ret, &record->l4.src_port, sizeof (record->l4.src_port));
        UPDATE_CRC64(ret, &record->l4.dst_port, sizeof (record->l4.dst_port));
        
        FIN_CRC64(ret);
        
        /* set packet count to 1 */
        record->flow_packets = 1;
        return(ret | FLOW_OK_BIT);
    }
    return internal_parse(dev, buf, buf_len, record);
}
