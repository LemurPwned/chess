#include <sys/socket.h>
#include <sys/capability.h>
#include <linux/capability.h>

#include <netpacket/packet.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h> 
#include <netinet/ether.h>
#include <net/ethernet.h>
#include <net/if.h>

#include <arpa/inet.h> 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define IP_FILTER 0x01
#define TCP_FILTER 0x02

struct arguments
{
  int packets;
  int max_length;
  int min_length;

  int src_port;            
  int dst_port;              
  char *ip_source;
  char *ip_dest;            
  char *interface;
  bool data_dump;
  bool promiscuous;

  char *out_file;
};

enum FLAG_TYPE{
    FIN = 0x01,
    SYNC = 0x02,
    ACK = 0x10,
    PUSH = 0x08,
    RST = 0x04,
    URG = 0x20,
};

struct flag_stats{
    int fin;
    int ack;
    int syn;
    int push;
    int rst;
    int urg;
};

struct ipv4_addr_stat{
    int acount;
    char **dst_addr_stack;
    char **src_addr_stack;
    int rep_dst[100];
    int rep_src[100];
};

struct param_stat{
    int window;
};

struct tcp_stat{
    int packet_count;
    int passed_packet;
    struct flag_stats fstat;
    struct ipv4_addr_stat astat;
    struct param_stat pstat;
};

struct packet_filter{
    char *ip_src;
    char *ip_dst;
    int tcp_src;
    int tcp_dst;
};

void ipv4_addres_collect(struct ip*, struct tcp_stat*);
void deduce_flag(int, char flag_buffer[64], struct tcp_stat*);
int address_in_stack(char **, char *, int);

int filter_packets(struct ip *ip_hdr, struct tcphdr* tcp_hdr,
				   struct packet_filter *filter);
int run_analyser(struct arguments *);