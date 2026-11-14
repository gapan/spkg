/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "check.h"

/* parse_pkgname()
 ************************************************************************/

#define VALID ((const gchar*)-1)

struct pkgname_case {
  const gchar* path;
  /* expected element 0..5 (path, short, version, arch, build, full) */
  const gchar* want[6];
};

static const struct pkgname_case pkgnames[] = {
  { "foo-1.0-x86_64-1gv.txz",
    { ".", "foo", "1.0", "x86_64", "1gv", "foo-1.0-x86_64-1gv" } },
  /* dashes in the short name, absolute directory */
  { "/var/cache/foo-bar-baz-1.0-i486-2.tgz",
    { "/var/cache", "foo-bar-baz", "1.0", "i486", "2", "foo-bar-baz-1.0-i486-2" } },
  /* no extension (installed package names), relative directory */
  { "dir/sub/foo-1.0-noarch-1",
    { "dir/sub", "foo", "1.0", "noarch", "1", "foo-1.0-noarch-1" } },
  /* all supported extensions are stripped */
  { "foo-1.0-noarch-1.tgz", { ".", "foo", "1.0", "noarch", "1", "foo-1.0-noarch-1" } },
  { "foo-1.0-noarch-1.tlz", { ".", "foo", "1.0", "noarch", "1", "foo-1.0-noarch-1" } },
  { "foo-1.0-noarch-1.tbr", { ".", "foo", "1.0", "noarch", "1", "foo-1.0-noarch-1" } },
  { "foo-1.0-noarch-1.tbz", { ".", "foo", "1.0", "noarch", "1", "foo-1.0-noarch-1" } },
  { "foo-1.0-noarch-1.tar", { ".", "foo", "1.0", "noarch", "1", "foo-1.0-noarch-1" } },
  /* unknown extensions are part of the build (untgz rejects the file later) */
  { "foo-1.0-noarch-1.zip", { ".", "foo", "1.0", "noarch", "1.zip", "foo-1.0-noarch-1.zip" } },
  /* only one extension is stripped */
  { "foo-1.0-noarch-1.tgz.tgz", { ".", "foo", "1.0", "noarch", "1.tgz", "foo-1.0-noarch-1.tgz" } },
  /* minimal valid name */
  { "a-b-c-d", { ".", "a", "b", "c", "d", "a-b-c-d" } },
  /* invalid: too few parts or empty parts */
  { "foo-1.0-noarch.tgz",  { ".", NULL, NULL, NULL, NULL, NULL } },
  { "foo--noarch-1.tgz",   { ".", NULL, NULL, NULL, NULL, NULL } },
  { "-1.0-noarch-1.tgz",   { ".", NULL, NULL, NULL, NULL, NULL } },
  { "foo-1.0-noarch-.tgz", { ".", NULL, NULL, NULL, NULL, NULL } },
  { "foo-1.0--1.tgz",      { ".", NULL, NULL, NULL, NULL, NULL } },
  { "",                    { ".", NULL, NULL, NULL, NULL, NULL } },
};

static void test_pkgname(void)
{
  guint i, e;
  for (i = 0; i < G_N_ELEMENTS(pkgnames); i++)
  {
    const struct pkgname_case* c = &pkgnames[i];
    for (e = 0; e < 6; e++)
    {
      gchar* got = parse_pkgname(c->path, e);
      if ((got == NULL) != (c->want[e] == NULL) ||
          (got && strcmp(got, c->want[e])))
      {
        printf("FAILED: parse_pkgname(\"%s\", %u) = \"%s\", want \"%s\"\n",
          c->path, e, got ? got : "(null)", c->want[e] ? c->want[e] : "(null)");
        check_failures++;
      }
      g_free(got);
    }
    /* element 6 is the validity check */
    gchar* valid = parse_pkgname(c->path, 6);
    CHECK((valid == VALID) == (c->want[1] != NULL));
  }

  CHECK(parse_pkgname(NULL, 1) == NULL);
  CHECK(parse_pkgname("foo-1.0-noarch-1.tgz", 7) == NULL);
}

/* parse_createlink() / parse_cleanuplink()
 ************************************************************************/

static void createlink_ok(const gchar* line, const gchar* dir,
                          const gchar* link, const gchar* target)
{
  gchar *d = NULL, *l = NULL, *t = NULL;
  gchar* buf = g_strdup(line);
  if (!parse_createlink(buf, &d, &l, &t))
  {
    printf("FAILED: parse_createlink(\"%s\") rejected a valid line\n", line);
    check_failures++;
  }
  else
  {
    CHECK_STR(d, dir);
    CHECK_STR(l, link);
    CHECK_STR(t, target);
  }
  CHECK(!parse_cleanuplink(buf));
  g_free(d);
  g_free(l);
  g_free(t);
  g_free(buf);
}

static void createlink_bad(const gchar* line)
{
  gchar *d = NULL, *l = NULL, *t = NULL;
  gchar* buf = g_strdup(line);
  if (parse_createlink(buf, &d, &l, &t))
  {
    printf("FAILED: parse_createlink(\"%s\") accepted an invalid line\n", line);
    check_failures++;
  }
  /* outputs are left alone on failure */
  CHECK(d == NULL && l == NULL && t == NULL);
  g_free(buf);
}

