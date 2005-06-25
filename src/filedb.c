/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sys.h"
#include "filedb.h"
#include "bench.h"

/* private 
 ************************************************************************/

#define PLD_SIZE_LIMIT 64
#define IDX_SIZE_LIMIT 32
#define MAXHASH (1024*128)

#define FDB_CHECKSUMS 1

/* autogrow is complicated, disabled for now (bcause not fully implemented) */
#define FDB_AUTOGROW 0

#define SHOW_STATS 1

struct fdb {
  gboolean is_open; /* flase if not open (may not be true if error occured during open) */
  gchar* dbdir; /* filedb dir with pld and idx files */
  struct error* err; /* error object */

  struct file_idx* idx; /* pointer to the index array */
  struct file_idx_hdr* ihdr; /* pointer to the index header */
  struct file_pld_hdr* phdr; /* pointer to the payload header */
  guint32 lastid; /* last used fileid */
  guint32 newoff; /* new offset for pld */

  /* internal mmap and file info */
  gint fd_idx; /* file descriptors */
  gint fd_pld;
  void* addr_idx; /* mmaped addresses */
  void* addr_pld;
  gsize size_pld; /* mmaped sizes */
  gsize size_idx;
  
  jmp_buf errjmp; /* where to jump on error */
};

#define e_set(n, fmt, args...) e_add(db->err, "filedb", __func__, n, fmt, ##args)

static __inline__ guint _fdb_hash(const gchar* path)
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

static __inline__ gint _fdb_strcmp(const guint32 h1, const gchar* s1, const guint32 h2, const gchar* s2)
{
  if (h1<h2)
    return -1;
  else if (h1>h2)
    return 1;
  return strcmp(s1,s2);
}

/* filedb on-disk data structures
 ************************************************************************/

/* index file */
struct file_idx_hdr {
  gchar magic[12]; /* "FDB.PLINDEX" */
  guint32 lastid;  /* file count, first id is 1 */
  guint32 hashmap[MAXHASH];
};

/* AVL tree leaf */
struct file_idx { /* 20 B */
  /* lnk must go first because of the way _ins_node works */
  guint32 lnk[2]; /* left file in the tree, 0 if none */
  guint32 hash;   /* full hash value of the file path */
  guint32 off;    /* offset to the payload file */
  guint16 refs;   /* ref count of the current file */
  gint8   bal;    /* balance */
  guint8  csum;   /* checksum (bytesum of file_idx = 0xff) */
};

/* payload file */
struct file_pld_hdr {
  gchar magic[12]; /* "FDB.PAYLOAD" */
  guint32 newoff; /* offset for the new data */
};

struct file_pld {
  guint16 doff; /* data offset */
  guint16 dlen; /* data length */
  guint8  csum; /* checksum (bytesum of file_pld = 0xff) */
  gchar path[]; /* path + '\0' + data */
};

/* filedb on-disk data structures manipulation functions
 ************************************************************************/

#if FDB_AUTOGROW == 1
static __inline__ void _fdb_grow_mmaped_file(struct fdb* db, void* adr, gsize* size, gint fd)
{
  msync(adr, *size, MS_ASYNC);
  gsize new_size = *size+GROWTH_FACTOR;
  lseek(fd,SEEK_SET,new_size-1);
  gchar z = 0;
  write(fd, &z, 1);
  void* newaddr = mremap(adr, MMAPSIZE, MMAPSIZE, /*MREMAP_MAYMOVE*/0);
  if (newaddr == (void*)-1)
  {
    e_set(FDB_OTHER,"autogrow failed (mremap returned with error: %s)", strerror(errno));
    longjmp(db->errjmp,1);
  }
  *size = new_size;
}
#endif

static __inline__ guint32 _fdb_new_pld(struct fdb* db, const gchar* path, const void* data, const guint16 dlen)
{
  guint32 size_path = strlen(path);
  guint32 size_entry = sizeof(struct file_pld) + size_path+1 + dlen;
  if (G_UNLIKELY(db->newoff + size_entry > db->size_pld))
  {
#if FDB_AUTOGROW == 1
    _fdb_grow_mmaped_file(db, db->addr_pld, &db->size_pld, db->fd_pld);
#else
    e_set(FDB_OTHER,"pld file is full");
    longjmp(db->errjmp,1);
#endif
  }
  guint32 offset = db->newoff;
  struct file_pld* pld = db->addr_pld + offset;
  db->newoff += size_entry;
  pld->doff = size_path+1;
  pld->dlen = dlen;
  strcpy(pld->path, path);
  if (dlen)
    memcpy(pld->path+size_path+1, data, dlen);
#if FDB_CHECKSUMS == 1
  /* calculate csum */
  pld->csum = 0;
  gint i; 
  guint8 sum=0; 
  for (i=0; i<sizeof(struct file_pld); i++)
    sum += ((guint8*)pld)[i];
  pld->csum = 0xff-sum;
#endif
  return offset;
}

