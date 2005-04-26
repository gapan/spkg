/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>

#include "pkgdb.h"
#include "sql.h"

#include "pkgtools.h"
#include "sysutils.h"

/* private 
 ************************************************************************/

struct {
  gchar* topdir;
  gchar* fpkdir;
  gchar* fpkdb;
  gint is_open;
} pkgdb = {
  .is_open = 0
};

/* public 
 ************************************************************************/

gint db_open(gchar* root)
{
  gchar** d;
  gchar* tmp;
  gint i;
  gchar* checkdirs[] = {
    "packages", "scripts", "removed_packages", "removed_scripts", "setup", 
    "fastpkg", 0
  };
  
  /* already open? */
  if (pkgdb.is_open)
    return 1;

  /* check dirs */
  for (d = checkdirs; *d != 0; d++)
  {
    gchar* tmpdir = g_strdup_printf("%s/%s/%s", root, PKGDB_DIR, *d);
    if (file_type(tmpdir) != FT_DIR)
    {
      rm_rf(tmpdir);
      mkdir_p(tmpdir);
      chmod(tmpdir, 0755);
    }
    g_free(tmpdir);
  }

  /* alloc dir strings */
  pkgdb.topdir = g_strdup_printf("%s/%s", root, PKGDB_DIR);
  pkgdb.fpkdir = g_strdup_printf("%s/%s/%s", root, PKGDB_DIR, "fastpkg");
  pkgdb.fpkdb = g_strdup_printf("%s/%s", pkgdb.fpkdir, "fastpkg.db");
 
  /* open sqlite database */
  sql_open(pkgdb.fpkdb);
  pkgdb.is_open = 1;

  /* check database */
  tmp = 0;
  for (i=0; i<256; i++)
  {
    g_free(tmp);
    tmp = g_strdup_printf("files_%02x", i);
    if (sql_table_exist(tmp))
      continue;
    goto recreate;
  }
  g_free(tmp);
  if (!sql_table_exist("packages"))
    goto recreate;
  return 0;
 recreate:

  sql_exec("BEGIN TRANSACTION;");
  for (i=0; i<256; i++)
  {
    sql_exec( 
      "CREATE TABLE files_%02x ("
      "  id INTEGER PRIMARY KEY,"
      "  path TEXT NOT NULL,"
      "  link TEXT DEFAULT NULL,"
      "  refs INTEGER NOT NULL DEFAULT 1"
      ");", i);
  }
  sql_exec(
    "CREATE TABLE packages ("
    "  id INTEGER PRIMARY KEY,"
    "  name TEXT UNIQUE NOT NULL,"
    "  shortname TEXT NOT NULL,"
    "  version TEXT NOT NULL,"
    "  arch TEXT NOT NULL,"
    "  build TEXT NOT NULL,"
    "  csize INTEGER,"
    "  usize INTEGER,"
    "  desc TEXT,"
    "  location TEXT"
    ");");
  sql_exec("COMMIT;");
  return 0;
}

void db_close()
{
  sql_close();
  g_free(pkgdb.topdir);
  g_free(pkgdb.fpkdir);
  g_free(pkgdb.fpkdb);
}

gint db_sync_fastpkgdb_to_legacydb()
{
  return 0;
}

static pkgdb_pkg_t* parse_legacydb_entry(gchar* pkg);
static void free_legacydb_entry(gpointer pkg);

