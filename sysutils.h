/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#ifndef __SYSUTILS_H
#define __SYSUTILS_H

#include <glib.h>

#ifndef __FILETYPE_ENUM
#define __FILETYPE_ENUM
typedef enum { FT_NONE=0, FT_DIR, FT_REG, FT_LNK, FT_BLK, FT_CHR, FT_FIFO, FT_SOCK } ftype_t;
#endif

extern gint verbose;

extern void notice(const gchar* f,...);
extern void err(gint e, const gchar* f,...);
extern void warn(const gchar* f,...);

extern gint file_type(gchar* path);

#endif
