#!/usr/bin/env python
from spkg import *
from sys import argv, exc_info

# For each file do:
for file in argv[1:]:
  try:
    # Open tgz file.
    tgz = untgz_open(file)
    # While we can successfully get next file's header from the archive...
    while tgz.get_header():
      # ...we will be extracting that file to a disk using its original name.
      tgz.write_file()
  except:
    # if error occured, show it
    print exc_info()[1]
    print 'FAILED!'
  else:
    print 'OK!'
