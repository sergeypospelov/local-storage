#CC=g++ -g -O3 -DNDEBUG
#CC=g++ -std=c++17 -g

SRCDIR = src
BINDIR = bin
INCDIR = include

PROTOBUF=./protobuf-3.18.1/src
PROTOC=$(PROTOBUF)/protoc

CXX = g++
CXXFLAGS = -O2 -Wall -std=c++17 -Werror -I $(INCDIR) -I $(PROTOBUF)
LDFLAGS = $(PROTOBUF)/.libs/libprotobuf.a -ldl -pthread

SERVER = server
CLIENT = client

all: data $(SERVER) $(CLIENT)

# binaries and main object files

data:
	mkdir data
	touch data/data
	touch data/log

kv: $(BINDIR) $(SRCDIR)/kv.pb.cc

$(SRCDIR)/kv.pb.cc: kv.proto
	$(PROTOC) --cpp_out=. ./kv.proto
	mv kv.pb.h $(INCDIR)
	mv kv.pb.cc $(SRCDIR)

MAINS = $(SERVER) $(CLIENT)

SRCS = $(wildcard $(SRCDIR)/.cpp) $(wildcard $(SRCDIR)*.pb.cc)
OBJS = $(subst .pb.cc,.pb.o,$(subst .cpp,.o,$(SRCS)))

OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(BINDIR)/%.o,$(wildcard $(SRCDIR)/*.cpp)) $(patsubst $(SRCDIR)/%.cc,$(BINDIR)/%.o,$(wildcard $(SRCDIR)/*.pb.cc))
OBJECTS_WITHOUT_MAINS = $(filter-out $(MAINS:%=$(BINDIR)/%.o), $(OBJECTS))

OBJECTS_SERVER = $(OBJECTS_WITHOUT_MAINS) $(BINDIR)/$(SERVER).o
OBJECTS_CLIENT = $(OBJECTS_WITHOUT_MAINS) $(BINDIR)/$(CLIENT).o

$(SERVER): $(BINDIR) $(OBJECTS_SERVER)
	$(CXX) $(OBJECTS_SERVER) -o $(SERVER) $(LDFLAGS)

$(CLIENT): $(BINDIR) $(OBJECTS_CLIENT)
	$(CXX) $(OBJECTS_CLIENT) -o $(CLIENT) $(LDFLAGS)

$(BINDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

$(BINDIR)/%.pb.o: $(SRCDIR)/%.pb.cc
	$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

include $(wildcard $(BINDIR)/*.d)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR) $(CLIENT) $(SERVER)
	rm -rf data

.PHONY: clean all
