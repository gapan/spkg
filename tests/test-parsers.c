/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondřej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "misc.h"
#include "check.h"

#define LINE "This is testing slackdesc."
#define HEAD "test (test slackdesc)"

struct slackdesc_case {
  const gchar* sname;     /* package short name passed to the parser */
  const gchar* input;     /* raw slack-desc contents */
  gint rv;                /* expected parse_slackdesc() return value */
  const gchar* want[MAX_SLACKDESC_LINES + 1]; /* expected lines, NULL-ended */
};

static const struct slackdesc_case cases[] = {
  /* more than MAX_SLACKDESC_LINES matching lines: extra ones are dropped */
  { "test",
    "test: " HEAD "\n"
    "test: " LINE "\n" "test: " LINE "\n" "test: " LINE "\n"
    "test: " LINE "\n" "test: " LINE "\n" "test: " LINE "\n"
    "test: " LINE "\n" "test: " LINE "\n" "test: " LINE "\n"
    "test: " LINE "\n" "test: " LINE "\n" "test: " LINE "\n"
    "test: " LINE "\n" "test: " LINE "\n",
    0,
    { HEAD, LINE, LINE, LINE, LINE, LINE, LINE, LINE, LINE, LINE, LINE, LINE,
      LINE, NULL } },
  /* no line matches the short name */
  { "test2",
    "test: " HEAD "\n"
    "test: " LINE "\n",
    1, { NULL } },
  /* last line without a trailing newline */
  { "test",
    "test: " HEAD "\n"
    "test: " LINE "\n"
    "test: " LINE,
    0, { HEAD, LINE, LINE, NULL } },
  /* indented lines and comments are ignored */
  { "test",
    "test: " HEAD "\n"
    "test: " LINE "\n"
    " test: " LINE "\n"
    "#bla\n"
    "test: " LINE "\n",
    0, { HEAD, LINE, LINE, NULL } },
  /* a single line, with and without newline */
  { "test", "test: " HEAD, 0, { HEAD, NULL } },
  { "test", "test: " HEAD "\n", 0, { HEAD, NULL } },
  /* a longer name sharing the prefix is not a match */
  { "test",
    "testing: other package\n"
    "test: " HEAD "\n",
    0, { HEAD, NULL } },
  /* a shorter name sharing the prefix is not a match either */
  { "tes", "test: " HEAD "\n", 1, { NULL } },
  /* only one space after the colon is stripped; empty lines are kept */
  { "test",
    "test:no space\n"
    "test:  two spaces\n"
    "test:\n",
    0, { "no space", " two spaces", "", NULL } },
  /* empty input */
  { "test", "", 1, { NULL } },
};

static void check_case(guint n, const struct slackdesc_case* c)
{
  gchar* dsc[MAX_SLACKDESC_LINES];
  gint i;

  printf("slackdesc case %u\n", n);
  CHECK_INT(parse_slackdesc(c->input, c->sname, dsc), c->rv);
  for (i = 0; i < MAX_SLACKDESC_LINES; i++)
  {
    CHECK_STR(dsc[i], c->want[i]);
    if (c->want[i] == NULL)
      break;
  }

  /* gen_slackdesc() must produce text that parses back to the same lines */
  gchar* gen = gen_slackdesc(c->sname, dsc);
  if (c->want[0] == NULL)
    CHECK(gen == NULL);
  else
  {
    gchar* again[MAX_SLACKDESC_LINES];
    CHECK(gen != NULL);
    if (gen != NULL)
    {
      CHECK_INT(parse_slackdesc(gen, c->sname, again), 0);
      for (i = 0; i < MAX_SLACKDESC_LINES; i++)
      {
        CHECK_STR(again[i], dsc[i]);
        g_free(again[i]);
      }
    }
  }
  g_free(gen);

  for (i = 0; i < MAX_SLACKDESC_LINES; i++)
    g_free(dsc[i]);
}

int main(int ac, char* av[])
{
  guint i;
  for (i = 0; i < G_N_ELEMENTS(cases); i++)
    check_case(i, &cases[i]);

  /* exact output format of gen_slackdesc() */
  gchar* dsc[MAX_SLACKDESC_LINES] = { "first", "second", NULL };
  gchar* gen = gen_slackdesc("pkg", dsc);
  CHECK_STR(gen, "pkg: first\npkg: second\n");
  g_free(gen);

  /* empty lines have no trailing space, like in slack-desc */
  gchar* dsc_empty[MAX_SLACKDESC_LINES] = { "first", "", "third", NULL };
  gen = gen_slackdesc("pkg", dsc_empty);
  CHECK_STR(gen, "pkg: first\npkg:\npkg: third\n");
  g_free(gen);

  return check_result();
}
