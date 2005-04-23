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
#include "pkgtools.h"
#include "sysutils.h"

/* implement them in C */
static gint rm_rf(gchar* p)
{
  gint rval;
  gchar* s = g_strdup_printf("/bin/rm -rf %s", p);
  rval = system(s);
  g_free(s);
  if (rval == 0)
    return 0;
  return 1;
}

static gint mkdir_p(gchar* p)
{
  gint rval;
  gchar* s = g_strdup_printf("/bin/mkdir -p %s", p);
  rval = system(s);
  g_free(s);
  if (rval == 0)
    return 0;
  return 1;
}

/* public functions */
static void sql_error(sqlite3 *db, char* err);

pkgdb_t* db_open(gchar* root)
{
  pkgdb_t *pdb;
  gchar** d;
  gchar* checkdirs[] = {
    "packages", "scripts", "removed_packages", "removed_scripts", "setup", 
    "fastpkg", 0
  };

  pdb = g_new0(pkgdb_t,1);
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

  pdb->topdir = g_strdup_printf("%s/%s", root, PKGDB_DIR);
  pdb->fpkdir = g_strdup_printf("%s/%s/%s", root, PKGDB_DIR, "fastpkg");
  pdb->fpkdb = g_strdup_printf("%s/%s", pdb->fpkdir, "fastpkg.db");
 
  sqlite3_open(pdb->fpkdb, &pdb->db);
  /*TODO: more checks here (if all tables are present in db, etc.) */
  sqlite3_exec(pdb->db, 
    "CREATE TABLE packages ("
    "  id INTEGER PRIMARY KEY,"
    "  name TEXT UNIQUE NOT NULL,"
    "  shortname TEXT UNIQUE NOT NULL,"
    "  version TEXT,"
    "  arch TEXT,"
    "  build TEXT,"
    "  csize INTEGER,"
    "  usize INTEGER,"
    "  desc TEXT,"
    "  location TEXT"
    ");",
    0,0,0);
//  sql_error(pdb->db,"exec");

  sqlite3_exec(pdb->db, 
    "CREATE TABLE files ("
    "  id INTEGER PRIMARY KEY,"
    "  path TEXT UNIQUE,"
    "  link TEXT DEFAULT NULL"
    ");",
    0,0,0);
//  sql_error(pdb->db,"exec");

  sqlite3_exec(pdb->db, 
    "CREATE TABLE files_map ("
    "  pkg_id INTEGER NOT NULL,"
    "  file_id INTEGER NOT NULL"
    ");",
    0,0,0);
//  sql_error(pdb->db,"exec");

  return pdb;
}

void db_close(pkgdb_t* pdb)
{
  if (pdb == 0)
    return;
  sqlite3_close(pdb->db);
  g_free(pdb->topdir);
  g_free(pdb->fpkdir);
  g_free(pdb->fpkdb);
  g_free(pdb);
}

gint db_sync_fastpkgdb_to_legacydb(pkgdb_t* db)
{
  return 0;
}

static pkgdb_pkg_t* parse_legacydb_entry(pkgdb_t* db, gchar* pkg);
static void free_legacydb_entry(gpointer pkg);

