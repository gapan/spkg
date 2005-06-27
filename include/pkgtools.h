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

#define PKG_EXIST E(0) /**< package exist */
#define PKG_NOTEX E(1) /**< package not exist */
#define PKG_BADNAME E(2) /**< package has invalid name */
#define PKG_CORRUPT E(3) /**< package is corrupted */
#define PKG_DB E(4) /**< package database error */

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_install(
  const gchar* pkgfile,
  const gchar* root,
  gboolean dryrun,
  gboolean verbose,
  struct error* e
);

/** Upgrade package <b>[not implemented]</b>.
 * 
 * @param pkgfile Package file.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_upgrade(
  const gchar* pkgfile,
  const gchar* root,
  gboolean dryrun,
  gboolean verbose,
  struct error* e
);

/** Remove package <b>[not implemented]</b>.
 * 
 * @param name Package name.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint pkg_remove(
  const gchar* name,
  const gchar* root,
  gboolean dryrun,
  gboolean verbose,
  struct error* e
);

#endif

/*! @} */
