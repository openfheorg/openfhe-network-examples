#!/bin/bash

set -ex 



rm -rf /base-container
mkdir -p /base-container
#sudo docker run -v /opt:/base-container:rw -d palisades-base-builder:latest "/bin/sh -c sleep 10"
docker pull ubuntu:jammy
docker run --rm --name builder -d palisades-base-builder:latest sleep 30
docker cp builder:/opt /base-container
pushd /base-container; sudo docker build -t palisades-library:jammy -f /palisades/min-base.dockerfile .; popd
