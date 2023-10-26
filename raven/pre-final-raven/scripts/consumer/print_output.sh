#!bin/bash

producer_name=$2
consumer_name=$1
channel_name="$producer_name"-"$consumer_name"
CID=$(cat /palisades/outputs/output_pre_containerid_c)

source_path=":/demoData/keys/consumer_aes_key_"$channel_name
dest_path="/palisades/outputs/demoData/consumer_aes_key_"$channel_name

sudo docker wait $CID

sudo docker cp "$CID$source_path" "$dest_path"

sudo docker logs $CID > /palisades/outputs/docker_output_c 2>&1 
sed 's/.^M//g' /palisades/outputs/docker_output_c > /palisades/outputs/docker_output_c_trim
mv /palisades/outputs/docker_output_c_trim /palisades/outputs/docker_output_c
