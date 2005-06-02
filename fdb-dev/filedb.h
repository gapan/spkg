/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup filedb_api */
/*! @{ */

#ifndef __FILEDB_H
#define __FILEDB_H

#include <glib.h>

/*! @if false */
#define FILEDB_DIR "var/log/fastpkg/filedb"
/*! @endif */

struct file_pld {
  guint16 rc;
//  guint16 mode;
  guint16 plen;
  guint16 llen;
  gchar data[];
  /* data[0] = 'path' + '\0' */
  /* data[plen+1] = 'link' + '\0' */
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

/** Get file from database.
 *
 * @param id File id.
 * @return 0 on error, file_pld pointer on success
 */
extern struct file_pld* fdb_get_file(guint32 id);

/** Get file id from database.
 *
 * @param path File path.
 * @return 0 on error, fileid on success
 */
extern guint32 fdb_get_file_id(gchar* path);

/** Add file to database.
 *
 * @param path File path.
 * @param link Link target if file is symlink.
 * @return 0 on error, fileid on success
 */
extern guint32 fdb_add_file(gchar* path, gchar* link);

#endif

/*! @} */
