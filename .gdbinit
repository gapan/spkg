#set logging file .gdb.log
#set logging overwrite on
#set logging on
set print pretty on
#set args -n -vvv -r .root -i aaa_base-10.2.0-noarch-4.tgz
set args -vvv -r .root -i aaa_elflibs-10.2.0-i486-4.tgz
file ./spkg
r


