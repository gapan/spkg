/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "pkgtools.h"
#include "pkgdb.h"

const gchar* root = "root";

int main(int ac, char* av[])
{
  int i;
  struct error* err = e_new();

  db_open(root, err);
  if (!e_ok(err))
    goto err_0;

  atexit(db_close);

  for (i=1;i<ac;i++)
  {
    printf("installing: %s\n", av[i]);
    pkg_install(av[i], root, 0, 0, err);
    if (!e_ok(err))
      goto err_0;
  }

  db_close();

 err_0:
  e_print(err);
  e_free(err);
  return 0;
}
