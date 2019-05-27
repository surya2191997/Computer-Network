all : server client

server: TCPserver_con.c
	gcc -o server TCPserver_con.c -lssl -lcrypto

client: TCPclient_con.c
	gcc -o client TCPclient_con.c -lssl -lcrypto

clean:
	rm client server
