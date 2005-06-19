/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include "pyspkg.h"
#include <structmember.h>
#include "pyspkg-priv.h"

#define PySpkg_TypeMethod(n,a,r,d) _PySpkg_TypeMethod(Untgz,n,a,r,d)
#define PySpkg_TM(n) _PySpkg_TM(Untgz,n)

/* Untgz_Type
 ************************************************************************/

Untgz* newUntgz(struct untgz_state* s)
{
  if (s == NULL)
    return NULL;
  Untgz *self = PyObject_NEW(Untgz, &Untgz_Type);
  if (self == NULL)
    return NULL;
  self->s = s;
  return self;
}

static void Untgz_dealloc(Untgz* self)
{
  untgz_close(self->s);
  PyMem_DEL(self);
}

#define GS_STR(n,id) case id: return PyString_FromString(self->s->n);
#define GS_INT(n,id) case id: return PyInt_FromLong(self->s->n);
static PyObject* Untgz_get(Untgz *self, void *closure)
{
  switch((int)closure)
  {
    GS_STR(tgzfile,1)
    GS_INT(usize,2)
    GS_INT(csize,3)
    GS_INT(f_type,4)
    GS_STR(f_name,5)
    GS_STR(f_link,6)
    GS_INT(f_size,7)
    GS_INT(f_mode,8)
    GS_INT(f_mtime,9)
    GS_INT(f_uid,10)
    GS_INT(f_gid,11)
    GS_STR(f_uname,12)
    GS_STR(f_gname,13)
    GS_INT(f_devmaj,14)
    GS_INT(f_devmin,15)
    default:
      PyErr_SetString(PyExc_TypeError, "get attr failed");
      return NULL;
  }
}

static int Untgz_set(Untgz *self, PyObject *value, void *closure)
{
  PyErr_SetString(PyExc_TypeError, "can't modify untgz object");
  return -1;
}

#define GS(n,id) {G_STRINGIFY(n), (getter)Untgz_get, (setter)Untgz_set, NULL, (void*)id},
static PyGetSetDef Untgz_getseters[] = {
  GS(tgzfile,1)
  GS(usize,2)
  GS(csize,3)
  GS(f_type,4)
  GS(f_name,5)
  GS(f_link,6)
  GS(f_size,7)
  GS(f_mode,8)
  GS(f_mtime,9)
  GS(f_uid,10)
  GS(f_gid,11)
  GS(f_uname,12)
  GS(f_gname,13)
  GS(f_devmaj,14)
  GS(f_devmin,15)
  {NULL}
};

PySpkg_TypeMethod(get_header, "", Bool, 
"Get next file's header from archive.")
{
  if (untgz_get_header(self->s))
  {
    if (untgz_error(self->s))
    {
      PyErr_SetString(PySpkgErrorObject, untgz_error(self->s));
      return NULL;
    }
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

PySpkg_TypeMethod(write_file, "[path]", Bool, 
"Write current file to disk.")
{
  if (untgz_write_file(self->s,0))
  {
    if (untgz_error(self->s))
    {
      PyErr_SetString(PySpkgErrorObject, untgz_error(self->s));
      return NULL;
    }
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

PySpkg_TypeMethod(write_data, "", Buffer, 
"Write current file to disk.")
{
  gchar* b;
  guint s;
  if (untgz_write_data(self->s, &b, &s))
  {
    if (untgz_error(self->s))
    {
      PyErr_SetString(PySpkgErrorObject, untgz_error(self->s));
      return NULL;
    }
    Py_RETURN_FALSE;
  }
  return PyBuffer_FromMemory(b,s);
}

static PyMethodDef Untgz_methods[] = {
  PySpkg_TM(get_header)
  PySpkg_TM(write_file)
  PySpkg_TM(write_data)
  {NULL, NULL}
};

PyTypeObject Untgz_Type = {
  PyObject_HEAD_INIT(NULL)
  .tp_basicsize = sizeof(Untgz),
  .tp_name = "spkg.Untgz",
  .tp_flags = Py_TPFLAGS_DEFAULT,
  .tp_dealloc = (destructor)Untgz_dealloc,
  .tp_getset = Untgz_getseters,
  .tp_methods = Untgz_methods,
//  .tp_print = (printfunc)Untgz_print,
};
