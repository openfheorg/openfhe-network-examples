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
tmux select-pane -T 'Controller'

tmux select-pane -t 1
tmux split-window -h
tmux select-layout even-horizontal  # Even horizontally here to get horizontal even tiles
tmux select-pane -T 'Node1'

# Reorganize panes
tmux select-pane -t 1
tmux split-window -v

tmux select-pane -T 'Node2'

tmux select-pane -t 3
tmux split-window -v
tmux select-pane -T 'Node3'

sleep 3
tmux select-pane -t 1
printf "Starting Controller\n"

tmux send 'bin/controller_statistics -n Controller -s localhost:50054 -m ../NetworkMaps/NetworkMap_statisticscompute.txt '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 3
printf "Starting Node1\n"
tmux send 'bin/network_statistics -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 2
printf "Starting Node2\n"
tmux send 'bin/network_statistics -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 4
printf "Starting Node3\n"
tmux send 'bin/network_statistics -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file '$ssl_cert_path ENTER

sleep 200

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "network_statistics" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
