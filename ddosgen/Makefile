
distdir = ddosgen-0.1

main:
	gcc -lpcap -pthread -O3 -Wall -o ddosgen ddosgen.c 

clean:
	rm mcrep

dist:  
	mkdir $(distdir)
	cp -R Makefile mcrep.c $(distdir)
	find $(distdir) -name .svn -delete
	tar czf $(distdir).tar.gz $(distdir)
	rm -rf $(distdir)

