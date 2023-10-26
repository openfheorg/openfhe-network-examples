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
operation="multiply"
num_of_clients=5
aborts=1

sleep 2

# Prep tmux
#run the palisade thresh-server
tmux set pane-border-status top
tmux select-pane -T 'Main'

tmux split-window -h
tmux select-pane -T 'Client1'

tmux select-pane -t 1
tmux split-window -h
tmux select-layout even-horizontal  # Even horizontally here to get horizontal even tiles
tmux select-pane -T 'Client2'

# Reorganize panes
tmux select-pane -t 0
tmux split-window -v

tmux select-pane -T 'Client3'

tmux select-pane -t 2
tmux split-window -v
tmux select-pane -T 'Client4'

tmux select-pane -t 4
tmux split-window -v
tmux select-pane -T 'Client5'

sleep 1
tmux select-pane -t 2
printf "Starting Client1\n"
tmux send 'bin/thresh_aborts_client -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER

sleep 1
tmux select-pane -t 4
printf "Starting Client2\n"
tmux send 'bin/thresh_aborts_client -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER

sleep 1
tmux select-pane -t 1
printf "Starting Client3\n"
if [[ $aborts -ne 1 ]]
then
	tmux send 'bin/thresh_aborts_client -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER
else
	tmux send 'bin/thresh_aborts_client -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation'-abort '$ssl_cert_path ENTER
fi

sleep 1
tmux select-pane -t 3
printf "Starting client4\n"
tmux send 'bin/thresh_aborts_client -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER

sleep 1
tmux select-pane -t 5
printf "Starting client5\n"
if [[ $aborts -ne 1 ]]
then
	tmux send 'bin/thresh_aborts_client -n Node5 -s localhost:50055 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER
else
	tmux send 'bin/thresh_aborts_client -n Node5 -s localhost:50055 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation'-abort '$ssl_cert_path ENTER
fi

sleep 400

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the Threshnet demo>"

shutdown server
PIDs="$(pgrep -af "thresh_client" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
