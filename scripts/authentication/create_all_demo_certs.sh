#!/bin/bash
for f in ../scripts/authentication/*; do
	if [ -d "$f" ]; then
		echo "$f"
		demo=`basename  $f`
		echo Generating certificates for demo $demo
		sh ../scripts/authentication/create_nodes_cert.sh $demo
	fi
done
