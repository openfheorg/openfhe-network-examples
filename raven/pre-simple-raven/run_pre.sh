#!/bin/bash

ssl_authentication=$1
producer_name=$2
consumer_name=$3

network_map=$4
access_map=$5
hostnames=$6

#./gen_interpreter.sh

sudo ANSIBLE_HOST_KEY_CHECKING=False ansible-playbook -i interpreters.yml -i .rvn/ansible-hosts run_pre.yml --extra-vars "ssl_authentication="${ssl_authentication} --extra-vars "producer_name="${producer_name} --extra-vars "consumer_name="${consumer_name} --extra-vars "network_map="${network_map} --extra-vars "access_map="${access_map} --extra-vars "hostnames="${hostnames}