static __inline__ guint32 _fdb_update_pld(struct fdb* db, struct file_pld* pld, const void* data, const guint16 dlen)
{
  if (dlen > pld->dlen)
    return _fdb_new_pld(db,pld->path,data,dlen);
  memcpy(pld->path+pld->doff, data, dlen);
#if FDB_CHECKSUMS == 1
  /* calculate new csum */
  pld->csum = 0;
  gint i; 
  guint8 sum=0; 
  for (i=0; i<sizeof(struct file_pld); i++)
    sum += ((guint8*)pld)[i];
  pld->csum = 0xff-sum;
#endif
  return (void*)pld-db->addr_pld;
}

static __inline__ guint32 _fdb_new_idx(struct fdb* db, guint32 offset, guint32 hash)
{
  guint32 id = db->lastid+1;
  if (G_UNLIKELY(sizeof(struct file_idx_hdr) + id*sizeof(struct file_idx) > db->size_idx))
  {
#if FDB_AUTOGROW == 1
    _fdb_grow_mmaped_file(db, db->addr_idx, &db->size_idx, db->fd_idx);
#else
    e_set(FDB_OTHER,"pld file is full");
    longjmp(db->errjmp,1);
#endif
  }
  struct file_idx* idx = db->idx+db->lastid;
  idx->off = offset;
  idx->hash = hash;
  idx->refs = 1;
  db->lastid++;
#if FDB_CHECKSUMS == 1
  /* calculate new csum */
  idx->csum = 0;
  guint8 sum=0; 
  gint i; 
  for (i=0; i<sizeof(struct file_idx); i++)
    sum += ((guint8*)idx)[i];
  idx->csum = 0xff-sum;
#endif
  return id;
}

static __inline__ void _build_pld_data(const struct fdb_file* file, void** data, guint32* dlen)
{
  if (file->link)
  {
    gint l = strlen(file->link);
    *data = file->link;
    *dlen = l+1;
  }
  *data = 0;
  *dlen = 0;
}

static __inline__ void _parse_pld_data(struct fdb_file* file, const void* data, const guint32 dlen)
{
  if (dlen)
    file->link = (gchar*)data;
  else
    file->link = 0;
}

/* functions for index access (db is always ok, idx is always ok) */
static __inline__ struct file_idx* _idx_from_id(struct fdb* db, guint32 id)
{
  if (id == 0)
    return 0;
  if (id > db->lastid)
  {
    e_set(FDB_OTHER,"invlaid idx id (%d)", id);
    longjmp(db->errjmp,1);
  }
  struct file_idx* idx = db->idx+(id-1);
#if FDB_CHECKSUMS == 1
  guint8 sum=0;
  gint i; 
  for (i=0; i<sizeof(struct file_idx); i++)
    sum += ((guint8*)idx)[i];
  if (sum != 0xff)
  {
    e_set(FDB_OTHER,"invlaid checksum of idx entry no.: %d", id);
    longjmp(db->errjmp,1);
  }
#endif
  return idx;
}

static __inline__ guint32 _id_from_idx(struct fdb* db, struct file_idx* idx)
{
  return idx-db->idx+1;
}

static __inline__ void _idx_set_lnk(struct file_idx* idx, gint lnk, guint32 id)
{
#if FDB_CHECKSUMS == 1
  gint i;
  guint8 dsum = 0;
  for(i=0;i<4;i++)
    dsum += ((guint8*)&idx->lnk[lnk])[i] - ((guint8*)&id)[i];
  idx->csum += dsum;
#endif
  idx->lnk[lnk] = id;
}

static __inline__ void _idx_set_bal(struct file_idx* idx, gint8 bal)
{
#if FDB_CHECKSUMS == 1
  idx->csum += idx->bal-bal;
#endif
  idx->bal = bal;
}

