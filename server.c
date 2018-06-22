#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "mov_register.h"
#include "sock_utils.h"

#define BUF_SIZE 1000
#define MCAST_ADDR "225.0.0.1"

struct chess_register global_register;
struct chess_move current_move;

struct sockaddr_in multicastAddr;

int command_processor(char *command){
  if (strcmp(command, "quit") == 0){
    closelog();
    exit(0);
  }
  else if (strcmp(command, "show") == 0){
    return 1;
  }
  else {
    return 0;
  }
}

int main(){
  char client_str[BUF_SIZE];
  char cmdline[BUF_SIZE];
  int move_counter = 0;
  int listen_fd, comm_fd;
  struct sockaddr_in servaddr;

  // opensyslog, LOG as user and include PID and print to console 
  // in case of errors 
  openlog("SYSTEM_LOG", LOG_PID|LOG_CONS, LOG_USER);

  global_register.t_count = 0;

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(22000);

  bzero(&multicastAddr, sizeof(multicastAddr));   /* Zero out structure */
  multicastAddr.sin_family = AF_INET;                 /* Internet address family */
  multicastAddr.sin_addr.s_addr = inet_addr(MCAST_ADDR);/* Multicast IP address */
  multicastAddr.sin_port = htons(6500);         /* Multicast port */

  bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));

  printf("Starting a server!\n");
  syslog(LOG_INFO, "Server starting...");
  listen(listen_fd, 10);

  if ((comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL)) < 0){
      printf("Failed connection");
      syslog(LOG_ERR, "Connection has failed");
      closelog();
      exit(0);
  }
  else printf("System connected\n");
  syslog(LOG_INFO, "Connection established");

  int on = 1;
  if (setsockopt(comm_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
      printf("SO_REUSEADDR failed to be set");
      syslog(LOG_ERR, "SO_REUSEADDR has failed");
  }

  char msg[BUF_SIZE];
  char move_string[300];

  // fetch mutlicast socket
  int mcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  create_sending_mcast_socket(&mcast_sock);

  while(true){
      bzero(client_str, BUF_SIZE);
      bzero(cmdline, BUF_SIZE);
      bzero(&current_move, sizeof(current_move));

      read(comm_fd, client_str, BUF_SIZE);
      // remove CR CF command
      remove_char_from_string('\n', client_str);
      // process client command
      if (command_processor(client_str) == 1){
        // send register to client
        bzero(&msg, sizeof(msg));
        get_register(&global_register, &msg);
        write(comm_fd, msg, strlen(msg)+1);
        continue;
      }

      if (!validate_move(client_str)) {
        strcpy(msg, "Invalid move, move again\n");
        write(comm_fd, msg, strlen(msg)+1);
        continue;
      }

      printf("Opponent has moved: %s\n", client_str);
      printf("It's your turn now\n");
      fgets(cmdline, BUF_SIZE, stdin);
      // remove CR CF command
      remove_char_from_string('\n', cmdline);

      // process server command
      while (command_processor(cmdline) == 1){
        print_register(&global_register);
        fgets(cmdline, BUF_SIZE, stdin);
        remove_char_from_string('\n', cmdline);
      }

      while (!validate_move(cmdline)) {
        printf("%s\n", "Invalid move, move again");
        fgets(cmdline, BUF_SIZE, stdin);
        remove_char_from_string('\n', cmdline);
      }

      current_move.move_id = move_counter++;
      strcpy(current_move.white, client_str);
      strcpy(current_move.black, cmdline);
      add_move_to_register(&global_register, &current_move);

      write(comm_fd, cmdline, strlen(cmdline)+1);
      // send move to multicast socket
      print_move(&current_move, &move_string);
      printf("%s", current_move);
      syslog(LOG_INFO, current_move.white);
      syslog(LOG_INFO, current_move.black);
      sendto(mcast_sock, move_string, strlen(move_string), 0,
              &multicastAddr, sizeof(multicastAddr));
      printf("Waiting for opponent to move...\n");
    }
  return 0;
}
