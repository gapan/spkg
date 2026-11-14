/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/* Tests for the package database, in a temporary root. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "pkgdb.h"
#include "sys.h"
#include "check.h"

#define PKGDIR "var/lib/pkgtools/packages/"
#define SCRDIR "var/lib/pkgtools/scripts/"

static gchar* root;
static struct error* e;

static gchar* read_file(const gchar* path)
{
  gchar* buf = NULL;
  g_file_get_contents(path, &buf, NULL, NULL);
  return buf;
}

static gboolean exists(const gchar* path)
{
  return sys_file_type(path, 0) != SYS_NONE;
}

static gboolean glob_exists(const gchar* pattern)
{
  gchar* cmd = g_strdup_printf("ls %s >/dev/null 2>&1", pattern);
  gint rv = system(cmd);
  g_free(cmd);
  return rv == 0;
}

/* Build a package object like the install command does. */
static struct db_pkg* make_pkg(gchar* name, const gchar* extra_file)
{
  struct db_pkg* p = db_alloc_pkg(name);
  if (p == NULL)
    return NULL;
  db_pkg_add_path(p, ".", DB_PATH_DIR);
  db_pkg_add_path(p, "install", DB_PATH_DIR);
  db_pkg_add_path(p, "install/slack-desc", DB_PATH_FILE);
  db_pkg_add_path(p, "usr", DB_PATH_DIR);
  db_pkg_add_path(p, "usr/share", DB_PATH_DIR);
  db_pkg_add_path(p, "usr/share/common", DB_PATH_DIR);
  db_pkg_add_path(p, "usr/share/common/shared", DB_PATH_FILE);
  db_pkg_add_path(p, extra_file, DB_PATH_FILE);
  return p;
}

/* opening and closing
 ************************************************************************/

static void test_open(void)
{
  printf("open\n");

  /* readonly mode doesn't create the database */
  CHECK_INT(db_open(root, TRUE, e), 1);
  CHECK(!e_ok(e));
  e_clean(e);
  CHECK(!exists("var"));

  CHECK_INT(db_open(root, FALSE, e), 0);
  CHECK(e_ok(e));
  CHECK_INT(sys_file_type(PKGDIR, 1), SYS_DIR);
  CHECK_INT(sys_file_type(SCRDIR, 1), SYS_DIR);
  CHECK_INT(sys_file_type("var/lib/pkgtools/setup", 1), SYS_DIR);
  CHECK_INT(sys_file_type("var/log/pkgtools/removed_packages", 1), SYS_DIR);
  CHECK_INT(sys_file_type("var/log/pkgtools/removed_scripts", 1), SYS_DIR);
  CHECK(exists(PKGDIR ".lock"));

  /* only one database can be open */
  CHECK_INT(db_open(root, FALSE, e), 1);
  CHECK(e_errno(e) & DB_OPEN);
  e_clean(e);
  db_close();

  /* a file in place of a database directory is replaced */
  CHECK(sys_rm_rf("var/lib/pkgtools/setup") == 0);
  CHECK(g_file_set_contents("var/lib/pkgtools/setup", "x", 1, NULL));
  CHECK_INT(db_open(root, FALSE, e), 0);
  CHECK_INT(sys_file_type("var/lib/pkgtools/setup", 1), SYS_DIR);
  db_close();

  /* readonly works once it exists */
  CHECK_INT(db_open(root, TRUE, e), 0);
  db_close();
  CHECK(e_ok(e));
}

/* run fn in a child process; returns TRUE if it exited normally */
static gboolean survives(void (*fn)(void))
{
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0)
  {
    if (!freopen("/dev/null", "w", stderr) || !freopen("/dev/null", "w", stdout))
      _exit(3);
    fn();
    _exit(0);
  }
  gint status;
  return waitpid(pid, &status, 0) == pid && WIFEXITED(status) &&
         WEXITSTATUS(status) == 0;
}

static void query_closed_db(void)
{
  /* should return NULL with DB_NOPEN */
  if (db_get_package_name("hello") != NULL)
    _exit(1);
}

static void test_closed(void)
{
  printf("closed database\n");
  /* functions called on a closed database fail; there is no error object
   * to report DB_NOPEN to then, which used to abort in e_add() */
  CHECK(survives(query_closed_db));
}

/* A second writer must not get the database while the first holds the
 * lock. */
