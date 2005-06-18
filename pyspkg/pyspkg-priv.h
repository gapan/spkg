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

#endif
