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
#define USED __attribute__((used))

#include "bench/tsc.h"

static USED void add_pkg(gchar* name)
{
  struct db_pkg* pkg;
  printf("loading legacy pkg %s...\n", name);
  fflush(stdout);
  pkg = db_legacy_get_pkg(name);
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

static USED void get_pkg(gchar* name)
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

static USED void del_pkg(gchar* name)
{
  printf("removing pkg %s...\n", name);
  fflush(stdout);
  if (db_rem_pkg(name))
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
}

#define TEST_SYNC_TO_DB 1
#define TEST_SYNC_FROM_DB 1

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
  
  nice(-10);

#if TEST_SYNC_TO_DB == 1
  start_timer(0);
  if (db_sync_legacydb_to_fastpkgdb())
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
  stop_timer(0);
  print_timer(0, "db_sync_legacydb_to_fastpkgdb()");
#endif

#if TEST_SYNC_FROM_DB == 1
  start_timer(0);
  if (db_sync_fastpkgdb_to_legacydb())
  {
    fprintf(stderr, "%s\n", db_error());
    exit(1);
  }
  stop_timer(0);
  print_timer(0, "db_sync_fastpkgdb_to_legacydb()");
#endif

  return 0;
}
