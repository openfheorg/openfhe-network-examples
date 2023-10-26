#!bin/bash

ssl_authentication=$1
ssl_arg=$1
input_file=$2

if [ "${ssl_authentication}" = "l" ]
then
  `cd /palisades/certs && mkdir Node2 && cd Node2 && cp ../ca.* . && sh /palisades/scripts/create_certs.sh Node2 n2`
  ssl_arg="$1 /certs"
fi

`mkdir -p outputs`

`sudo docker run -d --network host -p 50051:50051 -p 50052:50052 -v /palisades/certs:/certs -v /palisades/NetworkMaps:/NetworkMaps palisades-examples:jammy bash -c "/usr/local/bin/network_measure -n Node2 -s 10.0.1.2:50052 -m /NetworkMaps/NetworkMap_nm.txt -f $input_file -$ssl_arg" > /palisades/outputs/output_nm_containerid_2`

