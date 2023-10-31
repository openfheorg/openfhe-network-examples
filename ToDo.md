Todo
=

PRE
=

`demoscript_pre_grpc_tmux.sh` 

[x] verify operation and update README.md if needed.
[ ] should not report on failure of aes decrypt if other code fails. 
[ ] add checks for certs, an report if not there. 
[ ] add checks for demoData directory and report if not there. 

[ ] verify the two cases using different terminals. 

`bin/pre_server_demo`

[ ] manually run pre_server_demo with 5 windows

    Fails 
	bin/pre_server_demo -n KS_1 -k localhost:50051 -a demoData/accessMaps/pre_accessmap -l .
	with 
	
	/home/palisade/Documents/openfhe/main/src/pke/lib/scheme/bgvrns/bgvrns-parametergeneration.cpp:70 The specified ring dimension (1024) does not comply with HE standards recommendation (4096).

[ ] manually run pre_server_demo with 8 windows

	cant run till 5 window version works

`bin/pre`
 [x] verify operation and update README.md if needed.
 [x] change output to report full name of security mode 
 
 Threshold with aborts
 =
 
 [x ] run five window example
     had to change output to correct form. 

[x] run four window example with abort 
    I do not think this example works as written in multiple windows 
	removed the examle as the example from the readme file
 
 
 `demoscript_fhe_aborts_grpc_tmux.sh`

[x] verify operation and update README.md if needed.

does not work, some clients fail to connect try differnt port 50050 worked

[ ] need to add aborts as an input parameter to demo script it is true/fase
[ ] need to add abprts to the utils.cpp parameter documentation and updat readme and to announce abort in the code. 

`demoscript_fhe_aborts_grpc_taskset.sh`

[ ] This is not documented anywhere verify operation and update README.md if needed.

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


STOPPED HERE


`demoscript_p2p_adjacent_network_measure_diff.sh`

[ ] verify operation and update README.md if needed.

`demoscript_p2p_adjacent_network_measure_same.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_path_measurement.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_testbroadcast.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_testnodes.sh`

[ ] verify operation and update README.md if needed.
