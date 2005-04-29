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

typedef enum { UNTGZ_NONE=0, UNTGZ_DIR, UNTGZ_REG, UNTGZ_LNK, UNTGZ_SYM, UNTGZ_BLK, UNTGZ_CHR, UNTGZ_FIFO, UNTGZ_SOCK } ftype_t;

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

/** @brief Open tgz archive.
 *
 * @param tgzfile Path to the tgz file.
 * @return 0 on error, untgz_state* on success.
 */
extern struct untgz_state* untgz_open(gchar* tgzfile);

/** @brief Read next object from the tgz archive.
 *
 * Needs to be called before untgz_write_data or untgz_write_file.
 *
 * @param tgzfile Path to the tgz file.
 * @return .
 */
extern gint untgz_get_header(struct untgz_state* s);

extern gint untgz_write_data(struct untgz_state* s, guchar** buf, gsize* len);
extern gint untgz_write_file(struct untgz_state* s, gchar* altname);

/** @brief Close tgz archive.
 *
 * @param s Untgz object returned by untgz_open.
 */
extern void untgz_close(struct untgz_state* s);

#endif
