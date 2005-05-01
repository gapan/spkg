/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>

#include "untgz.h"

#define PRESERVE_DIRECTORY_MTIME 0

#if PRESERVE_DIRECTORY_MTIME == 1
#include <libgen.h>
#endif

/* private
 ************************************************************************/

#define AREGTYPE '\0'           /* regular file */
#define REGTYPE  '0'            /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

/* implemented GNU tar extensions */
#define GNUTYPE_LONGLINK 'K'    /* long link name */
#define GNUTYPE_LONGNAME 'L'    /* long file name */

/* unimplemented GNU tar extensions */
#define GNUTYPE_SPARSE   'S'    /* sparse file */
#define GNUTYPE_DUMPDIR  'D'    /* file names from dumped directory */
#define GNUTYPE_MULTIVOL 'M'    /* continuation of file from another volume */
#define GNUTYPE_NAMES    'N'    /* file name that does not fit into main hdr */
#define GNUTYPE_VOLHDR   'V'    /* tape/volume header */

#define BLOCKSIZE 512
#define SHORTNAMESIZE 100

struct tar_header {            /* byte offset */
  char name[100];              /*   0 */
  char mode[8];                /* 100 */
  char uid[8];                 /* 108 */
  char gid[8];                 /* 116 */
  char size[12];               /* 124 */
  char mtime[12];              /* 136 */
  char chksum[8];              /* 148 */
  char typeflag;               /* 156 */
  char linkname[100];          /* 157 */
  char magic[6];               /* 257 */
  char version[2];             /* 263 */
  char uname[32];              /* 265 */
  char gname[32];              /* 297 */
  char devmajor[8];            /* 329 */
  char devminor[8];            /* 337 */
  char prefix[155];            /* 345 */
};

union tar_block {
  struct tar_header h;
  guchar b[BLOCKSIZE];
};

static void throw_error(struct untgz_state* s, const gchar* format, ...)
{
  gchar* errstr;
  va_list ap;
  va_start(ap, format);
  errstr = g_strdup_vprintf(format, ap);
  va_end(ap);
  s->errstr = errstr;
  longjmp(s->errjmp, 1);
}

/* optimized conversion from octal ascii digits to uint */
static guint getoct(struct untgz_state* s, gchar *p, guint w)
{
  guint r=0, i;
  static const guchar oct_tab[256] = {
    [0 ... 255] = 0x10,
    ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3,
    ['4'] = 4, ['5'] = 5, ['6'] = 6, ['7'] = 7,
    ['\0'] = 0x20, [' '] = 0x20
  };

  for (i=0; i<w; i++)
  {
    guchar c = oct_tab[(guchar)p[i]];
    if (G_UNLIKELY(c == 0x20))
      break;
    else if (G_UNLIKELY(c == 0x10))
      throw_error(s, "%d: corrupted tgz archive (checksumming data block?)", s->blockid);
    else
      r = r*8+c;
  }
  return r;
}

/* header checksum validation */
static void validate_header_csum(struct untgz_state* s)
{
  guint i, head_csum, real_csum=0;
  union tar_block* b = (union tar_block*)s->bpos;

  head_csum = getoct(s, b->h.chksum, 6);
  memcpy(b->h.chksum, "        ", sizeof(b->h.chksum));
  for (i=0; G_LIKELY(i<BLOCKSIZE); i+=2)
    real_csum += b->b[i] + b->b[i+1];
  if (G_UNLIKELY(real_csum != head_csum))
    throw_error(s, "%d: corrupted tgz archive (invalid header checksum)", s->blockid);
}

/* this function returns pointer to the next tar block
 * return value is ptr if pointer points to the next block
 *                 0 if eof block occured (header with first byte zero)
 *                 longjmp on read error (or incomplete block)
 */
