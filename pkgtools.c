/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "pkgname.h"
#include "untgz.h"
#include "pkgdb.h"
#include "sys.h"

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

gint pkg_install(const gchar* pkgfile, const gchar* root, gboolean dryrun, gboolean verbose)
{
  gchar *name, *shortname;
  struct untgz_state* tgz=0;
  struct db_pkg* pkg=0;
  guint stamp = time(0)&0xffff;
  FILE* rollback;

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
  if ((pkg = db_get_pkg(name,0)))
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
  /*XXX: check for shortname match */

  /* open tgz */
  tgz = untgz_open(pkgfile);
  if (tgz == 0)
  {
    _pkg_set_error("installation failed: can't open package file (%s)", pkgfile);
    goto err1;
  }

  /* open rollback script */
  rollback = fopen("fastpkg-rollback.sh", "w");
  if (rollback == 0)
  {
    _pkg_set_error("installation failed: can't create rollback script");
    goto err2;
  }
  fprintf(rollback, "#!/bin/sh\n");

  pkg = db_alloc_pkg(name);
  pkg->location = g_strdup(pkgfile);

  /* for each file in package */
  while (untgz_get_header(tgz) == 0)
  {
    /* check special files */
    if (!strcmp(tgz->f_name, "install/slack-desc"))
    {
      guchar* buf;
      gsize len;
      untgz_write_data(tgz,&buf,&len);
      pkg->desc = buf;      
    }

    gchar* path = g_strdup_printf("%s/%s", root, tgz->f_name);
    gchar* spath = g_strdup_printf("%s/%s.install.%04x", root, tgz->f_name, stamp);

    pkg->files = g_slist_append(pkg->files, db_alloc_file(g_strdup(tgz->f_name), 0));

    if (tgz->f_type == UNTGZ_DIR)
    {
      if (access(path, F_OK) != 0)
        fprintf(rollback, "rmdir %s\n", path);
      untgz_write_file(tgz,path);
    }
    else
    {
      fprintf(rollback, "rm %s\n", path);
      untgz_write_file(tgz,path);
    }
    g_free(spath);
  }
  
  /* close rollback script */
  fclose(rollback);
  chmod("fastpkg-rollback.sh", 0755);

  /* error occured during extraction */
  if (tgz->errstr)
  {
    _pkg_set_error("installation failed: package is corrupted (%s)", pkgfile);
    goto err3;
  }

  pkg->usize = tgz->usize;
  pkg->csize = tgz->csize;
  
  /* close tgz */
  untgz_close(tgz);

  /* run ldconfig */
  /* run doinst sh */

  /* add package to the database */
  if (db_add_pkg(pkg))
  {
    if (db_errno() == DB_EXIST)
      _pkg_set_error("installation failed: can't add package to the database, package with the same name is already there (%s)", name);
    else
      _pkg_set_error("installation failed: can't add package to the database\n%s", db_error());
    goto err3;
  }

  db_free_pkg(pkg);
  g_free(name);
  g_free(shortname);
  return 0;
 err3:
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
