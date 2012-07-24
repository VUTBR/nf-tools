#!/bin/bash

args=("$@")
clean="clean"

if [[ $1 = $clean ]] ; then
	rm subor.o
	rm subor.py
	rm subor.pyc
	rm _subor.so
	rm subor_wrap.c
	rm subor_wrap.o
else
	swig -python subor.i
	echo -e "swig....OK"

	gcc -c subor.c subor_wrap.c -I /usr/include/python2.7
	echo -e "gcc.....OK"

	ld -shared subor.o subor_wrap.o -o _subor.so
	echo -e "ld......OK"
fi