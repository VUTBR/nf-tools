#!/bin/bash 

HOSTS="root@hawk.cis.vutbr.cz root@coyote.cis.vutbr.cz tpoder@185.62.109.9 root@test-ubuntu.net.vutbr.cz root@test-freebsd.net.vutbr.cz"
DIST="Net-NfDump-1.07"
EXT=".tar.gz"

for h in $HOSTS; do 
	echo "***********************************************"
	echo ${h}
	echo "***********************************************"
	scp ${DIST}${EXT} ${h}:/tmp/
	ssh ${h} "export AUTOMATED_TESTING=1 ; cd /tmp/ && (rm -rf ${DIST}/ ; tar xzf ${DIST}${EXT}) && cd ${DIST} && perl Makefile.PL && make -j 8 -j 8 && make test ; rm -rf Net-Dump-*"

done



