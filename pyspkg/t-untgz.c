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

/* callback wrapper */

struct cb_t {
  PyObject* o;
  Untgz* t;
  struct untgz_state* s;
};

static GSList* cb_list = 0;

static void cb_add(PyObject* o, struct untgz_state* s, Untgz* t)
{
  struct cb_t* cb = g_new0(struct cb_t, 1);
  cb->o = o;
  cb->s = s;
  cb->t = t;
  cb_list = g_slist_prepend(cb_list, cb);
}

static void cb_rem(struct untgz_state* s)
{
  GSList* l;
  for (l=cb_list; l!=0; l=l->next)
  {
    struct cb_t* cb = l->data;
    if (s==cb->s)
    {
      g_free(cb);
      cb_list = g_slist_remove_link(cb_list, l);
      return;
    }
  }
}

static void cb_call(struct untgz_state* s, gsize total, gsize current)
{
  GSList* l;
  for (l=cb_list; l!=0; l=l->next)
  {
    struct cb_t* cb = l->data;
    if (s==cb->s)
    {
      PyObject* args = Py_BuildValue("(Oii)", cb->t, total, current);
      PyEval_CallObject(cb->o, args);
      Py_DECREF(args);
      return;
    }
  }
}

/* Untgz_Type
 ************************************************************************/

static int Untgz_init(Untgz *self, PyObject *args, PyObject *kwds)
{
  PyObject *cb=NULL;
  char* path=0;
  static char *kwlist[] = {"path", "callback", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O!:Untgz", kwlist, &path, &PyFunction_Type, &cb))
    return -1; 

  struct untgz_state* s;
  if (cb)
    s = untgz_open(path,cb_call);
  else
    s = untgz_open(path,0);
  if (s == 0)
  {
    PyErr_SetString(PySpkgErrorObject, "can't open tgz file");
    return -1;
  }

  Py_XINCREF(self->callback = cb);
  self->s = s;
  if (cb)
    cb_add(cb, s, self);
  return 0;
}

static void Untgz_dealloc(Untgz* self)
{
  if (self->s)
  {
    cb_rem(self->s);
    untgz_close(self->s);
  }
  Py_XDECREF(self->callback);
  PyMem_DEL(self);
}

static int Untgz_set(Untgz *self, PyObject *value, void *closure)
{
  PyErr_SetString(PyExc_TypeError, "can't modify untgz object");
  return -1;
}

#define GS_STR(n,id) case id: return self->s->n?PyString_FromString(self->s->n):PyString_FromString("");
#define GS_INT(n,id) case id: return PyInt_FromLong(self->s->n);
#define GS(n,id) {G_STRINGIFY(n), (getter)Untgz_get, (setter)Untgz_set, NULL, (void*)id},
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
"Get next file's header from archive. Returns true if no more headers.")
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
  char* altname = 0;
  if (!PyArg_ParseTuple(args, "|s:write_file", &altname))
    return NULL;
  if (untgz_write_file(self->s,altname))
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
"Get buffer with file content.")
{
  gchar* b;
  gchar* bb;
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
  PyObject* buf = PyBuffer_New(s);
  if (bb)
  {
    PyBuffer_Type.tp_as_buffer->bf_getwritebuffer(buf, 0, (void**)&bb);
    memcpy(bb, b, s);
  }
  g_free(b);
  return (PyObject*)buf;
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
  .tp_new = PyType_GenericNew,
  .tp_init = (initproc)Untgz_init,
};
