/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include <structmember.h>

/* File_Type
 ************************************************************************/

File* newFile(struct db_file* p)
{
  if (p == NULL)
    return NULL;
  File *self = PyObject_NEW(File, &File_Type);
  if (self == NULL)
    return NULL;
  self->p = p;
  return self;
}

static void File_dealloc(File* self)
{
  PyMem_DEL(self);
}

static PyObject* File_get(File *self, void *closure)
{
  switch((int)closure)
  {
    case 1: return PyString_FromString(self->p->path);
    case 2: return PyString_FromString(self->p->link);
    case 3: return PyInt_FromLong(self->p->refs);
    case 4: return PyInt_FromLong(self->p->id);
    default:
      Py_INCREF(Py_None);
      return Py_None;
  }
}

static int File_set(File *self, PyObject *value, void *closure)
{
  PyErr_SetString(PyExc_TypeError, "can't modify file object");
  return -1;
}

static int File_print(File *self, FILE *fp, int flags)
{
  fprintf(fp, 
    "file object (id=%d)\n"
    "refs      = %d\n"
    "path      = '%s'\n"
    "link      = '%s'",
    self->p->id,
    self->p->refs,
    self->p->path,
    self->p->link?self->p->link:""
  );
  return 0;
}

static PyGetSetDef File_getseters[] = {
  {"path",       (getter)File_get, (setter)File_set, NULL, (void*)1},
  {"link",       (getter)File_get, (setter)File_set, NULL, (void*)2},
  {"refs",       (getter)File_get, (setter)File_set, NULL, (void*)2},
  {"id",         (getter)File_get, (setter)File_set, NULL, (void*)3},
  {NULL}
};

static PyMethodDef File_methods[] = {
  {NULL, NULL}
};

PyTypeObject File_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(File),
  .tp_name = "spkg.File",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)File_dealloc,
  .tp_getset = File_getseters,
  .tp_methods = File_methods,
  .tp_print = (printfunc)File_print,
};
