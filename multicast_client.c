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


#include "sock_utils.h"

#define BUFF_SIZE 1000
#define MCAST_ADDR "225.0.0.37"
#define MAX_EVENTS 100
struct sockaddr_in multicastAddr;
char alias[100];
int epoll_fd;
struct epoll_event event, events[MAX_EVENTS];

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
                  (struct sockaddr *) &multicastAddr, sizeof(multicastAddr));
  }
}

void receiving_process(int *mcast_sock_recv, struct ip_mreq mem){

  if (bind(*mcast_sock_recv, (struct sockaddr *) &multicastAddr,
                                                    sizeof(multicastAddr)) < 0){
    printf("%s\n", "Failed to bind a multicast socket");
  }

  int addrlen = sizeof(multicastAddr);
  char msgbuf[BUFF_SIZE];
  int nbytes, count;

  while(true){
    count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < count; i++){
      printf("%d\n", i);
      if ((nbytes=recvfrom(events[i].data.fd, msgbuf, BUFF_SIZE, 0,
                            (struct sockaddr *) &multicastAddr, &addrlen)) < 0) {
           perror("MESSAGE ERROR\n");
           exit(1);
      }
      printf("%s\n", msgbuf);
      fflush(stdout);
      bzero(msgbuf, sizeof(msgbuf));
    }
  }
}

int main(){
  int mcast_sock;
  int mcast_sock_send, mcast_sock_recv;

  // server multicast address
  multicastAddr.sin_family = AF_INET;
  multicastAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  multicastAddr.sin_port = htons(3300);

  // needed to join mcast membership group
  struct ip_mreq memb;
  memb.imr_multiaddr.s_addr=inet_addr(MCAST_ADDR);
  memb.imr_interface.s_addr=htonl(INADDR_ANY);


  // set sending socket
  create_server_mcast_socket(&mcast_sock_send);
  create_client_mcast_socket(&mcast_sock_recv, memb);

  int on = 1;
  if (setsockopt(mcast_sock_send, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0){
    perror("Failed to reuse address");
  }
  if (setsockopt(mcast_sock_recv, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0){
    perror("Failed to reuse address");
  }

  on = 0;
  if (setsockopt(mcast_sock_recv, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on)) < 0){
    printf("%s\n", "Failed to disable a multicast lopp");
  }

  // if (bind(mcast_sock_recv, (struct sockaddr *) &multicastAddr,
  //                                                   sizeof(multicastAddr)) < 0){
  //   printf("%s\n", "Failed to bind a multicast socket");
  // }
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

  event.events = EPOLLIN;
  event.data.fd = 0;

  if (epoll_fd < 0){
    perror("Epoll failed");
    exit(-1);
  }
  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &event)){
    perror("Failed to add epoll instance");
  }

  printf("%s\n", "Type in your alias for the chatroom");
  fgets(alias, 100, stdin);
  remove_char_from_string('\n', alias);


  int addrlen = sizeof(multicastAddr);
  char msgbuf[BUFF_SIZE];
  int nbytes, count;
  char msg[BUFF_SIZE];
  char *alias_msg;
  while(true){
    // printf("%s\n","EPOLL ITERATION");
    count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < count; i++){
      // printf("%d\n", i);
      if (events[i].events & EPOLLIN){
        if (events[i].data.fd != 0){
          if ((nbytes=recvfrom(events[i].data.fd, msgbuf, BUFF_SIZE, 0,
                                (struct sockaddr *) &multicastAddr, &addrlen)) < 0) {
               perror("MESSAGE ERROR\n");
               exit(1);
          }
          printf("Read: %d\n", nbytes);
          printf("%s\n", msgbuf);
          fflush(stdout);
          bzero(msgbuf, sizeof(msgbuf));
        }
        else {
          // printf("%s\n", alias);
          bzero(&msg, sizeof(msg));
          // bzero(&alias_msg, sizeof(alias_msg));
          // alias_msg = strcat(alias, " says:");
          // get msg from stdin
          printf("%s\n", "Type in message to send: ");
          // fgets(msg, BUFF_SIZE, stdin);
          // strcat(alias_msg, msg);
          nbytes = sendto(mcast_sock_send, alias, sizeof(alias), 0,
                        (struct sockaddr *) &multicastAddr, sizeof(multicastAddr));
          printf("Sending: %d\n", nbytes);
        }
      }
      else {

      }

    }
  }
  // if (fork() == 0) sending_process(&mcast_sock_send);
  // else receiving_process(&mcast_sock_recv, memb);
}
