#include "tcp_analyzer.h"


void ipv4_addres_collect(struct ip* ip_header, struct tcp_stat* stat){
    stat->astat.src_addr_stack[stat->astat.acount] = (char *) malloc(sizeof(char)*200);
    stat->astat.dst_addr_stack[stat->astat.acount] = (char *) malloc(sizeof(char)*200);
    char *src_addr = inet_ntoa(ip_header->ip_src);
	char *dst_addr = inet_ntoa(ip_header->ip_dst);
	int a = address_in_stack(stat->astat.src_addr_stack, src_addr, 
											stat->astat.acount);
	int b = address_in_stack(stat->astat.dst_addr_stack, dst_addr, 
											stat->astat.acount);
	if (!a && !b){
    	strcpy(stat->astat.src_addr_stack[stat->astat.acount], src_addr);
    	strcpy(stat->astat.dst_addr_stack[stat->astat.acount], dst_addr);
		stat->astat.rep_src[a] = 1;
		stat->astat.rep_dst[b] = 1;
		stat->astat.acount++;
	}
	else{
		stat->astat.rep_src[a]++;
		stat->astat.rep_dst[b]++;
	}
}

int address_in_stack(char **address_stack, char *new_address, 
					 int max_address){
    for (int i = 0; i < max_address; i++){
        if (!strcmp(address_stack[i], new_address)){
            return i;
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
		printf("FAILED IP SRC\n");
		return 0;
	}
	if (strcmp(filter->ip_dst, "") && 
		strcmp(inet_ntoa(ip_hdr->ip_dst), filter->ip_dst) != 0){
		printf("FAILED IP DST\n");
		return 0;
	}
	if (filter->tcp_src && 
		ntohs(tcp_hdr->th_sport) != filter->tcp_src){
		printf("FAILED TCP SRC\n");
		return 0;
	}
	if (filter->tcp_dst &&
		ntohs(tcp_hdr->th_dport) != filter->tcp_dst) {
		printf("FAILED TCP DST\n");
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

	FILE *f;
	cap_t cap = cap_get_proc(); 
	cap_flag_value_t v = 0; 
	cap_get_flag(cap, CAP_NET_ADMIN, CAP_EFFECTIVE, &v);
	if (!v){
		fprintf(stderr, "%s\n",
				"Please run with admin/sudo privileges");
		exit(-1);
	}
	if (strcmp(args->out_file, "") != 0){
		f = fopen(args->out_file, "w");
		if (f == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}
	}
	else{
		// just print to the screen
		f = stdout;
	}

    if( (if_idx =  if_nametoindex(args->interface)) == 0 ){
    	perror("interface name error:");
    	// return 1;
    }

	sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if( sockfd < 0 ){
		perror("socket error:");
	}
	if (args->promiscuous){
		fprintf(f, "%s\n", "Turning on promisc mode...");
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
			if (n > args->max_length || n < args->min_length){
				fprintf(f, "%d, %s\n", args->max_length, "Packet size invalid, skipping...");
				continue;
			}
			struct ethhdr *eth;
			struct ip *ipv4hdr;
			struct tcphdr *thdr;

			eth = (struct ethhdr *)buffer;
			ipv4hdr = (struct ip *)(buffer+sizeof(struct ethhdr));
			thdr = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + sizeof(struct ip));
			if( (ntohs(eth->h_proto) == ETHERTYPE_IP) && (ipv4hdr->ip_p == IPPROTO_TCP) ){
				fprintf(f, "%s\n", "-----------------------");
				if (filter_packets(ipv4hdr, thdr, &pkt_filter)){	
					fprintf(f, "PACKET NO. %d\n", tstat.packet_count);
					fprintf(f, "LENGTH: %d\n", n);
					fprintf(f, "IP_SRC: %s\nIP_DST: %s\n", inet_ntoa(ipv4hdr->ip_src),
														   inet_ntoa(ipv4hdr->ip_dst));
					fprintf(f, "MAC_SRC: %s\nMAC_DST: %s\n", ether_ntoa(eth->h_dest),
														     ether_ntoa(eth->h_source));
					ipv4_addres_collect(ipv4hdr, &tstat);
					deduce_flag(thdr->th_flags, flag_buffer, &tstat);
					fprintf(f, "FLAGS: %s\n", flag_buffer);
					if (args->data_dump){
						fprintf(f, "%s", "DATA = ");
						char *data = buffer + sizeof(struct ethhdr) +
									 sizeof(struct ip) + sizeof(struct tcphdr);
						for(int k=0; k< (n-sizeof(struct ethhdr)+
								sizeof(struct ip)+sizeof(struct tcphdr)); k++){
							if(isprint(data[k]))
								fprintf(f, "%c",data[k]);
							else
								fprintf(f,"%s", "-");		
						}
						fprintf(f, "%s", "\n");
					}	
					fflush(stdout);			
				}
				tstat.passed_packet++;
			}
			tstat.packet_count++;
			if( tstat.packet_count > args->packets)
				break;
		}
	}

	fprintf(f, "\nReceived %d packets\n", tstat.packet_count);
	fprintf(f, "TCP FLAGS SUMMARY: ACK %d, PUSH %d, SYNC %d\n", 
													tstat.fstat.ack, 
													tstat.fstat.push,
													tstat.fstat.syn);
	fprintf(f, "ADDRESS STACK: %d\n", tstat.astat.acount);
	for (int i=0; i < tstat.astat.acount; i++){
		fprintf(f, "SRC: %s, %d\n", tstat.astat.src_addr_stack[i], 
									tstat.astat.rep_src[i]);
		fprintf(f, "DST: %s, %d\n", tstat.astat.dst_addr_stack[i],
									tstat.astat.rep_dst[i]);
	}
	return 0;
}

