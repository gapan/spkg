/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ond�ej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <utime.h>
#include <errno.h>
#include <zlib.h>

#include <Judy.h>

#include "sys.h"
#include "misc.h"
#include "pkgdb.h"
#include "bench.h"

/* private 
 ************************************************************************/

struct db_state {
  gboolean is_open;
  gchar* topdir;
  gchar* pkgdir; /* /var/log/packages/ */
  gchar* scrdir; /* /var/log/scripts/ */
  struct error* err;
  void* files;
  gint fd_lock;
};

static struct db_state _db = {0};

#define e_set(n, fmt, args...) e_add(_db.err, "pkgdb", __func__, n, fmt, ##args)

#define _db_open_check(v) \
  if (!_db.is_open) \
  { \
    e_set(E_ERROR|DB_NOPEN, "package database is NOT open"); \
    return v; \
  }

#define LINEMATCH(s) (strncmp(line, s, sizeof(s)-1) == 0)
#define LINESIZE(s) (sizeof(s)-1)

/* public - open/close
 ************************************************************************/

gint db_open(const gchar* root, struct error* e)
{
  gchar** d;
  gchar* checkdirs[] = { "packages", "scripts", "removed_packages",
    "removed_scripts", "setup", 0 };

  g_assert(e != 0);
  _db.err = e;
  
  reset_timers();
  continue_timer(0);
  
  if (_db.is_open)
  {
    e_set(E_ERROR|DB_OPEN, "package database is already open");
    goto err_0;
  }
  
  if (root == 0 || *root == 0)
    root = "/";

  if (g_path_is_absolute(root))
    _db.topdir = g_strdup_printf("%s/%s", root, PKGDB_DIR);
  else
  {
    gchar* cwd = g_get_current_dir();
    _db.topdir = g_strdup_printf("%s/%s/%s", cwd, root, PKGDB_DIR);
    g_free(cwd);
  }
  _db.pkgdir = g_strdup_printf("%s/packages", _db.topdir);
  _db.scrdir = g_strdup_printf("%s/scripts", _db.topdir);

  /* check legacy and spkg db dirs */
  for (d = checkdirs; *d != 0; d++)
  {
    gchar* tmpdir = g_strdup_printf("%s/%s", _db.topdir, *d);
    /* if it is not a directory, clean it and create it */
    if (sys_file_type(tmpdir, 1) != SYS_DIR)
    {
      sys_rm_rf(tmpdir);
      sys_mkdir_p(tmpdir);
      chmod(tmpdir, 0755);
      /* if it is still not a directory, return with error */
      if (sys_file_type(tmpdir, 1) != SYS_DIR)
      {
        e_set(E_FATAL, "%s should be an accessible directory", tmpdir);
        g_free(tmpdir);
        goto err_1;
      }
    }
    g_free(tmpdir);
  }

  /* get lock */
  gchar *path_lock = g_strdup_printf("%s/.lock", _db.pkgdir);
  _db.fd_lock = sys_lock_new(path_lock, e);
  g_free(path_lock);
  if (_db.fd_lock == -1)
  {
    e_set(E_FATAL, "locking failure");
    goto err_1;
  }
  if (sys_lock_trywait(_db.fd_lock, 20, e))
  {
    e_set(E_FATAL, "locking failure");
    goto err_2;
  }

  _db.is_open = 1;
  stop_timer(0);

  return 0;

 err_2:
  sys_lock_del(_db.fd_lock);
 err_1:
  g_free(_db.topdir);
  g_free(_db.pkgdir);
  g_free(_db.scrdir);
  memset(&_db, 0, sizeof(_db));
 err_0:
  return 1;
}

void db_close()
{
  continue_timer(1);

  if (!_db.is_open)
  {
    e_set(E_ERROR, "database is not open");
    return;
  }

  sys_lock_del(_db.fd_lock);

  g_free(_db.topdir);
  g_free(_db.pkgdir);
  g_free(_db.scrdir);
  memset(&_db, 0, sizeof(_db));

  stop_timer(1);

  print_timer(0, "[pkgdb] db_open");
  print_timer(1, "[pkgdb] db_close");
  print_timer(2, "[pkgdb] db_get_pkg");
  print_timer(3, "[pkgdb] db_add_pkg");
  print_timer(8, "[pkgdb] files: judy");
  print_timer(9, "[pkgdb] files: read");
}

