/*----------------------------------------------------------------------*\
|* spkg - Slackware Linux Fast Package Management Tools                 *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup other_api Misc API


\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __SYS_H
#define __SYS_H

#include <glib.h>
#include <signal.h>

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

/** Directory part of the path.
 *
 * @param path path
 * @return directory part of the path
 */
extern gchar* sys_dirname(const gchar* path);

/** Last part of the path.
 *
 * @param path path
 * @return last part of the path
 */
extern gchar* sys_basename(const gchar* path);

#endif

/*! @} */
