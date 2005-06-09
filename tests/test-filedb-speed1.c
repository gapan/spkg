/*----------------------------------------------------------------------*\
|* spkg - Slackware Linux Fast Package Management Tools                 *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include "filedb.h"
#include "bench.h"

extern unsigned int files_cnt;
extern char* files[];

int main()
{
  gint j,id,i;
  struct fdb_file f;
  
  nice(-10);
  unlink("filedb/idx");
  unlink("filedb/pld");
  
  if (fdb_open("."))
  {
    printf("%s\n", fdb_error());
    return 1;
  }

  reset_timer(0);
  i=0;
  f.link = 0;
  f.mode = 0644;
  for (j=0; j<files_cnt; j++)
  {
    f.path = files[j];
    continue_timer(0);
    id = fdb_add_file(&f);
    stop_timer(0);
    if (++i == 5000)
    {
      i=0;
      printf("%u %lg\n", j+1, get_timer(0));
      reset_timer(0);
    }

    if (id == 0)
    {
      printf("error\n");
      break;
    }
  }

  fdb_close();

  return 0;
}
