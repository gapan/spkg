/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "path.h"
#include "bench.h"

static gint retval = 0;

void simplify_test(gchar* in, gchar* ok)
{
  start_timer(0);
  gchar* out = path_simplify(in);
  stop_timer(0);
  print_timer(0, "time");
  if (strcmp(out, ok))
  {
    printf("FAILED: \"%s\" -> \"%s\" (should be \"%s\")\n", in, out, ok);
    retval = 1;
  }
  else
    printf("OK: \"%s\" -> \"%s\"\n", in, out);
  g_free(out);
}

int main(int ac, char* av[])
{
  simplify_test("//a/b/.//c/.", "/a/b/c");

  start_timer(0);
  path_sanitize_slashes("//a/b/.//c/.");
  stop_timer(0);
  print_timer(0, "sl");

  simplify_test("/a/b/../c", "/a/c");
  simplify_test("/a/b/../../../c", "/c");
  simplify_test("/a/b/../../c/../../d", "/d");

  simplify_test("./a/b/../c", "a/c");
  simplify_test("a/b/../../../c", "../c");
  simplify_test("a/b/../../c/../d", "d");
  simplify_test("a/b/../../c/../../d", "../d");
  simplify_test("a/../b/../../c/../../d", "../../d");
  simplify_test("a/b/.././c/../../d/../../../", "../..");
  simplify_test("a/b/.././c/../../d/../../../e/f/.././g", "../../e/g");
  simplify_test("../a/b/.././c/../../d/../../../e/f/../../.././g", "../../../../g");

  return retval;
}
