# Local kv-storage bits

## Compilation instructions
1. Download protobuf https://github.com/protocolbuffers/protobuf/releases/tag/v3.18.1 and put it into ./protobuf-3.18.1
2. Run ```make all```

## Protocol description
* Each client uses a single tcp socket for communication with the server
* All messages have the following form: message_type (1 byte) message_len (4 bytes) message_data (message_len bytes)
* message_data is the serialized form for one of the messages described in `kv.proto`

## Run instructions
* Start the server @ port 4242: `./server 4242`
* Run put + get stages with 100 requests via the client: `./client 4242 100 put get`
* Run only the get stage via the client: `./client 4242 100 get`
* Run put + get stages with debug-level logging via the client: `VERBOSITY=4 ./client 4242 100 put get`

See the code for more details

## TODO
1. The hash table used in the server should be persistent (right now it's just an ordinary std::unordered_map)
2. After making this hash table persistent, implement a persistent key-value storage based on this hash index
