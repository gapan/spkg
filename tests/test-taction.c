/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/* Tests for the filesystem transaction layer used for rollback. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "taction.h"
#include "message.h"
#include "sys.h"
#include "check.h"

static void put(const gchar* path, const gchar* content)
{
  CHECK(g_file_set_contents(path, content, -1, NULL));
}

static gboolean content_is(const gchar* path, const gchar* want)
{
  gchar* got = NULL;
  gboolean ok = g_file_get_contents(path, &got, NULL, NULL) && !strcmp(got, want);
  g_free(got);
  return ok;
}

static gboolean link_to(const gchar* path, const gchar* want)
{
  gchar* got = g_file_read_link(path, NULL);
  gboolean ok = got && !strcmp(got, want);
  g_free(got);
  return ok;
}

static gboolean mode_is(const gchar* path, mode_t mode)
{
  struct stat st;
  return stat(path, &st) == 0 && (st.st_mode & 07777) == mode;
}

static gboolean same_file(const gchar* a, const gchar* b)
{
  struct stat sa, sb;
  return stat(a, &sa) == 0 && stat(b, &sb) == 0 &&
         sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

#define D(s) g_strdup(s) /* ta_* functions take ownership of paths */

/* remove the test tree from a previous case */
static gint sh_rm(void)
{
  return sys_file_type("t", 0) == SYS_NONE ? 0 : sys_rm_rf("t");
}

/* Filesystem state before the actions are registered, as the commands
 * leave it: new files are already extracted (to temporary names where
 * they replace existing files), nothing else has changed yet. */
static void setup(void)
{
  CHECK(sh_rm() == 0);
  CHECK(sys_mkdir_p("t/newdir") == 0);
  put("t/newdir/new", "new");                 /* extracted new file */
  put("t/existing", "old");                   /* file being upgraded */
  put("t/existing--###upgrade###", "updated"); /* its new content */
  put("t/forcesym", "in the way");            /* replaced by a symlink */
  put("t/forcelink", "in the way");           /* replaced by a hardlink */
  put("t/perm", "perm");
  CHECK(chmod("t/perm", 0600) == 0);
  CHECK(sys_mkdir_p("t/gone/sub") == 0);      /* removed on finalize */
  put("t/gone/sub/file", "x");
}

/* The same set of actions for the finalize, rollback and dry-run cases. */
static void register_actions(void)
{
  /* directory removals are registered first but done last */
  ta_remove_nothing(D("t/gone"), 1);
  ta_remove_nothing(D("t/gone/sub"), 1);
  ta_remove_nothing(D("t/gone/sub/file"), 0);

  ta_keep_remove(D("t/newdir"), 1);
  ta_keep_remove(D("t/newdir/new"), 0);
  ta_move_remove(D("t/existing--###upgrade###"), D("t/existing"));
  /* the hardlink target only exists after the move above: checks order */
  ta_link_nothing(D("t/hardlink"), D("t/existing"));
  ta_symlink_nothing(D("t/symlink"), D("existing"));
  ta_forcesymlink_nothing(D("t/forcesym"), D("existing"));
  ta_forcelink_nothing(D("t/forcelink"), D("t/existing"));
  ta_chperm_nothing(D("t/perm"), 0644, getuid(), getgid());
}

static void test_finalize(void)
{
  struct error* e = e_new();
  printf("finalize\n");
  setup();
  CHECK_INT(ta_initialize(FALSE, e), 0);
  register_actions();
  CHECK_INT(ta_finalize(), 0);
  CHECK(e_ok(e));

  CHECK(content_is("t/newdir/new", "new"));
  CHECK(content_is("t/existing", "updated"));
  CHECK_INT(sys_file_type("t/existing--###upgrade###", 0), SYS_NONE);
  CHECK(same_file("t/hardlink", "t/existing"));
  CHECK(link_to("t/symlink", "existing"));
  CHECK(link_to("t/forcesym", "existing"));
  CHECK(same_file("t/forcelink", "t/existing"));
  CHECK(mode_is("t/perm", 0644));
  CHECK_INT(sys_file_type("t/gone", 0), SYS_NONE);
  e_free(e);
}

