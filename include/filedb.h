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

*//*--------------------------------------------------------------------*/
/** @addtogroup filedb_api */
/*! @{ */

#ifndef __FILEDB_H
#define __FILEDB_H

#include <glib.h>

/** Filedb handle. */
struct fdb;

/** File data stored in database. */
struct fdb_file {
  gchar* path;  /**< File path. [required] */
  guint16 refs; /**< Reference count. */
  /* arbitrary data from here down */
  gchar* link;  /**< Target of the symlink. [optional] */
};

#define FDB_OK    0 /**< no error */
#define FDB_EXIST 1 /**< file exist */
#define FDB_NOTEX 2 /**< file not exist */
#define FDB_OTHER 3 /**< other error */
#define FDB_NOPEN 2 /**< filedb not open */

/** Open file database.
 *
 * You should check for errors using \ref fdb_error(). If fd was
 * successfully open, \ref fdb_error() will return 0.
 *
 * @param path Path to the file database directory.
 * @return fdb handle (always)
 */
extern struct fdb* fdb_open(const gchar* path);

/** Close file database.
 *
 * @param db File database handle.
 */
extern void fdb_close(struct fdb* db);

/** Returns description of the error if one occured in the last filedb library call.
 *
 * @param db File database handle.
 * @return Error string on error, 0 otherwise
 */
extern const gchar* fdb_error(struct fdb* db);

/** Returns error number (status of the last fdb library function call).
 *
 * @param db File database handle.
 * @return Error number
 */
extern gint fdb_errno(struct fdb* db);

/** Add file to the database.
 *
 * @param db File database handle.
 * @param file Pointer to the filled file information object.
 * @return 0 on error, fileid on success
 */
extern guint32 fdb_add_file(struct fdb* db, const struct fdb_file* file);

/** Get file id from the database.
 *
 * @param db File database handle.
 * @param path File path.
 * @return 0 on error, fileid on success
 */
extern guint32 fdb_get_file_id(struct fdb* db, const gchar* path);

/** Get file from database.
 *
 * @param db File database handle.
 * @param id File id.
 * @param file Pointer to the file information object to be filled. Data
 *        will be valid until next library call.
 * @return 0 on error, file_pld pointer on success
 */
extern gint fdb_get_file(struct fdb* db, const guint32 id, struct fdb_file* file);

/** Delete file from database.
 *
 * @param db File database handle.
 * @param id File id.
 * @return 0 on success, 1 on error
 */
extern gint fdb_rem_file(struct fdb* db, const guint32 id);

#endif

/*! @} */
