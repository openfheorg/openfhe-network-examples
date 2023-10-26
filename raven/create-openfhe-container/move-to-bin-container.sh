#!/bin/bash

set -ex 

WD=/exp-container

sudo rm -rf $WD
sudo mkdir -p $WD
sudo docker run --rm --name exp-builder -d palisades-examples-builder:latest sleep 10
sudo docker cp exp-builder:/opt $WD
sudo docker cp exp-builder:/palisade-serial-examples/build/bin $WD
sudo docker cp exp-builder:/lib/x86_64-linux-gnu $WD 
#added for libgomp1
pushd $WD; sudo docker build -t palisades-examples:jammy -f /palisades/bin-base.dockerfile .; popd
