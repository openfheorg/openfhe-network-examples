#!bin/bash

run_command=$1
ssl_arg=$2

if [ "${ssl_authentication}" = "l" ]
then
  `cd /palisades/certs && mkdir ${server_name} && cd ${server_name} && cp ../ca.* . && sh /scripts/authentication/create_certs.sh ${server_name} ${host_name}`
  ssl_arg="$1 /certs"
fi

`${run_command} ${ssl_arg}`
