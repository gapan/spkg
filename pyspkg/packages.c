typedef struct {
  PyObject_HEAD
  GSList* pkgs;
  GSList* cur;
  long size;
} Packages;

typedef struct {
  PyObject_HEAD
  GSList* cur;
} PackagesIter;

static PyTypeObject Packages_Type;
static PyTypeObject PackagesIter_Type;

#define Packages_Check(v)	((v)->ob_type == &Packages_Type)

/* ------------------------------------------------------------------------ */

static Packages* newPackages(GSList* pkgs)
{
  Packages *self;
  self = PyObject_NEW(Packages, &Packages_Type);
  if (self == NULL)
    return NULL;
  self->pkgs = pkgs;
  self->cur = pkgs;
  self->size = pkgs?g_slist_length(pkgs):0;
  return self;
}

static void Packages_dealloc(Packages* self)
{
  if (self->pkgs)
    db_free_packages(self->pkgs);
  PyMem_DEL(self);
}

/* ------------------------------------------------------------------------ */

static PyObject* Packages_count(Packages *it)
{
  return PyInt_FromLong(it->size);
}

/* ------------------------------------------------------------------------ */

static PySequenceMethods Packages_as_sequence = {
  .sq_length = (inquiry)Packages_count
};

static PackagesIter* newPackagesIter(Packages* p);

static PyTypeObject Packages_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Packages),
  .tp_name = "spkg.Packages",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Packages_dealloc,
  .tp_as_sequence = &Packages_as_sequence,
  .tp_iter = newPackagesIter,
};

/* ------------------------------------------------------------------------ */

static PackagesIter* newPackagesIter(Packages* p)
{
  PackagesIter *self;
  self = PyObject_NEW(PackagesIter, &PackagesIter_Type);
  if (self == NULL)
    return NULL;
  self->cur = p->pkgs;
  return self;
}

static Packages* PackagesIter_next(PackagesIter *it)
{
  if (it->cur)
  {
    Package* p = newPackage(it->cur->data,0);
    it->cur = it->cur->next;
    return p;
  }
  PyErr_SetNone(PyExc_StopIteration);
  return NULL;
}

static PyTypeObject PackagesIter_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(PackagesIter),
  .tp_name = "spkg.PackagesIter",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)PyObject_Del,
  .tp_iternext = (iternextfunc)PackagesIter_next,
};

/* ------------------------------------------------------------------------ */