gint db_sync_legacydb_to_fastpkgdb()
{
  DIR* d;
  gchar* tmpstr;
  struct dirent* de;
  pkgdb_pkg_t* p;
  sqlite3_stmt *q1, *q2;

  tmpstr = g_strdup_printf("%s/%s", pkgdb.topdir, "packages");
  d = opendir(tmpstr);
  g_free(tmpstr);
  if (d == NULL)
    return 1;

  sql_exec("PRAGMA synchronous = OFF;");
  sql_exec("PRAGMA temp_store = MEMORY;");
//  sql_exec("DELETE FROM packages;");

  q1 = sql_prep("INSERT INTO packages(name, shortname, version, arch, build, csize, usize, desc, location) VALUES(?,?,?,?,?,?,?,?,?);");
  while ((de = readdir(d)) != NULL)
  {
    GSList* l;
    int pkgid;
    
    if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;

    p = parse_legacydb_entry(de->d_name);
    if (p == 0)
    {
      closedir(d);
      return 1;
    }
    /* add package to the packages table */
    sqlite3_bind_text(q1, 1, p->name, -1, 0);
    sqlite3_bind_text(q1, 2, p->shortname, -1, 0);
    sqlite3_bind_text(q1, 3, p->version, -1, 0);
    sqlite3_bind_text(q1, 4, p->arch, -1, 0);
    sqlite3_bind_text(q1, 5, p->build, -1, 0);
    sqlite3_bind_text(q1, 8, p->desc, -1, 0);
    sqlite3_bind_text(q1, 9, p->location, -1, 0);
    sqlite3_bind_int(q1, 7, p->usize);
    sqlite3_bind_int(q1, 6, p->csize);
    sql_step(q1);
    pkgid = sql_rowid();
    sql_rest(q1);

#if 0
    sql_exec("BEGIN TRANSACTION; CREATE TEMP TABLE files_tmp ( path TEXT UNIQUE );");

    q2 = sql_prep("INSERT INTO package_files VALUES(?);");
    for (l=p->files; l!=0; l=l->next)
    { /* for each file or link */
      sqlite3_bind_text(q2, 1, l->data, -1, 0);
      sql_step(q2);
      sql_rest(q2);
    }
    sql_fini(q2);

    sql_exec("INSERT INTO files(pkg_id, path) SELECT %d,path FROM package_files;", pkgid);
    sql_exec("DROP TABLE package_files; COMMIT;");
#endif
    free_legacydb_entry(p);
  }

  sql_fini(q1);
  sql_exec("PRAGMA synchronous = ON;");
  
  closedir(d);
  return 0;
}

static void remove_eol(gchar* s)
{
  gsize l = strlen(s);
  if (l > 0 && s[l-1] == '\n')
    s[l-1] = '\0';
}

static gint pkgdb_parse_pkgname(pkgdb_pkg_t* p)
{
  regex_t re;
  regmatch_t rm[5];
  
  if (p == 0 || p->name == 0)
    return 1;
  if (regcomp(&re, "^(.+)-([^-]+)-([^-]+)-([^-]+)$", REG_EXTENDED))
    return 1;
  if (regexec(&re, p->name, 5, rm, 0))
  {
    regfree(&re);
    return 1;
  }
  p->shortname = g_strndup(p->name+rm[1].rm_so, rm[1].rm_eo-rm[1].rm_so);
  p->version =   g_strndup(p->name+rm[2].rm_so, rm[2].rm_eo-rm[2].rm_so);
  p->arch =      g_strndup(p->name+rm[3].rm_so, rm[3].rm_eo-rm[3].rm_so);
  p->build =     g_strndup(p->name+rm[4].rm_so, rm[4].rm_eo-rm[4].rm_so);
  regfree(&re);
  return 0;
}

static void free_legacydb_entry(gpointer pkg)
{
  pkgdb_pkg_t* p = pkg;
  GSList* l;
  if (p == 0)
    return;
  if (p->files) {
    for (l=p->files; l!=0; l=l->next)
      g_free(l->data);
    g_slist_free(p->files);
  }
  g_free(p->name);
  g_free(p->location);
  g_free(p->desc);
  g_free(p->shortname);
  g_free(p->version);
  g_free(p->arch);
  g_free(p->build);
  g_free(p);
}

