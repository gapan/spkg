typedef struct {
  PyObject_HEAD
  GSList* files;
} Files;

typedef struct {
  PyObject_HEAD
  GSList* cur;
  Files* files;
} FilesIter;

static PyTypeObject Files_Type;
static PyTypeObject FilesIter_Type;

#define Files_Check(v)	((v)->ob_type == &Files_Type)

/* ------------------------------------------------------------------------ */

static Files* newFiles(GSList* files)
{
  Files *self;
  self = PyObject_NEW(Files, &Files_Type);
  if (self == NULL)
    return NULL;
  self->files = files;
  return self;
}

static void Files_dealloc(Files* self)
{
  PyMem_DEL(self);
}

static FilesIter* newFilesIter(Files* p);

static PyTypeObject Files_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Files),
  .tp_name = "spkg.Files",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Files_dealloc,
  .tp_iter = newFilesIter,
};

/* ------------------------------------------------------------------------ */

static FilesIter* newFilesIter(Files* p)
{
  FilesIter *self;
  self = PyObject_NEW(FilesIter, &FilesIter_Type);
  if (self == NULL)
    return NULL;
  self->cur = p->files;
  self->files = p;
  Py_INCREF(self->files);
  return self;
}

static void FilesIter_dealloc(FilesIter* self)
{
  Py_DECREF(self->files);
  PyMem_DEL(self);
}

static Files* FilesIter_next(FilesIter *it)
{
  if (it->cur)
  {
    File* p = newFile(it->cur->data);
    it->cur = it->cur->next;
    return p;
  }
  PyErr_SetNone(PyExc_StopIteration);
  return NULL;
}

static PyTypeObject FilesIter_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(FilesIter),
  .tp_name = "spkg.FilesIter",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)FilesIter_dealloc,
  .tp_iternext = (iternextfunc)FilesIter_next,
};

/* ------------------------------------------------------------------------ */
