/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
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
#include <semaphore.h>

#include "sql.h"
#include "sys.h"
#include "pkgname.h"
#include "filedb.h"
#include "pkgdb.h"
#include "bench.h"

/* private 
 ************************************************************************/

struct db_state {
  gboolean is_open;
  gchar* topdir;
  gchar* dbfile;
  gchar* dbroot;
  struct error* err;
  struct fdb* fdb;
  sem_t* sem;
};

static struct db_state _db = {0};

#define e_set(n, fmt, args...) e_add(_db.err, "pkgdb", __func__, n, fmt, ##args)

#define _db_open_check(v) \
  if (!_db.is_open) \
  { \
    e_set(E_ERROR|DB_NOPEN, "package database is NOT open"); \
    return v; \
  }

#define SEMAPHORE_NAME "/sem.spkg.pkgdb"

/* public 
 ************************************************************************/

gint db_open(const gchar* root, struct error* e)
{
  gchar** d;
  gchar* checkdirs[] = {
    "packages", "scripts", "removed_packages", "removed_scripts", "setup", 
    "spkgdb", "packages.spkg", "scripts.spkg", 0
  };

  g_assert(e != 0);
  
  reset_timers();
  continue_timer(0);
  
  if (_db.is_open)
  {
    e_set(E_ERROR|DB_OPEN, "package database is already open");
    return 1;
  }
  
  if (root == 0)
    root = "/";

  _db.sem = sem_open(SEMAPHORE_NAME, O_CREAT, 0666, 1);
  if (_db.sem == SEM_FAILED)
  {
    e_set(E_FATAL, "can't open semaphore: %s", strerror(errno));
    goto err_0;
  }

  /* while we are not allowed to enter critical section, wait a while */
  gint s, c=0;
  do {
    /* wait up to 2 seconds for the semaphore */
    if (c++) /* no, not a c++ ;-) */
      usleep(50000); /*XXX: not a posix, but better than nothing */
    if (c > 40)
    {
      e_set(E_ERROR|DB_BLOCKED, "sem_trywait failed to get semaphore: %s", strerror(errno));
      goto err_1;
    }
    s = sem_trywait(_db.sem);
  } while(s == -1 && errno == EAGAIN);
  if (s == -1) /* this means, that last status was bad and not EAGAIN */
  {
    e_set(E_FATAL, "sem_trywait failed: %s", strerror(errno));
    goto err_1;
  }

  if (root[0] == '/')
    _db.topdir = g_strdup_printf("%s/%s", root, PKGDB_DIR);
  else
  {
    gchar* cwd = getcwd(0,0);
    _db.topdir = g_strdup_printf("%s/%s/%s", cwd, root, PKGDB_DIR); /*XXX: not portable */
    free(cwd);
  }

  /* check legacy and spkg db dirs */
  for (d = checkdirs; *d != 0; d++)
  {
    gchar* tmpdir = g_strdup_printf("%s/%s", _db.topdir, *d);
    /* if it is not a directory, clean it and create it */
    if (sys_file_type(tmpdir,1) != SYS_DIR)
    {
      sys_rm_rf(tmpdir);
      sys_mkdir_p(tmpdir);
      chmod(tmpdir, 0755);
      /* if it is still not a directory, return with error */
      if (sys_file_type(tmpdir,1) != SYS_DIR)
      {
        e_set(E_FATAL, "%s should be an accessible directory", tmpdir);
        g_free(tmpdir);
        goto err_2;
      }
    }
    g_free(tmpdir);
  }

  /* check spkg db file */
  _db.dbroot = g_strdup_printf("%s/%s", _db.topdir, "spkgdb");
  _db.dbfile = g_strdup_printf("%s/%s", _db.dbroot, "spkg.db");
  if (sys_file_type(_db.dbfile,0) != SYS_REG && sys_file_type(_db.dbfile,0) != SYS_NONE)
  {
    e_set(E_FATAL, "file %s is not accessible", _db.dbfile);
    goto err_3;
  }

  /* open file database */
  _db.fdb = fdb_open(_db.dbroot, e);
  if (_db.fdb == 0)
  {
    e_set(E_FATAL, "can't open file database");
    goto err_3;
  }

  /* open sql database */
  if (sql_open(_db.dbfile))
  {
    e_set(E_FATAL, "sql_open failed");
    goto err_4;
  }

  /* setup sql error handling */
  sql_push_context(SQL_ERRJUMP,0);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    e_set(E_FATAL, "%s", sql_error());
    goto err_5;
  }
  
  if (!sql_integrity_check())
  { /* sql exception occured */
    e_set(E_FATAL|DB_CORRUPT, "package database (database is corrupted)");
    goto err_5;
  }

  /* sqlite setup */
  sql_exec("PRAGMA temp_store = MEMORY;");
  sql_exec("PRAGMA synchronous = OFF;");

  sql_transaction_begin();

  /* if package table does not exist create it */
  if (!sql_table_exist("packages"))
  {
    sql_exec(
      "CREATE TABLE packages ("
      " id INTEGER PRIMARY KEY,"
      " name TEXT UNIQUE NOT NULL,"
      " shortname TEXT NOT NULL,"
      " version TEXT NOT NULL,"
      " arch TEXT NOT NULL,"
      " build TEXT NOT NULL,"
      " csize INTEGER,"
      " usize INTEGER,"
      " desc TEXT,"
      " doinst TEXT,"
      " location TEXT,"
      " files BLOB "
      ");"
    );
  }

  sql_pop_context(1);
  _db.is_open = 1;
  stop_timer(0);
  return 0;
 err_5:
  sql_close();
 err_4:
  fdb_close(_db.fdb);
  _db.fdb = 0;
 err_3:
  g_free(_db.dbfile);
  _db.dbfile = 0;
  g_free(_db.dbroot);
  _db.dbroot = 0;
 err_2:
  g_free(_db.topdir);
  _db.topdir = 0;
 err_1:
  sem_post(_db.sem);
 err_0:
  sem_unlink(SEMAPHORE_NAME);
  sem_close(_db.sem);
  return 1;
}

