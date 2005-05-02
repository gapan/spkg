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

#include <sqlite3.h>
#include <setjmp.h>

/** Error handling type. */
typedef enum { 
  SQL_ERREXIT,   /**< close sql_db and exit on error */
  SQL_ERRJUMP,   /**< longjump to sql_errjmp on error */
  SQL_ERRINFORM, /**< print error to stderr and return */
  SQL_ERRIGNORE  /**< return silently */
} sql_error_action;

/* give this beast more reasonable name */
typedef sqlite3_stmt sql_query;
typedef sqlite3 sql;

extern sql *sql_db;
extern char* sql_errstr;
extern jmp_buf sql_errjmp;
extern sql_error_action sql_erract;

/** Open SQLite database.
 *
 * @param file Database file.
 * @return sqlite3 object on success, 0 on error.
 */
extern sql* sql_open(const char* file);

/** Close SQLite database.
 *
 * @return sqlite3 object on success, 0 on error.
 */
extern int sql_close();

/** Execute SQL statement on the currently open database.
 *
 * @param sql SQL statement.
 * @return 0 on success (may not return)
 */
extern int sql_exec(const char* sql, ...);

/** Prepare SQL query for execution by the \ref sql_step function.
 *
 * @param sql SQL statement.
 * @return \ref sql_query pointer on success, 0 on error (may not return)
 */
extern sql_query* sql_prep(const char* sql, ...);

/** Free SQL query prepared by the \ref sql_prep function.
 *
 * @param  q SQL query.
 * @return 0 on success (may not return)
 */
extern int sql_fini(sql_query* q);

/** Reset SQL query for repeated execution.
 *
 * @param  q SQL query.
 * @return 0 on success (may not return)
 */
extern int sql_rest(sql_query* q);

/** Execute SQL query. This function can be run multiple times
 *  for each row in the result. If SQL query returns row, this
 *  function returns 1. If no more rows are available this 
 *  function returns 0.
 *
 * @param  q SQL query.
 * @return 1 if row is available, 0 if no more rows are available, 
 *         -1 on error (may not return)
 */
extern int sql_step(sql_query* q);

/** Get automatically generated id of the last INSERT query.
 *
 * @return ID
 */
extern long long int sql_rowid();

/** Get integer from the specified column of the current row.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return integer (may not return)
 */
extern int sql_get_int(sql_query* q, int c);

/** Get text string from the specified column of the current row.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return pointer to the text string (may not return)
 */
extern const unsigned char* sql_get_text(sql_query* q, int c);

/** Get integer from the specified column of the current row.
 *
 * @param q SQL query.
 * @param c Column position. Counts from 0.
 * @return integer (64bit) (may not return)
 */
extern long long int sql_get_int64(sql_query* q, int c);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @param v Argument value.
 * @return 1 on error, 0 on success (may not return)
 */
extern int sql_set_int(sql_query* q, int p, int v);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @param v Argument value.
 * @return 1 on error, 0 on success (may not return)
 */
extern int sql_set_text(sql_query* q, int p, const char* v);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @return 1 on error, 0 on success (may not return)
 */
extern int sql_set_null(sql_query* q, int p);

/** Set positional argument to the specified value.
 *
 * @param q SQL query.
 * @param p Argument position. Counts from 1.
 * @param v Argument value.
 * @return 1 on error, 0 on success (may not return)
 */
extern int sql_set_int64(sql_query* q, int p, long long int v);

/** Check if table with the specified name exists in the database.
 *
 * @param name Table name.
 * @return 1 if table exists
 */
extern int sql_table_exist(const char* name);

#endif

/*! @} */
