/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __PATH_H
#define __PATH_H

#include <glib.h>

/** Remove trailing slashes and squeeze any consecutive slashes into one.
 *
 * For examle "//path/to///" -> "/path/to"
 * @param path path to be sanitizd.
 * @return sanitized path (always)
 */
extern gchar* path_sanitize_slashes(const gchar* path);

/** Powerful path simplification function.
 *
 * Will do this: "/./path/../../to//./" -> "/to", or "././path/../../to//./" -> "../to"
 * @param path path to be simlified.
 * @return simplified path (always)
 */
extern gchar* path_simplify(const gchar* path);

/** Get path elements.
 *
 * @param path path to be splitted
 * @return string vector of path elements, must be freed using g_strfreev (always)
 */
extern gchar** path_get_elements(const gchar* path);

#endif

/*! @} */
