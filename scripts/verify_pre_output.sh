#!/bin/bash
if [[ $# -eq 0 ]]
then
	echo "Usage: verify_pre_output.sh file1 file2"
	exit
fi

if (diff -Bbq $1 $2)
then echo files are same
else echo files are different
fi
