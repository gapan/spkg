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
 * @return 
 */
extern gint installpkg(gchar* pkgfile);

/** @brief Upgrade package.
 * 
 * @param  pkgfile Package file.
 * @return 
 */
extern gint upgradepkg(gchar* pkgfile);

/** @brief Remove package.
 * 
 * @param  name Package name.
 * @return 
 */
extern gint removepkg(gchar* name);

#endif

/*! @} */

