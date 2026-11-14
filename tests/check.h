/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/* Minimal assertion helpers shared by the test programs. A test prints
 * one line per failed check and returns check_result() from main(), so
 * the automake test driver sees a non-zero exit status on failure. */
#ifndef SPKG__TESTS_CHECK_H
#define SPKG__TESTS_CHECK_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

static gint check_failures = 0;

#define CHECK(cond) \
  do { \
    if (!(cond)) \
    { \
      printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      check_failures++; \
    } \
  } while (0)

#define CHECK_STR(got, want) \
  do { \
    const gchar* _g = (got); \
    const gchar* _w = (want); \
    if (_g == NULL || _w == NULL ? _g != _w : strcmp(_g, _w) != 0) \
    { \
      printf("FAILED %s:%d: %s: got \"%s\", want \"%s\"\n", __FILE__, \
        __LINE__, #got, _g ? _g : "(null)", _w ? _w : "(null)"); \
      check_failures++; \
    } \
  } while (0)

#define CHECK_INT(got, want) \
  do { \
    glong _g = (got); \
    glong _w = (want); \
    if (_g != _w) \
    { \
      printf("FAILED %s:%d: %s: got %ld, want %ld\n", __FILE__, __LINE__, \
        #got, _g, _w); \
      check_failures++; \
    } \
  } while (0)

/* Known bug: cond describes the correct behaviour and is expected to be
 * false for now. Once it holds (the bug got fixed), the test fails with
 * XPASS so the XFAIL can be turned into a CHECK. */
#define XFAIL(cond) \
  do { \
    if (cond) \
    { \
      printf("XPASS %s:%d: %s (known bug fixed? turn this into a CHECK)\n", \
        __FILE__, __LINE__, #cond); \
      check_failures++; \
    } \
    else \
      printf("XFAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
  } while (0)

static __inline__ gint check_result(void)
{
  if (check_failures)
    printf("%d check(s) failed\n", check_failures);
  return check_failures ? 1 : 0;
}

/* Create a fresh temporary directory and chdir into it. Returns its
 * absolute path (free with g_free), or NULL on failure. */
static __inline__ gchar* check_enter_tmpdir(void)
{
  gchar* dir = g_build_filename(g_get_tmp_dir(), "spkg-test-XXXXXX", NULL);
  if (g_mkdtemp(dir) == NULL || chdir(dir) != 0)
  {
    printf("FAILED: can't create temporary directory %s\n", dir);
    g_free(dir);
    return NULL;
  }
  return dir;
}

#endif
