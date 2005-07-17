/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "sys.h"
#include "taction.h"

/* private
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(_ta.err, "taction", __func__, n, fmt, ##args)
#define _e_set(e, n, fmt, args...) e_add(e, "taction", __func__, n, fmt, ##args)

struct transaction {
  gboolean active;
  const gchar* root;
  struct error* err;
  GSList* list;
};

static struct transaction _ta = {
  .active = 0,
  .root = 0,
  .err = 0,
  .list = 0
};

/* action list handling */
typedef enum { MOVE, KEEP } t_on_finalize;
typedef enum { REMOVE } t_on_rollback;

struct action {
  gchar* path1;
  gchar* path2;
  gboolean is_dir;
  t_on_finalize on_finalize;
  t_on_rollback on_rollback;
};

static gint _ta_insert(
  gchar* path1,
  gchar* path2,
  gboolean is_dir,
  t_on_finalize on_finalize,
  t_on_rollback on_rollback
)
{
  struct action* a = g_new(struct action, 1);
  a->path1 = path1;
  a->path2 = path2;
  a->is_dir = is_dir;
  a->on_finalize = on_finalize;
  a->on_rollback = on_rollback;
  _ta.list = g_slist_prepend(_ta.list, a);
  return 1;
}

static void _ta_free_action(struct action* a)
{
  g_free(a->path1);
  g_free(a->path2);
  g_free(a);
}

/* public 
 ************************************************************************/

gint ta_initialize(const gchar* root, struct error* e)
{
  g_assert(e != 0);

  if (_ta.active)
  {
    _e_set(e, E_ERROR|TA_ACTIVE, "another transaction is in progress");
    return 1;
  }
  _ta.err = e;
  _ta.root = root;
  if (root == 0)
    _ta.root = "";
  _ta.active = 1;
  _ta.list = 0;
  return 0;
}

gint ta_keep_remove(gchar* path, gboolean is_dir)
{
  return _ta_insert(path,0,is_dir,KEEP,REMOVE);
}

gint ta_move_remove(gchar* path, gchar* fin_path)
{
  return _ta_insert(path,fin_path,0,MOVE,REMOVE);
}

gint ta_finalize()
{
  sigset_t oldsig;
  GSList* l;

  sys_sigblock(&oldsig);

  if (!_ta.active)
  {
    e_set(E_ERROR|TA_NACTIVE, "transaction is not initialized");
    goto err;
  }

  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_finalize == KEEP)
      continue;
    if (a->on_finalize == MOVE)
    {
      /*XXX: check for errors */
      rename(a->path1, a->path2);
    }
    _ta_free_action(a);
  }
  g_slist_free(_ta.list);
  _ta.list = 0;
  _ta.active = 0;

  sys_sigunblock(&oldsig);
  return 0;
 err:
  sys_sigunblock(&oldsig);
  return 1;
}

gint ta_rollback()
{
  sigset_t oldsig;
  GSList* l;

  sys_sigblock(&oldsig);
  if (!_ta.active)
  {
    e_set(E_ERROR|TA_NACTIVE, "transaction is not initialized");
    goto err;
  }

  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_rollback == REMOVE)
    {
      if (a->is_dir)
        rmdir(a->path1);
      else
        unlink(a->path1);
    }
    _ta_free_action(a);
  }
  g_slist_free(_ta.list);
  _ta.list = 0;
  _ta.active = 0;

  sys_sigunblock(&oldsig);
  return 0;
 err:
  sys_sigunblock(&oldsig);
  return 1;
}
