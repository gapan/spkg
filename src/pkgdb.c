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
#include <utime.h>
#include <errno.h>
#include <zlib.h>

#include <Judy.h>

#include "sys.h"
#include "misc.h"
#include "path.h"
#include "pkgdb.h"
#include "bench.h"

/* private 
 ************************************************************************/

struct db_state {
  gboolean is_open;
  gboolean filelist_loaded;
  gchar* topdir;
  gchar* pkgdir; /* /var/log/packages/ */
  gchar* scrdir; /* /var/log/scripts/ */
  struct error* err;
  void* paths;
  gint fd_lock;
};

static struct db_state _db = { 0 };

#define e_set(n, fmt, args...) e_add(_db.err, "pkgdb", __func__, n, fmt, ##args)

#define _db_open_check(v) \
  if (!_db.is_open) \
  { \
    e_set(E_ERROR|DB_NOPEN, "Package database is NOT open."); \
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
    "removed_scripts", "setup", NULL };

  g_assert(e != NULL);
  _db.err = e;
  
  if (_db.is_open)
  {
    e_set(E_ERROR|DB_OPEN, "Package database is already open.");
    goto err_0;
  }
  
  gchar* sane_root = sanitize_root_path(root);
  if (g_path_is_absolute(sane_root))
    _db.topdir = g_strdup_printf("%s%s", sane_root, PKGDB_DIR);
  else
  {
    gchar* cwd = g_get_current_dir();
    _db.topdir = g_strdup_printf("%s/%s%s", cwd, sane_root, PKGDB_DIR);
    g_free(cwd);
  }
  _db.pkgdir = g_strdup_printf("%s/packages", _db.topdir);
  _db.scrdir = g_strdup_printf("%s/scripts", _db.topdir);
  g_free(sane_root);

  /* check db dirs */
  for (d = checkdirs; *d != NULL; d++)
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
        e_set(E_FATAL, "Can't access package database directory. (%s)", tmpdir);
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
    e_set(E_FATAL, "Can't create lock.");
    goto err_1;
  }
  if (sys_lock_trywait(_db.fd_lock, 20, e))
  {
    e_set(E_FATAL, "Can't lock package database.");
    goto err_2;
  }

  _db.is_open = 1;
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
  if (!_db.is_open)
  {
    e_set(E_ERROR, "Package database is NOT open.");
    return;
  }

  sys_lock_del(_db.fd_lock);

  Word_t used;
  JSLFA(used, _db.paths);

  g_free(_db.topdir);
  g_free(_db.pkgdir);
  g_free(_db.scrdir);
  memset(&_db, 0, sizeof(_db));
}

/* public - filelist
 ************************************************************************/

void db_filelist_rem_pkg_paths(const struct db_pkg* pkg)
{
  gchar path[MAXPATHLEN];
  gint rc;
  Word_t *p1, *p2;

  strcpy(path, "");
  JSLF(p1, pkg->paths, path);
  while (p1 != NULL)
  {
    JSLG(p2, _db.paths, path);
    if (p2)
    {
      Word_t* refs = p2;
      if (*refs == 1)
      {
        JSLD(rc, _db.paths, path);
      }
      else
      {
        (*refs)--;
      }
    }
    JSLN(p1, pkg->paths, path);
  }
}

void db_filelist_add_pkg_paths(const struct db_pkg* pkg)
{
  gchar path[MAXPATHLEN];
  Word_t *p1, *p2;

  strcpy(path, "");
  JSLF(p1, pkg->paths, path);
  while (p1 != NULL)
  {
    JSLI(p2, _db.paths, path);
    (*p2)++;
    JSLN(p1, pkg->paths, path);
  }
}

