all : server client

server: TCPserver.c
	gcc -o server TCPserver.c -lssl -lcrypto

client: TCPclient.c
	gcc -o client TCPclient.c -lssl -lcrypto

clean:
	rm client server
