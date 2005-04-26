#!/bin/sh

rm -rf /var/log/fastpkg
./fastpkg f
gnuplot plots.gp4
mv hash.png /home/megi

exit
# create "empty" chroot environment
cp fastpkg testbox/bin
cd testbox || exit 1
env - /usr/sbin/chroot . sh -c 'fastpkg test'

exit
VG="valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --log-file=valgrind.log"
$VG ./fastpkg
#gdb