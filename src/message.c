// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "message.h"

static gint _msg_verbosity = 1;

void msg_setup(gint verbosity)
{
  _msg_verbosity = verbosity;
}

void msg(const gint type, const gchar* fmt, ...)
{
  g_assert(fmt != 0);
  if (_msg_verbosity < type)
    return;
  if (type == 2)
    printf("WARNING: ");
  if (type == 3)
    printf(" --> ");
  if (type == 4)
    printf(" --> ");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}