static pkgdb_pkg_t* parse_legacydb_entry(gchar* pkg)
{
  gchar *tmpstr;
  FILE *fp, *fs, *f;
  gchar *ln = 0;
  gsize len = 0;
  enum { HEADER, FILELIST, LINKLIST } state = HEADER;
  pkgdb_pkg_t* p=0;
  regex_t re_symlink,
          re_pkgname,
          re_pkgsize,
          re_desc;
  regmatch_t rm[4];

  if (pkg == 0)
    return 0;

  /* open package db entries */  
  tmpstr = g_strjoin("/", pkgdb.topdir, "packages", pkg, 0);
  fp = fopen(tmpstr, "r");
  g_free(tmpstr);
  tmpstr = g_strjoin("/", pkgdb.topdir, "scripts", pkg, 0);
  fs = fopen(tmpstr, "r");
  g_free(tmpstr);

  /* if main package db entry is not accessible return error */
  if (fp == NULL)
    goto err;

  /* compile regexps */
  if (regcomp(&re_symlink, "^\\( cd ([^ ]+) ; ln -sf ([^ ]+) ([^ ]+) \\)$", REG_EXTENDED) || 
      regcomp(&re_pkgname, "^([^:]+):[ ]*(.+)?$", REG_EXTENDED) ||
      regcomp(&re_desc, "^([^:]+):(.*)$", REG_EXTENDED) ||
      regcomp(&re_pkgsize, "^([^:]+):[ ]*([0-9]+) K$", REG_EXTENDED))
    g_error("can't compile regexps");

  p = g_new0(pkgdb_pkg_t, 1);
  p->name = g_strdup(pkg);
  if (pkgdb_parse_pkgname(p))
    goto err;
  
  /* for each line in the main package db entry file do: */
  f = fp;
  while (1)
  {
    if (getline(&ln, &len, f) == -1)
    { /* handle EOF */
      if (state == FILELIST)
      {
        if (fs == NULL) /* no linklist */
          break;
        f = fs;
        state = LINKLIST;
        continue;
      }
      else if (state == LINKLIST)
        break;
      goto err;
    }

    remove_eol(ln);
    switch (state)
    {
      case HEADER:
      {
        if (!regexec(&re_pkgsize, ln, 3, rm, 0))
        {
          ln[rm[1].rm_eo] = 0;
          ln[rm[2].rm_eo] = 0;
          if (!strcmp(ln, "COMPRESSED PACKAGE SIZE"))
          {
            p->csize = atol(ln+rm[2].rm_so);
          }
          else if (!strcmp(ln, "UNCOMPRESSED PACKAGE SIZE"))
          {
            p->usize = atol(ln+rm[2].rm_so);
          }
          else
            goto err;
        }
        else if (!regexec(&re_pkgname, ln, 3, rm, 0))
        {
          ln[rm[1].rm_eo] = 0;
          if (!strcmp(ln, "PACKAGE NAME") && rm[2].rm_so > 0)
          {
            ln[rm[2].rm_eo] = 0;
            if (strcmp(p->name, ln+rm[2].rm_so)) /* pkgname != requested pkgname */
              goto err;
          }
          else if (!strcmp(ln, "PACKAGE LOCATION") && rm[2].rm_so > 0)
          {
            ln[rm[2].rm_eo] = 0;
            p->location = g_strdup(ln+rm[2].rm_so);
          }
          else if (!strcmp(ln, "PACKAGE DESCRIPTION"))
          {
          }
          else if (!strcmp(ln, "FILE LIST"))
          {
            state = FILELIST;
          }
          else if (!strcmp(ln, p->shortname))
          {
            ln[rm[1].rm_eo] = ':';
            if (p->desc)
            {
              tmpstr = g_strconcat(p->desc, ln, "\n", 0);
              g_free(p->desc);
              p->desc = tmpstr;
            }
            else
              p->desc = g_strconcat(ln, "\n", 0);
          }
          else
            goto err;
        }
        else
          goto err;
      }
      break;
      case FILELIST:
        p->files = g_slist_append(p->files, g_strdup(ln));
      break;
      case LINKLIST:
      {
        if (regexec(&re_symlink, ln, 4, rm, 0))
          continue;
        ln[rm[1].rm_eo] = 0;
        ln[rm[2].rm_eo] = 0;
        ln[rm[3].rm_eo] = 0;
        tmpstr = g_strjoin("/", ln+rm[1].rm_so, ln+rm[3].rm_so, 0);
        // linktgt = g_strdup(ln+rm[2].rm_so);
//        p->files = g_slist_append(p->files, tmpstr);
      }
      break;
      default:
        goto err;
    }
  }

  goto err1;
 err:
  free_legacydb_entry(p);
  p = 0;
 err1:
  regfree(&re_symlink);
  regfree(&re_pkgname);
  regfree(&re_pkgsize);
  regfree(&re_desc);
  if (ln) free(ln);
  if (fp) fclose(fp);
  if (fs) fclose(fs);
  return p;
}

#if 0
pkgdb_pkg_t* pkgdb_find_pkg(gchar* pkg)
{
  pkgdb_pkg_t* p;
  p = g_tree_lookup(pkgdb.pkgs, pkg);
  if (p)
    return p;
  p = g_tree_search(pkgdb.pkgs, (GCompareFunc)strcmp_shortname, pkg);
  if (p)
    return p;
  return 0;
}
#endif
