/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
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

/** Parse slack-desc file from buffer into parts.
 *
 * @param slackdesc Buffer filled with contents of the raw slack-desc file.
 * @param sname Short name of the package.
 * @param desc Array of prased slack-desc lines.
 * @return 0 on success, 1 on error (i.e. not a single slack-desc line)
 */
extern gint parse_slackdesc(const gchar* slackdesc, const gchar* sname, gchar* desc[11]);

#endif

/*! @} */
