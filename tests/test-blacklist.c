/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"

int main(int ac, char* av[])
{
  gchar** bl = load_blacklist("blacklist");
  g_assert(bl != NULL);
  gint i;
  for (i=0; i<g_strv_length(bl); i++)
    printf("%d:%s:\n", i, bl[i]);
  g_strfreev(bl);
  return 0;
}
