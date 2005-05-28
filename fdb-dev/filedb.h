/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup pkgdb_api */
/*! @{ */

#ifndef __FILEDB_H
#define __FILEDB_H

#include <glib.h>

/*! @if false */
#define FILEDB_DIR "var/log/fastpkg/filedb"
/*! @endif */

/** File information structure. */
struct db_file {
  gchar* path; /**< full path to the file */
  gchar* link; /**< path to the link target (if file is symlink or link) */
  guint  dup:1; /**< true if file already exists in the archive */
  guint  padding:31; /**< just a padding to a dword */
};

/** Open file database.
 *
 * @param root root directory
 * @return 0 on success, 1 on error
 */
extern gint fdb_open(const gchar* root);

/** Close file database. 
 */
extern gint fdb_close();

/** Returns description of the error if one occured in the last filedb
 * library call.
 *
 * @return Error string on error, 0 otherwise
 */
extern gchar* fdb_error();

#endif

/*! @} */
