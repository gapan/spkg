/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/* Tests for the untgz archive reader. Archives are built with tar from a
 * small tree created in a temporary directory. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "untgz.h"
#include "sys.h"
#include "check.h"

/* 120 characters: longer than the 100 byte name field of a tar header */
#define LONG_NAME "long-name-0123456789012345678901234567890123456789" \
                  "0123456789012345678901234567890123456789012345678901234567890"
#define LONG_DIR  "deep-directory-name-0123456789012345678901234567890123456789"

static gint sh(const gchar* cmd)
{
  gint rv = system(cmd);
  if (rv != 0)
    printf("command failed (%d): %s\n", rv, cmd);
  return rv;
}

static gboolean have(const gchar* tool)
{
  gchar* cmd = g_strdup_printf("command -v %s >/dev/null 2>&1", tool);
  gint rv = system(cmd);
  g_free(cmd);
  return rv == 0;
}

/* The tree archived by every test. Members are listed explicitly so the
 * order in the archive is known. */
static const gchar* members =
  "./ ./dir/ ./dir/file ./dir/empty ./dir/mode640 ./dir/hardlink "
  "./dir/symlink ./dir/fifo ./dir/" LONG_NAME " ./dir/longlink ./dir/big";

static gboolean make_tree(void)
{
  return sh(
    "mkdir -p tree/dir && cd tree && "
    "printf 'file content\\n' > dir/file && chmod 644 dir/file && "
    ": > dir/empty && "
    "printf 'secret\\n' > dir/mode640 && chmod 640 dir/mode640 && "
    "ln dir/file dir/hardlink && "
    "ln -s file dir/symlink && "
    "mkfifo dir/fifo && "
    "printf 'long\\n' > dir/" LONG_NAME " && "
    "ln -s " LONG_NAME " dir/longlink && "
    "head -c 70000 /dev/zero | tr '\\0' 'x' > dir/big && "
    "touch -h -d @1000000000 dir/* && touch -d @1000000000 dir") == 0;
}

static gboolean make_archive(const gchar* name, const gchar* format, const gchar* comp)
{
  gchar* cmd = g_strdup_printf(
    "cd tree && tar --format=%s --no-recursion -cf - %s | %s > ../%s",
    format, members, comp, name);
  gboolean ok = sh(cmd) == 0;
  g_free(cmd);
  return ok;
}

/* read the whole archive and check every entry */
static void check_archive(const gchar* name)
{
  struct error* e = e_new();
  printf("archive %s\n", name);
  struct untgz_state* tgz = untgz_open(name, e);
  CHECK(tgz != NULL);
  if (tgz == NULL)
  {
    e_print(e);
    e_free(e);
    return;
  }

  struct { const gchar* name; untgz_filetype type; } want[] = {
    { "./",                   UNTGZ_DIR },
    { "./dir/",               UNTGZ_DIR },
    { "./dir/file",           UNTGZ_REG },
    { "./dir/empty",          UNTGZ_REG },
    { "./dir/mode640",        UNTGZ_REG },
    { "./dir/hardlink",       UNTGZ_LNK },
    { "./dir/symlink",        UNTGZ_SYM },
    { "./dir/fifo",           UNTGZ_FIFO },
    { "./dir/" LONG_NAME,     UNTGZ_REG },
    { "./dir/longlink",       UNTGZ_SYM },
    { "./dir/big",            UNTGZ_REG },
  };
  guint n = 0;
  gsize total = 0;

  while (untgz_get_header(tgz) == 0)
  {
    if (n >= G_N_ELEMENTS(want))
    {
      printf("FAILED: unexpected entry %s\n", tgz->f_name);
      check_failures++;
      break;
    }
    CHECK_STR(tgz->f_name, want[n].name);
    CHECK_INT(tgz->f_type, want[n].type);
    if (g_str_has_prefix(tgz->f_name, "./dir/"))
      CHECK_INT(tgz->f_mtime, 1000000000);
    CHECK_INT(tgz->f_uid, getuid());
    CHECK_INT(tgz->f_gid, getgid());

    gchar* buf = NULL;
    gsize len = 0;
    gint rv = untgz_write_data(tgz, &buf, &len);
    const gchar* fn = tgz->f_name;
    if (!strcmp(fn, "./dir/file"))
    {
      CHECK_INT(rv, 0);
      CHECK_INT(len, 13);
      CHECK_STR(buf, "file content\n");
      CHECK_INT(tgz->f_size, 13);
      CHECK_INT(tgz->f_mode, 0644);
    }
    else if (!strcmp(fn, "./dir/empty"))
    {
      CHECK_INT(rv, 0);
      CHECK_INT(len, 0);
    }
    else if (!strcmp(fn, "./dir/mode640"))
      CHECK_INT(tgz->f_mode, 0640);
    else if (!strcmp(fn, "./dir/hardlink"))
    {
      CHECK_INT(rv, 1); /* no data */
      CHECK_STR(tgz->f_link, "./dir/file");
    }
    else if (!strcmp(fn, "./dir/symlink"))
    {
      CHECK_INT(rv, 1);
      CHECK_STR(tgz->f_link, "file");
    }
    else if (!strcmp(fn, "./dir/" LONG_NAME))
      CHECK_STR(buf, "long\n");
    else if (!strcmp(fn, "./dir/longlink"))
      CHECK_STR(tgz->f_link, LONG_NAME);
    else if (!strcmp(fn, "./dir/big"))
    {
      CHECK_INT(rv, 0);
      CHECK_INT(len, 70000);
      CHECK(buf && buf[0] == 'x' && buf[69999] == 'x' && buf[70000] == '\0');
    }
    else
      CHECK_INT(rv, 1); /* directories and fifos have no data */

    /* data can only be read once */
    if (rv == 0)
    {
      gchar* again = NULL;
      gsize again_len = 0;
      CHECK_INT(untgz_write_data(tgz, &again, &again_len), 1);
      g_free(again);
    }
    g_free(buf);
    total += len;
    n++;
  }
  CHECK_INT(n, G_N_ELEMENTS(want));
  CHECK(e_ok(e));
  CHECK_INT(tgz->usize, total);
  untgz_close(tgz);
  e_free(e);
}

