/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pkgdb.h"
#include "bench.h"

int main(int ac, char* av[])
{
  if (db_open(0))
  {
    fprintf(stderr, "%s\n", db_error());
    return 1;
  }
  atexit(db_close);
  
  struct db_pkg* pkg = db_legacy_get_pkg("aspell-0.60-i486-2");
  
  printf(
    "PACKAGE NAME:              %s\n"
    "COMPRESSED PACKAGE SIZE:   %d K\n"
    "UNCOMPRESSED PACKAGE SIZE: %d K\n"
    "PACKAGE LOCATION:          %s\n"
    "PACKAGE DESCRIPTION:\n"
    "%s",
    pkg->name, pkg->csize, pkg->usize, pkg->location, pkg->desc
  );
  
  if (pkg->files)
  {
    GSList* l;
    printf("FILE LIST:\n");
    for (l=pkg->files; l!=0; l=l->next)
    {
      struct db_file* f = l->data;
      if (f->link)
        printf("%s -> %s\n", f->path, f->link);
      else
        printf("%s\n", f->path);
    }
  }
  
  return 0;
}