static void test_lock(void)
{
  printf("locking\n");
  gint ready[2];
  CHECK(pipe(ready) == 0);
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0)
  {
    /* hold the lock like another spkg process would */
    gint fd = open(PKGDIR ".lock", O_RDONLY);
    if (fd < 0 || flock(fd, LOCK_EX) != 0)
      _exit(1);
    if (write(ready[1], "x", 1) != 1)
      _exit(1);
    pause();
    _exit(0);
  }
  gchar c;
  CHECK(read(ready[0], &c, 1) == 1);

  /* readonly access doesn't need the lock */
  CHECK_INT(db_open(root, TRUE, e), 0);
  db_close();

  /* waits for the lock (about 2s), then gives up */
  gint rv = db_open(root, FALSE, e);
  CHECK_INT(rv, 1);
  if (rv == 0)
    db_close();
  e_clean(e);

  kill(pid, SIGTERM);
  waitpid(pid, NULL, 0);
  close(ready[0]);
  close(ready[1]);

  /* once released, the lock can be taken */
  CHECK_INT(db_open(root, FALSE, e), 0);
  db_close();
  CHECK(e_ok(e));
}

/* package objects
 ************************************************************************/

static void test_alloc(void)
{
  printf("package objects\n");
  struct db_pkg* p = db_alloc_pkg("foo-bar-1.2.3-x86_64-1gv");
  CHECK(p != NULL);
  if (p)
  {
    CHECK_STR(p->name, "foo-bar-1.2.3-x86_64-1gv");
    CHECK_STR(p->shortname, "foo-bar");
    CHECK_STR(p->version, "1.2.3");
    CHECK_STR(p->arch, "x86_64");
    CHECK_STR(p->build, "1gv");
    CHECK(p->time > 0);

    CHECK_INT(db_pkg_add_path(p, "usr/bin/foo", DB_PATH_FILE), 0);
    CHECK_INT(db_pkg_add_path(p, "usr/bin", DB_PATH_DIR), 0);
    CHECK_INT(db_pkg_get_path(p, "usr/bin/foo"), DB_PATH_FILE);
    CHECK_INT(db_pkg_get_path(p, "usr/bin"), DB_PATH_DIR);
    CHECK_INT(db_pkg_get_path(p, "usr/bin/bar"), DB_PATH_NONE);
    /* adding again changes the type */
    CHECK_INT(db_pkg_add_path(p, "usr/bin/foo", DB_PATH_SYMLINK), 0);
    CHECK_INT(db_pkg_get_path(p, "usr/bin/foo"), DB_PATH_SYMLINK);

    gchar* longpath = g_strnfill(MAXPATHLEN, 'a');
    CHECK_INT(db_pkg_add_path(p, longpath, DB_PATH_FILE), 1);
    g_free(longpath);
    db_free_pkg(p);
  }

  /* alloc reports errors through the open database */
  CHECK_INT(db_open(root, FALSE, e), 0);
  CHECK(db_alloc_pkg("not-a-package") == NULL);
  CHECK(!e_ok(e));
  e_clean(e);
  CHECK(db_alloc_pkg(NULL) == NULL);
  e_clean(e);
  db_close();
  db_free_pkg(NULL); /* allowed */
}

/* adding, reading and removing packages
 ************************************************************************/

static const gchar* hello_entry =
  "PACKAGE NAME:     hello-1.0-noarch-1\n"
  "COMPRESSED PACKAGE SIZE:     12K\n"
  "UNCOMPRESSED PACKAGE SIZE:     2.0M\n"
  "PACKAGE LOCATION: /tmp/hello-1.0-noarch-1.txz\n"
  "PACKAGE DESCRIPTION:\n"
  "hello: hello (test)\n"
  "hello:\n"
  "hello: second line\n"
  "FILE LIST:\n"
  "./\n"
  "install/\n"
  "install/slack-desc\n"
  "usr/\n"
  "usr/bin/hello\n"
  "usr/share/\n"
  "usr/share/common/\n"
  "usr/share/common/shared\n";

static const gchar* hello_script =
  "( cd usr/lib ; rm -rf libhello.so )\n"
  "( cd usr/lib ; ln -sf libhello.so.1 libhello.so )\n"
  "echo done\n";

