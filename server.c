#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mov_register.h"

void remove_char_from_string(char c, char *str)
{
    int i=0;
    int len = strlen(str)+1;

    for(i=0; i<len; i++)
    {
        if(str[i] == c)
        {
            // Move all the char following the char "c" by one to the left.
            strncpy(&str[i],&str[i+1],len-i);
        }
    }
}

// int command_processor()

int main(){
  char str[100];
  char cmdline[100];

  struct chess_register global_register;
  global_register.t_count = 0;
  struct chess_move current_move;
  int move_counter = 0;

  int listen_fd, comm_fd;

  struct sockaddr_in servaddr;

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

  bzero( &servaddr, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(22000);

  bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  listen(listen_fd, 10);

  if ((comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL)) < 0){
      printf("Failed connection");
      exit(0);
  }
  else printf("System connected\n");

  int on = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
      printf("SO_REUSEADDR failed to be set");
  }

  while(true)
  {
      bzero(str, 100);
      bzero(cmdline, 100);
      bzero(&current_move, sizeof(current_move));

      printf("Waiting for opponent to move...\n");
      read(comm_fd, str, 100);
      remove_char_from_string('\n', str);
      // remove CR CF command
      printf("Opponent has moved: %s\n", str);
      printf("It's your turn now\n");
      fgets(cmdline, 100, stdin);
      remove_char_from_string('\n', cmdline);
      current_move.move_id = ++move_counter;
      strcpy(current_move.white, str);
      strcpy(current_move.black, cmdline);
      add_move_to_register(&global_register, &current_move);

      write(comm_fd, cmdline, strlen(cmdline)+1);
    }
  return 0;
}
