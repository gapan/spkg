/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ond�ej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include <structmember.h>

/* File_Type
 ************************************************************************/

File* newFile(struct db_file* file, PyObject* parent)
{
  if (parent == NULL || file == NULL)
    return NULL;
  File *self = PyObject_NEW(File, &File_Type);
  if (self == NULL)
    return NULL;
  self->file = file;
  Py_XINCREF(self->parent = parent);
  return self;
}

static void File_dealloc(File* self)
{
  Py_XDECREF(self->parent);
  PyMem_DEL(self);
}

static int File_print(File *self, FILE *fp, int flags)
{
  fprintf(fp, 
    "file object (id=%d)\n"
    "refs      = %d\n"
    "path      = '%s'\n"
    "link      = '%s'",
    self->file->id,
    self->file->refs,
    self->file->path,
    self->file->link?self->file->link:""
  );
  return 0;
}

static int File_set(File *self, PyObject *value, void *closure)
{
  PyErr_SetString(PyExc_TypeError, "can't modify file object");
  return -1;
}

#define GS_STR(n,id) case id: return self->file->n?PyString_FromString(self->file->n):PyString_FromString("");
#define GS_INT(n,id) case id: return PyInt_FromLong(self->file->n);
#define GS(n,id) {G_STRINGIFY(n), (getter)File_get, (setter)File_set, NULL, (void*)id},
static PyObject* File_get(File *self, void *closure)
{
  switch((int)closure)
  {
    GS_STR(path,1)
    GS_STR(link,2)
    GS_INT(id,3)
    GS_INT(refs,4)
    default:
      Py_INCREF(Py_None);
      return Py_None;
  }
}

static PyGetSetDef File_getseters[] = {
  GS(path,1)
  GS(link,2)
  GS(id,3)
  GS(refs,4)
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

/* Files_Type
 ************************************************************************/

Files* newFiles(GSList* files, PyObject* parent)
{
  Files *self;
  if (parent == NULL || files == NULL)
    return NULL;
  self = PyObject_NEW(Files, &Files_Type);
  if (self == NULL)
    return NULL;
  self->files = files;
  Py_XINCREF(self->parent = parent);
  return self;
}

static void Files_dealloc(Files* self)
{
  Py_XDECREF(self->parent);
  PyMem_DEL(self);
}

PyTypeObject Files_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Files),
  .tp_name = "spkg.Files",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Files_dealloc,
  .tp_iter = (getiterfunc)newFilesIter,
};

/* FilesIter_Type
 ************************************************************************/

FilesIter* newFilesIter(Files* files)
{
  FilesIter *self;
  if (files == NULL)
    return NULL;
  self = PyObject_NEW(FilesIter, &FilesIter_Type);
  if (self == NULL)
    return NULL;
  self->cur = files->files;
  Py_INCREF(self->files = files);
  return self;
}

static void FilesIter_dealloc(FilesIter* self)
{
  Py_DECREF(self->files);
  PyMem_DEL(self);
}

static File* FilesIter_next(FilesIter *it)
{
  if (it->cur)
  {
    File* p = newFile(it->cur->data, it->files->parent);
    it->cur = it->cur->next;
    return p;
  }
  PyErr_SetNone(PyExc_StopIteration);
  return NULL;
}

PyTypeObject FilesIter_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(FilesIter),
  .tp_name = "spkg.FilesIter",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)FilesIter_dealloc,
  .tp_iternext = (iternextfunc)FilesIter_next,
};