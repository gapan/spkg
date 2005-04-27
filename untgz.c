/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/*
#include <errno.h>
*/
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "untgz.h"

/****************
 * internal types
 */

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
#define GNUTYPE_DUMPDIR  'D'    /* file names from dumped directory */
#define GNUTYPE_MULTIVOL 'M'    /* continuation of file from another volume */
#define GNUTYPE_NAMES    'N'    /* file name that does not fit into main hdr */
#define GNUTYPE_SPARSE   'S'    /* sparse file */
#define GNUTYPE_VOLHDR   'V'    /* tape/volume header */

#define BLOCKSIZE 512
#define SHORTNAMESIZE 100

struct tar_header {             /* byte offset */
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

/********************
 * internal functions
 */

static void throw_error(struct untgz_state* s, gchar* text)
{
  s->errstr = text;
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
      throw_error(s, "invalid octal number");
    else
      r = r*8+c;
  }
  return r;
}

/* header checksum validation */
static void validate_header_csum(struct untgz_state* s, union tar_block* b)
{
  guint i, head_csum, real_csum=0;

  head_csum = getoct(s, b->h.chksum, 6);
  memcpy(b->h.chksum, "        ", sizeof(b->h.chksum));
  for (i=0; G_LIKELY(i<BLOCKSIZE); i++)
    real_csum += b->b[i];
  if (G_UNLIKELY(real_csum != head_csum))
    throw_error(s, "header is corrupted (invalid checksum)");
}

static gint read_next_block(struct untgz_state* s, union tar_block* b, 
                            gboolean is_header)
{
  gsize len = gzread(s->gzf, b->b, BLOCKSIZE);
  if (G_LIKELY(len == BLOCKSIZE))
  {
    if (G_UNLIKELY(is_header))
    {
      if (G_UNLIKELY(b->b[0] == 0))
        return 1;
      validate_header_csum(s, b);
    }
    return 0;
  }
  else if (G_UNLIKELY(len == 0))
    return 1;
  else if (G_UNLIKELY(len < 0))
    throw_error(s, "gzread failed with error");
  throw_error(s, "invalid blocksize (early EOF)");
  return 0;
}

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

/*********************
 * untgz API functions
 */

struct untgz_state* untgz_open(gchar* tgzfile)
{
  struct untgz_state *s;
  gzFile *gzf;
  struct stat st;
  
  if (tgzfile == NULL)
    return NULL;
  gzf = gzopen(tgzfile, "rb");
  if (gzf == NULL)
    return NULL;

  s = g_new0(struct untgz_state,1);
  if (stat(tgzfile, &st) == 0)
    s->csize = st.st_size;
  s->gzf = gzf;
  s->tgzfile = g_strdup(tgzfile);
  return s;
}

gint untgz_get_header(struct untgz_state* s)
{
  gsize size;
  union tar_block b;

  if (s == NULL || s->errstr)
    return -1;
  if (s->eof)
    return 1;
  if (setjmp(s->errjmp) != 0)
    return -1;

  /* skip data blocks */
  if (s->f_type == FT_REG && s->data)
  {
    size = s->f_size;
    while (G_LIKELY(size > 0))
    {
      if (read_next_block(s, &b, 0))
        throw_error(s, "early EOF (missing file data block)");
      size = size<=BLOCKSIZE?0:size-BLOCKSIZE;
    }
//    throw_error(s, "expected data block read is missing");
  }

  /* reset state */
  g_free(s->f_name);
  g_free(s->f_link);
  s->f_name = s->f_link = NULL;
  s->f_type = FT_NONE;
  s->data = 0;

  while (1)
  {
    /* read next header */
    if (read_next_block(s, &b, 1))
    {
      /* no more blocks available */
      s->eof = 1;
      return 1;
    }

    size = getoct(s, b.h.size, 12);
    /* read extended filename and linktarget */
    if (b.h.typeflag == GNUTYPE_LONGLINK)
    {
      while (G_LIKELY(size > 0))
      {
        if (read_next_block(s, &b, 0))
          throw_error(s, "early EOF (missing link target block)");
        s->f_link = strnappend(s->f_link, b.b, BLOCKSIZE);
        size = size<=BLOCKSIZE?0:size-BLOCKSIZE;
      }
      continue;
    }
    else if (b.h.typeflag == GNUTYPE_LONGNAME)
    {
      while (G_LIKELY(size > 0))
      {
        if (read_next_block(s, &b, 0))
          throw_error(s, "early EOF (missing file name block)");
        s->f_name = strnappend(s->f_name, b.b, BLOCKSIZE);
        size = size<=BLOCKSIZE?0:size-BLOCKSIZE;
      }
      continue;
    }
    break;
  }

  /* parse header */
  s->f_size = size;
  s->f_mode = getoct(s, b.h.mode, 8);
  s->f_uid  = getoct(s, b.h.uid, 8);
  s->f_gid  = getoct(s, b.h.gid, 8);
  s->f_mtime = (time_t) getoct(s, b.h.mtime, 12);
  s->f_devmaj = getoct(s, b.h.devmajor, 8);
  s->f_devmin = getoct(s, b.h.devminor, 8);
  strncpy(s->f_uname, b.h.uname, 32);
  strncpy(s->f_gname, b.h.gname, 32);

  switch (b.h.typeflag)
  {
    case AREGTYPE: 
    case REGTYPE: s->f_type = FT_REG; s->data = 1; s->usize += size; break;
    case SYMTYPE: s->f_type = FT_LNK; break;
    case CHRTYPE: s->f_type = FT_CHR; break;
    case BLKTYPE: s->f_type = FT_BLK; break;
    case DIRTYPE: s->f_type = FT_DIR; break;
    default: throw_error(s, "unknown typeflag");
  }

  /* just one more check */
  if (s->f_name == 0)
    s->f_name = g_strndup(b.h.name, SHORTNAMESIZE);
  else if (strncmp(s->f_name, b.h.name, SHORTNAMESIZE))
    throw_error(s, "extended header mismatch");
  if (s->f_link == 0)
    s->f_link = g_strndup(b.h.linkname, SHORTNAMESIZE);
  else if (strncmp(s->f_link, b.h.linkname, SHORTNAMESIZE))
    throw_error(s, "extended header mismatch");

  return 0;
}

