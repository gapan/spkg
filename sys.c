/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "sys.h"

sys_ftype sys_file_type(gchar* path)
{
  struct stat s;
  if (lstat(path, &s) == 0)
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

/* implement them in C */
gint sys_rm_rf(gchar* p)
{
  gint rval;
  gchar* s = g_strdup_printf("/bin/rm -rf %s", p);
  rval = system(s);
  g_free(s);
  if (rval == 0)
    return 0;
  return 1;
}

gint sys_mkdir_p(gchar* p)
{
  gint rval;
  gchar* s = g_strdup_printf("/bin/mkdir -p %s", p);
  rval = system(s);
  g_free(s);
  if (rval == 0)
    return 0;
  return 1;
}



gint verbose = 2;

void notice(const gchar* f,...)
{
  va_list ap;
  if (verbose < 2)
    return;
  printf("notice: ");
  va_start(ap, f);
  vprintf(f, ap);
  va_end(ap);
  fflush(stdout);
}

void err(gint e, const gchar* f,...)
{
  va_list ap;
  printf("error: ");
  va_start(ap, f);
  vprintf(f, ap);
  va_end(ap);
  fflush(stdout);
  if (e)
    exit(e);
}

void warn(const gchar* f,...)
{
  va_list ap;
  if (verbose < 1)
    return;
  printf("warning: ");
  va_start(ap, f);
  vprintf(f, ap);
  va_end(ap);
  fflush(stdout);
  exit(1);
}
