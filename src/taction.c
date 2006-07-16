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
  GSList* endlist;
};

static struct transaction _ta = {
  .active = 0,
  .err = 0,
  .list = 0,
  .dryrun = 0
};

/* action list handling */
typedef enum {
  MOVE, KEEP, LINK, FORCELINK, SYMLINK, CHPERM, FORCESYMLINK, REMOVE,
  NOTHING
} t_action;

struct action {
  t_action on_finalize;
  t_action on_rollback;
  gchar* path1;
  gchar* path2;
  gboolean is_dir;
  gint mode;
  gint owner;
  gint group;
};

static struct action* _ta_insert(
  GSList** list,
  t_action on_finalize,
  t_action on_rollback
)
{
  g_assert(list != NULL);
  struct action* a = g_slice_new0(struct action);
  a->on_finalize = on_finalize;
  a->on_rollback = on_rollback;
  *list = g_slist_prepend(*list, a);
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
    _e_set(e, E_ERROR|TA_ACTIVE, "Can't start transaction while another transaction is still in progress.");
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
  struct action* a = _ta_insert(&_ta.list, KEEP, REMOVE);
  a->path1 = path;
  a->is_dir = is_dir;
}

void ta_move_remove(gchar* path, gchar* fin_path)
{
  g_assert(path != 0);
  g_assert(fin_path != 0);
  struct action* a = _ta_insert(&_ta.list, MOVE, REMOVE);
  a->path1 = path;
  a->path2 = fin_path;
}

