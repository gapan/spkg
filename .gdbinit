set print pretty on
set logging file .gdb.log
set logging overwrite on
set logging on
file ./sql
r
#file ./pkgdb
#r

#set args testpkg-1.0-i486-1.tgz
#file ./fastpkg
