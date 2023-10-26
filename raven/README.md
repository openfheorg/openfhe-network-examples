# How to Run Examples with the RAVEN Virtual Network Emulator System

Running examples with raven has two components:

1. Creating OpenFHE containers
1. Building raven nodes and executing scripts to run the example in the nodes 

# Creating OpenFHE containers

Uses a raven base ubuntu -2004 with minimal updates, only adding user rvn.

Raven virtual machine creates the containers.

Images are then saved as flat tar files can be then be used to load
later in the raven nodes.

The following tar files are created:

1. `base-min.tar` - docker image with openfhe-development repository,
   dev branch built locally and the `lib`, `include`, `share`, `bin`
   directories copied into the container (note the requirement for `dev`
   may be lifted in the future as features are released into the master
   branch).
1. `base.tar` - docker image with openfhe-development repository built
   and grpc installed locally.
1. `examples-min.tar` - docker image with ops5g examples repository
   built locally with the base-min image as the base image and the
   `build/bin`, `demoData` and `NetworkMaps` directories copied into the
   container.
1. `examples.tar` - docker image with ops5g examples repository built
   locally with the base-min image as the base image.


## Setup

Before running the script to create the OpenFHE containers, create an
access token from gitlab (this might be needed to clone the examples
repository) with the following steps:

1. Select the examples repository and go to User Settings.
1. Click on Access Tokens on the left side menu. Enter a Token name,
   expiry date and choose read access.
1. Copy the Token ID displayed after clicking on 'create personal
   access token'.
1. Enter this Token ID in line 22 of `builder-examples.dockerfile`
   replacing `<<username:accesstoken>>`.

```
cd create-openfhe-container
sudo ./run.sh # builds the raven virtual machine
sudo ./build-all-containers.sh # builds all the containers
```

`run.sh` script creates the builder node in raven framework that is
used to compile the palisade and ops5g examples code to create the
corresponding docker images. 

`build-all-containers.sh` script creates
and saves the tar files for the docker images.

* This is not the correct or proper way to package code, but it is quick and dirty.

# Building raven nodes and executing scripts to run the example on those nodes 

The different directories with raven uses the docker images built from
create-palisades-container to run the following examples:

 1. `nm-raven-demo`: an adjacent network measure example with two nodes
    and a router to simulate a connection between these two nodes
 1. `pre-simple-raven`: Proxy ReEncryption (PRE) example with one trust
    zone, two brokers, a key server, two producers and three consumers
    (charlie is an unauthorized consumer)
 1. `pre-final-raven`: PRE example with three trust zones, three
    brokers, three key servers, two producers and three consumers
    (charlie is an unauthorized consumer)

The examples folders is mounted as a `/palisades` directory in the raven VMs created.

The following files are part of every directory:

 - `model.js` -- defines the nodes and routers and the topology. 
 - `run.sh` -- builds the raven VMs as defined in `model.js`
 - `playbook.sh` -- configures network settings for the raven VMs
   created by `run.sh` using information defined in `vars/hosts.yml`


Steps to run examples:

> `cd <examples-directory>`

copy the tar files into the `<examples-directory>` (`nm-raven-demo`,
`pre-simple-raven`, `pre-final-raven`) using the following command:

> `sudo cp ../create-palisades-container/*.tar dockerimages/`

Then build and configure the raven VMs by running the following commands:

> `sudo ./run.sh`

> `sudo ./playbook.sh`

## Running with ssl

To run with `ssl`, we need to first create a root certificate that is
then used by the nodes to create their signed server and client
certificates (this is just a dummy setup to test that `ssl` with
certificates can be setup with raven nodes and works).

```
cd certs/
sh ../scripts/create_root_cert.sh
cd ..

```

# Network Measurement example with two nodes

To run the `nm-raven-demo` example without `ssl` using the input file
path `/data/demoData/threshnet_input_file_diff`, run

