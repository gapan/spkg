/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup pt_api */
/*! @{ */

#ifndef __PKGTOOLS_H
#define __PKGTOOLS_H

#include <glib.h>

/** @brief Install package.
 * 
 * @param  pkgfile Package file.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint installpkg(gchar* pkgfile, gboolean dryrun, gboolean verbose);

/** @brief Upgrade package.
 * 
 * @param  pkgfile Package file.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint upgradepkg(gchar* pkgfile, gboolean dryrun, gboolean verbose);

/** @brief Remove package.
 * 
 * @param  name Package name.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint removepkg(gchar* name, gboolean dryrun, gboolean verbose);

#endif

/*! @} */

