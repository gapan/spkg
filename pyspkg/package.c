typedef struct {
  PyObject_HEAD
  struct db_pkg* p;
  int free;
} Package;

static PyTypeObject Package_Type;
#define Package_Check(v)	((v)->ob_type == &Package_Type)

/* ------------------------------------------------------------------------ */

static Package* newPackage(struct db_pkg* p, int free)
{
  if (p == NULL)
    return NULL;
  Package *self = PyObject_NEW(Package, &Package_Type);
  if (self == NULL)
    return NULL;
  self->p = p;
  self->free = free;
  return self;
}

static void Package_dealloc(Package* self)
{
  if (self->free)
    db_free_pkg(self->p);
  PyMem_DEL(self);
}

/* ------------------------------------------------------------------------ */

static PyObject* Package_get(Package *self, void *closure)
{
  switch((int)closure)
  {
    case 1: return PyString_FromString(self->p->name);
    case 2: return PyString_FromString(self->p->shortname);
    case 3: return PyString_FromString(self->p->version);
    case 4: return PyString_FromString(self->p->arch);
    case 5: return PyString_FromString(self->p->build);
    case 6: return PyInt_FromLong(self->p->csize);
    case 7: return PyInt_FromLong(self->p->usize);
    case 8: return PyString_FromString(self->p->location);
    case 9: return PyString_FromString(self->p->doinst);
    case 10: return PyInt_FromLong(self->p->id);
    case 11: return newFiles(self->p->files);
    default:
      Py_INCREF(Py_None);
      return Py_None;
  }
}

static int Package_set(Package *self, PyObject *value, void *closure)
{
  PyErr_SetString(PyExc_TypeError, "can't modify package object");
  return -1;
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
    self->p->id,
    self->p->name,
    self->p->shortname,
    self->p->version,
    self->p->arch,
    self->p->build,
    self->p->csize,
    self->p->usize,
    self->p->location
  );
  return 0;
}

/* ------------------------------------------------------------------------ */

static PyGetSetDef Package_getseters[] = {
  {"name",       (getter)Package_get, (setter)Package_set, NULL, (void*)1},
  {"shortname",  (getter)Package_get, (setter)Package_set, NULL, (void*)2},
  {"version",    (getter)Package_get, (setter)Package_set, NULL, (void*)3},
  {"arch",       (getter)Package_get, (setter)Package_set, NULL, (void*)4},
  {"build",      (getter)Package_get, (setter)Package_set, NULL, (void*)5},
  {"csize",      (getter)Package_get, (setter)Package_set, NULL, (void*)6},
  {"usize",      (getter)Package_get, (setter)Package_set, NULL, (void*)7},
  {"location",   (getter)Package_get, (setter)Package_set, NULL, (void*)8},
  {"doinst",     (getter)Package_get, (setter)Package_set, NULL, (void*)9},
  {"id",         (getter)Package_get, (setter)Package_set, NULL, (void*)10},
  {"files",      (getter)Package_get, (setter)Package_set, NULL, (void*)11},
  {NULL}
};

static PyMethodDef Package_methods[] = {
//  { "", (PyCFunction) Package_demo, METH_VARARGS, PyDoc_STR("demo() -> None") },
  {NULL, NULL}
};

static PyTypeObject Package_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Package),
  .tp_name = "spkg.Package",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Package_dealloc,
  .tp_getset = Package_getseters,
  .tp_methods = Package_methods,
  .tp_print = (printfunc)Package_print,
};
