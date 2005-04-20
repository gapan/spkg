#ifndef __PKGDB_H
#define __PKGDB_H

#include <glib.h>
#include <setjmp.h>

#define PKGDB_DIR "var/log"

typedef struct pkgdb pkgdb_t;
typedef struct pkgdb_pkg pkgdb_pkg_t;

/** Package database structure. */
struct pkgdb {
  /** Package database top directory path. */
  gchar* dbdir;
  /** Packages tree. 
   *
   * key = name, val = \ref pkgdb_pkg_t*
   */
  GTree* pkgs;
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
extern pkgdb_t* pkgdb_open(gchar* root);

/** @brief Close package database.
 *
 * @param db Package database object.
 * @bug Glib leaks memory.
 */
extern void pkgdb_close(pkgdb_t* db);

/** @brief Load packge to the datasbase.
 *
 * @param db Package database object.
 * @param pkg Package name.
 * @return 0 on success, 1 if pkg not found
 */
extern gint pkgdb_load_pkg(pkgdb_t* db, gchar* pkg);

/** @brief Find package by the name.
 *
 * @param db Package database object.
 * @param pkg Package full name.
 * @return 0 if not found, 1 if exact match, 2 if shortname match
 */
extern pkgdb_pkg_t* pkgdb_find_pkg(pkgdb_t* db, gchar* pkg);

#endif
