/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#ifndef __UNTGZ_H
#define __UNTGZ_H

#include <glib.h>
#include <zlib.h>
#include <setjmp.h>

#ifndef __FILETYPE_ENUM
#define __FILETYPE_ENUM
typedef enum { FT_NONE=0, FT_DIR, FT_REG, FT_LNK, FT_BLK, FT_CHR, FT_FIFO, FT_SOCK } ftype_t;
#endif

typedef struct untgz_state untgz_state_t;
struct untgz_state {
  gchar*  tgzfile; /* tgzfile path */
  gsize   usize;
  gsize   csize;
  gzFile* gzf; /* tar stream */

  ftype_t f_type;
  gchar*  f_name; /* file name */
  gchar*  f_link; /* link target (if it is link) */
  gsize   f_size;
  gint    f_mode;
  time_t  f_mtime;
  uid_t   f_uid;
  gid_t   f_gid;
  gchar   f_uname[33];
  gchar   f_gname[33];
  guint   f_devmaj;
  guint   f_devmin;
  
  jmp_buf errjmp;
  gchar*  errstr;
  
  gboolean data;
  gboolean eof;
};

extern untgz_state_t* untgz_open(gchar* tgzfile);
extern gint untgz_get_next_head(untgz_state_t* s);
extern gint untgz_get_next_data(untgz_state_t* s, gchar* file);
extern void untgz_close(untgz_state_t* s);

#endif
