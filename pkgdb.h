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

#define DB_OK     0 /**< ok */
#define DB_ERROR  1 /**< error */
#define DB_NOPKG  2 /**< package not found */

/** File information structure. */
struct db_file {
  gchar* path; /**< full path to the file */
  gchar* link; /**< path to the link target (if file is symlink or link) */
  guint  dup:1; /**< true if file already exists in the archive */
  guint  padding:31; /**< just a padding to a dword */
};

/** Package information structure. */
struct db_pkg {
  /* name */
  gchar* name;       /**< full name of the package */
  gchar* shortname;  /**< short name of the package */
  gchar* version;    /**< version of the package */
  gchar* arch;       /**< architecture of the package */
  gchar* build;      /**< build of the package */

  /* details */
  gsize  csize;      /**< compressed size of the package */
  gsize  usize;      /**< uncompressed size of the package */
  gchar* location;   /**< original package location */
  gchar* desc;       /**< package description */
  
  GSList* files;     /**< list of the files in the package */
};

/** Open package database.
 *
 * @param root root directory
 * @return 0 on success, 1 on error
 */
extern gint db_open(gchar* root);

/** Close package database. 
 */
extern void db_close();

/** Returns description of the error if one occured in the last pkgdb
 * library call.
 *
 * @return Error string on error, 0 otherwise
 */
extern gchar* db_error();

/** Add package to the database.
 *
 * @param pkg \ref db_pkg object
 * @return 0 on success, 1 on error
 */
extern gint db_add_pkg(struct db_pkg* pkg);

/** Remove package from the database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 on success, 1 on error
 */
extern gint db_rem_pkg(gchar* name);

/** Get package from the database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @param files If files should be included. (this takes extra time)
 * @return 0 if not found, \ref db_pkg object on success
 */
extern struct db_pkg* db_get_pkg(gchar* name, gboolean files);

/** Add package to the legacy database.
 *
 * @param pkg \ref db_pkg object
 * @return 0 on success, 1 on error
 */
extern gint db_legacy_add_pkg(struct db_pkg* pkg);

/** Get package from legacy database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 if not found, \ref db_pkg object on success
 */
extern struct db_pkg* db_legacy_get_pkg(gchar* name);

/** Free \ref db_pkg object.
 *
 * @param pkg \ref db_pkg object
 */
extern void db_free_pkg(struct db_pkg* pkg);

/** Recreate legacydb from fastpkgdb.
 *
 * @return 0 on success, 1 on error
 */
extern gint db_sync_fastpkgdb_to_legacydb();

/** Recreate fastpkgdb from legacydb.
 *
 * @return 0 on success, 1 on error
 */
extern gint db_sync_legacydb_to_fastpkgdb();
 

#endif

/*! @} */
