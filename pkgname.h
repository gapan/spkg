/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __PKGNAME_H
#define __PKGNAME_H

#include <glib.h>

/** Parse package name into parts.
 *
 * @param path Package path.
 * @param elem Package name element.
 *  @li 0 path
 *  @li 1 shortname
 *  @li 2 version
 *  @li 3 arch
 *  @li 4 build
 *  @li 5 fullname
 *  @li 6 check (returns -1 if ok)
 * @return Requested package name element (g_malloced), 0 on error.
 */
extern gchar* parse_pkgname(const gchar* path, guint elem);

#endif

/*! @} */
