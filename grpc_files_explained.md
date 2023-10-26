Request/response definition with GRPC framework:
------------------------------------------------

GRPC uses protocol buffers for defining rpc methods and the messages
(and its attributes) exchanged between the clients and servers. The
code is structured in the following way:

`protos/pre_net.proto` - This is the protocol buffer file for grpc. It
defines RequestTags, RPCs, message types with its attributes. The
first line of the file defines the syntax of the file, we use `proto3`.
The RequestTags are used in requests and request servicers to track
the status of the request being processed.  (running make creates the
corresponding services in `.pb.h` files)

Server:

`include/pre_server.h` - This file is the core of the server
definition. This defines the request servicers on the server side for
different types of requests from the clients and the data structures
to be used by the server for storing data sent by the clients.

`include/pre_server_<custom>_request.h` - This file defines the actual
processing of the `<custom>` requests sent by the client to the
server. The datastructures defined in `pre_server.h` file are passed by
reference in processing the request.

Client: 

`include/pre_broker.h`, `pre_consumer.h`, `pre_producer.h` - Each type of
client has a corresponding `.h` file that defines the functions called
corresponding to each request required by the client (These are the
functions that processes the RPC calls and so have the same names as
the RPCs defined in `pre_net.proto` file).

`targets/pre_broker.cpp`, `pre_consumer.cpp`, `pre_producer.cpp` - files for
each type of client (command line flags could be added to identify
specific clients to customize flow) that contain the actual data
processing flow of these clients (note that broker acts like the
server and client at times depending on the action needed to be
performed).

