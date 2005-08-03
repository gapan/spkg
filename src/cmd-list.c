/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>

#include "cmd-private.h"

/* public 
 ************************************************************************/

gint cmd_list(
  const gchar* regexp,
  cmd_list_mode mode,
  gboolean legacydb,
  const struct cmd_options* opts,
  struct error* e
)
{
  g_assert(opts != 0);
  g_assert(e != 0);

  GSList* list;
  GSList* i;

  switch (mode)
  {
    case CMD_MODE_GLOB:
      list = db_query_glob(legacydb?DB_SOURCE_LEGACY:DB_SOURCE_SPKG, regexp, DB_QUERY_NAMES);
      for (i=list; i!=0; i=i->next)
        printf("%s\n", (gchar*)i->data);
      db_free_query(list, DB_QUERY_NAMES);
    break;
    case CMD_MODE_ALL:
    break;
    default:
      e_set(E_FATAL, "invalid mode");
  }
  
  return 1;
}
