# Duality Labs OpenFHE Experiments for Encrypted Network Measurement/Control and Secure Data Distribution with Proxy Re-Encryption

July 2024: full release 3.1 version - bug fixes for PRE, works with OpenFHE 1.2.0

This repository contains multiple examples of Fully Homomorphic
Encryption (FHE) for Proxy Re Encryption and Threshold-FHE encrypted
network control functions developed under DARPA funding for the I2O
Open, Programmable, Secure 5G (OPS5G) program. Note the utility of
these examples are valid for many forms of wireless and wireline
networks, not just 5G networks.

Questions should be forwarded to the Duality Principal Investigator,
Dr. David Bruce Cousins, email: dcousins@dualitytech.com

## Synopsis

This repository contains several sample programs for encrypted data
processing between cooperating network processes with the goals of:

* Distributing sensitive data throughout a network in a secure manner

* Performing distributed network measurement and control functions
  using Threshold FHE to increase the work effort required to subvert
  the system via side-channel attacks etc.
 
There are several example programs available that show how to use the
OpenFHE open-source library to build systems of multiple cooperating
heavyweight processes, using the Google RPC system both in a
client-server and peer-to-peer framework.

We show the capabilities of OpenFHE's Proxy-Re-encryption (allowing a
third party to re-encrypt Alice's encrypted data with Bob's decryption
key without involving Alice). We also show the use of OpenFHE's Threshold
Encryption (where multiple parties cooperate on a common computation
task and all participate in decryption).

**Note 1** several of these examples are first prototypes, and may not
represent the best way to distribute the responsibility of Crypto
context generation, and key distribution. As we develop better
examples, we may revise or delete earlier ones. Earlier examples will
always be available in prior repository releases.

**Note 2** the distribution contains a large number of unit tests for
the underlying communications frameworks in the `tests` directory. For
information on these tests (how to add tests, run tests, etc.) visit
the  [README.md](tests/README.md) file in the `tests` sub-directory.

**Note 3** Earlier releases had Some examples using the RAVEN network
simulation framework to emulate a multiple domain network. This system
is not well supported, and was difficult to maintain, so we removed
support for it this release.


## Acknowledgments and Distribution 

Distribution Statement ‘A’ (Approved for Public Release, Distribution
Unlimited).  This work is supported in part by DARPA. The views,
opinions, and/or findings expressed are those of the author(s) and
should not be interpreted as representing the official views or
policies of the Department of Defense or the U.S. Government.

This work is supported by DARPA through contract number HR001120C0157. 
This work was conducted by Duality Technologies as a subcontractor to
University of Southern California / Information Sciences Institute.

