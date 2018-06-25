CFLAGS = -Wall -pthread -lcap
GDB = -g

GLOB_OBJ = server.o client.o mov_register.o sock_utils.o multicast_client.o
SERV_DEP = server.o mov_register.o sock_utils.o
CLIENT_DEP = multicast_client.o sock_utils.o

CVER = -std=gnu11
all: app

app: $(GLOB_OBJ)
	gcc $(CFLAGS) client.c -o client
	gcc $(CFLAGS) $(SERV_DEP) -o server
	gcc $(CFLAGS) $(CLIENT_DEP) -o multicast_client

debug: $(GLOB_OBJ)
	gcc $(GDB) $(CFLAGS) client.c -o client
	gcc $(GDB) $(CFLAGS) $(SERV_DEP) -o server

server.o: server.c
	gcc -c server.c

client.o: client.c

multicast_client.o: multicast_client.c
	gcc -c multicast_client.c

sock_utils.o: sock_utils.c
	gcc -c sock_utils.c

mov_register.o: mov_register.c mov_register.h
	gcc -c mov_register.c

test: mov_register.o test.c
	gcc $(CVER) $(CFLAGS) mov_register.o test.c -o tests

analyser: tcp_analyzer.o
	gcc $(CVER) $(CFLAGS) tcp_analyzer.o filter_args.c -o tcp_analyzer

tcp_analyzer.o: tcp_analyzer.c
	gcc -c -lcap tcp_analyzer.c


clean:
	rm -rf *.o
	rm -f server client multicast_client
