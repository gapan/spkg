/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>

#include "pkgdb.h"

static void add_pkg(gchar* name)
{
  struct db_pkg* pkg;
  printf("loading legacy pkg %s...\n", name);
  fflush(stdout);
  pkg = db_get_legacy_pkg(name);
  if (pkg == 0)
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
  printf("adding pkg %s...\n", name);
  fflush(stdout);
  if (db_add_pkg(pkg))
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
}

static void get_pkg(gchar* name)
{
  struct db_pkg* pkg;
  printf("getting pkg %s...\n", name);
  fflush(stdout);
  pkg = db_get_pkg(name,1);
  if (pkg == 0)
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }

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
}

static void del_pkg(gchar* name)
{
  printf("removing pkg %s...\n", name);
  fflush(stdout);
  if (db_rem_pkg(name))
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
}

int main(int ac, char* av[])
{
  printf("opening db...\n");
  fflush(stdout);
  if (db_open(0))
  {
    fprintf(stderr, "%s\n", db_error());
    return 1;
  }
  atexit(db_close);
  
#if 0
  if (db_sync_legacydb_to_fastpkgdb())
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
#else
//  get_pkg("byacc-1.9-i386-1");
  get_pkg("tetex-2.0.2-i386-1");
//  add_pkg("byacc-1.9-i386-1");
//  add_pkg("tetex-2.0.2-i386-1");
//  del_pkg("tetex-2.0.2-i386-1");
#endif

  return 0;
}
