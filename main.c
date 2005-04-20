#include <stdio.h>
#include "untgz.h"
#include "pkgtools.h"

int main(int ac, char* av[])
{
  if (ac != 2)
    return 1;
  return installpkg(0, av[1]);
}
