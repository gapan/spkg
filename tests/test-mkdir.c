/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sys.h"

int main(int ac, char* av[])
{
  sys_mkdir_p("../.././a/b/c/d/e");
  sys_mkdir_p("./a/b/c/d/e");
  sys_mkdir_p("/a/b/c/d/e");
  return 0;
}
