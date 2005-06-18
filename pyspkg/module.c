/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include "pkgname.h"

PyObject* PySpkgErrorObject;

#define PySpkg_Method(n,a,r,d) \
  PyDoc_STRVAR(PySpkg_DOC_##n, G_STRINGIFY(PySpkg_DOC_##n) "(" a ")" " -> " G_STRINGIFY(r) "\n\n" d); \
  static PyObject* PySpkg_##n(PyObject* self, PyObject* args)

PySpkg_Method(db_open, "[root]", Null, "Open database.")
{
  const char* root;
  if (PyArg_ParseTuple(args, "s", &root))
  {
    if (db_open(root))
    {
      PyErr_SetString(PySpkgErrorObject, db_error()?db_error():"db err");
      return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (PyErr_Occurred())
    PyErr_Clear();
  if (PyArg_ParseTuple(args, ""))
  {
    if (db_open(0))
    {
      PyErr_SetString(PySpkgErrorObject, db_error()?db_error():"db err");
      return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (PyErr_Occurred())
    PyErr_Clear();
  PyErr_SetString(PySpkgErrorObject, "invalid arguments");
  return NULL;
}

PySpkg_Method(db_close, "", Null, "Close database.")
{
  if (PyArg_ParseTuple(args, ""))
  {
    db_close();
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyErr_SetString(PySpkgErrorObject, "invalid arguments");
  return NULL;
}

PySpkg_Method(parse_pkgname, "name, part", String, 
"Parse part from package name.")
{
  const char* name;
  int part;
  if (!PyArg_ParseTuple(args, "si", &name, &part))
    return NULL;
  if (parse_pkgname(name,6) != (char*)-1)
  {
    PyErr_SetString(PySpkgErrorObject, "invalid package name");
    return NULL;
  }
  if (part < 0 || part > 6)
  {
    PyErr_SetString(PySpkgErrorObject, "invalid package part");
    return NULL;
  }
  char* s = parse_pkgname(name,part);
  return PyString_FromString(s);
}

PySpkg_Method(db_get_pkg, "name, files", Package, 
"Get package from database. You may specify if "
"you want to extract package's filelist")
{
  char* name;
  int files;
  if (!PyArg_ParseTuple(args, "si", &name, &files))
  {
    PyErr_SetString(PySpkgErrorObject, "invalid arguments");
    return NULL;
  }
  struct db_pkg* p = db_get_pkg(name,files);
  if (p == 0)
  {
    PyErr_SetString(PySpkgErrorObject, db_error()?db_error():"db err");
    return NULL;
  }
  return (PyObject*)newPackage(p,1);
}

PySpkg_Method(db_legacy_get_pkg, "name, files", Package, 
"Get package from legacy database. You may specify if "
"you want to extract package's filelist")
{
  char* name;
  int files;
  if (!PyArg_ParseTuple(args, "si", &name, &files))
  {
    PyErr_SetString(PySpkgErrorObject, "invalid arguments");
    return NULL;
  }
  struct db_pkg* p = db_legacy_get_pkg(name,files);
  if (p == 0)
  {
    PyErr_SetString(PySpkgErrorObject, db_error()?db_error():"db err");
    return NULL;
  }
  return (PyObject*)newPackage(p,1);
}

PySpkg_Method(db_legacy_get_packages, "", Packages,
"Get all packages from legacy database")
{
  char* name;
  if (!PyArg_ParseTuple(args, "", &name))
    return NULL;
  GSList* l = db_legacy_get_packages();
  if (db_error())
  {
    PyErr_SetString(PySpkgErrorObject, db_error());
    return NULL;
  }
  return (PyObject*)newPackages(l);
}

PySpkg_Method(db_get_packages, "", Packages,
"Get all packages from database")
{
  char* name;
  if (!PyArg_ParseTuple(args, "", &name))
    return NULL;
  GSList* l = db_get_packages();
  if (db_error())
  {
    PyErr_SetString(PySpkgErrorObject, db_error());
    return NULL;
  }
  return (PyObject*)newPackages(l);
}

#define M(n) { G_STRINGIFY(n), PySpkg_##n, METH_VARARGS, PySpkg_DOC_##n },
static PyMethodDef PySpkg_methods[] = {
  M(db_open)
  M(db_close)
//  M(db_add_pkg)
  M(db_get_pkg)
//  M(db_rem_pkg)
//  M(db_legacy_add_pkg)
  M(db_legacy_get_pkg)
//  M(db_legacy_rem_pkg)
  M(db_get_packages)
  M(db_legacy_get_packages)
  M(parse_pkgname)
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
