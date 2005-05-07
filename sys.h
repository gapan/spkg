/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#ifndef __SYS_H
#define __SYS_H

#include <glib.h>

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

extern sys_ftype sys_file_type(gchar* path);

extern gint sys_rm_rf(gchar* p);
extern gint sys_mkdir_p(gchar* p);

/*XXX: remove (move to cli) */
extern gint verbose;
extern void notice(const gchar* f,...);
extern void err(gint e, const gchar* f,...);
extern void warn(const gchar* f,...);

#endif
