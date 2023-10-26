#!bin/bash

CID=$(cat /palisades/outputs/output_nm_containerid_1)

sudo docker wait $CID

sudo docker logs $CID > /palisades/outputs/docker_output_1 2>&1 
sed 's/.^M//g' /palisades/outputs/docker_output_1 > /palisades/outputs/docker_output_1_trim
mv /palisades/outputs/docker_output_1_trim /palisades/outputs/docker_output_1
