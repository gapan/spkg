#!/usr/bin/env python
from distutils.core import setup, Extension

spkg = Extension('spkg',
#  define_macros = [],
  include_dirs = ['/usr/include', 'include', 'libs/sqlite', 'libs/glib'],
  libraries = ['spkg', 'glib-2.0', 'sqlite3'],
  library_dirs = ['libs/sqlite', 'libs/glib', '.build', '/usr/lib'],
  sources = ['pyspkg/module.c'],
  depends = ['pyspkg/package.c','pyspkg/packages.c','pyspkg/file.c','pyspkg/files.c']
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
