#include "tcp_analyzer.h"


#define SA struct sockaddr
//#define MYPTNO 0x0800 //IPv4
//#define MYPTNO ETH_P_IPV6

void ipv4_addres_collect(struct ip* ip_header, struct tcp_stat* stat){
    stat->astat.src_addr_stack[stat->packet_count] = (char *) malloc(sizeof(char)*200);
    stat->astat.dst_addr_stack[stat->packet_count] = (char *) malloc(sizeof(char)*200);
    char *addr = inet_ntoa(ip_header->ip_src);
	if (!address_in_stack(stat->astat.src_addr_stack, addr, stat->astat.acount)){
    	strcpy(stat->astat.src_addr_stack[stat->packet_count], addr);
		stat->astat.acount++;
	}
    addr = inet_ntoa(ip_header->ip_dst);
	if (!address_in_stack(stat->astat.src_addr_stack, addr, stat->astat.acount)){
    	strcpy(stat->astat.dst_addr_stack[stat->packet_count], addr);
		stat->astat.acount++;
	}
}

int address_in_stack(char **address_stack, char *new_address, int max_address){
    for (int i = 0; i < max_address; i++){
        if (!strcmp(address_stack[i], new_address)){
            return 1;
        }
    }
    return 0;
}

void deduce_flag(int flag, char flag_buffer[64], struct tcp_stat* stat){
	bzero(flag_buffer, sizeof(char)*64);
	strcpy(flag_buffer, "");
	if (flag & 0x01){
		strcat(flag_buffer, "FIN ");
		stat->fstat.fin++;
	}
	if (flag & 0x02){
		strcat(flag_buffer, "SYN ");
		stat->fstat.syn ++;
	}
	if (flag & 0x04){
		strcat(flag_buffer, "RST ");
		stat->fstat.rst ++;
	}
	if (flag & 0x08){
		strcat(flag_buffer, "PUSH ");
		stat->fstat.push ++;
	}
	if (flag & 0x10){
		strcat(flag_buffer, "ACK ");
		stat->fstat.ack ++;
	}
	if (flag & 0x20){
		strcat(flag_buffer, "URG ");
		stat->fstat.urg ++;
	}
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

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if( sockfd < 0 )
		perror("socket error:");

	if (strcmp(argv[1], "promsc") == 0){
		printf("Turning on promisc mode...\n");
		struct packet_mreq mreq;
		bzero(&mreq, sizeof(struct packet_mreq));
		mreq.mr_ifindex = if_idx;
		mreq.mr_type = PACKET_MR_PROMISC;
		if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(struct packet_mreq)) < 0){
			perror("Failed to set PROMISCUOUS MODE");
			return -1;
		}
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sll_family = AF_PACKET;
	servaddr.sll_protocol = htons(ETH_P_ALL);
//	servaddr.sll_ifindex = 0;//bind to every interface
	servaddr.sll_ifindex = if_idx;

	bzero(&cliaddr, sizeof(cliaddr));

	char buff[2048];
	char flag_buffer[64];
	int n = 0;
	int i=0,j =0;

	struct tcp_stat tstat;
	bzero(&tstat, sizeof(struct tcp_stat));
	tstat.astat.src_addr_stack =  (char **)malloc(sizeof(char *)*100);
	tstat.astat.dst_addr_stack =  (char **)malloc(sizeof(char *)*100);
	

	while(1){
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
			struct tcphdr *thdr;

			hdr = (struct ethhdr *)buff;
			ipv4hdr = (struct ip *)(buff+sizeof(struct ethhdr));
			thdr = (struct tcphdr *)(buff + sizeof(struct ethhdr) + sizeof(struct ip));
  			// printf("Ethernet proto: %x\n", ntohs(hdr->h_proto));
			// printf("IP protocol: %x\n", ipv4hdr->ip_p);
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

				ipv4_addres_collect(ipv4hdr, &tstat);
				deduce_flag(thdr->th_flags, flag_buffer, &tstat);

				printf("FLAGS: %s\n", flag_buffer);
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
				tstat.packet_count ++;
				if( i > 10)
					break;
			}
		}
	}

	printf("\nReceived %d packets\n", tstat.packet_count);
	printf("TCP FLAGS: ACK %d, PUSH %d, SYNC %d\n", tstat.fstat.ack, 
													tstat.fstat.push,
													tstat.fstat.syn);
	for (int i=0; i < tstat.astat.acount; i++){
		printf("SRC: %s\n", tstat.astat.src_addr_stack[i]);
		printf("DST: %s\n", tstat.astat.dst_addr_stack[i]);
	}
	return 0;
}

