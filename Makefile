LIB=protobuf-3.18.1/src/.libs/libprotobuf.a -ldl -pthread
INC=-I ./protobuf-3.18.1/src

all: server client

server: server.o protocol
	g++ -g -o server server.o kv.pb.o -O3 -DNDEBUG \
	$(LIB)

server.o: server.cpp protocol
	g++ -g -c -O3 -DNDEBUG server.cpp $(INC)

client: client.o protocol
	g++ -g -o client client.o kv.pb.o -O3 -DNDEBUG \
	$(LIB)

client.o: client.cpp protocol
	g++ -g -c -O3 -DNDEBUG client.cpp $(INC)

kv: kv.proto
	./protobuf-3.18.1/src/protoc --cpp_out=. kv.proto
	g++ -g -c -O3 -DNDEBUG kv.pb.cc $(INC)

protocol: kv
