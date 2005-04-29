#!/bin/sh
PKG=kdebase-3.3.2-i486-1.tgz

clean() { rm -rf install opt usr var ; }
bench() { time sh -c "$1" ; }
untgzrun() { echo "untgz:" ; bench "../untgz $PKG" ; }
targzrun() { echo "tar xzf:" ; bench "tar xzf $PKG" ; }
instprun() { echo "installpkg:" ; bench "ROOT=./ installpkg $PKG > /dev/null" ; }
gziprun() { echo "gzip:" ; bench "gzip -d < $PKG > /dev/null" ; }
zlibrun() { echo "zlib:" ; bench "./zlibbench $PKG" ; }

zlibrun
zlibrun
gziprun
exit
gziprun
gziprun
gziprun
exit
clean
untgzrun
clean
untgzrun
clean
untgzrun
clean
untgzrun

clean
targzrun
clean
targzrun
clean
targzrun
clean
targzrun

exit

clean
instprun