void db_close()
{
  continue_timer(1);
  fdb_close(_db.fdb);
  _db.fdb = 0;
  sql_close();

  /*XXX: check this */
  sem_post(_db.sem);
  sem_unlink(SEMAPHORE_NAME);
  sem_close(_db.sem);

  g_free(_db.dbfile);
  g_free(_db.topdir);
  g_free(_db.dbroot);
  g_blow_chunks();
  _db.is_open = 0;
  stop_timer(1);

  print_timer(0, "[pkgdb] db_open");
  print_timer(1, "[pkgdb] db_close");
  print_timer(2, "[pkgdb] db_get_pkg");
  print_timer(3, "[pkgdb] db_add_pkg");
  print_timer(4, "[pkgdb] db_legacy_get_pkg");
  print_timer(5, "[pkgdb] db_legacy_add_pkg");
  print_timer(6, "[pkgdb] db_free_pkg");
  print_timer(7, "[pkgdb] db_get_packages");
  print_timer(8, "[pkgdb] db_legacy_get_packages");
  print_timer(9, "[pkgdb] db_free_packages");
  print_timer(10, "[pkgdb] db_add_pkg:dbg");
}

struct db_pkg* db_alloc_pkg(gchar* name)
{
  struct db_pkg* p;
  if (name == 0 || !parse_pkgname(name, 6))
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
  struct db_pkg* p = pkg;
  GSList* l;
  if (p == 0)
    return;
  if (p->files)
  {
    for (l=p->files; l!=0; l=l->next)
    {
      struct db_file* f = l->data;
      g_free(f->path);
      g_free(f->link);
      g_free(f);
      l->data = 0;
    }
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
  stop_timer(6);
}

struct db_file* db_alloc_file(gchar* path, gchar* link)
{
  struct db_file* f;
  f = g_new0(struct db_file, 1);
  f->path = path;
  f->link = link;
  return f;
}

gint db_add_pkg(struct db_pkg* pkg)
{
  sql_query *q;
  GSList* l;
  continue_timer(3);

  _db_open_check(1)
  
  /* check if pkg contains everthing required */
  if (pkg == 0 || pkg->name == 0 || pkg->shortname == 0 
      || pkg->version == 0 || pkg->build == 0 || pkg->files == 0)
  {
    e_set(E_BADARG, "incomplete package structure");
    goto err_0;
  }
  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    e_set(E_FATAL|DB_SQL, "%s", sql_error());
    sql_pop_context(0);
    goto err_0;
  }

