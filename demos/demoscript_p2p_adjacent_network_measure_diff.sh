#!/bin/bash

echo "$#"

if [[ $# -ne 0 ]]
then
	echo "Running in interactive mode"
else
	echo "Running in non-interactive mode"
fi

#ssl_cert_path="-l ."
ssl_cert_path="-Wssloff"

sleep 2

# Prep tmux
#run the palisade thresh-server
tmux set pane-border-status top
tmux select-pane -T 'Main'

tmux split-window -h
tmux select-pane -T 'Node1'

tmux select-pane -t 1
tmux split-window -h
tmux select-layout even-horizontal  # Even horizontally here to get horizontal even tiles
tmux select-pane -T 'Node2'

sleep 3
tmux select-pane -t 1
printf "Starting Node1\n"

tmux send 'bin/network_measure -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_nm.txt -f demoData/threshnet_input_file_diff '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 2
printf "Starting Node2\n"
tmux send 'bin/network_measure -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_nm.txt -f demoData/threshnet_input_file_diff '$ssl_cert_path ENTER

sleep 200

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "network_measure" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
