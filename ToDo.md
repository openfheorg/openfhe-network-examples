Todo
=

PRE [all verified]
=

`demoscript_pre_grpc_tmux.sh` 

[x] verify operation and update README.md if needed.

[x] should not report on failure of aes decrypt if other code fails. 

[x] add checks for certs, an report if not there. 

[x] add checks for demoData directory and report if not there. 

[x] speed up demo waits.

[x] verify the two cases using different terminals. 

`bin/pre_server_demo`

[x] manually run pre_server_demo with 5 windows [currently being verified]

[x] manually run pre_server_demo with 8 windows

 [x] verify operation and update README.md if needed.
 
 [x] change output to report full name of security mode 
 
 [ ] update scripts to support command line arguments like `demoscript_threshnet_aborts_grpc_tmux`
    do merge of tmux into threshnet first. 
 
Threshold with aborts [all verified]
=
 
[x] run five window example

    had to change output to correct form. 

[x] run four window example with abort 

    I do not think this example works as written in multiple windows 
	removed the examle as the example from the readme file
 
 
 `demoscript_threshnet_grpc_tmux.sh`

[x] verify operation and update README.md if needed.

[x] need to add aborts as an input parameter to demo script it is true/false

[x] need to add aborts to the utils.cpp parameter documentation and updat readme and to announce abort in the code. 

[x] verify operation of command line arguments 

`demoscript_threshnet_grpc_tmux_taskset.sh`

[x ] This is not documented anywhere verify operation and update README.md if needed.

does the same as tmux except uses taskset. need to have a function
that limits the # processors to those available.  or just keep it.
	
Renamed to `demoscript_threshnet_grpc_tmux.sh and demoscript_threshnet_grpc_tmux.sh`

[x] merge taskset into `demoscript_threshnet_grpc_tmux.sh` and delete tmux version. 

Adjacent Co-Measurement [not verified as working]
===

`demoscript_adjacent_network_measure_diff.sh`

[ ] verify operation and update README.md if needed.

`demoscript_adjacent_network_measure_same.sh`

[ ] verify operation and update README.md if needed.

P2P [not verified as working]
=

`demoscript_fhe_aborts_grpc_p2p_tmux.sh`

[ ] verify operation and update README.md if needed.


`demoscript_fhe_aborts_grpc_p2p_taskset.sh`

[ ] This is not documented anywhere. Verify operation and update README.md if needed.

`demoscript_p2p_adjacent_network_measure_diff.sh`

[ ] verify operation and update README.md if needed.

`demoscript_p2p_adjacent_network_measure_same.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_path_measurement.sh`

[ ] verify operation and update README.md if needed.


test code [not verified as working]
--

`demoscript_p2p_testbroadcast.sh`

[ ] verify operation and update README.md if needed.


`demoscript_p2p_testnodes.sh`

[ ] verify operation and update README.md if needed.
