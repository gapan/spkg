/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "misc.h"

void test_parser(gchar* sn, gchar* sd)
{
  gchar* dsc[MAX_SLACKDESC_LINES];
  gint i;
  gint rv = parse_slackdesc(sd, sn, dsc);

  gchar* d = gen_slackdesc(sn, dsc);

  printf("***** (reval = %d)\n", rv);
  for (i=0;dsc[i]!=0 && i<MAX_SLACKDESC_LINES; i++)
    printf("%s\n", dsc[i]);
  printf("*****\n");
  printf("%s", d);
  printf("*****\n");
  for (i=0;i<MAX_SLACKDESC_LINES;i++)
    g_free(dsc[i]);
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
  guint i;
  for (i=0; i<sizeof(tests)/sizeof(tests[0]); i++)
    test_parser(tests[i][0], tests[i][1]);
  return 0;
}
