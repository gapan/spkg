#!/bin/sh
cat m-*.c \
  | sed -n 's/^PySpkg_Method(\([^, ]\+\).*/M(\1)/ p' \
  > methtab.c

cat m-*.c \
  | sed -n 's/^PySpkg_Method(\([^, ]\+\).*/extern PyObject* PySpkg_\1(PyObject* self, PyObject* args);/ p' \
  > pyspkg-meth.h

cat m-*.c \
  | sed -n 's/^PySpkg_Method(\([^, ]\+\).*/extern char PySpkg_DOC_\1[];/ p' \
  > pyspkg-doc.h
