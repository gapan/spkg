/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "pkgname.h"

void test_parser(gchar* sn, gchar* sd)
{
  gchar *l, *s;
  gint rv = parse_slackdesc(sd, sn, &s, &l);
  printf("***** (reval = %d)\n", rv);
  printf(sd);
  printf("*****\n");
  printf(s);
  printf("*****\n");
  printf(l);
  printf("*****\n");
  g_free(l);
  g_free(s);
}

gchar* tests[][2] = {
 { "test",
"test: test (test slackdesc)\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
 },
 { "test2",
"test: test (test slackdesc)\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc.\n"
 },
 { "test",
"test: test (test slackdesc)\n"
"test: This is testing slackdesc.\n"
"test: This is testing slackdesc."
 },
 { "test",
"test: test (test slackdesc)\n"
"test: This is testing slackdesc.\n"
" test: This is testing slackdesc.\n"
"#bla\n"
"test: This is testing slackdesc.\n"
 },
 { "test",
"test: test (test slackdesc)"
 },
 { "test",
"test: test (test slackdesc)\n"
 },
};

int main(int ac, char* av[])
{
  gint i;
  for (i=0;i<sizeof(tests)/sizeof(tests[0]);i++)
    test_parser(tests[i][0], tests[i][1]);
  return 0;
}
