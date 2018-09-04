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
#include <sys/epoll.h>
#include <pthread.h>

#include "sock_utils.h"

#define BUFF_SIZE 1000
#define MCAST_ADDR "239.0.0.0"
#define MAX_EVENTS 100
struct sockaddr_in multicastAddrSend;
struct sockaddr_in multicastAddrReceive;
char alias[100];
int epoll_fd;
struct epoll_event event, events[MAX_EVENTS];
int mcast_sock_send, mcast_sock_recv;


void *read_stdin(){
  char msg[BUFF_SIZE], alias_msg[BUFF_SIZE];
  int nbytes;
  printf("%s\n", "Type in message to send:");
  while(true){
    bzero(&msg, sizeof(msg));
    bzero(&alias_msg, sizeof(alias_msg));
    read(0, msg, BUFF_SIZE);
    if (strcmp(msg, "quit\n") == 0){
      exit(0);
    }
    strcpy(alias_msg, alias);
    strcat(alias_msg, msg);
    nbytes = sendto(mcast_sock_send, alias_msg, strlen(alias_msg), 0,
                  (struct sockaddr *) &multicastAddrSend, sizeof(multicastAddrSend));
  }
}

void *epoll_read(){
  char msgbuf[BUFF_SIZE];
  int nbytes, count;
  int addrlen = sizeof(multicastAddrReceive);
  while(true){
    bzero(&msgbuf, sizeof(msgbuf));
    count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < count; i++){
      if ((nbytes=recvfrom(events[i].data.fd, msgbuf, BUFF_SIZE, 0,
                            (struct sockaddr *) &multicastAddrReceive, &addrlen)) < 0) {
           perror("MESSAGE ERROR\n");
           exit(1);
      }
      printf("%s", msgbuf);
      bzero(msgbuf, sizeof(msgbuf));
    }
  }
}

int main(int argc, char *argv[]){
  int port = 1234;
  bzero(&multicastAddrReceive, sizeof(multicastAddrReceive));
  bzero(&multicastAddrSend, sizeof(multicastAddrSend));
  multicastAddrSend.sin_family = AF_INET;
  multicastAddrSend.sin_addr.s_addr = inet_addr(MCAST_ADDR);
  multicastAddrSend.sin_port = htons(port);

  multicastAddrReceive.sin_family = AF_INET;
  // receiving socket gets different local interface
  multicastAddrReceive.sin_addr.s_addr = htonl(INADDR_ANY);
  multicastAddrReceive.sin_port = htons(port);

  // create two sockets
  mcast_sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  mcast_sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // for receiving socket only
  struct ip_mreq mem;
  // needed to join mcast membership group
  // only receiving socket
  mem.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
  mem.imr_interface.s_addr = inet_addr("127.0.0.1");
  create_receiving_mcast_socket(&mcast_sock_recv, mem);

  int optval = 1;
  // set reuse port for receiving port and reuse addr
  if (setsockopt(mcast_sock_recv, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
        perror("Failed to reuse address\n");
  }
  if (setsockopt(mcast_sock_recv, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0){
    perror("Failed to reuse port\n");
  }
  // bind a receiving socket
  if (bind(mcast_sock_recv, (struct sockaddr *) &multicastAddrReceive,
                                                    sizeof(multicastAddrReceive)) < 0){
    printf("%s\n", "Failed to bind a multicast socket");
  }
  
  struct in_addr local_interface;
  local_interface.s_addr = inet_addr("127.0.0.1");
  if (setsockopt(mcast_sock_send, IPPROTO_IP, IP_MULTICAST_IF, &local_interface, 
      sizeof(local_interface)) < 0){
      perror("Failed to set interface\n");
  }

  // get epoll instance
  epoll_fd = epoll_create1(0);
  bzero(&event, sizeof(struct epoll_event));
  event.events = EPOLLIN;
  event.data.fd = mcast_sock_recv;

  if (epoll_fd < 0){
    perror("Epoll failed");
    exit(-1);
  }
  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mcast_sock_recv, &event)){
    perror("Failed to add epoll instance");
  }

  printf("%s\n", "Type in your alias for the chatroom");
  fgets(alias, 100, stdin);
  remove_char_from_string('\n', alias);
  char msg[1024];
  sprintf(msg, "New person has joined the chat: %s\n", alias);
  sendto(mcast_sock_send, msg, strlen(msg), 0,
                  (struct sockaddr *) &multicastAddrSend, sizeof(multicastAddrSend));
  strcat(alias, ": ");

  pthread_t reading_stdin, reading_socket;
  if (pthread_create(&reading_stdin, NULL, read_stdin, NULL) < 0){
    perror("Failed creating an external thread");
  }
  if (pthread_create(&reading_socket, NULL, epoll_read, NULL) < 0){
    perror("Failed creating an external thread");
  }

  while(true){}
  return 0;
}
