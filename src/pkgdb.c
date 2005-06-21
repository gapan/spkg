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
#include <errno.h>

#include "sql.h"
#include "sys.h"
#include "pkgname.h"
#include "filedb.h"
#include "pkgdb.h"
#include "bench.h"

/* private 
 ************************************************************************/

static gboolean _db_is_open = 0;
static gchar* _db_topdir = 0;
static gchar* _db_dbfile = 0;
static gchar* _db_dbroot = 0;
static gchar* _db_errstr = 0;
static gint   _db_errno = 0;

static __inline__ void _db_reset_error()
{
  if (G_UNLIKELY(_db_errstr != 0))
  {
    g_free(_db_errstr);
    _db_errstr = 0;
  }
  _db_errno = DB_OK;
}

#define _db_open_check(v) \
  if (!_db_is_open) \
  { \
    _db_set_error(DB_CLOSED, "trying to access closed package database"); \
    return v; \
  }

static void _db_set_error(gint n, const gchar* fmt, ...)
{
  va_list ap;
  _db_reset_error();
  _db_errno = n;
  va_start(ap, fmt);
  _db_errstr = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  _db_errstr = g_strdup_printf("error[pkgdb]: %s", _db_errstr);
}

/* public 
 ************************************************************************/

gint db_open(const gchar* root)
{
  gchar** d;
  gchar* checkdirs[] = {
    "packages", "scripts", "removed_packages", "removed_scripts", "setup", 
    "spkgdb", "packages.spkg", "scripts.spkg", 0
  };

  reset_timers();
  continue_timer(0);
  
  _db_reset_error();
  if (_db_is_open)
  {
    _db_set_error(DB_OPEN, "can't open package database (it is already open)");
    return 1;
  }
  
  if (root == 0)
    root = "/";

  if (root[0] == '/')
    _db_topdir = g_strdup_printf("%s/%s", root, PKGDB_DIR);
  else
  {
    gchar* cwd = getcwd(0,0);
    _db_topdir = g_strdup_printf("%s/%s/%s", cwd, root, PKGDB_DIR); /*XXX: not portable */
    free(cwd);
  }

  /* check legacy and spkg db dirs */
  for (d = checkdirs; *d != 0; d++)
  {
    gchar* tmpdir = g_strdup_printf("%s/%s", _db_topdir, *d);
    /* if it is not a directory, clean it and create it */
    if (sys_file_type(tmpdir,1) != SYS_DIR)
    {
      sys_rm_rf(tmpdir);
      sys_mkdir_p(tmpdir);
      chmod(tmpdir, 0755);
      /* if it is still not a directory, return with error */
      if (sys_file_type(tmpdir,1) != SYS_DIR)
      {
        _db_set_error(DB_OTHER, "can't open package database (%s should be an accessible directory)", tmpdir);
        g_free(tmpdir);
        goto err0;
      }
    }
    g_free(tmpdir);
  }

  /* check spkg db file */
  _db_dbroot = g_strdup_printf("%s/%s", _db_topdir, "spkgdb");
  _db_dbfile = g_strdup_printf("%s/%s", _db_dbroot, "spkg.db");
  if (sys_file_type(_db_dbfile,0) != SYS_REG && sys_file_type(_db_dbfile,0) != SYS_NONE)
  {
    _db_set_error(DB_OTHER, "can't open package database (%s is not accessible)", _db_dbfile);
    goto err1;
  }

  /* open file database */
  if (fdb_open(_db_dbroot))
  {
    _db_set_error(DB_OTHER, "can't open file database\n%s", fdb_error());
    goto err1;
  }

  /* open sql database */
  if (sql_open(_db_dbfile))
  {
    _db_set_error(DB_OTHER, "can't open package database (sql open failed)\n");
    goto err1;
  }

  /* setup sql error handling */
  sql_push_context(SQL_ERRJUMP,0);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    _db_set_error(DB_OTHER, "can't open package database (sql error)\n%s", sql_error());
    goto err2;
  }
  
  if (!sql_integrity_check())
  { /* sql exception occured */
    _db_set_error(DB_OTHER, "can't open package database (database is corrupted)");
    goto err2;
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
  _db_is_open = 1;
  stop_timer(0);
  return 0;

 err2:
  sql_close();
 err1:
  g_free(_db_dbfile);
  g_free(_db_dbroot);
 err0:
  g_free(_db_topdir);
  return 1;
}

