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

#include "taction.h"
#include "message.h"

/* private
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(_ta.err, "taction", __func__, n, fmt, ##args)
#define _e_set(e, n, fmt, args...) e_add(e, "taction", __func__, n, fmt, ##args)

struct transaction {
  gboolean active;
  gboolean dryrun;
  struct error* err;
  GSList* list;
};

static struct transaction _ta = {
  .active = 0,
  .err = 0,
  .list = 0,
  .dryrun = 0
};

/* action list handling */
typedef enum { MOVE, KEEP, LINK, SYMLINK } t_on_finalize;
typedef enum { REMOVE, NOTHING } t_on_rollback;

struct action {
  gchar* path1;
  gchar* path2;
  gboolean is_dir;
  t_on_finalize on_finalize;
  t_on_rollback on_rollback;
};

static void _ta_insert(
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
}

static void _ta_free_action(struct action* a)
{
  g_free(a->path1);
  g_free(a->path2);
  g_free(a);
}

/* public 
 ************************************************************************/

gint ta_initialize(gboolean dryrun, struct error* e)
{
  g_assert(e != 0);

  if (_ta.active)
  {
    _e_set(e, E_ERROR|TA_ACTIVE, "another transaction is in progress");
    return 1;
  }
  _ta.err = e;
  _ta.active = 1;
  _ta.dryrun = dryrun;
  _ta.list = 0;
  return 0;
}

void ta_keep_remove(gchar* path, gboolean is_dir)
{
  g_assert(path != 0);
  _ta_insert(path,0,is_dir,KEEP,REMOVE);
}

void ta_move_remove(gchar* path, gchar* fin_path)
{
  g_assert(path != 0);
  g_assert(fin_path != 0);
  _ta_insert(path,fin_path,0,MOVE,REMOVE);
}

void ta_link_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  _ta_insert(path,src_path,0,LINK,NOTHING);
}

void ta_symlink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  _ta_insert(path,src_path,0,SYMLINK,NOTHING);
}

gint ta_finalize()
{
  GSList* l;

  if (!_ta.active)
  {
    e_set(E_ERROR|TA_NACTIVE, "transaction is not initialized");
    goto err;
  }

  _ta.list = g_slist_reverse(_ta.list);
  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_finalize == MOVE)
    {
      if (!_ta.dryrun)
      {
        if (rename(a->path1, a->path2) == -1)
        {
          _warning("failed mv %s %s", a->path1, a->path2);
          goto next_action;
        }
      }
      _message("mv %s %s", a->path1, a->path2);
    }
    else if (a->on_finalize == LINK)
    {
      if (!_ta.dryrun)
      {
        if (link(a->path2, a->path1) == -1)
        {
          _warning("ln %s %s", a->path2, a->path1);
          goto next_action;
        }
      }
      _message("ln %s %s", a->path2, a->path1);
    }
    else if (a->on_finalize == SYMLINK)
    {
      if (!_ta.dryrun)
      {
        if (symlink(a->path2, a->path1) == -1)
        {
          _warning("ln -s %s %s", a->path2, a->path1);
          goto next_action;
        }
      }
      _message("ln -s %s %s", a->path2, a->path1);
    }
   next_action:
    _ta_free_action(a);
  }
  g_slist_free(_ta.list);
  _ta.list = 0;
  _ta.active = 0;

  return 0;
 err:
  return 1;
}

gint ta_rollback()
{
  GSList* l;

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
      {
        if (!_ta.dryrun)
        {
          if (rmdir(a->path1) == -1)
          {
            _warning("rmdir %s", a->path1);
            goto next_action;
          }
        }
        _message("rmdir %s", a->path1);
      }
      else
      {
        if (!_ta.dryrun)
        {
          if (unlink(a->path1) == -1)
          {
            _warning("rmdir %s", a->path1);
            goto next_action;
          }
        }
        _message("rm %s", a->path1);
      }
    }
    else if (a->on_rollback == NOTHING)
    {
    }
   next_action:
    _ta_free_action(a);
  }
  g_slist_free(_ta.list);
  _ta.list = 0;
  _ta.active = 0;

  return 0;
 err:
  return 1;
}
