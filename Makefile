PROTOBUF=./protobuf-3.18.1/src
CC=g++ -g -O3 -DNDEBUG
PROTOC=$(PROTOBUF)/protoc
LIB=$(PROTOBUF)/.libs/libprotobuf.a -ldl -pthread
INC=-I $(PROTOBUF)

all: server client

server: server.o protocol kv
	$(CC) -o server server.o protocol.o kv.pb.o $(LIB)

server.o: server.cpp protocol kv
	$(CC) -c server.cpp $(INC)

client: client.o protocol kv
	$(CC) -o client client.o protocol.o kv.pb.o $(LIB)

client.o: client.cpp protocol kv
	$(CC) -c client.cpp $(INC)

kv: kv.proto
	$(PROTOC) --cpp_out=. kv.proto
	$(CC) -c kv.pb.cc $(INC)

#protocol: protocol.h protocol.cpp kv
protocol: protocol.h protocol.cpp
	$(CC) -c protocol.cpp $(INC)
