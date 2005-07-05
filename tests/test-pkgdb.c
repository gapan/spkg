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

#define TEST_SYNC_TO_DB 1
#define TEST_SYNC_FROM_DB 0

int main(int ac, char* av[])
{
  struct error* err = e_new();

  nice(-10);

  db_open(0,err);
  if (!e_ok(err))
    goto err_0;

  atexit(db_close);

#if TEST_SYNC_TO_DB == 1
  start_timer(0);
  db_sync_from_legacydb();
  if (!e_ok(err))
    goto err_0;
  stop_timer(0);
  print_timer(0, "db_sync_from_legacydb");
#endif

#if TEST_SYNC_FROM_DB == 1
  start_timer(0);
  db_sync_to_legacydb();
  if (!e_ok(err))
    goto err_0;
  stop_timer(0);
  print_timer(0, "db_sync_to_legacydb");
#endif

 err_0:
  e_print(err);
  e_free(err);
  return 1;
}
