#!/usr/bin/env python
from spkg import *

db_open('root')

p = db_legacy_get_packages()
for pkg in p:
  pk = db_get_pkg(pkg.name,1)
  print pk
  for f in pk.files:
    print f

p = db_get_packages()
for pkg in p:
  pk = db_get_pkg(pkg.name,1)
  print pk
  for f in pk.files:
    print f

db_close()
