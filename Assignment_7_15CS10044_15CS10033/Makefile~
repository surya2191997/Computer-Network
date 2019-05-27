all : sender receiver

sender: sender.cpp
	g++ -o sender sender.cpp -pthread

receiver: receiver.cpp
	g++ -o receiver receiver.cpp -pthread

clean:
	rm sender receiver
