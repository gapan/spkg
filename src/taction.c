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
#include <errno.h>
#include <time.h>
#include <string.h>

#include "taction.h"
#include "message.h"
#include "sys.h"

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
typedef enum { MOVE, KEEP, LINK, FORCELINK, SYMLINK, CHPERM, FORCESYMLINK } t_on_finalize;
typedef enum { REMOVE, NOTHING } t_on_rollback;

struct action {
  t_on_finalize on_finalize;
  t_on_rollback on_rollback;
  gchar* path1;
  gchar* path2;
  gboolean is_dir;
  gint mode;
  gint owner;
  gint group;
};

static struct action* _ta_insert(
  t_on_finalize on_finalize,
  t_on_rollback on_rollback
)
{
  struct action* a = g_slice_new0(struct action);
  a->on_finalize = on_finalize;
  a->on_rollback = on_rollback;
  _ta.list = g_slist_prepend(_ta.list, a);
  return a;
}

static void _ta_free_action(struct action* a)
{
  g_free(a->path1);
  g_free(a->path2);
  g_slice_free(struct action, a);
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
  struct action* a = _ta_insert(KEEP, REMOVE);
  a->path1 = path;
  a->is_dir = is_dir;
}

void ta_move_remove(gchar* path, gchar* fin_path)
{
  g_assert(path != 0);
  g_assert(fin_path != 0);
  struct action* a = _ta_insert(MOVE, REMOVE);
  a->path1 = path;
  a->path2 = fin_path;
}

void ta_link_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(LINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_symlink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(SYMLINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_forcesymlink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(FORCESYMLINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_forcelink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(FORCELINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_chperm_nothing(gchar* path, gint mode, gint owner, gint group)
{
  g_assert(path != 0);
  struct action* a = _ta_insert(CHPERM, NOTHING);
  a->path1 = path;
  a->mode = mode;
  a->owner = owner;
  a->group = group;
}

gint ta_finalize()
{
  GSList* l;

  if (!_ta.active)
  {
    e_set(E_ERROR|TA_NACTIVE, "transaction is not initialized");
    return 1;
  }

  _ta.list = g_slist_reverse(_ta.list);
  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_finalize == MOVE)
    {
      _notice("mv %s %s", a->path1, a->path2);
      if (!_ta.dryrun)
      {
        if (rename(a->path1, a->path2) == -1)
        {
          _warning("failed mv %s %s", a->path1, a->path2);
          continue;
        }
      }
    }
    else if (a->on_finalize == LINK)
    {
      _notice("ln %s %s", a->path2, a->path1);
      if (!_ta.dryrun)
      {
        if (link(a->path2, a->path1) == -1)
        {
          _warning("failed ln %s %s", a->path2, a->path1);
          continue;
        }
      }
    }
    else if (a->on_finalize == FORCELINK)
    {
      _notice("rm -rf %s", a->path1);
      _notice("ln %s %s", a->path2, a->path1);
      if (!_ta.dryrun)
      {
        if (sys_rm_rf(a->path1))
        {
          _warning("failed rm -rf %s", a->path1);
          continue;
        }
        if (link(a->path2, a->path1) == -1)
        {
          _warning("failed ln %s %s", a->path2, a->path1);
          continue;
        }
      }
    }
    else if (a->on_finalize == SYMLINK)
    {
      _notice("ln -s %s %s", a->path2, a->path1);
      if (!_ta.dryrun)
      {
        if (symlink(a->path2, a->path1) == -1)
        {
          _warning("failed ln -s %s %s", a->path2, a->path1);
          continue;
        }
      }
    }
    else if (a->on_finalize == FORCESYMLINK)
    {
      _notice("rm -rf %s", a->path1);
      _notice("ln -s %s %s", a->path2, a->path1);
      if (!_ta.dryrun)
      {
        if (sys_rm_rf(a->path1))
        {
          _warning("failed rm -rf %s", a->path1);
          continue;
        }
        if (symlink(a->path2, a->path1) == -1)
        {
          _warning("failed ln -s %s %s", a->path2, a->path1);
          continue;
        }
      }
    }
    else if (a->on_finalize == CHPERM)
    {
      /* chmod */
      _notice("chmod %05o %s", a->mode, a->path1);
      if (!_ta.dryrun)
      {
        if (chmod(a->path1, a->mode) == -1)
        {
          _warning("failed chmod %05o %s: %s", a->mode, a->path1, strerror(errno));
        }
      }
      /* chown */
      _notice("chown %d:%d %s", a->owner, a->group, a->path1);
      if (!_ta.dryrun)
      {
        if (chown(a->path1, a->owner, a->group) == -1)
        {
          _warning("failed chown %05o %s: %s", a->mode, a->path1, strerror(errno));
        }
      }
    }
  }

  g_slist_foreach(_ta.list, (GFunc)_ta_free_action, 0);
  g_slist_free(_ta.list);
  memset(&_ta, 0, sizeof(_ta));
  return 0;
}

gint ta_rollback()
{
  GSList* l;

  if (!_ta.active)
  {
    e_set(E_ERROR|TA_NACTIVE, "transaction is not initialized");
    return 1;
  }

  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_rollback == REMOVE)
    {
      if (a->is_dir)
      {
        _notice("rmdir %s", a->path1);
        if (!_ta.dryrun)
        {
          if (rmdir(a->path1) == -1)
          {
            _warning("failed rmdir %s", a->path1);
            continue;
          }
        }
      }
      else
      {
        _notice("rm %s", a->path1);
        if (!_ta.dryrun)
        {
          if (unlink(a->path1) == -1)
          {
            _warning("failed rmdir %s", a->path1);
            continue;
          }
        }
      }
    }
  }

  g_slist_foreach(_ta.list, (GFunc)_ta_free_action, 0);
  g_slist_free(_ta.list);
  memset(&_ta, 0, sizeof(_ta));
  return 0;
}
