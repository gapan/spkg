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

static void touch(const gchar* path)
{
  CHECK(g_file_set_contents(path, "x", 1, NULL));
}

int main(int ac, char* av[])
{
  /* everything happens inside a private temporary directory */
  gchar* tmp = check_enter_tmpdir();
  if (tmp == NULL)
    return 1;

  /* single regular file */
  touch("file");
  CHECK_INT(sys_rm_rf("file"), 0);
  CHECK_INT(sys_file_type("file", 0), SYS_NONE);

  /* missing path is an error */
  CHECK(sys_rm_rf("missing") != 0);

  /* nested tree with files, empty dirs and a fifo */
  CHECK(sys_mkdir_p("tree/a/b/c") == 0);
  CHECK(sys_mkdir_p("tree/empty") == 0);
  touch("tree/top");
  touch("tree/a/mid");
  touch("tree/a/b/c/deep");
  touch("tree/.hidden");
  CHECK(mkfifo("tree/a/fifo", 0644) == 0);
  CHECK_INT(sys_rm_rf("tree"), 0);
  CHECK_INT(sys_file_type("tree", 0), SYS_NONE);

  /* symlinks are removed, never followed: the targets must survive */
  CHECK(sys_mkdir_p("keep/sub") == 0);
  touch("keep/precious");
  touch("keep/sub/precious");
  CHECK(sys_mkdir_p("victim") == 0);
  CHECK(symlink("../keep", "victim/dirlink") == 0);
  CHECK(symlink("../keep/precious", "victim/filelink") == 0);
  CHECK(symlink("nowhere", "victim/dangling") == 0);
  CHECK_INT(sys_rm_rf("victim"), 0);
  CHECK_INT(sys_file_type("victim", 0), SYS_NONE);
  CHECK_INT(sys_file_type("keep/precious", 0), SYS_REG);
  CHECK_INT(sys_file_type("keep/sub/precious", 0), SYS_REG);

  /* a symlink given directly is removed itself, not its target */
  CHECK(symlink("keep", "toplink") == 0);
  CHECK_INT(sys_rm_rf("toplink"), 0);
  CHECK_INT(sys_file_type("toplink", 0), SYS_NONE);
  CHECK_INT(sys_file_type("keep/precious", 0), SYS_REG);

  CHECK(chdir("/") == 0);
  CHECK_INT(sys_rm_rf(tmp), 0);
  g_free(tmp);
  return check_result();
}
