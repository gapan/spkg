/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup untgz_api */
/*! @{ */

#ifndef __UNTGZ_H
#define __UNTGZ_H

#include <glib.h>
#include <zlib.h>
#include <setjmp.h>

/** Enable or disable parent directory modification and access times 
 *  preservation. 
 */
#define UNTGZ_PRESERVE_DIR_TIMES 0

/** File type. */
typedef enum { 
  UNTGZ_NONE=0, /**< nothing read yet */
  UNTGZ_DIR,    /**< directory */
  UNTGZ_REG,    /**< regular file (with associated data) */
  UNTGZ_LNK,    /**< hard link */
  UNTGZ_SYM,    /**< symbolic link */
  UNTGZ_BLK,    /**< block device */
  UNTGZ_CHR,    /**< character device */
  UNTGZ_FIFO    /**< fifo */
} filetype_t;

/*! @if false */
/* optimal (according to the benchmark), must be multiple of 512 */
#define BLOCKBUFSIZE (512*64) 
/*! @endif */

/** Untgz state structure. */
struct untgz_state {
  /* archive information */
  gchar*  tgzfile;     /**< tgz file path */
  gsize   usize;       /**< uncompressed size of the files in the archive */
  gsize   csize;       /**< compressed size of the archive  */
  gzFile* gzf;         /**< gzio tar stream */

  /* current file information */
  filetype_t f_type;   /**< type of the current file */
  gchar*  f_name;      /**< name of the current file */
  gchar*  f_link;      /**< file that current file links to */
  gsize   f_size;      /**< size of the current file */
  gint    f_mode;      /**< mode of the current file */
  time_t  f_mtime;     /**< mtime of the current file */
  uid_t   f_uid;       /**< uid of the current file */
  gid_t   f_gid;       /**< gid of the current file */
  gchar   f_uname[33]; /**< user name of the current file */
  gchar   f_gname[33]; /**< group name of the current file */
  guint   f_devmaj;    /**< major number of the device */
  guint   f_devmin;    /**< minor number of the device */

  /* error handling */
  jmp_buf errjmp;      /**< used by the trigger_error for the 
                            exception-like error handling */
  gchar*  errstr;      /**< error string */

  mode_t  old_umask;   /**< saved umask */
  gboolean data;       /**< data from the current file were not read, yet */
  gboolean written;    /**< file was written */
  gboolean eof;        /**< end of archive reached */
  
  /* internal block buffer  */
  guchar   bbuf[BLOCKBUFSIZE]; /**< block buffer */
  guchar*  bend;       /**< points to the end of the buffer */
  guchar*  bpos;       /**< points to the current block */
  gint     blockid;    /**< block id (512*blockid == position of the block 
                            in the input file) */
};

/** Open tgz archive.
 *
 * @param tgzfile Path to the tgz archive.
 * @return Pointer to the \ref untgz_state object on success, 0 on error.
 */
extern struct untgz_state* untgz_open(gchar* tgzfile);

/** Read next file header from the archive.
 *
 * Needs to be called before untgz_write_data or untgz_write_file.
 *
 * @param s Pointer to the \ref untgz_state object.
 * @return 0 on success, 1 on end of archive, -1 on error.
 */
extern gint untgz_get_header(struct untgz_state* s);

/** Write data for the current file to the buffer.
 *
 * If file is empty, len will be set to 0 and buf will be unchanged.
 * If file has no data (s->data == 0), do nothing.
 *
 * @param s Pointer to the \ref untgz_state object.
 * @param buf Pointer to the pointer that will be updated with the address
 *            of the buffer with the data from the current file in archive.
 * @param len Pointer to the integer that will be updated with the size of
 *            the buffer.
 * @return 0 on success, 1 if no data, -1 on error.
 */
extern gint untgz_write_data(struct untgz_state* s, guchar** buf, gsize* len);

/** Write current file to the disk.
 *
 * @param s Pointer to the \ref untgz_state object.
 * @param altname Optional alternative name for the file that will be extracted
 *        from the archive. If this is 0 then the original name is used.
 * @return 0 on success, 1 if file was already written, -1 on error.
 */
extern gint untgz_write_file(struct untgz_state* s, gchar* altname);

/** Close archive and free \ref untgz_state object.
 *
 * @param s Pointer to the \ref untgz_state object.
 */
extern void untgz_close(struct untgz_state* s);

#endif

/*! @} */
