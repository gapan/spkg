/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "sql.h"

int main(int ac, char* av[])
{
  sql_query* q;

  // open database
  if (sql_open("test.db"))
  {
    fprintf(stderr,"error: can't open sql database\n");
    return 1;
  }
  
  // Automatically close databse at exit.
  atexit((void(*)(void))sql_close);

  // Save current context and open new context with exception 
  // like error handling.
  sql_push_context(SQL_ERRJUMP, 0);
  // This code will be called on exception.
  sql_error_handler()
  { 
    // This is unneccessary here, because contexts will be popped 
    // automatically by the sql_close, but it will not do any harm.
    // This will became neccessary when used in library function to
    // restore caller's context.
    sql_pop_context(0);
    fprintf(stderr, "%s\n", sql_error());
    return 1;
  }

  // Do some internal sqlite3 hacks to speedup queries.
  sql_exec("PRAGMA temp_store = MEMORY;");
  sql_exec("PRAGMA synchronous = OFF;");

  // Begin SQL transaction.
  sql_transaction_begin();

  if (!sql_table_exist("tab"))
  { 
    // If given table does not exist in database create it...
    sql_exec("CREATE TABLE tab(id INTEGER PRIMARY KEY, name TEXT, age INTEGER DEFAULT 0);");

    // ...and fill it with some data.
    q = sql_prep("INSERT INTO tab(name) VALUES(?);");
    char* names[] = { "bob", "bill", "ben", 0 };
    char** n = names;
    while (*n != 0)
    {
      sql_set_text(q, 1, *n);
      sql_step(q);
      sql_rest(q);
      n++;
    }
    sql_fini(q);
  }
  else
  {
    // If given table exist in database update it.
    sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 20, "bob");
    sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 30, "bill");
    sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 40, "ben");
  }

  // Display table contents.
  q = sql_prep("SELECT id,name,age FROM tab;");
  while (sql_step(q))
  {
    printf("%d: %s (%d)\n", sql_get_int(q,0), sql_get_text(q,1), sql_get_int(q,2));
  }
  sql_fini(q);

  // Commit transaction, close current context and restore previous 
  // context.
  sql_pop_context(1);
  return 0;
}
