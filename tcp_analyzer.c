#include "tcp_analyzer.h"


void ipv4_addres_collect(struct ip* ip_header, struct tcp_stat* stat){
    stat->astat.src_addr_stack[stat->packet_count] = (char *) malloc(sizeof(char)*200);
    stat->astat.dst_addr_stack[stat->packet_count] = (char *) malloc(sizeof(char)*200);
    char *addr = inet_ntoa(ip_header->ip_src);
	if (!address_in_stack(stat->astat.src_addr_stack, addr, 
											stat->astat.acount)){
    	strcpy(stat->astat.src_addr_stack[stat->packet_count], addr);
		stat->astat.acount++;
	}
    addr = inet_ntoa(ip_header->ip_dst);
	if (!address_in_stack(stat->astat.src_addr_stack, addr, 
											stat->astat.acount)){
    	strcpy(stat->astat.dst_addr_stack[stat->packet_count], addr);
		stat->astat.acount++;
	}
}

int address_in_stack(char **address_stack, char *new_address, 
				int max_address){
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

int filter_packets(struct ip *ip_hdr, struct tcphdr* tcp_hdr, struct packet_filter *filter){
	if (strcmp(filter->ip_src, "") && 
		strcmp(inet_ntoa(ip_hdr->ip_src), filter->ip_src) != 0){
		printf("FAILED IP SRC");
		return 0;
	}
	if (strcmp(filter->ip_dst, "") && 
		strcmp(inet_ntoa(ip_hdr->ip_dst), filter->ip_dst) != 0){
		printf("FAILED IP DST");
		return 0;
	}
	if (filter->tcp_src && 
		ntohs(tcp_hdr->th_sport) != filter->tcp_src){
		printf("FAILED TCP SRC");
		return 0;
	}
	if (filter->tcp_dst &&
		ntohs(tcp_hdr->th_dport) != filter->tcp_dst) {
		printf("FAILED TCP DST");
		return 0;
	}
	else{
		return 1;
	}
	return 0;
}


int run_analyser(struct arguments *args){
	int	sockfd;
	struct sockaddr_ll	servaddr, cliaddr;
    int if_idx;

    if( (if_idx =  if_nametoindex(args->interface)) == 0 ){
    	perror("interface name error:");
    	// return 1;
    }

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if( sockfd < 0 ){
		perror("socket error:");
	}
	if (args->promiscuous){
		printf("%s\n", "Turning on promisc mode...");
		struct packet_mreq mreq;
		bzero(&mreq, sizeof(struct packet_mreq));
		mreq.mr_ifindex = if_idx;
		mreq.mr_type = PACKET_MR_PROMISC;
		if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, 
				&mreq, sizeof(struct packet_mreq)) < 0){
			perror("Failed to set PROMISCUOUS MODE");
			return -1;
		}
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sll_family = AF_PACKET;
	servaddr.sll_protocol = htons(ETH_P_ALL);
	servaddr.sll_ifindex = if_idx;

	bzero(&cliaddr, sizeof(cliaddr));

	char buffer[2048];
	char flag_buffer[64];
	int n = 0;

	struct tcp_stat tstat;
	bzero(&tstat, sizeof(struct tcp_stat));
	tstat.astat.src_addr_stack =  (char **)malloc(sizeof(char *)*100);
	tstat.astat.dst_addr_stack =  (char **)malloc(sizeof(char *)*100);
	
	struct packet_filter pkt_filter = {
		args->ip_source, 
		args->ip_dest,
		args->src_port,
		args->dst_port
	};

	while(1){
		int length=sizeof(cliaddr);
		n = recvfrom(sockfd, buffer, 2048, 0, 
					(struct sockaddr*)&cliaddr, &length);
		if( n < 0 )
			perror("socket error:");
		else{
			buffer[n] = 0;
			struct ethhdr *hdr;
			struct ip *ipv4hdr;
			struct tcphdr *thdr;

			hdr = (struct ethhdr *)buffer;
			ipv4hdr = (struct ip *)(buffer+sizeof(struct ethhdr));
			thdr = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + sizeof(struct ip));
			if( (ntohs(hdr->h_proto) == ETHERTYPE_IP) &&  (ipv4hdr->ip_p == IPPROTO_TCP) ){
				if (filter_packets(&ipv4hdr, &thdr, &pkt_filter)){		
					ipv4_addres_collect(ipv4hdr, &tstat);
					deduce_flag(thdr->th_flags, flag_buffer, &tstat);
					if (args->data_dump){
						printf("%s", "DATA = ");
						char *data = buffer + sizeof(struct ethhdr) +
									 sizeof(struct ip) + sizeof(struct tcphdr);
						for(int k=0; k< (n-sizeof(struct ethhdr)+
								sizeof(struct ip)+sizeof(struct tcphdr)); k++){
							if(isprint(data[k]))
								printf("%c",data[k] );
							else
								printf("%s", "-");		
						}
						printf("%s", "\n");
					}	
					fflush(stdout);			
				}
				tstat.packet_count++;
				if( tstat.packet_count > args->packets)
					break;
			}
		}
	}

	printf("\nReceived %d packets\n", tstat.packet_count);
	printf("TCP FLAGS: ACK %d, PUSH %d, SYNC %d\n", tstat.fstat.ack, 
													tstat.fstat.push,
													tstat.fstat.syn);
	printf("WHATEWAH");
	printf("ADDRESS STACK: %d\n", tstat.astat.acount);
	for (int i=0; i < tstat.astat.acount; i++){
		printf("SRC: %s\n", tstat.astat.src_addr_stack[i]);
		printf("DST: %s\n", tstat.astat.dst_addr_stack[i]);
	}
	return 0;
}

