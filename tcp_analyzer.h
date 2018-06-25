#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

#include <arpa/inet.h> 
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/in.h> 
#include <net/ethernet.h>
#include <net/if.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>



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
};

struct param_stat{
    int window;
};
struct tcp_stat{
    int packet_count;
    struct flag_stats fstat;
    struct ipv4_addr_stat astat;
    struct param_stat pstat;
};

void deduce_flag(int, char flag_buffer[64], struct tcp_stat*);
void ipv4_addres_collect(struct ip*, struct tcp_stat*);
int address_in_stack(char **, char *, int);