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

#define PKG_OK    0 /**< no error */
#define PKG_EXIST 1 /**< file exist */
#define PKG_NOTEX 2 /**< file not exist */
#define PKG_OTHER 3 /**< other error */

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
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