gint db_filelist_load(gboolean force_reload)
{
  _db_open_check(1)

  if (_db.filelist_loaded && !force_reload)
    return 0;

  DIR* d = opendir(_db.pkgdir);
  if (d == NULL)
  {
    e_set(E_FATAL, "Can't open db directory. (%s)", strerror(errno));
    goto err_0;
  }
  gchar sbuf[1024*128];

  db_filelist_free();

  gchar* line = NULL;
  gint size = 0;
  Word_t* refs;
  struct dirent* de;

  /* for each package database entry */
  while ((de = readdir(d)) != NULL)
  {
    gchar* name = de->d_name;
    /* is this valid package database entry file? */
    if (parse_pkgname(name, 6) == NULL)
      continue;
    
    /* open it */
    gchar* tmpstr = g_strdup_printf("%s/%s", _db.pkgdir, name);
    FILE* f = fopen(tmpstr, "r");
    g_free(tmpstr);
    if (f == NULL) /* main package entry can't be open */
    {
      e_set(E_ERROR, "Can't open package database file %s. (%s)", name, strerror(errno));
      goto err_1;
    }
    /* set bigger buffer */
    setvbuf(f, sbuf, _IOFBF, sizeof(sbuf));

    /* for each line */
    gint linelen;
    gint files = 0;
    while ((linelen = getline(&line, &size, f)) >= 0)
    {
      /* remove new line character from the end of the line */
      if (linelen > 0 && line[linelen-1] == '\n')
        line[linelen-1] = '\0', linelen--;
      /* if we are in the files section, add path to the list */
      if (files)
      {
        /* remove trailing / character from the end of the line */
        if (linelen > 0 && line[linelen-1] == '/')
          line[linelen-1] = '\0', linelen--;
        /* check path size limit */
        if (linelen >= MAXPATHLEN)
        {
          fclose(f);
          strcpy(line+100, "...");
          e_set(E_ERROR, "Path too long in the package database file %s. (%s)", name, line);
          goto err_1;
        }
        JSLI(refs, _db.paths, line);
        (*refs)++;
      }
      else if (LINEMATCH("FILE LIST:"))
        files++;
    }
    fclose(f);

    /* open and parse installation script for symbolic links */
    tmpstr = g_strdup_printf("%s/%s", _db.scrdir, name);
    f = fopen(tmpstr, "r");
    g_free(tmpstr);
    if (f == NULL) /* script package entry can't be open */
      continue; /* ignore if can't be open (may not exist) XXX: check if errno is NOTEX */
    setvbuf(f, sbuf, _IOFBF, sizeof(sbuf));

    /* for each line */
    while ((linelen = getline(&line, &size, f)) >= 0)
    {
      /* remove new line character from the end of the line */
      if (linelen > 0 && line[linelen-1] == '\n')
        line[linelen-1] = '\0', linelen--;

      /* parse create link line */
      gchar *dir, *link, *target;
      if (parse_createlink(line, &dir, &link, &target))
      {
        gchar* path = g_strdup_printf("%s/%s", dir, link);
        gchar* sane_path = path_simplify(path);
        g_free(path);
        g_free(dir);
        g_free(link);
        g_free(target);
        /* add */
        /* check path size limit */
        if (strlen(sane_path) >= MAXPATHLEN)
        {
          strcpy(sane_path+100, "...");
          e_set(E_ERROR, "Path too long in the package database file %s. (%s)", name, line);
          fclose(f);
          g_free(sane_path);
          goto err_1;
        }
        JSLI(refs, _db.paths, sane_path);
        (*refs)++;
        g_free(sane_path);
      }
    }
    fclose(f);
  }

  if (line)
    free(line);

  closedir(d);
  _db.filelist_loaded = TRUE;
  return 0;
 err_1:
  db_filelist_free();
  closedir(d);
 err_0:
  return 1;
}

gulong db_filelist_get_path_refs(const gchar* path)
{
  Word_t* refs;
  JSLG(refs, _db.paths, path);
  if (refs == NULL)
    return 0;
  return (gulong)(*refs);
}

void db_filelist_free()
{
  Word_t used;
  JSLFA(used, _db.paths);
  _db.filelist_loaded = FALSE;
}

/* public - memmory management
 ************************************************************************/

struct db_pkg* db_alloc_pkg(gchar* name)
{
  struct db_pkg* p;
  if (name == NULL)
  {
    e_set(E_ERROR, "Package name can't be NULL.");
    return NULL;
  }
  if (parse_pkgname(name, 6) != (gchar*)-1)
  {
    e_set(E_ERROR, "Invalid package name. (%s)", name);
    return NULL;
  }
  p = g_new0(struct db_pkg, 1);
  p->name = parse_pkgname(name, 5);
  p->shortname = parse_pkgname(name, 1);
  p->version = parse_pkgname(name, 2);
  p->arch = parse_pkgname(name, 3);
  p->build = parse_pkgname(name, 4);
  p->time = time(NULL);
  return p;
}

