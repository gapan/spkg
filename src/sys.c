/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "sys.h"

#define e_set(e, n, fmt, args...) e_add(e, "filedb", __func__, n, fmt, ##args)

sys_ftype sys_file_type(const gchar* path, gboolean deref)
{
  struct stat s;
  gint rs;
  if (deref)
    rs = stat(path, &s);
  else
    rs = lstat(path, &s);
  if (rs == 0)
  {
    if (S_ISREG(s.st_mode)) return SYS_REG;
    if (S_ISDIR(s.st_mode)) return SYS_DIR;
    if (S_ISLNK(s.st_mode)) return SYS_SYM;
    if (S_ISBLK(s.st_mode)) return SYS_BLK;
    if (S_ISCHR(s.st_mode)) return SYS_CHR;
    if (S_ISFIFO(s.st_mode)) return SYS_FIFO;
    if (S_ISSOCK(s.st_mode)) return SYS_SOCK;
  }
  if (errno == ENOENT || errno == ENOTDIR)
    return SYS_NONE;
  return SYS_ERR;
}

time_t sys_file_mtime(const gchar* path, gboolean deref)
{
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

/*XXX: implement this in C */
gint sys_rm_rf(const gchar* path)
{
  gint rval;
  gchar* s = g_strdup_printf("/bin/rm -rf %s", path);
  rval = system(s);
  g_free(s);
  if (rval == 0)
    return 0;
  return 1;
}

/*XXX: implement this in C */
gint sys_mkdir_p(const gchar* path)
{
  gint rval;
  gchar* s = g_strdup_printf("/bin/mkdir -p %s", path);
  rval = system(s);
  g_free(s);
  if (rval == 0)
    return 0;
  return 1;
}

gchar* sys_setcwd(const gchar* path)
{
  gchar pwd[2048];
  
  if (getcwd(pwd, 2048) == 0)
    return 0;
    
  if (path)
  {
    if (sys_file_type(path, 1) != SYS_DIR)
      return 0;
    if (chdir(path) == -1)
      return 0;
  }
  return g_strdup(pwd);
}

void sys_sigblock(sigset_t* sigs)
{
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
