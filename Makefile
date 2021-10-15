PROTOBUF=./protobuf-3.18.1/src
CC=g++ -g -O3 -DNDEBUG
PROTOC=$(PROTOBUF)/protoc
LIB=$(PROTOBUF)/.libs/libprotobuf.a -ldl -pthread
INC=-I $(PROTOBUF)

all: server client

server: server.o protocol
	$(CC) -o server server.o kv.pb.o $(LIB)

server.o: server.cpp protocol
	$(CC) -c server.cpp $(INC)

client: client.o protocol
	$(CC) -o client client.o kv.pb.o $(LIB)

client.o: client.cpp protocol
	$(CC) -c client.cpp $(INC)

kv: kv.proto
	$(PROTOC) --cpp_out=. kv.proto
	$(CC) -c kv.pb.cc $(INC)

protocol: kv
