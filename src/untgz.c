/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <utime.h>
#include <fcntl.h>
#include <setjmp.h>
#include <zlib.h>

#include "untgz.h"
#include "sys.h"

/* Enable or disable parent directory modification and access times 
   preservation. */
#define UNTGZ_PRESERVE_DIR_TIMES 0

#if UNTGZ_PRESERVE_DIR_TIMES == 1
#include <libgen.h>
#endif

#include "bench.h"

/* optimal (according to the benchmark), must be multiple of 512 */
#define BLOCKBUFSIZE (512*100)
#define WRITEBUFSIZE (1024*16)

enum {
  COMPTYPE_GZIP,
  COMPTYPE_LZMA,
  COMPTYPE_NONE
};

struct untgz_state_internal {
  /* error handling */
  jmp_buf errjmp;
  struct error* err;
  mode_t old_umask; /* saved umask */

  gboolean errblock; /* if set all operation is bloked and error is returned */
  gboolean data;/* data from the current file were not read, yet */
  gboolean written;/* current file was written to disk (or buffer) */
  gboolean eof;/* end of archive reached */

  gint comptype;
  gzFile* gzf; /* gzio tar stream */
  FILE* fp;    /* file stream */
  
  /* internal block buffer data */
  gchar bbuf[BLOCKBUFSIZE]; /* block buffer */
  gchar* bend; /* points to the end of the buffer */
  gchar* bpos; /* points to the current block */
  gint blockid; /* block id (512*blockid == position of the block in the input file) */

  gchar wbuf[WRITEBUFSIZE]; /* write buffer */
};

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

#define e_set(n, fmt, args...) e_add(s->i->err, "untgz", __func__, n, fmt, ##args)
#define e_jump() longjmp(s->i->errjmp, 1)

#define _e_set(e, n, fmt, args...) e_add(e, "untgz", __func__, n, fmt, ##args)

#define e_throw(n, fmt, args...) \
  G_STMT_START { \
  e_set(n, fmt, ##args); \
  e_jump(); \
  } G_STMT_END

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
      e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (bad oct - possibly trying to checksum data block)", s->i->blockid);
    else
      r = r*8+c;
  }
  return r;
}

/* header checksum validation */
static void validate_header_csum(struct untgz_state* s)
{
  struct untgz_state_internal* i = s->i;
  guint n, head_csum, real_csum=0;
  union tar_block* b = (union tar_block*)i->bpos;

  head_csum = getoct(s, b->h.chksum, 6);
  memcpy(b->h.chksum, "        ", 8);
  for (n=0; G_LIKELY(n<BLOCKSIZE); n+=2)
    real_csum += (guchar)b->b[n] + (guchar)b->b[n+1];
  if (G_UNLIKELY(real_csum != head_csum))
    e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (invalid header checksum)", i->blockid);
}

/* this function returns pointer to the next tar block
 * return value is ptr if pointer points to the next block
 *                 0 if eof block occured (header with first byte zero)
 *                 longjmp on read error (or incomplete block)
 */
static union tar_block* read_next_block(struct untgz_state* s, gboolean is_header)
{
  struct untgz_state_internal* i = s->i;
  i->bpos += BLOCKSIZE;
  if (i->bpos >= i->bend)
  { /* reload */
    continue_timer(6);
    gint read;

    if (i->comptype == COMPTYPE_GZIP)
      read = gzread(i->gzf, i->bbuf, BLOCKBUFSIZE);
    else
      read = fread(i->bbuf, 1, BLOCKBUFSIZE, i->fp);

    stop_timer(6);
    if (read < BLOCKSIZE)
    {
      gint err;
      if (i->comptype == COMPTYPE_GZIP)
        err = (read == 0 && !gzeof(i->gzf));
      else
        err = (read == 0 && !feof(i->fp));
      if (err)
        e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (gzread failed)", i->blockid);
      else
        e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (early EOF)", i->blockid);
    }
    i->bpos = i->bbuf;
    i->bend = i->bbuf + read;
  }
  if (is_header)
  {
    if (G_UNLIKELY(i->bpos[0] == 0))
      return 0;
    continue_timer(8);
    validate_header_csum(s);
    stop_timer(8);
  }
  i->blockid++;
  return (union tar_block*)i->bpos;
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

struct untgz_state* untgz_open(const gchar* tgzfile, struct error* e)
{
  struct untgz_state* s=0;
  gzFile *gzf;
  FILE* fp;
  struct stat st;
  gint comptype;

  continue_timer(0);
  continue_timer(1);

  g_assert(tgzfile != 0);
  g_assert(e != 0);

  if (stat(tgzfile, &st) == -1)
  {
    _e_set(e, E_ERROR, "can't stat file: %s", tgzfile);
    return NULL;
  }