/* skip over file data without reading it */
static void check_skipping(const gchar* name)
{
  struct error* e = e_new();
  struct untgz_state* tgz = untgz_open(name, e);
  CHECK(tgz != NULL);
  guint n = 0;
  while (tgz && untgz_get_header(tgz) == 0)
    n++;
  CHECK_INT(n, 11);
  CHECK(e_ok(e));
  if (tgz)
    untgz_close(tgz);
  e_free(e);
}

/* extract entries to disk and check the result */
static void check_extract(const gchar* name)
{
  struct error* e = e_new();
  struct untgz_state* tgz = untgz_open(name, e);
  CHECK(tgz != NULL);
  if (tgz == NULL)
  {
    e_free(e);
    return;
  }
  CHECK(sh("rm -rf out && mkdir out") == 0);
  while (untgz_get_header(tgz) == 0)
  {
    struct stat st;
    if (!strcmp(tgz->f_name, "./dir/mode640"))
    {
      CHECK_INT(untgz_write_file(tgz, "out/mode640"), 0);
      CHECK_INT(untgz_write_file(tgz, "out/mode640"), 1); /* already written */
      CHECK(stat("out/mode640", &st) == 0);
      CHECK_INT(st.st_mode & 07777, 0640);
      CHECK_INT(st.st_mtime, 1000000000);
      CHECK_INT(st.st_size, 7);
    }
    else if (!strcmp(tgz->f_name, "./dir/big"))
    {
      CHECK_INT(untgz_write_file(tgz, "out/big"), 0);
      CHECK(stat("out/big", &st) == 0);
      CHECK_INT(st.st_size, 70000);
    }
    else if (!strcmp(tgz->f_name, "./dir/"))
    {
      CHECK_INT(untgz_write_file(tgz, "out/dir"), 0);
      CHECK_INT(sys_file_type("out/dir", 0), SYS_DIR);
    }
    else if (!strcmp(tgz->f_name, "./dir/fifo"))
    {
      CHECK_INT(untgz_write_file(tgz, "out/fifo"), 0);
      CHECK_INT(sys_file_type("out/fifo", 0), SYS_FIFO);
    }
    else if (!strcmp(tgz->f_name, "./dir/file"))
    {
      CHECK_INT(untgz_write_file(tgz, "out/file"), 0);
      CHECK(chmod("out/file", 0600) == 0);
    }
    else if (!strcmp(tgz->f_name, "./dir/symlink"))
    {
      /* the symlink points to out/file */
      CHECK_INT(untgz_write_file(tgz, "out/symlink"), 0);
      CHECK_INT(sys_file_type("out/symlink", 0), SYS_SYM);
      /* metadata is set on the symlink itself, its target is untouched */
      CHECK(stat("out/file", &st) == 0 && (st.st_mode & 07777) == 0600);
      CHECK(lstat("out/symlink", &st) == 0 && st.st_mtime == 1000000000);
    }
  }
  CHECK(e_ok(e));
  untgz_close(tgz);
  e_free(e);
}