static __inline__ void _idx_set_refs(struct file_idx* idx, guint16 refs)
{
#if FDB_CHECKSUMS == 1
  gint i;
  guint8 dsum = 0;
  for(i=0;i<2;i++)
    dsum += ((guint8*)&idx->refs)[i] - ((guint8*)&refs)[i];
  idx->csum += dsum;
#endif
  idx->refs = refs;
}

static __inline__ void _idx_set_off(struct file_idx* idx, guint32 off)
{
#if FDB_CHECKSUMS == 1
  gint i;
  guint8 dsum = 0;
  for(i=0;i<4;i++)
    dsum += ((guint8*)&idx->off)[i] - ((guint8*)&off)[i];
  idx->csum += dsum;
#endif
  idx->off = off;
}

static __inline__ struct file_pld* _pld_from_idx(struct fdb* db, struct file_idx* idx)
{
  if (idx->off >= db->size_pld || idx->off == 0)
  {
    e_set(FDB_OTHER,"invlaid pld offset (0x%08X)", idx->off);
    longjmp(db->errjmp,1);
  }
  struct file_pld* pld = db->addr_pld+idx->off;
#if FDB_CHECKSUMS == 1
  guint8 sum=0;
  gint i; 
  for (i=0; i<sizeof(struct file_pld); i++)
    sum += ((guint8*)pld)[i];
  if (sum != 0xff)
  {
    e_set(FDB_OTHER,"invlaid checksum of pld entry at offset: 0x%08X", idx->off);
    longjmp(db->errjmp,1);
  }
#endif
  /* checksum pld */
  return pld;
}

/* returns 1 if inserted, 0 if it already exists */
#define AVL_MAX_HEIGHT 16
static guint32 _fdb_insert_node(struct fdb* db, const struct fdb_file* file)
{
  struct file_idx *y, *z; /* Top node to update balance factor, and parent. */
  struct file_idx *p, *q; /* Iterator, and parent. */
  struct file_idx *i;     /* Newly inserted node. */
  struct file_idx *w;     /* New root of rebalanced subtree. */
  gint dir;
  guchar da[AVL_MAX_HEIGHT];
  gint k = 0;
  guint32 id;
  gchar* path = file->path;
  guint32 hash = _fdb_hash(path);
  guint32 shash = hash%MAXHASH;
  
  void* data;
  guint32 dlen;
  _build_pld_data(file, &data, &dlen);

  /*HACK: only lnk[0] of z can be modified from now on (and must not be modified
  by _idx_set_* functions if z remains unchanged) */
  z = (struct file_idx *)&db->ihdr->hashmap[shash]; /* root node pointer */
  y = _idx_from_id(db,db->ihdr->hashmap[shash]); /* root node pointer */
  dir = 0;
  for (q=z, p=y; p!=NULL; q=p, p=_idx_from_id(db,p->lnk[dir]))
  {
    gint cmp = _fdb_strcmp(hash, path, p->hash, _pld_from_idx(db,p)->path);
    if (G_UNLIKELY(cmp == 0))
    {
      _idx_set_off(p, _fdb_update_pld(db, _pld_from_idx(db,p), data, dlen));
      _idx_set_refs(p, p->refs+1);
      return _id_from_idx(db,p);
    }
    if (p->bal != 0)
      z = q, y = p, k = 0;
    da[k++] = dir = cmp > 0;
  }

  id = _fdb_new_pld(db, file->path, data, dlen); /* id = offset */
  id = _fdb_new_idx(db, id, hash);
  i = _idx_from_id(db, id);

#if FDB_CHECKSUMS == 1
  if (q == (void*)&db->ihdr->hashmap[shash]) /* dir will be surely 0 in this case */
    q->lnk[dir] = id;
  else
#endif
    _idx_set_lnk(q,dir,id);

  if (y==0)
    return id;

  for (p=y, k=0; p!=i; p=_idx_from_id(db,p->lnk[da[k]]), k++)
  {
    if (da[k] == 0)
      _idx_set_bal(p,p->bal-1);
    else
      _idx_set_bal(p,p->bal+1);
  }

  if (y->bal == -2)
  {
    struct file_idx *x = _idx_from_id(db,y->lnk[0]);
    if (x->bal == -1)
    {
      w = x;
      _idx_set_lnk(y,0,x->lnk[1]);
      _idx_set_lnk(x,1,_id_from_idx(db,y));
      _idx_set_bal(x,0);
      _idx_set_bal(y,0);
    }
    else
    {
      if (G_UNLIKELY(x->bal != +1))
      {
        e_set(FDB_OTHER,"corrupted idx file (broken AVL tree)");
        longjmp(db->errjmp,1);
      }
      w = _idx_from_id(db,x->lnk[1]);
      _idx_set_lnk(x,1,w->lnk[0]);
      _idx_set_lnk(w,0,_id_from_idx(db,x));
      _idx_set_lnk(y,0,w->lnk[1]);
      _idx_set_lnk(w,1,_id_from_idx(db,y));
      if (w->bal == -1)
      {
        _idx_set_bal(x,0);
        _idx_set_bal(y,1);
      }
      else if (w->bal == 0)
      {
        _idx_set_bal(x,0);
        _idx_set_bal(y,0);
      }
      else /* |w->bal == +1| */
      {
        _idx_set_bal(x,-1);
        _idx_set_bal(y,0);
      }
      _idx_set_bal(w,0);
    }
  }
  else if (y->bal == +2)
  {
    struct file_idx *x = _idx_from_id(db,y->lnk[1]);
    if (x->bal == +1)
    {
      w = x;
      _idx_set_lnk(y,1,x->lnk[0]);
      _idx_set_lnk(x,0,_id_from_idx(db,y));
      _idx_set_bal(x,0);
      _idx_set_bal(y,0);
    }
    else
    {
      if (G_UNLIKELY(x->bal != -1))
      {
        e_set(FDB_OTHER,"corrupted idx file (broken AVL tree)");
        longjmp(db->errjmp,1);
      }
      w = _idx_from_id(db,x->lnk[0]);
      _idx_set_lnk(x,0,w->lnk[1]);
      _idx_set_lnk(w,1,_id_from_idx(db,x));
      _idx_set_lnk(y,1,w->lnk[0]);
      _idx_set_lnk(w,0,_id_from_idx(db,y));
      if (w->bal == +1)
      {
        _idx_set_bal(x,0);
        _idx_set_bal(y,-1);
      }
      else if (w->bal == 0)
      {
        _idx_set_bal(x,0);
        _idx_set_bal(y,0);
      }
      else /* |w->bal == -1| */
      {
        _idx_set_bal(x,1);
        _idx_set_bal(y,0);
      }
      _idx_set_bal(w,0);
    }
  }
  else
    return id;

  /*HACK: if z is hash table pointer, we can't do _idx_set_lnk on it */
  gint d = _id_from_idx(db,y) != z->lnk[0];
#if FDB_CHECKSUMS == 1
  if (z == (void*)&db->ihdr->hashmap[shash])
    z->lnk[d] = _id_from_idx(db,w);
  else
#endif
    _idx_set_lnk(z,d,_id_from_idx(db,w));

  return id;
}

