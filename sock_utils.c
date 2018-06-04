#include "sock_utils.h"

void create_sending_mcast_socket(int *mcast_sock_lock){
  // for multicast udp socket is needed
  int on = 1;
  // this allows for more than one processes to bind to that address
  if (setsockopt(*mcast_sock_lock, SOL_SOCKET, SO_REUSEADDR, &on,
                                                            sizeof(on)) <0){
      printf("%s\n", "Failed to set SO_REUSEADDR in a multicast socket");
  }
  int ttl = 6;
  if (setsockopt(*mcast_sock_lock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
                                                              sizeof(ttl)) < 0){
      printf("%s\n", "Failed to set TTL in a multicast socket");
  }

  on = 1;
  if (setsockopt(*mcast_sock_lock, IPPROTO_IP, IP_MULTICAST_LOOP, &on,
                                                              sizeof(on)) < 0){
      printf("%s\n", "Failed to disable loop in a multicast socket");
  }
}

void create_receiving_mcast_socket(int *mcast_sock_lock, struct ip_mreq mem){
  // for multicast udp socket is needed
  int on = 1;
  // this allows for more than one processes to bind to that address
  if (setsockopt(*mcast_sock_lock, SOL_SOCKET, SO_REUSEADDR, &on,
                                                            sizeof(on)) <0){
      printf("%s\n", "Failed to set SO_REUSEADDR in a multicast socket");
  }

  if (setsockopt(*mcast_sock_lock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mem,
                                                            sizeof(mem)) <0){
      printf("%s\n", "Failed to set MEMBERSHIP in a multicast socket");
  }
  int ttl = 6;
  if (setsockopt(*mcast_sock_lock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
                                                              sizeof(ttl)) < 0){
      printf("%s\n", "Failed to set TTL in a multicast socket");
  }
  if (setsockopt(*mcast_sock_lock, IPPROTO_IP, IP_MULTICAST_LOOP, &on,
                                                              sizeof(on)) < 0){
      printf("%s\n", "Failed to disable loop in a multicast socket");
  }
}

void remove_char_from_string(char c, char *str)
{
   /*
   Stack overflow
   */
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

ssize_t	writen(int fd, const void *vptr, size_t n) {
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
