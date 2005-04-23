/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#ifndef __PKGDB_H
#define __PKGDB_H

#include <glib.h>
#include <setjmp.h>
#include <sqlite3.h>

#define PKGDB_DIR "var/log"

typedef struct pkgdb pkgdb_t;
typedef struct pkgdb_pkg pkgdb_pkg_t;

/** Package database structure. */
struct pkgdb {
  /** Package database top directory path. */
  gchar* topdir;
  gchar* fpkdir;
  gchar* fpkdb;
  sqlite3 *db;
};

/** Package information structure. */
struct pkgdb_pkg {
  /** Name. */
  gchar* name;
  gchar* shortname;
  gchar* version;
  gchar* arch;
  gchar* build;

  /** Compressed size. */
  gsize  csize;
  /** Uncompressed size. */
  gsize  usize;
  /** Original package location. */ 
  gchar* location;
  /** Package description. */ 
  gchar* desc;
  /** File list.
   *
   * key = path, val = link target, NULL otherwise.
   */
  GTree* files;
};

/** @brief Open package database and loads it from the disk.
 *
 * @param root Root directory.
 * @return Package database object.
 */
extern pkgdb_t* db_open(gchar* root);

/** @brief Close package database.
 *
 * @param db Package database object.
 */
extern void db_close(pkgdb_t* db);

/** @brief Recreate legacydb from fastpkgdb.
 *
 * @param db Package database object.
 * @return 0 on success, 1 on error
 */
extern gint db_sync_fastpkgdb_to_legacydb(pkgdb_t* db);

/** @brief Recreate fastpkgdb from legacydb.
 *
 * @param db Package database object.
 * @return 0 on success, 1 on error
 */
extern gint db_sync_legacydb_to_fastpkgdb(pkgdb_t* db);
 
/** @brief Find package by name.
 *
 * @param db Package database object.
 * @param pkg Package full or short name.
 * @return 0 if not found, 1 if exact match, 2 if shortname match
 */
extern pkgdb_pkg_t* db_find_pkg(pkgdb_t* db, gchar* pkg);

#endif
