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

	for certdir in Node1 Node2 Node3 Node4
	do
		if [[ ! -d "./${certdir}" ]]
		then
			echo "Certificate directory $./${certdir} not found, please run certificate generation script for p2p_testbroadcast_demo"
			exit -1
		fi
	done	

else
	echo "Running without ssl"
	ssl_cert_path="-Wssloff"
fi

tmux set pane-border-status top
tmux select-pane -T 'Node1'

tmux select-pane -t 0
tmux split-window -h
tmux select-layout even-horizontal  # Even horizontally here to get horizontal even tiles
tmux select-pane -T 'Node3'

# Reorganize panes
tmux select-pane -t 0
tmux split-window -v

tmux select-pane -T 'Node2'

tmux select-pane -t 2
tmux split-window -v
tmux select-pane -T 'Node4'

# Now we have 4 panes [(1/2), (3/4)]

# Now we start the clients first
tmux select-pane -t 1 
printf "Starting node 2\n"
tmux send 'bin/testbroadcast -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 2 
printf "Starting node 3\n"
tmux send 'bin/testbroadcast -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 3 
printf "Starting node 4\n"
tmux send 'bin/testbroadcast -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 0 
printf "Starting node 1 (broadcaster)\n"
tmux send 'bin/testbroadcast -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMapBroadcast.txt '$ssl_cert_path ENTER


tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the demo>"

shutdown server
PIDs="$(pgrep -af "testbroadcast" | awk '{print $1}')"
kill -9 "$PIDs"

shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
