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

#include "pkgname.h"
#include "untgz.h"
#include "pkgdb.h"
#include "sys.h"

#include "pkgtools.h"

gint installpkg(gchar* pkgfile, gboolean dryrun, gboolean verbose)
{
  gchar *name, *shortname;
  struct untgz_state* tgz;
  struct db_pkg* pkg;
  GSList* filelist = 0;
  
  /* check if file exist */
  if (sys_file_type(pkgfile,1) != SYS_REG)
  {
    err(0,"package file does not exist: %s\n", pkgfile);
    return 1;
  }

  /* parse package name from the file path */
  if ((name = parse_pkgname(pkgfile,5)) == 0 
      || (shortname = parse_pkgname(pkgfile,1)) == 0)
  {
    err(0,"package name is invalid: %s\n", pkgfile);
    return 1;
  }

  /* check if package is in the database */  
  if ((pkg = db_get_pkg(name,0)))
  {
    err(0,"package is already installed: %s\n", name);
    g_free(name);
    g_free(shortname);
    return 1;
  }

  /* open tgz */
  tgz = untgz_open(pkgfile);
  if (tgz == 0)
  {
    err(0,"can't open package: %s\n", pkgfile);
    g_free(name);
    g_free(shortname);
    return 1;
  }

  /* for each file do extraction or check */
  while (untgz_get_header(tgz) == 0)
  {
    /* if not dir transform name to the install.XXXXXX.filename */
    /* add file to the rollback list */
    /* add file to the update db list */
    
    untgz_write_file(tgz,0);
    continue;
    filelist = g_slist_append(filelist, g_strdup(tgz->f_name));
    if (tgz->f_type == SYS_DIR)
      printf("f: %-30s %6d %-10s %-10s\n", tgz->f_name, tgz->f_size, tgz->f_uname, tgz->f_gname);
    else
      printf("f: %-30s.install %6d %-10s %-10s\n", tgz->f_name, tgz->f_size, tgz->f_uname, tgz->f_gname);
  }  
  printf("s: %8d %8d\n", tgz->csize, tgz->usize);
  
  /* close tgz */
  untgz_close(tgz);

  /* run ldconfig */
  /* run doinst sh */
  /* add package to the database */

  g_free(name);
  g_free(shortname);
  return 0;
}

gint upgradepkg(gchar* pkgfile, gboolean dryrun, gboolean verbose)
{
  /* check package db */
  /* check target files */
  /*  */
  return 0;
}

gint removepkg(gchar* pkgfile, gboolean dryrun, gboolean verbose)
{
  /* check package db */
  /* check target files */
  /*  */
  return 0;
}
