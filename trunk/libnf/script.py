#!/usr/bin/python

import sys
import subor

# Help function
def help():
	print "USAGE: ./script -r <nfdump file>"
	return


if ((len(sys.argv) == 3) and (sys.argv[1] == '-r')):
	# C funcion call
	subor.head([sys.argv[1], sys.argv[2]])
else:
	# Error: Bad arguments
	help()

exit()