**Note** previous DARPA approved releases of some source code examples
contained herin were done under the title "PALISADE Serialization
examples". PALISADE versions have been ported to OpenFHE and can be
found at the [OpenFHE Serial Examples repository](https://github.com/openfheorg/openfhe-serial-examples). That
repository contains basic examples of threshold and proxy
re-encryption use cases between multiple parties using basic
Inter-process communication (IPC) based on file/mutex and TCP/IP
sockets.

# The Example Systems

A short description of the various examples systems now follows. All
systems are to be found in the `src` sub-directory of the root
directory.


## Multi-Hop, Muli-Trust Zone PRE Network For AES Key Distribution Demo using Client-Server based GRPC

Found in the `src/pre_grpc_demo` directory. This example demonstrates
an application to distribute sensitive data such as symmetric keys
from producers to consumers through multiple brokers across multiple
trust zones. A single trusted server in the first trust zone generates
OpenFHE Crypto Contexts for servers in other trust zones, brokers,
producers and consumers. The producers and consumers generate their
own public key, secret key pairs. The server generates the key pair
for each broker in its trust zone and re-encryption keys from upstream
broker to downstream broker or consumer.  A cipher-text generated by
the producer is relayed through the brokers to the consumer.  It also
includes an example of denying access to unauthorized consumers. Note
that the brokers can perform a re-encryption but cannot decrypt.

The PRE Network example is built with the google RPC framework.
Please note one has to install gRPC and Google Protobufs before this
can be compiled (see section below). Some detailed information on how
gRPC is used for client/server examples can be found in the file
`src/pre_grpc_demo/Grpc_files_explained.md`


GRPC also allows authenticated
connections between the clients and server using `ssl`. This is done by
generating certificates for the servers and clients using the scripts
in `scripts/authentication`. The folder in which the certificate files
are generated (usually the build folder) is passed as a command line
argument to the server and the client targets to establish a secure
channel for communication. Refer to "Certificate setup for gRPC
communications" section below under "Building Instructions".

A single thread test bench program `pre` is used to test parameter
settings. A Python script `scripts/multihop_params.py` and associated
support files provide the means to determine the appropriate crypto
parameters to use for a given configuration. Instructions for running
a multi-hop example are provided below. An additional script
`multi_pre_test_singlethread.sh` in the `benchmarks` folder can be
used to run the target `benchmarks/pre_d.cpp` (needs to be moved to
`pre_grpc_demo/targets` to be built) multiple times in order to
conduct decryption failure rate experiments.

## Peer-to-Peer Network Communication Framework

Found in `src/peer_to_peer_framework` that defines the communication
framework (based on GRPC) to send and receive messages between
multiple nodes without an intermediate server. The code defines a
`node` object. The nodes are initialized as a GRPC server that can
receive messages from other node servers, with internal message
queuing and broadcast. To send a message to another node, the local
node creates an internal client instance and sends the message. There
is simple handshaking to detect failed nodes. There are no examples
within this directory. Rather, all peer-to-peer examples use this
code.


The following files define the different functionalities of the Node object.

1. `node.h` : the node implementation class and the functions that are
   invoked in the applications to register a node, create message
   queues and send and get messages

1. `node_internal_client.h` : create a client instance within the node
   server to send messages to other node servers.

1. `node_request.h`: The node server side processing of the messages
   received from other nodes and assigning it to the correct message
   queues.

1. `node_server_async_base.h` : gRPC object for the node server to handle
   async requests

1. `comm_utils(.h,.cpp)` : utilities for creating the message
   structure, message queue objects and processing command line
   arguments.

## Threshhold FHE with Abort Recovery Examples 

There are a few different versions of this example: 

* `src/threshold_fhe_aborts` contains single file implementations used
  for benchmarking the core crypto operations without any socket
  communications. The file `threshold_fhe.cpp` runs without testing
  aborts, and `threshold_fhe_aborts.cpp` tests the abort recovery system.
  
* `src/threshnet_aborts_grpc_demo` is a client-server version.

* `src/threshnet_aborts_p2p_demo` is a peer-to-peer version. 

Both client-server and peer-to-peer use GRPC for secure socket
communications, but are coded in a completely different manner. 

All these examples implement multiparty a threshold FHE computation
service using gRPC.  Demo scripts to run a threshold example with 5
parties in both client-server and peer-to-peer settings are in the
`demos` folder (see below). Additional files for running the examples with 3 and 7
parties can be found in the `benchmarks` folder.

## Network Adjacent Co-Measurement Client-Server Examples

Found in the `src\threshnet_adjacent_measure_demo` directory, this example
demonstrated two nodes comparing the data from a common measurement via
a controller that randomizes the ciphertexts from both nodes. The only
information extracted is whether the measurements are the same or not. 

Note a peer-to-peer example of this can be found in the
`src/network_measure_examples` as `network_measure` and
`network_measure_with_controller` for versions with and without a
controller.

## Network Measurement Examples (are built using the peer to peer framework for GRPC communication)

Found in `src/network_measure_examples`. The two examples include 

1) an adjacent network measure, where two nodes take measurements of a
shared integer value (such as the bandwidth between them) and
determine if the measurement is the same or different in a secure
manner. There are configurations with and without a third party
controller node. 

2) Measurement of values along a path. This example accumulates values
in a vector from each node in a path, and computes some operations  on the
accumulated data. This is done using threshold FHE.

The current path measurement example allows for computation of some
statistics (such as mean, squares of mean and cubes of mean as of now)
by accumulating encrypted data along a path of nodes. A trusted
controller receives the partial decryption shares and does the
computation. The same ciphertext is used to accumulate data from
multiple nodes (treated as a register). The following files are
specific to the path measurement example

1. `register_functions.h` : functions used for accumulation of data
   into the ciphertext.

1. `path_measure_crypto_functions.h, .cpp` : The cryptographic
   functions such as joint public key, evaluation keys generation
   along a path.

# Building Instructions

Here are instructions for building each of the examples. 

## Building Examples with gRPC

Several examples use gRPC as their interprocess communications
system. They require gRPC and Google protobufs to be installed on your
system.  Based on suggestions from Google's documentation, we
recommend installing gRPC locally if it is not pre-installed in your
system.

Detailed instructions for this can be found in the file
`Installing_gRPC.md` located in the root directory. Please refer to
those and isntall gRPC on your system before proceeding to the
remainder of the build instrutions below.

## Build instructions for Ubuntu

