/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
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

static gchar* _pkg_errstr = 0;

static __inline__ void _pkg_reset_error()
{
  if (G_UNLIKELY(_pkg_errstr != 0))
  {
    g_free(_pkg_errstr);
    _pkg_errstr = 0;
  }
}

static void _pkg_set_error(const gchar* fmt, ...)
{
  va_list ap;

  _pkg_reset_error();
  va_start(ap, fmt);
  _pkg_errstr = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  _pkg_errstr = g_strdup_printf("error[pkg]: %s", _pkg_errstr);
}

/* public 
 ************************************************************************/

gchar* pkg_error()
{
  return _pkg_errstr;
}

/* install steps:
 * - checks (pkg file exists, pkg name is valid, pkg is not in db)
 * - open untgz
 * - initialize file transaction
 * - for each file in pkg do:
 *   - check path for validity (not an absolute path, etc.)
 *   - check for special file (install/doinst.sh, etc.)
 *   - push file or dir to transaction log
 * - finalize file transaction
 */

gint pkg_install(const gchar* pkgfile, const gchar* root, gboolean dryrun, gboolean verbose)
{
  gchar *name, *shortname;
  struct untgz_state* tgz=0;
  struct db_pkg* pkg=0;

  _pkg_reset_error();

  /* check if file exist */
  if (sys_file_type(pkgfile,1) != SYS_REG)
  {
    _pkg_set_error("installation failed: package file does not exist (%s)", pkgfile);
    goto err0;
  }

  /* parse package name from the file path */
  if ((name = parse_pkgname(pkgfile,5)) == 0 
      || (shortname = parse_pkgname(pkgfile,1)) == 0)
  {
    _pkg_set_error("installation failed: package name is invalid (%s)", pkgfile);
    goto err0;
  }

  /* check if package is in the database */  
  pkg = db_get_pkg(name,0);
  if (pkg)
  {
    _pkg_set_error("installation failed: package is already installed (%s)", name);
    db_free_pkg(pkg);
    goto err1;
  }
  if (db_errno() != DB_NOTEX)
  {
    _pkg_set_error("installation failed: db_get_pkg failed (%s)\n%s", name, db_error());
    goto err1;
  }

  /*XXX: check for shortname match (maybe) */

  /* open tgz */
  tgz = untgz_open(pkgfile, 0);
  if (tgz == 0)
  {
    _pkg_set_error("installation failed: can't open package file (%s)", pkgfile);
    goto err1;
  }

  if (verbose)
    printf("install: package file opened: %s\n", pkgfile);

  /* init transaction */
  if (!dryrun)
  {
    if (ta_initialize())
    {
      _pkg_set_error("installation failed: can't initialize transaction");
      goto err2;
    }
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
      _pkg_set_error("installation failed: package contains files with absolute paths");
      goto err3;
    }

    /* check for metadata files */
    if (!strcmp(tgz->f_name, "install/slack-desc"))
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
        if (verbose)
          printf("install: %s\n", desc[i]);
        g_free(desc[i]);
      }  
      continue;
    }
    else if (!strcmp(tgz->f_name, "install/doinst.sh"))
    {
      gchar* buf;
      gsize len;
      untgz_write_data(tgz,&buf,&len);
      gchar* doinst = 0;
      
      gchar *b, *e, *ln, *n=buf;
      while(iter_lines(&b, &e, &n, &ln))
      { /* for each line */
        gchar* dir;
        gchar* link;
        gchar* target;
        if (parse_createlink(ln, &dir, &link, &target))
        {
          gchar* path = g_strdup_printf("%s/%s", dir, link);
          g_free(dir);
          g_free(link);
          if (verbose)
            printf("install[symlinking]: %s -> %s\n", path, target);
          pkg->files = g_slist_prepend(pkg->files, db_alloc_file(path, target));
        }
        else if (!parse_cleanuplink(ln))
        {
          /* append doinst.sh buffer */
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
      pkg->doinst = doinst;
      continue;
    }

    gchar* path = g_strdup_printf("%s/%s", root, tgz->f_name);
//    gchar* spath = g_strdup_printf("%s/%s.install.%04x", root, tgz->f_name, stamp);
    pkg->files = g_slist_append(pkg->files, db_alloc_file(g_strdup(tgz->f_name), 0));

    if (verbose)
      printf("install[extracting]: %s\n", path);

    if (!dryrun)
    {
      if (tgz->f_type == UNTGZ_DIR)
      {
        if (access(path, F_OK) != 0)
          ta_add_action(TA_MOVE,path,0);
        untgz_write_file(tgz,path);
      }
      else
      {
        ta_add_action(TA_MOVE,path,0);
        untgz_write_file(tgz,path);
      }
    }
    g_free(path);
//    g_free(spath);
  }
  
  /* error occured during extraction */
  if (untgz_error(tgz))
  {
    _pkg_set_error("%s\ninstallation failed: package is corrupted (%s)", untgz_error(tgz), pkgfile);
    goto err3;
  }

  /* finalize transaction */
  if (!dryrun)
    ta_finalize();

  pkg->usize = tgz->usize/1024;
  pkg->csize = tgz->csize/1024;
  
  /* close tgz */
  untgz_close(tgz);
  tgz = 0;

  gchar* old_cwd = sys_setcwd(root);
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
      if (system(". install/doinst.sh"))
        printf("install: doinst.sh failed\n");
      sys_setcwd(old_cwd);
    }
  }

  /* add package to the database */
//  if (!dryrun)
  {
    if (verbose)
      printf("install: updating database with package: %s\n", name);
    if (db_legacy_add_pkg(pkg))
    {
      _pkg_set_error("installation failed: can't add package to the legacy database\n%s", db_error());
      goto err3;
    }
    if (db_add_pkg(pkg))
    {
      if (db_errno() == DB_EXIST)
        _pkg_set_error("installation failed: can't add package to the database, package with the same name is already there (%s)", name);
      else
        _pkg_set_error("installation failed: can't add package to the database\n%s", db_error());
      goto err3;
    }
  }

  if (verbose)
    printf("install: done installing package: %s\n", name);

  db_free_pkg(pkg);
  g_free(name);
  g_free(shortname);
  return 0;

 err3:
  if (!dryrun)
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

gint pkg_upgrade(const gchar* pkgfile, const gchar* root, gboolean dryrun, gboolean verbose)
{
  _pkg_reset_error();
  /* check package db */
  /* check target files */
  /*  */
  return 0;
}

gint pkg_remove(const gchar* pkgfile, const gchar* root, gboolean dryrun, gboolean verbose)
{
  _pkg_reset_error();
  /* check package db */
  /* check target files */
  /*  */
  return 0;
}
