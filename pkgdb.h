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

#ifndef __PKGDB_H
#define __PKGDB_H

#include <glib.h>

/*! @if false */
#define PKGDB_DIR "var/log"
/*! @endif */

/** Package information structure. */
struct db_pkg {
  /* name */
  gchar* name;       /**< full name of the package */
  gchar* shortname;  /**< short name of the package */
  gchar* version;    /**< version of the package */
  gchar* arch;       /**< architecture of the package */
  gchar* build;      /**< build of the package */
  /* size */
  gsize  csize;      /**< compressed size of the package */
  gsize  usize;      /**< uncompressed size of the package */

  gchar* location;   /**< original package location */
  gchar* desc;       /**< package description */
  GSList* files;     /**< list of the files in the package */
};

/** Open package database.
 *
 * Database is checked for validity.
 *
 * @param root Root directory.
 * @return Package database object.
 */
extern gint db_open(gchar* root);

/** Close package database.
 *
 * @param db Package database object.
 */
extern void db_close();

/** Recreate legacydb from fastpkgdb.
 *
 * @param db Package database object.
 * @return 0 on success, 1 on error
 */
extern gint db_sync_fastpkgdb_to_legacydb();

/** Recreate fastpkgdb from legacydb.
 *
 * @param db Package database object.
 * @return 0 on success, 1 on error
 */
extern gint db_sync_legacydb_to_fastpkgdb();
 
/** Find package by name.
 *
 * @param db Package database object.
 * @param pkg Package full or short name.
 * @return 0 if not found, 1 if exact match, 2 if shortname match
 */
extern struct db_pkg* db_find_pkg(gchar* pkg);

#endif

/*! @} */
