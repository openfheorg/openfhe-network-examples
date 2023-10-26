#!bin/bash

ssl_authentication=$1
ssl_arg=$1
broker_name=$2

input="/palisades/"$3
certificate_hostnames="/palisades/"$4

found_params=0
run_command=""

while IFS=':' read -r nodename params
do
  if [ "${nodename}" = "${broker_name}" ]
  then
    run_command="/usr/local/bin/pre_broker_demo -n "${broker_name}" "$params 
    found_params=1
    break
  else
    found_params=0
  fi
done < "${input}"

if [ ${found_params} = 0 ]
then
  echo "No entry in Network map for "${broker_name} > /palisades/outputs/errors/brokererror
fi


if [ "${ssl_authentication}" = "l" ]
then
  found_host=0
  while IFS=':' read -r nodename hostname
  do
    if [ "${nodename}" = "${broker_name}" ]
    then
      found_host=1
      `cd /palisades/certs && mkdir ${broker_name} && cd ${broker_name} && cp ../ca.* . && sh /palisades/scripts/authentication/create_certs.sh ${broker_name} ${hostname}`
      break
    else
      found_host=0
    fi
  done < "${certificate_hostnames}"

  if [ ${found_host} = 0 ]
  then
    echo "No entry in hostnames for "${broker_name} > /palisades/outputs/errors/brokererror
  fi

  ssl_arg="$1 /certs"
fi

`sudo docker run -d --network host -p 50050:50050 -p 50051:50051 -p 50052:50052 -v /palisades/certs:/certs palisades-examples:jammy bash -c "${run_command} -${ssl_arg}" > docker_container_id_b`

