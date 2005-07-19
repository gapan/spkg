/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include "pyspkg-priv.h"

#include "pkgname.h"

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
