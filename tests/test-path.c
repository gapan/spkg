/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondřej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "path.h"
#include "misc.h"
#include "bench.h"
#include "check.h"

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

void slashes_test(gchar* in, gchar* ok)
{
  gchar* out = path_sanitize_slashes(in);
  CHECK_STR(out, ok);
  g_free(out);
}

/* elements of a path joined by '|' */
void elements_test(gchar* in, gchar* ok)
{
  gchar** v = path_get_elements(in);
  gchar* joined = g_strjoinv("|", v);
  CHECK_STR(joined, ok);
  /* an empty path has no elements; otherwise one more than separators */
  guint want = 0;
  if (*ok)
  {
    gchar* c;
    for (want = 1, c = ok; *c; c++)
      want += *c == '|';
  }
  CHECK_INT(g_strv_length_compat(v), want);
  g_free(joined);
  g_strfreev(v);
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
  sanitize_root_test("", "/");  /* ROOT= means ROOT=/ (19c6014) */
  sanitize_root_test("./", "");
  sanitize_root_test("./bob", "bob/");
  sanitize_root_test("bob", "bob/");

  printf("sanitize slashes tests\n");

  slashes_test("//a//b//", "/a/b");
  slashes_test("/a/b", "/a/b");
  slashes_test("a///b", "a/b");
  slashes_test("a/", "a");
  slashes_test("/", "/");
  slashes_test("//", "/");
  slashes_test("", "");
  slashes_test("./a/../b/", "./a/../b"); /* only slashes are touched */

  printf("path elements tests\n");

  elements_test("a/b/c", "a|b|c");
  elements_test("/a/b", "|a|b");    /* absolute: empty first element */
  elements_test("//a//b//", "|a|b");
  elements_test("a", "a");
  elements_test("/", "|");
  elements_test("", "");

  /* path_simplify() keeps what it can't resolve and never fails */
  simplify_test("", "");
  simplify_test(".", "");
  simplify_test("..", "..");
  simplify_test("/a/../../b", "/b");
  simplify_test("a/./b/./", "a/b");
  simplify_test("usr/lib/../../../../etc/passwd", "../../etc/passwd");

  /* climbing to or above / gives / (not "", the current directory) */
  simplify_test("/..", "/");
  simplify_test("/../..", "/");
  simplify_test("/a/../..", "/");
  simplify_test("/a/..", "/");
  simplify_test("/./..", "/");
  sanitize_root_test("/..", "/");
  sanitize_root_test("/a/../..", "/");

  return retval | check_result();
}
