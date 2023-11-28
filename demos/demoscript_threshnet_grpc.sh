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
taskset_cmd_5=

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
	abort_flag="-a"
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
	taskset_cmd_5="taskset -c 5"

	ncores=$(nproc)

	if [[ $ncores -lt 6 ]]
	then
		echo "number of logical cores = $ncores, must be >= 6 to run this demo"	
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

tmux select-pane -t 0
tmux split-window -v
tmux select-pane -T 'Server'


printf "Starting server\n"
tmux send "$taskset_cmd_0"' bin/thresh_server -n KS -i localhost -p 50050 '$ssl_cert_path ENTER


if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi

tmux select-pane -t 3
printf "Starting Client1\n"
tmux send "$taskset_cmd_1"' bin/thresh_client -n client1 -p 50050 -i localhost -d 1 -m 5 -c '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 5
printf "Starting Client2\n"
tmux send "$taskset_cmd_2"' bin/thresh_client -n client2 -p 50050 -i localhost -d 2 -m 5 -c '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 2
printf "Starting Client3\n"

tmux send "$taskset_cmd_3"' bin/thresh_client -n client3 -p 50050 -i localhost -d 3 -m 5 '$abort_flag' -c '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 4
printf "Starting client4\n"
tmux send "$taskset_cmd_4"' bin/thresh_client -n client4 -p 50050 -i localhost -d 4 -m 5 -c '$operation' '$ssl_cert_path ENTER

if [[ $interactive_on -ne 1 ]]; then sleep 1; else read -p "Hit any key>" ; printf "\n"; fi
tmux select-pane -t 6
printf "Starting client5\n"	
tmux send "$taskset_cmd_5"' bin/thresh_client -n client5 -p 50050 -i localhost -d 5 -m 5 '$abort_flag' -c '$operation' '$ssl_cert_path ENTER

tmux select-pane -t 0
read -p "Hit any key to close windows and shut down the Threshnet demo>"

#shutdown server
PIDs="$(pgrep -af "thresh_client" | awk '{print $1}')"
kill -9 "$PIDs"

#shutdown tmux
tmux kill-server

#cleanup files
rm -f  client_*.txt server_*.txt

#put terminal back into a good state. 
reset
