#!/usr/bin/python

import os
import sys
import signal
import nfreader

pidnum=os.getpid()

f=open('pidname', 'w')
f.write(str(pidnum))
f.close()

nfreader.main(sys.argv[0:])

sys.exit(0)