> `sudo ./duality_install.sh Wssloff /data/demoData/threshnet_input_file_diff`

The script `duality_install.sh` is used to install pre-requisites in
each node to run the adjacent network measure example and runs the
docker container using the docker image loaded from
`examples-min.tar`. It takes as arguments the flag for running with or
without `ssl`, and the input file path for the example.

To run the same example with `ssl`, run the following command:

> `sudo ./duality_install.sh l /data/demoData/threshnet_input_file_diff`

The output can be verified from the output files
`outputs/docker_ouput_1` and `outputs/docker_output_2`.

Just for reference, running `duality_install.sh` script is equivalent to
the following manual steps without `ssl` (the certificate setup would
have to be done manually as well to run with `ssl`):

`ssh` into the machines with `eval $(sudo rvn ssh Node1)`, `eval $(sudo rvn ssh Node2)`

In both nodes, run the following:

> `sudo apt-get install docker.io`

load the docker image from the tar file: 

> `sudo docker load -i /palisades/dockerimages/examples-min.tar`

run the docker container with the loaded docker image: 

> `sudo docker run -it --network host -p 50051:50051 -p 50052:50052 -v /palisades/certs:/certs -v /palisades/NetworkMaps:/NetworkMaps palisades-examples:jammy /bin/bash`

inside the docker container in Node1, run the following command:

> `/usr/local/bin/network_measure -n Node1 -s 10.0.0.1:50051 -m /NetworkMaps/NetworkMap_nm.txt -f /data/demoData/threshnet_input_file_diff -Wssloff`

inside the docker container in Node1, run the following command:

> `/usr/local/bin/network_measure -n Node2 -s 10.0.1.2:50052 -m /NetworkMaps/NetworkMap_nm.txt -f /data/demoData/threshnet_input_file_diff -Wssloff`


# PRE with one trust zone (pre-simple-raven) and PRE with three trust zones (pre-final-raven)

The processes in this example need to be run in a specific sequence
(key server is active first, upstream processes are active before
their corresponding downstream processes). So for this example, the
`duality_install.sh` only installs the required pre-requisites for the
raven VMs in parallel. The script `run_pre.sh` runs the PRE processes in
sequence as listed in the `interpreter.yml`.

Run the pre-requisites with the `duality_install.sh` as:

> `sudo ./duality_install.sh`

To run the `pre-simple-raven` or `pre-final-raven` example without
`ssl`, run the following command from the corresponding directory

> `sudo ./run_pre.sh Wssloff <producer-name> <consumer-name> <NetworkMap-path> <AccessMap-path>`

For example, 

> `sudo ./run_pre.sh Wssloff P0 C0 NetworkMaps/pre_networkmap AccessMaps/pre_accessmap`

To run the `pre-simple-raven` or `pre-final-raven` example without
`ssl`, run the following command from the corresponding directory

> `sudo ./run_pre.sh l <producer-name> <consumer-name> <NetworkMap-path> <hostnames-file-path>`

For example

> `sudo ./run_pre.sh l P0 C0 NetworkMaps/pre_networkmap AccessMaps/pre_accessmap scripts/authentication/hostnames`

# Making changes to PRE topology

To make changes to the topology, the following files need to be modified:

1. `model.js` - to include any new nodes and link it to the correct switch
1. `vars/hosts.yml` - hostname and ip address of the new nodes added
   in `model.js`. The ip address needs to be in the right subnet mask
   of the switch the node is linked to.
1. `interpreters.yml` (the sequence of the nodes listed need to be in
   the same order to maintain upstream-downstream order in the pre
   application)

1. `playbook.yml [switches]` - To modify switch number of
connections - If adding new nodes to the topology results in more than
3 or 4 connections to a particular switch, this needs to be modified
as needed in this file

1. `NetworkMaps/` - To modify network map for the pre application

1. To run the application with `ssl`, the hostnames for the all the
   new nodes need to be specified in
   `scripts/authentication/hostnames` file.