static void sql_error(sqlite3 *db, char* err)
{
  switch (sqlite3_errcode(db))
  {
    case SQLITE_OK:
    case SQLITE_DONE:
    break;
    default:
    fprintf(stderr, "error: sqlite3_%s: %s\n", err, sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }
}

gint db_sync_legacydb_to_fastpkgdb(pkgdb_t* db)
{
  DIR* d;
  gchar* tmpstr;
  struct dirent* de;
  pkgdb_pkg_t* p;
  sqlite3_stmt* stmt;

  tmpstr = g_strdup_printf("%s/%s", db->topdir, "packages");
  d = opendir(tmpstr);
  g_free(tmpstr);
  if (d == NULL)
    return 1;

  sqlite3_exec(db->db, "PRAGMA synchronous = OFF;",0,0,0);
  sql_error(db->db,"exec");
  sqlite3_exec(db->db, "DELETE FROM packages;",0,0,0);
  sql_error(db->db,"exec");

  sqlite3_prepare(db->db, "INSERT INTO packages(name, shortname, version, arch, build, csize, usize, desc, location) VALUES(?,?,?,?,?,?,?,?,?);", -1, &stmt, 0);
  sql_error(db->db,"prepare");
  while ((de = readdir(d)) != NULL)
  {
    if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;

    p = parse_legacydb_entry(db, de->d_name);
    if (p == 0)
    {
      closedir(d);
      return 1;
    }
    printf("processing %s\n", p->name);

    sqlite3_bind_text(stmt, 1, p->name, -1, 0);
    sqlite3_bind_text(stmt, 2, p->shortname, -1, 0);
    sqlite3_bind_text(stmt, 3, p->version, -1, 0);
    sqlite3_bind_text(stmt, 4, p->arch, -1, 0);
    sqlite3_bind_text(stmt, 5, p->build, -1, 0);
    sqlite3_bind_text(stmt, 8, p->desc, -1, 0);
    sqlite3_bind_text(stmt, 9, p->location, -1, 0);
    sqlite3_bind_int(stmt, 7, p->usize);
    sqlite3_bind_int(stmt, 6, p->csize);
    sqlite3_step(stmt);
    sql_error(db->db,"step");
    sqlite3_reset(stmt);
    
    free_legacydb_entry(p);
  }

  sqlite3_finalize(stmt);
  sql_error(db->db,"finalize");
  sqlite3_exec(db->db, "PRAGMA synchronous = ON;",0,0,0);
  
  closedir(d);
  return 0;
}

static gint strcmp_shortname(gchar* a, gchar* b)
{
  gchar* pn = parse_pkgname(a,1);
  gint r = strcmp(b,pn);
  g_free(pn);
  return r;
}

static void free_string(gpointer str)
{
  g_free(str);
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
  if (p == 0)
    return;
  if (p->files) g_tree_destroy(p->files);
  g_free(p->name);
  g_free(p->location);
  g_free(p->desc);
  g_free(p->shortname);
  g_free(p->version);
  g_free(p->arch);
  g_free(p->build);
  g_free(p);
}

static pkgdb_pkg_t* parse_legacydb_entry(pkgdb_t* db, gchar* pkg)
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
  tmpstr = g_strjoin("/", db->topdir, "packages", pkg, 0);
  fp = fopen(tmpstr, "r");
  g_free(tmpstr);
  tmpstr = g_strjoin("/", db->topdir, "scripts", pkg, 0);
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
      {
        if (p->files == 0)
          p->files = g_tree_new_full((GCompareDataFunc)strcmp, 0, free_string, free_string);
        g_tree_replace(p->files, g_strdup(ln), 0);
      }
      break;
      case LINKLIST:
      {
        if (regexec(&re_symlink, ln, 4, rm, 0))
          continue;
        ln[rm[1].rm_eo] = 0;
        ln[rm[2].rm_eo] = 0;
        ln[rm[3].rm_eo] = 0;
        tmpstr = g_strjoin("/", ln+rm[1].rm_so, ln+rm[3].rm_so, 0);
        if (p->files == 0)
          p->files = g_tree_new_full((GCompareDataFunc)strcmp, 0, free_string, free_string);
        g_tree_replace(p->files, tmpstr, g_strdup(ln+rm[2].rm_so));
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
pkgdb_pkg_t* pkgdb_find_pkg(pkgdb_t* db, gchar* pkg)
{
  pkgdb_pkg_t* p;
  p = g_tree_lookup(db->pkgs, pkg);
  if (p)
    return p;
  p = g_tree_search(db->pkgs, (GCompareFunc)strcmp_shortname, pkg);
  if (p)
    return p;
  return 0;
}
#endif
