#set logging file .gdb.log
#set logging overwrite on
#set logging on
set print pretty on
file ./test-path
#b filedb.c:193
#b db_open
r
