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
static gchar* _doinst_opt_blacklist[] = {
  "aaa_base",
  "bin",
  "glibc-solibs",
  "glibc",
  NULL
};

static gboolean _blacklisted(const gchar* shortname, gchar** blacklist)
{
  gchar** i = blacklist;
  while (*i)
  {
    if (!strcmp(*i, shortname))
      return TRUE;
    i++;
  }
  return FALSE;
}

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
  gchar *buf = NULL, *desc[11] = {0};
  gsize len;
  untgz_write_data(tgz, &buf, &len);
  parse_slackdesc(buf, pkg->shortname, desc);
  pkg->desc = gen_slackdesc(pkg->shortname, desc);
  g_free(buf);

  /* free description */
  gint i;
  for (i=0;i<11;i++)
  {
    _notice("| %s", desc[i]);
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
  if (opts->no_optsyms || _blacklisted(pkg->shortname, _doinst_opt_blacklist))
  {
    if (opts->safe)
    {
      _warning("In safe mode, install script is not executed,"
      " and with disabled optimized symlink creation, symlinks"
      " will not be created.%s", _blacklisted(pkg->shortname, _doinst_opt_blacklist) ?
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
          e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkg->location);
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
    e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkg->location);
    goto err0;
  }

  /* EXIT: free(fullpath), free(buf) */
      
  /* optimize out symlinks creation from doinst.sh */
  gchar *doinst = NULL;
  gchar *b, *end, *ln = NULL, *n = buf, *sane_link_path = NULL,
        *link_target = NULL, *link_fullpath = NULL;
  while(iter_lines(&b, &end, &n, &ln))
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
        e_set(E_ERROR, "detected symlink with unsafe path: %s", sane_link_path);
        goto parse_failed;
      }

      _notice("detected symlink %s -> %s", sane_link_path, link_target);
      db_add_file(pkg, sane_link_path, link_target); /* target is freed by db_free_pkg() */
      sys_ftype ex_type = sys_file_type_stat(link_fullpath, 0, &ex_stat);

      if (ex_type == SYS_ERR)
      {
        e_set(E_ERROR, "can't stat path for symlink %s", sane_link_path);
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
          e_set(E_ERROR, "can't create symlink over existing %s %s", ex_type == SYS_DIR ? "directory" : "file", sane_link_path);
          goto parse_failed;
        }
        else
        {
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
      /*XXX: this is a stupid braindead hack */
      gchar* nd;
      if (doinst)
        nd = g_strdup_printf("%s%s\n", doinst, ln);
      else
        nd = g_strdup_printf("%s\n", ln);
      g_free(doinst);
      doinst = nd;          
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
    if (!opts->dryrun)
    {
      if (sys_write_buffer_to_file(fullpath, doinst, 0, e))
      {
        e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkg->location);
        goto err0;
      }
    }
    g_free(doinst);
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

