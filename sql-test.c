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

int main(int ac, char* av[])
{
  char* names[] = { "bob", "bill", "ben", 0 };
  char** name;
  sql_query* q;

  sql_push_erract(SQL_ERRJUMP);
  if (setjmp(sql_errjmp) == 1)
  { /* exception occured */
    fprintf(stderr, "%s\n", sql_errstr);
    exit(1);
    sql_close();
  }

  /* open database */
  sql_open("test.db");

  if (!sql_table_exist("tab"))
  {
    /* create table */
    sql_exec("CREATE TABLE tab(id INTEGER PRIMARY KEY, name TEXT, age INTEGER DEFAULT 0);");

    /* fill the table */
    q = sql_prep("INSERT INTO tab(name) VALUES(?);");
    name = names;
    while (*name != 0)
    {
      sql_set_text(q, 1, *name);
      sql_step(q);
      sql_rest(q);
      name++;
    }
    sql_fini(q);
  }
  /* update table */
  sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 20, "bob");
  sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 33, "bill");

  /* query table */
  q = sql_prep("SELECT id,name,age FROM tab;");
  while (sql_step(q))
    printf("%d: %s (%d)\n", sql_get_int(q,0), sql_get_text(q,1), sql_get_int(q,2));
  sql_fini(q);

  /* close database */
  sql_close();
  return 0;
}
