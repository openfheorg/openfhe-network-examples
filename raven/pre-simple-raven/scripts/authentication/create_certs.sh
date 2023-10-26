#!/bin/bash

mypass="pass123"

nodename=$1
hostname=$2

if [ "${nodename}" = "" ]
then
    servername="server"
    clientname="client"
else
    servername="server_${nodename}"
    clientname="client_${nodename}"  
fi

if [ "${hostname}" = "" ]
then
    hostname="localhost"
fi

# Without the following command you get an error: "Can't load /root/.rnd into RNG"
touch ~/.rnd

# the server common name (CN) must be used by clients to create channel:
# https://stackoverflow.com/questions/40623793/use-ssl-in-grpc-client-server-communication
# https://support.dnsimple.com/articles/what-is-common-name/
# Generate valid Server Key/Cert. 192.168.1.135 is my laptop where I start the server
# server.key - "private key" to encrypt outgoing messages from the server
# server.crt - "public key" to decrypt messages from the server
openssl genrsa -passout pass:${mypass} -des3 -out ${servername}.key 4096
openssl req -passin pass:${mypass} -new -key ${servername}.key -out ${servername}.csr -subj  "/C=US/ST=New Jersey/L=Newark/O=Duality Technologies Inc./OU=Server/CN=$hostname"
openssl x509 -req -passin pass:${mypass} -days 365 -in ${servername}.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out ${servername}.crt

# Remove passphrase from the Server Key
openssl rsa -passin pass:$mypass -in ${servername}.key -out ${servername}.key

# Generate valid Client Key/Cert. 172.20.22.156 is my laptop where I start clients
# client.key - "private key" to encrypt outgoing messages from the client
# client.crt - "public key" to decrypt messages from the client
openssl genrsa -passout pass:${mypass} -des3 -out ${clientname}.key 4096
openssl req -passin pass:${mypass} -new -key ${clientname}.key -out ${clientname}.csr -subj  "/C=US/ST=New Jersey/L=Newark/O=Duality Technologies Inc./OU=Client/CN=$hostname"
openssl x509 -passin pass:${mypass} -req -days 365 -in ${clientname}.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out ${clientname}.crt

### # Remove passphrase from Client Key
openssl rsa -passin pass:${mypass} -in ${clientname}.key -out ${clientname}.key

# Delete ca.key and all intermediate files:
rm -f *.csr
