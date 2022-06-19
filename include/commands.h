/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup pt_api Package Tools API

Each of the folowing functions implement particular commad.

*/
/** @addtogroup pt_api */
/*! @{ */

#ifndef SPKG__COMMANDS_H
#define SPKG__COMMANDS_H

#include <glib.h>
#include "error.h"

G_BEGIN_DECLS

#define CMD_EXIST   E(0) /**< package already exists */
#define CMD_NOTEX   E(1) /**< package does not exist */

/** Common package command options structure. */
struct cmd_options {
  gchar* root;         /**< Root directory. */
  gboolean safe;       /**< Play it safe (i.e. don't replace existing files). */
  gboolean force;      /**< Play it hard (force install,...). */
  gboolean purge;      /**< Purge remnants of .new configuration files */
  gboolean dryrun;     /**< Don't touch filesystem. */
  gint verbosity;      /**< Verbosity level. */
  gboolean no_scripts; /**< Turn off scripts (doinst.sh) execution. */
  gboolean no_optsyms; /**< Turn off symlink optimizations. */
  gboolean no_ldconfig; /**< Turn off ldconfig execution. */
  gboolean no_gtk_update_icon_cache; /**< Turn off gtk-update-icon-cache execution. */
  gboolean reinstall;  /**< Reinstall package when it is already installed (for upgrade). */
  gchar** bl_symopts;  /**< Blacklist for symlink optimizations. */
};

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_install(const gchar* pkgfile, const struct cmd_options* opts, struct error* e);

/** Upgrade package.
 * 
 * @param pkgfile Package file.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_upgrade(const gchar* pkgfile, const struct cmd_options* opts, struct error* e);

/** Remove package.
 * 
 * @param pkgname Package name.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_remove(const gchar* pkgname, const struct cmd_options* opts, struct error* e);

/** List packages.
 * 
 * @param arglist List of glob expressions. If NULL, all packages will be shown.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_list(GSList* arglist, const struct cmd_options* opts, struct error* e);

G_END_DECLS

#endif

/*! @} */
