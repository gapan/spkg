/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sys.h"

#include "filedb.h"

//#include "tsc.h"

/* private 
 ************************************************************************/

#define PLD_SIZE_LIMIT 64
#define IDX_SIZE_LIMIT 32

struct fdb {
  gboolean is_open;
  gchar* dbdir;
  gchar* errstr;

  struct file_idx* idx; /* pointer to the index array */
  struct file_idx_hdr* ihdr; /* pointer to the index header */
  struct file_pld_hdr* phdr; /* pointer to the payload header */

  guint32 lastid;
  guint32 newoff;

  /* internal mmap and file info */
  void* addr_idx;
  void* addr_pld;
  gint fd_idx;
  gint fd_pld;
  gsize size_pld;
  gsize size_idx;
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

#define MAXHASH 32
static __inline__ guint _hash(const gchar* path)
{
  guint h=0,i=0;
  guint primes[8] = {27137,19171,17313,11131,5771,3737,1313,2317};
  while (*path!=0)
  {
    h += *path * primes[i++&0x07];
    path++;
  }
  return h;
}

static __inline__ gint faststrcmp(const guint32 h1, const gchar* s1, const guint32 h2, const gchar* s2)
{
  if (h1<h2)
    return -1;
  else if (h1>h2)
    return 1;
  else
    return strcmp(s1,s2);
}

/* AVL binary tree
 ************************************************************************/

/* ondisk data structures */

/*XXX: gross hack */
struct file_idx { /* AVL tree leaf */
  guint32 lnk[2]; /* left file in the tree, 0 if none */
  guint32 off; /* offset to the payload file */
  guint32 hash; /* full hash value of the file path */
  gint8   bal; /* balance value */
};

struct file_pld_hdr {
  gchar magic[12]; /* "FDB.PAYLOAD" */
  guint32 newoff; /* offset for the new data */
};

struct file_idx_hdr {
  gchar magic[12]; /* "FDB.PAYLOAD" */
  guint32 lastid;  /* file count, first id is 1 */
  guint32 hashmap[MAXHASH];
};

static guint32 _alloc_node(gchar* path, gchar* link)
{
  guint32 size_path, size_link, pld_entry_size;
  struct file_pld* pld;
  guint32 id;

  size_path = strlen(path);
  size_link = link?strlen(link):0;
  id = _fdb.lastid+1;

  pld_entry_size = sizeof(struct file_pld) + size_path+1 + size_link+1;

#if 0
  /* calculate required sizes of the idx and pld files */
  guint32 required_idx_size, required_pld_size;
  required_idx_size = sizeof(struct file_idx_hdr) + id*sizeof(struct file_idx);
  required_pld_size = _fdb.newoff + pld_entry_size;
  if (G_UNLIKELY(required_pld_size > _fdb.size_pld || required_idx_size > _fdb.size_idx))
    return 0;
#endif

  /* add file to index */
  _fdb.idx[id-1].off = _fdb.newoff;
  _fdb.lastid++;
  /* add file to payload */
  pld = _fdb.addr_pld + _fdb.newoff;
  _fdb.newoff += pld_entry_size;
  pld->rc = 1;
  pld->plen = size_path;
  pld->llen = size_link;
  strcpy(pld->data, path);
  if (G_UNLIKELY(size_link))
    strcpy(pld->data+size_path+1, link);
  return id;
}

/* reused from gnuavl library */
static __inline__ struct file_idx* _node(guint32 id) { return id?_fdb.idx+id-1:NULL; }
static __inline__ struct file_pld* _pld(struct file_idx* idx) { return idx?_fdb.addr_pld+idx->off:0; }
static __inline__ guint32 _id(struct file_idx* idx) { return idx?idx-_fdb.idx+1:0; }

/* returns 1 if inserted, 0 if it already exists */
#define AVL_MAX_HEIGHT 32
static guint32 _ins_node(guint32 root, guint32 hash, gchar* path, gchar* link, void* proot)
{
  struct file_idx *y, *z; /* Top node to update balance factor, and parent. */
  struct file_idx *p, *q; /* Iterator, and parent. */
  struct file_idx *i;     /* Newly inserted node. */
  struct file_idx *w;     /* New root of rebalanced subtree. */
  gint dir;
  guchar da[AVL_MAX_HEIGHT];
  gint k = 0;
  guint32 id;
  
  /*XXX: gross hack */
  z = (struct file_idx *)proot;
  y = _node(root);
  dir = 0;
  for (q=z, p=y; p!=NULL; q=p, p=_node(p->lnk[dir]))
  {
    gint cmp = faststrcmp(hash, path, p->hash, _pld(p)->data);
    if (cmp == 0)
      return _id(p);
    if (p->bal != 0)
      z = q, y = p, k = 0;
    da[k++] = dir = cmp > 0;
  }
  
  /* this will break every pointer from above */
  id = _alloc_node(path,link);
  i = _node(id);
  i->hash = hash;
  q->lnk[dir] = id;

  if (y==0)
    return id;

  for (p=y, k=0; p!=i; p=_node(p->lnk[da[k]]), k++)
    if (da[k] == 0)
      p->bal--;
    else
      p->bal++;

  if (y->bal == -2)
  {
    struct file_idx *x = _node(y->lnk[0]);
    if (x->bal == -1)
    {
      w = x;
      y->lnk[0] = x->lnk[1];
      x->lnk[1] = _id(y);
      x->bal = y->bal = 0;
    }
    else
    {
//      g_assert (x->bal == +1);
      w = _node(x->lnk[1]);
      x->lnk[1] = w->lnk[0];
      w->lnk[0] = _id(x);
      y->lnk[0] = w->lnk[1];
      w->lnk[1] = _id(y);
      if (w->bal == -1)
        x->bal = 0, y->bal = +1;
      else if (w->bal == 0)
        x->bal = y->bal = 0;
      else /* |w->bal == +1| */
        x->bal = -1, y->bal = 0;
      w->bal = 0;
    }
  }
  else if (y->bal == +2)
  {
    struct file_idx *x = _node(y->lnk[1]);
    if (x->bal == +1)
    {
      w = x;
      y->lnk[1] = x->lnk[0];
      x->lnk[0] = _id(y);
      x->bal = y->bal = 0;
    }
    else
    {
//      g_assert(x->bal == -1);
      w = _node(x->lnk[0]);
      x->lnk[0] = w->lnk[1];
      w->lnk[1] = _id(x);
      y->lnk[1] = w->lnk[0];
      w->lnk[0] = _id(y);
      if (w->bal == +1)
        x->bal = 0, y->bal = -1;
      else if (w->bal == 0)
        x->bal = y->bal = 0;
      else /* |w->bal == -1| */
        x->bal = +1, y->bal = 0;
      w->bal = 0;
    }
  }
  else
    return id;

  z->lnk[_id(y) != z->lnk[0]] = _id(w);

  return id;
}

/* public 
 ************************************************************************/

gchar* fdb_error()
{
  return _fdb.errstr;
}

gint fdb_open(const gchar* root)
{
  gchar *path_idx, *path_pld;
  gchar z = 0;
  gint j;

  if (_fdb.is_open)
  {
    _fdb_set_error("can't open filedb: trying to open opened file database");
    goto err_r;
  }

  _fdb.dbdir = g_strdup_printf("%s/%s", root, FILEDB_DIR);

  if (sys_file_type(_fdb.dbdir,0) == SYS_NONE)
    sys_mkdir_p(_fdb.dbdir);

  if (sys_file_type(_fdb.dbdir,0) != SYS_DIR)
  {
    _fdb_set_error("can't open filedb: can't create file database directory: %s", _fdb.dbdir);
    goto err_0;
  }

  /* check if fdb directory is ok */
  if (access(_fdb.dbdir, R_OK|W_OK|X_OK))
  {
    _fdb_set_error("can't open filedb: inaccessible file database directory: %s", strerror(errno));
    goto err_0;
  }

  /* prepare index object and some paths */
  path_idx = g_strdup_printf("%s/idx", _fdb.dbdir);
  path_pld = g_strdup_printf("%s/pld", _fdb.dbdir);

  /* open index and payload files */
  _fdb.fd_pld = open(path_pld, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (_fdb.fd_pld == -1)
  {
    _fdb_set_error("can't open filedb: can't open pld file: %s", strerror(errno));
    goto err_1;
  }

  _fdb.fd_idx = open(path_idx, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (_fdb.fd_idx == -1)
  {
    _fdb_set_error("can't open filedb: can't open idx file: %s", strerror(errno));
    goto err_2;
  }
  
  /* get file sizes */
  _fdb.size_idx = lseek(_fdb.fd_idx, 0, SEEK_END);
  _fdb.size_pld = lseek(_fdb.fd_pld, 0, SEEK_END);
  if (_fdb.size_idx == -1 || _fdb.size_pld == -1)
  {
    _fdb_set_error("can't open filedb: lseek failed: %s", strerror(errno));
    goto err_3;
  }
  lseek(_fdb.fd_idx, 0, SEEK_SET);
  lseek(_fdb.fd_pld, 0, SEEK_SET);
  
  if (_fdb.size_idx == 0)
  { /* empty idx file (create new) */
    struct file_idx_hdr head;
    /*XXX: check writes */
    lseek(_fdb.fd_idx, 1024*1024*IDX_SIZE_LIMIT-1, SEEK_SET);
    write(_fdb.fd_idx, &z, 1);
    _fdb.size_idx = 1024*1024*IDX_SIZE_LIMIT;

    /* create initial header */
    strcpy(head.magic, "FDB.PLINDEX");
    head.lastid = 0;
    for (j=0;j<MAXHASH;j++)
      head.hashmap[j] = 0;

    /* write header */
    lseek(_fdb.fd_idx, 0, SEEK_SET);
    write(_fdb.fd_idx, (void*)&head, sizeof(head));
  }
  else
  {
    struct file_idx_hdr head;
    if (_fdb.size_idx != IDX_SIZE_LIMIT*1024*1024)
    {
      _fdb_set_error("can't open filedb: invalid idx file (its size must be multiple of 4096)");
      goto err_3;
    }

    /* read header */
    read(_fdb.fd_idx, &head, sizeof(head));
    if (strncmp(head.magic, "FDB.PLINDEX", 11) != 0)
    {
      _fdb_set_error("can't open filedb: invalid idx file (wrong magic)");
      goto err_3;
    }

    if (head.lastid*sizeof(struct file_idx)+sizeof(head) > _fdb.size_idx)
    {
      _fdb_set_error("can't open filedb: invalid idx file (corrupted)");
      goto err_3;
    }
  }

  /* read header of the payload file */
  if (_fdb.size_pld == 0)
  {
    struct file_pld_hdr head;

    lseek(_fdb.fd_pld, 1024*1024*PLD_SIZE_LIMIT-1, SEEK_SET);
    write(_fdb.fd_pld, &z, 1);
    _fdb.size_pld = 1024*1024*PLD_SIZE_LIMIT;

    strcpy(head.magic, "FDB.PAYLOAD");
    head.newoff = sizeof(head);
    lseek(_fdb.fd_pld, 0, SEEK_SET);
    write(_fdb.fd_pld, (void*)&head, sizeof(head));
  }
  else
  {
    struct file_pld_hdr head;
    if (_fdb.size_pld != PLD_SIZE_LIMIT*1024*1024)
    {
      _fdb_set_error("can't open filedb: invalid pld file (its size must be multiple of 4096)");
      goto err_3;
    }

    read(_fdb.fd_pld, &head, sizeof(head));
    if (strncmp(head.magic, "FDB.PAYLOAD", 11) != 0)
    {
      _fdb_set_error("can't open filedb: invalid pld file (wrong magic)");
      goto err_3;
    }

    if (head.newoff > _fdb.size_pld)
    {
      _fdb_set_error("can't open filedb: invalid pld file (corrupted)");
      goto err_3;
    }
  }
  
  /* do mmaps of payload and index */
  _fdb.addr_idx = mmap(0, _fdb.size_idx, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, _fdb.fd_idx, 0);
  if (_fdb.addr_idx == (void*)-1)
    goto err_3;
  _fdb.addr_pld = mmap(0, _fdb.size_pld, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, _fdb.fd_pld, 0);
  if (_fdb.addr_pld == (void*)-1)
    goto err_4;
  _fdb.idx = _fdb.addr_idx + sizeof(struct file_idx_hdr);
  _fdb.ihdr = _fdb.addr_idx;
  _fdb.phdr = _fdb.addr_pld;
  _fdb.lastid = _fdb.ihdr->lastid;
  _fdb.newoff = _fdb.phdr->newoff;

  _fdb.is_open = 1;
  g_free(path_idx);
  g_free(path_pld);

  return 0;
 err_5:
  munmap(_fdb.addr_pld, _fdb.size_pld);
 err_4:
  munmap(_fdb.addr_idx, _fdb.size_idx);
 err_3:
  close(_fdb.fd_pld);
 err_2:
  close(_fdb.fd_idx);
 err_1:
  g_free(path_idx);
  g_free(path_pld);
 err_0:
  g_free(_fdb.dbdir);
  _fdb.dbdir = 0;
 err_r:
  return 1;
}

gint fdb_close()
{
  _fdb_open_check(1)

  _fdb.ihdr->lastid = _fdb.lastid;
  _fdb.phdr->newoff = _fdb.newoff;

  msync(_fdb.addr_pld, _fdb.size_pld, MS_ASYNC);
  msync(_fdb.addr_idx, _fdb.size_idx, MS_ASYNC);
  munmap(_fdb.addr_pld, _fdb.size_pld);
  munmap(_fdb.addr_idx, _fdb.size_idx);

  g_free(_fdb.dbdir);
  memset(&_fdb, 0, sizeof(_fdb));
  return 0;
}

guint32 fdb_add_file(gchar* path, gchar* link)
{
  guint hash = _hash(path);
  guint32 id;
  id = _ins_node(_fdb.ihdr->hashmap[hash%MAXHASH], hash, path, link, &_fdb.ihdr->hashmap[hash%MAXHASH]);
  return id;
}

guint32 fdb_get_file_id(gchar* path)
{
  struct file_idx *p;
  guint32 hash = _hash(path);
  guint32 root = _fdb.ihdr->hashmap[hash%MAXHASH];

  if (root == 0)
    return 0;
  for (p=_node(root); p!=NULL; )
  {
    gint cmp = faststrcmp(hash, path, p->hash, _pld(p)->data);
    if (cmp < 0)
      p = _node(p->lnk[0]);
    else if (cmp > 0)
      p = _node(p->lnk[1]);
    else
      return _id(p);
  }
  return 0;
}

struct file_pld* fdb_get_file(guint32 id)
{
  return _pld(_node(id));
}
