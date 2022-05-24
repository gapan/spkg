// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#ifndef SPKG__CMD_COMMON_H
#define SPKG__CMD_COMMON_H

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "misc.h"
#include "path.h"
#include "sys.h"
#include "taction.h"

#include "misc.h"
#include "cmd-private.h"

extern gboolean _check_libc_libs(const gchar* path);

extern void _run_ldconfig(const gchar* root, const struct cmd_options* opts);

extern void _gtk_update_icon_cache(const gchar* root, const struct cmd_options* opts);

extern gboolean _unsafe_path(const gchar* path);

extern void _read_slackdesc(struct untgz_state* tgz, struct db_pkg* pkg);

extern gint _read_doinst_sh(struct untgz_state* tgz, struct db_pkg* pkg,
                   const gchar* root, const gchar* sane_path,
                   const struct cmd_options* opts, struct error* e,
                   const gboolean do_upgrade, struct db_pkg* ipkg);

#endif // SPKG__CMD_COMMON_H
