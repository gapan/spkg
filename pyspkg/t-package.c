/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include <structmember.h>

/* Package_Type
 ************************************************************************/

Package* newPackage(struct db_pkg* pkg, Packages* pkgs)
{
  if (pkg == NULL)
    return NULL;
  Package *self = PyObject_NEW(Package, &Package_Type);
  if (self == NULL)
    return NULL;
  self->pkg = pkg;
  Py_XINCREF(self->pkgs = pkgs);
  return self;
}

static void Package_dealloc(Package* self)
{
  if (!self->pkgs)
    db_free_pkg(self->pkg);
  Py_XDECREF(self->pkgs);
  PyMem_DEL(self);
}

static int Package_print(Package *self, FILE *fp, int flags)
{
  fprintf(fp, 
    "package object (id=%d)\n"
    "name      = '%s'\n"
    "shortname = '%s'\n"
    "version   = '%s'\n"
    "arch      = '%s'\n"
    "build     = '%s'\n"
    "csize     = %u\n"
    "usize     = %u\n"
    "location  = '%s'",
    self->pkg->id,
    self->pkg->name,
    self->pkg->shortname,
    self->pkg->version,
    self->pkg->arch,
    self->pkg->build,
    self->pkg->csize,
    self->pkg->usize,
    self->pkg->location
  );
  return 0;
}

static int Package_set(Package *self, PyObject *value, void *closure)
{
  PyErr_SetString(PyExc_TypeError, "can't modify package object");
  return -1;
}

#define GS_STR(n,id) case id: return self->pkg->n?PyString_FromString(self->pkg->n):PyString_FromString("");
#define GS_INT(n,id) case id: return PyInt_FromLong(self->pkg->n);
#define GS(n,id) {G_STRINGIFY(n), (getter)Package_get, (setter)Package_set, NULL, (void*)id},
static PyObject* Package_get(Package *self, void *closure)
{
  switch((int)closure)
  {
    GS_STR(name,1)
    GS_STR(shortname,2)
    GS_STR(version,3)
    GS_STR(arch,4)
    GS_STR(build,5)
    GS_INT(csize,6)
    GS_INT(usize,7)
    GS_STR(location,8)
    GS_STR(doinst,9)
    GS_INT(id,10)
    case 11:
      return (PyObject*)newFiles(self->pkg->files, self->pkgs?(PyObject*)self->pkgs:(PyObject*)self);
    default:
      Py_INCREF(Py_None);
      return (PyObject*)Py_None;
  }
}

static PyGetSetDef Package_getseters[] = {
  GS(name,1)
  GS(shortname,2)
  GS(version,3)
  GS(arch,4)
  GS(build,5)
  GS(csize,6)
  GS(usize,7)
  GS(location,8)
  GS(doinst,9)
  GS(id,10)
  GS(files,11)
  {NULL}
};

static PyMethodDef Package_methods[] = {
  {NULL, NULL}
};

PyTypeObject Package_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Package),
  .tp_name = "spkg.Package",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Package_dealloc,
  .tp_getset = Package_getseters,
  .tp_methods = Package_methods,
  .tp_print = (printfunc)Package_print,
};

/* Packages_Type
 ************************************************************************/

Packages* newPackages(GSList* pkgs)
{
  Packages *self;
  self = PyObject_NEW(Packages, &Packages_Type);
  if (self == NULL)
    return NULL;
  self->pkgs = pkgs;
  return self;
}

static void Packages_dealloc(Packages* self)
{
  if (self->pkgs)
    db_free_packages(self->pkgs);
  PyMem_DEL(self);
}

PyTypeObject Packages_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Packages),
  .tp_name = "spkg.Packages",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Packages_dealloc,
  .tp_iter = (getiterfunc)newPackagesIter,
};

/* PackagesIter_Type
 ************************************************************************/

PackagesIter* newPackagesIter(Packages* pkgs)
{
  PackagesIter *self;
  if (pkgs == NULL)
    return NULL;
  self = PyObject_NEW(PackagesIter, &PackagesIter_Type);
  if (self == NULL)
    return NULL;
  self->cur = pkgs->pkgs;
  Py_INCREF(self->pkgs = pkgs);
  return self;
}

static void PackagesIter_dealloc(PackagesIter* self)
{
  Py_DECREF(self->pkgs);
  PyMem_DEL(self);
}

static Package* PackagesIter_next(PackagesIter *it)
{
  if (it->cur)
  {
    Package* p = newPackage(it->cur->data,it->pkgs);
    it->cur = it->cur->next;
    return p;
  }
  PyErr_SetNone(PyExc_StopIteration);
  return NULL;
}

PyTypeObject PackagesIter_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(PackagesIter),
  .tp_name = "spkg.PackagesIter",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)PackagesIter_dealloc,
  .tp_iternext = (iternextfunc)PackagesIter_next,
};
