/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup untgz_api Tgz Archive Extraction API

Untgz is the robust implementation of tgz archive browser/extractor.
It can open multiple files at once.

@section usage Typical usage

Following code shows typical \ref untgz_api usage.

@code
#include <stdlib.h>
#include <stdio.h>
#include "untgz.h"

void status(struct untgz_state* tgz, gsize total, gsize current)
{
  printf("%d\n", 100*current/total);
  fflush(stdout);
}

int main(int ac, char* av[])
{
  gint i;
  struct error* err = e_new();

  // For each file do:
  for (i=1;i<ac;i++)
  {
    // Open tgz file.
    struct untgz_state* tgz = untgz_open(av[i], status, err);
    if (tgz == 0)
    {
      e_print(err);
      e_clean(err);
      continue;
    }
    // While we can successfully get next file's header from the archive...
    while (untgz_get_header(tgz) == 0)
    {
      // ...we will be extracting that file to a disk using its original name...
      if (untgz_write_file(tgz, 0))
      {
        // ...until something goes wrong.
        break;
      }
    }
    // And if something went wrong...
    if (!e_ok(err))
    {
      // ...we will alert user.
      e_print(err);
      e_clean(err);
    }
    
    // Close file.
    untgz_close(tgz);
  }

  e_free(err);
  return 0;
}
@endcode

*//*--------------------------------------------------------------------*/
/** @addtogroup untgz_api */
/*! @{ */

#ifndef SPKG__UNTGZ_H
#define SPKG__UNTGZ_H

#include <glib.h>
#include <time.h>
#include <sys/stat.h>

#include "error.h"

#define UNTGZ_CORRUPT E(0) /**< archive is corrupt */
#define UNTGZ_BLOCKED E(1) /**< untgz is blocked because of corrupt archive */
#define UNTGZ_BADIO E(2) /**< can't open/write/create/update/whatever file */
#define UNTGZ_BADMETA E(3) /**< can't set metainformation for extracted file 
                           (file was extracted successfully!) */

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
} untgz_filetype;

/** internal opaque data type */
struct untgz_state_internal;

/** Untgz state structure. */
struct untgz_state {
  /* archive information */
  gchar*  tgzfile;     /**< tgz file path */
  gsize   usize;       /**< sum of sizes of the uncompressed files */
  gsize   csize;       /**< compressed size of the archive  */

  /* current file information */
  untgz_filetype f_type; /**< type of the current file */
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

  struct untgz_state_internal* i; /**< this is no-no for a library user */
};

/** Untgz status callback. */
typedef void(*untgz_status_cb)(struct untgz_state* s, gsize total, gsize current);

/** Open tgz archive.
 *
 * @param tgzfile Path to the tgz archive.
 * @param scb Status callback. If you don't need this, set it to zero.
 *            Note that if archive is corrupted, behavior is undefined.
 *            Also note, that use of callback is not possible on files
 *            where lseek is not posible.
 * @param e Error object.
 * @return Pointer to the \ref untgz_state object on success, 0 on error.
 */
extern struct untgz_state* untgz_open(const gchar* tgzfile, untgz_status_cb scb, struct error* e);

/** Close archive and free \ref untgz_state object.
 *
 * @param s Pointer to the \ref untgz_state object.
 */
extern void untgz_close(struct untgz_state* s);

/** Read next file header from the archive.
 *
 * Needs to be called before \ref untgz_write_data or \ref untgz_write_file.
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
 * Buffer is appended with 0 byte. (zero terminated)
 *
 * @param s Pointer to the \ref untgz_state object.
 * @param buf Pointer to the pointer that will be updated with the address
 *            of the buffer with the data from the current file in archive.
 * @param len Pointer to the integer that will be updated with the size of
 *            the buffer.
 * @return 0 on success, 1 if no data, -1 on error.
 */
extern gint untgz_write_data(struct untgz_state* s, gchar** buf, gsize* len);

/** Write current file to the disk.
 *
 * @param s Pointer to the \ref untgz_state object.
 * @param altname Optional alternative name for the file that will be extracted
 *        from the archive. If this is 0 then the original name is used.
 * @return 0 on success, 1 if file was already written, -1 on error.
 */
extern gint untgz_write_file(struct untgz_state* s, gchar* altname);

#endif

/*! @} */
