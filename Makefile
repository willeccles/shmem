.PHONY: all clean

all: daemon client

daemon: daemon.c common.h
	$(CC) -O3 -pthread -lrt -std=c11 -o daemon daemon.c

client: client.c common.h
	$(CC) -O3 -pthread -lrt -std=c11 -o client client.c

clean:
	$(RM) client daemon