static void test_links(void)
{
  /* the exact form makepkg writes */
  createlink_ok("( cd usr/lib ; ln -sf libfoo.so.1 libfoo.so )",
                "usr/lib", "libfoo.so", "libfoo.so.1");
  createlink_ok("( cd . ; ln -sf ../a b )", ".", "b", "../a");
  /* trailing text after " )" is ignored (and dropped from doinst.sh) */
  createlink_ok("( cd usr/lib ; ln -sf a b ) # comment", "usr/lib", "b", "a");

  /* anything else stays in doinst.sh and is run by the shell */
  createlink_bad("( cd usr/lib ; ln -sf libfoo.so.1 libfoo.so)");
  createlink_bad("(cd usr/lib ; ln -sf a b )");
  createlink_bad("( cd usr/lib ; ln -s a b )");
  createlink_bad("( cd usr/lib ;  ln -sf a b )");
  createlink_bad("( cd usr/lib ; ln -sf a )");
  createlink_bad("( cd usr/lib ; ln -sf a b c )");
  createlink_bad("( cd  ; ln -sf a b )");
  createlink_bad("( cd usr/lib ; rm -rf libfoo.so )");
  createlink_bad("ln -sf a b");
  createlink_bad("");

  CHECK(parse_cleanuplink("( cd usr/lib ; rm -rf libfoo.so )"));
  CHECK(parse_cleanuplink("( cd usr/lib ; rm -rf libfoo.so ) # comment"));
  CHECK(!parse_cleanuplink("( cd usr/lib ; rm -rf libfoo.so)"));
  CHECK(!parse_cleanuplink("( cd usr/lib ; rm -rf a b )"));
  CHECK(!parse_cleanuplink("( cd usr/lib ; rm -f libfoo.so )"));
  CHECK(!parse_cleanuplink("( cd usr/lib ; ln -sf a b )"));
  CHECK(!parse_cleanuplink("rm -rf /"));
  CHECK(!parse_cleanuplink(""));
}

/* iter_str_lines() / iter_buf_lines()
 ************************************************************************/

/* join all lines returned by iter_str_lines() with '|' */
static gchar* str_lines(const gchar* text)
{
  GString* out = g_string_new("");
  gchar *b, *e, *ln, *n = (gchar*)text;
  gint count = 0;
  while (iter_str_lines(&b, &e, &n, &ln))
  {
    CHECK_INT(e - b, strlen(ln));
    g_string_append_printf(out, "%s%s", count++ ? "|" : "", ln);
    g_free(ln);
  }
  return g_string_free(out, FALSE);
}

/* same for iter_buf_lines() on the first len bytes of text */
static gchar* buf_lines(const gchar* text, gsize len)
{
  GString* out = g_string_new("");
  gchar* buf = g_malloc(len); /* no terminating zero */
  memcpy(buf, text, len);
  gchar *b, *e, *ln, *n = buf;
  gint count = 0;
  while (iter_buf_lines(&b, &e, &n, buf + len, &ln))
  {
    CHECK_INT(e - b, strlen(ln));
    g_string_append_printf(out, "%s%s", count++ ? "|" : "", ln);
    g_free(ln);
  }
  g_free(buf);
  return g_string_free(out, FALSE);
}

#define CHECK_LINES(fn, want) \
  do { gchar* _l = fn; CHECK_STR(_l, want); g_free(_l); } while (0)

static void test_iter_lines(void)
{
  CHECK_LINES(str_lines("a\nb\n"), "a|b");
  CHECK_LINES(str_lines("a\nb"), "a|b");
  CHECK_LINES(str_lines("a\n\nb\n"), "a||b");
  CHECK_LINES(str_lines("\n"), "");
  CHECK_LINES(str_lines(""), "");
  CHECK_LINES(str_lines("single"), "single");

  CHECK_LINES(buf_lines("a\nb\n", 4), "a|b");
  CHECK_LINES(buf_lines("a\nb", 3), "a|b");
  CHECK_LINES(buf_lines("a\n\nb\n", 5), "a||b");
  CHECK_LINES(buf_lines("a\n\n", 3), "a|");
  CHECK_LINES(buf_lines("abc\ndef", 5), "abc|d"); /* stops at eof */

  /* without ln, only the pointers are set */
  gchar *b, *e, *n = "first\nsecond";
  CHECK(iter_str_lines(&b, &e, &n, NULL));
  CHECK_INT(e - b, 5);
  CHECK(iter_str_lines(&b, &e, &n, NULL));
  CHECK_INT(e - b, 6);
  CHECK(n == NULL);
  CHECK(!iter_str_lines(&b, &e, &n, NULL));
}

/* g_strv_length_compat()
 ************************************************************************/

static void test_strv_length(void)
{
  gchar* none[] = { NULL };
  gchar* three[] = { "a", "", "c", NULL };
  CHECK_INT(g_strv_length_compat(none), 0);
  CHECK_INT(g_strv_length_compat(three), 3);
}

int main(int ac, char* av[])
{
  test_pkgname();
  test_links();
  test_iter_lines();
  test_strv_length();
  return check_result();
}
