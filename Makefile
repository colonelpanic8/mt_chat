CC = gcc
CFLAGS = -g

.PHONY: clean

all: server client

server: server.o bulletin.o dicth.o prime.o
	gcc -g -lpthread -o server server.o bulletin.o dicth.o prime.o

server.o: server.c bulletin.h

dicth.o: dicth.c dicth.h dict.h prime.h

prime.o: prime.h

client: client.o bulletin.o
	gcc -g -lpthread -o client client.o bulletin.o

client.o: client.c bulletin.h dicth.h

bulletin.o: bulletin.c

clean:
	rm *.o
	rm server
	rm client