/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include "pkgname.h"

#include "pyspkg-priv.h"
#include "pyspkg-doc.h"

PyObject* PySpkgErrorObject;

static PyMethodDef PySpkg_methods[] = {
#include "methtab.c"
  {NULL, NULL}
};

PyDoc_STRVAR(module_doc, "Python bindings for spkg.");

#define InitType(t) \
  if (PyType_Ready(&t##_Type) < 0) return;

PyMODINIT_FUNC initspkg(void)
{
  InitType(Package)
  InitType(Packages)
  InitType(PackagesIter)
  InitType(File)
  InitType(Files)
  InitType(FilesIter)
  InitType(Untgz)

  PyObject *m = Py_InitModule3("spkg", PySpkg_methods, module_doc);
  if (m == 0)
    return;
  if (PySpkgErrorObject == NULL)
  {
    PySpkgErrorObject = PyErr_NewException("spkg.error", NULL, NULL);
    if (PySpkgErrorObject == NULL)
      return;
  }
  Py_INCREF(PySpkgErrorObject);
  PyModule_AddObject(m, "error", PySpkgErrorObject);
}
