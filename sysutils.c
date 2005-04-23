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
#include <unistd.h>
#include "sysutils.h"

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

/* retval:
 *  0 = can't be determined
 *  1 = regular file
 *  2 = directory
 *  3 = symlink
 *  4 = block device
 *  5 = character device
 *  6 = fifo
 *  7 = socket
 */
 
gint file_type(gchar* path)
{
  struct stat s;
  if (stat(path, &s) == 0)
  {
    if (S_ISREG(s.st_mode)) return FT_REG;
    if (S_ISDIR(s.st_mode)) return FT_DIR;
    if (S_ISLNK(s.st_mode)) return FT_LNK;
    if (S_ISBLK(s.st_mode)) return FT_BLK;
    if (S_ISCHR(s.st_mode)) return FT_CHR;
    if (S_ISFIFO(s.st_mode)) return FT_FIFO;
    if (S_ISSOCK(s.st_mode)) return FT_SOCK;
  }
  return FT_NONE;
}