  if (g_str_has_suffix(tgzfile, ".tgz"))
    comptype = COMPTYPE_GZIP;
  else if (g_str_has_suffix(tgzfile, ".tlz"))
    comptype = COMPTYPE_LZMA;
  else if (g_str_has_suffix(tgzfile, ".tar"))
    comptype = COMPTYPE_NONE;
  else
  {
    _e_set(e, E_ERROR, "unknown package type: %s", tgzfile);
    return NULL;
  }

  if (comptype == COMPTYPE_GZIP)
  {
    gzf = gzopen(tgzfile, "rb");
    if (gzf == NULL)
    {
      _e_set(e, E_ERROR, "can't gzopen file: %s", tgzfile);
      return NULL;
    }
  }
  else if (comptype == COMPTYPE_LZMA)
  {
    gchar* escaped = g_shell_quote(tgzfile);
    gchar* cmd = g_strdup_printf("lzma -d -c %s", escaped);
    fp = popen(cmd, "r");
    g_free(escaped);
    if (fp == NULL)
    {
      _e_set(e, E_ERROR, "can't popen command: %s", cmd);
      g_free(cmd);
      return NULL;
    }
    g_free(cmd);
  }  
  else if (comptype == COMPTYPE_NONE)
  {
    fp = fopen(tgzfile, "r");
    if (fp == NULL)
    {
      _e_set(e, E_ERROR, "can't open file: %s", tgzfile);
      return NULL;
    }
  }

  s = g_new0(struct untgz_state,1);
  s->i = g_new0(struct untgz_state_internal,1);
  struct untgz_state_internal* i = s->i;
  s->tgzfile = g_strdup(tgzfile);
  i->gzf = gzf;
  i->fp = fp;
  i->comptype = comptype;
  s->csize = st.st_size;
  i->old_umask = umask(0);
  i->blockid = -1;
  i->err = e;

  stop_timer(1);
  return s;
}

void untgz_close(struct untgz_state* s)
{
  g_assert(s != 0);
  continue_timer(2);

  struct untgz_state_internal* i = s->i;

  if (i->comptype == COMPTYPE_GZIP)
    gzclose(i->gzf);
  else if (i->comptype == COMPTYPE_LZMA)
    pclose(i->fp);
  else
    fclose(i->fp);

  umask(i->old_umask);
  g_free(s->f_name);
  g_free(s->f_link);
  g_free(s->tgzfile);
  s->f_name = s->f_link = s->tgzfile = 0;
  i->gzf = 0;
  g_free(i);
  s->i = 0;
  g_free(s);

  stop_timer(2);
  stop_timer(0);

  print_timer(0, "[untgz] extraction");
  print_timer(1, "[untgz] untgz_open");
  print_timer(2, "[untgz] untgz_close");
  print_timer(3, "[untgz] untgz_get_header");
  print_timer(4, "[untgz] untgz_write_file");
  print_timer(5, "[untgz] untgz_write_data");

  print_timer(6, "[untgz] gzread");
  print_timer(7, "[untgz] fwrite");
  print_timer(8, "[untgz] chksum");
}

gint untgz_get_header(struct untgz_state* s)
{
  off_t remaining;
  union tar_block* b;
  g_assert(s != 0);

  continue_timer(3);
  struct untgz_state_internal* i = s->i;

  if (G_UNLIKELY(i->errblock))
  {
    e_set(E_ERROR|UNTGZ_BLOCKED, "[block:%d] untgz is blocked", i->blockid);
    goto err_0;
  }
  else if (G_UNLIKELY(i->eof))
    goto ret_1;
  if (G_UNLIKELY(setjmp(i->errjmp) != 0))
  {
    e_set(E_PASS, "error thrown");
    goto err_0;
  }
  
  /* skip data blocks if write_data or write_file was not called after previous get_header */
  if (s->f_type == UNTGZ_REG && i->data)
  {
    remaining = s->f_size;
    while (G_LIKELY(remaining > 0))
    {
      if (read_next_block(s, 0) == 0)
        e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (missing data block)", i->blockid);
      remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
    }
  }

  /* reset state */
  g_free(s->f_name);
  g_free(s->f_link);
  s->f_name = s->f_link = 0;
  s->f_type = UNTGZ_NONE;
  i->data = 0;
  i->written = 0;

  while (1)
  {
    /* read next header */
    b = read_next_block(s, 1);
    if (b == 0)
    {
      /* empty header => end of archive */
      i->eof = 1;
      return 1;
    }

    remaining = getoct(s, b->h.size, sizeof(b->h.size));
    /* read longname and/or longlink */
    if (b->h.typeflag == GNUTYPE_LONGLINK)
    {
      while (G_LIKELY(remaining > 0))
      {
        b = read_next_block(s, 0);
        if (b == 0)
          e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (missing longlink data block)", i->blockid);
        s->f_link = strnappend(s->f_link, (gchar*)b->b, BLOCKSIZE);
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
          e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (missing longname data block)", i->blockid);
        s->f_name = strnappend(s->f_name, (gchar*)b->b, BLOCKSIZE);
        remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
      }
      continue;
    }
    break;
  }

  /* parse header */
  s->f_size = remaining;
  s->f_mode = getoct(s, b->h.mode, sizeof(b->h.mode)) & 07777;
  s->f_uid = getoct(s, b->h.uid, sizeof(b->h.uid));
  s->f_gid = getoct(s, b->h.gid, sizeof(b->h.gid));
  s->f_mtime = (time_t) getoct(s, b->h.mtime, sizeof(b->h.mtime));
  s->f_devmaj = getoct(s, b->h.devmajor, sizeof(b->h.devmajor));
  s->f_devmin = getoct(s, b->h.devminor, sizeof(b->h.devminor));
  strncpy(s->f_uname, b->h.uname, sizeof(b->h.uname));
  strncpy(s->f_gname, b->h.gname, sizeof(b->h.gname));

  switch (b->h.typeflag)
  {
    case AREGTYPE: 
    case REGTYPE: s->f_type = UNTGZ_REG; i->data = 1; s->usize += remaining; break;
    case DIRTYPE: s->f_type = UNTGZ_DIR; break;
    case SYMTYPE: s->f_type = UNTGZ_SYM; break;
    case LNKTYPE: s->f_type = UNTGZ_LNK; break;
    case CHRTYPE: s->f_type = UNTGZ_CHR; break;
    case BLKTYPE: s->f_type = UNTGZ_BLK; break;
    case FIFOTYPE: s->f_type = UNTGZ_FIFO; break;
    default: e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] \"corrupted\" tgz archive (unimplemented typeflag [%c])", i->blockid, b->h.typeflag);
  }

  if (s->f_name == 0)
    s->f_name = g_strndup(b->h.name, SHORTNAMESIZE);
  /* just one more (unnecessary) check */
  else if (strncmp(s->f_name, b->h.name, SHORTNAMESIZE-1)) /* -1 because it's zero terminated */
    e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (longname mismatch)", i->blockid);
  if (s->f_link == 0)
    s->f_link = g_strndup(b->h.linkname, SHORTNAMESIZE);
  /* just one more (unnecessary) check */
  else if (strncmp(s->f_link, b->h.linkname, SHORTNAMESIZE-1)) /* -1 because it's zero terminated */
    e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (longlink mismatch)", i->blockid);

  stop_timer(3);
  return 0;
 err_0:
  stop_timer(3);
  return -1;
 ret_1:
  stop_timer(3);
  return 1;
}

