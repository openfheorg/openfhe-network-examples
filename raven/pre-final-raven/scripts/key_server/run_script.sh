#!bin/bash

ssl_authentication=$1
ssl_arg=$1
server_name=$2

input="/palisades/"$3
access_map="/"$4
certificate_hostnames="/palisades/"$5

found_params=0

run_command=""

while IFS=':' read -r nodename params
do
  echo "here nodename "${nodename}"; servername "${server_name}
  if [ "${nodename}" = "${server_name}" ]
  then
    found_params=1
    run_command="/usr/local/bin/pre_server_demo -n "${server_name}" -a "${access_map}" "${params} 
    break
  else
    found_params=0
  fi
done < "${input}"

if [ ${found_params} = 0 ]
then
  echo "No entry in Network map for "${server_name} > /palisades/outputs/errors/servererror
fi

if [ "${ssl_authentication}" = "l" ]
then
  found_host=0
  while IFS=':' read -r nodename hostname
  do
    if [ "${nodename}" = "${server_name}" ]
    then
      found_host=1
      `cd /palisades/certs && mkdir ${server_name} && cd ${server_name} && cp ../ca.* . && sh /palisades/scripts/authentication/create_certs.sh ${server_name} ${hostname}`
      break
    else
      found_host=0
    fi
  done < "${certificate_hostnames}"

  if [ ${found_host} = 0 ]
  then
    echo "No entry in hostname for "${server_name} > /palisades/outputs/errors/servererror
  fi

  ssl_arg="$1 /certs"
fi

`sudo docker run -d --network host -p 50050-50055:50050-50055 -p 50060-50065:50060-50065 -p 50070-50075:50070-50075 -v /palisades/AccessMaps:/AccessMaps -v /palisades/certs:/certs palisades-examples:jammy bash -c "${run_command} -${ssl_arg}" > docker_container_id_s`