/* broken archives must fail cleanly */
static void check_broken(void)
{
  struct error* e = e_new();
  struct untgz_state* tgz;

  /* missing file and unknown extension */
  CHECK(untgz_open("missing.tgz", e) == NULL);
  CHECK(!e_ok(e));
  e_clean(e);
  CHECK(sh("cp test.tgz test.zip") == 0);
  CHECK(untgz_open("test.zip", e) == NULL);
  CHECK(!e_ok(e));
  e_clean(e);

  /* truncated in the middle of a file */
  CHECK(sh("head -c 20000 test.tar > trunc.tar") == 0);
  tgz = untgz_open("trunc.tar", e);
  CHECK(tgz != NULL);
  if (tgz)
  {
    gint rv;
    gchar* buf = NULL;
    gsize len;
    while ((rv = untgz_get_header(tgz)) == 0)
    {
      if (untgz_write_data(tgz, &buf, &len) < 0)
        break;
      g_free(buf);
      buf = NULL;
    }
    CHECK(!e_ok(e));
    CHECK(e_errno(e) & UNTGZ_CORRUPT);
    e_clean(e);
    /* no more entries after an error */
    CHECK(untgz_get_header(tgz) != 0);
    /* the reader stays blocked after a corrupt archive error */
    CHECK(e_errno(e) & UNTGZ_BLOCKED);
    e_clean(e);
    gchar* buf2 = NULL;
    gsize len2;
    CHECK_INT(untgz_write_data(tgz, &buf2, &len2), -1);
    CHECK(e_errno(e) & UNTGZ_BLOCKED);
    e_clean(e);
    untgz_close(tgz);
  }

  /* the same, extracting to disk: the partial file is removed, the reader
   * is blocked, and files extracted before are kept */
  tgz = untgz_open("trunc.tar", e);
  CHECK(tgz != NULL);
  if (tgz)
  {
    CHECK(sh("rm -rf part && mkdir part") == 0);
    gint rv = 0;
    while (untgz_get_header(tgz) == 0)
    {
      if (tgz->f_type != UNTGZ_REG)
        continue;
      gchar* out = g_build_filename("part", strrchr(tgz->f_name, '/') + 1, NULL);
      rv = untgz_write_file(tgz, out);
      g_free(out);
      if (rv < 0)
        break;
    }
    CHECK_INT(rv, -1);
    CHECK(e_errno(e) & UNTGZ_CORRUPT);
    e_clean(e);
    CHECK_INT(sys_file_type("part/big", 0), SYS_NONE);
    CHECK_INT(sys_file_type("part/file", 0), SYS_REG);
    CHECK_INT(untgz_get_header(tgz), -1);
    CHECK(e_errno(e) & UNTGZ_BLOCKED);
    e_clean(e);
    untgz_close(tgz);
  }

  /* truncated gzip stream */
  CHECK(sh("head -c 300 test.tgz > trunc.tgz") == 0);
  tgz = untgz_open("trunc.tgz", e);
  if (tgz)
  {
    gchar* buf = NULL;
    gsize len;
    while (untgz_get_header(tgz) == 0)
    {
      if (untgz_write_data(tgz, &buf, &len) < 0)
        break;
      g_free(buf);
      buf = NULL;
    }
    CHECK(!e_ok(e));
    e_clean(e);
    untgz_close(tgz);
  }

  /* corrupted header checksum (first byte of the first file name) */
  CHECK(sh("cp test.tar badsum.tar && printf X | "
           "dd of=badsum.tar bs=1 seek=0 conv=notrunc 2>/dev/null") == 0);
  tgz = untgz_open("badsum.tar", e);
  CHECK(tgz != NULL);
  if (tgz)
  {
    CHECK_INT(untgz_get_header(tgz), -1);
    CHECK(e_errno(e) & UNTGZ_CORRUPT);
    e_clean(e);
    untgz_close(tgz);
  }

  /* not an archive at all (starting with a non-zero byte: a header block
   * starting with zero means end of archive) */
  CHECK(sh("{ printf garbage; head -c 4096 /dev/urandom; } | gzip > garbage.tgz") == 0);
  tgz = untgz_open("garbage.tgz", e);
  if (tgz)
  {
    CHECK_INT(untgz_get_header(tgz), -1);
    CHECK(!e_ok(e));
    e_clean(e);
    untgz_close(tgz);
  }

  /* empty archive: no entries, no error */
  CHECK(sh("head -c 10240 /dev/zero > empty.tar") == 0);
  tgz = untgz_open("empty.tar", e);
  CHECK(tgz != NULL);
  if (tgz)
  {
    CHECK_INT(untgz_get_header(tgz), 1);
    CHECK(e_ok(e));
    untgz_close(tgz);
  }

  e_free(e);
}

