# Duality Labs OpenFHE Experiments for Encrypted Network Mueasurement/Control and Secure Data Distribution with Proxy Re-Encryption

This repository contains multiple examples of Fully Homomorphic
Encryption (FHE) for Proxy Re Encryption and Threshold-FHE encrypted
network control functions developed under DARPA funding for the I2O
Open, Programmable, Secure 5G (OPS5G) program. Note the utility of
these examples are valid for many forms of wireless and wireline
networks, not just 5G networks.

**This repository is still in pre-release form.** It contains several
examples of network based FHE applications. Not all examples have been
hardened with additional error checking or detailed installation
instructions, and so may not operate properly. We will annotate this
README file with status updates explicitly identify the status of each
example system. We also suggest monitoring the [ToDo](ToDo.md) file to
review the status of the various demonstrations before attempting to
run them.

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

**Note 3** Some examples use the RAVEN network simulation framework to
emulate a multiple domain network. This is a complex system, so users
are advised to become familiar with the simpler client/server and
peer-to-peer demonstrations that can be run on a multicore system (or
smaller network).

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
can be compiled (see section below). GRPC also allows authenticated
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

## Threshhold FHE with Abort Recovery Examples 

There are a few different versions of this example: 

* `src/threshold_fhe_aborts` contains single file implementations used
  for benchmarking the core crypto operations without any socket
  communications. The file `threshold_fhe.cpp` runs without testing
  aborts, and `threshold_fhe_aborts.cpp` tests the abort recovery system.
  
* `src/threshnet_aborts_grpc` is a client-server version.

* `src/threshnet_aborts_p2p_demo` is a peer-to-peer version. 

Both client-server and peer-to-peer use GRPC for secure socket
communications, but are coded in a completely different manner. 

All these examples implement multiparty a threshold FHE computation
service using gRPC.  Demo scripts to run a threshold example with 5
parties in both client-server and peer-to-peer settings are in the
`demos` folder (see below). Additional files for running the examples with 3 and 7
parties can be found in the `benchmarks` folder.

## Network Adjacent Co-Measurement Client-Server Example

Found in the `src\thresh_net_adjacent_measure` directory, this example
demonstrated two nodes comparing the data from a common measurement via
a controller that randomizes the ciphertexts from both nodes. The only
information extracted is whether the measurements are the same or not. 

Note a peer-to-peer example of this can be found in the
`src/network_measure_examples` as `network_measure` and
`network_measure_with_controller` for versions with and without a
controller.

## Network Measurement Examples (are built using the peer to peer framework for GRPC communication)

Found in `src/network_measure_examples`. The examples include adjacent
network measure between two nodes without a controller and path
measurement. The path measurement example is to compute statistics
among the nodes where the data is accumulated along a path of nodes.

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
   `dev` branch of OpenFHE  (note the requirement for `dev`
   may be lifted in the future as features are released into the master
   branch).

	Full instructions for this install are to be found in the
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

