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
num_of_clients=7
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

tmux select-pane -t 0
tmux split-window -v
tmux select-pane -T 'Server'

tmux select-pane -t 5
tmux split-window -h
tmux select-pane -T 'Client6'

tmux select-pane -t 7
tmux split-window -h
tmux select-pane -T 'Client7'

tmux select-pane -t 1
sleep 1
printf "Starting server\n"
tmux send 'taskset -c 0 bin/thresh_server -n KS -i localhost -p 12345 '$ssl_cert_path ENTER

tmux select-pane -t 3
printf "Starting Client1\n"
tmux send 'taskset -c 1 bin/thresh_client -n client1 -p 12345 -i localhost -d 1 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER

tmux select-pane -t 5
printf "Starting Client2\n"
tmux send 'taskset -c 2 bin/thresh_client -n client2 -p 12345 -i localhost -d 2 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER

tmux select-pane -t 2
printf "Starting Client3\n"
if [[ $aborts -ne 1 ]]
then
	tmux send 'taskset -c 3 bin/thresh_client -n client3 -p 12345 -i localhost -d 3 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER
else
	tmux send 'taskset -c 3 bin/thresh_client -n client3 -p 12345 -i localhost -d 3 -m '$num_of_clients' -a 1 -c '$operation' '$ssl_cert_path ENTER
fi

tmux select-pane -t 4
printf "Starting client4\n"
tmux send 'taskset -c 4 bin/thresh_client -n client4 -p 12345 -i localhost -d 4 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER

tmux select-pane -t 7
printf "Starting client5\n"
if [[ $aborts -ne 1 ]]
then
	tmux send 'taskset -c 5 bin/thresh_client -n client5 -p 12345 -i localhost -d 5 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER
else
	tmux send 'taskset -c 5 bin/thresh_client -n client5 -p 12345 -i localhost -d 5 -m '$num_of_clients' -a 1 -c '$operation' '$ssl_cert_path ENTER
fi

tmux select-pane -t 6
printf "Starting client6\n"
tmux send 'taskset -c 6 bin/thresh_client -n client6 -p 12345 -i localhost -d 6 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER

tmux select-pane -t 8
printf "Starting client7\n"
if [[ $aborts -ne 1 ]]
then
	tmux send 'taskset -c 7 bin/thresh_client -n client7 -p 12345 -i localhost -d 7 -m '$num_of_clients' -c '$operation' '$ssl_cert_path ENTER
else
	tmux send 'taskset -c 7 bin/thresh_client -n client7 -p 12345 -i localhost -d 7 -m '$num_of_clients' -a 1 -c '$operation' '$ssl_cert_path ENTER
fi

sleep 300

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the Threshnet demo>"

shutdown server
PIDs="$(pgrep -af "thresh_client" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
