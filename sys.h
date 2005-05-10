/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __SYS_H
#define __SYS_H

#include <glib.h>

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

#endif

/*! @} */
