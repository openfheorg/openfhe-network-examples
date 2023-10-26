#!bin/bash

CID=$(cat /palisades/outputs/output_nm_containerid_2)

sudo docker wait $CID

sudo docker logs $CID > /palisades/outputs/docker_output_2 2>&1
sed 's/.^M//g' /palisades/outputs/docker_output_2 > /palisades/outputs/docker_output_2_trim
mv /palisades/outputs/docker_output_2_trim /palisades/outputs/docker_output_2
