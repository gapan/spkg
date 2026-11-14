/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondřej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "sys.h"
#include "check.h"

int main(int ac, char* av[])
{
  /* everything happens inside a private temporary directory */
  gchar* tmp = check_enter_tmpdir();
  if (tmp == NULL)
    return 1;

  /* relative path, several levels deep */
  CHECK_INT(sys_mkdir_p("a/b/c/d/e"), 0);
  CHECK_INT(sys_file_type("a/b/c/d/e", 0), SYS_DIR);

  /* existing directories are fine, and only the missing part is created */
  CHECK_INT(sys_mkdir_p("a/b/c/d/e"), 0);
  CHECK_INT(sys_mkdir_p("a/b/x"), 0);
  CHECK_INT(sys_file_type("a/b/x", 0), SYS_DIR);

  /* "." and ".." components and repeated slashes are simplified */
  CHECK_INT(sys_mkdir_p("./f//g/./h"), 0);
  CHECK_INT(sys_file_type("f/g/h", 0), SYS_DIR);
  CHECK_INT(sys_mkdir_p("i/j/../k"), 0);
  CHECK_INT(sys_file_type("i/k", 0), SYS_DIR);
  CHECK_INT(sys_file_type("i/j", 0), SYS_NONE);

  /* absolute path */
  gchar* abs = g_build_filename(tmp, "abs", "l", "m", NULL);
  CHECK_INT(sys_mkdir_p(abs), 0);
  CHECK_INT(sys_file_type(abs, 0), SYS_DIR);
  g_free(abs);

  /* new directories get mode 0755 (before umask) */
  struct stat st;
  mode_t mask = umask(0);
  umask(mask);
  CHECK(stat("a/b/c", &st) == 0 && (st.st_mode & 07777) == (0755 & ~mask));

  /* a regular file in the way is an error and nothing is created past it */
  CHECK(g_file_set_contents("file", "x", 1, NULL));
  CHECK(sys_mkdir_p("file/n/o") != 0);
  CHECK_INT(sys_file_type("file", 0), SYS_REG);
  CHECK(sys_mkdir_p("file") != 0);

  /* a symlink to a directory is followed */
  CHECK(symlink("a/b", "link") == 0);
  CHECK_INT(sys_mkdir_p("link/p"), 0);
  CHECK_INT(sys_file_type("a/b/p", 0), SYS_DIR);

  CHECK(chdir("/") == 0);
  CHECK_INT(sys_rm_rf(tmp), 0);
  g_free(tmp);
  return check_result();
}
