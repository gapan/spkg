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

/* packages that can't be optimized, until they are fixed */
static gchar* blacklist[] = {
  "aaa_base",
  "bin",
  "glibc-solibs",
};

static gint blacklisted(gchar* shortname)
{
  gint i;
  for (i=0; i<sizeof(blacklist)/sizeof(blacklist[0]); i++)
    if (!strcmp(blacklist[i], shortname))
      return 1;
  return 0;
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
  gchar *name, *shortname;
  if ((name = parse_pkgname(pkgfile,5)) == 0 
      || (shortname = parse_pkgname(pkgfile,1)) == 0)
  {
    e_set(E_ERROR|CMD_BADNAME, "package name is invalid (%s)", pkgfile);
    goto err0;
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

  /* open package's tgz archive */
  struct untgz_state* tgz=0;
  tgz = untgz_open(pkgfile, 0, e);
  if (tgz == 0)
  {
    e_set(E_ERROR|CMD_NOTEX,"can't open package file (%s)", pkgfile);
    goto err1;
  }

  _notice("package file opened: %s", pkgfile);

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
  gchar* sane_path = 0;
  /* for each file in package */
  while (untgz_get_header(tgz) == 0)
  {
    _safe_breaking_point(err3);

    /* check file path */
    g_free(sane_path);
    sane_path = path_simplify(tgz->f_name);
    if (sane_path == 0 || sane_path[0] == '/' || !strncmp(sane_path, "../", 3))
    {
      /* some damned fucker created this package to mess our system */
      e_set(E_ERROR|CMD_CORRUPT,"package contains file with unsecure path: %s", tgz->f_name);
      goto err3;
    }

    /* add ./ */
    if (sane_path[0] == '\0')
    {
      db_add_file(pkg, "./", 0);
      g_free(sane_path);
      sane_path = 0;
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

      gchar *buf = NULL, *desc[11] = {0};
      gsize len;
      untgz_write_data(tgz, &buf, &len);
      parse_slackdesc(buf, shortname, desc);
      pkg->desc = gen_slackdesc(shortname, desc);
      g_free(buf);

      /* free description */
      gint i;
      for (i=0;i<11;i++)
      {
        _inform("| %s", desc[i]);
        g_free(desc[i]);
      }  
      continue;
    }
    else if (!strcmp(sane_path, "install/doinst.sh"))
    {
      if (tgz->f_size > 1024*512) /* 512K is enough for all. :) */
      {
        e_set(E_ERROR|CMD_CORRUPT, "doinst.sh file is too big (%d kB)", tgz->f_size/1024);
        goto err3;
      }

      gchar* fullpath = g_strdup_printf("%s/%s", opts->root, sane_path);
      if (opts->no_optsyms || blacklisted(shortname)) /* optimization disabled, just extract */
      {
        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, fullpath))
          {
            g_free(fullpath);
            e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkgfile);
            goto err3;
          }
        }
        g_free(fullpath);
        has_doinst = 1;
        continue;
      }

      /* read doinst.sh into buffer */
      gchar* buf;
      gsize len;
      untgz_write_data(tgz, &buf, &len);
      
      /* optimize out symlinks creation from doinst.sh */
      gchar *b, *end, *ln, *n = buf;
      gchar* doinst = 0;
      while(iter_lines(&b, &end, &n, &ln))
      { /* for each line */
        gchar* dir;
        gchar* link;
        gchar* target;
        /* create link line */
        if (parse_createlink(ln, &dir, &link, &target))
        {
          gchar* path = g_strdup_printf("%s/%s", dir, link);
          g_free(dir);
          g_free(link);
          _notice("detected symlink %s -> %s", path, target);
          db_add_file(pkg, path, target); /* target is freed by db_free_pkg() */

          gchar* fullpath = g_strdup_printf("%s/%s", opts->root, path);
          struct stat ex_stat;
          sys_ftype ex_type = sys_file_type_stat(fullpath, 0, &ex_stat);
          g_free(path);

          g_free(ln);
          ln = NULL;
          
          if (ex_type == SYS_ERR)
          {
            _warning("can't stat path for symlink %s", path);
            goto extract_failed;
          }
          else if (ex_type == SYS_DIR)
          {
            _warning("can't create symlink over directory %s", path);
            goto extract_failed;
          }
          else if (ex_type == SYS_NONE)
          {
            ta_symlink_nothing(fullpath, g_strdup(target));
            fullpath = NULL;
          }
          else
          {
            _warning("can't create symlink over existing file %s", path);
            goto extract_failed;
          }
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
        ln = NULL;
      } /* iter_lines */

      /* we always store full doinst.sh in database */
      pkg->doinst = buf;

      /* write stripped down version of doinst.sh to a file */
      if (doinst != 0)
      {
        if (!opts->dryrun)
        {
          if (sys_write_buffer_to_file(fullpath, doinst, (gsize)0, e))
          {
            e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkgfile);
            g_free(fullpath);
            goto err3;        
          }
        }
        g_free(doinst);
        has_doinst = 1;
      }
      
      g_free(fullpath);
      continue;
    }
    else if (!strncmp(sane_path, "install/", 8) && strcmp(sane_path, "install"))
      continue;

    /* following strings can be freed by the ta_* code, if so, you must zero
       variables after passing them to a ta_* function */
    gchar* fullpath = g_strdup_printf("%s/%s", opts->root, sane_path);
    gchar* temppath = g_strdup_printf("%s--###install###", fullpath);

    /* add file to the package */
    if (tgz->f_type == UNTGZ_DIR)
    {
      gchar* path = g_strdup_printf("%s/", sane_path);
      db_add_file(pkg, path, 0);
      g_free(path);
    }
    else
      db_add_file(pkg, sane_path, 0);

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

    /* get information about installed file from filesystem and filedb */
    struct stat ex_stat;
    sys_ftype ex_type = sys_file_type_stat(fullpath, 0, &ex_stat);
