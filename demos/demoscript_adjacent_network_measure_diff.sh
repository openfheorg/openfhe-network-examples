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
tmux select-pane -T 'Server'

tmux select-pane -t 1
tmux split-window -h
tmux select-layout even-horizontal  # Even horizontally here to get horizontal even tiles
tmux select-pane -T 'Controller'

# Reorganize panes
tmux select-pane -t 1
tmux split-window -v

tmux select-pane -T 'Node1'

tmux select-pane -t 3
tmux split-window -v
tmux select-pane -T 'Node2'


sleep 3
tmux select-pane -t 1
printf "Starting Server\n"

tmux send 'bin/thresh_server_measure -i localhost -p 12345 '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 3
printf "Starting Controller\n"

tmux send 'bin/thresh_controller_measure -i localhost -p 12345 -o localhost:50051 -m 2 -s 1 '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 2
printf "Starting Node1\n"

tmux send 'bin/thresh_client_measure -i localhost -p 12345 -o localhost:50051 -d 1 -n Node1 -m 2 -s 1 -f demoData/threshnet_input_file_diff '$ssl_cert_path ENTER

sleep 3
tmux select-pane -t 4
printf "Starting Node2\n"
tmux send 'bin/thresh_client_measure -i localhost -p 12345 -o localhost:50051 -d 2 -n Node2 -m 2 -s 1 -f demoData/threshnet_input_file_diff '$ssl_cert_path ENTER

sleep 200

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "thresh_client_measure" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
