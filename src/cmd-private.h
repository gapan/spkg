// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
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
#include "untgz.h"

#define _safe_breaking_point(label) \
  do { \
    if (sig_break) \
    { \
      e_set(E_BREAK, "terminated by signal"); \
      goto label; \
    } \
  } while(0)

static __inline__ gint _mode_differ(struct untgz_state* u, struct stat* st)
{
  return ((u->f_mode ^ st->st_mode) & 07777);
}

static __inline__ gint _gid_or_uid_differ(struct untgz_state* u, struct stat* st)
{
  return (u->f_uid != st->st_uid || u->f_gid != st->st_gid);
}

static __inline__ gint _size_differ(struct untgz_state* u, struct stat* st)
{
  return (u->f_size != st->st_size);
}

static __inline__ gint _rdev_differ(struct untgz_state* u, struct stat* st)
{
  if (S_ISCHR(st->st_mode) && u->f_type == UNTGZ_CHR && 
     (dev_t)(((u->f_devmaj << 8) & 0xFF00) | (u->f_devmin & 0xFF)) == st->st_rdev)
    return 0;
  if (S_ISBLK(st->st_mode) && u->f_type == UNTGZ_BLK &&
     (dev_t)(((u->f_devmaj << 8) & 0xFF00) | (u->f_devmin & 0xFF)) == st->st_rdev)
    return 0;
  return 1;
}

#endif // SPKG__CMD_PRIVATE_H
