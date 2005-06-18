#!/usr/bin/env python
from spkg import *

tgz = untgz_open('rxvt-2.7.10-i486-2.tgz')
while tgz.get_header():
  print tgz.f_name
  tgz.write_file()
