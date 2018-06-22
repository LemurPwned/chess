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
#define MCAST_ADDR "225.0.0.1"
#define MAX_EVENTS 100
struct sockaddr_in multicastAddrSend;
struct sockaddr_in multicastAddrReceive;
char alias[100];
int epoll_fd;
struct epoll_event event, events[MAX_EVENTS];
int mcast_sock_send, mcast_sock_recv;


void *read_stdin(){
  char msg[BUFF_SIZE];
  int nbytes;
  while(true){
    bzero(&msg, sizeof(msg));
    read(0, msg, BUFF_SIZE);
    printf("%s\n", "Type in message to send: ");
    nbytes = sendto(mcast_sock_send, msg, strlen(msg), 0,
                  (struct sockaddr *) &multicastAddrSend, sizeof(multicastAddrSend));
    printf("Sending: %d, %s\n", nbytes, msg);
  }
}

void *epoll_read(){
  char msgbuf[BUFF_SIZE];
  int nbytes, count;
  int addrlen = sizeof(multicastAddrReceive);

  while(true){
    bzero(&msgbuf, sizeof(msgbuf));
    // bzero(&alias_msg, sizeof(alias_msg));
    // alias_msg = strcat(alias, " says:");
    // get msg from stdin

    // fgets(msg, BUFF_SIZE, stdin);
    // strcat(alias_msg, msg);

    count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < count; i++){
      printf("%s\n", "READING EVENT");
      if ((nbytes=recvfrom(events[i].data.fd, msgbuf, BUFF_SIZE, 0,
                            (struct sockaddr *) &multicastAddrReceive, &addrlen)) < 0) {
           perror("MESSAGE ERROR\n");
           exit(1);
      }
      printf("Read: %d\n", nbytes);
      printf("%s\n", msgbuf);
      bzero(msgbuf, sizeof(msgbuf));
    }
  }
}
void sending_process(int *mcast_sock_send){

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
    sendto(*mcast_sock_send, alias_msg, strlen(alias_msg)-1, 0,
                  (struct sockaddr *) &multicastAddrSend, sizeof(multicastAddrSend));
  }
}

void receiving_process(int *mcast_sock_recv, struct ip_mreq mem){
  int addrlen = sizeof(multicastAddrReceive);
  char msgbuf[BUFF_SIZE];
  int nbytes, count;

  while(true){
    count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < count; i++){
      printf("%d\n", i);
      if ((nbytes=recvfrom(events[i].data.fd, msgbuf, BUFF_SIZE, 0,
                            (struct sockaddr *) &multicastAddrReceive, &addrlen)) < 0) {
           perror("MESSAGE ERROR\n");
           exit(1);
      }
      printf("%s\n", msgbuf);
      fflush(stdout);
      bzero(msgbuf, sizeof(msgbuf));
    }
  }
}

int main(int argc, char *argv[]){
  bzero(&multicastAddrReceive, sizeof(multicastAddrReceive));
  bzero(&multicastAddrSend, sizeof(multicastAddrSend));
  // server multicast address, common to both sockets
  printf("%d %d\n", strtol(argv[1], NULL, 10), strtol(argv[2], NULL, 10));
  int p1 = strtol(argv[1], NULL, 10);
  int p2 = strtol(argv[2], NULL, 10);
  multicastAddrSend.sin_family = AF_INET;
  multicastAddrSend.sin_addr.s_addr = inet_addr(MCAST_ADDR);
  multicastAddrSend.sin_port = htons(p1);


  multicastAddrReceive.sin_family = AF_INET;
  // receiving socket gets different local interface
  multicastAddrReceive.sin_addr.s_addr = htonl(INADDR_ANY);
  multicastAddrReceive.sin_port = htons(p2);

  // create two sockets
  mcast_sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  mcast_sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // bind a receiving socket
  if (bind(mcast_sock_recv, (struct sockaddr *) &multicastAddrReceive,
                                                    sizeof(multicastAddrReceive)) < 0){
    printf("%s\n", "Failed to bind a multicast socket");
  }
  // set sending socket
  create_sending_mcast_socket(&mcast_sock_send);

  struct ip_mreq mem;
  // needed to join mcast membership group
  // only receiving socket
  mem.imr_multiaddr.s_addr=inet_addr(MCAST_ADDR);
  mem.imr_interface.s_addr=htonl(INADDR_ANY);
  create_receiving_mcast_socket(&mcast_sock_recv, mem);



  // get epoll instance
  epoll_fd = epoll_create1(0);
  bzero(&event, sizeof(struct epoll_event));
  event.events = EPOLLIN;
  event.data.fd = mcast_sock_recv;

  printf("%d\n", mcast_sock_recv);
  if (epoll_fd < 0){
    perror("Epoll failed");
    exit(-1);
  }
  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mcast_sock_recv, &event)){
    perror("Failed to add epoll instance");
  }

  // event.events = EPOLLIN;
  // event.data.fd = 0;
  //
  // if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event)){
  //   perror("Failed to add epoll instance");
  // }

  printf("%s\n", "Type in your alias for the chatroom");
  // fgets(alias, 100, stdin);
  // remove_char_from_string('\n', alias);


  char msgbuf[BUFF_SIZE];
  int nbytes, count;
  char *alias_msg;

  pthread_t reading_stdin, reading_socket;
  if (pthread_create(&reading_stdin, NULL, read_stdin, NULL) < 0){
    perror("Failed creating an external thread");
  }
  if (pthread_create(&reading_socket, NULL, epoll_read, NULL) < 0){
    perror("Failed creating an external thread");
  }

  while(true){
  }

  // if (fork() == 0) sending_process(&mcast_sock_send);
  // else receiving_process(&mcast_sock_recv, memb);
}
