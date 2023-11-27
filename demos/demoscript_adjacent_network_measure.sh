#!/bin/bash

usage()
{
    echo "usage: demoscript_adjacent_network_measure.sh [[-i(ternactive)] [-s(sl on)] [-d(ifferent)] | [-h]] "
}

#make sure tmux is running
tmuxout=$(pgrep tmux | head -n 1)
echo "$tmuxout"

if [[ "$tmuxout" -eq "" ]]
then
   echo "please run tmux in another terminal window"
   exit
fi
   
#set defaults
interactive_on=
ssl_on=
different_on=

while [ "$1" != "" ]; do
    case $1 in
        -i | --interactive )    interactive_on=1
                                ;;
        -d | --different )      different_on=1
                                ;;
        -s | --ssl )            ssl_on=1
                                ;;
        -h | --help )           usage
                                exit
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

if [[ "$interactive_on" = 1 ]]
then
	echo "Running in interactive mode"
else
	echo "Running in non-interactive mode"
fi

if [[ "$different_on" = 1 ]]
then
	echo "Running with different measurements"
	diff_flag="diff"
else
	echo "Running without aborts"
	diff_flag="same"
fi

if [[ "$ssl_on" = 1 ]]
then
	echo "Running with ssl"
	ssl_cert_path="-l ."
	

	
else
	echo "Running without ssl"
	ssl_cert_path="-Wssloff"
fi

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

tmux send 'bin/thresh_server_measure -n KS -i localhost -p 50000 '"$ssl_cert_path" ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 3
printf "Starting Controller\n"

tmux send 'bin/thresh_controller_measure -n Controller -i localhost -p 50000 -o localhost:50051 -m 2 -s 1 '"$ssl_cert_path" ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 2
printf "Starting Node1\n"

tmux send 'bin/thresh_client_measure -n Node1 -i localhost -p 50000 -o localhost:50051 -d 1 -n Node1 -m 2 -s 1 -f demoData/threshnet_input_file_'$diff_flag' '"$ssl_cert_path" ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 4
printf "Starting Node2\n"
tmux send 'bin/thresh_client_measure -n Node2 -i localhost -p 50000 -o localhost:50051 -d 2 -n Node2 -m 2 -s 1 -f demoData/threshnet_input_file_'$diff_flag' '"$ssl_cert_path" ENTER

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "thresh_client_measure" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
