12/01/2020: PALISADE Serialization Examples v1.0 is released

* Initial prototype release
* Simple Client-Server Real Number Serialization examples - files and
  mutex, and boost sockets
* Tested with PALISADE v1.10.5

7/7/2021: PALISADE Serialization Examples v2.0 is released

* Added the following examples: 
  * Threshold Encryption Network Service Example 
  * Proxy-re-encryption (PRE) Network Service Example -- IPC with Asynchronous Server 
  * PRE Network Service For AES Key Distribution Demo

* Tested with PALISADE v1.11.3

Note in late 2022 new development work on PALISADE ceased, and all new
development migrated to the follow on OpenFHE library.

Feb 2023: OpenFHE Experiments for Ops5G (v3.0) is released

* Use of PALISADE deprecated, all examples now run under OpenFHE

* Deprecated serialization examples that used boost ASIO sockets or
  mutex and files. Now all examples use gRPC in either a client-server
  or peer-to-peer framework.
  
* Updated the PRE Network demos to support multiple brokers, multiple
  key servers / trust zones, mutliple channels and network maps.
  
* Updated Threshold FHE Examples to include recovery from Aborted
  nodes -- two recovery modes are supported: abort by a single node
  and abort by a minority number of nodes.
  
* Peer-to-peer communication framework based on gRPC. Used to simplify
   several examples and reduce network traffic (by eliminating round
   trip messages to a server).
   
* Two Network measurement examples: 1) Adjacent measurment
  co-verification between two nodes, with and without a controller
  node, both as client-server and peer-to-peer 2) Statistics
  measurement accumulation along a path.

