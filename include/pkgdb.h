/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup pkgdb_api Package Database API

\ref pkgdb_api consists of three groups of functions:

\section funcs1 Database manipulation functions
\li \ref db_open and \ref db_close

\section funcs2 Package manipulation functions
\li \ref db_get_pkg, \ref db_add_pkg and \ref db_rem_pkg

\section funcs3 Packages list query functions
These functions returns list of package names.
\li \ref db_query

*/
/** @addtogroup pkgdb_api */
/*! @{ */

#ifndef SPKG__PKGDB_H
#define SPKG__PKGDB_H

#include <glib.h>
#include <time.h>
#include "error.h"

G_BEGIN_DECLS

/** Directory where the package database is stored. */
#define PKGDB_DIR "var/log"

#define DB_NOPEN   E(0) /**< Database is not open. */
#define DB_OPEN    E(1) /**< Database is already open (when opening). */
#define DB_CORRUPT E(2) /**< Database is corrupted. */
#define DB_NOTEX   E(3) /**< Package not exist in database. */
#define DB_EXIST   E(4) /**< Package already in database. */
#define DB_BLOCKED E(6) /**< DB is open by another proccess. */

#ifndef MAXPATHLEN
#define MAXPATHLEN 8192 /**< Maximum length of the path. */
#endif

/** What to get when getting package from database. */
typedef enum {
  DB_GET_FULL,         /**< get everything */
  DB_GET_WITHOUT_FILES /**< get basic info (everyhing except file list and doinst.sh) */
} db_get_type;

/** What to get when querying package list from database. */
typedef enum {
  DB_QUERY_PKGS_WITH_FILES,    /**< query list of actual packages with files */
  DB_QUERY_PKGS_WITHOUT_FILES, /**< query list of actual packages without files */
  DB_QUERY_NAMES               /**< query list of package names */
} db_query_type;

/** Path file type. */
typedef enum {
  DB_PATH_NONE = 0, /**< No such path. */
  DB_PATH_FILE,     /**< File. */
  DB_PATH_DIR,      /**< Directory. */
  DB_PATH_SYMLINK   /**< Symlink from the script. */
} db_path_type;

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
  guint  csize;      /**< compressed size of the package in kB [O] */
  guint  usize;      /**< uncompressed size of the package in kB [O] */
  gchar* location;   /**< original package location (path) [O] */
  gchar* desc;       /**< cleaned (i.e. without rubbish :) package description [O] */
  gchar* doinst;     /**< complete doinst.sh script contents [O] */

  /* file list */  
  void* paths;       /**< [JudySL->db_path_type] list of normalized paths in the package [O] */
};

/** Open package database.
 *
 * @param root root directory
 * @param readonly open package database in redonly mode
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint db_open(const gchar* root, gboolean readonly, struct error* e);

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

/** Add path to the package.
 *
 * @param pkg [db_pkg] Package object.
 * @param path Path.
 * @param type Path type.
 * @return 1 on failure (path too long), 0 on success.
 */
extern gint db_pkg_add_path(struct db_pkg* pkg, const gchar* path, db_path_type type);

/** Get path from the package.
 *
 * @param pkg [db_pkg] Package object.
 * @param path Path.
 * @return Path type.
 */
extern db_path_type db_pkg_get_path(struct db_pkg* pkg, const gchar* path);

/** Add package to the database.
 *
 * @param pkg [db_pkg] Package object.
 * @return 0 on success, 1 on error
 */
extern gint db_add_pkg(struct db_pkg* pkg);

/** Get package from the database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @param type What to get. See \ref db_get_type.
 * @return 0 if not found, \ref db_pkg object on success
 */
extern struct db_pkg* db_get_pkg(gchar* name, db_get_type type);

/** Remove package from the database.
 *
 * @param name Package name (something like: blah-1.0-i486-1)
 * @return 0 on success, 1 on error
 */
extern gint db_rem_pkg(gchar* name);

/** Replace package @b origname with package @b pkg.
 *
 * @param origname Original package name (something like: blah-1.0-i486-1)
 * @param pkg [db_pkg] Package object.
 * @return 0 on success, 1 on error
 */
extern gint db_replace_pkg(gchar* origname, struct db_pkg* pkg);

/** Package selector callback function.
 *
 * @param pkg filled package structure
 * @param data arbitrary data passed from the \ref db_query function.
 * @return 1 if package should be selected, 0 otherwise, -1 on error.
 */
typedef gint (*db_selector)(const struct db_pkg* pkg, void* data);

/** Get packages list from the database.
 *
 * @param cb package selector callback function
 * @param data arbitrary data passed to the package selector function
 * @param type format of result. see \ref db_query_type
 * @return list of package names, 0 if empty or error
 */
extern GSList* db_query(db_selector cb, void* data, db_query_type type);

/** Free packages list returned by \ref db_query().
 *
 * @param pkgs list of package names returned by \ref db_query()
 * @param type must be the same as when the query was called. see \ref db_query_type
 */
extern void db_free_query(GSList* pkgs, db_query_type type);

/** Get full name of the installed package from namespec.
 *
 * Namespec can be:
 * - shortname: kdebase
 * - name: kdebase-3.3-i486-1
 * - file: kdebase-3.3-i486-1.tgz
 *
 * @param name Namespec.
 * @return Full package name.
 */
extern gchar* db_get_package_name(const gchar* name);

/** Load filelist from the package database.
 *
 * @param force_reload If filelist is already loaded thuis function does nothing.
 * Set this parameter to true if you want to reload already loaded filelist.
 * @return 0 on success, 1 on failure.
 */
extern gint db_filelist_load(gboolean force_reload);

/** Free loaded filelist.
 */
extern void db_filelist_free();

/** Lookup path refs in the filelist.
 *
 * @param path Path.
 * @return Number of references to the path in various packages.
 */
extern gulong db_filelist_get_path_refs(const gchar* path);

/** Add paths from the package to the filelist.
 *
 * @param pkg Package object.
 */
extern void db_filelist_add_pkg_paths(const struct db_pkg* pkg);

/** Remove paths from the package from the filelist.
 *
 * @param pkg Package object.
 */
extern void db_filelist_rem_pkg_paths(const struct db_pkg* pkg);

G_END_DECLS

#endif

/*! @} */
