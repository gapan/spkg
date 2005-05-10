/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ond�ej (megi) Jirman, 2005 *|
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
  db_open(root);

  if (pkg_install("fastpkg-0.9.0-i486-1.tgz", root, 1, 1))
    printf("%s\n", pkg_error());
  
  if (pkg_install("kdebase-3.3.2-i486-1.tgz", root, 1, 1))
    printf("%s\n", pkg_error());

  db_close();
  return 0;
}


