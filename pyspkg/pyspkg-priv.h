/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#ifndef __PYSPKG_PRIV_H
#define __PYSPKG_PRIV_H

#define PySpkg_Method(n,a,r,d) \
  char PySpkg_DOC_##n[] = G_STRINGIFY(PySpkg_DOC_##n) "(" a ")" " -> " G_STRINGIFY(r) "\n\n" d; \
  PyObject* PySpkg_##n(PyObject* self, PyObject* args)

#define PySpkg_M(n) { G_STRINGIFY(n), PySpkg_##n, METH_VARARGS, PySpkg_DOC_##n },

#define _PySpkg_TypeMethod(t,n,a,r,d) \
  static char PySpkg_##t##_DOC_##n[] = G_STRINGIFY(PySpkg_DOC_##n) "(" a ")" " -> " G_STRINGIFY(r) "\n\n" d; \
  static PyObject* PySpkg_##t##_##n(t* self, PyObject* args)

#define _PySpkg_TM(t,n) { G_STRINGIFY(n), (PyCFunction)PySpkg_##t##_##n, METH_VARARGS, PySpkg_##t##_DOC_##n },

#endif
