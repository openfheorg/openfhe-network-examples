#!/bin/bash

./gen_interpreter.sh

sudo ANSIBLE_HOST_KEY_CHECKING=False ansible-playbook -i interpreters.yml -i .rvn/ansible-hosts playbook.yml
