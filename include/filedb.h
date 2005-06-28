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
#include "error.h"

/** Filedb handle. */
struct fdb;

/** File data stored in database. */
struct fdb_file {
  gchar* path;  /**< File path. [required] */
  guint16 refs; /**< Reference count. */
  /* arbitrary data from here down */
  gchar* link;  /**< Target of the symlink. [optional] */
};

#define FDB_NOTEX   E(0) /**< file not exist in database */
#define FDB_NOPEN   E(1) /**< database is not open */
#define FDB_FULL    E(2) /**< db full */
#define FDB_CORRUPT E(3) /**< db corrupted */
#define FDB_BLOCKED E(4) /**< db is open by another proccess */

/** Open file database.
 *
 * @param path Path to the file database directory.
 * @param e Error object.
 * @return fdb handle if ok
 */
extern struct fdb* fdb_open(const gchar* path, struct error* e);

/** Close file database.
 *
 * @param db File database handle.
 */
extern void fdb_close(struct fdb* db);

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