void db_free_pkg(struct db_pkg* pkg)
{
  if (pkg == NULL)
    return;

  Word_t freed;
  JSLFA(freed, pkg->paths);
  g_free(pkg->name);
  g_free(pkg->shortname);
  g_free(pkg->version);
  g_free(pkg->arch);
  g_free(pkg->build);
  g_free(pkg->location);
  g_free(pkg->desc);
  g_free(pkg->doinst);
  memset(pkg, 0, sizeof(*pkg));
  g_free(pkg);
}

gint db_pkg_add_path(struct db_pkg* pkg, const gchar* path, db_path_type type)
{
  Word_t* ptype;
  /* check path size limit */
  if (strlen(path) >= MAXPATHLEN)
    return 1;
  JSLI(ptype, pkg->paths, path);
  *ptype = type;
  return 0;
}

db_path_type db_pkg_get_path(struct db_pkg* pkg, const gchar* path)
{
  Word_t* ptype;
  JSLG(ptype, pkg->paths, path);
  if (ptype != NULL)
    return (db_path_type)(*ptype);
  return DB_PATH_NONE;
}

/* public - main database package operations
 ************************************************************************/

static gint _db_add_pkg(struct db_pkg* pkg, gchar* origname)
{
  FILE *pf, *sf;
  gchar *ppath, *spath, *ppath_tmp, *spath_tmp;

  _db_open_check(1)

  /* check if pkg contains everthing required */
  if (pkg == NULL || pkg->name == NULL || pkg->paths == NULL)
  {
    e_set(E_BADARG, "Incomplete package structure.");
    goto err_0;
  }

  ppath = g_strdup_printf("%s/%s", _db.pkgdir, pkg->name);
  spath = g_strdup_printf("%s/%s", _db.scrdir, pkg->name);
  ppath_tmp = g_strdup_printf("%s/.%s", _db.pkgdir, pkg->name);
  spath_tmp = g_strdup_printf("%s/.%s", _db.scrdir, pkg->name);

  /* check if package file exists */
  if (origname == NULL)
  {
    if (sys_file_type(ppath, 1) != SYS_NONE)
    {
      e_set(E_FATAL|DB_EXIST, "Package is already in the database %s. (%s)", ppath, strerror(errno));
      goto err_1;
    }
  }

  /* write package file (tmp) */
  pf = fopen(ppath_tmp, "w");
  if (pf == NULL)
  {
    e_set(E_FATAL, "Can't open package file %s. (%s)", ppath_tmp, strerror(errno));
    goto err_1;
  }
  fchmod(fileno(pf), 0644);

  /* construct header */
  if (fprintf(pf,
    "PACKAGE NAME:              %s\n"
    "COMPRESSED PACKAGE SIZE:   %u K\n"
    "UNCOMPRESSED PACKAGE SIZE: %u K\n"
    "PACKAGE LOCATION:          %s\n"
    "PACKAGE DESCRIPTION:\n"
    "%s"
    "FILE LIST:\n",
    pkg->name, pkg->csize, pkg->usize,
    pkg->location ? pkg->location : "",
    pkg->desc ? pkg->desc : "") < 0)
  {
    goto err_2;
  }
  
  /* construct filelist */
  gchar path[MAXPATHLEN];
  gint* ptr;
  strcpy(path, "");
  JSLF(ptr, pkg->paths, path);
  while (ptr != NULL)
  {
    if (*ptr == DB_PATH_FILE)
    {
      if (fprintf(pf, "%s\n", path) < 0)
        goto err_2;
    }
    else if (*ptr == DB_PATH_DIR)
    {
      if (fprintf(pf, "%s/\n", path) < 0)
        goto err_2;
    }
    JSLN(ptr, pkg->paths, path);
  }

  /* close it */
  fclose(pf);

  /* change mtime */
  struct utimbuf dt = { pkg->time, pkg->time };
  if (utime(ppath_tmp, &dt) == -1)
  {
    e_set(E_ERROR, "Can't utime package entry %s. (%s)", ppath_tmp, strerror(errno));
    goto err_1;
  }

  /* wirte script file if necessary */
  if (pkg->doinst)
  {
    sf = fopen(spath_tmp, "w");
    if (sf == NULL)
    {
      e_set(E_FATAL, "Can't open script file %s. (%s)", spath_tmp, strerror(errno));
      goto err_1;
    }
    fchmod(fileno(sf), 0755);
    if (fprintf(sf, "%s", pkg->doinst) < 0)
    {
      e_set(E_FATAL, "Can't write into script file %s. (%s)", spath_tmp, strerror(errno));
      fclose(sf);
      goto err_1;
    }
    fclose(sf);
  }

  if (origname)
  {
    if (db_rem_pkg(origname))
    {
      e_set(E_FATAL, "Can't remove original package from the database. (%s)", origname);
      goto err_1;
    }
  }

  /* finally put package and script files into place */
  if (rename(ppath_tmp, ppath) < 0)
  {
    e_set(E_FATAL, "Can't finalize package database update. Rename failed %s. You'll need to fix package database by hand. (%s)", ppath_tmp, strerror(errno));
    goto err_1;
  }

  if (pkg->doinst)
  {
    if (rename(spath_tmp, spath) < 0)
    {
      e_set(E_FATAL, "Can't finalize package database update. Rename failed %s. You'll need to fix package database by hand. (%s)", spath_tmp, strerror(errno));
      goto err_1;
    }
  }

  g_free(ppath);
  g_free(spath);
  g_free(ppath_tmp);
  g_free(spath_tmp);
  return 0;

 err_2:
  e_set(E_FATAL, "Can't write into package file %s. (%s)", ppath_tmp, strerror(errno));
  fclose(pf);
 err_1:
  /* just ingore errors here */
  g_free(ppath);
  g_free(spath);
  g_free(ppath_tmp);
  g_free(spath_tmp);
 err_0:
  return 1;
}

