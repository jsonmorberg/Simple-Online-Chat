CCFLAGS=-Wall
CC=gcc

all: participant observer server

participant: participant.c
	$(CC) $(CCFLAGS) -o participant participant.c

observer: observer.c
	$(CC) $(CCFLAGS) -o observer observer.c

server: server.c
	$(CC) $(CCFLAGS) -o server server.c

clean:
	rm -f participant observer server
