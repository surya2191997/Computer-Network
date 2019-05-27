all : server client

server: test_server.cpp
	g++ -std=c++11 -o server test_server.cpp 

client: test_client.cpp
	g++ -o client test_client.cpp 

clean:
	rm client server
