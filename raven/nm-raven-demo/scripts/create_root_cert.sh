#!/bin/bash

mypass="pass123"
# Without the following command you get an error: "Can't load /root/.rnd into RNG"
touch ~/.rnd

# Generate valid CA ("certificate authority")
# ca.key - "private key" to create "public" certificates. it must be kept secret/deleted
# ca.crt - CA "public" certificates to verify client's and server's certificates
openssl genrsa -passout pass:$mypass -des3 -out ca.key 4096
openssl req -passin pass:$mypass -new -x509 -days 365 -key ca.key -out ca.crt -subj  "/C=US/ST=New Jersey/L=Newark/O=Duality Technologies Inc./OU=Root/CN=Root CA"

rm -f *.csr