static void test_add_get(void)
{
  printf("add and get\n");
  CHECK_INT(db_open(root, FALSE, e), 0);

  struct db_pkg* p = make_pkg("hello-1.0-noarch-1", "usr/bin/hello");
  p->csize = 12;
  p->usize = 2048;
  p->location = g_strdup("/tmp/hello-1.0-noarch-1.txz");
  p->desc = g_strdup("hello: hello (test)\nhello:\nhello: second line\n");
  p->doinst = g_strdup(hello_script);
  p->time = 1000000000;
  CHECK_INT(db_add_pkg(p), 0);
  CHECK(e_ok(e));

  /* exact pkgtools format, sorted file list, directories end with / */
  gchar* entry = read_file(PKGDIR "hello-1.0-noarch-1");
  CHECK_STR(entry, hello_entry);
  g_free(entry);
  gchar* script = read_file(SCRDIR "hello-1.0-noarch-1");
  CHECK_STR(script, hello_script);
  g_free(script);
  struct stat st;
  CHECK(stat(PKGDIR "hello-1.0-noarch-1", &st) == 0 && st.st_mtime == 1000000000);
  CHECK((st.st_mode & 07777) == 0644);
  CHECK(stat(SCRDIR "hello-1.0-noarch-1", &st) == 0 && (st.st_mode & 07777) == 0755);
  CHECK(!exists(PKGDIR ".hello-1.0-noarch-1")); /* no temporary file left */

  /* adding it again fails */
  CHECK_INT(db_add_pkg(p), 1);
  CHECK(e_errno(e) & DB_EXIST);
  e_clean(e);
  db_free_pkg(p);

  /* incomplete package objects are rejected */
  p = db_alloc_pkg("empty-1.0-noarch-1");
  CHECK_INT(db_add_pkg(p), 1);
  e_clean(e);
  CHECK(!exists(PKGDIR "empty-1.0-noarch-1"));
  db_free_pkg(p);

  /* read it back */
  p = db_get_pkg("hello-1.0-noarch-1", DB_GET_FULL);
  CHECK(p != NULL);
  if (p)
  {
    CHECK_STR(p->shortname, "hello");
    CHECK_INT(p->csize, 12);
    CHECK_INT(p->usize, 2048);
    CHECK_STR(p->location, "/tmp/hello-1.0-noarch-1.txz");
    CHECK_STR(p->desc, "hello: hello (test)\nhello:\nhello: second line\n");
    CHECK_INT(p->time, 1000000000);
    CHECK_INT(db_pkg_get_path(p, "."), DB_PATH_DIR);
    CHECK_INT(db_pkg_get_path(p, "usr/share/common"), DB_PATH_DIR);
    CHECK_INT(db_pkg_get_path(p, "usr/bin/hello"), DB_PATH_FILE);
    /* symlinks come from the script */
    CHECK_INT(db_pkg_get_path(p, "usr/lib/libhello.so"), DB_PATH_SYMLINK);
    CHECK_INT(db_pkg_get_path(p, "usr/lib"), DB_PATH_NONE);
    CHECK(p->doinst && !strncmp(p->doinst, hello_script, strlen(hello_script)));
    db_free_pkg(p);
  }

  p = db_get_pkg("hello-1.0-noarch-1", DB_GET_WITHOUT_FILES);
  CHECK(p != NULL);
  if (p)
  {
    CHECK_STR(p->location, "/tmp/hello-1.0-noarch-1.txz");
    CHECK(p->paths == NULL);
    CHECK(p->doinst == NULL);
    db_free_pkg(p);
  }

  /* missing and invalid names */
  CHECK(db_get_pkg("missing-1.0-noarch-1", DB_GET_FULL) == NULL);
  CHECK(e_errno(e) & DB_NOTEX);
  e_clean(e);
  CHECK(db_get_pkg("hello", DB_GET_FULL) == NULL);
  e_clean(e);
  CHECK(db_get_pkg(NULL, DB_GET_FULL) == NULL);
  e_clean(e);

  /* sizes of 10M and more are stored in whole megabytes */
  p = make_pkg("big-1.0-noarch-1", "usr/bin/big");
  p->csize = 900;
  p->usize = 20000;
  CHECK_INT(db_add_pkg(p), 0);
  db_free_pkg(p);
  p = db_get_pkg("big-1.0-noarch-1", DB_GET_WITHOUT_FILES);
  CHECK(p != NULL);
  if (p)
  {
    CHECK_INT(p->csize, 900);
    CHECK_INT(p->usize, 19 * 1024);
    db_free_pkg(p);
  }

  db_close();
}

static gint select_short(const struct db_pkg* p, void* data)
{
  return !strcmp(p->shortname, data);
}