gint untgz_write_data(struct untgz_state* s, guchar** buf, gsize* len)
{
  guchar* buf_tmp=0;  

  if (s == NULL || s->errstr)
    return -1;
  if (s->eof)
    return 1;
  if (setjmp(s->errjmp) != 0)
  {
    g_free(buf_tmp);
    return -1;
  }

  if (s->f_type == FT_REG && s->data)
  {
    gsize pos=0;
    buf_tmp = g_malloc(s->f_size);
    while (G_LIKELY(s->f_size > 0))
    {
      if (read_next_block(s, (union tar_block*)buf_tmp+pos*BLOCKSIZE, 0))
        throw_error(s, "early EOF (missing file data block)");
    }
    s->data = 0;
    *buf = buf_tmp;
    *len = s->f_size;
    return 0;
  }
  return 1;
}

gint untgz_write_file(struct untgz_state* s, gchar* altname)
{
  struct utimbuf t;
  union tar_block b;
  gchar* path;
  FILE* f;

  if (s == NULL || s->errstr)
    return -1;
  if (s->eof)
    return 1;
  if (setjmp(s->errjmp) != 0)
  {
    return -1;
  }
  
  if (altname)
    path = altname;
  else
    path = s->f_name;

  switch (s->f_type)
  {
    case FT_REG:
      if (!s->data)
        return 1;
      gsize size = s->f_size;
      f = fopen(path, "w");
      if (f == 0)
        throw_error(s, "can't open file for writing");
      while (G_LIKELY(size > 0))
      {
        if (read_next_block(s, &b, 0))
          throw_error(s, "early EOF (missing file data block)");
        fwrite(b.b, size>BLOCKSIZE?BLOCKSIZE:size, 1, f);
        size = size<=BLOCKSIZE?0:size-BLOCKSIZE;
      }
      fclose(f);
      s->data = 0;
      break;
    case FT_LNK:
      symlink(s->f_link, path);
      break;
    case FT_CHR:
      mknod(path, S_IFCHR, (dev_t)((((s->f_devmaj & 0xFF) 
            << 8) & 0xFF00) | (s->f_devmin & 0xFF)));
      break;
    case FT_BLK:
      mknod(path, S_IFBLK, (dev_t)((((s->f_devmaj & 0xFF) 
            << 8) & 0xFF00) | (s->f_devmin & 0xFF)));
      break;
    case FT_DIR:
      /* because of the way tar stores directories, there 
         is no need to have mkdir_r here */
      mkdir(path, 0755); 
      break;
    default:
      break;
  }

  chown(path, s->f_uid, s->f_gid);
  chmod(path, s->f_mode);
  t.actime = t.modtime = s->f_mtime;
  utime(path, &t);
  return 1;
}

void untgz_close(struct untgz_state* s)
{
  if (s == NULL)
    return;

  gzclose(s->gzf);
  g_free(s->f_name);
  g_free(s->f_link);
  g_free(s->tgzfile);
  s->f_name = s->f_link = s->tgzfile = NULL;
  g_free(s);
}
