#!/usr/bin/env python
from sys import argv, exc_info, path
path.append('../build/lib.linux-i686-2.4')
from spkg import *

db_open('root')
db_close()