void ta_link_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(&_ta.list, LINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_symlink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(&_ta.list, SYMLINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_forcesymlink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(&_ta.list, FORCESYMLINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_forcelink_nothing(gchar* path, gchar* src_path)
{
  g_assert(path != 0);
  g_assert(src_path != 0);
  struct action* a = _ta_insert(&_ta.list, FORCELINK, NOTHING);
  a->path1 = path;
  a->path2 = src_path;
}

void ta_chperm_nothing(gchar* path, gint mode, gint owner, gint group)
{
  g_assert(path != 0);
  struct action* a = _ta_insert(&_ta.list, CHPERM, NOTHING);
  a->path1 = path;
  a->mode = mode;
  a->owner = owner;
  a->group = group;
}

void ta_remove_nothing(gchar* path, gint is_dir)
{
  g_assert(path != 0);
  if (is_dir)
  {
    struct action* a = _ta_insert(&_ta.endlist, REMOVE, NOTHING);
    a->path1 = path;
    a->is_dir = is_dir;
  }
  else
  {
    struct action* a = _ta_insert(&_ta.list, REMOVE, NOTHING);
    a->path1 = path;
    a->is_dir = is_dir;
  }
}

gint ta_finalize()
{
  GSList* l;

  if (!_ta.active)
  {
    e_set(E_ERROR|TA_NACTIVE, "Can't finalize transaction, because no transaction was started.");
    return 1;
  }

  _ta.list = g_slist_reverse(_ta.list);
  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_finalize == MOVE)
    {
      _notice("Moving %s -> %s", a->path1, a->path2);
      if (!_ta.dryrun)
      {
        if (rename(a->path1, a->path2) == -1)
        {
          _warning("Failed to move %s -> %s (%s)", a->path1, a->path2, strerror(errno));
        }
      }
    }
    else if (a->on_finalize == LINK)
    {
      _notice("Creating hardlink %s -> %s", a->path1, a->path2);
      if (!_ta.dryrun)
      {
        if (link(a->path2, a->path1) == -1)
        {
          _warning("Failed to create hardlink %s -> %s (%s)", a->path1, a->path2, strerror(errno));
        }
      }
    }
    else if (a->on_finalize == FORCELINK)
    {
      _notice("Removing path %s", a->path1);
      if (!_ta.dryrun)
      {
        if (sys_rm_rf(a->path1))
        {
          _warning("Failed to remove path %s", a->path1);
          continue;
        }
      }
      _notice("Creating hardlink %s -> %s", a->path1, a->path2);
      if (!_ta.dryrun)
      {
        if (link(a->path2, a->path1) == -1)
        {
          _warning("Failed to create hardlink %s -> %s (%s)", a->path1, a->path2, strerror(errno));
        }
      }
    }
    else if (a->on_finalize == SYMLINK)
    {
      _notice("Creating symlink %s -> %s", a->path1, a->path2);
      if (!_ta.dryrun)
      {
        if (symlink(a->path2, a->path1) == -1)
        {
          _warning("Failed to create symlink %s -> %s (%s)", a->path1, a->path2, strerror(errno));
        }
      }
    }
    else if (a->on_finalize == FORCESYMLINK)
    {
      _notice("Removing path %s", a->path1);
      if (!_ta.dryrun)
      {
        if (sys_rm_rf(a->path1))
        {
          _warning("Failed to remove path %s", a->path1);
          continue;
        }
      }
      _notice("Creating symlink %s -> %s", a->path1, a->path2);
      if (!_ta.dryrun)
      {
        if (symlink(a->path2, a->path1) == -1)
        {
          _warning("Failed to create symlink %s -> %s (%s)", a->path1, a->path2, strerror(errno));
        }
      }
    }
    else if (a->on_finalize == CHPERM)
    {
      /* chmod */
      _notice("Changing mode on %s to %04o", a->path1, a->mode);
      if (!_ta.dryrun)
      {
        if (chmod(a->path1, a->mode) == -1)
        {
          _warning("Failed to cange mode on %s to %04o (%s)", a->path1, a->mode, strerror(errno));
        }
      }
      /* chown */
      _notice("Changing owner on %s to %d:%d", a->path1, a->owner, a->group);
      if (!_ta.dryrun)
      {
        if (chown(a->path1, a->owner, a->group) == -1)
        {
          _warning("Failed to cange owner on %s to %d:%d (%s)", a->path1, a->owner, a->group, strerror(errno));
        }
      }
    }
    else if (a->on_finalize == REMOVE)
    {
      if (!a->is_dir)
      {
        _notice("Removing file %s", a->path1);
        if (!_ta.dryrun)
        {
          if (unlink(a->path1))
          {
            _warning("Failed to remove file %s. (%s)", a->path1, strerror(errno));
          }
        }
      }
    }
  }

  for (l=_ta.endlist; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_finalize == REMOVE)
    {
      if (a->is_dir)
      {
        _notice("Removing directory %s", a->path1);
        if (!_ta.dryrun)
        {
          if (rmdir(a->path1))
          {
            _warning("Failed to remove directory %s. (%s)", a->path1, strerror(errno));
          }
        }
      }
    }
  }

  g_slist_foreach(_ta.endlist, (GFunc)_ta_free_action, 0);
  g_slist_free(_ta.endlist);
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
    e_set(E_ERROR|TA_NACTIVE, "Can't rollback transaction, because no transaction was started.");
    return 1;
  }

  for (l=_ta.list; l!=0; l=l->next)
  {
    struct action* a = l->data;
    if (a->on_rollback == REMOVE)
    {
      if (a->is_dir)
      {
        _notice("Removing directory %s", a->path1);
        if (!_ta.dryrun)
        {
          if (rmdir(a->path1) == -1)
          {
            _warning("Failed to remove directory %s (%s)", a->path1, strerror(errno));
          }
        }
      }
      else
      {
        _notice("Removing file %s", a->path1);
        if (!_ta.dryrun)
        {
          if (unlink(a->path1) == -1)
          {
            _warning("Failed to remove file %s (%s)", a->path1, strerror(errno));
          }
        }
      }
    }
  }

  g_slist_foreach(_ta.endlist, (GFunc)_ta_free_action, 0);
  g_slist_free(_ta.endlist);
  g_slist_foreach(_ta.list, (GFunc)_ta_free_action, 0);
  g_slist_free(_ta.list);
  memset(&_ta, 0, sizeof(_ta));
  return 0;
}