void _extract_file(struct untgz_state* tgz, struct db_pkg* pkg,
                   const gchar* sane_path, const gchar* root,
                   const struct cmd_options* opts, struct error* e)
{
  /* following strings can be freed by the ta_* code, if so, you must zero
     variables after passing them to a ta_* function */
  gchar* fullpath = g_strdup_printf("%s%s", root, sane_path);
  gchar* temppath = g_strdup_printf("%s--###install###", fullpath);

  /* EXIT: free(fullpath), free(temppath) */

  /* add file to the package */
  if (tgz->f_type == UNTGZ_DIR)
  {
    gchar* path = g_strdup_printf("%s/", sane_path);
    db_add_file(pkg, path, NULL);
    g_free(path);
  }
  else
    db_add_file(pkg, sane_path, NULL);

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

  if (ex_type == SYS_ERR)
  {
    e_set(E_ERROR, "lstat failed on path %s", sane_path);
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
        _notice("installed direcory already exists %s", sane_path);
        if (_mode_differ(tgz, &ex_stat) || _gid_or_uid_differ(tgz, &ex_stat))
        {
          _warning("directory already exists, but with different permissions %s", sane_path);
          if (!opts->safe)
          {
            _warning("forcing new permissions: %d:%d %03o", tgz->f_uid, tgz->f_gid, tgz->f_mode, sane_path);
            ta_chperm_nothing(fullpath, tgz->f_mode, tgz->f_uid, tgz->f_gid);
            fullpath = NULL;
          }
          else
          {
            e_set(E_ERROR, "can't change existing directory permissions in safe mode %s", sane_path);
            goto extract_failed;
          }
        }
      }
      else if (ex_type == SYS_NONE)
      {
        _notice("mkdir %s", sane_path);

        if (!opts->dryrun)
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;

        ta_keep_remove(fullpath, 1);
        fullpath = NULL;
      }
      else
      {
        e_set(E_ERROR, "can't mkdir over ordinary file %s", sane_path);
        goto extract_failed;
      }
    }
    break;
    case UNTGZ_SYM: /* wtf?, symlinks are not permitted to be in package */
    {
      e_set(E_ERROR, "symlink in archive %s", sane_path);
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
        e_set(E_ERROR, "can't lstat link target file %s", linkpath);
        g_free(linkpath);
        goto extract_failed;
      }
      else if (tgt_type == SYS_REG)
      {
        _notice("hardlink %s -> %s (postponed)", sane_path, tgz->f_link);

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
            e_set(E_ERROR, "can't create hardlink over existing %s %s", ex_type == SYS_DIR ? "directory" : "file", sane_path);
            g_free(linkpath);
            goto extract_failed;
          }
          else
          {
            ta_forcelink_nothing(fullpath, linkpath);
            fullpath = NULL;
          }
        }
      }
      else
      {
        e_set(E_ERROR, "hardlink target must be a regular file %s", linkpath);
        g_free(linkpath);
        goto extract_failed;
      }
    }
    break;
    case UNTGZ_NONE:
    {
      e_set(E_ERROR, "what's this? (%s)", sane_path);
      goto extract_failed;
    }
    break;
    default: /* ordinary file */
    {
      if (ex_type == SYS_DIR)
      {
        /* target path is a directory, bad! */
        e_set(E_ERROR, "can't extract file over existing directory %s", sane_path);
        goto extract_failed;
      }
      else if (ex_type == SYS_NONE)
      {
        _notice("extracting %s", sane_path);

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
          e_set(E_ERROR, "file already exist %s", sane_path);
          goto extract_failed;
        }

        _notice("extracting %s", sane_path);

        sys_ftype tmp_type = sys_file_type(temppath, 0);
        if (tmp_type == SYS_ERR)
        {
          e_set(E_ERROR, "can't lstat temporary file %s", temppath);
          goto extract_failed;
        }
        else if (tmp_type == SYS_DIR)
        {
          e_set(E_ERROR, "temporary file is a directory! %s", temppath);
          goto extract_failed;
        }
        else if (tmp_type != SYS_NONE)
        {
          _warning("found temporary file, removing %s", temppath);
          if (unlink(temppath) < 0)
          {
            e_set(E_ERROR, "removal failed %s: %s", temppath, strerror(errno));
            goto extract_failed;
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
  e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkg->location);
  goto done;
}

/* public 
 ************************************************************************/

gint cmd_install(const gchar* pkgfile, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgfile != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  msg_setup("install", opts->verbosity);
  _inform("installing package %s", pkgfile);

  /* check if file exist and is regular file */
  if (sys_file_type(pkgfile,1) != SYS_REG)
  {
    e_set(E_ERROR|CMD_NOTEX, "package file does not exist (%s)", pkgfile);
    goto err0;
  }

  /* parse package name from the file path */
  gchar *name = NULL, *shortname = NULL;
  if ((name = parse_pkgname(pkgfile,5)) == 0 
      || (shortname = parse_pkgname(pkgfile,1)) == 0)
  {
    e_set(E_ERROR|CMD_BADNAME, "package name is invalid (%s)", pkgfile);
    goto err1;
  }

  _safe_breaking_point(err1);

  /* check if package is already in the database */  
  struct db_pkg* pkg=0;
  pkg = db_get_pkg(name, DB_GET_WITHOUT_FILES);
  if (pkg)
  {
    e_set(E_ERROR|CMD_EXIST, "package is already installed (%s)", name);
    db_free_pkg(pkg);
    goto err1;
  }
  if (! (e_errno(e) & DB_NOTEX))
  { /* if error was not because of nonexisting package, then terminate */
    e_set(E_ERROR, "internal error (%s)", name);
    goto err1;
  }
  e_clean(e); /* cleanup error object */

  _safe_breaking_point(err1);

  /*EXIT: free:name, free:shortname */

  /* open package's tgz archive */
  struct untgz_state* tgz=0;
  tgz = untgz_open(pkgfile, 0, e);
  if (tgz == 0)
  {
    e_set(E_ERROR|CMD_NOTEX,"can't open package file (%s)", pkgfile);
    goto err1;
  }

  _notice("package file opened: %s", pkgfile);

  /* EXIT: free(name), free(shortname), untgz_close(tgz) */

  /* init transaction */
  if (ta_initialize(opts->dryrun, e))
  {
    e_set(E_ERROR,"can't initialize transaction");
    goto err2;
  }

  /* alloc package object */
  pkg = db_alloc_pkg(name);
  pkg->location = g_strdup(pkgfile);

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
    if (sane_path == 0 || _unsafe_path(sane_path))
    {
      /* some damned fucker created this package to mess our system */
      e_set(E_ERROR|CMD_CORRUPT,"package contains file with unsafe path: %s", tgz->f_name);
      goto err3;
    }

    /* check for ./ */
    if (sane_path[0] == '\0')
    {
      db_add_file(pkg, "./", 0);
      continue;
    }

    /* check for metadata files */
    if (!strcmp(sane_path, "install/slack-desc"))
    {
      if (tgz->f_size > 1024*4) /* 4K is enough */
      {
        e_set(E_ERROR|CMD_CORRUPT, "slack-desc file is too big (%d kB)", tgz->f_size/1024);
        goto err3;
      }
      _read_slackdesc(tgz, pkg);
      continue;
    }
    else if (!strcmp(sane_path, "install/doinst.sh"))
    {
      if (tgz->f_size > 1024*512) /* 512K is enough for all. :) */
      {
        e_set(E_ERROR|CMD_CORRUPT, "doinst.sh file is too big (%d kB)", tgz->f_size/1024);
        goto err3;
      }
      has_doinst = _read_doinst_sh(tgz, pkg, root, sane_path, opts, e);
      if (!e_ok(e))
      {
        e_set(E_ERROR|CMD_CORRUPT, "doinst.sh file parsing failed");
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
    e_set(E_ERROR|CMD_CORRUPT,"package is corrupted (%s)", pkgfile);
    goto err3;
  }

  pkg->usize = tgz->usize/1024;
  pkg->csize = tgz->csize/1024;

  /* add package to the database */
  _notice("updating legacy database");
  if (!opts->dryrun)
  {
    if (db_add_pkg(pkg))
    {
      e_set(E_ERROR|CMD_DB,"can't add package to the database");
      goto err3;
    }
    _safe_breaking_point(err4);
  }

  /* update filelist */
  _notice("adding files to filelist...");
  db_filelist_add_pkg_files(pkg);

  /* finalize transaction */
  _notice("finalizing transaction");
  ta_finalize();

  /* close tgz */
  _notice("closing package");
  untgz_close(tgz);
  tgz = NULL;

  /* EXIT: free(name), free(shortname), free(root), db_free_pkg(pkg) */

  if (!opts->dryrun && !opts->no_scripts && has_doinst && !opts->safe)
  {
    gchar* old_cwd = sys_setcwd(root);
    if (old_cwd)
    {
      /* run doinst sh */
      if (sys_file_type("install/doinst.sh", 0) == SYS_REG)
      {
        _notice("running doinst.sh");
        if (system(". install/doinst.sh"))
          _warning("doinst.sh failed");
        unlink("install/doinst.sh");
      }
      sys_setcwd(old_cwd);
    }
  }

  /* run ldconfig */
  if (!opts->no_ldconfig)
  {
    if (access("/sbin/ldconfig", X_OK) == 0)
    {
      gchar* ldconf_file = g_strdup_printf("%setc/ld.so.conf", root);
      if (access(ldconf_file, R_OK) == 0)
      {
        gchar* qroot = g_shell_quote(root);
        gchar* cmd = g_strdup_printf("/sbin/ldconfig -r %s", qroot);

        _notice("running /sbin/ldconfig -r %s", qroot);
        if (!opts->dryrun)
        {
          if (system(cmd))
            _warning("ldconfig failed");
        }

        g_free(cmd);
        g_free(qroot);
      }
      g_free(ldconf_file);
    }
    else
    {
      _warning("/sbin/ldconfig was not found");
    }
  }

  if (!opts->dryrun)
  {
    gchar* install_path = g_strdup_printf("%sinstall", root);
    rmdir(install_path);
    g_free(install_path);
  }

  /* EXIT: free(name), free(shortname), free(root), db_free_pkg(pkg) */

  _notice("finished");

  db_free_pkg(pkg);
  g_free(root);
  g_free(name);
  g_free(shortname);
  return 0;

 err4:
  db_rem_pkg(name);
 err3:
  _notice("rolling back");
  g_free(sane_path);
  g_free(root);
  ta_rollback();
  db_free_pkg(pkg);
 err2:
  _notice("closing package");
  untgz_close(tgz);
 err1:
  g_free(name);
  g_free(shortname);
 err0:
  e_set(E_ERROR,"package installation terimanted (%s)", pkgfile);
  _notice("installation terminated");
  return 1;
}
