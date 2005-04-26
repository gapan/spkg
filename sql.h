/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#ifndef __SQL_H
#define __SQL_H

#include <sqlite3.h>
#include <setjmp.h>

typedef enum { 
  SQL_ERREXIT, SQL_ERRJUMP, SQL_ERRINFORM, SQL_ERRIGNORE
} sql_error_action;

extern sqlite3 *sql_db;
extern char* sql_errstr;
extern jmp_buf sql_errjmp;
extern sql_error_action sql_erract;

extern sqlite3* sql_open(const char* file);
extern int sql_close();
extern int sql_exec(const char* sql, ...);
extern sqlite3_stmt* sql_prep(const char* sql, ...);
extern int sql_fini(sqlite3_stmt* s);
extern int sql_rest(sqlite3_stmt* s);
extern int sql_step(sqlite3_stmt* s);
extern sqlite_int64 sql_rowid();
extern int sql_table_exist(const char* name);

#endif
