#!/usr/bin/env python
from distutils.core import setup, Extension
from glob import glob

src = ['pyspkg/pyspkg.c']
src.extend(glob('pyspkg/t-*.c'))
src.extend(glob('pyspkg/m-*.c'))

spkg = Extension('spkg',
#  define_macros = [],
  include_dirs = ['/usr/include', 'include', 'libs/sqlite', 'libs/glib'],
  libraries = ['spkg', 'glib-2.0', 'sqlite3', 'z'],
  library_dirs = ['libs/sqlite', 'libs/glib', '.build', '/usr/lib'],
  sources = src,
  depends = ['pyspkg/pyspkg.h', 'pyspkg/pyspkg-priv.h', '.build/libspkg.a']
)

setup(
  name = 'pyspkg',
  version = '1.0',
  description = 'Python bindings for spkg.',
  author = 'Ondrej Jirman',
  author_email = 'megous@megous.com',
  url = 'http://spkg.megous.com',
  ext_modules = [spkg]
)
