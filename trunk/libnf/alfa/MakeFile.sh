#!/bin/bash

clean="clean"
path="../nfdump/bin"
#pythonv="2.6"
pythonv=$(python -V 2>&1 | cut -f 2 -d" " | cut -f 1,2 -d".")

echo "Python version ${pythonv}"

if [[ $1 = $clean ]] ; then
	rm $path/nfreader.o
	rm $path/nfreader.py
	rm $path/nfreader.pyc
	rm $path/_nfreader.so
	rm $path/nfreader_wrap.c
	rm $path/nfreader_wrap.o
	rm $path/nfreader.i
	rm $path/script.py
else
	swig -python nfreader.i
	echo -e "swig....OK"

	gcc -c $path/nfreader.c -D main=oldmain nfreader_wrap.c -I /usr/include/python${pythonv}
	echo -e "gcc.....OK"

	cp script.py $path
	cp nfreader.i $path
	mv nfreader.o $path
	mv nfreader.py $path
	mv nfreader_wrap.c $path
	mv nfreader_wrap.o $path

	ld -shared $path/nfreader.o $path/nfreader_wrap.o $path/minilzo.o $path/nfx.o $path/nffile.o $path/flist.o $path/util.o -o $path/_nfreader.so
	echo -e "ld......OK"
fi
