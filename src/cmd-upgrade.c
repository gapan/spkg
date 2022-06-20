// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <Judy.h>

#include "cmd-common.h"

/* private 
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(e, "upgrade", __func__, n, fmt, ##args)

static void _delete_leftovers(struct db_pkg* pkg, struct db_pkg* ipkg,
                              const gchar* root, const struct cmd_options* opts,
                              struct error* e)
{
  gchar path[MAXPATHLEN];
  gint* ptype;
  
  /* 
   - for each path in original package
     - check if that path exist in the newly instaled package (either as dir or regular file)
     - check if that path exist in the system file database
     - remove if not
  */
  
  /* for each path in the original package */
  strcpy(path, "");
  JSLF(ptype, ipkg->paths, path);
  while (ptype != NULL)
  {
    /* search for a path in the newly installed package */
    db_path_type ptype_new = db_pkg_get_path(pkg, path);
    /* if it exists, continue with next path */
    if (ptype_new != DB_PATH_NONE)
      goto skip;

    /* skip paths we don't want to remove */
    if (!strcmp(path, "install") ||!strncmp(path, "install/", 8) || !strcmp(path, "."))
      goto skip;

    /* get refs to this path from the global filelist */
    gint refs = db_filelist_get_path_refs(path);
    /* if it's referenced more than once (i.e. not only by
       this package), continue with next path */
    if (refs > 1)
    {
      _notice("Keeping file %s (used by another package)", path);
      goto skip;
    }

    /* get full path and check file on the filesystem */
    gchar* fullpath = g_strdup_printf("%s%s", root, path);
    struct stat st;
    sys_ftype type = sys_file_type_stat(fullpath, 0, &st);
    if (type == SYS_ERR)
    {
      _warning("File type check failed, assuming file does not exist. (%s)", path);
      type = SYS_NONE;
    }

    if (type == SYS_DIR)
    {
      if (*ptype != DB_PATH_DIR)
      {
        _warning("Expecting file, but getting directory. (%s) It will not be removed.", path);
      }
      else
      {
        _debug("Removing directory %s (postponed)", path);
        ta_remove_nothing(fullpath, TRUE);
        fullpath = NULL;
      }
    }
    else if (type == SYS_NONE)
    {
      _warning("File was already removed. (%s)", path);
    }
    else
    {
      if (*ptype != DB_PATH_FILE && *ptype != DB_PATH_SYMLINK)
      {
        _warning("Expecting directory, but getting file. (%s) It will not be removed.", path);
      }
      else
      {
        if (st.st_mtime > pkg->time)
        {
          _warning("File was changed after installation. (%s)", path);
          if (opts->safe)
            goto skip_free;
        }
        /* don't remove symlinks to critical libc libraries */
        if (_check_libc_libs(path) == 0 && *ptype == DB_PATH_SYMLINK) {
          _debug("Libc library (%s). Not removing symlink.", path);
        } else {
          _debug("Removing file %s (postponed)", path);
          ta_remove_nothing(fullpath, FALSE);
        }
        fullpath = NULL;
      }
    }

   skip_free:
    g_free(fullpath);
   skip:
    JSLN(ptype, ipkg->paths, path);
  }
}

/* public 
 ************************************************************************/

