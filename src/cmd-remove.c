// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
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

/* purge configuration files from the system. Processed configuration files
 * have their .new suffix removed and don't appear in the package list. So,
 * remove the .new suffix and try to remove that file */
static void _purge(gchar* root, gchar* path)
{
  gint path_len = strlen(path);
  /* only run this for .new files  */
  if (path_len > 4 && !strcmp(path + path_len - 4, ".new"))
  {
    /* strip the .new suffix */
    gchar* path_processed = g_strdup_printf("%.*s", path_len - 4, path);
    gchar* fullpath = g_strdup_printf("%s%s", root, path_processed);

    struct stat st;
    sys_ftype type = sys_file_type_stat(fullpath, 0, &st);
    if (type == SYS_ERR)
    {
      _warning("File type check failed, assuming file does not exist. (%s)", path_processed);
      type = SYS_NONE;
    }
    /* Since this is a processed configuration file, it will not appear in the
     * current package's list. But if it appears in another package's list,
     * then it should not be removed. */
    gint refs = db_filelist_get_path_refs(path_processed);
    if (refs > 0)
    {
      _notice("Not purging file %s (used by another package)", path_processed);
    }
    else
    {
      /* OK, let's go on to remove it */
      if (type == SYS_DIR)
      {
        _warning("Expecting file, but getting directory. (%s)", path_processed);
      }
      else if (type == SYS_NONE)
      {
        _warning("File was already removed. (%s)", path_processed);
      }
      else
      {
        _notice("Purging file %s", path_processed);
        if (unlink(fullpath) < 0)
          _warning("Can't purge file %s. (%s)", path_processed, strerror(errno));
      }
    }
    g_free(path_processed);
    g_free(fullpath);
  }
}

/* public 
 ************************************************************************/

gint cmd_remove(const gchar* pkgname, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgname != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  gchar path[MAXPATHLEN];

  /*
   - load package from db
   - load list of all installed paths
   - remove files that has ref == 1
   - remove symlinks that has ref == 1
   - remove dirs that has ref == 1
   - update list of all installed files
   - update package database: remove package desc
  */

  msg_setup(opts->verbosity);

  _safe_breaking_point(err0);

  /* get package from database */
  gchar* real_pkgname = db_get_package_name(pkgname);
  if (real_pkgname == NULL)
  {
    e_set(E_ERROR|CMD_NOTEX, "Package not found. (%s)", pkgname);
    goto err0;
  }

  _inform("Removing package %s...", real_pkgname);
  
  struct db_pkg* pkg = db_get_pkg(real_pkgname, DB_GET_FULL);
  if (pkg == NULL)
  {
    e_set(E_ERROR|CMD_NOTEX, "Can't get package from the database. (%s)", real_pkgname);
    goto err1;
  }

  /* we will need filelist, so get it if it is not already loaded */
  _debug("Loading list of all installed files...");
  if (db_filelist_load(FALSE))
  {
    e_set(E_ERROR, "Can't load list of all installed files.");
    goto err1;
  }

  _safe_breaking_point(err2);

  _debug("Removing files...");

  gchar* root = sanitize_root_path(opts->root);

  gint* ptype;
  strcpy(path, "");
  JSLF(ptype, pkg->paths, path);
  while (ptype != NULL)
  {
    if (*ptype != DB_PATH_FILE)
      goto skip1;

    /* skip paths we don't want to remove */
    if (!strcmp(path, "install") ||!strncmp(path, "install/", 8) || !strcmp(path, "."))
      goto skip1;

    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    struct stat st;
    sys_ftype type = sys_file_type_stat(fullpath, 0, &st);
    if (type == SYS_ERR)
    {
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_path_refs(path);
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
        _warning("File was already removed. (%s)", path);
        if (opts->purge)
          _purge(root, path);
      }
      else
      {
        if (st.st_mtime > pkg->time)
        {
          _warning("File was changed after installation. (%s)", path);
          if (opts->safe)
            goto skip1_free;
        }
        _notice("Removing file %s", path);
        if (!opts->dryrun)
        {
          if (unlink(fullpath) < 0)
            _warning("Can't remove file %s. (%s)", path, strerror(errno));
          if (opts->purge)
            _purge(root, path);
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
    JSLN(ptype, pkg->paths, path);
  }

  _debug("Removing symlinks...");

  strcpy(path, "");
  JSLF(ptype, pkg->paths, path);
  while (ptype != NULL)
  {
    if (*ptype != DB_PATH_SYMLINK)
      goto skip2;

    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    sys_ftype type = sys_file_type(fullpath, 0);
    if (type == SYS_ERR)
    {
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_path_refs(path);
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
        _warning("Symlink was already removed. (%s)", path);
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
    JSLN(ptype, pkg->paths, path);
  }

  _debug("Removing directories...");

  memset(path, 0xff, sizeof(path)-1);
  path[sizeof(path)-1] = '\0';
  JSLL(ptype, pkg->paths, path);
  while (ptype != NULL)
  {
    if (*ptype != DB_PATH_DIR)
      goto skip3;

    /* skip paths we don't want to remove */
    if (!strcmp(path, "install") ||!strncmp(path, "install/", 8) || !strcmp(path, "."))
      goto skip3;

    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    sys_ftype type = sys_file_type(fullpath, 0);
    if (type == SYS_ERR)
    {
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    gint refs = db_filelist_get_path_refs(path);
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
        _warning("Directory was already removed. (%s)", path);
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
    JSLP(ptype, pkg->paths, path);
  }

  g_free(root);

  _debug("Removing package files from the list of all installed files...");
  db_filelist_rem_pkg_paths(pkg);

  _debug("Removing package from the database...");
  if (!opts->dryrun)
  {
    if (db_rem_pkg(real_pkgname))
    {
      e_set(E_ERROR, "Can't remove package from the database. (%s)", real_pkgname);
      goto err2;
    }
  }

  _debug("Removal finished!");

  db_free_pkg(pkg);
  g_free(real_pkgname);
  return 0;

 err2:
  db_free_pkg(pkg);
 err1:
  g_free(real_pkgname);
 err0:
  e_set(E_PASS,"Package removal failed!");
  return 1;
}
