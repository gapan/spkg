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
  struct error* err = e_new();

  db_open(0, err);
  if (!e_ok(err))
    goto err_0;

  atexit(db_close);
  
  start_timer(0);
  struct db_pkg* pkg = db_legacy_get_pkg("tetex-2.0.2-i386-1",1);
  if (!e_ok(err))
    goto err_0;
  stop_timer(0);

  start_timer(1);
  pkg = db_legacy_get_pkg("ncurses-5.4-i486-2",1);
  if (!e_ok(err))
    goto err_0;
  stop_timer(1);
  
  print_timer(0, "P");
  print_timer(1, "S");

 err_0:
  e_print(err);
  e_free(err);
  return 1;
}
