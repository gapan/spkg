#!/usr/bin/env python
from sys import argv, exc_info, path
path.append('../build/lib.linux-i686-2.4')
from spkg import *

def status(tgz, total, current):
  print "\033[0G[%s] (%u of %u)..." % (tgz.tgzfile,current,total)

# For each file do:
for file in argv[1:]:
  try:
    # Open tgz file.
    tgz = Untgz(file, status)
    # While we can successfully get next file's header from the archive...
    while tgz.get_header():
      if tgz.f_name == 'install/slack-desc':
        # ...we will be extracting that file to a disk using its original name.
        print tgz.write_data()
      else:
        # ...we will be extracting that file to a disk using its original name.
        tgz.write_file()
  except:
    # if error occured, show it
    print exc_info()[1]
    print 'FAILED!'
  else:
    print 'OK!'