static union tar_block* read_next_block(struct untgz_state* s, gboolean is_header)
{
  s->bpos += BLOCKSIZE;
  if (G_UNLIKELY(s->bpos >= s->bend))
  { /* reload */
    gint read = gzread(s->gzf, s->bbuf, BLOCKBUFSIZE);
    if (read < BLOCKSIZE)
    {
      if (read == 0 && !gzeof(s->gzf))
        throw_error(s, "%d: corrupted tgz archive (gzread failed)", s->blockid);
      throw_error(s, "%d: corrupted tgz archive (early EOF)", s->blockid);
    }
    s->bpos = s->bbuf;
    s->bend = s->bbuf + read;
  }
  if (is_header)
  {
    if (G_UNLIKELY(s->bpos[0] == 0))
      return 0;
    validate_header_csum(s);
  }
  s->blockid++;
  return (union tar_block*)s->bpos;
}

/* append two strings (reallocate memmory) */
static gchar* strnappend(gchar* dst, gchar* src, gsize size)
{
  gsize newsize = 1+size;
  if (G_UNLIKELY(src == 0))
    return dst;
  if (G_LIKELY(dst != 0))
  {
    newsize += strlen(dst);
    dst = g_realloc(dst, newsize);
  }
  else
    dst = g_malloc0(newsize);
  dst = strncat(dst, src, size);
  return dst;
}

/* public 
 ************************************************************************/

struct untgz_state* untgz_open(gchar* tgzfile)
{
  struct untgz_state* s;
  gzFile *gzf;
  struct stat st;
  
  if (tgzfile == 0)
    return 0;
  if (stat(tgzfile, &st) == -1)
    return 0;
  gzf = gzopen(tgzfile, "rb");
  if (gzf == 0)
    return 0;

  s = g_new0(struct untgz_state,1);
  s->tgzfile = g_strdup(tgzfile);
  s->gzf = gzf;
  s->csize = st.st_size;
  s->old_umask = umask(0);
  s->blockid = -1;
  return s;
}