void db_close()
{
  continue_timer(1);
  _db_reset_error();
  fdb_close();
  sql_close();
  g_free(_db_dbfile);
  g_free(_db_topdir);
  g_free(_db_dbroot);
  g_blow_chunks();
  _db_is_open = 0;
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

gint db_errno()
{
  return _db_errno;
}

gchar* db_error()
{
  return _db_errstr;
}

struct db_pkg* db_alloc_pkg(gchar* name)
{
  struct db_pkg* p;

  if (name == 0 || !parse_pkgname(name, 6))
    return 0;
    
  p = g_new0(struct db_pkg, 1);
  p->name = parse_pkgname(name, 5);
  p->shortname = parse_pkgname(name, 1);
  p->version = parse_pkgname(name, 2);
  p->arch = parse_pkgname(name, 3);
  p->build = parse_pkgname(name, 4);
  return p;
}

void db_free_pkg(struct db_pkg* pkg)
{
  continue_timer(6);
  struct db_pkg* p = pkg;
  GSList* l;
  if (p == 0)
    return;
  if (p->files) {
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

  _db_reset_error();
  _db_open_check(1)
  
  /* check if pkg contains everthing required */
  if (pkg == 0 || pkg->name == 0 || pkg->shortname == 0 
      || pkg->version == 0 || pkg->build == 0 || pkg->files == 0)
  {
    _db_set_error(DB_OTHER, "can't add package to the database (incomplete package structure)");
    goto err_0;
  }
  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    _db_set_error(DB_OTHER, "can't add package to the database (sql error)\n%s", sql_error());
    sql_pop_context(0);
    goto err_0;
  }

  /* check if package already exists in db */
  q = sql_prep("SELECT id FROM packages WHERE name == '%q';", pkg->name);
  if (sql_step(q))
  { /* if package exists */
    _db_set_error(DB_EXIST, "can't add package to the database (same package is already there - %s)", pkg->name);
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
    f->id = fdb_add_file(&fdb);
    f->refs = fdb.refs;
    fi_array[i++] = f->id;
  }

  /* add pkg to the pacakge table */
  q = sql_prep("INSERT INTO packages(name, shortname, version, arch, build, csize, usize, desc, location, files, doinst)"
               " VALUES(?,?,?,?,?,?,?,?,?,?,?);");
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

  _db_reset_error();
  _db_open_check(0)

  if (name == 0)
  {
    _db_set_error(DB_OTHER, "can't retrieve package from the database (name not given)");
    goto err_0;
  }

  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    _db_set_error(DB_OTHER, "can't retrieve package from the database (sql error)\n%s", sql_error());
    sql_pop_context(0);
    db_free_pkg(p);
    goto err_0;
  }

  q = sql_prep("SELECT id, name, shortname, version, arch, build, csize,"
                   " usize, desc, location, files, doinst FROM packages WHERE name == '%q';", name);
  if (!sql_step(q))
  {
    _db_set_error(DB_NOTEX, "can't retrieve package from the database (package is not there - %s)", name);
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
    fdb_get_file(fi_array[i], &f);
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
  
  _db_reset_error();
  _db_open_check(1)

  if (name == 0)
  {
    _db_set_error(DB_OTHER, "can't remove package from the database (name not given)");
    return 1;
  }
  
  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    _db_set_error(DB_OTHER, "can't remove package from the database (sql error)\n%s", sql_error());
    sql_pop_context(0);
    return 1;
  }

  /* check if package is in db */
  q = sql_prep("SELECT id,files FROM packages WHERE name == '%q';", name);
  if (!sql_step(q))
  { /* if package does not exists */
    _db_set_error(DB_NOTEX, "can't remove package from the database (package is not there - %s)", name);
    sql_pop_context(0);
    return 1;
  }
  pid = sql_get_int(q, 0);

  guint fi_size = sql_get_size(q, 1)/sizeof(guint32);
  guint32 *fi_array = (guint32*)sql_get_blob(q, 1);
  guint i;
  for (i=0; i<fi_size; i++)
    fdb_del_file(fi_array[i]);
  
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

  _db_reset_error();
  _db_open_check(1)

  /* check if pkg contains everthing required */
  if (pkg == 0 || pkg->name == 0 || pkg->files == 0)
  {
    _db_set_error(DB_OTHER, "can't add package to the legacy database (incomplete package structure)");
    goto err_0;
  }

  ppath = g_strdup_printf("%s/packages.spkg/%s", _db_topdir, pkg->name);
  spath = g_strdup_printf("%s/scripts.spkg/%s", _db_topdir, pkg->name);

  pf = fopen(ppath, "w");
  if (pf == 0)
  {
    _db_set_error(DB_OTHER, "can't add package to the legacy database (can't open package file %s)", strerror(errno));
    goto err_1;
  }
  sf = fopen(spath, "w");
  if (sf == 0)
  {
    _db_set_error(DB_OTHER, "can't add package to the legacy database (can't open script file)");
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

  ret = 0;
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

  continue_timer(4);

  /* open legacy package db entries */  
  tmpstr = g_strdup_printf("%s/packages/%s", _db_topdir, name);
  fp = open(tmpstr, O_RDONLY);
  g_free(tmpstr);
  if (fp == -1) /* main package entry can't be open */
    goto err_0;
  sp = lseek(fp, 0, SEEK_END)+1;
  ap = mmap(0, sp, PROT_READ, MAP_SHARED, fp, 0);
  if (ap == (void*)-1)
  {
    close(fp);
    goto err_0;
  }

  tmpstr = g_strdup_printf("%s/scripts/%s", _db_topdir, name);
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
      sscanf(size, " %u ", &p->csize); /*XXX: no checking here */
       m[1] = 1;
    }
    else if (!m[2] && LINEMATCH("UNCOMPRESSED PACKAGE SIZE:"))
    {
      gchar* size = b+LINESIZE("UNCOMPRESSED PACKAGE SIZE:");
      sscanf(size, " %u ", &p->usize); /*XXX: no checking here */
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
  gchar* p = g_strdup_printf("%s/packages/%s", _db_topdir, name);
  gchar* s = g_strdup_printf("%s/scripts/%s", _db_topdir, name);
  gint ret = 1;
  if (sys_file_type(p, 0) == SYS_REG)
  {
    unlink(p);
    if (sys_file_type(s, 0) == SYS_REG)
      unlink(p);
    ret = 0;
  }
  g_free(p);
  g_free(s);
  return ret;
}

GSList* db_get_packages()
{
  sql_query *q;
  GSList *pkgs=0;

  _db_reset_error();
  _db_open_check(0)

  continue_timer(7);
  
  /* sql error handler */
  sql_push_context(SQL_ERRJUMP,1);
  if (setjmp(sql_errjmp) == 1)
  { /* sql exception occured */
    _db_set_error(DB_OTHER, "can't get packages from the database (sql error)\n%s", sql_error());
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

  _db_reset_error();
  _db_open_check(0)

  continue_timer(8);

  gchar* pdir = g_strdup_printf("%s/packages", _db_topdir);
  DIR* d = opendir(pdir);
  g_free(pdir);
  if (d == NULL)
  {
    _db_set_error(DB_OTHER, "can't get legacy database packages (can't open legacy db directory)");
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
      gchar* err = g_strdup(db_error());
      _db_set_error(DB_OTHER, "can't get legacy database packages (internal error)\n%s", err);
      g_free(err);
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

  _db_reset_error();
  _db_open_check(1)

  pkgs = db_get_packages();
  if (pkgs == 0 && db_error() == 0)
    return 0; /* no packages */
  else if (pkgs == 0)
  {
    gchar* err = g_strdup(db_error());
    _db_set_error(DB_OTHER, "can't synchronize database (internal error)\n%s", err);
    g_free(err);
    goto err_0;
  }
 
  for (l=pkgs; l!=0; l=l->next)
  { /* for each package */
    struct db_pkg* pkg = l->data;
    struct db_pkg* p = db_get_pkg(pkg->name,1);
    if (p == 0)
    {
      gchar* err = g_strdup(db_error());
      _db_set_error(DB_OTHER, "can't synchronize database (internal error)\n%s", err);
      g_free(err);
      goto err_1;
    }
    if (db_legacy_add_pkg(p))
    {
      gchar* err = g_strdup(db_error());
      _db_set_error(DB_OTHER, "can't synchronize database (internal error)\n%s", err);
      g_free(err);
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
  gchar* tmpstr = g_strdup_printf("%s/%s", _db_topdir, "packages");
  gint ret = 1;

  _db_reset_error();
  _db_open_check(1)

  d = opendir(tmpstr);
  g_free(tmpstr);
  if (d == NULL)
  {
    _db_set_error(DB_OTHER, "can't synchronize database (legacy database directory not found)");
    goto err_0;
  }
  
  sql_exec("DELETE FROM packages;"); /*XXX: "unchecked" */

  while ((de = readdir(d)) != NULL)
  {
    struct db_pkg* p=0;
    if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
      continue;
    p = db_legacy_get_pkg(de->d_name,1);
    if (p == 0)
    {
      gchar* err = g_strdup(db_error());
      _db_set_error(DB_OTHER, "can't synchronize database (internal error)\n%s", err);
      g_free(err);
      goto err_1;
    }
    if (db_add_pkg(p))
    {
      gchar* err = g_strdup(db_error());
      _db_set_error(DB_OTHER, "can't synchronize database (internal error)\n%s", err);
      g_free(err);
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