gint untgz_write_data(struct untgz_state* s, gchar** buf, gsize* len)
{
  gchar* buffer=0;  
  union tar_block* b;
  gsize position=0;
  gsize remaining;

  g_assert(s != 0);
  g_assert(buf != 0);
  g_assert(len != 0);

  continue_timer(5);

  struct untgz_state_internal* i = s->i;

  if (G_UNLIKELY(i->errblock))
  {
    e_set(E_ERROR|UNTGZ_BLOCKED, "[block:%d] untgz is blocked", i->blockid);
    goto err_0;
  }
  else if (i->eof || s->f_type != UNTGZ_REG || i->data == 0) /* no data for current file */
    goto ret_1;
  if (setjmp(i->errjmp) != 0)
  {
    g_free(buffer);
    e_set(E_PASS, "error thrown");
    goto err_0;
  }

  remaining=s->f_size;
  buffer = g_malloc(remaining+1);
  while (G_LIKELY(remaining > 0))
  {
    b = read_next_block(s, 0);
    if (b == 0)
      e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (missing data block)", i->blockid);
    memcpy(buffer+position, b, remaining>BLOCKSIZE?BLOCKSIZE:remaining);
    remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
    position += BLOCKSIZE;
  }
  i->data = 0;
  buffer[s->f_size] = 0; /* zero terminate buffer */
  *buf = buffer;
  *len = s->f_size;

  stop_timer(5);
  return 0;
 err_0:
  stop_timer(5);
  return -1;
 ret_1:
  stop_timer(5);
  return 1;
}

