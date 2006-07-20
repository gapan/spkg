/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef SPKG__PATH_H
#define SPKG__PATH_H

#include <glib.h>

G_BEGIN_DECLS

/** Remove trailing slashes and squeeze any consecutive slashes into one.
 *
 * For examle "//path/to///" -> "/path/to"
 * @param path path to be sanitizd.
 * @return sanitized path (always)
 */
extern gchar* path_sanitize_slashes(const gchar* path) G_GNUC_MALLOC;

/** Powerful path simplification function.
 *
 * Will do this: "/./path/../../to//./" -> "/to", or "././path/../../to//./" -> "../to"
 * @param path path to be simlified.
 * @return simplified path (always)
 */
extern gchar* path_simplify(const gchar* path) G_GNUC_MALLOC;

/** Get path elements.
 *
 * @param path path to be splitted
 * @return string vector of path elements, must be freed using g_strfreev (always)
 */
extern gchar** path_get_elements(const gchar* path) G_GNUC_MALLOC;

/** Sanitize root path.
 *
 * / -> '/'
 * ./ -> ''
 * ./bla -> bla/
 * /bla/bla -> /bla/bla/
 * 
 * @param root Root path.
 * @return sanitized root path, should be freed by the caller
 */
extern gchar* sanitize_root_path(const gchar* root) G_GNUC_MALLOC;

G_END_DECLS

#endif

/*! @} */
