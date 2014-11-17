#!/bin/bash 

HOSTS="root@hawk.cis.vutbr.cz root@coyote.cis.vutbr.cz tpoder@185.62.109.9 test-ubuntu"
DIST="Net-NfDump-1.04"
EXT=".tar.gz"

for h in $HOSTS; do 
	scp ${DIST}${EXT} ${h}:/tmp/
	ssh ${h} "export AUTOMATED_TESTING=1 ; cd /tmp/ && (rm -rf ${DIST}/ ; tar xzf ${DIST}${EXT}) && cd ${DIST} && perl Makefile.PL && make test ; rm -rf Net-Dump-*"

done



