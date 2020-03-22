#!/bin/bash

gcc -o process process.c

let "i=0"
rm processed.csv

for f in csv/*.gz; do
	let "i=i+1"
	echo $i $f
	cat $f | gunzip | ./process >> processed.csv
done
