/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>

#include "sql.h"

#include "files.c"

#include "tsc.h"

int main(int ac, char* av[])
{
  sql_query* q;
  int j;

  sql_push_context(SQL_ERRJUMP);
  if (setjmp(sql_errjmp) == 1)
  { /* exception occured */
    sql_pop_context();
    fprintf(stderr, "%s\n", sql_error());
    sql_close();
    exit(1);
  }

  /* open database */
  sql_open("bench.db");
  sql_exec("CREATE TABLE tab(id INTEGER PRIMARY KEY, path TEXT);");
  sql_exec("CREATE INDEX tab_idx ON tab(path);");
  sql_exec("BEGIN EXCLUSIVE TRANSACTION;");

  /* fill the table */
  reset_timer(0);
  reset_timer(1);
  reset_timer(2);
  reset_timer(3);
  q = sql_prep("INSERT INTO tab(path) VALUES(?);");
  for (j=0; j<sizeof(files)/sizeof(files[0]); j++)
  {
    continue_timer(0);
    sql_set_text(q, 1, files[j]);
    stop_timer(0);
    continue_timer(1);
    sql_step(q);
    stop_timer(1);
    continue_timer(2);
    sql_rest(q);
    stop_timer(2);
  }
  sql_fini(q);
  print_timer(0, "set_text");
  print_timer(1, "step");
  print_timer(2, "rest");

  sql_exec("COMMIT TRANSACTION;");

  /* close database */
  sql_pop_context();
  sql_close();
  return 0;
}
