/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup sql_api SQL Database API

This is simplistic wrapper API for the sqlite3 database engine. It is
optimized for the single connection at time oriented programming. It has
some features for lazy programmers, like exceptions-like error handling,
query stacks, transaction contexts, etc.

@section assum Features

@li One connection at time. (no need to pass sql handle everywhere)
@li Contexts. (no need to care for freeing sql queries and properly
    closing transactions)
@li Exceptions. (no need to check every sqlite3 call for error, just
    handle it all in one place)
@li Lazy writer. (yeah, shorter names of functions)

@section errs Error handling

Basically, if error occured call to \ref sql_error() will return error 
string. Otherwise it returns 0.

All errors that are returned by the sqlite3 RDBMS are exceptional.
Well designed program should never encounter an error from sql library.

There are 4 user selectable error handling modes:
<dl>
 <dt>\ref SQL_ERREXIT [default]</dt>
 <dd>Print error message to the stderr, close openned queries and 
     call exit(1).</dd>

 <dt>\ref SQL_ERRJUMP</dt>
 <dd>Store error message to the \ref sql_error() and do a longjmp.</dd>

 <dt>\ref SQL_ERRINFORM</dt>
 <dd>Print error message to the stderr and store it to the \ref sql_error(), 
     then return from function in which error occured.</dd>

 <dt>\ref SQL_ERRIGNORE</dt>
 <dd>Store error message to the \ref sql_error(), then return from function 
     in which error occured.</dd>
</dl>

Not every function usese this kind of error handling. Fuctions that
are using it are designated using <b>[E]</b>. Other functions are
using glib assertions. So be careful to debug your code with assertions
enabled.

@section usage Typical usage

Following code shows typical \ref sql_api usage.

@code
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
@endcode

*//*--------------------------------------------------------------------*/
/** @addtogroup sql_api */
/*! @{ */

#ifndef __SQL_H
#define __SQL_H

#include <glib.h>
#include <sqlite3.h>
#include <setjmp.h>

/** How many queries can be opened at one time. */
#define SQL_OPEN_QUERIES_LIMIT 4

/** How many contexts can be used at one time. */
#define SQL_CONTEXT_STACK_LIMIT 4

/* give these beasts more reasonable names */
/** SQL database object. */
typedef sqlite3 sql;
/** SQL query object. */
typedef sqlite3_stmt sql_query;

/** Error handling type. Whenever error occurs, the \ref sql_error() is set
 * to point to the error message. If function executes successfully. \ref
 * sql_error() is set to 0.
 */
typedef enum { 
  SQL_ERREXIT=0, /**< print error to stderr, then call \ref sql_close() and exit(1) */
  SQL_ERRJUMP,   /**< longjump to \ref sql_error_handler() */
  SQL_ERRINFORM, /**< print error to stderr and return error to the caller */
  SQL_ERRIGNORE  /**< return error to the caller */
} sql_erract;

/** This can be used to save known state for error recovery in the \ref SQL_ERRJUMP
 * mode. Don't use this directly. Use \ref sql_error_handler() instead. */
extern jmp_buf sql_errjmp;

/** Just a helper macro for jump based error handling. (See code example.) */
#define sql_error_handler() if (setjmp(sql_errjmp) == 1)

/** Open database.
 *
 * @param file Database file.
 * @return 0 on success, 1 on error.
 */
extern gint sql_open(const gchar* file);

/** Close database.
 *
 */
extern void sql_close();

/** Get error message if last call to the sql library resulted in error. 
 *
 * @return 0 if no error occured, pointer to the error message if it does.
 */
extern gchar* sql_error();

/** Push context, set error action and begin transaction (optionally).
 *
 * @param act Error action.
 * @param begin Whether to begin transaction in new context.
 */
extern void sql_push_context(sql_erract act, gboolean begin);

/** Cleanup current context and restore previous context.
 *
 * This function closes all queries opened in current context
 * and may commit or rollback transaction if it was started in
 * current context.
 *
 * @param commit Whether to commit or rollback transaction.
 */
