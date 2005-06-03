#include <stdio.h>
#include "filedb.h"
#include "tsc.h"

extern unsigned int files_cnt;
extern char* files[];

int main()
{
  gint fd,j,id;
  struct fdb_file f;
  
  start_timer(0);
  if (fdb_open("."))
  {
    printf("%s\n", fdb_error());
    exit(1);
  }
  stop_timer(0);
  print_timer(0, "open");

  start_timer(0);
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
  stop_timer(0);
  print_timer(0, "add");

  fdb_close();

  return 0;
}
