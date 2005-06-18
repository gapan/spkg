typedef struct {
  PyObject_HEAD
  struct db_pkg* p;
} Package;

static PyTypeObject Package_Type;
#define Package_Check(v)	((v)->ob_type == &Package_Type)

/* ------------------------------------------------------------------------ */

static Package* newPackage(struct db_pkg* p)
{
  Package *self;
  self = PyObject_NEW(Package, &Package_Type);
  if (self == NULL)
    return NULL;
  self->p = p;
  return self;
}

static void Package_dealloc(Package* self)
{
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
    "package object\n"
    "name      = %s\n"
    "shortname = %s\n"
    "version   = %s\n"
    "arch      = %s\n"
    "build     = %s\n",
    self->p->name,
    self->p->shortname,
    self->p->version,
    self->p->arch,
    self->p->build
  );
  return 0;
}

/* ------------------------------------------------------------------------ */

static PyGetSetDef Package_getseters[] = {
  {"name",       (getter)Package_get, (setter)Package_set, "package name", (void*)1},
  {"shortname",  (getter)Package_get, (setter)Package_set, "package name", (void*)2},
  {"version",    (getter)Package_get, (setter)Package_set, "package name", (void*)3},
  {"arch",       (getter)Package_get, (setter)Package_set, "package name", (void*)4},
  {"build",      (getter)Package_get, (setter)Package_set, "package name", (void*)5},
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
