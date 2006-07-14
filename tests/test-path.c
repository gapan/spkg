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
  gchar* out = path_simplify(in);
  if (strcmp(out, ok))
  {
    printf("FAILED: \"%s\" -> \"%s\" (should be \"%s\")\n", in, out, ok);
    retval = 1;
  }
  else
    printf("OK: \"%s\" -> \"%s\"\n", in, out);
  g_free(out);
}

void sanitize_root_test(gchar* in, gchar* ok)
{
  gchar* out = sanitize_root_path(in);
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
  printf("simplify path tests\n");

  simplify_test("//a/b/.//c/.", "/a/b/c");
  simplify_test("./", "");
  simplify_test("/", "/");

  simplify_test("/a/b/../c", "/a/c");
  simplify_test("/a/b/../../../c", "/c");
  simplify_test("/a/b/../../c/../../d", "/d");

  simplify_test("./a/b/../c", "a/c");
  simplify_test("a/b/../../../c", "../c");
  simplify_test("a/b/../../c/../d", "d");
  simplify_test("a/b/../../c/../../d", "../d");
  simplify_test("a/../b/../..//c/../../d", "../../d");
  simplify_test("a/b/.././c/../../d/../../../", "../..");
  simplify_test("a/b/.././c/../../d/..//../../e//f//.././g", "../../e/g");
  simplify_test("../a/b/.././c/../../d/../../../e/f/../../.././g", "../../../../g");

  printf("sanitize root path tests\n");

  sanitize_root_test(NULL, "/");
  sanitize_root_test("/", "/");
  sanitize_root_test("//", "/");
  sanitize_root_test("", "");
  sanitize_root_test("./", "");
  sanitize_root_test("./bob", "bob/");
  sanitize_root_test("bob", "bob/");

  return retval;
}