gint untgz_write_file(struct untgz_state* s, gchar* altname)
{
  struct utimbuf t;
  union tar_block* b;
  gchar* path;
  gsize remaining;
#if UNTGZ_PRESERVE_DIR_TIMES == 1
  gchar *dpath_tmp = 0, *dpath;
  struct utimbuf dt;
#endif

  g_assert(s != 0);
  struct untgz_state_internal* i = s->i;
  continue_timer(4);

  if (G_UNLIKELY(i->errblock))
  {
    e_set(E_ERROR|UNTGZ_BLOCKED, "[block:%d] untgz is blocked", i->blockid);
    goto err_0;
  }
  else if (G_UNLIKELY(i->written || i->eof))
    goto ret_1;
  if (G_UNLIKELY(setjmp(i->errjmp) != 0))
  {
#if UNTGZ_PRESERVE_DIR_TIMES == 1
    g_free(dpath_tmp);
#endif
    e_set(E_PASS, "error thrown");
    goto err_0;
  }

  if (altname)
    path = altname;
  else
    path = s->f_name;

#if UNTGZ_PRESERVE_DIR_TIMES == 1
  {
    struct stat st;
    dpath_tmp = g_strdup(path);
    dpath = dirname(dpath_tmp);
    if (stat(dpath, &st) == -1)
      e_throw(E_ERROR, "can't stat parent directory: %s", strerror(errno));
    dt.actime = st.st_atime;
    dt.modtime = st.st_mtime;
  }
#endif

  switch (s->f_type)
  {
    case UNTGZ_REG:
    {
      if (!i->data)
        goto ret_1;
      remaining = s->f_size;
      FILE* f = fopen(path, "w");
      if (f == 0)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't open file for writing: %s", strerror(errno));
      if (setvbuf(f, i->wbuf, _IOFBF, WRITEBUFSIZE))
        e_throw(E_ERROR|UNTGZ_BADIO, "can't setup write buffer");
      while (G_LIKELY(remaining > 0))
      {
        b = read_next_block(s, 0);
        if (b == 0)
          e_throw(E_ERROR|UNTGZ_CORRUPT, "[block:%d] corrupted tgz archive (missing data block)", i->blockid);
        continue_timer(7);
        if (fwrite(b->b, remaining>BLOCKSIZE?BLOCKSIZE:remaining, 1, f) != 1)
          e_throw(E_ERROR|UNTGZ_BADIO, "can't write to a file: %s", strerror(errno));
        stop_timer(7);
        remaining = remaining<=BLOCKSIZE?0:remaining-BLOCKSIZE;
      }
      fclose(f);
      i->data = 0;
      break;
    }
    case UNTGZ_SYM:
      if (symlink(s->f_link, path) == -1)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't create symlink: %s", strerror(errno));
      break;
    case UNTGZ_FIFO:
      if (mkfifo(path, s->f_mode) == -1)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't create fifo: %s", strerror(errno));
      break;
    case UNTGZ_LNK:
      printf("** ln %s %s", s->f_link, path);
      if (link(s->f_link, path) == -1)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't create hardlink: %s", strerror(errno));
      break;
    case UNTGZ_CHR:
      if (mknod(path, S_IFCHR, (dev_t)((((s->f_devmaj & 0xFF) 
            << 8) & 0xFF00) | (s->f_devmin & 0xFF))) == -1)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't create chrdev: %s", strerror(errno));
      break;
    case UNTGZ_BLK:
      if (mknod(path, S_IFBLK, (dev_t)((((s->f_devmaj & 0xFF) 
            << 8) & 0xFF00) | (s->f_devmin & 0xFF))) == -1)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't create blkdev: %s", strerror(errno));
      break;
    case UNTGZ_DIR:
      /* because of the way tar stores directories, there 
         is no need to have mkdir_r here */
      if (mkdir(path, 0700) == -1 && errno != EEXIST)
        e_throw(E_ERROR|UNTGZ_BADIO, "can't create directory: %s", strerror(errno));
      break;
    default:
      e_throw(E_ERROR, "unknown file type [%d]", s->f_type);
      break;
  }

  if (chown(path, s->f_uid, s->f_gid) == -1)
    e_throw(E_ERROR|UNTGZ_BADMETA, "can't chown file: %s", strerror(errno));
  if (chmod(path, s->f_mode) == -1)
    e_throw(E_ERROR|UNTGZ_BADMETA, "can't chmod file: %s", strerror(errno));
  t.actime = t.modtime = s->f_mtime;
  if (utime(path, &t) == -1)
    e_throw(E_ERROR|UNTGZ_BADMETA, "can't utime file: %s", strerror(errno));

#if UNTGZ_PRESERVE_DIR_TIMES == 1
  if (utime(dpath, &dt) == -1)
    e_throw(E_ERROR|UNTGZ_BADMETA, "can't utime parent directory: %s", strerror(errno));
  g_free(dpath_tmp);
#endif

  i->written = 1;
  stop_timer(4);
  return 0;
 err_0:
  stop_timer(4);
  return -1;
 ret_1:
  stop_timer(4);
  return 1;
}
