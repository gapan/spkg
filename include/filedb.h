/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup filedb_api File Database API

Filedb is extremely fast file database for package manager. It can handle
from 300,000 to 1,000,000 file requests per second on P4 1.5GHz. (add, get,
rem)

Basic operations on filedb are:

@li Add file
@li Get file
@li Rem file

@section add Add file

It is necessary to go through database and find file
with same path

\*----------------------------------------------------------------------*/
/** @addtogroup filedb_api */
/*! @{ */

#ifndef __FILEDB_H
#define __FILEDB_H

#include <glib.h>

/** File database directory */
#define FILEDB_DIR "filedb"

/** File data stored in database. */
struct fdb_file {
  gchar* path;  /**< File path. [required] */
  gchar* link;  /**< Target of the symlink. [optional] */
  guint16 mode; /**< File mode. */
  guint16 refs; /**< Reference count. */
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

/** Add file to database.
 *
 * @param file Pointer to the filled file information object.
 * @return 0 on error, fileid on success
 */
extern guint32 fdb_add_file(struct fdb_file* file);

/** Get file id from database.
 *
 * @param path File path.
 * @return 0 on error, fileid on success
 */
extern guint32 fdb_get_file_id(gchar* path);

/** Get file from database.
 *
 * @param id File id.
 * @param file Pointer to the file information object to be filled.
 * @return 0 on error, file_pld pointer on success
 */
extern gint fdb_get_file(guint32 id, struct fdb_file* file);

/** Delete file from database.
 *
 * @param id File id.
 * @return 0 on success, 1 on error
 */
extern gint fdb_del_file(guint32 id);

#endif

/*! @} */
