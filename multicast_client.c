#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUFF_SIZE 1000
#define MCAST_ADDR "225.0.0.37"

struct sockaddr_in multicastAddr;

int main(){
  int mcast_sock;
  struct ip_mreq mreq;

  mcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  // set multicast address
  multicastAddr.sin_family = AF_INET;
  multicastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  multicastAddr.sin_port = htons(3000);

  // bind client addr
  int on = 1;
  if (setsockopt(mcast_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
      printf("%s\n", "Failed to set SO_REUSEADDR in a multicast socket");
  }
  // this allows for more than one processes to bind to that address

  int ttl = 2;
  if (setsockopt(mcast_sock, IPPROTO_IP, IP_MULTICAST_TTL,
                                                        &ttl, sizeof(ttl)) < 0){
      printf("%s\n", "Failed to set TTL in a multicast socket");
  }

  if (bind(mcast_sock, (struct sockaddr *) &multicastAddr,
                                          sizeof(multicastAddr)) < 0){
    printf("%s\n", "Failed to bind a multicast socket");
  }


  mreq.imr_multiaddr.s_addr=inet_addr(MCAST_ADDR);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);

  if (setsockopt(mcast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                                        &mreq, sizeof(mreq)) < 0){
      printf("%s\n", "Failed to set MEMBERSHIP in a multicast socket");
  }

  char msgbuf[BUFF_SIZE];
  int nbytes;
  while(1){
      int addrlen = sizeof(multicastAddr);
      if ((nbytes=recvfrom(mcast_sock, msgbuf, BUFF_SIZE, 0,
           (struct sockaddr *) &multicastAddr, &addrlen)) < 0) {
           perror("recvfrom");
           exit(1);
      }
      printf("%s\n", msgbuf);
  }
}
