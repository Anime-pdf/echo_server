CC      = gcc
CFLAGS  = -g
RM      = rm -f

default: all

all: clean Client Server

Client: client.c
	$(CC) $(CFLAGS) -o Client client.c

Server: server.c
	$(CC) $(CFLAGS) -o Server server.c

clean veryclean:
	$(RM) Client Server