gint db_add_pkg(struct db_pkg* pkg)
{
  return _db_add_pkg(pkg, NULL);
}

gint db_replace_pkg(gchar* origname, struct db_pkg* pkg)
{
  return _db_add_pkg(pkg, origname);
}

struct db_pkg* db_get_pkg(gchar* name, db_get_type type)
{
  FILE *fp, *fs;
  gchar *tmpstr;
  gchar stream_buf1[1024*128];
  gchar stream_buf2[1024*64];
  struct db_pkg* p = NULL;
  
  if (name == NULL)
  {
    e_set(E_BADARG, "Package name can't be NULL.");
    goto err_0;
  }
  if (parse_pkgname(name, 6) != (gchar*)-1)
  {
    e_set(E_ERROR, "Invalid package name. (%s)", name);
    goto err_0;
  }
  if (type != DB_GET_WITHOUT_FILES && type != DB_GET_FULL)
  {
    e_set(E_BADARG, "Invalid get type value. (must be DB_GET_WITHOUT_FILES or DB_GET_FULL)");
    goto err_0;
  }

  _db_open_check(NULL)

  /* open package db entries */  
  tmpstr = g_strdup_printf("%s/%s", _db.pkgdir, name);
  fp = fopen(tmpstr, "r");
  time_t mtime = sys_file_mtime(tmpstr, 1);
  g_free(tmpstr);
  if (fp == NULL) /* main package entry can't be open */
  {
    e_set(E_ERROR | (errno == ENOENT ? DB_NOTEX : 0), "Can't open package database entry file %s. (%s)", name, strerror(errno));
    goto err_0;
  }
  if (mtime == (time_t)-1) /* package time can't be retrieved */
  {
    e_set(E_ERROR, "Can't get package installation time for %s. (%s)", name, strerror(errno));
    goto err_0;
  }
  setvbuf(fp, stream_buf1, _IOFBF, sizeof(stream_buf1));

  /*XXX: better checks here (if NOTEX, or other error) */
  tmpstr = g_strdup_printf("%s/%s", _db.scrdir, name);
  fs = fopen(tmpstr, "r");
  g_free(tmpstr);
  if (fs)
    setvbuf(fs, stream_buf2, _IOFBF, sizeof(stream_buf2));

  p = db_alloc_pkg(name);
  if (p == NULL)
    goto err_1;
  p->time = mtime;

  /* parse main package file */
  gint snl = strlen(p->shortname);
  gint m[5] = { 0 }; /* if particular line was matched it can't 
  occur anymore, so we cache info about already matched lines */

