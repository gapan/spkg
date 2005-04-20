#include <stdio.h>
#include "untgz.h"
#include "pkgtools.h"

int main(int ac, char* av[])
{
  if (ac != 2)
    return 1;
  return installpkg(0, av[1]);
#if 0
  untgz_state_t* uts;
  package_t* pkg;
  
  pkg = parse_pkgname(av[1]);
  if (!pkg)
    return 1;
  
  printf("processing package %s\n", pkg->shortname);
  
  uts = untgz_open(av[1]);
  if (uts == 0)
  {
    printf("error opening file\n");
    return 1;
  }
  while (untgz_get_next_head(uts) == 0)
  {
    printf("f: %-30s %6d %-10s %-10s\n", uts->f_name, uts->f_size, uts->f_uname, uts->f_gname);
  }  
  printf("s: %8d %8d\n", uts->csize, uts->usize);
  untgz_close(uts);
#endif
  return 0;
}
