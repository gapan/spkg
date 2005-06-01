#include "filedb.h"
#include "files.c"

int main()
{
  gint fd,j,id;
  
  if (fdb_open("."))
  {
    printf("%s\n", fdb_error());
    exit(1);
  }

  for (j=0; j<sizeof(files)/sizeof(files[0]); j++)
    id = _get_node(files[j]);

  fdb_close();

  return 0;
}