static gint select_error(const struct db_pkg* p, void* data)
{
  return -1;
}

static void test_query(void)
{
  printf("query\n");
  CHECK_INT(db_open(root, TRUE, e), 0);

  /* full name, short name, file name */
  gchar* n;
  CHECK_STR(n = db_get_package_name("hello"), "hello-1.0-noarch-1"); g_free(n);
  CHECK_STR(n = db_get_package_name("hello-1.0-noarch-1"), "hello-1.0-noarch-1"); g_free(n);
  CHECK_STR(n = db_get_package_name("/some/dir/hello-1.0-noarch-1.txz"), "hello-1.0-noarch-1"); g_free(n);
  /* a full name only matches exactly (commands look up the short name
   * separately to find other versions) */
  CHECK(db_get_package_name("hello-2.0-noarch-1") == NULL);
  CHECK(db_get_package_name("hell") == NULL);
  CHECK(db_get_package_name("missing") == NULL);

  /* all names, sorted; the lock and temporary files are ignored */
  CHECK(g_file_set_contents(PKGDIR ".tmp-1.0-noarch-1", "x", 1, NULL));
  GSList* l = db_query(NULL, NULL, DB_QUERY_NAMES);
  CHECK_INT(g_slist_length(l), 2);
  if (g_slist_length(l) == 2)
  {
    CHECK_STR(l->data, "big-1.0-noarch-1");
    CHECK_STR(l->next->data, "hello-1.0-noarch-1");
  }
  db_free_query(l, DB_QUERY_NAMES);
  unlink(PKGDIR ".tmp-1.0-noarch-1");

  /* with a selector */
  l = db_query(select_short, "hello", DB_QUERY_PKGS_WITH_FILES);
  CHECK_INT(g_slist_length(l), 1);
  if (l)
  {
    struct db_pkg* p = l->data;
    CHECK_STR(p->name, "hello-1.0-noarch-1");
    CHECK_INT(db_pkg_get_path(p, "usr/bin/hello"), DB_PATH_FILE);
  }
  db_free_query(l, DB_QUERY_PKGS_WITH_FILES);

  l = db_query(select_short, "hello", DB_QUERY_PKGS_WITHOUT_FILES);
  CHECK_INT(g_slist_length(l), 1);
  if (l)
    CHECK(((struct db_pkg*)l->data)->paths == NULL);
  db_free_query(l, DB_QUERY_PKGS_WITHOUT_FILES);

  CHECK(db_query(select_short, "none", DB_QUERY_NAMES) == NULL);
  CHECK(e_ok(e));

  /* selector errors fail the query */
  CHECK(db_query(select_error, NULL, DB_QUERY_NAMES) == NULL);
  CHECK(!e_ok(e));
  e_clean(e);

  db_close();
}

static void test_filelist(void)
{
  printf("file list\n");
  CHECK_INT(db_open(root, FALSE, e), 0);
  CHECK_INT(db_filelist_load(FALSE), 0);

  /* hello and big share these */
  CHECK_INT(db_filelist_get_path_refs("usr/share/common/shared"), 2);
  CHECK_INT(db_filelist_get_path_refs("usr/share"), 2);
  CHECK_INT(db_filelist_get_path_refs("."), 2);
  /* owned by one package */
  CHECK_INT(db_filelist_get_path_refs("usr/bin/hello"), 1);
  CHECK_INT(db_filelist_get_path_refs("usr/lib/libhello.so"), 1); /* from the script */
  /* unknown, and not stored with the trailing slash */
  CHECK_INT(db_filelist_get_path_refs("usr/bin/none"), 0);
  CHECK_INT(db_filelist_get_path_refs("usr/share/"), 0);

  /* updating the list for a package */
  struct db_pkg* p = db_get_pkg("big-1.0-noarch-1", DB_GET_FULL);
  CHECK(p != NULL);
  if (p)
  {
    db_filelist_rem_pkg_paths(p);
    CHECK_INT(db_filelist_get_path_refs("usr/share/common/shared"), 1);
    CHECK_INT(db_filelist_get_path_refs("usr/bin/big"), 0);
    db_filelist_add_pkg_paths(p);
    CHECK_INT(db_filelist_get_path_refs("usr/share/common/shared"), 2);
    CHECK_INT(db_filelist_get_path_refs("usr/bin/big"), 1);
    db_free_pkg(p);
  }

  /* already loaded: no reload unless forced */
  CHECK(g_file_set_contents(PKGDIR "extra-1.0-noarch-1",
    "PACKAGE NAME:     extra-1.0-noarch-1\nFILE LIST:\nusr/bin/hello\n", -1, NULL));
  CHECK_INT(db_filelist_load(FALSE), 0);
  CHECK_INT(db_filelist_get_path_refs("usr/bin/hello"), 1);
  CHECK_INT(db_filelist_load(TRUE), 0);
  CHECK_INT(db_filelist_get_path_refs("usr/bin/hello"), 2);
  unlink(PKGDIR "extra-1.0-noarch-1");

  db_filelist_free();
  CHECK_INT(db_filelist_get_path_refs("usr/bin/hello"), 0);
  db_close();
  CHECK(e_ok(e));
}