/* ustar stores long paths split into a prefix and a name field */
static void check_ustar_prefix(void)
{
  gchar* path = "./" LONG_DIR "/" LONG_DIR "/file";
  gchar* cmd = g_strdup_printf("mkdir -p ustar/" LONG_DIR "/" LONG_DIR " && "
    ": > ustar/%s && cd ustar && tar --format=ustar -cf ../ustar.tar %s",
    path, path);
  CHECK(sh(cmd) == 0);
  g_free(cmd);

  struct error* e = e_new();
  struct untgz_state* tgz = untgz_open("ustar.tar", e);
  CHECK(tgz != NULL);
  if (tgz)
  {
    CHECK_INT(untgz_get_header(tgz), 0);
    /* the path is put back together from the prefix and name fields */
    CHECK_STR(tgz->f_name, path);
    untgz_close(tgz);
  }
  e_free(e);
}

/* xz checks the data it wrote only at the end of a block: the reader must
 * report the decompressor's failure at the end of the archive. Random data
 * is stored by xz as it is, so the flipped byte doesn't stop decoding. */
static void check_decompressor_status(void)
{
  if (!have("xz") || !have("xzdec"))
  {
    printf("decompressor status skipped, xz or xzdec not found\n");
    return;
  }
  printf("decompressor status\n");
  CHECK(sh("mkdir -p rnd && head -c 300000 /dev/urandom > rnd/random && "
           "tar -C rnd -cf - random | xz > rnd.txz && cp rnd.txz bad.txz && "
           "printf '\\125' | dd of=bad.txz bs=1 seek=150000 conv=notrunc 2>/dev/null") == 0);

  struct error* e = e_new();
  const gchar* files[] = { "rnd.txz", "bad.txz" };
  guint f;
  for (f = 0; f < G_N_ELEMENTS(files); f++)
  {
    struct untgz_state* tgz = untgz_open(files[f], e);
    CHECK(tgz != NULL);
    if (!tgz)
      continue;
    gint rv;
    gchar* buf = NULL;
    gsize len;
    while ((rv = untgz_get_header(tgz)) == 0)
    {
      if (untgz_write_data(tgz, &buf, &len) < 0)
        break;
      g_free(buf);
      buf = NULL;
    }
    if (f == 0)
    {
      CHECK_INT(rv, 1);   /* intact: end of archive */
      CHECK(e_ok(e));
    }
    else
    {
      CHECK_INT(rv, -1);  /* corrupted: reported at the end */
      CHECK(e_errno(e) & UNTGZ_CORRUPT);
    }
    e_clean(e);
    untgz_close(tgz);
  }
  e_free(e);
}

int main(int ac, char* av[])
{
  gchar* tmp = check_enter_tmpdir();
  if (tmp == NULL)
    return 1;
  if (!make_tree())
    return 1;

  struct { const gchar* name; const gchar* tool; const gchar* comp; } formats[] = {
    { "test.tar", NULL,     "cat" },
    { "test.tgz", NULL,     "gzip -9" },
    { "test.txz", "xzdec",  "xz" },
    { "test.tlz", "lzip",   "lzip -c" },
    { "test.tbr", "brotli", "brotli -c" },
  };
  guint i;
  for (i = 0; i < G_N_ELEMENTS(formats); i++)
  {
    if (formats[i].tool && !have(formats[i].tool))
    {
      printf("archive %s skipped, %s not found\n", formats[i].name, formats[i].tool);
      continue;
    }
    if (!make_archive(formats[i].name, "gnu", formats[i].comp))
    {
      check_failures++;
      continue;
    }
    check_archive(formats[i].name);
  }

  check_skipping("test.tgz");
  check_extract("test.tgz");
  check_broken();
  check_ustar_prefix();
  check_decompressor_status();

  CHECK(chdir("/") == 0);
  CHECK_INT(sys_rm_rf(tmp), 0);
  g_free(tmp);
  return check_result();
}
