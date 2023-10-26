#!/bin/bash

security_model=1
for k in {1..1000}
do
  echo "iteration "$k
  bin/pre_d -m $security_model -d 170 #single threaded multi-hop pre

done
