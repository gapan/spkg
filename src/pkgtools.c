/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ond�ej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "pkgname.h"
#include "untgz.h"
#include "pkgdb.h"
#include "sys.h"
#include "taction.h"
#include "pkgtools.h"

/* private 
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(e, "pkgtools", __func__, n, fmt, ##args)

/* public 
 ************************************************************************/

gint pkg_install(const gchar* pkgfile, const struct pkg_options* opts, struct error* e)
{
  gchar *name, *shortname;
  struct untgz_state* tgz=0;
  struct db_pkg* pkg=0;
  gchar* doinst = 0;

  g_assert(pkgfile != 0);
  g_assert(opts != 0);
  g_assert(e != 0);

  /* check if file exist */
  if (sys_file_type(pkgfile,1) != SYS_REG)
  {
    e_set(E_ERROR|PKG_NOTEX,"package file does not exist (%s)", pkgfile);
    goto err0;
  }

  /* parse package name from the file path */
  if ((name = parse_pkgname(pkgfile,5)) == 0 
      || (shortname = parse_pkgname(pkgfile,1)) == 0)
  {
    e_set(E_ERROR|PKG_BADNAME,"package name is invalid (%s)", pkgfile);
    goto err0;
  }

  /* check if package is in the database */  
  pkg = db_get_pkg(name,0);
  if (pkg)
  {
    e_set(E_ERROR|PKG_EXIST,"package is already installed (%s)", name);
    db_free_pkg(pkg);
    goto err1;
  }
  if (! (e_errno(e) & DB_NOTEX))
  {
    e_set(E_ERROR,"internal error (%s)", name);
    goto err1;
  }
  e_clean(e);

  /* open tgz */
  tgz = untgz_open(pkgfile, 0, e);
  if (tgz == 0)
  {
    e_set(E_ERROR|PKG_NOTEX,"can't open package file (%s)", pkgfile);
    goto err1;
  }

  if (opts->verbose)
    printf("install: package file opened: %s\n", pkgfile);

  /* init transaction */
  if (ta_initialize(opts->dryrun, e))
  {
    e_set(E_ERROR,"can't initialize transaction");
    goto err2;
  }

  /* alloc package object */
  pkg = db_alloc_pkg(name);
  pkg->location = g_strdup(pkgfile);

  /* for each file in package */
  while (untgz_get_header(tgz) == 0)
  {
    /* check file path */
    if (tgz->f_name[0] == '/') /* XXX: what checks are neccessary? */
    {
      /* some damned fucker created this package to mess our system */
      e_set(E_ERROR|PKG_CORRUPT,"package contains files with absolute paths");
      goto err3;
    }

    /* check for metadata files */
    if (!strcmp(tgz->f_name, "install/slack-desc") || 
        !strcmp(tgz->f_name, "./install/slack-desc"))
    {
      gchar *buf, *desc[11] = {0};
      gsize len;
      gint i;
      
      untgz_write_data(tgz,&buf,&len);
      parse_slackdesc(buf,shortname,desc);
      pkg->desc = gen_slackdesc(shortname,desc);

      /* free description */
      for (i=0;i<11;i++)
      {
        if (opts->verbose)
          printf("install: %s\n", desc[i]);
        g_free(desc[i]);
      }  
      continue;
    }
    else if (!strcmp(tgz->f_name, "install/doinst.sh") ||
             !strcmp(tgz->f_name, "./install/doinst.sh"))
    {
      /* read doinst.sh into buffer XXX: check if it is not too big */
      gchar* buf;
      gsize len;
      untgz_write_data(tgz,&buf,&len);
      
      /* optimize out symlinks creation from doinst.sh */
      gchar *b, *e, *ln, *n=buf;
      while(iter_lines(&b, &e, &n, &ln))
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
          if (opts->verbose)
            printf("install[symlinking]: %s -> %s\n", path, target);
          pkg->files = g_slist_prepend(pkg->files, db_alloc_file(path, target));
        }
        /* if this is not 'delete old file' line... */
        else if (!parse_cleanuplink(ln))
        {
          /* ...append it to doinst buffer */
          gchar* nd;
          if (doinst)
            nd = g_strdup_printf("%s%s\n", doinst, ln);
          else
            nd = g_strdup_printf("%s\n", ln);
          g_free(doinst);
          doinst = nd;          
        }
        g_free(ln);
      }
      /* we always store full doinst.sh in database */
      pkg->doinst = buf;
      continue;
    }

    /* add file to db */
    pkg->files = g_slist_append(pkg->files, db_alloc_file(g_strdup(tgz->f_name), 0));

    /* following strings can be freed by the ta code, if so you must zero these
       variables after passing them to a ta_* */
    gchar* fullpath = g_strdup_printf("%s/%s", opts->root, tgz->f_name);
    gchar* temppath = g_strdup_printf("%s--###install###", fullpath);
    sys_ftype existing = sys_file_type(fullpath, 0);

    /* error handling in file installation code
     * ----------------------------------------
     * paranoid -
     * cautious - 
     * normal   -
     * brutal   -
     */

    /* preinstall file (installation will be finished by ta_finalize) */
    switch(tgz->f_type)
    {
      case UNTGZ_DIR: /* we have directory */
        if (existing == SYS_DIR)
        {
          if (opts->verbose)
            printf("install: #mkdir %s\n", tgz->f_name);
          /* installed directory already exist */
//          struct stat st;
//          lstat(fullpath,st);          
        }
        else if (existing == SYS_NONE)
        {
          if (opts->verbose)
            printf("install: mkdir %s\n", tgz->f_name);
          if (!opts->dryrun)
            if (untgz_write_file(tgz, fullpath))
              goto extract_failed;
          if (ta_keep_remove(fullpath, 1))
            goto transact_insert_failed;
          fullpath = 0;
        }
        else if (existing == SYS_ERR)
        {
          if (opts->verbose)
            printf("install[BUG]: stat failed %s\n", tgz->f_name);
          /*XXX: bug */
        }
        else
        {
          if (opts->verbose)
            printf("install[BUG]: can't mkdir over ordinary file %s\n", tgz->f_name);
          /*XXX: bug (ordinary file) */
        }
      break;
      case UNTGZ_SYM: /* wtf?, symlinks are not permitted to be in package */
        if (opts->verbose)
          printf("install[BUG]: symlink in archive %s\n", tgz->f_name);
        /* XXX: bug */
      break;
      case UNTGZ_LNK: /* hardlinks are special beasts, most easy solution is to 
        postpone hardlink creation into transaction finalization phase */
      {
        gchar* linkpath = g_strdup_printf("%s/%s", opts->root, tgz->f_link);
        if (opts->verbose)
          printf("install: hardlink found %s -> %s\n", tgz->f_name, tgz->f_link);
        if (ta_link_nothing(fullpath, linkpath))
          goto transact_insert_failed;
        fullpath = 0;
      }
      break;
      case UNTGZ_NONE:
        /*XXX: bug */
      break;
      default: /* ordinary file */
        if (existing == SYS_DIR)
        {
          if (opts->verbose)
            printf("install[BUG]: can't extract file over dir %s\n", tgz->f_name);
          /* target path is a directory, bad! */
        }
        else if (existing == SYS_NONE)
        {
          if (opts->verbose)
            printf("install: extracting: %s\n", tgz->f_name);
          if (!opts->dryrun)
            if (untgz_write_file(tgz, fullpath))
              goto extract_failed;
          if (ta_keep_remove(fullpath, 0))
            goto transact_insert_failed;
          fullpath = temppath = 0;
        }
        else if (existing == SYS_ERR)
        {
          if (opts->verbose)
            printf("install[BUG]: stat failed %s\n", tgz->f_name);
          /*XXX: bug */
        }
        else /* file already exist there */
        {
          /*XXX: here we may check file types, etc. */
          if (opts->verbose)
            printf("install: already exist: %s\n", tgz->f_name);
          if (!opts->dryrun)
            if (untgz_write_file(tgz, temppath))
              goto extract_failed;
          if (ta_move_remove(temppath, fullpath))
            goto transact_insert_failed;
          fullpath = temppath = 0;
        }
    }
    if (0) /* common error handling */
    {
     transact_insert_failed:
      e_set(E_ERROR|PKG_BADIO,"transaction insert failed for file %s (%s)", tgz->f_name, pkgfile);
      goto err3;
     extract_failed:
      e_set(E_ERROR|PKG_BADIO,"file extraction failed %s (%s)", tgz->f_name, pkgfile);
      goto err3;
    }
    g_free(temppath);
    g_free(fullpath);
  }
  
  /* error occured during extraction */
  if (!e_ok(e))
  {
    e_set(E_ERROR|PKG_CORRUPT,"package is corrupted (%s)", pkgfile);
    goto err3;
  }

  /* finalize transaction */
  ta_finalize();

  pkg->usize = tgz->usize/1024;
  pkg->csize = tgz->csize/1024;
  
  /* close tgz */
  untgz_close(tgz);
  tgz = 0;

  if (!opts->dryrun)
  {
    gchar* old_cwd = sys_setcwd(opts->root);
    if (old_cwd)
    {
#if 0
      /* run ldconfig */
      printf("install: running ldconfig\n");
      if (system("/sbin/ldconfig -r ."))
        printf("install: ldconfig failed\n");
#endif
      /* run doinst sh */
      if (sys_file_type("install/doinst.sh",0) == SYS_REG)
      {
        printf("install: running doinst.sh\n");
  //      if (system(". install/doinst.sh"))
  //        printf("install: doinst.sh failed\n");
      }
      sys_setcwd(old_cwd);
    }
  }

  /* add package to the database */
  if (!opts->dryrun)
  {
    if (opts->verbose)
      printf("install: updating database with package: %s\n", name);
    if (db_legacy_add_pkg(pkg))
    {
      e_set(E_ERROR|PKG_DB,"can't add package to the legacy database");
      goto err3;
    }
    if (db_add_pkg(pkg))
    {
      e_set(E_ERROR|PKG_DB,"can't add package to the database");
      goto err3;
    }
  }

  if (opts->verbose)
    printf("install: done installing package: %s\n", name);

  db_free_pkg(pkg);
  g_free(name);
  g_free(shortname);
  return 0;

 err3:
  ta_rollback();
  db_free_pkg(pkg);
 err2:
  untgz_close(tgz);
 err1:
  g_free(name);
  g_free(shortname);
 err0:
  return 1;
}

gint pkg_upgrade(const gchar* pkgfile, const struct pkg_options* opts, struct error* e)
{
  g_assert(pkgfile != 0);
  g_assert(opts != 0);
  g_assert(e != 0);
  e_set(E_FATAL,"command is not implemented");
  return 1;
}

gint pkg_remove(const gchar* pkgname, const struct pkg_options* opts, struct error* e)
{
  g_assert(pkgname != 0);
  g_assert(opts != 0);
  g_assert(e != 0);
  e_set(E_FATAL,"command is not implemented");
  return 1;
}
