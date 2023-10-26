#!bin/bash

ssl_authentication=Wssloff

if $ssl_authentication
`kill -9 $(pgrep -f "pre_server_demo")`
`bin/pre_server_demo -p 12345 -$ssl_authentication`
