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
  db_open(root);

  for (i=1;i<ac;i++)
  {
    printf("installing: %s\n", av[i]);
    if (pkg_install(av[i], root, 1, 1))
    {
      printf("%s\n", pkg_error());
      break;
    }
  }

  db_close();
  return 0;
}
