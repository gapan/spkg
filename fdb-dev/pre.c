#include "filedb.h"

int main()
{
  gint fd,j,id;
  
  if (fdb_open("."))
  {
    printf("%s\n", fdb_error());
    exit(1);
  }

  fdb_close();

  return 0;
}