  gchar* line = NULL;
  gint size = 0;
  gint linelen;
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
        e_set(E_ERROR, "Package file name doesn't match with package name. (%s != %s)", p->name, name);
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
        e_set(E_ERROR, "Can't parse compressed package size. (%s)", size);
        goto err_1;
      }
      m[1] = 1;
    }
    else if (!m[2] && LINEMATCH("UNCOMPRESSED PACKAGE SIZE:"))
    {
      gchar* size = line + LINESIZE("UNCOMPRESSED PACKAGE SIZE:");
      if (sscanf(size, " %u ", &p->usize) != 1)
      {
        e_set(E_ERROR, "Can't parse uncompressed package size. (%s)", size);
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
      gchar* desc = g_strconcat(p->desc ? p->desc : "", line, "\n", NULL);
      g_free(p->desc);
      p->desc = desc;
    }
    else if (LINEMATCH("FILE LIST:"))
    {
      goto parse_files;
    }
    else
    {
      e_set(E_ERROR, "Corrupt package database entry. (%s)", p->name);
      goto err_1;
    }
  }
  goto err_1;

 parse_files:
  if (type == DB_GET_WITHOUT_FILES)
    goto fini;

  while ((linelen = getline(&line, &size, fp)) >= 0)
  {
    gint* ptr;
    db_path_type type = DB_PATH_FILE;
    if (linelen > 0 && line[linelen-1] == '\n')
      line[linelen-1] = '\0', linelen--;
    if (linelen > 0 && line[linelen-1] == '/')
      line[linelen-1] = '\0', linelen--, type = DB_PATH_DIR;
    /* check path size limit */
    if (linelen >= MAXPATHLEN)
    {
      strcpy(line+100, "...");
      e_set(E_ERROR, "Path too long in the package database file %s. (%s)", name, line);
      goto err_1;
    }
    JSLI(ptr, p->paths, line);
    *ptr = type;
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
      gchar* sane_path = path_simplify(path);
      g_free(path);
      g_free(dir);
      g_free(link);
      g_free(target);
      /* check path size limit */
      if (strlen(sane_path) >= MAXPATHLEN)
      {
        strcpy(sane_path+100, "...");
        e_set(E_ERROR, "Path too long in the package database script %s. (%s)", name, sane_path);
        g_free(sane_path);
        goto err_1;
      }
      /* add */
      gint* ptr;
      JSLI(ptr, p->paths, sane_path);
      *ptr = DB_PATH_SYMLINK;
      g_free(sane_path);
    }
  }

  fseek(fs, 0, SEEK_END);
  guint script_size = ftell(fs);
  if (script_size > 512*1024)
  {
    e_set(E_ERROR, "Script file is too big %s. (%u kB)", p->name, script_size / 1024);
    goto err_1;
  }
  if (script_size)
  {
    p->doinst = g_malloc(script_size);
    fseek(fs, 0, SEEK_SET);
    if (fread(p->doinst, script_size, 1, fs) != 1)
    {
      e_set(E_ERROR, "Can't read script file %s. (%s)", p->name, strerror(errno));
      goto err_1;
    }
  }

 fini:
  if (line)
    free(line);
  if (fs)
    fclose(fs);
  fclose(fp);
 err_0:
  return p;
 err_1:
  db_free_pkg(p);
  p = NULL;
  goto fini;
}

static gchar* _get_date()
{
  static gchar buf[100];
  time_t t = time(NULL);
  struct tm* ts = localtime(&t);
  strftime(buf, sizeof(buf), "%F,%T", ts);
  return buf;
}

gint db_rem_pkg(gchar* name)
{
  _db_open_check(1)

  gint ret = 1;

  if (name == NULL)
  {
    e_set(E_BADARG, "Package name can't be NULL.");
    goto err_0;
  }
  if (parse_pkgname(name, 6) != (gchar*)-1)
  {
    e_set(E_ERROR, "Invalid package name. (%s)", name);
    goto err_0;
  }

  gchar* p = g_strdup_printf("%s/%s", _db.pkgdir, name);
  gchar* s = g_strdup_printf("%s/%s", _db.scrdir, name);
  gchar* rp = g_strdup_printf("%s/removed_packages/%s-removed-%s", _db.topdir, name, _get_date());
  gchar* rs = g_strdup_printf("%s/removed_scripts/%s-removed-%s", _db.topdir, name, _get_date());

  if (sys_file_type(p, 1) != SYS_REG)
  {
    e_set(E_ERROR|DB_NOTEX, "Package is not in the database. (%s)", name);
    goto err_1;
  }
  if (rename(p, rp) < 0)
  {
    e_set(E_ERROR, "Package file removal failed. (%s)", strerror(errno));
    goto err_1;
  }
  if (sys_file_type(s, 1) == SYS_REG)
  {
    if (rename(s, rs) < 0)
    {
      e_set(E_ERROR, "Script file removal failed. (%s)", strerror(errno));
      goto err_1;
    }
  }

  ret = 0;
 err_1:
  g_free(p);
  g_free(s);
  g_free(rp);
  g_free(rs);
 err_0:
  return ret;
}

