/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include "pyspkg-priv.h"

#include "pkgname.h"

PySpkg_Method(untgz_open, "path", UntgzState, "Open tgzfile.")
{
  const char* path;
  if (!PyArg_ParseTuple(args, "s", &path))
    return NULL;
  struct untgz_state* tgz = untgz_open(path,0);
  if (tgz == 0)
  {
    PyErr_SetString(PySpkgErrorObject, "can't open tgz file");
    return NULL;
  }
  return (PyObject*)newUntgz(tgz);
}
