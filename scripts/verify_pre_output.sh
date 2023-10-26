#!/bin/bash

if diff -Bbq $1 $2
then echo files are same
else echo files are different
fi
