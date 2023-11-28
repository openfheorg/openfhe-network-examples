#!/bin/bash

usage()
{
    echo "usage: demoscript_threshnet_grpc_tmux.sh [[-i(ternactive)] [-s(sl on)] [-a(bort on)] [-t(askset on) ] [-c(omputation add|multiply|vectorsum] | [-h]] "
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
aborts_on=
operation="add"

taskset_on= 
taskset_cmd_0=
taskset_cmd_1=
taskset_cmd_2=
taskset_cmd_3=
taskset_cmd_4=

while [ "$1" != "" ]; do
    case $1 in
        -a | --abort )          aborts_on=1
                                ;;
        -i | --interactive )    interactive_on=1
                                ;;
        -c | --computation )      shift
								operation=$1
                                ;;
        -s | --ssl )            ssl_on=1
                                ;;
        -t | --taskset )        taskset_on=1
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

if [[ "$aborts_on" = 1 ]]
then
	echo "Running with aborts activated"
	abort_flag="-abort"
else
	echo "Running without aborts"
	abort_flag=
fi

if [[ "$ssl_on" = 1 ]]
then
	echo "Running with ssl"
	ssl_cert_path="-l ."
else
	echo "Running without ssl"
	ssl_cert_path="-Wssloff"
fi

if [[ $taskset_on = 1 ]]
then
	echo "Running with taskset on multiple nodes"
	taskset_cmd_0="taskset -c 0"
	taskset_cmd_1="taskset -c 1"
	taskset_cmd_2="taskset -c 2"
	taskset_cmd_3="taskset -c 3"
	taskset_cmd_4="taskset -c 4"

	ncores=$(nproc)

	if [[ $ncores -lt 5 ]]
	then
		echo "number of logical cores = $ncores, must be >= 5 to run this demo"	
		exit -1
	else
		echo "number of logical cores = $ncores"		
	fi
else
	echo "Running without taskset"
fi
   
echo "computing "${operation}

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

printf "No Server in p2p mode"
tmux select-pane -t 2
printf "Starting Client1\n"
tmux send "$taskset_cmd_0"' bin/thresh_aborts_client -n Node1 -s localhost:50051 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 4
printf "Starting Client2\n"
tmux send "$taskset_cmd_1"' bin/thresh_aborts_client -n Node2 -s localhost:50052 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 1
printf "Starting Client3\n"

tmux send "$taskset_cmd_2"' bin/thresh_aborts_client -n Node3 -s localhost:50053 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation$abort_flag' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 3
printf "Starting client4\n"
tmux send "$taskset_cmd_3"' bin/thresh_aborts_client -n Node4 -s localhost:50054 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 5
printf "Starting client5\n"	
tmux send "$taskset_cmd_4"' bin/thresh_aborts_client -n Node5 -s localhost:50055 -m ../NetworkMaps/NetworkMap_threshnetaborts.txt -f demoData/threshnetdemo_input_file -e '$operation$abort_flag' '$ssl_cert_path ENTER


tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the Threshnet demo>"

#shutdown stray programs
PIDs="$(pgrep -af "thresh_aborts_client" | awk '{print $1}')"
kill -9 "$PIDs"

#shutdown tmux
tmux kill-server

#put terminal back into a good state. 
reset
