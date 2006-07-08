/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup pt_api Package Tools API


*//*--------------------------------------------------------------------*/
/** @addtogroup pt_api */
/*! @{ */

#ifndef SPKG__COMMANDS_H
#define SPKG__COMMANDS_H

#include <glib.h>
#include "error.h"

#define CMD_EXIST   E(0) /**< package exist */
#define CMD_NOTEX   E(1) /**< package not exist */
#define CMD_BADNAME E(2) /**< package has invalid name */
#define CMD_CORRUPT E(3) /**< package is corrupted */
#define CMD_BADIO   E(4) /**< failed filesystem operation */
#define CMD_DB      E(5) /**< package database error */

/** Mode of operation of the install command. */
typedef enum {
  CMD_MODE_PARANOID=1, /**< paranoid mode */
  CMD_MODE_NORMAL,     /**< normal mode */
  CMD_MODE_BRUTAL,     /**< brutal mode */
} cmd_install_mode;

/** Mode of operation of the list command. */
typedef enum {
  CMD_MODE_ALL,        /**<  */
  CMD_MODE_GLOB,       /**<  */
} cmd_list_mode;

/** Common package command options structure. */
struct cmd_options {
  gchar* root;        /**< Root directory. */
  gboolean dryrun;    /**< Don't touch filesystem. */
  gint verbosity;     /**< Verbosity level. */
  gboolean noscripts; /**< Turn off scripts (doinst.sh) execution. */
};

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param mode Installation mode.
 * @param optsyms Optimize symlinks.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_install(
  const gchar* pkgfile,
  cmd_install_mode mode,
  gboolean optsyms,
  const struct cmd_options* opts,
  struct error* e
);

/** Upgrade package <b>[not implemented]</b>.
 * 
 * @param pkgfile Package file.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_upgrade(const gchar* pkgfile, const struct cmd_options* opts, struct error* e);

/** Remove package <b>[not implemented]</b>.
 * 
 * @param pkgname Package name.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_remove(const gchar* pkgname, const struct cmd_options* opts, struct error* e);

/** List packages.
 * 
 * @param regexp Regular expression.
 * @param mode List mode.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_list(
  const gchar* regexp,
  cmd_list_mode mode,
  const struct cmd_options* opts,
  struct error* e
);

#endif

/*! @} */
