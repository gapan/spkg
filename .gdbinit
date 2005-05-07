set print pretty on
set logging file .gdb.log
set logging overwrite on
set logging on
file ./pkgdb
b db_add_pkg
r

#set args testpkg-1.0-i486-1.tgz
#file ./fastpkg