static void test_rollback(void)
{
  struct error* e = e_new();
  printf("rollback\n");
  setup();
  CHECK_INT(ta_initialize(FALSE, e), 0);
  register_actions();
  CHECK_INT(ta_rollback(), 0);
  CHECK(e_ok(e));

  /* new files and directories are removed (files before their dirs) */
  CHECK_INT(sys_file_type("t/newdir", 0), SYS_NONE);
  /* the temporary file is removed and the original is untouched */
  CHECK_INT(sys_file_type("t/existing--###upgrade###", 0), SYS_NONE);
  CHECK(content_is("t/existing", "old"));
  /* nothing else happened */
  CHECK_INT(sys_file_type("t/hardlink", 0), SYS_NONE);
  CHECK_INT(sys_file_type("t/symlink", 0), SYS_NONE);
  CHECK(content_is("t/forcesym", "in the way"));
  CHECK(content_is("t/forcelink", "in the way"));
  CHECK(mode_is("t/perm", 0600));
  CHECK(content_is("t/gone/sub/file", "x"));
  e_free(e);
}

static void test_dryrun(void)
{
  struct error* e = e_new();
  gint pass;
  for (pass = 0; pass < 2; pass++)
  {
    printf("dry-run %s\n", pass ? "rollback" : "finalize");
    setup();
    CHECK_INT(ta_initialize(TRUE, e), 0);
    register_actions();
    CHECK_INT(pass ? ta_rollback() : ta_finalize(), 0);
    CHECK(e_ok(e));

    /* nothing at all is touched */
    CHECK(content_is("t/newdir/new", "new"));
    CHECK(content_is("t/existing--###upgrade###", "updated"));
    CHECK(content_is("t/existing", "old"));
    CHECK_INT(sys_file_type("t/hardlink", 0), SYS_NONE);
    CHECK_INT(sys_file_type("t/symlink", 0), SYS_NONE);
    CHECK(content_is("t/forcesym", "in the way"));
    CHECK(content_is("t/forcelink", "in the way"));
    CHECK(mode_is("t/perm", 0600));
    CHECK(content_is("t/gone/sub/file", "x"));
  }
  e_free(e);
}

static void test_state(void)
{
  struct error* e = e_new();
  printf("transaction state\n");

  /* only one transaction at a time */
  CHECK_INT(ta_initialize(FALSE, e), 0);
  CHECK_INT(ta_initialize(FALSE, e), 1);
  CHECK(e_errno(e) & TA_ACTIVE);
  e_clean(e);
  CHECK_INT(ta_finalize(), 0);

  /* a new one can be started after finalize and after rollback */
  CHECK_INT(ta_initialize(FALSE, e), 0);
  CHECK_INT(ta_rollback(), 0);
  CHECK_INT(ta_initialize(FALSE, e), 0);
  CHECK_INT(ta_finalize(), 0);
  CHECK(e_ok(e));

  /* Finishing a transaction that wasn't started returns 1. There is no
   * error object to report to then, which used to abort in e_add(), so
   * this runs in a child process. */
  gint i;
  for (i = 0; i < 2; i++)
  {
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0)
    {
      /* silence the GLib assertion message */
      if (!freopen("/dev/null", "w", stderr) || !freopen("/dev/null", "w", stdout))
        _exit(3);
      _exit(i ? ta_rollback() : ta_finalize());
    }
    gint status;
    CHECK(waitpid(pid, &status, 0) == pid);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 1);
  }
  e_free(e);
}

int main(int ac, char* av[])
{
  gchar* tmp = check_enter_tmpdir();
  if (tmp == NULL)
    return 1;
  msg_setup(0); /* keep the action log quiet */

  test_finalize();
  test_rollback();
  test_dryrun();
  test_state();

  CHECK(chdir("/") == 0);
  CHECK_INT(sys_rm_rf(tmp), 0);
  g_free(tmp);
  return check_result();
}
