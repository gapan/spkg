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

void _sig_handler(int sig)
{
  switch (sig)
  {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
    case SIGHUP:
      sig_break = 1;
    break;
    case SIGUSR1:
    case SIGUSR2:
    case SIGPIPE:
    default:
    break;
  }
}

#define e_set(n, fmt, args...) e_add(e, "sigtrap", __func__, n, fmt, ##args)

/* public
 ************************************************************************/

gboolean sig_break = 0;

gint sig_trap(struct error* e)
{
  struct sigaction act;
  act.sa_handler = _sig_handler;
  act.sa_flags = 0;
  if ( sigemptyset(&act.sa_mask) ||
       sigaction(SIGINT, &act, 0) ||
       sigaction(SIGQUIT, &act, 0) ||
       sigaction(SIGHUP, &act, 0) ||
       sigaction(SIGTERM, &act, 0) ||
       sigaction(SIGUSR1, &act, 0) ||
       sigaction(SIGUSR2, &act, 0) ||
       sigaction(SIGPIPE, &act, 0) )
  {
    e_set(E_FATAL, "signal trapping setup failed");
    return 1;
  }
  return 0;
}
