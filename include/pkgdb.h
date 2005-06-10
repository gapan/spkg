/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup pkgdb_api Package Database API


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
#define DB_CLOSED 1 /**< database is [already] closed (on any call except db_open) */
#define DB_OPEN   2 /**< database is already open (when opening) */
#define DB_OTHER  3 /**< other error */
#define DB_NOTEX  4 /**< package not exist (when getting from db) */
#define DB_EXIST  5 /**< other package with same name exist (when adding into db) */

/** File information structure. */
struct db_file {
  guint32 id;   /**< filedb file id */
  guint16 refs; /**< true if file already exists in the archive */
  gchar* path;  /**< full path to the file */
  gchar* link;  /**< path to the link target (if file is symlink or link) */
  guint16 mode; /**< file mode */
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

  /* internals */
  gint id;           /**< unique package id */
};

/** Open package database.
 *
 * @param root root directory
 * @return 0 on success, 1 on error
 */
extern gint db_open(const gchar* root);

/** Close package database. 
 */
extern void db_close();

/** Returns description of the error if one occured in the last pkgdb
 * library call.
 *
 * @return Error string on error, 0 otherwise
 */
extern gchar* db_error();

/** Returns the error number if error occured in the last pkgdb
 * library call.
 *
 * @return Nonzero error number on error, 0 otherwise
 */
extern gint db_errno();

/** Create package object.
 *
 * Package name is checked and parsed.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 if invalid name, \ref db_pkg object on success
 */
extern struct db_pkg* db_alloc_pkg(gchar* name);

/** Create file object.
 *
 * @param path File path. (it is not strduped)
 * @param link File link (if it is symlink). (it is not strduped)
 * @return \ref db_file object on success
 */
extern struct db_file* db_alloc_file(gchar* path, gchar* link);

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

/** Get packages list from the database.
 *
 * @return packages list, 0 on error
 */
extern GSList* db_get_packages();

/** Free packages list returned by \ref db_get_packages().
 *
 * @param pkgs Packages list returned by \ref db_get_packages()
 */
extern void db_free_packages(GSList* pkgs);

/** Synchronize legacy databse with spkg database.
 *
 * @return 0 on success, 1 on error
 */
extern gint db_sync_to_legacydb();

/** Synchronize spkg databse with legacy database.
 *
 * @return 0 on success, 1 on error
 */
extern gint db_sync_from_legacydb();
 

#endif

/*! @} */
