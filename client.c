#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>


int main(){
  int sockfd,n;
  char sendline[100];
  char recvline[100];
  struct sockaddr_in servaddr;

  sockfd = socket(AF_INET,SOCK_STREAM,0);
  bzero(&servaddr,sizeof servaddr);

  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(22000);

  inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));

  connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

  bool your_turn = true;
  while(true)
  {
      bzero(sendline, 100);
      bzero(recvline, 100);
      // get stdin

      printf("It's your turn now\n");
      fgets(sendline, 100, stdin);
      write(sockfd, sendline, strlen(sendline)+1);

      printf("Waiting for opponent to move...\n");
      read(sockfd, recvline, 100);
      printf("Opponent has moved: %s\n",recvline);
  }
  return 0;
}
