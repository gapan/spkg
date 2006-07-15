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

/* private
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(e, "remove", __func__, n, fmt, ##args)

/* public 
 ************************************************************************/

gint cmd_remove(const gchar* pkgname, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgname != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  gchar path[4096];
  void **ptr;

  msg_setup(opts->verbosity);
  _inform("Removing package %s...", pkgname);

  _safe_breaking_point(err0);

  /* get package from database */
  gchar* real_pkgname = db_get_package_name(pkgname);
  if (real_pkgname == NULL)
  {
    e_set(E_ERROR, "Package not found. (%s)", pkgname);
    goto err0;
  }
  
  struct db_pkg* pkg = db_get_pkg(real_pkgname, DB_GET_FULL);
  if (pkg == NULL)
  {
    e_set(E_ERROR, "Can't get package from the database. (%s)", real_pkgname);
    goto err1;
  }

  _notice("Loading list of all installed files...");

  /* we will need filelist, so get it if it is not already loaded */
  db_filelist_load(FALSE);

  _safe_breaking_point(err2);

  _notice("Removing files...");

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
    struct stat st;
    sys_ftype type = sys_file_type_stat(fullpath, 0, &st);
    if (type == SYS_ERR)
    {
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_file(path);
    if (refs == 0)
    {
      _warning("File is in the package but not in the filelist. (%s)", path);
    }
    else if (refs == 1)
    {
      if (type == SYS_DIR)
      {
        _warning("Expecting file, but getting directory. (%s)", path);
      }
      else if (type == SYS_NONE)
      {
        _warning("File was already removed: %s", path);
      }
      else
      {
        if (st.st_mtime > pkg->time)
        {
          _warning("File was changed after installation: %s", path);
          if (opts->safe)
            goto skip1_free;
        }
        _notice("Removing file %s", path);
        if (!opts->dryrun)
        {
          if (unlink(fullpath) < 0)
            _warning("Can't remove file %s. (%s)", path, strerror(errno));
        }
      }
    }
    else
    {
      _notice("Keeping file %s (used by another package)", path);
    }

   skip1_free:
    g_free(fullpath);

   skip1:
    JSLN(ptr, pkg->files, path);
  }

  _notice("Removing symlinks...");

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
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_link(path);
    if (refs == 0)
    {
      _warning("Symlink is in the package but not in filelist. (%s)", path);
    }
    else if (refs == 1)
    {
      if (type == SYS_SYM)
      {
        _notice("Removing symlink %s", path);
        if (!opts->dryrun)
        {
          if (unlink(fullpath) < 0)
            _warning("Can't remove symlink %s. (%s)", path, strerror(errno));
        }
      }
      else if (type == SYS_NONE)
      {
        _warning("Symlink was already removed: %s", path);
      }
      else
      {
        _warning("Expecting symlink, but getting something else. (%s)", path);
      }
    }
    else
    {
      _notice("Keeping symlink %s (used by another package)", path);
    }

    g_free(fullpath);

   skip2:
    JSLN(ptr, pkg->files, path);
  }

  _notice("Removing directories...");

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
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_file(path);
    if (refs == 0)
    {
      _warning("Directory is in the package but not in filelist. (%s)", path);
    }
    else if (refs == 1)
    {
      if (type == SYS_DIR)
      {
        _notice("Removing directory %s", path);
        if (!opts->dryrun)
        {
          if (rmdir(fullpath) < 0)
            _warning("Can't remove directory %s. (%s)", path, strerror(errno));
        }
      }
      else if (type == SYS_NONE)
      {
        _warning("Directory was already removed: %s", path);
      }
      else
      {
        _warning("Expecting directory, but getting something else. (%s)", path);
      }
    }
    else
    {
      _notice("Keeping directory %s (used by another package)", path);
    }

    g_free(fullpath);

   skip3:
    JSLP(ptr, pkg->files, path);
  }

  g_free(root);

  _notice("Removing package files from the list of all installed files...");
  db_filelist_rem_pkg_files(pkg);

  _notice("Removing package from the database...");
  if (!opts->dryrun)
  {
    if (db_rem_pkg(real_pkgname))
    {
      e_set(E_ERROR, "Can't remove package from the database. (%s)", real_pkgname);
      goto err2;
    }
  }

  _notice("Removal finished!");

  db_free_pkg(pkg);
  g_free(real_pkgname);
  return 0;

 err2:
  db_free_pkg(pkg);
 err1:
  g_free(real_pkgname);
 err0:
  e_set(E_ERROR,"Package removal failed!");
  return 1;
}