1. Move to that directory and run `cmake`. The examples were initially
   built with PALISADE and have been updated to work with OpenFHE.

   > `cd build`
   
   If you have not added them to your `$PATH` environement variable,
   specify the directories where you have installed `gRPC` an
   `Protobufs` to the `cmake` command. For example if I installed it in `/home/thisuser/opt/grpc` then you need to run this as:
   
  > ` cmake  -DProtobuf_DIR=/home/thisuser/opt/grpc/lib/cmake/protobuf -DgRPC_DIR=/home/thisuser/opt/grpc/lib/cmake/grpc ..`

	Note if you used a different install directory for OpenFHE (for
    example if I installed it in `/home/thisuser/opt/OpenFHE` then you need to
    run this as 
	
	> `cmake -DCMAKE_INSTALL_PREFIX=/home/palisade/opt/openfhe64_1_1_1/ -DProtobuf_DIR=/home/thisuser/opt/grpc/lib/cmake/protobuf -DgRPC_DIR=/home/palisade/opt/grpc/lib/cmake/grpc ..`

	Note: If you have multiple versions (revisions) of OpenFHE on
    your system, `cmake` may find the wrong one and cause build errors
    (`cmake` uses an elaborate set of search rules to find a library,
    and it may not be the version you expect). If you have strange
    build errors, consider using the above `-DCMAKE_INSTALL_PREFIX` to
    point to the correct version.

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

# Running The Examples

Before running the examples, copy the `demoData` folder into the build
directory From the `build` directory (required by some examples).

Please check that the particular example has been verified as operational. If it has not then *caveat emptor*, try them but beware!

## Multihop PRE Network Examples [all verified as working]

---------------

### PRE Network For AES Key Distribution Demo with multiple brokers using Google RPC

This example allows for PRE between a producer and consumer through
multiple broker hops in multiple trust zones. Each broker either
reencrypts for a broker (self) if the re-encrypted ciphertext is being
passed on to another broker, or re-encrypts to the consumer's key if
the re-encrypted ciphertext is being passed on from the broker to a
consumer. The number of brokers that can be used depends on the
security model and the parameters. The multihop example can be run
either using the single file implementation (`pre.cpp`) or the
multiple process implementation using `gRPC`.

The gRPC demo example demonstrates an application to distribute
sensitive data such as symmetric keys from producers to consumers. A
single trusted server in the first trust zone generates OpenFHE Crypto
Contexts for trusted key servers in other trust zones, producers,
consumers and brokers. Producers and consumer generate their own keys
pairs and the key server in each trust zone generates the key pair for
brokers in its trust zone. It also includes an example of denying
access to unauthorized consumers. Note that the broker cannot decrypt
the ciphertexts sent to them.

The demo requires additional third party applications to be installed:

* `tmux`
* `mpv`
* `openssl` (should be installed in most linux distros). 
* `zenity` (should be installed in most linux distros). 

> `sudo apt-get install tmux`

> `sudo apt-get install mpv`

#### Running the demoscript using the tmux terminal multiplexor

To run the demo, build the example by following instructions under "Building"

1. copy the `demoData` directory found in the root directory and it's
   contents to the `build` directory
   
   >`cp -rf demoData build`

1. open a new terminal window in the `build\bindirectory, expand it to a
   large size, and type

> `tmux`

to open the multiple terminal windows that will show the various
   component printouts.
   
1. In another window cd to the `build` directory of this repo and type

   > `../demos/demoscript_pre_grpc_tmux.sh`

1. Within the multiple windows you will see the following steps occur:
   1. The Producer generates an AES key and encrypts a video (displayed)
   1. The Server generates an OpenFHE Crypto Context CC. 
   1. The Brokers requests the CC from the Server and the Server generates a
      key pair for each broker.
   1. The Producer requests the CC and sends its key pair to the Server and
      ciphertext (encrypted AES key) to its downstream broker. The broker 
      reencrypts the ciphertext and caches it.
   1. The Consumer requests the CC, sends its public key to the Server and 
      requests a ciphertext for a specific channel from its upstream broker.
   1. The broker requests for a reencryption key and the public key of
      its upstream broker from the server. This is done recursively
      until a broker connected to the producer is reached and the
      requested ciphertext is passed along the chain of brokers to the
      consumer. Each broker in the chain cache the ciphertext
      reencrypted to its own key.
   1. The Consumer decrypts the AES key using their private key
      then decrypts the media using the AES key (displayed).
   1. A new unauthorized Consumer then requests for the ciphertext
      from a channel to its upstream broker. This broker receives an
      empty re-encryption key from the server and a garbage
      ciphertext.
   1. The server, not recognizing the new Consumer, responds with a
      bogus empty key to the upstream broker and the consumer receives
      a garbage ciphertext.
   1. The unauthorized Consumer then tries to decrypts the encrypted
      AES and fails (error shown). Click on the error window to dismiss it.
   
	  
If you see errors like `no server running on /tmp/tmux-1000/default`
then you forgot to run `tmux` in another window, and the demo will not
run properly.  The script is timed so events can be seen occurring
sequentially with built in pauses. It can also run interactively by
adding a second parameter after the script command:

> `../demos/demoscript_pre_grpc_tmux.sh interactive`

If you see an error `Cannot load libcuda.so.1` you can ignore it. It is from the `mpv` video display program.


A simple access control is emulated using an Access Map file (here
`demoData/accessMaps/pre_accessmap`) that is a white list of all
authorized consumers. This needs to be passed as a command line
argument with the `-a` flag to the key server. If a consumer is not in
the white list, then it is considered unauthorized.  In the above
example When an unauthorized consumer named `charlie` requests for
ciphertext from `broker_2`, the broker receives an empty reencryption
key and the consumer receives a random garbage ciphertext.

Once the consumer and producer are finished, the server
and the brokers remain running.  The producer and consumer programs
can be run again and again.

#### Running the modules manually

*Currently under construction, see [ToDo](Todo.md) for status.*

The example demo can also be run manually in multiple terminals. The
following flags apply to the different processes `pre_server_demo`,
`pre_broker_demo`, `pre_producer_demo` and `pre_consumer_demo`.

```
   -n process_name -> client_name
   -k key_sever_socket_address
   -d broker_socket_address
   -u upstream_broker_socket_address
   -i upstream_broker_name
   -s upstream_server_name
   -a access_map_file
   -c channel_name (producer_name-consumer_name)
   -m security_mode (INDCPA, FIXED_NOISE_HRA, NOISE_FLOODING_HRA, NOISE_FLOODING_HRA_HYBRID) [required]
```
**Manual example 1:**

This is a manually configured example with two brokers, where `consumer_1` is connected to
`broker_2`, `broker_2` is connected to `broker_1` and producer `alice` is connected
to `broker_1`. The channel from `consumer_1` to producer `alice` is

![manual example 1 network](docs/pre_network_1.png)

> `alice -> B1 -> B2 -> consumer_1`

Before running be sure to delete all `*.txt` files from `bin` that may be left over
from prior runs before running. Also remove any previous run's consumer keys with

> `rm demoData/keys/consumer_aes_key_alice-consumer_1` 

First generate alice's dummy AES key used for her data payload:

> `cp ../demoData/keys/producer_aes_key_P0 demoData/keys/producer_aes_key_alice`

then execute the following in separate terminals. 

> `bin/pre_server_demo -n KS_1 -k localhost:50051 -a demoData/accessMaps/pre_accessmap -m INDCPA -l .`

> `bin/pre_broker_demo -n B1 -k localhost:50051 -d localhost:50052  -m INDCPA -l .`

> `bin/pre_broker_demo -n B2 -k localhost:50051 -u localhost:50052 -m INDCPA -i B1 -d localhost:50053 -l .`

> `bin/pre_producer_demo -n alice -k localhost:50051 -d localhost:50052 -m INDCPA -l .`

> `bin/pre_consumer_demo -n consumer_1 -k localhost:50051 -u localhost:50053 -m INDCPA -i B2 -c alice-consumer_1 -l .`

You can then verify that the contents of the `consumer_1`'s received data
is the same as `alice`'s sent data by comparing the content of the files
`producer_aes_key_alice` and `consumer_aes_key_consumer_1` in the
`demoData/keys` folder using the following script:

> `../scripts/verify_pre_output.sh demoData/keys/producer_aes_key_alice demoData/keys/consumer_aes_key_alice-consumer_1` 

Once the consumer and producer are finished, the server
and the brokers remain running.  The producer and consumer programs
can be run again and again.

Note if you see a node terminate with :

```
terminate called after throwing an instance of 'lbcrypto::config_error'
  what():  /home/palisade/opt/openfhe64_1_1_1/include/openfhe/pke/cryptocontext.h:360 Key is nullptr
```

odds are that node is not in the access list and was handed garbage keys as a result. 


**Manual example 2:**

The demo can also be run across multiple trust zones (i.e. multiple
key servers that communicate with each other across the zones, each
serving keys to brokers and producer/consumers within their own
zones). There can also be multiple brokers for each zone, allowing you
to build distribution chains and trees for very large fan-out of
secure data.

To run an example with two trust zones with two brokers in each zone (note the use of a diffferent access map), run the following in multiple separate terminals. 

![manual example 1 network](docs/pre_network_2.png)

> `bin/pre_server_demo -n KS_1 -k localhost:50050 -a demoData/accessMaps/pre_accessmap2 -m INDCPA -l .`

> `bin/pre_server_demo -n KS_2 -k localhost:50060 -s localhost:50050 -a demoData/accessMaps/pre_accessmap2 -m INDCPA -l .`

> `bin/pre_broker_demo -n broker_1 -k localhost:50050 -d localhost:50051 -m INDCPA -l .`

> `bin/pre_broker_demo -n broker_2 -k localhost:50050 -u localhost:50051 -i broker_1 -d localhost:50052 -m INDCPA -l .`

> `bin/pre_broker_demo -n broker_3 -k localhost:50060 -u localhost:50052 -i broker_2 -d localhost:50063 -m INDCPA -l .`

> `bin/pre_broker_demo -n broker_4 -k localhost:50060 -u localhost:50063 -i broker_3 -d localhost:50064 -m INDCPA -l .`

> `bin/pre_producer_demo -n alice -k localhost:50050 -d localhost:50051 -m INDCPA -l .`

> `bin/pre_consumer_demo -n consumer_1 -k localhost:50060 -u localhost:50064 -i broker_4 -c alice-consumer_1 -m INDCPA -l .`

Again, you can run the producer and consumers multiple times, and verify the result as shown in the previous manual example.




**Extensions**

These modules are designed to run across networks with multiple IP
domains. If you want to experiment with this you need to replace
`localhost` with actual hostnames, and also (if using SSL) generate
the appropriate certificates.  The advanced user is directed to the
`raven` directory to see how such networks can be configured (in our
case we use the `raven` network emulation to build our networks).

### Single file PRE implementation -- allows for testing/timing measurment without network overhead. [verified as working]

------------------------------------------------------------------

A single thread implementation that emulates multiple hops for four
different security options can be run using the target `bin/pre`. The
target `bin/pre` has reliable timing using high\_resolution\_clock. This
can be used to verify correct functionality for new parameters sets
without having to launch 100's of brokers in different terminals.

Running the following command runs the pre protocol with IND-CPA
secure parameters and 1 hop by default.

> `bin/pre`

To set the security mode use the flag

`-m #`

where

0 = CPA secure PRE,

1 = fixed 20 bits noise (Bounded HRA secure),

2 = provable secure HRA noise flooding

3 = provable secure HRA noise flooding with Hybrid key switching

To set the number of hops use
`-d <number of hops>`

Running the following command runs the pre protocol with Provable HRA
secure parameters and 5 hops.  > `bin/pre -m 2 -d 5`


## Threshhold Network using Google RPC [verified as working]

The Threshhold Network example is built with the google RPC framework
and generalized to any number of clients. GRPC also allows
authenticated connections between the clients and server using
ssl. The folder in which the certificate files are generated (usually
the build folder) is passed as a command line argument to the server
and the client targets to establish a secure channel for
communication.

Remember to generate certificates for these demos:

> `sh ../scripts/authentication/create_nodes_cert.sh threshnet_aborts`

And to run the various components (manually) in different windows in
the `build` directory.

Running the Server: 

> `bin/thresh_server -n KS -i localhost -p 12345 -l <certificate-location>`

Running the clients:

> `bin/thresh_client -n <client_name> -p 12345 -i localhost -d <client_id> -l <certificate-location> -m <total_num_of_parties>`

the -d flag specifies the client id (an integer from 1 to
total\_num\_of\_parties , 

-a 1 specifies to abort this node partway through the run.

-l specifies the certificate location, 

-n specifies the name of the client, 

-p specifies the port number of the `thresh_server_demo`, 

-i specifies the hostname of the server, 

-m specifies the total number of clients participating in the computation,

-c for the type of computation (which can be 'add', 'multiply' or 'vectorsum')

For example to run a set of four clients + server on the same system,
open five terminal windows and cd to build, and in the first enter

> `bin/thresh_server -n KS -i localhost -p 12345 -l .`

and in the next four enter (one for each terminal)

>`bin/thresh_client -n alice -p 12345 -i localhost -d 1 -l . -m 4 -c multiply`

>`bin/thresh_client -n bob -p 12345 -i localhost -d 2 -l . -m 4 -c multiply`

>`bin/thresh_client -n carol -p 12345 -i localhost -d 3 -l . -m 4 -c multiply`

>`bin/thresh_client -n david -p 12345 -i localhost -d 4 -l . -m 4 -c multiply`

The clients each generate a unique cipher text, and the group
generates the sum and product over them.

The correct output for this example would be 

```

Resulting Fused Mult Plaintext: 
( 1 16 81 256 81 16 1 16 256 81 81 24 ... )

 Resulting Fused AddPlaintext: 
( 4 8 12 16 12 8 4 8 16 12 12 10 ... )

```

Remember to delete the intermediate files generated with 

	> `rm -f server_*.txt client_*.txt`

The Threshnet example also allows to run the aborts protocol when one
or more of parties (minority) drops off before decryption. To test
this, we can use the `-a 1` flag to manually drop off parties. Suppose
there are three clients running, one party can drop off and the other
parties can still complete the distributed decryption process. 

The demoscript runs five clients with and without aborts based on
setting the variable aborts in the script to 1 and 0 respectively. The
variable operation in the script can be set to 'add', 'multiply' or
'vectorsum' to run the threshold example with 5 clients with the
corresponding operation. Once the variables are set in the script as
required, the script can be run as

>`../demos/demoscript_threshnet_grpc_tmux.sh`

There is also a version that runs each party on a separate node (for
more accurate timing) using `taskset`:

>`../demos/demoscript_threshnet_grpc_tmux_taskset.sh`

However you computer needs to have at least 6 logical cpus to run the script.

## Threshhold Network Measurement protocol using Client-Server Google RPC (gRPC) [not verified as working]

The structure of this implementation is very similar to the Threshold
example. The data flow involves two nodes that send their measurement
(ciphertext) to a controller (could be untrusted).

To run an example with two clients, a server and a controller, 

Running the server:

> `bin/thresh_server_measure -n KS -i localhost -p 12345 -Wssloff`

Running the controller:

> `bin/thresh_controller_measure -n Controller -i localhost -p 12345 -o localhost:50051 -m 2 -s 1 -Wssloff`

Running the clients:

> `bin/thresh_client_measure -i localhost -p 12345 -o localhost:50051 -d 1 -n Node1 -m 2 -s 1 -f demoData/threshnet_input_file_same -Wssloff`

> `bin/thresh_client_measure -i localhost -p 12345 -o localhost:50051 -d 2 -n Node2 -m 2 -s 1 -f demoData/threshnet_input_file_same -Wssloff`

The following flags are used:

`-d` flag specifies the client id (an integer from 1 to
`total_num_of_parties` 

`-l` specifies the certificate location,

`-Wssloff` specifies running the protocol without `ssl` authentication

`-n` specifies the name of the client

`-p` specifies the port number of the `thresh_server_measure`

`-i` specifies the hostname of the server

`-m` specifies the total number of clients participating in the
computation

`-s` specifies the session id

`-f` specifies the file location of the client's data

We have four test files (`threshnet_input_file_same_1`,
`threshnet_input_file_same_2`, `threshnet_input_file_diff_1`,
`threshnet_input_file_diff_2`). The files with `*_same_*` for the two
clients have the same data and the files with `*_diff_*` have
different data for the two clients.

The example can also be run using the shell scripts
`demos/demoscript_adjacent_network_measure_same.sh` and
`demos/demoscript_adjacent_network_measure_diff.sh`.

## Peer to Peer network communication (using gRPC) [not verified as working]
In this framework, each individual node communicates with all other
nodes as peers. All peer to peer communications is done through the
Node object which internally acts as both GRPC client and server. Each
node connects to another node based on rules specified in a Network
map, which is provided as input while launching each node in the
terminal. The Node object also maintain a message queue for each of the
other nodes it is allowed to connect to, as specified in the network
map. They can then send or get messages to and from these message
queues as needed for applications built on top of this
framework. 

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

The `testnode.cpp` target file is to test the different methods
defined for the Node service. The `networkmeasure.cpp` file runs a
network measurement protocol between two nodes (launched as two
instances of the target with the appropriate parameters).

The network map format is the following: suppose we have nodes 1,2 and
3 where 1 connects to 2 and 3, 2 connects to 1 and 3 and 3 connects to
1 and 2. This is specified in the network map as:

```
Node1-Node2@localhost:50052,Node3@localhost:50053
Node2-Node1@localhost:50051,Node3@localhost:50053
Node3-Node1@localhost:50051,Node2@localhost:50052
```

The example network map in this repo NetworkMap.txt specifies such a
map for 5 nodes to run with testnode.

To run the testnode target, copy the demoData folder into the `build`
directory and run the following command in the terminal from the
`build` directory

> `bin/testnode -n <node_name> -s <socket_address> -m <network_map_file_path> -Wssloff`

Example for running a node 'Node1': 

> `bin/testnode -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap.txt -Wssloff`

The testnode example with 5 nodes can be run by running the shell
script `demos/demoscript_p2p_testnodes.sh`.

command line flags:

-n node_name

-s socket address of node

-m location of network map file

-f location of application input file

-l location of ssl certificates (or use -Wssloff)

## Network Measurement & Control examples (using the peer to peer gRPC framework) [not verified as working]

To run the network measurement protocol with two nodes, open two
terminals and run the following commands from the `build` directory:

> `bin/network_measure -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_nm.txt -f ../demoData/threshnet_input_file_diff -Wssloff`

> `bin/network_measure -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_nm.txt -f ../demoData/threshnet_input_file_diff -Wssloff`

There are two pairs of example input files
(`threshnet_input_file_diff_Node1`, `threshnet_input_file_diff_Node2`)
and (`threshnet_input_file_same_Node1`,
`threshnet_input_file_same_Node2`) that has different and same
measurements as input for the two nodes respectively. The example can
also be run by running the shell script
`demos/demoscript_p2p_adjacent_network_measure_diff.sh` and
`demos/demoscript_p2p_adjacent_network_measure_same.sh`.

The network measure example with controller also can be run with peer
to peer framework as follows:

> `bin/controller_network_measure -n Controller -s localhost:50053 -m ../NetworkMaps/NetworkMap_nm_controller.txt -Wssloff`

> `bin/network_measure_with_controller -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_nm_controller.txt -f ../demoData/threshnet_input_file_diff -Wssloff`

> `bin/network_measure_with_controller -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_nm_controller.txt -f ../demoData/threshnet_input_file_diff -Wssloff`


The Path measurement example allows for computation of some statistics
(such as mean, squares of mean and cubes of mean as of now) by
accumulating encrypted data along a path of nodes. A trusted
controller receives the partial decryption shares and does the
computation. The same ciphertext is used to accumulate data from
multiple nodes (treated as a register). The following files are
specific to the path measurement example

1. `register_functions.h` : functions used for accumulation of data
   into the ciphertext.

1. `path_measure_crypto_functions.h, .cpp` : The cryptographic
   functions such as joint public key, evaluation keys generation
   along a path.

The example with three nodes and a controller can be run using the
shell script `demos/demoscript_p2p_path_measurement.sh`

To run the example without the script, run the following commands from
the `build` directory:

> `bin/controller_statistics -n Controller -s <socket_address> -m <network_map_file_path> -Wssloff`

> `bin/network_statistics -n <node_name> -s <socket_address> -m <network_map_file_path> -f <input_file_path> -Wssloff`

For a controller and three nodes, the example run would be the following commands:

> `bin/controller_statistics -n Controller -s localhost:50054 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -Wssloff`

> `bin/network_statistics -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file -Wssloff`

> `bin/network_statistics -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file -Wssloff`

> `bin/network_statistics -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file -Wssloff`

A threshold network with aborts example above in the gRPC
client/server setting has been implemented in the peer-to-peer
framework without the need for a server to route messages between the
nodes. To run the example with 5 nodes where nodes 3 and 5 abort, run
the following commands:

> `bin/thresh_aborts_client -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e multiply -Wssloff`

> `bin/thresh_aborts_client -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e multiply -Wssloff`

> `bin/thresh_aborts_client -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e multiply-abort -Wssloff`

> `bin/thresh_aborts_client -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e multiply -Wssloff`

> `bin/thresh_aborts_client -n Node5 -s localhost:50055 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e multiply-abort -Wssloff`

The argument `-e` specifies the computation to be performed (add,
multiply or vectorsum) along with whether the node aborts before
sending the partial ciphertext. To run the example without the nodes
aborting, only pass the computation to be performed.

## RAVEN network emulation based examples [not verified as working]

This set of examples have not been completely verified. For the curious please read the [README.md](raven/README.md) file in the `raven` subdirectory.

