/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error.h"

/* private 
 ************************************************************************/

struct error {
  gchar* string;
  gint number;
  gint bad;
};

/* public
 ************************************************************************/

gchar* e_string(struct error* e)
{
  return e->string;
}

gint e_errno(struct error* e)
{
  return e->number;
}

struct error* e_new()
{
  return g_new0(struct error, 1);
}

void e_free(struct error* e)
{
  e_clean(e);
  g_free(e);
}

void e_add(
  struct error* e,
  const char* context,
  const char* function,
  gint errnum,
  gchar* errfmt,
  ...
)
{
  va_list ap;
  va_start(ap, errfmt);
  gchar* msg = g_strdup_vprintf(errfmt, ap);
  va_end(ap);
  gchar* tmp = g_strdup_printf("error[%s:%s](%d:%d): %s\n", context, function, errnum&0xff, (errnum>>8)&0xff, msg);
  g_free(msg);
  msg = tmp;
  if (e->string)
  {
    msg = g_strdup_printf("%s%s", e->string, tmp);
    g_free(e->string);
    g_free(tmp);
  }
  e->string = msg;
  if (errnum != E_PASS)
    e->number = errnum;
  e->bad = 1;
}

void e_clean(struct error* e)
{
  if (G_UNLIKELY(e->string != 0))
  {
    g_free(e->string);
    e->string = 0;
  }
  e->number = E_OK;
  e->bad = 0;
}

void e_print(struct error* e)
{
  if (e->bad && e->string)
    fputs(e->string, stderr);
}

gint e_ok(struct error* e)
{
  return !e->bad;
}
