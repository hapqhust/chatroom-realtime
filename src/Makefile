CFLAGS 		= 	-Wall -ggdb3 -pthread
CC 			= 	gcc
CLIENT_OBJS =	client.o
SERVER_OBJS =	server.o
LIB_OBJS 	= 	caesar.o
TARGETS 	=	client server

all: ${TARGETS}

clean:
	rm -f ${CLIENT_OBJS} ${SERVER_OBJS} ${LIB_OBJS} ${TARGETS}

run-client: 
	./client 127.0.0.1 5550

run-server: 
	./server 5550

val-server: 
	valgrind --tool=memcheck --leak-check=yes ./server 5550

val-client:
	valgrind --tool=memcheck --leak-check=yes ./client 127.0.0.1 5550

caesar: ${LIB_OBJS}
	${CC} -o $@ ${LIB_OBJS}

client: ${CLIENT_OBJS} ${LIB_OBJS}
	${CC} ${CFLAGS} -o $@ ${CLIENT_OBJS} ${LIB_OBJS}

server: ${SERVER_OBJS} ${LIB_OBJS}
	${CC} ${CFLAGS} -o $@ ${SERVER_OBJS} ${LIB_OBJS}

client.o: client.c
server.o: server.c
caesar.o: caesar.c
