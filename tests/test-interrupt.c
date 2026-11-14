/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/* Tests for commands interrupted by a signal right after the package
 * database was updated, or in the middle of an upgrade that replaces a
 * symlink with a directory. The test is linked with --wrap for
 * db_add_pkg(), db_replace_pkg() and ta_remove_move(): the wrappers call
 * the real functions and then set sig_break, as if SIGINT arrived at that
 * moment.
 *
 * Whatever the commands do then, the database must hold exactly one
 * version of the package, and the files must be those of that version. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "pkgdb.h"
#include "commands.h"
#include "sigtrap.h"
#include "sys.h"
#include "check.h"

static gboolean interrupt = FALSE;

gint __real_db_add_pkg(struct db_pkg* pkg);
gint __real_db_replace_pkg(gchar* origname, struct db_pkg* pkg);

gint __wrap_db_add_pkg(struct db_pkg* pkg)
{
  gint rv = __real_db_add_pkg(pkg);
  if (interrupt)
    sig_break = TRUE;
  return rv;
}

gint __wrap_db_replace_pkg(gchar* origname, struct db_pkg* pkg)
{
  gint rv = __real_db_replace_pkg(origname, pkg);
  if (interrupt)
    sig_break = TRUE;
  return rv;
}

/* called when an upgrade has moved the old version's symlink aside to
   replace it with a directory */
static gboolean interrupt_replace = FALSE;
void __real_ta_remove_move(gchar* path, gchar* orig_path);

void __wrap_ta_remove_move(gchar* path, gchar* orig_path)
{
  __real_ta_remove_move(path, orig_path);
  if (interrupt_replace)
    sig_break = TRUE;
}

static gint sh(const gchar* cmd)
{
  gint rv = system(cmd);
  if (rv != 0)
    printf("command failed (%d): %s\n", rv, cmd);
  return rv;
}

/* hello-N: usr/share/hello/common and usr/share/hello/vN */
static void make_pkg(gint v)
{
  gchar* cmd = g_strdup_printf(
    "mkdir -p stage%d/usr/share/hello && "
    "echo common%d > stage%d/usr/share/hello/common && "
    "echo v%d > stage%d/usr/share/hello/v%d && "
    "tar -C stage%d --transform 's,^\\./\\(.\\),\\1,S' -czf hello-%d-noarch-1.tgz .",
    v, v, v, v, v, v, v, v);
  CHECK(sh(cmd) == 0);
  g_free(cmd);
}

static gboolean exists(const gchar* path)
{
  return sys_file_type(path, 0) != SYS_NONE;
}

/* app-1: usr/share/app/data -> data-1 (made by doinst.sh), data-1/f
 * app-2: usr/share/app/data/ is a directory with f and g */
static void make_app(void)
{
  CHECK(sh("mkdir -p app1/usr/share/app/data-1 app1/install && "
           "echo f1 > app1/usr/share/app/data-1/f && "
           "echo '( cd usr/share/app ; rm -rf data )' > app1/install/doinst.sh && "
           "echo '( cd usr/share/app ; ln -sf data-1 data )' >> app1/install/doinst.sh && "
           "tar -C app1 --transform 's,^\\./\\(.\\),\\1,S' -czf app-1-noarch-1.tgz . && "
           "mkdir -p app2/usr/share/app/data && "
           "echo f2 > app2/usr/share/app/data/f && "
           "echo g2 > app2/usr/share/app/data/g && "
           "tar -C app2 --transform 's,^\\./\\(.\\),\\1,S' -czf app-2-noarch-1.tgz .") == 0);
}

static gboolean content_is(const gchar* path, const gchar* want)
{
  gchar* got = NULL;
  gboolean ok = g_file_get_contents(path, &got, NULL, NULL) && !strcmp(got, want);
  g_free(got);
  return ok;
}

/* the installed version of app (-1 if inconsistent): 1 with the symlink,
   2 with the directory */
static gint app_version(void)
{
  gboolean e1 = exists("root/var/lib/pkgtools/packages/app-1-noarch-1");
  gboolean e2 = exists("root/var/lib/pkgtools/packages/app-2-noarch-1");
  gchar* target = g_file_read_link("root/usr/share/app/data", NULL);
  gint v = -1;

  if (exists("root/usr/share/app/data--###symlink###"))
    printf("    the moved symlink is left behind\n");
  else if (e1 && !e2 && target && !strcmp(target, "data-1") &&
           content_is("root/usr/share/app/data-1/f", "f1\n") &&
           !exists("root/usr/share/app/data-1/g"))
    v = 1;
  else if (e2 && !e1 && !target &&
           sys_file_type("root/usr/share/app/data", 0) == SYS_DIR &&
           content_is("root/usr/share/app/data/f", "f2\n") &&
           content_is("root/usr/share/app/data/g", "g2\n") &&
           !exists("root/usr/share/app/data-1"))
    v = 2;
  else
    printf("    inconsistent: app-1 %s, app-2 %s, data -> %s\n",
           e1 ? "installed" : "not installed", e2 ? "installed" : "not installed",
           target ? target : "(not a symlink)");
  g_free(target);
  return v;
}