gint untgz_get_header(struct untgz_state* s)
{
  gsize remaining;
  union tar_block* b;

  if (G_UNLIKELY(s == 0 || s->errstr))
    return -1;
  if (G_UNLIKELY(s->eof))
    return 1;
  if (G_UNLIKELY(setjmp(s->errjmp) != 0))
    return -1;
  
  /* skip data blocks if write_data or write_file was not called after previous get_header */
  if (s->f_type == UNTGZ_REG && s->data)
  {
    remaining = s->f_size;
    while (G_LIKELY(remaining > 0))
    {
      if (read_next_block(s, 0) == 0)
        throw_error(s, "%d: corrupted tgz archive (missing data block)", s->blockid);
      remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
    }
  }

  /* reset state */
  g_free(s->f_name);
  g_free(s->f_link);
  s->f_name = s->f_link = 0;
  s->f_type = UNTGZ_NONE;
  s->data = 0;
  s->written = 0;

  while (1)
  {
    /* read next header */
    b = read_next_block(s, 1);
    if (b == 0)
    {
      /* empty header => end of archive */
      s->eof = 1;
      return 1;
    }

    remaining = getoct(s, b->h.size, 12);
    /* read longname and/or longlink */
    if (b->h.typeflag == GNUTYPE_LONGLINK)
    {
      while (G_LIKELY(remaining > 0))
      {
        b = read_next_block(s, 0);
        if (b == 0)
          throw_error(s, "%d: corrupted tgz archive (missing longlink data block)", s->blockid);
        s->f_link = strnappend(s->f_link, b->b, BLOCKSIZE);
        remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
      }
      continue;
    }
    else if (b->h.typeflag == GNUTYPE_LONGNAME)
    {
      while (G_LIKELY(remaining > 0))
      {
        b = read_next_block(s, 0);
        if (b == 0)
          throw_error(s, "%d: corrupted tgz archive (missing longname data block)", s->blockid);
        s->f_name = strnappend(s->f_name, b->b, BLOCKSIZE);
        remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
      }
      continue;
    }
    break;
  }

  /* parse header */
  s->f_size = remaining;
  s->f_mode = getoct(s, b->h.mode, sizeof(b->h.mode));
  s->f_uid  = getoct(s, b->h.uid, sizeof(b->h.uid));
  s->f_gid  = getoct(s, b->h.gid, sizeof(b->h.gid));
  s->f_mtime = (time_t) getoct(s, b->h.mtime, sizeof(b->h.mtime));
  s->f_devmaj = getoct(s, b->h.devmajor, sizeof(b->h.devmajor));
  s->f_devmin = getoct(s, b->h.devminor, sizeof(b->h.devminor));
  strncpy(s->f_uname, b->h.uname, sizeof(b->h.uname));
  strncpy(s->f_gname, b->h.gname, sizeof(b->h.gname));

  switch (b->h.typeflag)
  {
    case AREGTYPE: 
    case REGTYPE: s->f_type = UNTGZ_REG; s->data = 1; s->usize += remaining; break;
    case DIRTYPE: s->f_type = UNTGZ_DIR; break;
    case SYMTYPE: s->f_type = UNTGZ_SYM; break;
    case LNKTYPE: s->f_type = UNTGZ_LNK; break;
    case CHRTYPE: s->f_type = UNTGZ_CHR; break;
    case BLKTYPE: s->f_type = UNTGZ_BLK; break;
    default: throw_error(s, "%d: \"corrupted\" tgz archive (unimplemented typeflag [%c])", s->blockid, b->h.typeflag);
  }

  if (s->f_name == 0)
    s->f_name = g_strndup(b->h.name, SHORTNAMESIZE);
  /* just one more (unnecessary) check */
  else if (strncmp(s->f_name, b->h.name, SHORTNAMESIZE-1)) /* -1 because it's zero terminated */
    throw_error(s, "%d: corrupted tgz archive (longname mismatch)", s->blockid);
  if (s->f_link == 0)
    s->f_link = g_strndup(b->h.linkname, SHORTNAMESIZE);
  /* just one more (unnecessary) check */
  else if (strncmp(s->f_link, b->h.linkname, SHORTNAMESIZE-1)) /* -1 because it's zero terminated */
    throw_error(s, "%d: corrupted tgz archive (longlink mismatch)", s->blockid);
  return 0;
}

gint untgz_write_data(struct untgz_state* s, guchar** buf, gsize* len)
{
  guchar* buffer=0;  
  union tar_block* b;
  gsize position=0;
  gsize remaining;

  if (s == 0 || s->errstr || buf == 0 || len == 0)
    return -1;
  if (s->f_type != UNTGZ_REG || s->data == 0) /* no data for current file */
    return 1;
  if (setjmp(s->errjmp) != 0)
  {
    g_free(buffer);
    return -1;
  }

  remaining=s->f_size;
  buffer = g_malloc(remaining);
  while (G_LIKELY(remaining > 0))
  {
    b = read_next_block(s, 0);
    if (b == 0)
      throw_error(s, "%d: corrupted tgz archive (missing data block)", s->blockid);
    memcpy(buffer+position, b, remaining>BLOCKSIZE?BLOCKSIZE:remaining);
    remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
    position += BLOCKSIZE;
  }
  s->data = 0;
  *buf = buffer;
  *len = s->f_size;
  return 0;
}

