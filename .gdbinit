set print pretty on
set logging file .gdb.log
set logging overwrite on
set logging on
file ./test-pkgdb-legacy
b pkgdb.c:566
r
