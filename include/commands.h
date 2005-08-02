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

/** mode of operation of command */
typedef enum {
  CMD_MODE_PARANOID=1, /**<  */
  CMD_MODE_NORMAL,     /**<  */
  CMD_MODE_BRUTAL,     /**<  */
  CMD_MODE_GLOB,       /**<  */
  CMD_MODE_ALL,        /**<  */
  CMD_MODE_FROMLEGACY, /**<  */
  CMD_MODE_TOLEGACY,   /**<  */
} cmd_mode;

/** Common package command options structure. */
struct cmd_options {
  gchar* root;       /**< Root directory. */
  gboolean dryrun;   /**< Don't touch filesystem or database. */
  gint verbosity;    /**< Verbosity level 0=errors only, 1=warnings only, 2=all messages. */
  gboolean noptsym;  /**< Turn off symlinks optimization. */
  gboolean nodoinst; /**< Turn off doinst.sh execution. */
  cmd_mode mode;     /**< Mode of operation of command. */
};

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_install(const gchar* pkgfile, const struct cmd_options* opts, struct error* e);

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

/** Synchronize package databases.
 * 
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_sync(const struct cmd_options* opts, struct error* e);

/** List packages <b>[not implemented]</b>.
 * 
 * @param regexp Regular expression.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint cmd_list(const gchar* regexp, const struct cmd_options* opts, struct error* e);

#endif

/*! @} */
