/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <Judy.h>

#include "cmd-private.h"

/* public 
 ************************************************************************/

gint cmd_remove(const gchar* pkgname, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgname != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  gchar path[4096];
  void **ptr;

  msg_setup("remove", opts->verbosity);

  _inform("removing package %s", pkgname);

  _safe_breaking_point(err0);

  _notice("geting package from database: %s", pkgname);
  /* get package from database */
  struct db_pkg* pkg = db_get_pkg(pkgname, DB_GET_FULL);
  if (pkg == NULL)
  {
    if (!(e_errno(e) & DB_NOTEX))
      e_set(E_ERROR|CMD_EXIST, "package is not installed (%s)", pkgname);
    else
      e_set(E_ERROR, "internal error (%s)", pkgname);
    db_free_pkg(pkg);
    goto err0;
  }

  _notice("loading list of files...");

  /* we will need filelist, so get it if it is not already loaded */
  db_filelist_load(FALSE);

  _safe_breaking_point(err1);

  _notice("removing files...");

//        if (!opts->dryrun)

  strcpy(path, "");
  JSLF(ptr, pkg->files, path);
  while (ptr != NULL)
  {
    gint len = strlen(path);
    /* skip links and dirs */
    if ((len > 0 && path[len-1] == '/') || (*ptr != 0))
      goto skip1;

    gint refs = db_filelist_get_file(path);
    if (refs == 0)
    {
      _warning("file is in package but not in filelist: %s", path);
    }
    else if (refs == 1)
    {
      _notice("removing file: %s", path);
    }
    else
    {
      _notice("keeping file: %s", path);
    }

   skip1:
    JSLN(ptr, pkg->files, path);
  }

  _notice("removing links...");

  strcpy(path, "");
  JSLF(ptr, pkg->files, path);
  while (ptr != NULL)
  {
    gint len = strlen(path);
    /* skip links and regular files */
    if ((len > 0 && path[len-1] == '/') || (*ptr == 0))
      goto skip2;

    gint refs = db_filelist_get_link(path);
    if (refs == 0)
    {
      _warning("link is in package but not in filelist: %s", path);
    }
    else if (refs == 1)
    {
      _notice("removing link: %s", path);
    }
    else
    {
      _notice("keeping link: %s", path);
    }

   skip2:
    JSLN(ptr, pkg->files, path);
  }

  _notice("removing directories...");

  memset(path, 0xff, sizeof(path)-1);
  path[sizeof(path)-1] = '\0';
  JSLL(ptr, pkg->files, path);
  while (ptr != NULL)
  {
    gint len = strlen(path);
    /* skip links and regular files */
    if ((len > 0 && path[len-1] != '/') || (*ptr != 0))
      goto skip3;

    gint refs = db_filelist_get_file(path);
    if (refs == 0)
    {
      _warning("directory is in package but not in filelist: %s", path);
    }
    else if (refs == 1)
    {
      _notice("removing directory: %s", path);
    }
    else
    {
      _notice("keeping directory: %s", path);
    }

   skip3:
    JSLP(ptr, pkg->files, path);
  }

  _notice("finished");

  return 0;

 err1:
  db_free_pkg(pkg);
 err0:
  _notice("removal terminated");
  return 1;
}
