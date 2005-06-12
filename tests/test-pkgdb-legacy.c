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
  
  start_timer(0);
  struct db_pkg* pkg = db_legacy_get_pkg("tetex-2.0.2-i386-1");
  stop_timer(0);

  start_timer(1);
  pkg = db_legacy_get_pkg("ncurses-5.4-i486-2");
  stop_timer(1);
  
  print_timer(0, "P");
  print_timer(1, "S");
  return 0;
  
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
