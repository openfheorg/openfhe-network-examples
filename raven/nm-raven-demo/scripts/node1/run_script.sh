#!bin/bash

ssl_authentication=$1
ssl_arg=$1
input_file=$2

if [ "${ssl_authentication}" = "l" ]
then
  `cd /palisades/certs && mkdir Node1 && cd Node1 && cp ../ca.* . && sh /palisades/scripts/create_certs.sh Node1 n1`
  ssl_arg="$1 /certs"
fi

`sudo docker run -d --network host -p 50051:50051 -p 50052:50052 -v /palisades/certs:/certs -v /palisades/NetworkMaps:/NetworkMaps palisades-examples:jammy bash -c "/usr/local/bin/network_measure -n Node1 -s 10.0.0.1:50051 -m /NetworkMaps/NetworkMap_nm.txt -f $input_file -$ssl_arg" > /palisades/outputs/output_nm_containerid_1`