/* public - generic database package queries
 ************************************************************************/

static gint _query_compare(gconstpointer a, gconstpointer b, gpointer data)
{
  db_query_type type = (db_query_type)data;
  if (type == DB_QUERY_PKGS_WITH_FILES || type == DB_QUERY_PKGS_WITHOUT_FILES)
  {
    const struct db_pkg* apkg = a;
    const struct db_pkg* bpkg = b;
    return strcmp(apkg->name, bpkg->name);
  }
  else if (type == DB_QUERY_NAMES)
  {
    return strcmp(a, b);
  }
  return 0;
}

GSList* db_query(db_selector cb, void* data, db_query_type type)
{
  GSList *pkgs = NULL;

  _db_open_check(NULL)

  if (type != DB_QUERY_PKGS_WITH_FILES &&
      type != DB_QUERY_PKGS_WITHOUT_FILES &&
      type != DB_QUERY_NAMES)
  {
    e_set(E_BADARG, "Invalid query type.");
    goto err_0;
  }

  DIR* d = opendir(_db.pkgdir);
  if (d == NULL)
  {
    e_set(E_FATAL, "Can't open package database directory. (%s)", strerror(errno));
    goto err_0;
  }
  
  struct dirent* de;
  while ((de = readdir(d)) != NULL)
  {
    if (de->d_name[0] == '.')
      continue;
    struct db_pkg* p;
    gchar* name = de->d_name;
    if (parse_pkgname(name, 6) == NULL)
      continue;
    /* if cb == NULL, then package matches */
    if (cb != NULL)
    {
      /* otherwise get package from database ask the selector if it 
         likes this package */
      p = db_get_pkg(name, DB_GET_WITHOUT_FILES);
      if (p == NULL)
      {
        e_set(E_ERROR, "Can't get package from the database.");
        goto err_1;
      }
      gint rs = cb(p, data);
      if (rs != 0 && rs != 1)
      {
        db_free_pkg(p);
        e_set(E_ERROR, "Package database query filter returned error.");
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
        if (p == NULL)
        {
          e_set(E_ERROR, "Can't get package from the database.");
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
  return g_slist_sort_with_data(pkgs, _query_compare, (gpointer)type);
 err_1:
  db_free_query(pkgs, type);
  closedir(d);
 err_0:
  return NULL;
}

void db_free_query(GSList* pkgs, db_query_type type)
{
  if (pkgs == NULL)
    return;

  void (*data_free_func)(void*) = g_free;
  if (type == DB_QUERY_PKGS_WITH_FILES ||
      type == DB_QUERY_PKGS_WITHOUT_FILES)
    data_free_func = (void (*)(void*))db_free_pkg;
  
  g_slist_foreach(pkgs, (GFunc)data_free_func, NULL);
  g_slist_free(pkgs);
}

gchar* db_get_package_name(const gchar* namespec)
{
  _db_open_check(NULL)

  if (namespec == NULL)
  {
    e_set(E_FATAL, "Namespec can't be NULL.");
    goto err_0;
  }

  gchar* pkgname = NULL;
  DIR* d = opendir(_db.pkgdir);
  struct dirent* de;
  if (d == NULL)
  {
    e_set(E_FATAL, "Can't open package database directory. (%s)", strerror(errno));
    goto err_0;
  }

  /* first search for exact match if namespec looks like full name */
  gchar* searched_name = parse_pkgname(namespec, 5);
  if (searched_name)
  {
    /* exact search */
    while ((de = readdir(d)) != NULL)
    {
      if (de->d_name[0] == '.')
        continue;
      gchar* cur_name = de->d_name;
      if (parse_pkgname(cur_name, 6) == NULL)
        continue;
      if (!strcmp(searched_name, cur_name))
      {
        pkgname = searched_name;
        goto done;
      }
    }
    g_free(searched_name);
  }

  /* try to use namespec as shortname */
  rewinddir(d);
  while ((de = readdir(d)) != NULL)
  {
    if (de->d_name[0] == '.')
      continue;
    gchar* cur_name = de->d_name;
    gchar* cur_shortname = parse_pkgname(cur_name, 1);
    if (cur_shortname == NULL)
      continue;
    if (!strcmp(namespec, cur_shortname))
    {
        pkgname = g_strdup(cur_name);
        g_free(cur_shortname);
        goto done;
      }
    g_free(cur_shortname);
  }

 done:
  closedir(d);
  return pkgname;
 err_0:
  return NULL;
}
