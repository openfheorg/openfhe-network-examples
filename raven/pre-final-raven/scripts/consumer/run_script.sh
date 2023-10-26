#!bin/bash

ssl_authentication=$1
ssl_arg=$1
producer_name=$3
consumer_name=$2
channel_name="${producer_name}-${consumer_name}"

input="/palisades/"$4
certificate_hostnames="/palisades/"$5

found_params=0
run_command=""

while IFS=':' read -r nodename params
do
  if [ "${nodename}" = "${consumer_name}" ]
  then
    run_command="/usr/local/bin/pre_consumer_demo -n "${consumer_name}" "$params" -c "${channel_name}
    found_params=1
    break
  else
    found_params=0
  fi
done < "${input}"

if [ ${found_params} = 0 ]
then
  echo "No entry in Network map for "${consumer_name} > /palisades/outputs/errors/consumererror
fi

if [ "${ssl_authentication}" = "l" ]
then
  found_host=0
  while IFS=':' read -r nodename hostname
  do
    if [ "${nodename}" = "${consumer_name}" ]
    then
      `cd /palisades/certs && mkdir ${consumer_name} && cd ${consumer_name} && cp ../ca.* . && sh /palisades/scripts/authentication/create_certs.sh ${consumer_name} ${hostname}`
      found_host=1
      break
    else
      found_host=0
    fi
  done < "${certificate_hostnames}"

  if [ ${found_host} = 0 ]
  then
    echo "No entry in hostnames for "${consumer_name} > /palisades/outputs/errors/consumererror
  fi

  ssl_arg="$1 /certs"
fi

`sudo docker run -d --network host -p 50050-50055:50050-50055 -p 50060-50065:50060-50065 -p 50070-50075:50070-50075 -v /palisades/certs:/certs palisades-examples:jammy bash -c "${run_command} -${ssl_arg}" > /palisades/outputs/output_pre_containerid_c`

