/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <fnmatch.h>

#include "cmd-private.h"

/* private
 ************************************************************************/

typedef GSList* (*query_func)(db_selector, void*, db_query_type);

static gint _glob_selector(const struct db_pkg* p, void* d)
{
  gint s = fnmatch(d, p->name, 0);
  if (s == FNM_NOMATCH)
    return 0;
  if (s == 0)
    return 1;
  return -1;
}

/* public 
 ************************************************************************/

gint cmd_list(
  const gchar* regexp,
  cmd_list_mode mode,
  const struct cmd_options* opts,
  struct error* e
)
{
  g_assert(opts != 0);
  g_assert(e != 0);

  GSList* list;
  GSList* i;

  switch (mode)
  {
    case CMD_MODE_GLOB:
      list = db_query(_glob_selector, regexp, DB_QUERY_NAMES);
      if (!e_ok(e))
      {
        e_set(E_ERROR, "query failed");
        goto err;
      }
      for (i=list; i!=0; i=i->next)
        printf("%s\n", (gchar*)i->data);
      db_free_query(list, DB_QUERY_NAMES);
    break;
    case CMD_MODE_ALL:
      list = db_query(0, 0, DB_QUERY_NAMES);
      if (!e_ok(e))
      {
        e_set(E_ERROR, "query failed");
        goto err;
      }
      for (i=list; i!=0; i=i->next)
        printf("%s\n", (gchar*)i->data);
      db_free_query(list, DB_QUERY_NAMES);
    break;
    default:
      e_set(E_FATAL, "invalid mode");
      goto err;
  }

  return 0;
 err:
  return 1;
}
