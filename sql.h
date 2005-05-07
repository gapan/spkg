/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup sql_api */
/*! @{ */

#ifndef __SQL_H
#define __SQL_H

#include <glib.h>
#include <sqlite3.h>
#include <setjmp.h>

/** Set how many queries can be opened at one time. */
#define SQL_OPEN_QUERIES_LIMIT 16

/* give these beasts more reasonable names */
/** SQL database object. */
typedef sqlite3 sql;
/** SQL query object. */
typedef sqlite3_stmt sql_query;

/** Error handling type. Whenever error occurs, the \ref sql_errstr is set
 * to point to the error message. If function executes successfully. \ref
 * sql_errstr is set to 0.
 */
typedef enum { 
  SQL_ERREXIT,   /**< print error to stderr, fini all queries, close \ref 
                      sql_db and exit */
  SQL_ERRJUMP,   /**< longjump to \ref sql_errjmp */
  SQL_ERRINFORM, /**< print error to stderr and set \ref sql_errstr */
  SQL_ERRIGNORE  /**< set \ref sql_errstr */
} sql_erract;

/** Active database. */
extern sql* sql_db;

/** Error message if last call to the sql library resulted in error. */
extern gchar* sql_errstr;

/** This can be used to save known state for error recovery in the \ref SQL_ERRJUMP mode. */
extern jmp_buf sql_errjmp;

/** Open SQLite database.
 *
 * @param file Database file.
 * @return sqlite3 object on success, 0 on error.
 */
extern sql* sql_open(const gchar* file);

/** Close SQLite database.
 *
 * @return sqlite3 object on success, 0 on error.
 */
extern gint sql_close();

/** Set error action.
 *
 * @param act Error action.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_push_erract(sql_erract act);

/** Restore error action.
 *
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_pop_erract();

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
 * @param  q SQL query.
 * @return 0 on success (may not return)
 */
extern gint sql_fini(sql_query* q);

/** Reset SQL query for repeated execution.
 *
 * @param  q SQL query.
 * @return 0 on success (may not return)
 */
extern gint sql_rest(sql_query* q);

/** Execute SQL query. This function can be run multiple times
 *  for each row in the result. If SQL query returns row, this
 *  function returns 1. If no more rows are available this 
 *  function returns 0.
 *
 * @param  q SQL query.
 * @return 1 if row is available, 0 if no more rows are available, 
 *         -1 on error (may not return)
 */
extern gint sql_step(sql_query* q);

/** Get automatically generated id of the last INSERT query.
 *
 * @return ID
 */
extern gint64 sql_rowid();

/** Get integer from the specified column of the current row.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return integer (may not return)
 */
extern gint sql_get_int(sql_query* q, guint c);

/** Get text string from the specified column of the current row.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return pointer to the text string (may not return)
 */
extern gchar* sql_get_text(sql_query* q, guint c);

/** Get integer from the specified column of the current row.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return integer (64bit) (may not return)
 */
extern gint64 sql_get_int64(sql_query* q, guint c);

/** Check if the specified column of the current row is NULL.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return 1 if NULL, 0 otherwise (may not return)
 */
extern gboolean sql_get_null(sql_query* q, guint c);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @param v Argument value.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_int(sql_query* q, gint p, gint v);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @param v Argument value.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_text(sql_query* q, gint p, const gchar* v);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_null(sql_query* q, gint p);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @param v Argument value.
 * @return 1 on error, 0 on success (may not return)
 */
extern gint sql_set_int64(sql_query* q, gint p, gint64 v);

/** Check if table with the specified name exists in the database.
 *
 * @param name Table name.
 * @return 1 if table exists
 */
extern gint sql_table_exist(const gchar* name, ...);

#endif

/*! @} */
