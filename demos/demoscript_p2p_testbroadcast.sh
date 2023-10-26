#!/bin/bash

echo "Num args received: $#"

if [[ $# -ne 0 ]]
then
	echo "Running in interactive mode"
else
	echo "Running in non-interactive mode"
fi

#ssl_cert_path="-l ."
ssl_cert_path="-Wssloff"

# Take down any previous sessions and spin up new ones
sleep 1

tmux split-window -v
tmux select-pane -t 0
tmux split-window -h
tmux select-pane -t 2
tmux split-window -h
tmux split-window -h

# Now we have 4 panes [(1/2), (3/4)]

# Now we start the clients first
tmux select-pane -t 1 
printf "Starting node 2\n"
tmux send 'bin/testbroadcast -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

sleep 1
tmux select-pane -t 2 
printf "Starting node 3\n"
tmux send 'bin/testbroadcast -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

sleep 1
tmux select-pane -t 3 
printf "Starting node 4\n"
tmux send 'bin/testbroadcast -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

sleep 1
tmux select-pane -t 4 
printf "Starting node 1 (server)\n"
tmux send 'bin/testbroadcast -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

sleep 40
