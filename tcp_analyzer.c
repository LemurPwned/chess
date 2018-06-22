#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <unistd.h>
#include <ctype.h>


#define SA struct sockaddr
//#define MYPTNO 0x0800 //IPv4
#define MYPTNO ETH_P_ALL
//#define MYPTNO ETH_P_IPV6

void print_flag(int flag){
	switch(flag){
		case 0x01:
			printf("TH_FIN");
			break;
		case 0x02:
			printf("TH_SYN");
			break;
		case 0x04:
			printf("TH_RST");
			break;
		case 0x08:
			printf("TH_PUSH");
			break;
		case 0x10:
			printf("TH_ACK");
			break;
		case 0x20:
			printf("TH_URG");
			break;
		default:
			printf("COMBINATION OF FLAGS");
			break;
	}
	printf("\n");
}

int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_ll	servaddr, cliaddr;
    int if_idx;

    char *name="wlp3s0";

	// if (argc != 2){
	// 	fprintf(stderr, "usage: %s <Interface name> \n", argv[0]);
	// 	return 1;
	// }

    //destination's ifindex and addr is known

    if( (if_idx =  if_nametoindex(name)) == 0 ){
    	perror("interface name error:");
    	return 1;
    }

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(MYPTNO));
	if( sockfd < 0 )
		perror("socket error:");


    struct packet_mreq mreq;
	bzero(&mreq, sizeof(struct packet_mreq));
    mreq.mr_ifindex = if_idx;
    mreq.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(struct packet_mreq)) < 0){
        perror("Failed to set PROMISCUOUS MODE");
        return -1;
    }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sll_family = AF_PACKET;
	servaddr.sll_protocol = htons(MYPTNO);
//	servaddr.sll_ifindex = 0;//bind to every interface
	servaddr.sll_ifindex = if_idx;

	// if( bind(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0 )
	// 	perror("bind error:");

	bzero(&cliaddr, sizeof(cliaddr));

	char buff[2048];
	int n = 0;
	int i=0,j =0;
	for(;;){
		int length=sizeof(cliaddr);
		n = recvfrom(sockfd, buff, 2048, 0, (SA*)&cliaddr, &length);
		if( n < 0 )
			perror("socket error:");
		else{
			buff[n] = 0;
			j++;

			// struct ethhdr {
			//         unsigned char   h_dest[ETH_ALEN];       /* destination eth addr */
			//         unsigned char   h_source[ETH_ALEN];     /* source ether addr    */
			//         __be16          h_proto;                /* packet type ID field */
			// } __attribute__((packed));

			struct ethhdr *hdr;
			struct ip *ipv4hdr;
			struct udphdr *uhdr;
			struct tcphdr *thdr;

			hdr = (struct ethhdr *)buff;
			ipv4hdr = (struct ip *)(buff+sizeof(struct ethhdr));
			// uhdr = (struct udphdr *)(buff+sizeof(struct ethhdr)+sizeof(struct ip));
			thdr = (struct tcphdr *)(buff + sizeof(struct ethhdr) + sizeof(struct ip));
  			printf("Ethernet proto: %x\n", ntohs(hdr->h_proto));
			printf("IP protocol: %x\n", ipv4hdr->ip_p);
			fflush(stdout);

			if( (ntohs(hdr->h_proto) == ETHERTYPE_IP) &&  (ipv4hdr->ip_p == IPPROTO_TCP) ){

				printf("SRC MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
					(int) hdr->h_source[0], (int) hdr->h_source[1], (int) hdr->h_source[2],
					(int) hdr->h_source[3], (int) hdr->h_source[4],(int) hdr->h_source[5] );
				printf("DST MAC addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
					(int) hdr->h_dest[0], (int) hdr->h_dest[1], (int) hdr->h_dest[2],
					(int) hdr->h_dest[3], (int) hdr->h_dest[4],(int) hdr->h_dest[5] );
				printf("Proto = 0x%04x\n", ntohs( hdr->h_proto));

				printf("IP src addr = %s\n", inet_ntoa( ipv4hdr->ip_src));
				printf("IP dst addr = %s\n", inet_ntoa( ipv4hdr->ip_dst));
				printf("TCP src port = %u\n", ntohs( thdr->th_sport));
				printf("TCP dst port = %u\n", ntohs( thdr->th_dport));

				char *out=buff+sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct tcphdr);
				int k=0;
				printf("TCP FLAGS %x\n", thdr->th_flags);
				print_flag(thdr->th_flags);
				printf("DATA = ");
				
				for(k=0; k< (n-sizeof(struct ethhdr)+sizeof(struct ip)+sizeof(struct tcphdr)); k++){
					if((isprint(out[k])))
						printf("%c",out[k] );
					else
						printf("-");		
				}
				printf("\n");
				fflush(stdout);
				i++;
				if( i > 10)
					break;
			}
		}
	}

	printf("\nReceived %d packets\n",j);

	return 0;
}

