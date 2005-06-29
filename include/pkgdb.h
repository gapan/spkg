/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup pkgdb_api Package Database API


*//*--------------------------------------------------------------------*/
/** @addtogroup pkgdb_api */
/*! @{ */

#ifndef __PKGDB_H
#define __PKGDB_H

#include <glib.h>
#include <time.h>
#include "error.h"

/** Directory where the package database is stored. */
#define PKGDB_DIR "var/log"

#define DB_NOPEN   E(0) /**< database is not open */
#define DB_OPEN    E(1) /**< database is already open (when opening) */
#define DB_CORRUPT E(2) /**< database is corrupted */
#define DB_NOTEX   E(3) /**< package not exist in database */
#define DB_EXIST   E(4) /**< package already in database */
#define DB_SQL     E(5) /**< sqlite error */
#define DB_BLOCKED E(6) /**< db is open by another proccess */

typedef enum {
  DB_GET_ALL=0, /**< get everything */
  DB_GET_BASIC, /**< get basic info (everyhing except file list and doinst.sh) */
  DB_GET_NAMES  /**< get only names of packages */
} db_get_type;

/** File information structure. 
 *
 * @li N - no touch: user should not modify such variable
 * @li R - required: user must set such variable
 * @li O - optional: user may set such variable
 * @li A - automat: automatically set variable
 */
struct db_file {
  guint32 id;   /**< unique file id asigned by filedb [N] */
  guint16 refs; /**< reference count of the file (by how many packages file is referenced) [N] */
  gchar* path;  /**< relative path to the file (relative to the root) [R] */
  gchar* link;  /**< path to the link target (if file is symlink or link) [O] */
};

/** Package information structure. */
struct db_pkg {
  /* name */
  gchar* name;       /**< full name of the package [R] */
  gchar* shortname;  /**< short name of the package [NA] */
  gchar* version;    /**< version of the package [NA] */
  gchar* arch;       /**< architecture of the package [NA] */
  gchar* build;      /**< build of the package [NA] */

  /* details */
  time_t time;       /**< package installation/upgrade time [NA] */
  gsize  csize;      /**< compressed size of the package in kB [O] */
  gsize  usize;      /**< uncompressed size of the package in kB [O] */
  gchar* location;   /**< original package location (path) [O] */
  gchar* desc;       /**< cleaned (i.e. without rubbish :) package description [O] */
  gchar* doinst;     /**< complete doinst.sh script contents [O] */
  
  GSList* files;     /**< list of the files in the package [O] */

  /* internals */
  gint id;           /**< unique package id assigned by pkgdb [N] */
};

/** Open package database.
 *
 * @param root root directory
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint db_open(const gchar* root, struct error* e);

/** Close package database. 
 */
extern void db_close();

/** Create package object.
 *
 * Package name is checked and automatically parsed into parts.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 if invalid name, \ref db_pkg object on success
 */
extern struct db_pkg* db_alloc_pkg(gchar* name);

/** Free \ref db_pkg object.
 *
 * @param pkg \ref db_pkg object
 */
extern void db_free_pkg(struct db_pkg* pkg);

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

/** Get package from the database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @param files If files should be included. (this takes extra time)
 * @return 0 if not found, \ref db_pkg object on success
 */
extern struct db_pkg* db_get_pkg(gchar* name, gboolean files);

/** Remove package from the database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 on success, 1 on error
 */
extern gint db_rem_pkg(gchar* name);

/** Add package to the legacy database.
 *
 * @param pkg \ref db_pkg object
 * @return 0 on success, 1 on error
 */
extern gint db_legacy_add_pkg(struct db_pkg* pkg);

/** Get package from legacy database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @param files If files should be included. (this takes extra time)
 * @return 0 if not found, \ref db_pkg object on success
 */
extern struct db_pkg* db_legacy_get_pkg(gchar* name, gboolean files);

/** Remove package from legacy database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 on success, 1 on error
 */
extern gint db_legacy_rem_pkg(gchar* name);

/** Get packages list from the database.
 *
 * @return packages list, 0 on error
 */
extern GSList* db_get_packages();

/** Get packages list from the legacy database.
 *
 * @return packages list, 0 on error
 */
extern GSList* db_legacy_get_packages();

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
