#include <Python.h>
#include <structmember.h>

#include "pkgdb.h"
#include "pkgname.h"

static PyObject* ErrorObject;

#include "file.c"
#include "files.c"

#include "package.c"
#include "packages.c"

static PyObject* spkg_db_open(PyObject* self, PyObject* args)
{
  const char* root;
  if (PyArg_ParseTuple(args, "s", &root))
  {
    if (db_open(root))
    {
      PyErr_SetString(ErrorObject, db_error()?db_error():"db err");
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
      PyErr_SetString(ErrorObject, db_error()?db_error():"db err");
      return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (PyErr_Occurred())
    PyErr_Clear();
  PyErr_SetString(ErrorObject, "invalid arguments");
  return NULL;
}

static PyObject* spkg_db_close(PyObject* self, PyObject* args)
{
  if (PyArg_ParseTuple(args, ""))
  {
    db_close();
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyErr_SetString(ErrorObject, "invalid arguments");
  return NULL;
}

static PyObject* spkg_parse_pkgname(PyObject* self, PyObject* args)
{
  const char* name;
  int part;
  if (!PyArg_ParseTuple(args, "si", &name, &part))
    return NULL;
  if (parse_pkgname(name,6) != (char*)-1)
  {
    PyErr_SetString(ErrorObject, "invalid package name");
    return NULL;
  }
  if (part < 0 || part > 6)
  {
    PyErr_SetString(ErrorObject, "invalid package part");
    return NULL;
  }
  char* s = parse_pkgname(name,part);
  return PyString_FromString(s);
}

static PyObject* spkg_db_get_pkg(PyObject* self, PyObject* args)
{
  char* name;
  int files;
  if (!PyArg_ParseTuple(args, "si", &name, &files))
  {
    PyErr_SetString(ErrorObject, "invalid arguments");
    return NULL;
  }
  struct db_pkg* p = db_get_pkg(name,files);
  if (p == 0)
  {
    PyErr_SetString(ErrorObject, db_error()?db_error():"db err");
    return NULL;
  }
  return (PyObject*)newPackage(p,1);
}

static PyObject* spkg_db_legacy_get_pkg(PyObject* self, PyObject* args)
{
  char* name;
  int files;
  if (!PyArg_ParseTuple(args, "si", &name, &files))
  {
    PyErr_SetString(ErrorObject, "invalid arguments");
    return NULL;
  }
  struct db_pkg* p = db_legacy_get_pkg(name,files);
  if (p == 0)
  {
    PyErr_SetString(ErrorObject, db_error()?db_error():"db err");
    return NULL;
  }
  return (PyObject*)newPackage(p,1);
}

static PyObject* spkg_db_legacy_get_packages(PyObject* self, PyObject* args)
{
  char* name;
  if (!PyArg_ParseTuple(args, "", &name))
    return NULL;
  GSList* l = db_legacy_get_packages();
  if (db_error())
  {
    PyErr_SetString(ErrorObject, db_error());
    return NULL;
  }
  return (PyObject*)newPackages(l);
}

static PyObject* spkg_db_get_packages(PyObject* self, PyObject* args)
{
  char* name;
  if (!PyArg_ParseTuple(args, "", &name))
    return NULL;
  GSList* l = db_get_packages();
  if (db_error())
  {
    PyErr_SetString(ErrorObject, db_error());
    return NULL;
  }
  return (PyObject*)newPackages(l);
}

static PyMethodDef spkg_methods[] = {
  { "db_open", spkg_db_open, METH_VARARGS, PyDoc_STR("db_open() -> None") },
  { "db_close", spkg_db_close, METH_VARARGS, PyDoc_STR("db_close() -> None") },

  { "db_get_pkg", spkg_db_get_pkg, METH_VARARGS, PyDoc_STR("db_get_pkg() -> None") },

  { "db_legacy_get_pkg", spkg_db_legacy_get_pkg, METH_VARARGS, PyDoc_STR("db_get_pkg() -> None") },
  { "db_legacy_get_packages", spkg_db_legacy_get_packages, METH_VARARGS, PyDoc_STR("db_get_pkg() -> None") },
  { "db_get_packages", spkg_db_get_packages, METH_VARARGS, PyDoc_STR("db_get_pkg() -> None") },

  { "parse_pkgname", spkg_parse_pkgname, METH_VARARGS, PyDoc_STR("parse_pkgname(s,i) -> s") },
  {NULL, NULL}
};

PyDoc_STRVAR(module_doc, "Python bindings for spkg.");

PyMODINIT_FUNC initspkg(void)
{
  if (PyType_Ready(&Package_Type) < 0)
    return;
  if (PyType_Ready(&Packages_Type) < 0)
    return;
  if (PyType_Ready(&PackagesIter_Type) < 0)
    return;

  PyObject *m = Py_InitModule3("spkg", spkg_methods, module_doc);
  if (m == 0)
    return;
  if (ErrorObject == NULL)
  {
    ErrorObject = PyErr_NewException("spkg.error", NULL, NULL);
    if (ErrorObject == NULL)
      return;
  }
  Py_INCREF(ErrorObject);
  PyModule_AddObject(m, "error", ErrorObject);
}
