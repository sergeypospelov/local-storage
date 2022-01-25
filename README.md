# Local kv-storage bits

## Compilation instructions
1. Download protobuf https://github.com/protocolbuffers/protobuf/releases/tag/v3.18.1 and put it into ./protobuf-3.18.1
2. Run ```make kv```
3. Run ```make all```

## Protocol description
* Each client uses a single tcp socket for communication with the server
* All messages have the following form: message_type (1 byte) message_len (4 bytes) message_data (message_len bytes)
* message_data is the serialized form for one of the messages described in `kv.proto`
* Each request and each response message contains a request_id field used to match responses vs requests

## Run instructions
* Start the server @ port 4242: `./server 4242`
* Run put + get stages with 100 requests via the client: `./client 4242 100 put get`
* Run only the get stage via the client: `./client 4242 100 get`
* Run put + get stages with debug-level logging via the client: `VERBOSITY=4 ./client 4242 100 put get`

See the code for more details