gint cmd_upgrade(const gchar* pkgfile, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgfile != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  /*
   - check if package file exists
   - check/parse package name format
   - get installed package full name (using shortname)
   - check if names are different (package not already installed)
   - load installed package from database
   - load global filelist
   - open package tgz file
   - initialize transaction
   - prepare empty db_pkg object for newly installed package
   - for each file in the package:
     - sanitize path
     - handle special files (install/)
     - extract file
   - build delete after upgrade list of files and put them into transaction
   - update package database: remove old package desc, install new package desc
   - finish transaction
   - close package file
   - run doinst.sh
   - run ldconfig
   - run gtk-update-icon-cache in the hicolor icon dir
   - remove install/
  */

  msg_setup(opts->verbosity);

  /* check if file exist and is regular file */
  if (sys_file_type(pkgfile,1) != SYS_REG)
  {
    e_set(E_ERROR, "Package file does not exist. (%s)", pkgfile);
    goto err0;
  }

  /* parse package name from the file path */
  gchar *name = NULL, *shortname = NULL;
  if ((name = parse_pkgname(pkgfile,5)) == 0 
      || (shortname = parse_pkgname(pkgfile,1)) == 0)
  {
    e_set(E_ERROR, "Package name is invalid. (%s)", pkgfile);
    goto err1;
  }
  
  _safe_breaking_point(err1);

  /* check if package is already in the database */  
  gchar* installed_pkgname = db_get_package_name(name); /* search by full name */
  if (installed_pkgname)
  {
    if (opts->reinstall)
    {
      _inform("Reinstalling package %s...", name);
    }
    else
    {
      e_set(E_ERROR|CMD_EXIST, "Package is already installed. (%s)", installed_pkgname);
      g_free(installed_pkgname);
      goto err1;
    }
  }
  else
  {
    installed_pkgname = db_get_package_name(shortname);
    if (installed_pkgname == NULL)
    {
      e_set(E_ERROR|CMD_NOTEX, "There is no installed package named %s.", shortname);
      goto err1;
    }
    _inform("Upgrading package %s -> %s...", installed_pkgname, name);
  }

  _safe_breaking_point(err1);

  /*EXIT: free(name), free(shortname), free(installed_pkgname) */

  /* load original package from database */
  struct db_pkg *pkg = NULL, *ipkg = NULL;
  ipkg = db_get_pkg(installed_pkgname, DB_GET_FULL);
  if (ipkg == NULL)
  {
    e_set(E_ERROR|CMD_NOTEX, "Can't load package from database. (%s)", installed_pkgname);
    g_free(installed_pkgname);
    goto err1;
  }
  g_free(installed_pkgname);

  /* EXIT: free(name), free(shortname), db_free_pkg(ipkg) */

  /* open package's tgz archive */
  struct untgz_state* tgz = NULL;
  tgz = untgz_open(pkgfile, e);
  if (tgz == NULL)
  {
    e_set(E_ERROR,"Can't open package file. (%s)", pkgfile);
    goto err2;
  }

  /* we will need filelist, so get it if it is not already loaded */
  _debug("Loading list of all installed files...");
  if (db_filelist_load(FALSE))
  {
    e_set(E_ERROR, "Can't load list of all installed files.");
    goto err2;
  }

  /* EXIT: free(name), free(shortname), db_free_pkg(ipkg), untgz_close(tgz) */

  /* init transaction */
  if (ta_initialize(opts->dryrun, e))
  {
    e_set(E_ERROR,"Can't initialize transaction.");
    goto err2;
  }

  /* alloc package object */
  pkg = db_alloc_pkg(name);
  if (pkg == NULL)
    goto err2;
  pkg->location = g_strdup(pkgfile);

  gboolean need_ldconfig = 0;
  gboolean need_update_icon_cache = 0;
  gboolean has_doinst = 0;
  gchar* sane_path = NULL;
  gchar* root = sanitize_root_path(opts->root);

  /* EXIT: free(name), free(shortname), untgz_close(tgz),
     ta_finalize/rollback(), free(root), db_free_pkg(pkg),
     db_free_pkg(ipkg) */

  /* for each file in package */
  while (untgz_get_header(tgz) == 0)
  {
    _safe_breaking_point(err3);

    /* EXIT: free(name), free(shortname), untgz_close(tgz),
       ta_finalize/rollback(), free(root), db_free_pkg(pkg),
       db_free_pkg(pkg), free(sane_path) */

    /* check file path */
    g_free(sane_path);
    sane_path = path_simplify(tgz->f_name);
    if (sane_path == NULL || _unsafe_path(sane_path))
    {
      /* some damned fucker created this package to mess our system */
      e_set(E_ERROR,"Package contains file with unsafe path. (%s)", tgz->f_name);
      goto err3;
    }

    /* check for ./ */
    if (sane_path[0] == '\0')
    {
      db_pkg_add_path(pkg, ".", DB_PATH_DIR);
      continue;
    }
    
    /* check if package contains .so libraries */
    if (!need_ldconfig && g_str_has_suffix(sane_path, ".so"))
      need_ldconfig = 1;

    /* check if package contains .desktop files */
    if (!need_update_icon_cache && g_str_has_suffix(sane_path, ".desktop"))
      need_update_icon_cache = 1;

    /* add file to the package */
    if (db_pkg_add_path(pkg, sane_path, tgz->f_type == UNTGZ_DIR ? DB_PATH_DIR : DB_PATH_FILE))
    {
      e_set(E_ERROR, "Can't add path to the package, it's too long. (%s)", sane_path);
      goto err3;
    }

    /* check for metadata files */
    if (!strcmp(sane_path, "install/slack-desc"))
    {
      if (tgz->f_size > 1024*4) /* 4K is enough */
      {
        e_set(E_ERROR, "Package description file is too big. (%ld kB)", tgz->f_size / 1024);
        goto err3;
      }
      _read_slackdesc(tgz, pkg);
      db_pkg_add_path(pkg, "install/slack-desc", DB_PATH_FILE);
      continue;
    }
    else if (!strcmp(sane_path, "install/doinst.sh"))
    {
      if (tgz->f_size > DOINSTSH_MAX_SIZE)
      {
        e_set(E_ERROR, "Installation script is too big. (%ld kB)", tgz->f_size / 1024);
        goto err3;
      }
      has_doinst = _read_doinst_sh(tgz, pkg, root, sane_path, opts, e, TRUE, ipkg);
      if (!e_ok(e))
      {
        e_set(E_ERROR, "Installation script processing failed.");
        goto err3;
      }
      db_pkg_add_path(pkg, "install/doinst.sh", DB_PATH_FILE);
      continue;
    }
    else if (!strncmp(sane_path, "install/", 8) && strcmp(sane_path, "install"))
      continue;

    _extract_file(tgz, pkg, sane_path, root, opts, e, TRUE, ipkg);
    if (!e_ok(e))
      goto err3;
  }
  g_free(sane_path);
  sane_path = NULL;

  /* EXIT: free(name), free(shortname), untgz_close(tgz),
     ta_finalize/rollback(), free(root), db_free_pkg(pkg),
     db_free_pkg(ipkg) */
  
  /* error occured during extraction */
  if (!e_ok(e))
  {
    e_set(E_ERROR,"Package file is corrupted. (%s)", pkgfile);
    goto err3;
  }

  pkg->usize = tgz->usize/1024;
  pkg->csize = tgz->csize/1024;

  /* add package to the database */
  _debug("Updating package database...");
  if (!opts->dryrun)
  {
    if (db_replace_pkg(ipkg->name, pkg))
    {
      e_set(E_ERROR,"Can't add package to the database.");
      goto err3;
    }
    _safe_breaking_point(err4);
  }

  /* delete leftover files:
   * - for each file in installed package check if file does not
   *   exist in then new package and remove it in that case
   */
  _delete_leftovers(pkg, ipkg, root, opts, e);

  /* update filelist */
  db_filelist_rem_pkg_paths(ipkg);
  db_filelist_add_pkg_paths(pkg);

  /* finalize transaction */
  _debug("Finalizing transaction...");
  ta_finalize();

  /* close tgz */
  untgz_close(tgz);
  tgz = NULL;

  /* EXIT: free(name), free(shortname), free(root), db_free_pkg(pkg),
     db_free_pkg(ipkg) */

  if (!opts->dryrun && !opts->no_scripts && has_doinst && !opts->safe)
  {
    gchar* doinst_path = g_strdup_printf("%sinstall/doinst.sh", root);
    /* run doinst.sh if it exists */
    if (sys_file_type(doinst_path, 0) == SYS_REG)
    {
      gchar* qroot = g_shell_quote(root);
      gchar* cmd = g_strdup_printf("cd %s && . install/doinst.sh -install", qroot);
      g_free(qroot);

      _notice("Running post-installation script...");
      gint rv = system(cmd);
      if (rv < 0)
        _warning("Can't execute post-installation script. (%s)", strerror(errno));
      else if (rv > 0)
        _warning("Post-installation script failed. (%d)", rv);
      unlink(doinst_path);

      g_free(cmd);
    }
    g_free(doinst_path);
  }

  /* run ldconfig */
  if (need_ldconfig && !opts->no_ldconfig)
    _run_ldconfig(root, opts);

  /* run gtk-update-icon-cache */
  if (need_update_icon_cache && !opts->no_gtk_update_icon_cache)
    _gtk_update_icon_cache(root, opts);

  if (!opts->dryrun)
  {
    gchar* install_path = g_strdup_printf("%sinstall", root);
    rmdir(install_path);
    g_free(install_path);
  }

  /* EXIT: free(name), free(shortname), free(root), db_free_pkg(pkg),
     db_free_pkg(ipkg) */

  _debug("Package upgrade finished!");

  db_free_pkg(ipkg);
  db_free_pkg(pkg);
  g_free(root);
  g_free(name);
  g_free(shortname);
  return 0;

 err4:
  db_rem_pkg(name);
 err3:
  _debug("Rolling back...");
  g_free(sane_path);
  g_free(root);
  ta_rollback();
 err2:
  if (pkg)
    db_free_pkg(pkg);
  if (ipkg)
    db_free_pkg(ipkg);
  if (tgz)
    untgz_close(tgz);
 err1:
  g_free(name);
  g_free(shortname);
 err0:
  e_set(E_PASS,"Package upgrade failed!");
  return 1;
}