static void test_remove_replace(void)
{
  printf("remove and replace\n");

  /* readonly mode can't change anything */
  CHECK_INT(db_open(root, TRUE, e), 0);
  CHECK_INT(db_rem_pkg("big-1.0-noarch-1"), 1);
  e_clean(e);
  struct db_pkg* p = make_pkg("new-1.0-noarch-1", "usr/bin/new");
  CHECK_INT(db_add_pkg(p), 1);
  e_clean(e);
  db_free_pkg(p);
  CHECK(exists(PKGDIR "big-1.0-noarch-1"));
  CHECK(!exists(PKGDIR "new-1.0-noarch-1"));
  db_close();

  CHECK_INT(db_open(root, FALSE, e), 0);

  /* removed entries and scripts are moved to the log directories */
  CHECK_INT(db_rem_pkg("hello-1.0-noarch-1"), 0);
  CHECK(!exists(PKGDIR "hello-1.0-noarch-1"));
  CHECK(!exists(SCRDIR "hello-1.0-noarch-1"));
  CHECK(glob_exists("var/log/pkgtools/removed_packages/hello-1.0-noarch-1-removed-*"));
  CHECK(glob_exists("var/log/pkgtools/removed_scripts/hello-1.0-noarch-1-removed-*"));
  CHECK(db_get_package_name("hello") == NULL);

  CHECK_INT(db_rem_pkg("hello-1.0-noarch-1"), 1);
  CHECK(e_errno(e) & DB_NOTEX);
  e_clean(e);
  CHECK_INT(db_rem_pkg("hello"), 1); /* needs the full name */
  e_clean(e);

  /* replace: the new entry appears, the old one is moved away */
  p = make_pkg("big-2.0-noarch-1", "usr/bin/big2");
  CHECK_INT(db_replace_pkg("big-1.0-noarch-1", p), 0);
  db_free_pkg(p);
  CHECK(exists(PKGDIR "big-2.0-noarch-1"));
  CHECK(!exists(PKGDIR "big-1.0-noarch-1"));
  CHECK(glob_exists("var/log/pkgtools/removed_packages/big-1.0-noarch-1-removed-*"));
  gchar* n;
  CHECK_STR(n = db_get_package_name("big"), "big-2.0-noarch-1"); g_free(n);

  /* replacing a package that isn't there fails and adds nothing */
  p = make_pkg("ghost-2.0-noarch-1", "usr/bin/ghost");
  CHECK_INT(db_replace_pkg("ghost-1.0-noarch-1", p), 1);
  e_clean(e);
  db_free_pkg(p);
  CHECK(!exists(PKGDIR "ghost-2.0-noarch-1"));
  CHECK(!exists(PKGDIR ".ghost-2.0-noarch-1")); /* no temporary entry left */

  db_close();
}

/* entries written by pkgtools
 ************************************************************************/

