/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#ifndef SPKG__CMD_PRIVATE_H
#define SPKG__CMD_PRIVATE_H

#include "pkgdb.h"
#include "sigtrap.h"
#include "message.h"
#include "commands.h"

#define e_set(n, fmt, args...) e_add(e, "command", __func__, n, fmt, ##args)

#define _safe_breaking_point(label) \
  do { \
    if (sig_break) \
    { \
      e_set(E_BREAK, "terminated by signal"); \
      goto label; \
    } \
  } while(0)

#endif