Please note that we have not tried installing and running these
examples on windows or macOS. If anyone does try this, please update
this file with instructions and generate a merge request.
It's recommended to use at least Ubuntu 19 gnu g++ 7 or greater.

1. Install prerequisites (if not already installed): `g++`, `cmake`,
   `make`, `boost` and `autoconf`. Sample commands using `apt-get` are
   listed below. It is possible that these are already installed on
   your system.

	> `sudo apt-get install build-essential #this already includes g++`

	> `sudo apt-get install autoconf`

	> `sudo apt-get install make`

	> `sudo apt-get install cmake`

	> `sudo apt-get install libboost-all-dev`

	Note that `sudo apt-get install g++-<version>` can be used to
	install a specific version of the compiler. You can use `g++
	--version` to check the version of `g++` that is the current system
	default. The version of boost installation required might be 1.71 or
	higher for compatibility.
	
1. Install OpenFHE on your system. The examples codes here require using the
   `v1.2.0` tag of the   OpenFHE `development` repository to be found 
   [here](https://github.com/openfheorg/openfhe-development/tree/v1.2.0 "openfhe v1.2.0 repo").
   

	Full instructions for installing OpenFHE are to be found in the
`README.md` file in the OpenFHE repo.

	Run `make install` at the end to install the system to the default
location (you can change this location, but then you will have to
change the CMakefile in this repo to reflect the new location).

	Note you may have to execute the following on your system to
automatically find the installed libraries and include files:

	> `sudo ldconfig`

1. Clone this repo onto your system.
1. Create the build directory

   > `mkdir build`

1. Move to that directory and run `cmake`. 

   > `cd build`
   
   If you have not added them to your `$PATH` environement variable,
   specify the directories where you have installed `gRPC` an
   `Protobufs` to the `cmake` command. For example if I installed it in `/home/thisuser/opt/grpc` then you need to run this as:
   
  > ` cmake  -DProtobuf_DIR=/home/thisuser/opt/grpc/lib/cmake/protobuf -DgRPC_DIR=/home/thisuser/opt/grpc/lib/cmake/grpc ..`

	Note if you used a different install directory for OpenFHE (for
    example if I installed it in `/home/thisuser/opt/openfhe64_1_2_0` then I would need to
    run this as 
	
	> `cmake -DOPENFHE_INSTALL_DIR=/home/thisuser/opt/openfhe64_1_2_0/ -DProtobuf_DIR=/home/thisuser/opt/grpc/lib/cmake/protobuf -DgRPC_DIR=/home/palisade/opt/grpc/lib/cmake/grpc ..`

	Note: If you have multiple versions (revisions) of OpenFHE on
    your system, `cmake` may find the wrong one and cause build errors
    (`cmake` uses an elaborate set of search rules to find a library,
    and it may not be the version you expect). If you have strange
    build errors, consider using the above `-DOPENFHE_INSTALL_DIR` to
    point specifically to the correct version. Refer to the main `CMakeLists.txt` for more details. 

1. Build the examples using `make`. Please note that OpenFHE
   serialization uses the `CEREAL` system for crypto object
   serialization which can result in some long compile times.

   > `make`

	Executables for all the examples will be in the `build/bin`
    directory. If you have a multicore system `make -j` will speed up
    the build process.


## Certificate setup for GRPC communications

GRPC allows to enable communications with `ssl` if needed. When `ssl`
is enabled, the nodes use certificates to authenticate each other. A
test certificate setup is provided for the examples using the scripts in
authentication folder. Each example built with GRPC has a folder in
the authentication folder and lists the nodes needed for the example
in the `nodes` file.

To generate certificates, run the following command from the build
directory:

> `sh ../scripts/authentication/create_root_cert.sh`

> `sh ../scripts/authentication/create_nodes_cert.sh <example_directory_name>`

For example, to run the adjacent measure example without controller, run

> `sh ../scripts/authentication/create_nodes_cert.sh adjacent_network_measure`

Please make sure that the node names listed in the `nodes` file is the
same as the process name when a GRPC process is triggered in command
line with the `-n` flag. Remove the node folders, `*.crt` and `*.key` files
in the build directory after running each example to be sure there is
no overlap of nodes between examples.

Please look at the `scripts/authentication` directory to determine the
appropriate directory names to use.

If you are going to generate all the demo scripts then you can run the file

> `sh ../scripts/authentication/create_all_demo_certs.sh`


Several Examples are available for each feature. Please see the file 
[Running_Examples.md](Running_Examples.md) for complete detailed instructions.

Note some classes of examples are still currently being worked on. If a class has been 
completed, it will be marked **Verified** in the tile. 
