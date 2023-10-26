#!bin/bash

CID=$(sudo docker container ps -aq --filter "status=running")

if [ ! -z "$CID" ]
then
`sudo docker container stop $CID`
fi

RCID=$(sudo docker container ps -aq)
if [ ! -z "$RCID" ]
then
`sudo docker container rm $RCID`
fi

#remove all docker images
IID=$(sudo docker images ps -aq)
if [ ! -z "$IID" ]
then
`sudo docker rmi $IID`
fi


