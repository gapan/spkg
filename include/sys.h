/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup other_api Misc API


*//*--------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __SYS_H
#define __SYS_H

#include <glib.h>
#include <signal.h>
#include <time.h>

#include "error.h"

/** File type. */
typedef enum { 
  SYS_ERR=0, /**< can't determine type */
  SYS_NONE,  /**< file does not exist */
  SYS_DIR,   /**< directory */
  SYS_REG,   /**< regular file */
  SYS_SYM,   /**< symbolic link */
  SYS_BLK,   /**< block device */
  SYS_CHR,   /**< character device */
  SYS_FIFO,  /**< fifo */
  SYS_SOCK   /**< socket */
} sys_ftype;

/** Get type of the file.
 *
 * @param path File path.
 * @param deref Dereference symlinks.
 * @return \ref sys_ftype
 */
extern sys_ftype sys_file_type(const gchar* path, gboolean deref);

/** Get mtime of the file.
 *
 * @param path File path.
 * @param deref Dereference symlinks.
 * @return -1 on error, mtime on success
 */
extern time_t sys_file_mtime(const gchar* path, gboolean deref);

/** Implementation of the rm -rf.
 *
 * @param path File path.
 * @return 0 on success, 1 on error
 */
extern gint sys_rm_rf(const gchar* path);

/** Implementation of the mkdir -p.
 *
 * @param path Directory path.
 * @return 0 on success, 1 on error
 */
extern gint sys_mkdir_p(const gchar* path);

/** Set cwd and return pwd.
 *
 * @param path new working directory. (0 if you want to get cwd)
 * @return 0 on failure, pointer to pwd on success
 */
extern gchar* sys_setcwd(const gchar* path);

/** Block all signals.
 *
 * @param sigs pointer to the place where will be stored original sigset_t
 */
extern void sys_sigblock(sigset_t* sigs);

/** Unblock all signals.
 *
 * @param sigs pointer to the place where \ref sys_sigblock stored original sigset_t
 */
extern void sys_sigunblock(sigset_t* sigs);

/** Create new lock.
 *
 * @param path lock file path
 * @param e error object
 * @return -1 on failure, lock file descriptor on success
 */
extern gint sys_lock_new(const gchar* path, struct error* e);

/** Try to acquire lock.
 *
 * @param fd lock file descriptor
 * @param timeout timeout in 100ms units
 * @param e error object
 * @return 1 on failure, 0 on success
 */
extern gint sys_lock_trywait(gint fd, gint timeout, struct error* e);

/** Give up lock.
 *
 * @param fd lock file descriptor
 * @param e error object
 * @return 1 on failure, 0 on success
 */
extern gint sys_lock_put(gint fd, struct error* e);

/** Close lock file.
 *
 * @param fd lock file descriptor
 */
extern void sys_lock_del(gint fd);

#endif

/*! @} */