gint untgz_write_file(struct untgz_state* s, gchar* altname)
{
  struct utimbuf t;
  union tar_block* b;
  gchar* path;
  gsize remaining;
  int fd;
#if PRESERVE_DIRECTORY_MTIME == 1
  gchar *dpath_tmp = 0, *dpath;
  struct utimbuf dt;
#endif

  if (G_UNLIKELY(s == 0 || s->errstr))
    return -1;
  if (G_UNLIKELY(s->written))
    return 1;
  if (G_UNLIKELY(setjmp(s->errjmp) != 0))
  {
#if PRESERVE_DIRECTORY_MTIME == 1
    g_free(dpath_tmp);
#endif
    return -1;
  }

  if (altname)
    path = altname;
  else
    path = s->f_name;

#if PRESERVE_DIRECTORY_MTIME == 1
  {
    struct stat st;
    dpath_tmp = g_strdup(path);
    dpath = dirname(dpath_tmp);
    if (stat(dpath, &st) == -1)
      throw_error(s, "tgz extraction failed (can't stat parent directory): %s", strerror(errno));
    dt.actime = st.st_atime;
    dt.modtime = st.st_mtime;
  }
#endif

  switch (s->f_type)
  {
    case UNTGZ_REG:
    {
      if (!s->data)
        return 1;
      remaining = s->f_size;
      fd = open(path, O_CREAT|O_TRUNC|O_WRONLY);
      if (fd < 0)
        throw_error(s, "tgz extraction failed (can't write to a file): %s", strerror(errno));
      while (G_LIKELY(remaining > 0))
      {
        b = read_next_block(s, 0);
        if (b == 0)
          throw_error(s, "%d: corrupted tgz archive (missing data block)", s->blockid);
        write(fd, b->b, remaining>BLOCKSIZE?BLOCKSIZE:remaining);
        remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
      }
      close(fd);
      s->data = 0;
      break;
    }
    case UNTGZ_SYM:
      if (symlink(s->f_link, path) == -1)
        throw_error(s, "tgz extraction failed (can't create symlink): %s", strerror(errno));
      break;
    case UNTGZ_LNK:
      if (link(s->f_link, path) == -1)
        throw_error(s, "tgz extraction failed (can't create hardlink): %s", strerror(errno));
      break;
    case UNTGZ_CHR:
      if (mknod(path, S_IFCHR, (dev_t)((((s->f_devmaj & 0xFF) 
            << 8) & 0xFF00) | (s->f_devmin & 0xFF))) == -1)
        throw_error(s, "tgz extraction failed (can't create chrdev): %s", strerror(errno));
      break;
    case UNTGZ_BLK:
      if (mknod(path, S_IFBLK, (dev_t)((((s->f_devmaj & 0xFF) 
            << 8) & 0xFF00) | (s->f_devmin & 0xFF))) == -1)
        throw_error(s, "tgz extraction failed (can't create blkdev): %s", strerror(errno));
      break;
    case UNTGZ_DIR:
      /* because of the way tar stores directories, there 
         is no need to have mkdir_r here */
      if (mkdir(path, 0755) == -1 && errno != EEXIST)
        throw_error(s, "tgz extraction failed (can't create directory): %s", strerror(errno));
      break;
    default:
      throw_error(s, "tgz extraction failed (unknown file type [%d])", s->f_type);
      break;
  }

  if (chown(path, s->f_uid, s->f_gid) == -1)
    throw_error(s, "tgz extraction failed (can't chown file): %s", strerror(errno));
  if (chmod(path, s->f_mode) == -1)
    throw_error(s, "tgz extraction failed (can't chmod file): %s", strerror(errno));
  t.actime = t.modtime = s->f_mtime;
  if (utime(path, &t) == -1)
    throw_error(s, "tgz extraction failed (can't utime file): %s", strerror(errno));

#if PRESERVE_DIRECTORY_MTIME == 1
  if (utime(dpath, &dt) == -1)
    throw_error(s, "tgz extraction failed (can't utime parent directory): %s", strerror(errno));
  g_free(dpath_tmp);
#endif

  s->written = 1;
  return 0;
}

void untgz_close(struct untgz_state* s)
{
  if (s == 0)
    return;

  umask(s->old_umask);
  gzclose(s->gzf);
  g_free(s->f_name);
  g_free(s->f_link);
  g_free(s->tgzfile);
  g_free(s->errstr);
  s->f_name = s->f_link = s->tgzfile = s->errstr = 0;
  s->gzf = 0;
  g_free(s);
}