extern void sql_pop_context(gboolean commit);

/** Begin transaction.
 *
 */
extern void sql_transaction_begin();

/** End transaction.
 *
 * @param commit If transaction shoud be commited set this to 1. 
 *        Otherwise it will be rolled back.
 */
extern void sql_transaction_end(gboolean commit);

/** Execute SQL query <b>[E]</b>.
 *
 * @param sql SQL statement.
 * @return 0 on success.
 */
extern gint sql_exec(const gchar* sql, ...);

/** Prepare SQL query for execution by the \ref sql_step function <b>[E]</b>.
 *
 * @param sql SQL statement.
 * @return \ref sql_query object on success, 0 on error.
 */
extern sql_query* sql_prep(const gchar* sql, ...);

/** Free SQL query prepared by the \ref sql_prep function <b>[E]</b>.
 *
 * @param  query Prepared \ref sql_query object.
 * @return 0 on success, 1 on error.
 */
extern gint sql_fini(sql_query* query);

/** Reset SQL query for repeated execution <b>[E]</b>.
 *
 * @param  query Prepared \ref sql_query object.
 * @return 0 on success, 1 on error.
 */
extern gint sql_rest(sql_query* query);

/** Execute SQL query <b>[E]</b>. 
 *
 * This function can be run multiple times for each row in the result.
 *
 * @param  query Prepared \ref sql_query object.
 * @return 1 if row is available, 0 if no more rows are available, 
 *         -1 on error.
 */
extern gint sql_step(sql_query* query);

/** Get automatically generated id of the last INSERT query.
 *
 * @return The last inserted row's ID.
 */
extern gint64 sql_rowid();

/** Get integer from the specified column of the current row.
 *
 * @param query Prepared and 'stepped' SQL query.
 * @param col Column position. Counts from 0.
 * @return integer
 */
extern gint sql_get_int(sql_query* query, guint col);

/** Get text string from the specified column of the current row.
 *
 * @param query Prepared and 'stepped' SQL query.
 * @param col Column position. Counts from 0.
 * @return pointer to the text string
 */
extern gchar* sql_get_text(sql_query* query, guint col);

/** Get blob from the specified column of the current row.
 *
 * @param query Prepared and 'stepped' SQL query.
 * @param col Column position. Counts from 0.
 * @return pointer to the blob
 */
extern const void* sql_get_blob(sql_query* query, guint col);

/** Get blob size from the specified column of the current row.
 *
 * @param query Prepared and 'stepped' SQL query.
 * @param col Column position. Counts from 0.
 * @return blob size
 */
extern guint sql_get_size(sql_query* query, guint col);

/** Get integer from the specified column of the current row.
 *
 * @param query Prepared and 'stepped' SQL query.
 * @param col Column position. Counts from 0.
 * @return integer (64bit)
 */
extern gint64 sql_get_int64(sql_query* query, guint col);

/** Check if the specified column of the current row is NULL.
 *
 * @param query Prepared and 'stepped' SQL query.
 * @param col Column position. Counts from 0.
 * @return 1 if NULL, 0 otherwise
 */
extern gboolean sql_get_null(sql_query* query, guint col);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Parameter value.
 */
extern void sql_set_int(sql_query* query, gint par, gint val);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val String pointer. (data needs to be preserved until next \ref sql_step call)
 */
extern void sql_set_text(sql_query* query, gint par, const gchar* val);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Blob pointer. (data needs to be preserved until next \ref sql_step call)
 * @param len Blob size in bytes.
 */
extern void sql_set_blob(sql_query* query, gint par, void* val, guint len);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 */
extern void sql_set_null(sql_query* query, gint par);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Parameter value.
 */
extern void sql_set_int64(sql_query* query, gint par, gint64 val);

/** Check if table with the specified name exists in the database <b>[E]</b>.
 *
 * @param name Table name.
 * @return 1 if table exists, 0 otherwise.
 */
extern gint sql_table_exist(const gchar* name);

#endif

/*! @} */