static void test_pkgtools_entries(void)
{
  printf("pkgtools entries\n");
  CHECK_INT(db_open(root, FALSE, e), 0);

  /* as written by Slackware 15.0 installpkg */
  CHECK(g_file_set_contents(PKGDIR "bash-5.1.016-x86_64-1",
    "PACKAGE NAME:     bash-5.1.016-x86_64-1\n"
    "COMPRESSED PACKAGE SIZE:     1.6M\n"
    "UNCOMPRESSED PACKAGE SIZE:     8.3M\n"
    "PACKAGE LOCATION: /mnt/slackware/a/bash-5.1.016-x86_64-1.txz\n"
    "PACKAGE DESCRIPTION:\n"
    "bash: bash (sh-compatible shell)\n"
    "bash:\n"
    "FILE LIST:\n"
    "./\n"
    "bin/\n"
    "bin/bash4.new\n"
    "install/\n"
    "install/doinst.sh\n"
    "install/slack-desc\n"
    "usr/\n"
    "usr/doc/bash-5.1.016/\n", -1, NULL));
  CHECK(g_file_set_contents(SCRDIR "bash-5.1.016-x86_64-1",
    "( cd bin ; rm -rf sh )\n"
    "( cd bin ; ln -sf bash sh )\n", -1, NULL));

  struct db_pkg* p = db_get_pkg("bash-5.1.016-x86_64-1", DB_GET_FULL);
  CHECK(p != NULL);
  if (p)
  {
    CHECK_INT(p->csize, (guint)(1.6 * 1024));
    CHECK_INT(p->usize, (guint)(8.3 * 1024));
    CHECK_STR(p->location, "/mnt/slackware/a/bash-5.1.016-x86_64-1.txz");
    CHECK_STR(p->desc, "bash: bash (sh-compatible shell)\nbash:\n");
    CHECK_INT(db_pkg_get_path(p, "."), DB_PATH_DIR);
    CHECK_INT(db_pkg_get_path(p, "usr/doc/bash-5.1.016"), DB_PATH_DIR);
    CHECK_INT(db_pkg_get_path(p, "bin/bash4.new"), DB_PATH_FILE);
    CHECK_INT(db_pkg_get_path(p, "bin/sh"), DB_PATH_SYMLINK);
    db_free_pkg(p);
  }
  CHECK(e_ok(e));

  /* entry without a script and with an empty description */
  CHECK(g_file_set_contents(PKGDIR "nodesc-1.0-noarch-1",
    "PACKAGE NAME:     nodesc-1.0-noarch-1\n"
    "COMPRESSED PACKAGE SIZE:     1K\n"
    "UNCOMPRESSED PACKAGE SIZE:     1K\n"
    "PACKAGE LOCATION: nodesc-1.0-noarch-1.tgz\n"
    "PACKAGE DESCRIPTION:\n"
    "FILE LIST:\n"
    "usr/bin/nodesc\n", -1, NULL));
  p = db_get_pkg("nodesc-1.0-noarch-1", DB_GET_FULL);
  CHECK(p != NULL);
  if (p)
  {
    CHECK(p->desc == NULL);
    CHECK(p->doinst == NULL);
    CHECK_INT(db_pkg_get_path(p, "usr/bin/nodesc"), DB_PATH_FILE);
    db_free_pkg(p);
  }

  /* broken entries are rejected */
  CHECK(g_file_set_contents(PKGDIR "wrongname-1.0-noarch-1",
    "PACKAGE NAME:     other-1.0-noarch-1\nFILE LIST:\n", -1, NULL));
  CHECK(db_get_pkg("wrongname-1.0-noarch-1", DB_GET_FULL) == NULL);
  e_clean(e);
  CHECK(g_file_set_contents(PKGDIR "badsize-1.0-noarch-1",
    "PACKAGE NAME:     badsize-1.0-noarch-1\n"
    "COMPRESSED PACKAGE SIZE:     lots\nFILE LIST:\n", -1, NULL));
  CHECK(db_get_pkg("badsize-1.0-noarch-1", DB_GET_FULL) == NULL);
  e_clean(e);
  CHECK(g_file_set_contents(PKGDIR "garbage-1.0-noarch-1",
    "PACKAGE NAME:     garbage-1.0-noarch-1\nsomething else\nFILE LIST:\n", -1, NULL));
  CHECK(db_get_pkg("garbage-1.0-noarch-1", DB_GET_FULL) == NULL);
  e_clean(e);
  CHECK(g_file_set_contents(PKGDIR "nofiles-1.0-noarch-1",
    "PACKAGE NAME:     nofiles-1.0-noarch-1\n", -1, NULL));
  CHECK(db_get_pkg("nofiles-1.0-noarch-1", DB_GET_FULL) == NULL);
  e_clean(e);

  db_close();
}

int main(int ac, char* av[])
{
  root = check_enter_tmpdir();
  if (root == NULL)
    return 1;
  e = e_new();

  test_open();
  test_closed();
  test_lock();
  test_alloc();
  test_add_get();
  test_query();
  test_filelist();
  test_remove_replace();
  test_pkgtools_entries();

  e_free(e);
  CHECK(chdir("/") == 0);
  CHECK_INT(sys_rm_rf(root), 0);
  g_free(root);
  return check_result();
}
