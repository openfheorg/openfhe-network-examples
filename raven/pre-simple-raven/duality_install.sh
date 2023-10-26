#!/bin/bash

ssl_authentication=$1
producer_name=$2
consumer_name=$3

network_map=$4
hostnames=$5

#./gen_interpreter.sh

sudo ANSIBLE_HOST_KEY_CHECKING=False ansible-playbook -i interpreters.yml -i .rvn/ansible-hosts duality_install.yml
