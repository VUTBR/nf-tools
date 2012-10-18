#!/bin/bash

PYTHON_VERSION=`python -V 2>&1 | cut -f 2 -d" " | cut -f 1,2 -d"."`
SCRIPTS_PATH=`pwd`
OBJ_PATH_NFDUMP="../nfdump/bin/"
OBJ_PATH="obj/"
LIB_PATH="lib/"
OBJECTS_NFDUMP="	${OBJ_PATH_NFDUMP}nfdump.o 
					${OBJ_PATH_NFDUMP}nfstat.o 
					${OBJ_PATH_NFDUMP}nfexport.o 
					${OBJ_PATH_NFDUMP}nf_common.o 
					${OBJ_PATH_NFDUMP}nflowcache.o 
					${OBJ_PATH_NFDUMP}util.o 
					${OBJ_PATH_NFDUMP}minilzo.o 
					${OBJ_PATH_NFDUMP}nffile.o 
					${OBJ_PATH_NFDUMP}nfx.o 
					${OBJ_PATH_NFDUMP}nfxstat.o 
					${OBJ_PATH_NFDUMP}flist.o 
					${OBJ_PATH_NFDUMP}fts_compat.o 
					${OBJ_PATH_NFDUMP}grammar.o 
					${OBJ_PATH_NFDUMP}scanner.o 
					${OBJ_PATH_NFDUMP}nftree.o 
					${OBJ_PATH_NFDUMP}ipconv.o 
					${OBJ_PATH_NFDUMP}nfprof.o"

OBJECTS="	nfdump_wrap.o
			${OBJ_PATH}nfdump.o 
			${OBJ_PATH}nfstat.o 
			${OBJ_PATH}nfexport.o 
			${OBJ_PATH}nf_common.o 
			${OBJ_PATH}nflowcache.o 
			${OBJ_PATH}util.o 
			${OBJ_PATH}minilzo.o 
			${OBJ_PATH}nffile.o 
			${OBJ_PATH}nfx.o 
			${OBJ_PATH}nfxstat.o 
			${OBJ_PATH}flist.o 
			${OBJ_PATH}fts_compat.o 
			${OBJ_PATH}grammar.o 
			${OBJ_PATH}scanner.o 
			${OBJ_PATH}nftree.o 
			${OBJ_PATH}ipconv.o 
			${OBJ_PATH}nfprof.o"

cd ../nfdump
./configure
make
cd ${SCRIPTS_PATH}
cp ${OBJECTS_NFDUMP} obj/

swig -python nfdump.i
gcc -c nfdump_wrap.c -I /usr/include/python${PYTHON_VERSION}
ld -shared ${OBJECTS} -o _nfdump.so