void db_filelist_rem_pkg_files(const struct db_pkg* pkg)
{
  gchar path[4096];
  gint rc;
  void **p1, **p2;
  strcpy(path, "");
  JSLF(p1, pkg->files, path);
  while (p1 != NULL)
  {
    if (*p1 == 0) // not link
    {
      JSLG(p2, _db.files, path);
      if (p2 && (gint)*p2 == 1)
      {
        JSLD(rc, _db.files, path);
      }
      else
        (*p2)--;
    }
    JSLN(p1, pkg->files, path);
  }
}

void db_filelist_add_pkg_files(const struct db_pkg* pkg)
{
  gchar path[4096];
  void **p1, **p2;
  strcpy(path, "");
  JSLF(p1, pkg->files, path);
  while (p1 != NULL)
  {
    if (*p1 == 0) // not link
    {
      JSLI(p2, _db.files, path);
      (*p2)++;
    }
    JSLN(p1, pkg->files, path);
  }
}

gint db_filelist_load()
{
  DIR* d = opendir(_db.pkgdir);
  if (d == NULL)
  {
    e_set(E_FATAL, "can't open legacy db directory");
    goto err_0;
  }
  gchar sbuf[1024*128];

  db_filelist_free();

  gchar* line = 0;
  gint size = 0;
  
  struct dirent* de;
  while ((de = readdir(d)) != NULL)
  {
    gchar* name = de->d_name;
    if (parse_pkgname(name, 6) == 0)
      continue;

    gchar* tmpstr = g_strdup_printf("%s/%s", _db.pkgdir, name);
    FILE* f = fopen(tmpstr, "r");
    g_free(tmpstr);
    if (f == NULL) /* main package entry can't be open */
    {
      e_set(E_ERROR, "can't open package entry file: %s: %s", name, strerror(errno));
      goto err_1;
    }
    setvbuf(f, sbuf, _IOFBF, sizeof(sbuf));
    
    gint linelen;
    gint files = 0;
    while ((linelen = getline(&line, &size, f)) >= 0)
    {
      if (linelen > 0 && line[linelen-1] == '\n')
        line[linelen-1] = '\0', linelen--;
      if (files)
      {
        void** ptr;
        JSLI(ptr, _db.files, line);
        (*ptr)++;
      }
      else if (LINEMATCH("FILE LIST:"))
      {
        files++;
      }
	}
    
    fclose(f);
  }

  if (line)
    free(line);

  closedir(d);
  return 0;
 err_1:
  db_filelist_free();
  closedir(d);
 err_0:
  return 1;
}

gint db_filelist_get_file(const gchar* path)
{
  void **ptr;
  JSLG(ptr, _db.files, path);
  if (ptr == NULL)
    return 0;
  return (gint)*ptr;
}

void db_filelist_free()
{
  guint used;
  JSLFA(used, _db.files);
  printf("used %d\n", used);
}

/* public - memmory management
 ************************************************************************/

struct db_pkg* db_alloc_pkg(gchar* name)
{
  struct db_pkg* p;
  g_assert(name != 0);
  if (parse_pkgname(name, 6) != (gchar*)-1)
  {
    e_set(E_ERROR, "invalid package name");
    return 0;
  }
  p = g_new0(struct db_pkg, 1);
  p->name = parse_pkgname(name, 5);
  p->shortname = parse_pkgname(name, 1);
  p->version = parse_pkgname(name, 2);
  p->arch = parse_pkgname(name, 3);
  p->build = parse_pkgname(name, 4);
  p->time = time(0);
  return p;
}

void db_free_pkg(struct db_pkg* pkg)
{
  continue_timer(6);
  guint freed;
  if (pkg == 0)
    return;
  JSLFA(freed, pkg->files);
  g_free(pkg->name);
  g_free(pkg->shortname);
  g_free(pkg->version);
  g_free(pkg->arch);
  g_free(pkg->build);
  g_free(pkg->location);
  g_free(pkg->desc);
  g_free(pkg->doinst);
  g_free(pkg);
  stop_timer(6);
}

void db_add_file(struct db_pkg* pkg, gchar* path, gchar* link_target)
{
  void** ptr;
  JSLI(ptr, pkg->files, path);
  *ptr = link_target;
}

/* public - main database package operations
 ************************************************************************/

