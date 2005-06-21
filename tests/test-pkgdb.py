#!/usr/bin/env python
from sys import argv, exc_info, path
path.append('../build/lib.linux-i686-2.4')
from spkg import *

def print_package(p):
    print '%s (%s) c=%ukB, u=%ukB' % (p.shortname, p.version, p.csize, p.usize)

db_open()

print 'Listing legacy database:'
for p in db_legacy_get_packages(): print_package(p)

print 'Listing spkg database:'
for p in db_get_packages(): print_package(p)

db_close()