  /* check if package already exists in db */
  q = sql_prep("SELECT id FROM packages WHERE name == '%q';", pkg->name);
  if (sql_step(q))
  { /* if package exists */
    e_set(E_ERROR|DB_EXIST, "package is already in database (%s)", pkg->name);
    sql_pop_context(0);
    goto err_0;
  }
  sql_fini(q);

  guint fi_size = g_slist_length(pkg->files);
  guint32 *fi_array = g_malloc(sizeof(guint32)*fi_size);
  guint i = 0;

  for (l=pkg->files; l!=0; l=l->next)
  { /* for each file */
    struct db_file* f = l->data;
    struct fdb_file fdb;
    fdb.path = f->path;
    fdb.link = f->link;    
    f->id = fdb_add_file(_db.fdb, &fdb);
    f->refs = fdb.refs;
    fi_array[i++] = f->id;
  }

  /* add pkg to the pacakge table */
  q = sql_prep("INSERT INTO packages(name, shortname, version, arch, build, csize, usize, desc, location, files, doinst, time)"
               " VALUES(?,?,?,?,?,?,?,?,?,?,?,?);");
  sql_set_text(q, 1, pkg->name);
  sql_set_text(q, 2, pkg->shortname);
  sql_set_text(q, 3, pkg->version);
  sql_set_text(q, 4, pkg->arch);
  sql_set_text(q, 5, pkg->build);
  sql_set_int(q, 6, pkg->csize);
  sql_set_int(q, 7, pkg->usize);
  sql_set_text(q, 8, pkg->desc);
  sql_set_text(q, 9, pkg->location);
  sql_set_blob(q, 10, fi_array, fi_size*sizeof(*fi_array));
  sql_set_text(q, 11, pkg->doinst);
  sql_set_int64(q, 12, pkg->time);
  sql_step(q);
  sql_fini(q);

  g_free(fi_array);

  continue_timer(10);
  sql_pop_context(1);
  stop_timer(10);
  stop_timer(3);
  return 0;
 err_0:
  stop_timer(3);
  return 1;
}

struct db_pkg* db_get_pkg(gchar* name, gboolean files)
{
  sql_query *q;
  struct db_pkg* p=0;
  gint pid;
  continue_timer(2);

  _db_open_check(0)

  if (name == 0)
  {
    e_set(E_BADARG, "package name missing");
    goto err_0;
  }

  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    e_set(E_FATAL|DB_SQL, "%s", sql_error());
    sql_pop_context(0);
    db_free_pkg(p);
    goto err_0;
  }

  q = sql_prep("SELECT id, name, shortname, version, arch, build, csize,"
                   " usize, desc, location, files, doinst, time FROM packages WHERE name == '%q';", name);
  if (!sql_step(q))
  {
    e_set(E_ERROR|DB_NOTEX, "package is NOT in database (%s)", name);
    sql_pop_context(0);
    goto err_0;
  }

  p = g_new0(struct db_pkg, 1);
  pid = sql_get_int(q, 0);
  p->name = g_strdup(name);
  p->shortname = g_strdup(sql_get_text(q, 2));
  p->version = g_strdup(sql_get_text(q, 3));
  p->arch = g_strdup(sql_get_text(q, 4));
  p->build = g_strdup(sql_get_text(q, 5));
  p->csize = sql_get_int(q, 6);
  p->usize = sql_get_int(q, 7);
  p->time = sql_get_int64(q, 12);
  p->desc = g_strdup(sql_get_text(q, 8));
  p->location = g_strdup(sql_get_text(q, 9));
  p->doinst = g_strdup(sql_get_text(q, 11));

  /* caller don't want files list, so it's enough here */
  if (files == 0)
    goto ok;

  guint fi_size = sql_get_size(q, 10)/sizeof(guint32);
  guint32 *fi_array = (guint32*)sql_get_blob(q, 10);
  
  guint i;
  struct fdb_file f;
  struct db_file* file;
  for (i=0; i<fi_size; i++)
  {
    fdb_get_file(_db.fdb, fi_array[i], &f);
    file = g_new0(struct db_file, 1);
    file->path = g_strdup(f.path);
    file->link = f.link?g_strdup(f.link):0;
    file->id = fi_array[i];
    p->files = g_slist_prepend(p->files, file);
  }
  p->files = g_slist_reverse(p->files);

 ok:
  sql_pop_context(0);
  stop_timer(2);
  return p;
 err_0:
  stop_timer(2);
  return 0;
}

