/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup sql_api SQL Database API

This is simplistic wrapper API for the sqlite3 database engine. It is
optimized for the single connection at time oriented programming.

@section assum Assumptions

@li One connection at time
@li Database is open when calling any func except \ref sql_open

@section errs Error handling

Basically, if error occured sql_errstr contains pointer to the error
message, otherwise it is 0. \ref sql_error() is not preserved between
calls to the sql library.

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

@section usage Typical usage

Following code shows \ref sql_api usage.

@code
#include "sql.h"

int main(int ac, char* av[])
{
  char* names[] = { "bob", "bill", "ben", 0 };
  char** name;
  sql_query* q;

  sql_push_context(SQL_ERRJUMP);
  if (setjmp(sql_errjmp) == 1)
  {
    sql_pop_context();
    fprintf(stderr, "%s\n", sql_error());
    sql_close();
    exit(1);
  }

  // open database
  sql_open("test.db");

  if (!sql_table_exist("tab"))
  {
    // create table
    sql_exec("CREATE TABLE tab(id INTEGER PRIMARY KEY, name TEXT, age INTEGER DEFAULT 0);");

    // fill the table
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
  // update table
  sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 20, "bob");
  sql_exec("UPDATE tab SET age = %d WHERE name == '%q';", 33, "bill");

  // query table
  q = sql_prep("SELECT id,name,age FROM tab;");
  while (sql_step(q))
    printf("%d: %s (%d)\n", sql_get_int(q,0), sql_get_text(q,1), sql_get_int(q,2));
  sql_fini(q);

  // close database
  sql_pop_context();
  sql_close();
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

/** Set how many queries can be opened at one time. */
#define SQL_OPEN_QUERIES_LIMIT 4

/** Set how many contexts can be used at one time. */
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
  SQL_ERREXIT=0,   /**< print error to stderr, fini all queries, \ref sql_close() and exit */
  SQL_ERRJUMP,   /**< longjump to \ref sql_errjmp */
  SQL_ERRINFORM, /**< print error to stderr and set \ref sql_error() */
  SQL_ERRIGNORE  /**< set \ref sql_error() */
} sql_erract;

/** This can be used to save known state for error recovery in the \ref SQL_ERRJUMP mode. */
extern jmp_buf sql_errjmp;

/** Open SQLite database.
 *
 * @param file Database file.
 * @return sqlite3 object on success, 0 on error.
 */
extern gint sql_open(const gchar* file);

/** Close SQLite database.
 *
 * @return sqlite3 object on success, 0 on error.
 */
extern gint sql_close();

/** Get error message if last call to the sql library resulted in error. 
 *
 * @return 0 if no error occured, pointer to the error message if it does.
 */
extern gchar* sql_error();

/** Push context and set error action.
 *
 * @param act Error action.
 * @param begin Whether to begin transaction in new context.
 * @return 1 if we are on the top of the context stack, 0 on success
 */
extern gint sql_push_context(sql_erract act, gboolean begin);

/** Restore previous context.
 *
 * @param commit Whether to commit transaction that was started 
 *        in the current context.
 * @return 1 if we have reached toplevel context, 0 on success
 */
extern gint sql_pop_context(gboolean commit);

/** Begin transaction.
 *
 * @return 1 on error, 0 on success
 */
extern gint sql_transaction_begin();

/** End transaction.
 *
 * @param commit If transaction shoud be commited set this to 1.
 * @return 1 on error, 0 on success
 */
extern gint sql_transaction_end(gboolean commit);

/** Execute SQL statement on the currently open database.
 *
 * @param sql SQL statement.
 * @return 0 on success (may not return)
 */
extern gint sql_exec(const gchar* sql, ...);

/** Prepare SQL query for execution by the \ref sql_step function.
 *
 * @param sql SQL statement.
 * @return \ref sql_query pointer on success, 0 on error (may not return)
 */
extern sql_query* sql_prep(const gchar* sql, ...);

/** Free SQL query prepared by the \ref sql_prep function.
 *
 * @param  query SQL query.
 * @return 0 on success (may not return)
 */
extern gint sql_fini(sql_query* query);

/** Reset SQL query for repeated execution.
 *
 * @param  query SQL query.
 * @return 0 on success (may not return)
 */
extern gint sql_rest(sql_query* query);

/** Execute SQL query. This function can be run multiple times
 *  for each row in the result. If SQL query returns row, this
 *  function returns 1. If no more rows are available this 
 *  function returns 0.
 *
 * @param  query SQL query.
 * @return 1 if row is available, 0 if no more rows are available, 
 *         -1 on error (may not return)
 */
extern gint sql_step(sql_query* query);

/** Get automatically generated id of the last INSERT query.
 *
 * @return ID
 */
extern gint64 sql_rowid();

/** Get integer from the specified column of the current row.
 *
 * @param query SQL query.
 * @param col Column position. Counts from 0.
 * @return integer (may not return)
 */
extern gint sql_get_int(sql_query* query, guint col);

/** Get text string from the specified column of the current row.
 *
 * @param query SQL query.
 * @param col Column position. Counts from 0.
 * @return pointer to the text string (may not return)
 */
extern gchar* sql_get_text(sql_query* query, guint col);

/** Get blob from the specified column of the current row.
 *
 * @param query SQL query.
 * @param col Column position. Counts from 0.
 * @return pointer to the blob (may not return)
 */
extern const void* sql_get_blob(sql_query* query, guint col);

/** Get blob size from the specified column of the current row.
 *
 * @param query SQL query.
 * @param col Column position. Counts from 0.
 * @return blob size (may not return)
 */
extern guint sql_get_size(sql_query* query, guint col);

/** Get integer from the specified column of the current row.
 *
 * @param query SQL query.
 * @param col Column position. Counts from 0.
 * @return integer (64bit) (may not return)
 */
extern gint64 sql_get_int64(sql_query* query, guint col);

/** Check if the specified column of the current row is NULL.
 *
 * @param query SQL query.
 * @param col Column position. Counts from 0.
 * @return 1 if NULL, 0 otherwise (may not return)
 */
extern gboolean sql_get_null(sql_query* query, guint col);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Parameter value.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_int(sql_query* query, gint par, gint val);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Parameter value.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_text(sql_query* query, gint par, const gchar* val);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Blob pointer.
 * @param len Blob size in bytes.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_blob(sql_query* query, gint par, void* val, guint len);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_null(sql_query* query, gint par);

/** Set positional argument to the specified value.
 *
 * @param query SQL query.
 * @param par Parameter position. Counts from 1.
 * @param val Parameter value.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_int64(sql_query* query, gint par, gint64 val);

/** Check if table with the specified name exists in the database.
 *
 * @param name Table name.
 * @return 1 if table exists
 */
extern gint sql_table_exist(const gchar* name, ...);

#endif

/*! @} */
