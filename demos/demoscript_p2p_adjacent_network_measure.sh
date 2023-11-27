#!/bin/bash

usage()
{
    echo "usage: demoscript_adjacent_network_measure.sh [[-i(ternactive)] [-s(sl on)] [-d(ifferent)] | [-h]] "
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
	
	for certdir in Node1 Node2 
	do
		if [[ ! -d "./${certdir}" ]]
		then
			echo "Certificate directory $./${certdir} not found, please run certificate generation script for adjacent_network_measure"
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
tmux select-pane -T 'Node1'

tmux select-pane -t 1
tmux split-window -h
tmux select-layout even-horizontal  # Even horizontally here to get horizontal even tiles
tmux select-pane -T 'Node2'

tmux select-pane -t 1
printf "No Server in P2P mode"

printf "Starting Node1\n"

tmux send 'bin/network_measure -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_nm.txt -f demoData/threshnet_input_file_'$diff_flag' '$ssl_cert_path ENTER
if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi


tmux select-pane -t 2
printf "Starting Node2\n"
tmux send 'bin/network_measure -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_nm.txt -f demoData/threshnet_input_file_'$diff_flag' '$ssl_cert_path ENTER

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "network_measure" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
