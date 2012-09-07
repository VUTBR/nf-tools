PYTHON_VERSION=`python -V 2>&1 | cut -f 2 -d" " | cut -f 1,2 -d"."`
NFDUMP_PATH=../nfdump/bin

CC = gcc
LD = ld
SHARED = -shared
CFLAGS = -D main=oldmain -I /usr/include/python${PYTHON_VERSION}
OBJECTS = ${NFDUMP_PATH}/nfreader.o ${NFDUMP_PATH}/nfreader_wrap.o ${NFDUMP_PATH}/minilzo.o \
		${NFDUMP_PATH}/nfx.o ${NFDUMP_PATH}/nffile.o ${NFDUMP_PATH}/flist.o ${NFDUMP_PATH}/util.o


all: nfreader_wrap.o
	$(CC) $(CFLAGS) -c nfreader_wrap.c ${NFDUMP_PATH}/nfreader.c

	cp script.py nfreader.i ${NFDUMP_PATH}
	mv nfreader.o nfreader.py ${NFDUMP_PATH}
	mv nfreader_wrap.c nfreader_wrap.o ${NFDUMP_PATH}

	${LD} ${SHARED} ${OBJECTS} -o ${NFDUMP_PATH}/_nfreader.so

nfreader_wrap.o: nfreader_wrap.c	

nfreader_wrap.c: nfreader.i
	swig -python nfreader.i	


.PHONY: clean
clean : 
	rm -f ${NFDUMP_PATH}/nfreader.o ${NFDUMP_PATH}/nfreader.py ${NFDUMP_PATH}/nfreader.pyc ${NFDUMP_PATH}/nfreader.i
	rm -f ${NFDUMP_PATH}/_nfreader.so ${NFDUMP_PATH}/nfreader_wrap.c ${NFDUMP_PATH}/nfreader_wrap.o ${NFDUMP_PATH}/script.py