gint db_add_pkg(struct db_pkg* pkg)
{
  FILE* pf;
  FILE* sf;
  gchar *ppath, *spath;
  gint ret = 1;

  continue_timer(5);

  _db_open_check(1)

  /* check if pkg contains everthing required */
  if (pkg == 0 || pkg->name == 0 || pkg->files == 0)
  {
    e_set(E_BADARG, "incomplete package structure");
    goto err_0;
  }

  ppath = g_strdup_printf("%s/%s", _db.pkgdir, pkg->name);
  spath = g_strdup_printf("%s/%s", _db.scrdir, pkg->name);

  if (sys_file_type(ppath,0) != SYS_NONE)
  {
    e_set(E_FATAL, "package is already in database (%s)", strerror(errno));
    goto err_1;
  }

  pf = fopen(ppath, "w");
  if (pf == 0)
  {
    e_set(E_FATAL, "can't open package file (%s)", strerror(errno));
    goto err_1;
  }
  sf = fopen(spath, "w");
  if (sf == 0)
  {
    e_set(E_FATAL, "can't open script file");
    goto err_2;
  }

  /* construct header */
  fprintf(pf,
    "PACKAGE NAME:              %s\n"
    "COMPRESSED PACKAGE SIZE:   %u K\n"
    "UNCOMPRESSED PACKAGE SIZE: %u K\n"
    "PACKAGE LOCATION:          %s\n"
    "PACKAGE DESCRIPTION:\n"
    "%s"
    "FILE LIST:\n",
    pkg->name, pkg->csize, pkg->usize, pkg->location?pkg->location:"", pkg->desc?pkg->desc:""
  );
  
  if (pkg->doinst)
    fprintf(sf, "%s", pkg->doinst);

  /* construct filelist */
  gchar path[4096];
  void** ptr;
  strcpy(path, "");
  JSLF(ptr, pkg->files, path);
  while (ptr != NULL)
  {
    if (*ptr == 0)
      fprintf(pf, "%s\n", path);
    JSLN(ptr, pkg->files, path);
  }

  ret = 0;
 err_3: 
  fclose(sf);
 err_2:
  fclose(pf);

  struct utimbuf dt = { pkg->time, pkg->time };
  if (utime(ppath, &dt) == -1)
  {
    e_set(E_ERROR, "can't utime package entry: %s", strerror(errno));
    goto err_3;
  }

 err_1:
  g_free(ppath);
  g_free(spath);
 err_0:
  stop_timer(5);
  return ret;
}

struct db_pkg* db_get_pkg(gchar* name, db_get_type type)
{
  FILE *fp, *fs;
  gchar *tmpstr;
  gchar stream_buf1[1024*128];
  gchar stream_buf2[1024*64];
  struct db_pkg* p = 0;
  
  if (name == 0)
  {
    e_set(E_BADARG, "package name missing");
    goto err_0;
  }
  if (parse_pkgname(name, 6) != (gchar*)-1)
  {
    e_set(E_ERROR, "invalid package name: %s", name);
    goto err_0;
  }
  if (type != DB_GET_WITHOUT_FILES && type != DB_GET_FULL)
  {
    e_set(E_BADARG, "invalid get type");
    goto err_0;
  }

  _db_open_check(0)

  continue_timer(4);

  /* open legacy package db entries */  
  tmpstr = g_strdup_printf("%s/%s", _db.pkgdir, name);
  fp = fopen(tmpstr, "r");
  time_t mtime = sys_file_mtime(tmpstr,0);
  g_free(tmpstr);
  if (fp == NULL) /* main package entry can't be open */
  {
    e_set(E_ERROR | (errno == ENOENT ? DB_NOTEX : 0), "can't open main package entry file: %s", strerror(errno));
    goto err_0;
  }
  if (mtime == (time_t)-1) /* package time can't be retrieved */
  {
    e_set(E_ERROR, "can't get main package entry file mtime: %s", strerror(errno));
    goto err_0;
  }
  setvbuf(fp, stream_buf1, _IOFBF, sizeof(stream_buf1));

  /*XXX: better checks here (if NOTEX, or other error) */
  tmpstr = g_strdup_printf("%s/%s", _db.scrdir, name);
  fs = fopen(tmpstr, "r");
  g_free(tmpstr);
  if (fs)
    setvbuf(fs, stream_buf2, _IOFBF, sizeof(stream_buf2));

  p = g_new0(struct db_pkg, 1);
  p->name = g_strdup(name);
  p->shortname = parse_pkgname(p->name, 1);
  p->version = parse_pkgname(p->name, 2);
  p->arch = parse_pkgname(p->name, 3);
  p->build = parse_pkgname(p->name, 4);
  p->time = mtime;

  /* parse main package file */
  gint snl = strlen(p->shortname);
  gint m[5] = { 0 }; /* if particular line was matched it can't 
  occur anymore, so we cache info about already matched lines */

