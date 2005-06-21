/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include "filedb.h"

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

  i=0;
  f.link = 0;
  for (j=0; j<files_cnt; j++)
  {
    f.path = files[j];
    id = fdb_add_file(&f);
    if (id == 0)
    {
      printf("error\n");
      break;
    }
  }

  fdb_close();

  return 0;
}
