/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sys.h"

#include "filedb.h"

/* private 
 ************************************************************************/

struct file_pld {
  guint32 flen;
  guint32 llen;
//  guint32 mode;
  gchar data[];
  /* data[0] = 'path' + '\0' */
  /* data[flen+1] = 'link' + '\0' */
};

struct file_idx {
  guint32 lid;
  guint32 rid;
  guint32 off;
};

struct file_index {
  guint32 hash;
  void* addr_idx;
  void* addr_pld;
  gint fd_idx;
  gint fd_pld;
  struct file_idx* idx;
  gsize len_idx;
  gsize size_pld;
  gsize size_idx;
};

struct fdb {
  gboolean is_open;
  gchar* dbdir;
  gchar* errstr;
};

static struct fdb _fdb = {0};

static __inline__ void _fdb_reset_error()
{
  if (G_UNLIKELY(_fdb.errstr != 0))
  {
    g_free(_fdb.errstr);
    _fdb.errstr = 0;
  }
}

#define _fdb_open_check(v) \
  if (!_fdb.is_open) \
  { \
    _fdb_set_error("trying to access closed file database"); \
    return v; \
  }

static void _fdb_set_error(const gchar* fmt, ...)
{
  va_list ap;
  _fdb_reset_error();
  va_start(ap, fmt);
  _fdb.errstr = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  _fdb.errstr = g_strdup_printf("error[filedb]: %s", _fdb.errstr);
}

#define MAXHASH 512
static __inline__ guint _fdb_hash(const gchar* path)
{
  guint h=0,i=0;
  gint primes[8] = {3, 5, 7, 11, 13, 17, 7/*19*/, 5/*23*/};
  while (*path!=0)
  {
    h += *path * primes[i&0x07];
    path++; i++;
  }
  return h%MAXHASH;
}

/* file index balanced binary tree
 ************************************************************************/

struct file_index* _idx_open(guint32 hash)
{
  struct file_index* i;
  gchar *path_idx, *path_pld;
  struct file_idx header = {.lid = 0x10feef01, .rid = 0x00000001, .off = -1};
  gint j;
  
  /* prepare index object and some paths */
  i = g_new0(struct file_index,1);
  i->hash = hash;
  path_idx = g_strdup_printf("%s/idx.%03x", _fdb.dbdir, hash);
  path_pld = g_strdup_printf("%s/pld.%03x", _fdb.dbdir, hash);

