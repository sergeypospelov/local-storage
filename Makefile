#CC=g++ -g -O3 -DNDEBUG
CC=g++ -std=c++17 -g
PROTOC=$(PROTOBUF)/protoc

PROTOBUF=./protobuf-3.18.1/src
LIB=$(PROTOBUF)/.libs/libprotobuf.a -ldl -pthread
INC=-I $(PROTOBUF)

COMMON_O=kv.pb.o log.o protocol.o rpc.o PersistentStorage.o

all: client server

# binaries and main object files

client: client.o common
	$(CC) -o client client.o $(COMMON_O) $(LIB)

client.o: client.cpp common
	$(CC) -c client.cpp $(INC)

server: server.o common
	$(CC) -o server server.o $(COMMON_O) $(LIB)

server.o: server.cpp common
	$(CC) -c server.cpp $(INC)

# libs

common: kv log protocol rpc PersistentStorage

log: log.h log.cpp
	$(CC) -c log.cpp $(INC)

kv: kv.proto
	$(PROTOC) --cpp_out=. kv.proto
	$(CC) -c kv.pb.cc $(INC)

protocol: protocol.h protocol.cpp
	$(CC) -c protocol.cpp $(INC)

rpc: rpc.h rpc.cpp
	$(CC) -c rpc.cpp $(INC)

PersistentStorage: PersistentStorage.hpp PersistentStorage.cpp
	$(CC) -c PersistentStorage.cpp $(INC)
