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

#include "cmd-common.h"

/* private 
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(e, "install", __func__, n, fmt, ##args)

/* public 
 ************************************************************************/

gint cmd_install(const gchar* pkgfile, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgfile != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  /*
   - check if package file exists
   - check/parse package name format
   - get installed package by full name (check if not already installed)
   - get installed package by short name (check if older version not already installed)
   - open package tgz file
   - initialize transaction
   - prepare empty db_pkg obejct for newly installed package
   - for each file in the package:
     - sanitize path
     - handle special files (install/)
     - check on disk status of the file
     - install file if possible
   - update package database: install new package desc
   -NO- update filelist
   - finish transaction
   - close package file
   - run doinst.sh
   - run gtk-update-icon-cache in the hicolor icon dir
   - run ldconfig
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
  gchar* installed_pkgname = db_get_package_name(name);
  if (installed_pkgname)
  {
    e_set(E_ERROR|CMD_EXIST, "Package is already installed. (%s)", installed_pkgname);
    g_free(installed_pkgname);
    goto err1;
  }

  if (!opts->force)
  {
    installed_pkgname = db_get_package_name(shortname);
    if (installed_pkgname)
    {
      e_set(E_ERROR|CMD_EXIST, "Different package with same short name is already installed. (%s)", installed_pkgname);
      g_free(installed_pkgname);
      goto err1;
    }
  }

  _safe_breaking_point(err1);

  _inform("Installing package %s...", name);

  /* EXIT: free(name), free(shortname) */

  /* open package's tgz archive */
  struct untgz_state* tgz=0;
  tgz = untgz_open(pkgfile, e);
  if (tgz == 0)
  {
    e_set(E_ERROR,"Can't open package file. (%s)", pkgfile);
    goto err1;
  }

  /* EXIT: free(name), free(shortname), untgz_close(tgz) */

  /* init transaction */
  if (ta_initialize(opts->dryrun, e))
  {
    e_set(E_ERROR,"Can't initialize transaction.");
    goto err2;
  }

  /* alloc package object */
  struct db_pkg* pkg = db_alloc_pkg(name);
  if (pkg == NULL)
    goto err2;
  pkg->location = g_strdup(pkgfile);

  gboolean need_ldconfig = 0;
  gboolean need_update_icon_cache = 0;
  gboolean has_doinst = 0;
  gchar* sane_path = NULL;
  gchar* root = sanitize_root_path(opts->root);

  /* EXIT: free(name), free(shortname), untgz_close(tgz),
     ta_finalize/rollback(), free(root), db_free_pkg(pkg) */

  /* for each file in package */
  while (untgz_get_header(tgz) == 0)
  {
    _safe_breaking_point(err3);

    /* EXIT: free(name), free(shortname), untgz_close(tgz),
       ta_finalize/rollback(), free(root), db_free_pkg(pkg),
       free(sane_path) */

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
      if (tgz->f_size > 1024*1024*32) /* 32M is enough for all. :) now, really! */
      {
        e_set(E_ERROR, "Installation script is too big. (%ld kB)", tgz->f_size / 1024);
        goto err3;
      }
      has_doinst = _read_doinst_sh(tgz, pkg, root, sane_path, opts, e, FALSE, NULL);
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

    _extract_file(tgz, pkg, sane_path, root, opts, e, FALSE, NULL);
    if (!e_ok(e))
      goto err3;
  }
  g_free(sane_path);
  sane_path = NULL;

  /* EXIT: free(name), free(shortname), untgz_close(tgz),
     ta_finalize/rollback(), free(root), db_free_pkg(pkg) */
  
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
    if (db_add_pkg(pkg))
    {
      e_set(E_ERROR,"Can't add package to the database.");
      goto err3;
    }
    _safe_breaking_point(err4);
  }

  /* update filelist */
//  db_filelist_add_pkg_paths(pkg);

  /* finalize transaction */
  _debug("Finalizing transaction...");
  ta_finalize();

  /* close tgz */
  untgz_close(tgz);
  tgz = NULL;

  /* EXIT: free(name), free(shortname), free(root), db_free_pkg(pkg) */

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

  /* EXIT: free(name), free(shortname), free(root), db_free_pkg(pkg) */

  _debug("Package installation finished!");

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
  db_free_pkg(pkg);
 err2:
  untgz_close(tgz);
 err1:
  g_free(name);
  g_free(shortname);
 err0:
  e_set(E_PASS,"Package installation failed!");
  return 1;
}
