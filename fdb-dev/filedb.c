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

/* private 
 ************************************************************************/

#if 1
#define PLD_SIZE_LIMIT 64
#define IDX_SIZE_LIMIT 32
#else
#define PLD_SIZE_LIMIT 1
#define IDX_SIZE_LIMIT 1
#endif

struct fdb {
  gboolean is_open;
  gchar* dbdir;
  gchar* errstr;

  struct file_idx* idx; /* pointer to the index array */
  struct file_idx_hdr* ihdr; /* pointer to the index header */
  struct file_pld_hdr* phdr; /* pointer to the payload header */
  void* pptr; /* payload pointer */
  guint32 lastid; /* id count (count of files in database/next free file id) */
  /* ids start from 1 and ends in lastid */

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
#if 1
  guint h=0,i=0;
  gint primes[8] = {3, 5, 7, 11, 13, 17, 7/*19*/, 5/*23*/};
  while (*path!=0)
  {
    h += *path * primes[i&0x07];
    path++; i++;
  }
  return h%MAXHASH;
#else
  guint32 h=0, a=31415, b=27183;
  for (;*path!=0;path++,a=a*b%(MAXHASH-1))
    h = (a*h+*path)%MAXHASH;
  return h;
#endif
}

/* AVL binary tree
 ************************************************************************/

/* ondisk data structures */
struct file_pld {
  guint16 rc;
//  guint16 mode;
  guint16 plen;
  guint16 llen;
  gchar data[];
  /* data[0] = 'path' + '\0' */
  /* data[plen+1] = 'link' + '\0' */
};

/*XXX: gross hack */
struct file_idx { /* AVL tree leaf */
  guint32 lnk[2]; /* left file in the tree, 0 if none */
  guint32 off; /* offset to the payload file */
  gint8   bal; /* balance value */
};

struct file_pld_hdr {
  gchar magic[12]; /* "FDB.PAYLOAD" */
};

struct file_idx_hdr {
  gchar magic[12]; /* "FDB.PAYLOAD" */
  guint32 lastid;  /* file count, first id is 1 */
  guint32 hashmap[MAXHASH];
};

guint32 _alloc_node(gchar* path, gchar* link)
{
  gsize required_idx_size, required_pld_size;
  gsize pld_off = sizeof(struct file_pld_hdr);
  guint32 id;
  struct file_pld* pld;
  gsize size_path, size_link;

  size_path = path?strlen(path):0;
  size_link = link?strlen(link):0;

  /* calculate required sizes of the idx and pld files */
  required_idx_size = sizeof(struct file_idx_hdr) + (_fdb.lastid+1)*sizeof(struct file_idx);
  if (G_LIKELY(_fdb.lastid > 0))
  {
    pld_off = _fdb.idx[_fdb.lastid-1].off;
    pld = _fdb.addr_pld + pld_off;
    pld_off += pld->plen + pld->llen + sizeof(struct file_pld) + 2;
  }
  required_pld_size = sizeof(struct file_pld_hdr) + sizeof(struct file_pld) + pld_off
                      + size_path+1 + size_link+1;

  if (G_UNLIKELY(required_pld_size > _fdb.size_pld || required_idx_size > _fdb.size_idx))
    return 0;

  /* add file to payload */
  pld = _fdb.addr_pld + pld_off;
  pld->rc = 1;
  pld->plen = size_path;
  pld->llen = size_link;
  if (size_path)
    strcpy(pld->data, path);
  if (size_link)
    strcpy(pld->data+size_path+1, link);

  /* add file to index */
  id = ++_fdb.lastid;
  _fdb.idx[id-1].off = pld_off;

  return id;
}

/* reused from gnuavl library */
static __inline__ struct file_idx* _node(guint32 id) { return id?_fdb.idx+id-1:NULL; }
static __inline__ struct file_pld* _pld(struct file_idx* idx) { return idx?_fdb.addr_pld+idx->off:0; }
static __inline__ guint32 _id(struct file_idx* idx) { return idx?idx-_fdb.idx+1:0; }

static void _print_subtree(FILE* f, guint32 id)
{
  struct file_idx *n = _node(id);
  char* fillcolors[] = { "yellow" , "green", "red"};
  gint col;

  if (id == 0)
    return;

  if (n->bal == 0)
    col = 1;
  else if (n->bal < 0)
    col = 0;
  else
    col = 2;
   
  fprintf(f, "  s%u [style=filled, fillcolor=%s, label=\"<l>%u|<c>%u|<r>%u\"];\n", id, fillcolors[col], n->lnk[0], id, n->lnk[1]);
  if (n->lnk[0])
  {
    _print_subtree(f,n->lnk[0]);
    fprintf(f, "  s%u:l -> s%u:c;\n", id, n->lnk[0]);
  }
  if (n->lnk[1])
  {
    _print_subtree(f,n->lnk[1]);
    fprintf(f, "  s%u:r -> s%u:c;\n", id, n->lnk[1]);
  }
}

static void _print_index(gchar* file)
{
  FILE* f;
  guint32 i;
  f = fopen(file, "w");
  fprintf(f, "digraph structs {\n"
             "  node [shape=record];\n");
  fprintf(f, "  hash [style=filled, fillcolor=blue, label=\"hashtab");
  for(i=0; i<MAXHASH; i++)
    fprintf(f, "|<h%u>%u", i, i);
  fprintf(f, "\"]\n");
  for(i=0; i<MAXHASH; i++)
  {
    guint32 root = _fdb.ihdr->hashmap[i];
    if (root)
    {
      _print_subtree(f, root);
      fprintf(f, "  hash:h%u -> s%u;\n", i, root);
    }
  }
  fprintf(f, "}\n");
  fclose(f);
}

/* returns 1 if inserted, 0 if it already exists */
#define AVL_MAX_HEIGHT 64
static guint32 _ins_node(guint32 root, gchar* path, gchar* link, void* proot)
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
    gint cmp = strcmp(path, _pld(p)->data);
    if (cmp == 0)
      return _id(p);
    if (p->bal != 0)
      z = q, y = p, k = 0;
    da[k++] = dir = cmp > 0;
  }
  
  /* this will break every pointer from above */
  id = _alloc_node(path,link);
  i = _node(id);
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
      g_assert (x->bal == +1);
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
      g_assert(x->bal == -1);
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

guint32 _get_node(gchar* path)
{
  struct file_idx *p;
  guint hash = _hash(path);
  guint32 root = _fdb.ihdr->hashmap[hash];

  if (root == 0)
    return 0;
  for (p=_node(root); p!=NULL; )
  {
    gint cmp = strcmp(path, _pld(p)->data);
    if (cmp < 0)
      p = _node(p->lnk[0]);
    else if (cmp > 0)
      p = _node(p->lnk[1]);
    else
      return _id(p);
  }
  return 0;
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

    _fdb.lastid = head.lastid;
    if (_fdb.lastid*sizeof(struct file_idx)+sizeof(head) > _fdb.size_idx)
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

  ((struct file_idx_hdr*)_fdb.addr_idx)->lastid = _fdb.lastid;

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
  id = _ins_node(_fdb.ihdr->hashmap[hash], path, link, &_fdb.ihdr->hashmap[hash]);
  return id;
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
