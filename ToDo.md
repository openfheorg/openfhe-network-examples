Todo
=

PRE
=

`demoscript_pre_grpc_tmux.sh` 

[x] verify operation and update README.md if needed.

 used to work, does not now. regnerated certs, doing the regeneration again. 
 

[x] should not report on failure of aes decrypt if other code fails. 

[x ] add checks for certs, an report if not there. 

[x ] add checks for demoData directory and report if not there. 

[x ] speed up demo waits.

[ ] verify the two cases using different terminals. 

`bin/pre_server_demo`

[ ] manually run pre_server_demo with 5 windows

    Fails 
	bin/pre_server_demo -n KS_1 -k localhost:50051 -a demoData/accessMaps/pre_accessmap -l .
	with 
	
	/home/palisade/Documents/openfhe/main/src/pke/lib/scheme/bgvrns/bgvrns-parametergeneration.cpp:70 The specified ring dimension (1024) does not comply with HE standards recommendation (4096).
	
	
	runs for -m INDCPA
	so we make -m manditory that was the issue.
	
	
	now alice cannot find key
	
Server's acknowledgement: Received PKEY. Thank you, Your Server]
Server's acknowledgement public key: Received PKEY. Thank you, Your Server]
Unable to open key file

STOPPED HERE

[ ] manually run pre_server_demo with 8 windows

	cant run till 5 window version works

`bin/pre`

 [x] verify operation and update README.md if needed.
 
 [x] change output to report full name of security mode 
 
Threshold with aborts
=
 
[x] run five window example

    had to change output to correct form. 

[x] run four window example with abort 

    I do not think this example works as written in multiple windows 
	removed the examle as the example from the readme file
 
 
 `demoscript_fhe_aborts_grpc_tmux.sh`

[x] verify operation and update README.md if needed.

does not work, some clients fail to connect try differnt port 50050 worked

[ ] need to add aborts as an input parameter to demo script it is true/false

[x] need to add abprts to the utils.cpp parameter documentation and updat readme and to announce abort in the code. 

`demoscript_fhe_aborts_grpc_taskset.sh`

[x ] This is not documented anywhere verify operation and update README.md if needed.

does the same as tmux except uses taskset. need to have a function
that limits the # processors to those available.  or just keep it.
	
Renamed to `demoscript_threshnet_grpc_tmux.sh and demoscript_threshnet_grpc_tmux.sh`

Adjacent Co-Measurement
===

*STOPPED HERE*

`demoscript_adjacent_network_measure_diff.sh`

[ ] verify operation and update README.md if needed.

`demoscript_adjacent_network_measure_same.sh`

[ ] verify operation and update README.md if needed.

P2P
=

`demoscript_fhe_aborts_grpc_p2p_tmux.sh`

[ ] verify operation and update README.md if needed.


`demoscript_fhe_aborts_grpc_p2p_taskset.sh`

[ ] This is not documented anywhere verify operation and update README.md if needed.

`demoscript_p2p_adjacent_network_measure_diff.sh`

[ ] verify operation and update README.md if needed.

`demoscript_p2p_adjacent_network_measure_same.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_path_measurement.sh`

[ ] verify operation and update README.md if needed.


test code? 
--

`demoscript_p2p_testbroadcast.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_testnodes.sh`

[ ] verify operation and update README.md if needed.
