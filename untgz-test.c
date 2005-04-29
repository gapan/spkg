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
#include "untgz.h"

int main(int ac, char* av[])
{
  if (ac == 2)
  {
    struct untgz_state* tgz;
    tgz = untgz_open(av[1]);
    if (tgz == 0)
      return 1;
    while (untgz_get_header(tgz) == 0)
      untgz_write_file(tgz,0);
    untgz_close(tgz);
  }
  return 0;
}
