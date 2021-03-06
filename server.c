#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>

#include "mov_register.h"
#include "sock_utils.h"

#define BUF_SIZE 1000
#define MCAST_ADDR "239.0.0.0"
#define MAX_SOCK 2

struct chess_register global_register;
struct chess_move current_move;

struct sigaction siga;
volatile sig_atomic_t move_on_flag = 1;
int default_timeout;
int current_socket;


void move_timeout(int sig){
  // changes move_on_flag to 0 which means that 
  // the move is over
  move_on_flag = 0;
  char *err_msg[BUF_SIZE];
  sprintf(err_msg, "You ran out of time. Game lost...\n");
  int n = write(current_socket, err_msg, strlen(err_msg));
  close(current_socket);
  exit(-1); 
} 

void reset_and_start_timer(int time_required){
  move_on_flag = 1; // resets the signal timer
  // sets up the alarm again
  siga.sa_handler = move_timeout;
  siga.sa_flags = 0; 
  if (sigaction(SIGALRM, &siga, NULL) < 0){
    perror("Sigaction");
  }
  alarm(time_required);
}

int process_command(char *command, int socket){
  if (strcmp(command, "quit") == 0){
    closelog();
    exit(0);
  }
  else if (strcmp(command, "show") == 0){
    char reg_buff[BUF_SIZE];
    bzero(reg_buff, sizeof reg_buff);
    get_register(&global_register, reg_buff);
    if (write(socket, reg_buff, strlen(reg_buff)+1) < 0){
      perror("Write error");
    }
    return 1;
  }
  else {
    return 0;
  } 
}

int main(int argc, char *argv[]){
  char client_str[BUF_SIZE];
  char cmdline[BUF_SIZE];
  char msg[BUF_SIZE];
  char move_string[BUF_SIZE];
  int n;
  int move_counter = 0;
  struct sockaddr_in servaddr, multicastAddr;
  struct in_addr local_interface;

  int listen_fd, white_fd, black_fd, mcast_sock;
  // fetch mutlicast socket
  mcast_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  listen_fd = socket(AF_INET, SOCK_STREAM, 0);

  // opensyslog, LOG as user and include PID and print to console 
  // in case of errors 
  openlog("SYSTEM_LOG", LOG_PID|LOG_CONS, LOG_USER);

  global_register.t_count = 0;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htons(INADDR_ANY);
  servaddr.sin_port = htons(22000);

  bzero(&multicastAddr, sizeof(multicastAddr));   /* Zero out structure */
  multicastAddr.sin_family = AF_INET;                 /* Internet address family */
  multicastAddr.sin_addr.s_addr = inet_addr(MCAST_ADDR); /* Multicast IP address */
  multicastAddr.sin_port = htons(1234);         /* Multicast port */

  local_interface.s_addr = inet_addr("127.0.0.1");
  if (setsockopt(mcast_sock, IPPROTO_IP, IP_MULTICAST_IF, &local_interface, 
      sizeof(local_interface)) < 0){
      perror("Failed to set interface\n");
  }

  int on = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
      printf("SO_REUSEADDR failed to be set");
      syslog(LOG_ERR, "SO_REUSEADDR has failed");
  }

  if (bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
    perror("Bind error");
  }

  printf("Starting a server!\n");
  syslog(LOG_INFO, "Server starting...");

  if (listen(listen_fd, MAX_SOCK) < 0){
    perror("Listen error");
  }

  if ((white_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL)) < 0){
      printf("Failed connection");
      syslog(LOG_ERR, "Connection has failed");
      closelog();
      exit(0);
  }
  else printf("White connected\n");

  if ((black_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL)) < 0){
      printf("Failed connection");
      syslog(LOG_ERR, "Connection has failed");
      closelog();
      exit(0);
  }
  else printf("Black connected\n");
  syslog(LOG_INFO, "Connection established");


  if (argc < 2){
    default_timeout = 30; // 30 sec is default 
  }else{
    default_timeout = strtol(argv[1], NULL, 10);
  }
  printf("Timeout for move in this game is %d seconds.\n", default_timeout);
  while(true){
    // white begin
    bzero(&msg, sizeof msg);
    sprintf(msg, "Opponent has moved %s\n", current_move.black);
    strcat(msg, "White move");
    bzero(&current_move, sizeof(current_move));
    n = write(white_fd, msg, strlen(msg)+1);
    if (n < 0){
      syslog(LOG_ERR, "Write error");
      perror("Write error");
    }
    while(true) {
      bzero(&move_string, sizeof move_string);
      // get move from white
      current_socket = white_fd;
      reset_and_start_timer(default_timeout);
      n = read(white_fd, move_string, BUF_SIZE);
      if (n < 0){
        syslog(LOG_ERR, "Read error");
        perror("Read error");
      }
      remove_char_from_string('\n', move_string);
      if (process_command(move_string, white_fd)){
        continue;
      }
      // repeat the move if necessary
      if (!validate_move(move_string)){
        bzero(&msg, sizeof msg);
        strcpy(msg, "Invalid move. Please move again");
        n = write(white_fd, msg, strlen(msg)+1);
        if (n < 0){
          syslog(LOG_ERR, "Write error");
          perror("Write error");
        }
      }
      else{
        // copy white move
        strcpy(current_move.white, move_string);
        break;
      }
    }

    bzero(&msg, sizeof msg);
    // black move
    sprintf(msg, "Opponent has moved %s\n", current_move.white);
    strcat(msg, "Black move");
    n = write(black_fd, msg, strlen(msg)+1);
    if (n < 0){
      syslog(LOG_ERR, "Write error");
      perror("Write error");
    }
    while(true) {
      bzero(&move_string, sizeof move_string);
      // get move from black
      current_socket = black_fd;
      reset_and_start_timer(default_timeout);
      n = read(black_fd, move_string, BUF_SIZE);
      if (n < 0){
        syslog(LOG_ERR, "Read error");
        perror("Read error");
      }
      remove_char_from_string('\n', move_string);

      if (process_command(move_string, black_fd)){
        continue;
      }
      if (!validate_move(move_string)){
        bzero(&msg, sizeof msg);
        strcpy(msg, "Invalid move. Please move again");
        n = write(black_fd, msg, strlen(msg)+1);
        if (n < 0){
          syslog(LOG_ERR, "Write error");
          perror("Write error");
        }
      }
      else{
        strcpy(current_move.black, move_string);
        break;
      }
    }
    current_move.move_id = move_counter++;
    syslog(LOG_INFO, current_move.white);
    syslog(LOG_INFO, current_move.black);
    bzero(&move_string, sizeof move_string);
    print_move(&current_move, &move_string);
    add_move_to_register(&global_register, &current_move); 
    printf("Sending move: %s\n", move_string);
    sendto(mcast_sock, move_string, strlen(move_string), 0,
              (struct sockaddr*)&multicastAddr, sizeof(multicastAddr)); 
  }
  return 0;
}