gint db_rem_pkg(gchar* name)
{
  sql_query *q;
  gint pid;
  
  _db_open_check(1)

  if (name == 0)
  {
    e_set(E_BADARG, "name needed");
    return 1;
  }
  
  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    e_set(E_FATAL|DB_SQL, "%s", sql_error());
    sql_pop_context(0);
    return 1;
  }

  /* check if package is in db */
  q = sql_prep("SELECT id,files FROM packages WHERE name == '%q';", name);
  if (!sql_step(q))
  { /* if package does not exists */
    e_set(E_ERROR|DB_NOTEX, "package is NOT in database (%s)", name);
    sql_pop_context(0);
    return 1;
  }
  pid = sql_get_int(q, 0);

  guint fi_size = sql_get_size(q, 1)/sizeof(guint32);
  guint32 *fi_array = (guint32*)sql_get_blob(q, 1);
  guint i;
  for (i=0; i<fi_size; i++)
    fdb_rem_file(_db.fdb, fi_array[i]);
  
  sql_fini(q);

  /* remove package from packages table */
  sql_exec("DELETE FROM packages WHERE id == %d;", pid);

  sql_pop_context(1);
  return 0;
}

gint db_legacy_add_pkg(struct db_pkg* pkg)
{
  GSList* l;
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

  ppath = g_strdup_printf("%s/packages.spkg/%s", _db.topdir, pkg->name);
  spath = g_strdup_printf("%s/scripts.spkg/%s", _db.topdir, pkg->name);

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
    "COMPRESSED PACKAGE SIZE:   %d K\n"
    "UNCOMPRESSED PACKAGE SIZE: %d K\n"
    "PACKAGE LOCATION:          %s\n"
    "PACKAGE DESCRIPTION:\n"
    "%s"
    "FILE LIST:\n",
    pkg->name, pkg->csize, pkg->usize, pkg->location?pkg->location:"", pkg->desc?pkg->desc:""
  );
  
  if (pkg->doinst)
    fprintf(sf, "%s", pkg->doinst);

  /* construct filelist and script for links creation */
  for (l=pkg->files; l!=0; l=l->next)
  {
    struct db_file* f = l->data;
    if (!f->link)
      fprintf(pf, "%s\n", f->path);
  }

  struct utimbuf dt = { pkg->time, pkg->time };
  if (utime(ppath, &dt) == -1)
  {
    e_set(E_ERROR, "can't utime package entry: %s", strerror(errno));
    goto err_3;
  }

  ret = 0;
 err_3: 
  fclose(sf);
 err_2:
  fclose(pf);
 err_1:
  g_free(ppath);
  g_free(spath);
 err_0:
  stop_timer(5);
  return ret;
}

struct db_pkg* db_legacy_get_pkg(gchar* name, gboolean files)
{
  gint fp, fs;
  gchar *ap, *as=0;
  gsize sp, ss=0;
  struct db_pkg* p=0;
  gchar *tmpstr;

  if (name == 0)
    return 0;

  _db_open_check(0)

  continue_timer(4);

  /* open legacy package db entries */  
  tmpstr = g_strdup_printf("%s/packages/%s", _db.topdir, name);
  fp = open(tmpstr, O_RDONLY);
  time_t mtime = sys_file_mtime(tmpstr,0);
  g_free(tmpstr);
  if (fp == -1) /* main package entry can't be open */
  {
    e_set(E_ERROR, "can't open main package entry file: %s", strerror(errno));
    goto err_0;
  }
  if (mtime == (time_t)-1) /* package time can't be retrieved */
  {
    e_set(E_ERROR, "can't get main package entry file mtime: %s", strerror(errno));
    goto err_0;
  }
  sp = lseek(fp, 0, SEEK_END)+1;
  ap = mmap(0, sp, PROT_READ, MAP_SHARED, fp, 0);
  if (ap == (void*)-1)
  {
    e_set(E_ERROR, "can't mmap main package entry file: %s", strerror(errno));
    close(fp);
    goto err_0;
  }

  tmpstr = g_strdup_printf("%s/scripts/%s", _db.topdir, name);
  fs = open(tmpstr, O_RDONLY);
  g_free(tmpstr);
  if (fs != -1) /* script entry can't be open */
  {
    ss = lseek(fs, 0, SEEK_END)+1;
    as = mmap(0, ss, PROT_READ, MAP_SHARED, fs, 0);
    if (as == (void*)-1)
    {
      close(fs);
      fs = -1;
    }
  }

