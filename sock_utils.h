#ifndef SOCK_UTILS_H
#define SOCK_UTILS_H

#include <sys/types.h>   /* basic system data types */
#include <sys/socket.h>  /* basic socket definitions */
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


void create_server_mcast_socket(int *mcast_sock_lock);
void create_client_mcast_socket(int *mcast_sock_lock, struct ip_mreq mem);
void remove_char_from_string(char c, char *str);
ssize_t	writen(int fd, const void *vptr, size_t n);

#endif
