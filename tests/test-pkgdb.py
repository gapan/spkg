#!/usr/bin/env python
from spkg import *

def pp(p):
    print '%s (%s) c=%ukB, u=%ukB' % (p.shortname, p.version, p.csize, p.usize)

if __name__ == '__main__':
    db_open()
    print 'Listing legacy database:'
    pkgs = db_legacy_get_packages()
    for p in pkgs:
        pp(p)

    print 'Listing spkg database:'
    pkgs = db_get_packages()
    for p in pkgs:
        pp(p)

    pkgs = 0
    p = 0
    db_close()
