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
#include <regex.h>
#include "pkgtools.h"
#include "untgz.h"
#include "pkgdb.h"
#include "sys.h"

opts_t opts = {
  .install = 0,
  .upgrade = 0,
  .remove = 0,
  .root = 0,
  .verbose = 0,
  .check = 0,
  .force = 0,
  .files = 0,
};

/* elem:
 * 0 = path
 * 1 = shortname
 * 2 = version
 * 3 = arch
 * 4 = build
 * 5 = fullname
 */
gchar* parse_pkgname(gchar* path, guint elem)
{
  regex_t re1, re2;
  regmatch_t rm1[3];
  regmatch_t rm2[5];
  gint rval;
  gchar* fullname=0;
  gchar* result=0;
  if (path == 0)
    return 0;

  /* equivalent basename <path> .tgz */
  path = g_strdup(path);
  if (g_str_has_suffix(path, ".tgz"))
    *g_strrstr(path, ".tgz") = 0;
  if (regcomp(&re1, "^(.*/)?([^/]+)$", REG_EXTENDED))
    g_error("regexp compilation failed");
  rval = regexec(&re1, path, 3, rm1, 0);
  regfree(&re1);
  if (rval)
    goto ret;
  fullname = g_strndup(path+rm1[2].rm_so, rm1[2].rm_eo-rm1[2].rm_so);

  /* separate parts of the package name */
  if (regcomp(&re2, "^(.+)-([^-]+)-([^-]+)-([^-]+)$", REG_EXTENDED))
    g_error("regexp compilation failed");
  rval = regexec(&re2, fullname, 5, rm2, 0);
  regfree(&re2);
  if (rval)
    goto ret;

  switch (elem)
  {
    case 0:
      if (rm1[1].rm_so != -1)
        result = g_strndup(path+rm1[1].rm_so, rm1[1].rm_eo-rm1[1].rm_so);
      break;
    case 1: case 2: case 3: case 4:
      result = g_strndup(fullname+rm2[elem].rm_so, rm2[elem].rm_eo-rm2[elem].rm_so);
      break;
    case 5:
      result = fullname;
      fullname = 0;
      break;
    default:
      break;
  }
 ret:
  g_free(path);
  g_free(fullname);
  return result;
}

/** @brief Install package from the pkgfile.
 * 
 * Install package while creating pkgdb entry and then update database.
 *
 * @param  pkgfile Package file.
 * @return 
 */
gint installpkg(gchar* pkgfile)
{
  gchar *name, *shortname;
  struct untgz_state* tgz;
  struct db_pkg* pkg;
  GSList* filelist = 0;
  
  /* check if file exist */
  if (sys_file_type(pkgfile) != SYS_REG)
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
  if ((pkg = db_get_pkg(name)))
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

gint upgradepkg(gchar* pkgfile)
{
  /* check package db */
  /* check target files */
  /*  */
  return 0;
}

gint removepkg(gchar* pkgfile)
{
  /* check package db */
  /* check target files */
  /*  */
  return 0;
}
