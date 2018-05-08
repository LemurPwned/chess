#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sock_utils.h"

#define BUFF_SIZE 1000
#define MCAST_ADDR "225.0.0.37"

struct sockaddr_in multicastAddr;
char alias[100];

void sending_process(int *mcast_sock_send){
  // set sending socket
  create_server_mcast_socket(mcast_sock_send);
  char msg[BUFF_SIZE];
  remove_char_from_string('\n', alias);
  char *alias_msg;
  while(true){
    bzero(&msg, sizeof(msg));
    bzero(&alias_msg, sizeof(alias_msg));
    alias_msg = strcat(alias, " says:");
    // get msg from stdin
    printf("%s\n", "Type in message to send: ");
    fgets(msg, BUFF_SIZE, stdin);
    strcat(alias_msg, msg);
    sendto(*mcast_sock_send, alias_msg, strlen(alias_msg), 0,
                  (struct sockaddr *) &multicastAddr, sizeof(multicastAddr));
  }
}

void receiving_process(int *mcast_sock_recv, struct ip_mreq mem){
  create_client_mcast_socket(mcast_sock_recv, mem);

  if (bind(*mcast_sock_recv, (struct sockaddr *) &multicastAddr,
                                                    sizeof(multicastAddr)) < 0){
    printf("%s\n", "Failed to bind a multicast socket");
  }

  int addrlen = sizeof(multicastAddr);
  char msgbuf[BUFF_SIZE];
  int nbytes;

  while(true){
    if ((nbytes=recvfrom(*mcast_sock_recv, msgbuf, BUFF_SIZE, 0,
                          (struct sockaddr *) &multicastAddr, &addrlen)) < 0) {
         perror("MESSAGE ERROR");
         exit(1);
    }
    printf("%s\n", msgbuf);
  }
}

int main(){
  int mcast_sock;
  int mcast_sock_send, mcast_sock_recv;

  // server multicast address
  multicastAddr.sin_family = AF_INET;
  multicastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  multicastAddr.sin_port = htons(3000);

  // needed to join mcast membership group
  struct ip_mreq memb;
  memb.imr_multiaddr.s_addr=inet_addr(MCAST_ADDR);
  memb.imr_interface.s_addr=htonl(INADDR_ANY);

  printf("%s\n", "Type in your alias for the chatroom");
  fgets(alias, 100, stdin);

  if (fork() == 0) sending_process(&mcast_sock_send);
  else receiving_process(&mcast_sock_recv, memb);
}
