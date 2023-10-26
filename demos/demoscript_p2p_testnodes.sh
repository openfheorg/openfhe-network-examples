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
num_of_clients=5

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

# Reorganize panes
tmux select-pane -t 0
tmux split-window -v

tmux select-pane -T 'Node3'

tmux select-pane -t 2
tmux split-window -v
tmux select-pane -T 'Node4'

tmux select-pane -t 4
tmux split-window -v
tmux select-pane -T 'Node5'

sleep 3
tmux select-pane -t 2
printf "Starting Node1\n"

tmux send 'bin/testnode -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 4
printf "Starting Node2\n"
tmux send 'bin/testnode -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 1
printf "Starting Node3\n"
tmux send 'bin/testnode -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 3
printf "Starting Node4\n"
tmux send 'bin/testnode -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 5
printf "Starting Node5\n"
tmux send 'bin/testnode -n Node5 -s localhost:50055 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

sleep 40

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "thresh_client" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
