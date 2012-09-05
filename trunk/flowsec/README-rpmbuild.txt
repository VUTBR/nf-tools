HOW TO BUILD RPM PACKAGE

1. Install "rpm-build" package

	sudo yum install rpm-build

2. Create a folder tree with the main folder named "rpmbuild" and inside create folders "BUILD", "BUILDROOT", "RPMS", "SOURCES", "SPECS", "SRPMS", "tmp".

	cd ~
	mkdir rpmbuild; cd rpmbuild
	mkdir BUILD BUILDROOT RPMS SOURCES SPECS SRPMS tmp

3. In the home folder create the file ".rpmmacros" with the following content:

	%_topdir /path/to/your/homefolder/rpmbuild
	%_tmppath /path/to/your/homefolder/rpmbuild/tmp

   And replace /path/to/your/homefolder with the actual path.

4. Put the spec file inside SPECS folder.

	mv flowsec.spec /path/to/your/homefolder/SPECS

5. Move the folders "bin", "conf", "init" and "man" to the folder named "flowsec-1". Then gzip it and move this archive to "SOURCES" folder.

	mkdir flowsec-1
	mv bin conf init man flowsec-1
	tar -czvf flowsec.tar.gz flowsec-1
	mv flowsec.tar.gz /path/to/your/homefolder/rpmbuild/SOURCES

6. Run

	rpmbuild -ba <specfile>

   in order to build both RPMs and source RPM packages.

7. Finished. You find RPMs in rpmbuild/RPMS and source RPM in rpmbuild/SRPMS.
