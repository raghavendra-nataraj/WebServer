CC = gcc
FLAGS = -lpthread

all: server client server_UDP client_UDP clean

server: server.o 
	$(CC) -o $@ server.o $(FLAGS)

client: client.o
	$(CC) -o $@ $^

server_UDP: server_UDP.o
	$(CC) -o $@ $^ $(FLAGS)

client_UDP: client_UDP.o
	$(CC) -o $@ $^

clean:
	rm *.o
