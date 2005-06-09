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
  gint j,id;
  struct fdb_file f;
  
  nice(-10);
  unlink("filedb/idx");
  unlink("filedb/pld");
  
  start_timer(0);
  if (fdb_open("."))
  {
    printf("%s\n", fdb_error());
    return 1;
  }
  stop_timer(0);
//  print_timer(0, "fdb_open");

  start_timer(1);
  f.link = 0;
  f.mode = 0644;
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
  stop_timer(1);
//  print_timer(1, "fdb_add_file");

  start_timer(2);
  for (j=0; j<files_cnt; j++)
  {
    id = fdb_get_file_id(files[j]);

    if (id == 0)
    {
      printf("error\n");
      break;
    }
  }
  stop_timer(2);
//  print_timer(2, "fdb_get_file_id");

  start_timer(3);
  fdb_close();
  stop_timer(3);
//  print_timer(3, "fdb_close");

  printf("%lg %lg %lg %lg\n", get_timer(0), get_timer(1), get_timer(2), get_timer(3));

  return 0;
}
