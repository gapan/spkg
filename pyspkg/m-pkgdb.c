/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include "pyspkg-priv.h"

#include "pkgname.h"

PySpkg_Method(db_open, "[root]", Null, "Open database.")
{
  const char* root=0;
  if (PyArg_ParseTuple(args, "|s:db_open", &root))
  {
    if (db_open(root, spkg_error))
    {
      PyErr_SetString(PySpkgErrorObject, e_string(spkg_error));
      return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
  }
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
  return NULL;
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
    PyErr_SetString(PySpkgErrorObject, e_string(spkg_error));
    return NULL;
  }
  return (PyObject*)newPackage(p,0);
}

PySpkg_Method(db_legacy_get_pkg, "name, files", Package, 
"Get package from legacy database. You may specify if "
"you want to extract package's filelist")
{
  char* name;
  int files;
  if (!PyArg_ParseTuple(args, "si", &name, &files))
    return NULL;
  struct db_pkg* p = db_legacy_get_pkg(name,files);
  if (p == 0)
  {
    PyErr_SetString(PySpkgErrorObject, e_string(spkg_error));
    return NULL;
  }
  return (PyObject*)newPackage(p,0);
}

PySpkg_Method(db_legacy_get_packages, "", Packages,
"Get all packages from legacy database")
{
  char* name;
  if (!PyArg_ParseTuple(args, "", &name))
    return NULL;
  GSList* l = db_legacy_get_packages();
  if (!e_ok(spkg_error))
  {
    PyErr_SetString(PySpkgErrorObject, e_string(spkg_error));
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
  if (!e_ok(spkg_error))
  {
    PyErr_SetString(PySpkgErrorObject, e_string(spkg_error));
    return NULL;
  }
  return (PyObject*)newPackages(l);
}

PySpkg_Method(db_add_pkg, "pkg", Null,
"Add package to the database")
{
  PyErr_SetString(PySpkgErrorObject, "not binded");
  return NULL;
}

PySpkg_Method(db_rem_pkg, "name", Null,
"Remove package from the database")
{
  PyErr_SetString(PySpkgErrorObject, "not binded");
  return NULL;
}

PySpkg_Method(db_legacy_add_pkg, "pkg", Null,
"Add package to the legacy database")
{
  PyErr_SetString(PySpkgErrorObject, "not binded");
  return NULL;
}

PySpkg_Method(db_legacy_rem_pkg, "name", Null,
"Remove package from the legacy database")
{
  PyErr_SetString(PySpkgErrorObject, "not binded");
  return NULL;
}

PySpkg_Method(db_sync_to_legacydb, "", Null,
"Synchronize legacy database with spkg database")
{
  PyErr_SetString(PySpkgErrorObject, "not binded");
  return NULL;
}

PySpkg_Method(db_sync_from_legacydb, "", Null,
"Synchronize spkg database with legacy database")
{
  PyErr_SetString(PySpkgErrorObject, "not binded");
  return NULL;
}
