/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondřej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "check.h"

int main(int ac, char* av[])
{
  /* the blacklist fixture lives in the source dir (differs in VPATH builds) */
  const gchar* srcdir = getenv("srcdir");
  gchar* path = g_build_filename(srcdir ? srcdir : ".", "blacklist", NULL);

  gchar** bl = load_blacklist(path);
  CHECK(bl != NULL);
  if (bl == NULL)
  {
    printf("can't load %s\n", path);
    g_free(path);
    return check_result();
  }
  g_free(path);

  /* empty and commented lines are dropped, the rest is whitespace-stripped
   * (the fixture has "bla" twice: once plain, once indented) */
  CHECK_INT(g_strv_length(bl), 6);

  /* entries match exactly after stripping surrounding whitespace */
  CHECK(is_blacklisted("bla", bl));
  CHECK(is_blacklisted("ble", bl));
  CHECK(is_blacklisted("boo", bl));
  CHECK(is_blacklisted("asdfasdf", bl));
  CHECK(is_blacklisted("asdf asd", bl)); /* inner whitespace is kept */

  /* no stripped whitespace, partial matches or comment contents */
  CHECK(!is_blacklisted("  bla", bl));
  CHECK(!is_blacklisted("ble  ", bl));
  CHECK(!is_blacklisted("bl", bl));
  CHECK(!is_blacklisted("blah", bl));
  CHECK(!is_blacklisted("asdf", bl));
  CHECK(!is_blacklisted("", bl));
  CHECK(!is_blacklisted("# sdfasdf", bl));
  CHECK(!is_blacklisted("sdfasdf", bl));
  CHECK(!is_blacklisted("# sdfgsdfg", bl));

  /* NULL arguments are handled */
  CHECK(!is_blacklisted(NULL, bl));
  CHECK(!is_blacklisted("bla", NULL));
  g_strfreev(bl);

  /* missing file */
  CHECK(load_blacklist("/nonexistent/spkg/blacklist") == NULL);

  return check_result();
}
