#set logging file .gdb.log
#set logging overwrite on
#set logging on
set print pretty on
file ./test-filedb-speed
#b filedb.c:193
r
