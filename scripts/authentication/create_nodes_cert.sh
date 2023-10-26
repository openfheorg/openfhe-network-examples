#!bin/bash

example=$1
input="../scripts/authentication/${example}/nodes"
cert_script="../scripts/authentication/create_certs.sh"

#create root certificate and keys
sh ../scripts/authentication/create_root_cert.sh

#read from the nodes file to create certs for each node
while IFS='@' read -r nodename hostname
do
  echo ${nodename}
  `mkdir -p ${nodename} && cd ${nodename} && cp ../ca.* .`
  `sh ${cert_script} ${nodename} ${hostname}`
  `mv server_${nodename}.crt ${nodename}/`
  `mv server_${nodename}.key ${nodename}/`
  `mv client_${nodename}.crt ${nodename}/`
  `mv client_${nodename}.key ${nodename}/`
done < "${input}"

#remove root certificate key
rm -f ca.key