//    e_clean(e);

    if (ex_type == SYS_ERR)
    {
      _warning("lstat failed on path %s", sane_path);
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
          ta_chperm_nothing(fullpath, tgz->f_mode, tgz->f_uid, tgz->f_gid);
          fullpath = 0;
        }
        else if (ex_type == SYS_NONE)
        {
          _notice("mkdir %s", sane_path);
  
          if (!opts->dryrun)
            if (untgz_write_file(tgz, fullpath))
              goto extract_failed;
  
          ta_keep_remove(fullpath, 1);
          fullpath = 0;
        }
        else
        {
          _warning("can't mkdir over ordinary file %s", sane_path);
          goto extract_failed;
        }
      }
      break;
      case UNTGZ_SYM: /* wtf?, symlinks are not permitted to be in package */
      {
        _warning("symlink in archive %s", sane_path);
        goto extract_failed;
      }
      break;
      case UNTGZ_LNK: 
      { 
        /* hardlinks are special beasts, most easy solution is to 
         * postpone hardlink creation into transaction finalization phase 
         */
        gchar* linkpath = g_strdup_printf("%s/%s", opts->root, tgz->f_link);
        _notice("hardlink found %s -> %s (creation postponed)", sane_path, tgz->f_link);
        ta_link_nothing(fullpath, linkpath);
        fullpath = 0;
      }
      break;
      case UNTGZ_NONE:
        _warning("what's this? (%s)", sane_path);
        goto extract_failed;
      break;
      default: /* ordinary file */
        if (ex_type == SYS_DIR)
        {
          /* target path is a directory, bad! */
          _warning("can't extract file over existing directory %s", sane_path);
          goto extract_failed;
        }
        else if (ex_type == SYS_NONE)
        {
          _notice("extracting %s", sane_path);
  
          if (!opts->dryrun)
            if (untgz_write_file(tgz, fullpath))
              goto extract_failed;
  
          ta_keep_remove(fullpath, 0);
          fullpath = 0;
        }
        else /* file already exist there */
        {
          _warning("file already exist %s", sane_path);

          sys_ftype tmp_type = sys_file_type(temppath, 0);
          if (tmp_type == SYS_ERR)
          {
            _warning("can't lstat temporary file %s", temppath);
            goto extract_failed;
          }
          else if (tmp_type == SYS_DIR)
          {
            _warning("temporary file is a directory! %s", temppath);
            goto extract_failed;
          }
          else if (tmp_type != SYS_NONE)
          {
            _warning("found temporary file, removing %s", temppath);
            if (unlink(temppath) < 0)
            {
              _warning("removal failed %s: %s", temppath, strerror(errno));
              goto extract_failed;
            }
          }

          if (!opts->dryrun)
            if (untgz_write_file(tgz, temppath))
              goto extract_failed;
  
          ta_move_remove(temppath, fullpath);
          fullpath = temppath = 0;
        }
      break;
    }
    if (0) /* common error handling */
    {
     extract_failed:
      e_set(E_ERROR|CMD_BADIO,"file extraction failed %s (%s)", sane_path, pkgfile);
      goto err3;
    }
    g_free(temppath);
    g_free(fullpath);
  }
  g_free(sane_path);
  sane_path = 0;
  
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
  tgz = 0;

  if (!opts->dryrun && !opts->no_scripts && has_doinst)
  {
    gchar* old_cwd = sys_setcwd(opts->root);
    if (old_cwd)
    {
#if 0
      /* run ldconfig */
      _notice("running ldconfig");
      if (system("/sbin/ldconfig -r ."))
        _warning("ldconfig failed");
#endif
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

  if (!opts->dryrun)
  {
    gchar* install_path = g_strdup_printf("%s/install", opts->root);
    rmdir(install_path);
    g_free(install_path);
  }

  _notice("finished");

  db_free_pkg(pkg);
  g_free(sane_path);
  g_free(name);
  g_free(shortname);
  return 0;

 err4:
  db_rem_pkg(name);
 err3:
  _notice("rolling back");
  g_free(sane_path);
  ta_rollback();
  db_free_pkg(pkg);
 err2:
  _notice("closing package");
  untgz_close(tgz);
 err1:
  g_free(name);
  g_free(shortname);
 err0:
  _notice("installation terminated");
  return 1;
}
