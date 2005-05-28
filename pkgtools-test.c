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

#include "pkgtools.h"
#include "pkgdb.h"

const gchar* root = "./,,root";

int main(int ac, char* av[])
{
  int i;
  db_open(root);

  for (i=1;i<ac;i++)
  {
    printf("installing: %s\n", av[i]);
    if (pkg_install(av[i], root, 0, 0))
      printf("%s\n", pkg_error());
  }

  db_close();
  return 0;
}


