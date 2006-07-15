/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/

#include "cmd-private.h"

/* private
 ************************************************************************/

#define e_set(n, fmt, args...) e_add(e, "upgrade", __func__, n, fmt, ##args)

/* public 
 ************************************************************************/

gint cmd_upgrade(const gchar* pkgfile, const struct cmd_options* opts, struct error* e)
{
  g_assert(pkgfile != 0);
  g_assert(opts != 0);
  g_assert(e != 0);
  e_set(E_FATAL, "Upgrade command is not yet implemented.");
  return 1;
}
