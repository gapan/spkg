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
static const gchar* _msg_prefix = "";

void msg_setup(const gchar* prefix, gint verbosity)
{
  g_assert(prefix != 0);
  _msg_verbosity = verbosity;
  _msg_prefix = prefix;
}

void msg(const gint type, const gchar* fmt, ...)
{
  g_assert(fmt != 0);
  if (_msg_verbosity < type)
    return;
  printf("%s: ", _msg_prefix);
  switch (type)
  {
    case 1: printf("WARN: "); break;
    case 2: printf("INFO: "); break;
    case 3: printf("NOTICE: "); break;
    case 4: printf("DEBUG: "); break;
  }
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}