  p = g_new0(struct db_pkg, 1);
  p->name = g_strdup(name);
  p->shortname = parse_pkgname(p->name, 1); /*XXX: no checking here */
  p->version = parse_pkgname(p->name, 2);
  p->arch = parse_pkgname(p->name, 3);
  p->build = parse_pkgname(p->name, 4);
  p->time = mtime;

  /* parse main package file */
  gint snl = strlen(p->shortname);
  gint m[5] = {0}; /* if particular line was matched it can't 
  occur anymore, so we cache info about already matched lines */

  gchar *b, *e, *ln, *n=ap;
  while(iter_lines(&b, &e, &n, 0))
  {
#define LINEMATCH(s) (strncmp(b, s, sizeof(s)-1) == 0)
#define LINESIZE(s) (sizeof(s)-1)
    if (!m[0] && LINEMATCH("PACKAGE NAME:"))
    {
      gchar* name = b+LINESIZE("PACKAGE NAME:");
      name = g_strstrip(g_strndup(name, e-name+1));
      g_free(name);
      /* skip whitespace */
      m[0] = 1;
    }
    else if (!m[1] && LINEMATCH("COMPRESSED PACKAGE SIZE:"))
    {
      gchar* size = b+LINESIZE("COMPRESSED PACKAGE SIZE:");
      if (sscanf(size, " %u ", &p->csize) != 1)
      {
        e_set(E_ERROR, "can't read compressed package size");
        goto err_1;
      }
      m[1] = 1;
    }
    else if (!m[2] && LINEMATCH("UNCOMPRESSED PACKAGE SIZE:"))
    {
      gchar* size = b+LINESIZE("UNCOMPRESSED PACKAGE SIZE:");
      if (sscanf(size, " %u ", &p->usize) != 1)
      {
        e_set(E_ERROR, "can't read compressed package size");
        goto err_1;
      }
      m[2] = 1;
    }
    else if (!m[3] && LINEMATCH("PACKAGE LOCATION:"))
    {
      gchar* loc = b+LINESIZE("PACKAGE LOCATION:");
      loc = g_strstrip(g_strndup(loc, e-loc+1));
      p->location = loc;
      m[3] = 1;
    }
    else if (!m[4] && LINEMATCH("PACKAGE DESCRIPTION:"))
      m[4] = 1;
    else if (strncmp(b, p->shortname, snl) == 0)
    {
      gchar* ln = g_strndup(b, e-b+1);
      gchar* ndesc = g_strconcat(p->desc?p->desc:"", ln, "\n", 0);
      g_free(ln);
      g_free(p->desc);
      p->desc = ndesc;
    }
    if (LINEMATCH("FILE LIST:"))
      goto parse_files;
  }
  goto err_1;

 parse_files:
  if (!files)
    goto fini;

  while(iter_lines(&b, &e, &n, &ln))
    p->files = g_slist_prepend(p->files, db_alloc_file(ln,0));

  if (fs == -1)
    goto fini;

  n = as;
  while(iter_lines(&b, &e, &n, &ln))
  {
    gchar* dir;
    gchar* link;
    gchar* target;
    if (parse_createlink(ln, &dir, &link, &target))
    {
      gchar* path = g_strdup_printf("%s/%s", dir, link);
      g_free(dir);
      g_free(link);
      p->files = g_slist_prepend(p->files, db_alloc_file(path,target));
    }
    g_free(ln);
  }

  p->doinst = g_strndup(as,ss-1);

 fini:
  p->files = g_slist_reverse(p->files);
  goto no_err;

 err_1:
  db_free_pkg(p);
  p=0;

 no_err:
  if (fs != -1)
  {
    munmap(as, ss);
    close(fs);
  }
  munmap(ap, sp);
  close(fp);
 err_0:
  stop_timer(4);
  return p;
}

