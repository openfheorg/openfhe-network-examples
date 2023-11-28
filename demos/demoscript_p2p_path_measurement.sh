#!/bin/bash

usage()
{
    echo "usage: demoscript_adjacent_network_measure.sh [[-i(ternactive)] [-s(sl on)] | [-h]] "
}

#make sure tmux is running
tmuxout=$(pgrep tmux | head -n 1)

if [[ "$tmuxout" -eq "" ]]
then
   echo "please run tmux in another terminal window"
   exit
fi
   
#set defaults
interactive_on=
ssl_on=

while [ "$1" != "" ]; do
    case $1 in
        -i | --interactive )    interactive_on=1
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


if [[ "$ssl_on" = 1 ]]
then
	echo "Running with ssl"
	ssl_cert_path="-l ."
	
	for certdir in Controller Node1 Node2
	do
		if [[ ! -d "./${certdir}" ]]
		then
			echo "Certificate directory $./${certdir} not found, please run certificate generation script for path_measurement"
			exit -1
		fi
	done	

else
	echo "Running without ssl"
	ssl_cert_path="-Wssloff"
fi

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

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 3
printf "Starting Node1\n"
tmux send 'bin/network_statistics -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 2
printf "Starting Node2\n"
tmux send 'bin/network_statistics -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 4
printf "Starting Node3\n"
tmux send 'bin/network_statistics -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_statisticscompute.txt -f demoData/threshnet_input_file '$ssl_cert_path ENTER

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

#shutdown programs if stuck
PIDs="$(pgrep -af "network_statistics" | awk '{print $1}')"
kill -9 "$PIDs"
PIDs="$(pgrep -af "controller_statistics" | awk '{print $1}')"
kill -9 "$PIDs"

#shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
