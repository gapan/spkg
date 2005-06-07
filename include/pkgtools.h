/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/**********************************************************************/
/** @defgroup pt_api Package Tools API


***********************************************************************/
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

/** Install package.
 * 
 * @param pkgfile Package file.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint pkg_install(const gchar* pkgfile, const gchar* root, gboolean dryrun, gboolean verbose);

/** Upgrade package <b>[not implemented]</b>.
 * 
 * @param pkgfile Package file.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint pkg_upgrade(const gchar* pkgfile, const gchar* root, gboolean dryrun, gboolean verbose);

/** Remove package <b>[not implemented]</b>.
 * 
 * @param name Package name.
 * @param root Root directory.
 * @param dryrun Don't touch filesystem or database.
 * @param verbose Be verbose.
 * @return 0 on success, 1 on error
 */
extern gint pkg_remove(const gchar* name, const gchar* root, gboolean dryrun, gboolean verbose);

#endif

/*! @} */

