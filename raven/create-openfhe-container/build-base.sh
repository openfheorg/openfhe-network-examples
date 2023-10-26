#!/bin/bash

set -ex

rm -rf /build
mkdir /build
pushd /build; sudo docker build -t palisades-base-builder:latest -f /palisades/builder-base.dockerfile .; popd