  /* open index and payload files */
  i->fd_pld = open(path_pld, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (i->fd_pld == -1)
    goto err_1;

  i->fd_idx = open(path_idx, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (i->fd_idx == -1)
    goto err_2;
  
  /* get file sizes */
  i->size_idx = lseek(i->fd_idx, 0, SEEK_END);
  i->size_pld = lseek(i->fd_pld, 0, SEEK_END);
  if (i->size_idx == -1 || i->size_pld == -1)
    goto err_3;
  lseek(i->fd_idx, 0, SEEK_SET);
  lseek(i->fd_pld, 0, SEEK_SET);
  
  /* check size and header of index file */
  if (i->size_idx == 0)
  {
    /* XXX: check writes */
    gchar zbuf[sizeof(header)] = {0};
    write(i->fd_idx, (void*)&header, sizeof(header));
    for (j=0; j<(1024+1); j++)
      write(i->fd_idx, zbuf, sizeof(header));
    i->size_idx = (1024+1)*sizeof(header);
  }
  else
  {
    struct file_idx header_read;
    if ((i->size_idx % sizeof(header)) != 0)
      goto err_3;
    i->len_idx = i->size_idx/sizeof(header)-1;
    lseek(i->fd_idx, 0, SEEK_SET);
    read(i->fd_idx, &header_read, sizeof(header));
    if (memcmp(&header, &header_read, sizeof(header)) != 0)
      goto err_3;
  }
  
  /* do mmaps of payload and index */
  i->addr_idx = mmap(0, i->size_idx, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_LOCKED, i->fd_idx, 0);
  if (i->addr_idx == (void*)-1)
    goto err_3;
  i->addr_pld = mmap(0, i->size_pld, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_LOCKED, i->fd_pld, 0);
  if (i->addr_pld == (void*)-1)
    goto err_4;
  i->idx = i->addr_idx+sizeof(header);

  g_free(path_idx);
  g_free(path_pld);
  return i;
 err_5:
  munmap(i->addr_pld, i->size_pld);
 err_4:
  munmap(i->addr_idx, i->size_idx);
 err_3:
  close(i->fd_pld);
 err_2:
  close(i->fd_idx);
 err_1:
  g_free(i);
  g_free(path_idx);
  g_free(path_pld);
 err_0:
  return 0;
}

void _idx_close(struct file_index* i, guint32 wr)
{
  if (i == 0)
    return;

  if (wr)
  {
    msync(i->addr_pld, i->size_pld, MS_SYNC);
    msync(i->addr_idx, i->size_idx, MS_SYNC);
  }

  munmap(i->addr_pld, i->size_pld);
  munmap(i->addr_idx, i->size_idx);
  memset(i, 0, sizeof(*i));
  g_free(i);
}

#define INDEX_CHUNK_SIZE 1024
gint _idx_new_node(struct file_index* i)
{
  gint id;
  /* check if we need to expand index file */
  if ((i->len_idx+2)*sizeof(struct file_idx) > i->size_idx)
  {
    gchar zbuf[sizeof(struct file_idx)] = {0};
    gsize new_size;
    gint j;
    
    lseek(i->fd_idx, 0, SEEK_END);
    for (j=0; j<INDEX_CHUNK_SIZE; j++)
      write(i->fd_idx, zbuf, sizeof(struct file_idx));
    new_size = i->size_idx + INDEX_CHUNK_SIZE*sizeof(struct file_idx);
    i->addr_idx = mremap(i->addr_idx, i->size_idx, new_size, MREMAP_MAYMOVE); /* XXX: check failure */
    i->size_idx = new_size;
  }
  id = i->len_idx++;
  return id;
}

gint _idx_ins_node(struct file_index* i, gint n)
{
}

/* public 
 ************************************************************************/

gchar* fdb_error()
{
  return _fdb.errstr;
}

gint fdb_open(const gchar* root)
{
  if (_fdb.is_open)
  {
    _fdb_set_error("trying to open opened file database");
    goto err_0;
  }

  _fdb.dbdir = g_strdup_printf("%s/%s", root, FILEDB_DIR);

  if (sys_file_type(_fdb.dbdir,0) == SYS_NONE)
    sys_mkdir_p(_fdb.dbdir);

  if (sys_file_type(_fdb.dbdir,0) != SYS_DIR)
  {
    _fdb_set_error("can't create file database directory: %s", _fdb.dbdir);
    goto err_1;
  }

  /* check if fdb directory is ok */
  if (access(_fdb.dbdir, R_OK|W_OK|X_OK))
  {
    _fdb_set_error("inaccessible file database directory: %s", strerror(errno));
    goto err_1;
  }

  _fdb.is_open = 1;

  return 0;
 err_1:
  g_free(_fdb.dbdir);
  _fdb.dbdir = 0;
 err_0:
  return 1;
}

gint fdb_close()
{
  _fdb_open_check(1)
  g_free(_fdb.dbdir);
  memset(&_fdb, 0, sizeof(_fdb));
  return 0;
}

gint fdb_add_files(GSList* files, gint grpid)
{
  GSList* l;
  for (l=files; l!=0; l=l->next)
  {
    struct db_file* f = l->data;
    
  }
}

GSList* fdb_get_files(gint grpid)
{
}

GSList* fdb_rem_files(gint grpid)
{
}

#if 0
struct fdb_file* fdb_alloc_file(gchar* path, gchar* link)
{
  struct db_file* f;
  f = g_new0(struct db_file, 1);
  f->path = path;
  f->link = link;
  return f;
}
#endif

int main()
{
  gint fd,j;
  struct file_index* i;
  
  if (fdb_open("."))
  {
    printf("%s\n", fdb_error());
    exit(1);
  }
  i = _idx_open(55);
  
  _idx_close(i,1);
  for (j=0; j<80000; j++)
    _idx_new_node(i);
  fdb_close();

  return 0;
}
