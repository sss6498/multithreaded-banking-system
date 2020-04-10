CC = gcc
CFLAGS = -Wall -g -lpthread

all: bankingServer_n_bankingClient

bankingServer_n_bankingClient: bankingServer.o bankingClient.o server_helper.o
	$(CC) $(CFLAGS)  -o bankingServer bankingServer.o server_helper.o
	$(CC) $(CFLAGS)  -o bankingClient bankingClient.o server_helper.o

bankingServer.o: bankingServer.c
	$(CC) $(CFLAGS) -c bankingServer.c

bankingClient.o: bankingClient.c
	$(CC) $(CFLAGS) -c bankingClient.c

banksys.o: server_helper.c server_helepr.h
	$(CC) $(CFLAGS) -c server_helper.c

clean:
	rm -rf *.o bankingClient bankingServer

