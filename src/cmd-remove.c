/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <Judy.h>

#include "sys.h"
#include "path.h"
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
  gchar* real_pkgname = db_get_package_name(pkgname);
  if (real_pkgname == NULL)
  {
    e_set(E_ERROR, "package not found (%s)", pkgname);
    goto err0;
  }
  
  struct db_pkg* pkg = db_get_pkg(real_pkgname, DB_GET_FULL);
  if (pkg == NULL)
  {
    e_set(E_ERROR, "internal error (%s)", real_pkgname);
    goto err1;
  }

  _notice("loading list of files...");

  /* we will need filelist, so get it if it is not already loaded */
  db_filelist_load(FALSE);

  _safe_breaking_point(err2);

  _notice("removing files...");

  gchar* root = sanitize_root_path(opts->root);

  strcpy(path, "");
  JSLF(ptr, pkg->files, path);
  while (ptr != NULL)
  {
    gint len = strlen(path);
    /* skip links and dirs */
    if ((len > 0 && path[len-1] == '/') || (*ptr != 0))
      goto skip1;

    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    sys_ftype type = sys_file_type(fullpath, 0);
    if (type == SYS_ERR)
    {
      _warning("can't determine file type, assuming it does not exist: %s", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_file(path);
    if (refs == 0)
    {
      _warning("file is in package but not in filelist: %s", path);
    }
    else if (refs == 1)
    {
      if (type == SYS_DIR)
      {
        _warning("this should not be directory, but file: %s", path);
      }
      else if (type == SYS_NONE)
      {
        _warning("file does not exist: %s", path);
      }
      else
      {
        _notice("removing file: %s", path);
        _debug("unlink(%s)", fullpath);
        if (!opts->dryrun)
        {
          if (unlink(fullpath) < 0)
            _warning("file removal failed: %s: %s", path, strerror(errno));
        }
      }
    }
    else
    {
      _notice("keeping file: %s", path);
    }

    g_free(fullpath);

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

    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    sys_ftype type = sys_file_type(fullpath, 0);
    if (type == SYS_ERR)
    {
      _warning("can't determine file type, assuming it does not exist: %s", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_link(path);
    if (refs == 0)
    {
      _warning("link is in package but not in filelist: %s", path);
    }
    else if (refs == 1)
    {
      if (type == SYS_SYM)
      {
        _notice("removing link: %s", path);
        _debug("unlink(%s)", fullpath);
        if (!opts->dryrun)
        {
          if (unlink(fullpath) < 0)
            _warning("link removal failed: %s: %s", path, strerror(errno));
        }
      }
      else if (type == SYS_NONE)
      {
        _warning("link does not exist: %s", path);
      }
      else
      {
        _warning("this file is not a link: %s", path);
      }
    }
    else
    {
      _notice("keeping link: %s", path);
    }

    g_free(fullpath);

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

    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    sys_ftype type = sys_file_type(fullpath, 0);
    if (type == SYS_ERR)
    {
      _warning("can't determine file type, assuming it does not exist: %s", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_file(path);
    if (refs == 0)
    {
      _warning("directory is in package but not in filelist: %s", path);
    }
    else if (refs == 1)
    {
      if (type == SYS_DIR)
      {
        _notice("removing directory: %s", path);
        _debug("rmdir(%s)", fullpath);
        if (!opts->dryrun)
        {
          if (rmdir(fullpath) < 0)
            _warning("directory removal failed: %s: %s", path, strerror(errno));
        }
      }
      else if (type == SYS_NONE)
      {
        _warning("dir does not exist: %s", path);
      }
      else
      {
        _warning("not a directory: %s", path);
      }
    }
    else
    {
      _notice("keeping directory: %s", path);
    }

    g_free(fullpath);

   skip3:
    JSLP(ptr, pkg->files, path);
  }

  g_free(root);

  _notice("removing files from filelist...");
  db_filelist_rem_pkg_files(pkg);

  _notice("removing package from database...");
  if (!opts->dryrun)
  {
    if (db_rem_pkg(real_pkgname))
    {
      e_set(E_ERROR, "can't remove package from database (%s)", real_pkgname);
      goto err2;
    }
  }

  _notice("finished");

  db_free_pkg(pkg);
  g_free(real_pkgname);
  return 0;

 err2:
  db_free_pkg(pkg);
 err1:
  g_free(real_pkgname);
 err0:
  _notice("removal failed");
  return 1;
}
