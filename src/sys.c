/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include "sys.h"
#include "path.h"

#define e_set(e, n, fmt, args...) e_add(e, "sys", __func__, n, fmt, ##args)

sys_ftype sys_file_type_stat(const gchar* path, gboolean deref, struct stat* s)
{
  g_assert(path != 0);
  struct stat st;
  gint rs;
  if (s == NULL)
    s = &st;
  if (deref)
    rs = stat(path, s);
  else
    rs = lstat(path, s);
  if (rs == 0)
  {
    if (S_ISREG(s->st_mode)) return SYS_REG;
    if (S_ISDIR(s->st_mode)) return SYS_DIR;
    if (S_ISLNK(s->st_mode)) return SYS_SYM;
    if (S_ISBLK(s->st_mode)) return SYS_BLK;
    if (S_ISCHR(s->st_mode)) return SYS_CHR;
    if (S_ISFIFO(s->st_mode)) return SYS_FIFO;
    if (S_ISSOCK(s->st_mode)) return SYS_SOCK;
  }
  if (errno == ENOENT || errno == ENOTDIR)
    return SYS_NONE;
  return SYS_ERR;
}

sys_ftype sys_file_type(const gchar* path, gboolean deref)
{
  return sys_file_type_stat(path, deref, 0);
}

time_t sys_file_mtime(const gchar* path, gboolean deref)
{
  g_assert(path != 0);
  struct stat s;
  gint rs;
  if (deref)
    rs = stat(path, &s);
  else
    rs = lstat(path, &s);
  if (rs == 0)
    return s.st_mtime;
  return (time_t)-1;
}

/* ripped off busybox and modified */
gint sys_rm_rf(const gchar* path)
{
  struct stat path_stat;

  /* if dir not exist return with error */
  if (lstat(path, &path_stat) < 0)
    return 1;

  /* dir */
  if (S_ISDIR(path_stat.st_mode))
  {
    DIR *dp;
    struct dirent *d;
    int status = 0;

    if ((dp = opendir(path)) == NULL)
      return 1;
    while ((d = readdir(dp)) != NULL)
    {
      if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
        continue;
      gchar *new_path = g_strdup_printf("%s/%s", path, d->d_name);
      if (sys_rm_rf(new_path))
        status = 1;
      g_free(new_path);
    }
    if (closedir(dp) < 0)
      return 1;
    if (rmdir(path) < 0)
      return 1;
    return status;
  }
  else /* nondir */
  {
    if (unlink(path) < 0)
      return 1;
    return 0;
  }
}

gint sys_mkdir_p(const gchar* path)
{
  gchar* simple_path;
  gchar** pathv;
  gint i, j, retval = 1, pathv_len;
  gchar* tmp, *tmp_end;

  g_assert(path != 0);
  
  simple_path = path_simplify(path);
  pathv = path_get_elements(simple_path);
  pathv_len = g_strv_length(pathv);
  tmp = tmp_end = g_malloc0(strlen(simple_path)+10);
  g_free(simple_path);

  for (i=0; i<pathv_len; i++)
  {
    /* build path */
    if (i > 0) /* absolute path or not first element */
      *(tmp_end++) = '/';
    if (i == 0 && **pathv == 0)
      continue;
    for (j=0; j<strlen(pathv[i]); j++)
      *(tmp_end++) = pathv[i][j];
    
    /* skip backreferences */
    if (!strcmp(pathv[i], ".."))
      continue;

    /* check dir */
    sys_ftype type = sys_file_type(tmp,0);
    if (type == SYS_DIR)
      continue;
    if (type == SYS_NONE)
    {
      if (mkdir(tmp, 0755) == -1)
        goto out;
    }
    else
      goto out;
  }

  retval = 0;
out:
  g_free(tmp);
  g_strfreev(pathv);
  return retval;
}

void sys_sigblock(sigset_t* sigs)
{
  g_assert(sigs != 0);
  gint rs;
  sigset_t s;
  sigfillset(&s);
  rs = sigprocmask(SIG_SETMASK, &s, sigs);
  if (rs == -1)
  {
    printf("panic: can't block signals\n");
    exit(1);
  }
}

void sys_sigunblock(sigset_t* sigs)
{
  g_assert(sigs != 0);
  gint rs;
  rs = sigprocmask(SIG_SETMASK, sigs, 0);
  if (rs == -1)
  {
    printf("panic: can't unblock signals\n");
    exit(1);
  }
}

gint sys_lock_new(const gchar* path, struct error* e)
{
  g_assert(path != 0);
  g_assert(e != 0);
  gint fd = open(path, O_CREAT, 0644);
  if (fd < 0)
  {
    e_set(e, E_FATAL, "can't open lock file: %s", strerror(errno));
    return -1;
  }
  return fd;
}

gint sys_lock_trywait(gint fd, gint timeout, struct error* e)
{
  g_assert(e != 0);
  gint c=0;
  while (1)
  {
    gint s = flock(fd, LOCK_NB|LOCK_EX);
    if (s == -1) /* we did not get lock */
    {
      if (errno != EWOULDBLOCK) /* and will not get it */
      {
        e_set(e, E_FATAL, "flock failed: %s", strerror(errno));
        return 1;
      }
      /* ok maybe we could get it a bit later */
      if (++c > timeout)
      {
        e_set(e, E_ERROR, "timed out waiting for a lock");
        return 1;
      } 
      usleep(100000); /* 100ms */
    }
    /* got it! */
    return 0;
  }
}

gint sys_lock_put(gint fd, struct error* e)
{
  g_assert(e != 0);
  gint s = flock(fd, LOCK_NB|LOCK_UN);
  if (s == -1) /* error */
  {
    e_set(e, E_FATAL, "flock failed: %s", strerror(errno));
    return 1;
  }
  return 0;
}

void sys_lock_del(gint fd)
{
  close(fd);
}

gint sys_write_buffer_to_file(const gchar* file, const gchar* buf, gsize len, struct error* e)
{
  g_assert(file != 0);
  g_assert(buf != 0);
  g_assert(e != 0);

  FILE* f = fopen(file, "w");
  if (f == NULL)
  {
    e_set(e, E_FATAL, "can't open file for writing: %s", strerror(errno));
    return 1;
  }
  if (len == 0)
    len = strlen(buf);
  if (1 != fwrite(buf, len, 1, f))
  {
    e_set(e, E_FATAL, "can't write data to a file: %s", file);
    fclose(f);
    return 1;
  }
  fclose(f);
  return 0;
}
