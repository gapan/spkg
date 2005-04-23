set print pretty on
set logging file .gdb.log
set logging overwrite on
set logging on
set args testpkg-1.0-i486-1.tgz
file ./installpkg
b pkgdb_open
r

