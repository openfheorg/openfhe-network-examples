#!bin/bash

CID=$(cat /palisades/outputs/output_pre_containerid_p)

sudo docker wait $CID

sudo docker logs $CID > /palisades/outputs/docker_output_p 2>&1 
sed 's/.^M//g' /palisades/outputs/docker_output_p > /palisades/outputs/docker_output_p_trim
mv /palisades/outputs/docker_output_p_trim /palisades/outputs/docker_output_p
