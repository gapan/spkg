/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ond�ej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup pt_api */
/*! @{ */

#ifndef __PKGTOOLS_H
#define __PKGTOOLS_H

#include <glib.h>

/** Returns description of the error if one occured in the last pkg
 * library call.
 *
 * @return Error string on error, 0 otherwise
 */
extern gchar* pkg_error();

/** @brief Install package.
 * 
 * @param  pkgfile Package file.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint pkg_install(gchar* pkgfile, gboolean dryrun, gboolean verbose);

/** @brief Upgrade package.
 * 
 * @param  pkgfile Package file.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint pkg_upgrade(gchar* pkgfile, gboolean dryrun, gboolean verbose);

/** @brief Remove package.
 * 
 * @param  name Package name.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint pkg_remove(gchar* name, gboolean dryrun, gboolean verbose);

#endif

/*! @} */

