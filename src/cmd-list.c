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

#define e_set(n, fmt, args...) e_add(e, "list", __func__, n, fmt, ##args)

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

static gchar* _get_date(time_t t)
{
  static gchar buf[100];
  struct tm* ts = localtime(&t);
  strftime(buf, sizeof(buf), "%F %T", ts);
  return buf;
}

/* public 
 ************************************************************************/

gint cmd_list(GSList* arglist, const struct cmd_options* opts, struct error* e)
{
  g_assert(opts != 0);
  g_assert(e != 0);

  gint verbose = opts->verbosity > 1;
  db_query_type type = verbose ? DB_QUERY_PKGS_WITHOUT_FILES : DB_QUERY_NAMES;

  GSList* list = db_query((db_selector)_list_selector, arglist, type);
  if (!e_ok(e))
  {
    e_set(E_ERROR, "Package query failed!");
    return 1;
  }

  GSList* i;
  for (i = list; i != 0; i = i->next)
  {
    if (verbose)
    {
      struct db_pkg* pkg = i->data;
      printf("+===================================================================+\n");
      printf("| %-65s |\n", pkg->name);
      printf("+===================================================================+\n");
      printf("NAME:    %s\n", pkg->shortname);
      printf("VERSION: %s\n", pkg->version);
      printf("ARCH:    %s\n", pkg->arch);
      printf("BUILD:   %s\n", pkg->build);
      printf("DATE:    %s\n", _get_date(pkg->time));
      printf("CSIZE:   %d kB\n", pkg->csize);
      printf("USIZE:   %d kB\n", pkg->usize);
      if (pkg->desc)
        printf("%s", pkg->desc);
    }
    else
      printf("%s\n", (gchar*)i->data);
  }

  db_free_query(list, type);

  return 0;
}
