#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <regex.h>
#include <string.h>

#include "pkgdb.h"
#include "pkgtools.h"

/* helper functions */

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

static void free_pkg(gpointer pkg)
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

/* public functions */

pkgdb_t* pkgdb_open(gchar* root)
{
  DIR* d;
  pkgdb_t* db;
  struct dirent* de;
  gchar* pkgdir;
  
  pkgdir = g_strjoin("/", root, PKGDB_DIR, "packages", 0);
  d = opendir(pkgdir);
  g_free(pkgdir);
  if (d == NULL)
    return NULL;
  db = g_new0(pkgdb_t,1);

  db->dbdir = g_strjoin("/", root, PKGDB_DIR, 0);
  while ((de = readdir(d)) != NULL)
  {
    if (pkgdb_load_pkg(db, de->d_name))
    {
      closedir(d);
      pkgdb_close(db);
      return 0;
    }
  }
  
  closedir(d);
  return db;
}

void pkgdb_close(pkgdb_t* db)
{
  if (db == 0)
    return;
  if (db->pkgs) g_tree_destroy(db->pkgs);
  g_free(db->dbdir);
  g_free(db);
}

/* compile regexps
 * open package db files for given package
 * load package information up to FILE_LIST: using regexps
 * load package file list
 * load package symbolic links list
 */
gint pkgdb_load_pkg(pkgdb_t* db, gchar* pkg)
{
  gint rval = 1;
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
    return rval;

  /* open package db entries */  
  tmpstr = g_strjoin("/", db->dbdir, "packages", pkg, 0);
  fp = fopen(tmpstr, "r");
  g_free(tmpstr);
  tmpstr = g_strjoin("/", db->dbdir, "scripts", pkg, 0);
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

  if (db->pkgs == 0)
    db->pkgs = g_tree_new_full((GCompareDataFunc)strcmp, 0, 0, free_pkg);
  g_tree_insert(db->pkgs, p->name, p);

  rval = 0;
  goto err1;
 err:
  free_pkg(p);
 err1:
  regfree(&re_symlink);
  regfree(&re_pkgname);
  regfree(&re_pkgsize);
  regfree(&re_desc);
  if (ln) free(ln);
  if (fp) fclose(fp);
  if (fs) fclose(fs);
  return rval;
}

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
