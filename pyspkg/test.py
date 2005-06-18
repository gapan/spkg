#!/usr/bin/env python
from spkg import *

db_open('root')
pkg = db_get_pkg('sane-1.0.15-i486-1')
db_close()

#n = 'blah/sane-1.0.15-i486-1.tgz'
#n = parse_pkgname(n,5)
#print db_get_pkg(n)

