#!/bin/bash

cd ../nfdump/bin
kill="kill"

if [[ $1 = $kill ]] ; then
	pidnum=`cat pidname`
	echo "Killing process no.:"$pidnum
	kill -9 $pidnum
	echo "Process killed"
	rm pidname
else
	./script.py $@
fi