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

static gint _list_selector(const struct db_pkg* p, GSList* args)
{
  GSList* i;
  if (args == NULL)
    return 1;
  for (i = args; i != 0; i = i->next)
  {
    gint s = fnmatch(i->data, p->name, 0);
    if (s != FNM_NOMATCH && s != 0)
      goto err;
    if (s == 0)
      return 1;
    s = fnmatch(i->data, p->shortname, 0);
    if (s != FNM_NOMATCH && s != 0)
      goto err;
    if (s == 0)
      return 1;
  }
  return 0;
 err:
  return -1;
}

/* public 
 ************************************************************************/

gint cmd_list(GSList* arglist, const struct cmd_options* opts, struct error* e)
{
  g_assert(opts != 0);
  g_assert(e != 0);

  GSList* list = db_query((db_selector)_list_selector, arglist, DB_QUERY_NAMES);
  if (!e_ok(e))
  {
    e_set(E_ERROR, "query failed");
    return 1;
  }
  GSList* i;
  for (i = list; i != 0; i = i->next)
    printf("%s\n", (gchar*)i->data);
  db_free_query(list, DB_QUERY_NAMES);

  return 0;
}
