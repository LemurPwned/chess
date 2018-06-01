#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define BUF_SIZE 1000

int main(){
  int sockfd;
  char sendline[BUF_SIZE];
  char recvline[BUF_SIZE];
  struct sockaddr_in servaddr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&servaddr, sizeof servaddr);

  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(22000);

  inet_pton(AF_INET,"127.0.0.1", &(servaddr.sin_addr));

  connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  int on = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
      printf("SO_REUSEADDR failed to be set");
  }
  while(true)
  {
      bzero(sendline, BUF_SIZE);
      bzero(recvline, BUF_SIZE);
      // get stdin
      fgets(sendline, BUF_SIZE, stdin);
      write(sockfd, sendline, strlen(sendline)+1);

      read(sockfd, recvline, BUF_SIZE);
  }
  return 0;
}
