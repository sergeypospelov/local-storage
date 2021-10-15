all: server client

server: server.o protocol
	g++ -g -o server server.o kv.pb.o -O3 -DNDEBUG \
	-pthread -ldl -L protobuf-3.18.1/src/.libs/ -l protobuf

server.o: server.cpp protocol
	g++ -g -c -O3 -DNDEBUG -pthread server.cpp

client: client.o protocol
	g++ -g -o client client.o kv.pb.o -O3 -DNDEBUG \
	-pthread -ldl -L protobuf-3.18.1/src/.libs/ -l protobuf

client.o: client.cpp protocol
	g++ -g -c -O3 -DNDEBUG -pthread client.cpp

kv: kv.proto
	./protobuf-3.18.1/src/protoc --cpp_out=. kv.proto
	g++ -g -c -O3 -DNDEBUG kv.pb.cc -I ./protobuf-3.18.1/src

protocol: kv
