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

#ifndef __PKGTOOLS_H
#define __PKGTOOLS_H

#include <glib.h>
#include "error.h"

#define PKG_EXIST   E(0) /**< package exist */
#define PKG_NOTEX   E(1) /**< package not exist */
#define PKG_BADNAME E(2) /**< package has invalid name */
#define PKG_CORRUPT E(3) /**< package is corrupted */
#define PKG_BADIO   E(4) /**< failed filesystem operation */
#define PKG_DB      E(5) /**< package database error */

typedef enum {
  PKG_MODE_PARANOID=1, /**<  */
  PKG_MODE_CAUTIOUS,   /**<  */
  PKG_MODE_NORMAL,     /**<  */
  PKG_MODE_BRUTAL,     /**<  */
  PKG_MODE_GLOB,     /**<  */
  PKG_MODE_ALL,     /**<  */
  PKG_MODE_FROMLEGACY,     /**<  */
  PKG_MODE_TOLEGACY,     /**<  */
} pkg_mode;

/** Common package command options structure. */
struct pkg_options {
  gchar* root;       /**< Root directory. */
  gboolean dryrun;   /**< Don't touch filesystem or database. */
  gint verbosity;    /**< Verbosity level 0=errors only, 1=warnings only, 2=all messages. */
  gboolean noptsym;  /**< Turn off symlinks optimization. */
  gboolean nodoinst; /**< Turn off doinst.sh execution. */
  pkg_mode mode;     /**< Mode of operation of command. */
};

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_install(const gchar* pkgfile, const struct pkg_options* opts, struct error* e);

/** Upgrade package <b>[not implemented]</b>.
 * 
 * @param pkgfile Package file.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_upgrade(const gchar* pkgfile, const struct pkg_options* opts, struct error* e);

/** Remove package <b>[not implemented]</b>.
 * 
 * @param pkgname Package name.
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_remove(const gchar* pkgname, const struct pkg_options* opts, struct error* e);

/** Synchronize package databases.
 * 
 * @param opts Options.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_sync(const struct pkg_options* opts, struct error* e);

#endif

/*! @} */
