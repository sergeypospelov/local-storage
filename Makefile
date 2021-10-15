all: server client

server: server.o
	g++ -g -o server server.o -O3 -DNDEBUG \
	-pthread -ldl

server.o: server.cpp
	g++ -g -c -O3 -DNDEBUG -pthread server.cpp

client: client.o
	g++ -g -o client client.o -O3 -DNDEBUG \
	-pthread -ldl

client.o: client.cpp
	g++ -g -c -O3 -DNDEBUG -pthread client.cpp
