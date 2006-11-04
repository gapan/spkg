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

#include "misc.h"
#include "path.h"
#include "sys.h"
#include "taction.h"

#include "cmd-private.h"

/* private 
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(e, "install", __func__, n, fmt, ##args)

/* packages that can't be optimized, until they are fixed */

static gboolean _unsafe_path(const gchar* path)
{
  if (g_path_is_absolute(path))
    return TRUE;
  else if (!strncmp(path, "../", 3))
    return TRUE;
  return FALSE;
}

static void _read_slackdesc(struct untgz_state* tgz, struct db_pkg* pkg)
{
  gchar *buf = NULL, *desc[MAX_SLACKDESC_LINES] = {0};
  gsize len;
  untgz_write_data(tgz, &buf, &len);
  parse_slackdesc(buf, pkg->shortname, desc);
  pkg->desc = gen_slackdesc(pkg->shortname, desc);
  g_free(buf);

  /* free description */
  gint i;
  for (i=0;i<MAX_SLACKDESC_LINES;i++)
  {
    _inform("| %s", desc[i] ? desc[i] : "");
    g_free(desc[i]);
  }  
}

static gint _read_doinst_sh(struct untgz_state* tgz, struct db_pkg* pkg,
                            const gchar* root, const gchar* sane_path,
                            const struct cmd_options* opts, struct error* e)
{
  gint has_doinst = 0;
  gchar* fullpath = g_strdup_printf("%s%s", root, sane_path);

  /* optimization disabled, just extract doinst script */
  if (opts->no_optsyms || is_blacklisted(pkg->shortname, opts->bl_symopts))
  {
    _debug("Symlink optimization is disabled.");
    if (opts->safe)
    {
      _warning("In safe mode, install script is not executed,"
      " and with disabled optimized symlink creation, symlinks"
      " will not be created.%s", is_blacklisted(pkg->shortname, opts->bl_symopts) ?
      " Note that this package is blacklisted for optimized"
      " symlink creation. This may change in the future as"
      " better heuristics are developed for extracting symlink"
      " creation code from install script." : "");
    }
    else
    {
      if (!opts->dryrun)
      {
        if (untgz_write_file(tgz, fullpath))
        {
          e_set(E_ERROR, "Failed to extract file %s.", sane_path);
          goto err0;
        }
      }
      has_doinst = 1;
    }
    goto done;
  }

  /* EXIT: free(fullpath) */

  /* read doinst.sh into buffer */
  gchar* buf;
  gsize len;
  if (untgz_write_data(tgz, &buf, &len))
  {
    e_set(E_ERROR, "Failed to read post-installation script into buffer.");
    goto err0;
  }

  /* EXIT: free(fullpath), free(buf) */

  _debug("Extracting symlinks from the post-installation script...");
      
  /* optimize out symlinks creation from doinst.sh */
  GSList *doinst = NULL;
  gsize doinst_len = 0;
  gchar *b, *end, *ln = NULL, *n = buf, *sane_link_path = NULL,
        *link_target = NULL, *link_fullpath = NULL;
  while(iter_str_lines(&b, &end, &n, &ln))
  { /* for each line */
    gchar *link_dir, *link_name;

    /* EXIT: free(fullpath), free(buf), free(ln) */

    /* create link line */
    if (parse_createlink(ln, &link_dir, &link_name, &link_target))
    {
      struct stat ex_stat;
      gchar* link_path = g_strdup_printf("%s/%s", link_dir, link_name);
      sane_link_path = path_simplify(link_path);
      link_fullpath = g_strdup_printf("%s%s", root, sane_link_path);
      g_free(link_dir);
      g_free(link_name);
      g_free(link_path);
      
      /* EXIT: free(fullpath), free(buf), free(ln), free(link_target),
         free(sane_link_path), free(link_fullpath) */

      /* link path checks (be careful here) */
      if (_unsafe_path(sane_link_path))
      {
        e_set(E_ERROR, "Package contains symlink with unsafe path. (%s)", sane_link_path);
        goto parse_failed;
      }

      _debug("Symlink detected: %s -> %s", sane_link_path, link_target);
      if (db_pkg_add_path(pkg, sane_link_path, DB_PATH_SYMLINK))
      {
        e_set(E_ERROR, "Can't add path to the package, it's too long. (%s)", sane_link_path);
        goto parse_failed;
      }
      sys_ftype ex_type = sys_file_type_stat(link_fullpath, 0, &ex_stat);

      if (ex_type == SYS_ERR)
      {
        e_set(E_ERROR, "Can't check path for symlink. (%s)", sane_link_path);
        goto parse_failed;
      }
      else if (ex_type == SYS_NONE)
      {
        ta_symlink_nothing(link_fullpath, g_strdup(link_target)); /* target is freed by db_free_pkg(), dup by taction finalize/rollback */
        link_fullpath = NULL;
      }
      else
      {
        if (opts->safe)
        {
          e_set(E_ERROR, "Can't create symlink over existing %s. (%s)", ex_type == SYS_DIR ? "directory" : "file", sane_link_path);
          goto parse_failed;
        }
        else
        {
          _warning("%s exists, where symlink should be created. It will be removed. (%s)", ex_type == SYS_DIR ? "Directory" : "File", sane_link_path);
          ta_forcesymlink_nothing(link_fullpath, g_strdup(link_target));
          link_fullpath = NULL;
        }
      }

      g_free(link_fullpath);
      g_free(sane_link_path);
    }
    /* if this is not 'delete old file' line... */
    else if (!parse_cleanuplink(ln))
    {
      /* ...append it to doinst buffer */
      doinst = g_slist_prepend(doinst, ln);
      doinst_len += end - b + 1; /* line len + eol */
      ln = NULL; /* will be freed later */
    }

    g_free(ln);
  } /* iter_lines */

  /* EXIT: free(fullpath), free(buf), free(doinst) */

  /* we always store full doinst.sh in database (buf freed by db_free_pkg()) */
  pkg->doinst = buf;

  /* EXIT: free(fullpath), free(doinst) */

  /* write stripped down version of doinst.sh to a file */
  if (doinst != NULL)
  {
    _debug("Saving optimized post-installation script...");

    if (!opts->dryrun)
    {
      gchar* doinst_buf = g_malloc(doinst_len*2);
      gchar* doinst_buf_ptr = doinst_buf;
      doinst = g_slist_reverse(doinst);
      GSList* i;
      for (i = doinst; i != 0; i = i->next)
      {
        strcpy(doinst_buf_ptr, i->data);
        doinst_buf_ptr += strlen(i->data);
        *doinst_buf_ptr = '\n';
        doinst_buf_ptr++;
      }
      if (sys_write_buffer_to_file(fullpath, doinst_buf, doinst_len, e))
      {
        e_set(E_ERROR, "Can't write buffer to file %s.", sane_path);
        g_free(doinst_buf);
        g_slist_foreach(doinst, (GFunc)g_free, 0);
        g_slist_free(doinst);
        goto err0;
      }
      g_free(doinst_buf);
    }
    g_slist_foreach(doinst, (GFunc)g_free, 0);
    g_slist_free(doinst);
    has_doinst = 1;
  }

 done:  
 err0:
  g_free(fullpath);
  return has_doinst;
 parse_failed:
  g_free(link_target);
  g_free(link_fullpath);
  g_free(sane_link_path);
  g_free(ln);
  goto err0;
}

