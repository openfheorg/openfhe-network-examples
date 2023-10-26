#!/bin/bash

set -ex

sudo docker build -t palisades-examples-builder:latest -f /palisades/builder-examples.dockerfile .
