#!/usr/bin/env python
from sys import argv, exc_info, path
path.append('../build/lib.linux-i686-2.4')
from spkg import *

for file in argv[1:]:
    try:
        tgz = Untgz(file)
        while tgz.get_header(): tgz.write_file()
    except:
        print exc_info()[1]