  gchar* line = 0;
  gint size = 0;
  gint linelen;
  continue_timer(9);
  while ((linelen = getline(&line, &size, fp)) >= 0)
  {
    if (linelen > 0 && line[linelen-1] == '\n')
	{
	  line[linelen-1] = '\0';
	  linelen--;
	}
    if (!m[0] && LINEMATCH("PACKAGE NAME:"))
    {
      gchar* name = g_strstrip(line + LINESIZE("PACKAGE NAME:"));
	  if (strcmp(name, p->name))
      {
        e_set(E_ERROR, "package names don't match: %s", p->name);
        goto err_1;
      }
      /* skip whitespace */
      m[0] = 1;
    }
    else if (!m[1] && LINEMATCH("COMPRESSED PACKAGE SIZE:"))
    {
      gchar* size = line + LINESIZE("COMPRESSED PACKAGE SIZE:");
      if (sscanf(size, " %u ", &p->csize) != 1)
      {
        e_set(E_ERROR, "can't read compressed package size");
        goto err_1;
      }
      m[1] = 1;
    }
    else if (!m[2] && LINEMATCH("UNCOMPRESSED PACKAGE SIZE:"))
    {
      gchar* size = line + LINESIZE("UNCOMPRESSED PACKAGE SIZE:");
      if (sscanf(size, " %u ", &p->usize) != 1)
      {
        e_set(E_ERROR, "can't read compressed package size");
        goto err_1;
      }
      m[2] = 1;
    }
    else if (!m[3] && LINEMATCH("PACKAGE LOCATION:"))
    {
      p->location = g_strdup(g_strstrip(line + LINESIZE("PACKAGE LOCATION:")));
      m[3] = 1;
    }
    else if (!m[4] && LINEMATCH("PACKAGE DESCRIPTION:"))
	{
      m[4] = 1;
	}
    else if (strncmp(line, p->shortname, snl) == 0)
    {
      gchar* desc = g_strconcat(p->desc ? p->desc : "", line, "\n", 0);
      g_free(p->desc);
      p->desc = desc;
    }
    else if (LINEMATCH("FILE LIST:"))
	{
      goto parse_files;
	}
    else
    {
      e_set(E_ERROR, "corrupt legacy package database");
      goto err_1;
    }
  }
  goto err_1;

 parse_files:
  if (type == DB_GET_WITHOUT_FILES)
    goto fini;

  while ((linelen = getline(&line, &size, fp)) >= 0)
  {
    if (linelen > 0 && line[linelen-1] == '\n')
	  line[linelen-1] = '\0';
    void** ptr;
    JSLI(ptr, p->files, line);
    *ptr = 0;
  }

  if (fs == NULL)
    goto fini;

  while ((linelen = getline(&line, &size, fs)) >= 0)
  {
    if (linelen > 0 && line[linelen-1] == '\n')
	  line[linelen-1] = '\0';

    gchar *dir, *link, *target;
    if (parse_createlink(line, &dir, &link, &target))
    {
      gchar* path = g_strdup_printf("%s/%s", dir, link);
      g_free(dir);
      g_free(link);
      void** ptr;
      JSLI(ptr, p->files, path);
      *ptr = target;
      g_free(path);
    }
  }
  
  if (line)
    free(line);

  fseek(fs, 0, SEEK_END);
  guint script_size = ftell(fs);
  if (script_size > 512*1024)
  {
    e_set(E_ERROR, "too big script file");
    goto err_1;
  }
  if (script_size)
  {
    p->doinst = g_malloc(script_size);
    fseek(fs, 0, SEEK_SET);
    if (fread(p->doinst, script_size, 1, fs) != 1)
    {
      e_set(E_ERROR, "can't read script file");
      goto err_1;
    }
  }

 fini:
  goto no_err;

 err_1:
  db_free_pkg(p);
  p=0;

 no_err:
  if (fs)
    fclose(fs);
  fclose(fp);
 err_0:
  stop_timer(4);
  return p;
}

gint db_rem_pkg(gchar* name)
{
  _db_open_check(1)

  gchar* p = g_strdup_printf("%s/%s", _db.pkgdir, name);
  gchar* s = g_strdup_printf("%s/%s", _db.scrdir, name);
  gint ret = 1;
  if (sys_file_type(p, 0) != SYS_REG)
  {
    e_set(E_ERROR|DB_NOTEX, "package is not in database");
    goto err_1;
  }
  if (unlink(p) == -1)
  {
    e_set(E_ERROR|DB_NOTEX, "unlink failed: %s", strerror(errno));
    goto err_1;
  }
  if (sys_file_type(s, 0) == SYS_REG)
  {
    if (unlink(s) == -1)
    {
      e_set(E_ERROR|DB_NOTEX, "unlink failed: %s", strerror(errno));
      goto err_1;
    }
  }
  ret = 0;
 err_1:
  g_free(p);
  g_free(s);
  return ret;
}

