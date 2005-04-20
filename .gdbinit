set print pretty on
set logging file gdb.log
set logging overwrite on
set logging on
#set args /jhkjh/g/a-1-i-1.tgz
#file ./test
file ./pkgdb
b strcmp_shortname
r
