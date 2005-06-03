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
  for (j=0; j<files_cnt; j++)
  {
    id = fdb_get_file_id(files[j]);

    if (id == 0)
    {
      printf("error\n");
      break;
    }
  }
  stop_timer(0);
  print_timer(0, "get");

  fdb_close();

  return 0;
}
