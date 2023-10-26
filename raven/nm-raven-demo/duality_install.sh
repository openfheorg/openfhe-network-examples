#!/bin/bash

ssl_authentication=$1
input_file=$2

./gen_interpreter.sh

sudo ANSIBLE_HOST_KEY_CHECKING=False ansible-playbook -i interpreters.yml -i .rvn/ansible-hosts duality_install.yml --extra-vars "ssl_authentication="$ssl_authentication --extra-vars "input_file="$input_file
