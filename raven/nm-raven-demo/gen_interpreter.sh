#!/bin/bash

filename="interpreters.yml"

# [] is the raven name for the group
hosts=$(cat .rvn/ansible-hosts | grep -v "\[" | cut -d ' ' -f 1)

printf "" > $filename
printf "[all:vars]\n\tansible_python_interpreter=/usr/bin/python3\n\n" >> $filename

printf "[nodes]\n" >> $filename
for host in $hosts; do
	if [[ "s" != "${host:0:1}" ]]; then
		printf "$host\n" >> $filename
	fi
done