gint db_legacy_rem_pkg(gchar* name)
{
  _db_open_check(1)

  gchar* p = g_strdup_printf("%s/packages/%s", _db.topdir, name);
  gchar* s = g_strdup_printf("%s/scripts/%s", _db.topdir, name);
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

GSList* db_get_packages()
{
  sql_query *q;
  GSList *pkgs=0;

  _db_open_check(0)

  continue_timer(7);
  
  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    e_set(E_FATAL|DB_SQL, "%s", sql_error());
    sql_pop_context(0);
    db_free_packages(pkgs);
    stop_timer(7);
    return 0;
  }

  q = sql_prep("SELECT id, name, shortname, version, arch, build, csize,"
               " usize, desc, location FROM packages;");
  while(sql_step(q))
  {
    struct db_pkg* p;
    p = g_new0(struct db_pkg, 1);
    p->id = sql_get_int(q, 0);
    p->name = g_strdup(sql_get_text(q, 1));
    p->shortname = g_strdup(sql_get_text(q, 2));
    p->version = g_strdup(sql_get_text(q, 3));
    p->arch = g_strdup(sql_get_text(q, 4));
    p->build = g_strdup(sql_get_text(q, 5));
    p->csize = sql_get_int(q, 6);
    p->usize = sql_get_int(q, 7);
    p->desc = g_strdup(sql_get_text(q, 8));
    p->location = g_strdup(sql_get_text(q, 9));

    pkgs = g_slist_prepend(pkgs, p);
  }

  sql_pop_context(0);
  stop_timer(7);
  return pkgs;
}

GSList* db_legacy_get_packages()
{
  GSList *pkgs=0;

  _db_open_check(0)

  continue_timer(8);

  gchar* pdir = g_strdup_printf("%s/packages", _db.topdir);
  DIR* d = opendir(pdir);
  g_free(pdir);
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
    struct db_pkg* p = db_legacy_get_pkg(de->d_name,1);
    if (p == 0)
    {
      e_set(E_FATAL, "can't get legacy package");
      goto err_1;
    }
    pkgs = g_slist_prepend(pkgs, p);
  }
  closedir(d);

  stop_timer(8);
  return pkgs;
 err_1:
  db_free_packages(pkgs);
  closedir(d);
 err_0:
  stop_timer(8);
  return 0;
}

void db_free_packages(GSList* pkgs)
{
  GSList* l;
  continue_timer(9);
  if (pkgs == 0)
  {
    stop_timer(9);
    return;
  }
  for (l=pkgs; l!=0; l=l->next)
    db_free_pkg(l->data);
  g_slist_free(pkgs);
  stop_timer(9);
}

gint db_sync_to_legacydb()
{
  GSList *pkgs, *l;
  gint ret = 1;

  _db_open_check(1)

  pkgs = db_get_packages();
  if (pkgs == 0 && e_ok(_db.err))
    return 0; /* no packages */
  else if (pkgs == 0)
  {
    e_set(E_FATAL, "can't get package list");
    goto err_0;
  }
 
  for (l=pkgs; l!=0; l=l->next)
  { /* for each package */
    struct db_pkg* pkg = l->data;
    struct db_pkg* p = db_get_pkg(pkg->name,1);
    if (p == 0)
    {
      e_set(E_FATAL, "can't get package from database");
      goto err_1;
    }
    if (db_legacy_add_pkg(p))
    {
      e_set(E_FATAL, "can't add package to legacy database");
      goto err_1;
    }
    db_free_pkg(p);
  }

  ret = 0;
 err_1:
  db_free_packages(pkgs);
 err_0:
  return ret;
}

gint db_sync_from_legacydb()
{
  DIR* d;
  struct dirent* de;
  gchar* tmpstr = g_strdup_printf("%s/%s", _db.topdir, "packages");
  gint ret = 1;

  _db_open_check(1)

  d = opendir(tmpstr);
  g_free(tmpstr);
  if (d == NULL)
  {
    e_set(E_FATAL, "legacy database directory not found");
    goto err_0;
  }
  
  sql_exec("DELETE FROM packages;"); /*XXX: unchecked (will exit on error) */

  while ((de = readdir(d)) != NULL)
  {
    struct db_pkg* p=0;
    if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;
    p = db_legacy_get_pkg(de->d_name,1);
    if (p == 0)
    {
      e_set(E_FATAL, "can't get legacy package");
      goto err_1;
    }
    if (db_add_pkg(p))
    {
      e_set(E_FATAL, "can't add package to database");
      db_free_pkg(p);
      goto err_1;
    }
    db_free_pkg(p);
  }

  ret = 0;
 err_1:
  closedir(d);
 err_0:
  return ret;
}
