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

  if (db_open(root))
  {
    fprintf(stderr, "%s\n", db_error());
    return 1;
  }

  for (i=1;i<ac;i++)
  {
    printf("installing: %s\n", av[i]);
    if (pkg_install(av[i], root, 0, 0, err))
    {
      printf("%s\n", e_string(err));
      e_free(err);
      break;
    }
  }

  db_close();
  return 0;
}