/* public - generic database package queries
 ************************************************************************/

GSList* db_query(db_selector cb, void* data, db_query_type type)
{
  GSList *pkgs=0;

  _db_open_check(0)

  if (type != DB_QUERY_PKGS_WITH_FILES &&
      type != DB_QUERY_PKGS_WITHOUT_FILES &&
      type != DB_QUERY_NAMES)
  {
    e_set(E_BADARG, "invalid query type");
    goto err_0;
  }

  DIR* d = opendir(_db.pkgdir);
  if (d == NULL)
  {
    e_set(E_FATAL, "can't open legacy db directory");
    goto err_0;
  }
  
  struct dirent* de;
  while ((de = readdir(d)) != NULL)
  {
    if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;
    struct db_pkg* p;
    gchar* name = de->d_name;
    if (parse_pkgname(name, 6) == 0)
      continue;
    /* if cb == 0, then package matches */
    if (cb != 0)
    {
      /* otherwise get package from database ask the selector if it 
         likes this package */
      p = db_get_pkg(name, DB_GET_WITHOUT_FILES);
      if (p == 0)
      {
        e_set(E_ERROR, "can't get package from database");
        goto err_1;
      }
      gint rs = cb(p, data);
      if (rs != 0 && rs != 1)
      {
        db_free_pkg(p);
        e_set(E_ERROR, "db_selector returned with error");
        goto err_1;
      }
      else if (rs == 0)
      {
        db_free_pkg(p);
        continue;
      }
      db_free_pkg(p);
    }
    db_get_type get_type = DB_GET_WITHOUT_FILES;
    switch (type)
    {
      case DB_QUERY_PKGS_WITH_FILES:
        get_type = DB_GET_FULL;
      case DB_QUERY_PKGS_WITHOUT_FILES:
        p = db_get_pkg(name, get_type);
        if (p == 0)
        {
          e_set(E_ERROR, "can't get package from database");
          goto err_1;
        }
        pkgs = g_slist_prepend(pkgs, p);
        break;
      case DB_QUERY_NAMES:
        pkgs = g_slist_prepend(pkgs, g_strdup(name));
        break;
    }
  }

  closedir(d);
  return pkgs;
 err_1:
  db_free_query(pkgs, type);
  closedir(d);
 err_0:
  return 0;
}

void db_free_query(GSList* pkgs, db_query_type type)
{
  if (pkgs == 0)
    return;

  void (*data_free_func)(void*) = g_free;
  if (type == DB_QUERY_PKGS_WITH_FILES ||
      type == DB_QUERY_PKGS_WITHOUT_FILES)
    data_free_func = (void (*)(void*))db_free_pkg;
  
  GSList* l;
  for (l=pkgs; l!=0; l=l->next)
    data_free_func(l->data);

  g_slist_free(pkgs);
}

gint db_foreach_package(db_selector cb, void* data, db_get_type type)
{
  _db_open_check(0)

  if (type != DB_GET_WITHOUT_FILES &&
      type != DB_GET_FULL)
  {
    e_set(E_BADARG, "invalid query type");
    goto err_0;
  }
  
  if (cb == 0)
  {
    e_set(E_BADARG, "callback not set");
    goto err_0;
  }

  DIR* d = opendir(_db.pkgdir);
  if (d == NULL)
  {
    e_set(E_FATAL, "can't open legacy db directory");
    goto err_0;
  }
  
  struct dirent* de;
  while ((de = readdir(d)) != NULL)
  {
    if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;
    struct db_pkg* p;
    gchar* name = de->d_name;
    if (parse_pkgname(name, 6) == 0)
      continue;
    /* if cb == 0, then package matches */
    /* otherwise get package from database ask the selector if it 
       likes this package */
    p = db_get_pkg(name, type);
    if (p == 0)
    {
      e_set(E_ERROR, "can't get package from database");
      goto err_1;
    }
    gint rs = cb(p, data);
    db_free_pkg(p);
    if (rs < 0)
    {
      e_set(E_ERROR, "callback returned error");
      goto err_1;
    }
  }

  closedir(d);
  return 0;

 err_1:
  closedir(d);
 err_0:
  return 1;
}
