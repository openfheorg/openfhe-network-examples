#!/bin/bash


usage()
{
    echo "usage: demoscript_p2p_testnodes.sh [[-i(ternactive)] [-s(sl on)] | [-h]] "
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

	for certdir in Node1 Node2 Node3 Node4 Node5
	do
		if [[ ! -d "./${certdir}" ]]
		then
			echo "Certificate directory $./${certdir} not found, please run certificate generation script for p2p_testnodes_demo"
			exit -1
		fi
	done	

else
	echo "Running without ssl"
	ssl_cert_path="-Wssloff"
fi

num_of_clients=5

# Prep tmux
#run the thresh-server
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

tmux select-pane -t 2
printf "Starting Node1\n"

tmux send 'bin/testnode -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER


if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 4
printf "Starting Node2\n"
tmux send 'bin/testnode -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 1
printf "Starting Node3\n"
tmux send 'bin/testnode -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 3
printf "Starting Node4\n"
tmux send 'bin/testnode -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 5
printf "Starting Node5\n"
tmux send 'bin/testnode -n Node5 -s localhost:50055 -m ../NetworkMaps/NetworkMap.txt '$ssl_cert_path ENTER

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "testnode" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
