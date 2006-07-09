#set logging file .gdb.log
#set logging overwrite on
#set logging on
set print pretty on
set args -n -r .root -d bin
file ./spkg
#b filedb.c:193
#b db_open
r