/* public 
 ************************************************************************/

struct fdb* fdb_open(const gchar* path, struct error* e)
{
  continue_timer(0);

  g_assert(path != 0);
  g_assert(e != 0);

  struct fdb* db = g_new0(struct fdb, 1);
  db->err = e;

  if (sys_file_type(path,0) == SYS_NONE)
    sys_mkdir_p(path);

  if (sys_file_type(path,0) != SYS_DIR)
  {
    e_set( FDB_OTHER, "can't create file database directory: %s", path);
    goto err_0;
  }

  /* check if fdb directory is ok */
  if (access(path, R_OK|W_OK|X_OK))
  {
    e_set( FDB_OTHER, "inaccessible file database directory: %s", strerror(errno));
    goto err_0;
  }

  /* prepare index object and some paths */
  gchar *path_idx = g_strdup_printf("%s/idx", path);
  gchar *path_pld = g_strdup_printf("%s/pld", path);

  /* open index and payload files */
  db->fd_pld = open(path_pld, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (db->fd_pld == -1)
  {
    e_set( FDB_OTHER, "can't open pld file: %s", strerror(errno));
    goto err_1;
  }

  db->fd_idx = open(path_idx, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  if (db->fd_idx == -1)
  {
    e_set( FDB_OTHER, "can't open idx file: %s", strerror(errno));
    goto err_2;
  }
  
  /* get file sizes */
  continue_timer(10);
  db->size_idx = lseek(db->fd_idx, 0, SEEK_END);
  db->size_pld = lseek(db->fd_pld, 0, SEEK_END);
  stop_timer(10);
  if (db->size_idx == -1 || db->size_pld == -1)
  {
    e_set( FDB_OTHER, "lseek failed: %s", strerror(errno));
    goto err_3;
  }
  lseek(db->fd_idx, 0, SEEK_SET);
  lseek(db->fd_pld, 0, SEEK_SET);
  
  gint j;
  gchar z = 0;
  if (db->size_idx == 0)
  { /* empty idx file (create new) */
    struct file_idx_hdr head;
    memset(&head, 0, sizeof(head));
    /*XXX: check writes */
    lseek(db->fd_idx, 1024*1024*IDX_SIZE_LIMIT-1, SEEK_SET);
    if (write(db->fd_idx, &z, 1) != 1)
    {
      e_set( FDB_OTHER, "write failed: %s", strerror(errno));
      goto err_3;
    }
    db->size_idx = 1024*1024*IDX_SIZE_LIMIT;

    /* create initial header */
    strcpy(head.magic, "FDB.PLINDEX");
    head.lastid = 0;
    for (j=0;j<MAXHASH;j++)
      head.hashmap[j] = 0;

    /* write header */
    lseek(db->fd_idx, 0, SEEK_SET);
    write(db->fd_idx, (void*)&head, sizeof(head));
  }
  else
  {
    struct file_idx_hdr head;
    if (db->size_idx < sizeof(head))
    {
      e_set( FDB_OTHER, "invalid idx file (file too small)");
      goto err_3;
    }

    /* read header */
    read(db->fd_idx, &head, sizeof(head));
    if (strncmp(head.magic, "FDB.PLINDEX", 11) != 0)
    {
      e_set( FDB_OTHER, "invalid idx file (wrong magic)");
      goto err_3;
    }

    if (head.lastid*sizeof(struct file_idx)+sizeof(head) > db->size_idx)
    {
      e_set( FDB_OTHER, "invalid idx file (file too small to fit entries given by lastid)");
      goto err_3;
    }
  }

  /* read header of the payload file */
  if (db->size_pld == 0)
  {
    struct file_pld_hdr head;
    memset(&head, 0, sizeof(head));

    lseek(db->fd_pld, 1024*1024*PLD_SIZE_LIMIT-1, SEEK_SET);
    write(db->fd_pld, &z, 1);
    db->size_pld = 1024*1024*PLD_SIZE_LIMIT;

    strcpy(head.magic, "FDB.PAYLOAD");
    head.newoff = sizeof(head);
    lseek(db->fd_pld, 0, SEEK_SET);
    write(db->fd_pld, (void*)&head, sizeof(head));
  }
  else
  {
    struct file_pld_hdr head;
    if (db->size_pld < sizeof(head))
    {
      e_set( FDB_OTHER, "invalid pld file (file too small)");
      goto err_3;
    }

    read(db->fd_pld, &head, sizeof(head));
    if (strncmp(head.magic, "FDB.PAYLOAD", 11) != 0)
    {
      e_set( FDB_OTHER, "invalid pld file (wrong magic)");
      goto err_3;
    }

    if (head.newoff > db->size_pld)
    {
      e_set( FDB_OTHER, "invalid pld file (corrupted)");
      goto err_3;
    }
  }
  
  /* do mmaps of payload and index */
  db->addr_idx = mmap(0, db->size_idx, PROT_READ|PROT_WRITE, MAP_SHARED, db->fd_idx, 0);
  if (db->addr_idx == (void*)-1)
    goto err_3;
  db->addr_pld = mmap(0, db->size_pld, PROT_READ|PROT_WRITE, MAP_SHARED, db->fd_pld, 0);
  if (db->addr_pld == (void*)-1)
    goto err_4;
  db->idx = db->addr_idx + sizeof(struct file_idx_hdr);
  db->ihdr = db->addr_idx;
  db->phdr = db->addr_pld;
  db->lastid = db->ihdr->lastid;
  db->newoff = db->phdr->newoff;

  db->is_open = 1;
  db->dbdir = g_strdup(path);
  g_free(path_idx);
  g_free(path_pld);

  stop_timer(0);
  return db;
  munmap(db->addr_pld, db->size_pld);
 err_4:
  munmap(db->addr_idx, db->size_idx);
 err_3:
  close(db->fd_pld);
 err_2:
  close(db->fd_idx);
 err_1:
  g_free(path_idx);
  g_free(path_pld);
 err_0:
  g_free(db);
  stop_timer(0);
  return 0;
}

void fdb_close(struct fdb* db)
{
  continue_timer(1);
  g_assert(db != 0);

  if (!db->is_open)
  {
    g_free(db);
    return;
  }

  db->ihdr->lastid = db->lastid;
  db->phdr->newoff = db->newoff;

#if SHOW_STATS == 1
  guint32 is = (db->lastid*sizeof(struct file_idx)+sizeof(struct file_idx_hdr));
  guint32 ps = db->newoff;
  printf("** filedb: pld size = %u kB (%1.1lf%%), idx size = %u kB (%1.1lf%%) (%u files)\n", 
    ps/1024, 100.0*ps/db->size_pld,
    is/1024, 100.0*is/db->size_idx, db->lastid);
#endif

  msync(db->addr_pld, db->size_pld, MS_ASYNC);
  msync(db->addr_idx, db->size_idx, MS_ASYNC);
  munmap(db->addr_pld, db->size_pld);
  munmap(db->addr_idx, db->size_idx);
  close(db->fd_pld);
  close(db->fd_idx);

  g_free(db->dbdir);
  memset(&db, 0, sizeof(db));
  g_free(db);

  stop_timer(1);
  print_timer(0, "[filedb] fdb_open");
  print_timer(1, "[filedb] fdb_close");
  print_timer(2, "[filedb] fdb_add_file");
  print_timer(3, "[filedb] fdb_get_file_id");
  print_timer(4, "[filedb] fdb_get_file");
  print_timer(5, "[filedb] fdb_del_file");
  print_timer(10, "[filedb] BENCH");
}

#define _fdb_call_entry_checks(v) \
  g_assert(db != 0); \
  if (!db->is_open) \
  { \
    e_set(FDB_NOPEN,"file database is NOT open"); \
    return v; \
  } \
  if (G_UNLIKELY(setjmp(db->errjmp) != 0)) \
  { \
    e_set(FDB_OTHER,"internal error"); \
    return v; \
  }

guint32 fdb_add_file(struct fdb* db, const struct fdb_file* file)
{
  g_assert(file != 0);
  _fdb_call_entry_checks(0)

  guint32 id;

  continue_timer(2);
  if (file->path == 0)
  {
    e_set( FDB_OTHER, "file path must be set at least");
    return 0;
  }
  id = _fdb_insert_node(db,file); /* always ok */ 
  stop_timer(2);
  return id;
}

guint32 fdb_get_file_id(struct fdb* db, const gchar* path)
{
  g_assert(path != 0);
  _fdb_call_entry_checks(0)

  struct file_idx *p;
  continue_timer(3);
  guint32 hash = _fdb_hash(path);
  guint32 root = db->ihdr->hashmap[hash%MAXHASH];

  if (root == 0)
    goto not_fnd;
  for (p=_idx_from_id(db,root); p!=NULL; )
  {
    gint cmp = _fdb_strcmp(hash, path, p->hash, _pld_from_idx(db,p)->path);
    if (cmp < 0)
      p = _idx_from_id(db,p->lnk[0]);
    else if (cmp > 0)
      p = _idx_from_id(db,p->lnk[1]);
    else
    {
      stop_timer(3);
      if (p->refs)
        return _id_from_idx(db,p);
      goto not_fnd;
    }
  }
  stop_timer(3);
 not_fnd:
  e_set( FDB_NOTEX, "file not found (%s)", path);
  return 0;
}

gint fdb_get_file(struct fdb* db, const guint32 id, struct fdb_file* file)
{
  g_assert(file != 0);
  _fdb_call_entry_checks(1)

  struct file_pld* pld;
  continue_timer(4);
  pld = _pld_from_idx(db,_idx_from_id(db,id));
  if (pld == 0)
  {
    e_set( FDB_OTHER, "invalid file id");
    stop_timer(4);
    return 1;
  }
  _parse_pld_data(file, pld->path+pld->doff, pld->dlen);
  stop_timer(4);
  return 0;
}

gint fdb_rem_file(struct fdb* db, const guint32 id)
{
  _fdb_call_entry_checks(1)

  continue_timer(5);
  struct file_idx* i = _idx_from_id(db,id);
  if (i->refs == 0)
  {
    e_set( FDB_NOTEX, "file already has zero refs");
    stop_timer(5);
    return 1;
  }
  i->refs--;
  stop_timer(5);
  return 0;
}
