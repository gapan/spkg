// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "sigtrap.h"

/* private 
 ************************************************************************/

static void _signal_handler(int sig)
{
  if (sig == SIGINT || sig == SIGQUIT || sig == SIGTERM || sig == SIGHUP)
    sig_break = 1;
}

#define e_set(n, fmt, args...) e_add(e, "sigtrap", __func__, n, fmt, ##args)

/* public
 ************************************************************************/

gboolean sig_break = 0;

gint sig_trap(struct error* e)
{
  g_assert(e != 0);
  struct sigaction act;
  act.sa_handler = _signal_handler;
  act.sa_flags = SA_RESTART;
  if ( sigemptyset(&act.sa_mask) ||
       sigaction(SIGINT, &act, NULL) ||
       sigaction(SIGQUIT, &act, NULL) ||
       sigaction(SIGHUP, &act, NULL) ||
       sigaction(SIGTERM, &act, NULL) ||
       sigaction(SIGUSR1, &act, NULL) ||
       sigaction(SIGUSR2, &act, NULL) ||
       sigaction(SIGPIPE, &act, NULL) )
  {
    e_set(E_FATAL, "signal trapping setup failed");
    return 1;
  }
  return 0;
}