/* the installed version of hello (0 if none, -1 if inconsistent) */
static gint installed_version(void)
{
  gboolean e1 = exists("root/var/lib/pkgtools/packages/hello-1-noarch-1");
  gboolean e2 = exists("root/var/lib/pkgtools/packages/hello-2-noarch-1");
  if (e1 && e2)
  {
    printf("    both versions are in the database\n");
    return -1;
  }
  gint v = e1 ? 1 : e2 ? 2 : 0;
  gboolean f1 = exists("root/usr/share/hello/v1");
  gboolean f2 = exists("root/usr/share/hello/v2");
  if (f1 != (v == 1) || f2 != (v == 2))
  {
    printf("    database has version %d, but files: v1 %s, v2 %s\n",
           v, f1 ? "present" : "absent", f2 ? "present" : "absent");
    return -1;
  }
  if (v == 0 && exists("root/usr/share/hello/common"))
  {
    printf("    no version in the database, but files are left\n");
    return -1;
  }
  return v;
}

int main(int ac, char* av[])
{
  gchar* tmp = check_enter_tmpdir();
  if (tmp == NULL)
    return 1;
  make_pkg(1);
  make_pkg(2);
  CHECK(sys_mkdir_p("root") == 0);

  struct error* e = e_new();
  struct cmd_options opts = { .root = "root", .verbosity = 0,
                              .no_ldconfig = TRUE,
                              .no_gtk_update_icon_cache = TRUE };
  CHECK_INT(db_open("root", FALSE, e), 0);

  printf("install interrupted after the database update\n");
  interrupt = TRUE;
  gint rv = cmd_install("hello-1-noarch-1.tgz", &opts, e);
  interrupt = FALSE;
  sig_break = FALSE;
  gint v = installed_version();
  printf("    returned %d, installed version %d\n", rv, v);
  CHECK(v == 0 || v == 1);
  CHECK(rv == 0 ? v == 1 : v == 0);
  e_clean(e);

  /* the actual starting point for the upgrade */
  if (v == 0)
    CHECK_INT(cmd_install("hello-1-noarch-1.tgz", &opts, e), 0);
  CHECK_INT(installed_version(), 1);
  e_clean(e);

  printf("upgrade interrupted after the database update\n");
  interrupt = TRUE;
  rv = cmd_upgrade("hello-2-noarch-1.tgz", &opts, e);
  interrupt = FALSE;
  sig_break = FALSE;
  v = installed_version();
  printf("    returned %d, installed version %d\n", rv, v);
  CHECK(v == 1 || v == 2);
  CHECK(rv == 0 ? v == 2 : v == 1);
  CHECK(exists("root/usr/share/hello/common"));
  e_clean(e);

  /* an upgrade that replaces the old version's symlink with a directory */
  make_app();

  printf("upgrade interrupted right after a symlink was moved aside\n");
  CHECK_INT(cmd_install("app-1-noarch-1.tgz", &opts, e), 0);
  CHECK_INT(app_version(), 1);
  interrupt_replace = TRUE;
  rv = cmd_upgrade("app-2-noarch-1.tgz", &opts, e);
  interrupt_replace = FALSE;
  sig_break = FALSE;
  v = app_version();
  printf("    returned %d, installed version %d\n", rv, v);
  CHECK(rv != 0);           /* interrupted before the database update */
  CHECK_INT(v, 1);          /* rolled back: the symlink is back */
  e_clean(e);

  printf("upgrade replacing a symlink interrupted after the database update\n");
  interrupt = TRUE;
  rv = cmd_upgrade("app-2-noarch-1.tgz", &opts, e);
  interrupt = FALSE;
  sig_break = FALSE;
  v = app_version();
  printf("    returned %d, installed version %d\n", rv, v);
  CHECK(v == 1 || v == 2);
  CHECK(rv == 0 ? v == 2 : v == 1);
  e_clean(e);

  if (v == 1)
    CHECK_INT(cmd_upgrade("app-2-noarch-1.tgz", &opts, e), 0);
  CHECK_INT(app_version(), 2);
  e_clean(e);

  db_close();
  e_free(e);
  CHECK(chdir("/") == 0);
  CHECK_INT(sys_rm_rf(tmp), 0);
  g_free(tmp);
  return check_result();
}
