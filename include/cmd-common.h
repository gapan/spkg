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

/* packages that can't be optimized, until they are fixed */

/* Check for libc symlinks to libraries that are critical to running sh. If
 * these symlinks are removed, doinst fails to run, because sh (bash) fails to
 * run. Since these are recreated with ldconfig when the glibc-solibs package
 * is installed, it's same to leave them where they are
 */
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