static int _untgz_to_db_file_type(int type)
{
  if (type == UNTGZ_DIR)
    return DB_PATH_DIR;
  else if (type == UNTGZ_SYM)
    return DB_PATH_SYMLINK;
  return DB_PATH_FILE;
}

static void _extract_file(struct untgz_state* tgz, struct db_pkg* pkg,
                   const gchar* sane_path, const gchar* root,
                   const struct cmd_options* opts, struct error* e)
{
  /* following strings can be freed by the ta_* code, if so, you must zero
     variables after passing them to a ta_* function */
  gchar* fullpath = g_strdup_printf("%s%s", root, sane_path);
  gchar* temppath = g_strdup_printf("%s--###install###", fullpath);

  /* EXIT: free(fullpath), free(temppath) */

  /* add file to the package */
  if (db_pkg_add_path(pkg, sane_path, _untgz_to_db_file_type(tgz->f_type)))
  {
    e_set(E_ERROR, "Can't add path to the package, it's too long. (%s)", sane_path);
    goto extract_failed;
  }

  /* Here we must check interaction of following conditions:
   *
   * - type of the file we are installing (tgz->f_type)
   * - type of the file on the filesystem (existing)
   * - presence of the file in the file database
   *
   * And decide what to do:
   *
   * - install new file
   * - overwrite existing file
   * - leave existing file alone
   * - issue error and rollback installation
   * - issue notice or warning
   */

  /* EXIT: free(fullpath), free(temppath) */

  /* get information about installed file from filesystem and filedb */
  struct stat ex_stat;
  sys_ftype ex_type = sys_file_type_stat(fullpath, 0, &ex_stat);
  sys_ftype ex_deref_type = SYS_NONE;
  if (ex_type == SYS_SYM)
    ex_deref_type = sys_file_type(fullpath, 1);

  if (ex_type == SYS_ERR)
  {
    e_set(E_ERROR, "Can't check path. (%s)", sane_path);
    goto extract_failed;
  }
  
  // installed file type
  switch (tgz->f_type)
  {
    case UNTGZ_DIR: /* we have directory */
    {
      if (ex_type == SYS_DIR)
      {
        /* installed directory already exist */
        if (_mode_differ(tgz, &ex_stat) || _gid_or_uid_differ(tgz, &ex_stat))
        {
          _notice("Directory already exists %s (but permissions differ)", sane_path);
          if (!opts->safe)
          {
            _notice("Permissions will be changed to owner=%d, group=%d, mode=%03o.", tgz->f_uid, tgz->f_gid, tgz->f_mode, sane_path);
            ta_chperm_nothing(fullpath, tgz->f_mode, tgz->f_uid, tgz->f_gid);
            fullpath = NULL;
          }
          else
          {
            e_set(E_ERROR, "Can't change existing directory permissions in safe mode. (%s)", sane_path);
            goto extract_failed;
          }
        }
        else
        {
          _debug("Direcory already exists %s", sane_path);
        }
      }
      else if (ex_type == SYS_SYM && ex_deref_type == SYS_DIR)
      {
        _warning("Direcory already exists *behind the symlink* on filesystem. This may break upgrade/remove if you change that symlink in the future. (%s)", sane_path);
      }
      else if (ex_type == SYS_NONE)
      {
        _notice("Creating directory %s", sane_path);

        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;
        }

        ta_keep_remove(fullpath, 1);
        fullpath = NULL;
      }
      else
      {
        e_set(E_ERROR, "Can't create directory over ordinary file. (%s)", sane_path);
        goto extract_failed;
      }
    }
    break;
    case UNTGZ_SYM: /* wtf?, symlinks are not permitted to be in package */
    {
      e_set(E_ERROR, "Symlink was found in the archive. (%s)", sane_path);
      goto extract_failed;
    }
    break;
    case UNTGZ_LNK: 
    { 
      /* hardlinks are special beasts, most easy solution is to 
       * postpone hardlink creation into transaction finalization phase 
       */
      gchar* linkpath = g_strdup_printf("%s%s", root, tgz->f_link);

      /* check file we will be linking to (it should be a regular file) */
      sys_ftype tgt_type = sys_file_type(linkpath, 0);
      if (tgt_type == SYS_ERR)
      {
        e_set(E_ERROR, "Can't check hardlink target path. (%s)", linkpath);
        g_free(linkpath);
        goto extract_failed;
      }
      else if (tgt_type == SYS_REG)
      {
        _debug("Hardlink detected: %s -> %s", sane_path, tgz->f_link);

        /* when creating hardlink, nothing must exist on created path */
        if (ex_type == SYS_NONE)
        {
          ta_link_nothing(fullpath, linkpath); /* link path freed by ta_* */
          fullpath = NULL;
        }
        else
        {
          if (opts->safe)
          {
            e_set(E_ERROR, "Can't create hardlink over existing %s. (%s)", ex_type == SYS_DIR ? "directory" : "file", sane_path);
            g_free(linkpath);
            goto extract_failed;
          }
          else
          {
            _warning("%s exists, where hardlink should be created. It will be removed. (%s)", ex_type == SYS_DIR ? "Directory" : "File", sane_path);
            ta_forcelink_nothing(fullpath, linkpath);
            fullpath = NULL;
          }
        }
      }
      else
      {
        if (!opts->dryrun)
        {
          e_set(E_ERROR, "Hardlink target is NOT a regular file. (%s)", linkpath);
          g_free(linkpath);
          goto extract_failed;
        }
        g_free(linkpath);
      }
    }
    break;
    case UNTGZ_NONE:
    {
      e_set(E_ERROR, "Broken tar archive.");
      goto extract_failed;
    }
    break;
    default: /* ordinary file */
    {
      _notice("Installing file %s", sane_path);

      if (ex_type == SYS_DIR)
      {
        /* target path is a directory, bad! */
        e_set(E_ERROR, "Can't create file over existing directory. (%s)", sane_path);
        goto extract_failed;
      }
      else if (ex_type == SYS_NONE)
      {
        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;
        }

        ta_keep_remove(fullpath, 0);
        fullpath = NULL;
      }
      else /* file already exist there */
      {
        if (opts->safe)
        {
          e_set(E_ERROR, "File already exist. (%s)", sane_path);
          goto extract_failed;
        }

        _warning("File already exist %s (it will be replaced)", sane_path);

        sys_ftype tmp_type = sys_file_type(temppath, 0);
        if (tmp_type == SYS_ERR)
        {
          e_set(E_ERROR, "Can't check temporary file path. (%s)", temppath);
          goto extract_failed;
        }
        else if (tmp_type == SYS_DIR)
        {
          e_set(E_ERROR, "Temporary file path is used by a direcotry. (%s)", temppath);
          goto extract_failed;
        }
        else if (tmp_type != SYS_NONE)
        {
          _warning("Temporary file already exists, removing %s", temppath);
          if (!opts->dryrun)
          {
            if (unlink(temppath) < 0)
            {
              e_set(E_ERROR, "Can't remove temporary file %s. (%s)", temppath, strerror(errno));
              goto extract_failed;
            }
          }
        }

        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, temppath))
            goto extract_failed;
        }

        ta_move_remove(temppath, fullpath);
        fullpath = temppath = NULL;
      }
    }
    break;
  }

 done:
  g_free(temppath);
  g_free(fullpath);
  return;
 extract_failed:
  e_set(E_ERROR, "Failed to extract file %s.", sane_path);
  goto done;
}

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
  installed_pkgname = db_get_package_name(shortname);
  if (installed_pkgname)
  {
    e_set(E_ERROR|CMD_EXIST, "Different package with same short name is already installed. (%s)", installed_pkgname);
    g_free(installed_pkgname);
    goto err1;
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

    /* check for metadata files */
    if (!strcmp(sane_path, "install/slack-desc"))
    {
      if (tgz->f_size > 1024*4) /* 4K is enough */
      {
        e_set(E_ERROR, "Package description file is too big. (%ld kB)", tgz->f_size / 1024);
        goto err3;
      }
      _read_slackdesc(tgz, pkg);
      continue;
    }
    else if (!strcmp(sane_path, "install/doinst.sh"))
    {
      if (tgz->f_size > 1024*512) /* 512K is enough for all. :) */
      {
        e_set(E_ERROR, "Installation script is too big. (%ld kB)", tgz->f_size / 1024);
        goto err3;
      }
      has_doinst = _read_doinst_sh(tgz, pkg, root, sane_path, opts, e);
      if (!e_ok(e))
      {
        e_set(E_ERROR, "Installation script processing failed.");
        goto err3;
      }
      continue;
    }
    else if (!strncmp(sane_path, "install/", 8) && strcmp(sane_path, "install"))
      continue;

    _extract_file(tgz, pkg, sane_path, root, opts, e);
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
  {
    if (access("/sbin/ldconfig", X_OK) == 0)
    {
      gchar* ldconf_file = g_strdup_printf("%setc/ld.so.conf", root);
      if (access(ldconf_file, R_OK) == 0)
      {
        gchar* qroot = g_shell_quote(root);
        gchar* cmd = g_strdup_printf("/sbin/ldconfig -r %s", qroot);
        g_free(qroot);

        _notice("Running /sbin/ldconfig...");
        if (!opts->dryrun)
        {
          gint rv = system(cmd);
          if (rv < 0)
            _warning("Can't execute /sbin/ldconfig. (%s)", strerror(errno));
          else if (rv > 0)
            _warning("Program /sbin/ldconfig failed. (%d)", rv);
        }

        g_free(cmd);
      }
      g_free(ldconf_file);
    }
    else
    {
      _warning("Program /sbin/ldconfig was not found on the system.");
    }
  }

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
