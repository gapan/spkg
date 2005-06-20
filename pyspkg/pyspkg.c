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

#define AddType(t) \
  Py_INCREF(&t##_Type); \
  PyModule_AddObject(m, G_STRINGIFY(t), (PyObject*)&t##_Type);

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

  PyModule_AddIntConstant(m, "UNTGZ_NONE", 0);
  PyModule_AddIntConstant(m, "UNTGZ_DIR", 1);
  PyModule_AddIntConstant(m, "UNTGZ_REG", 2);
  PyModule_AddIntConstant(m, "UNTGZ_LNK", 3);
  PyModule_AddIntConstant(m, "UNTGZ_SYM", 4);
  PyModule_AddIntConstant(m, "UNTGZ_BLK", 5);
  PyModule_AddIntConstant(m, "UNTGZ_CHR", 6);
  PyModule_AddIntConstant(m, "UNTGZ_FIFO", 7);

  Py_INCREF(PySpkgErrorObject);
  PyModule_AddObject(m, "error", PySpkgErrorObject);

  AddType(Package)
  AddType(Packages)
  AddType(File)
  AddType(Files)
  AddType(Untgz)
}
