/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "untgz.h"

int main(int ac, char* av[])
{
  // Check if we have something on the command line.
  if (ac > 1)
    return 1;
  
  gint i;
  // For each file do:
  for (i=1;i<ac;i++)
  {
    // Open tgz file.
    struct untgz_state* tgz = untgz_open(av[i]);
    if (tgz == 0)
    {
      fprintf(stderr, "error: can't open tgz file\n");
      continue;
    }
    // While we can successfully get next file's header from the archive...
    while (untgz_get_header(tgz) == 0)
    {
      // ...we will be extracting that file to a disk using its original name...
      if (untgz_write_file(tgz, 0))
      {
        // ...until something goes wrong.
        break;
      }
    }
    // And if something went wrong...
    if (tgz->errstr)
    {
      // ...we will alert user.
      fprintf(stderr, "error: %s\n", tgz->errstr);
    }
    
    // Close file.
    untgz_close(tgz);
  }
  return 0;
}
