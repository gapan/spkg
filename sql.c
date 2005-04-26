#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sql.h"

sqlite3 *sql_db=0;
char* sql_errstr=0;
jmp_buf sql_errjmp;
sql_error_action sql_erract = SQL_ERREXIT;

static sql_error_action _sql_erract; /* saved */

static void sql_handle_error(const char* func, const char* err, const char* sql)
{
  switch (sql_erract)
  {
    case SQL_ERRJUMP:
      sql_errstr = strdup(err);
      longjmp(sql_errjmp,1);
    default:
    case SQL_ERREXIT:
      fprintf(stderr, "error[%s]: %s\n", func, err);
      if (sql)
        fprintf(stderr, "failed sql query: %s\n", sql);
      if (sql_db)
        sqlite3_close(sql_db);
      exit(1);
    case SQL_ERRINFORM:
      fprintf(stderr, "error[%s]: %s\n", func, err);
      if (sql)
        fprintf(stderr, "failed sql query: %s\n", sql);
    break;
    case SQL_ERRIGNORE:
    break;
  }
}

sqlite3* sql_open(const char* file)
{
  int rs;
  sqlite3* db=0;
  rs = sqlite3_open(file, &db);
  if (rs == SQLITE_OK)
  {
    if (sql_db == 0)
      sql_db = db;
    return db;
  }
  sql_handle_error(__func__, sqlite3_errmsg(db), 0);
  sqlite3_close(db);
  return 0;
}

int sql_close()
{
  int rs;
  rs = sqlite3_close(sql_db);
  if (rs == SQLITE_OK)
  {
    sql_db = 0;
    return 0;
  }
  sql_handle_error(__func__, sqlite3_errmsg(sql_db), 0);
  return 1;
}

int sql_exec(const char* sql, ...)
{
  va_list ap;
  char* tmp;
  int rs;
  va_start(ap, sql);
  tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  rs = sqlite3_exec(sql_db,tmp,0,0,0);
  if (rs == SQLITE_OK)
  {
    sqlite3_free(tmp);
    return 0;
  }
  sql_handle_error(__func__, sqlite3_errmsg(sql_db), tmp);
  sqlite3_free(tmp);
  return 1;
}

sqlite3_stmt* sql_prep(const char* sql, ...)
{
  sqlite3_stmt* s;
  va_list ap;
  char* tmp;
  int rs;
  va_start(ap, sql);
  tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  rs = sqlite3_prepare(sql_db, tmp, -1, &s, 0);
  if (rs == SQLITE_OK)
  {
    sqlite3_free(tmp);
    return s;
  }
  sql_handle_error(__func__, sqlite3_errmsg(sql_db), tmp);
  sqlite3_free(tmp);
  return 0;
}

int sql_fini(sqlite3_stmt* s)
{
  int rs;
  rs = sqlite3_finalize(s);
  if (rs == SQLITE_OK)
    return 0;
  sql_handle_error(__func__, sqlite3_errmsg(sql_db), 0);
  return 1;
}

int sql_rest(sqlite3_stmt* s)
{
  int rs;
  rs = sqlite3_reset(s);
  if (rs == SQLITE_OK)
    return 0;
  sql_handle_error(__func__, sqlite3_errmsg(sql_db), 0);
  return 1;
}

int sql_step(sqlite3_stmt* s)
{
  int rs;
  rs = sqlite3_step(s);
  if (rs == SQLITE_DONE)
    return 0;
  else if (rs == SQLITE_ROW)
    return 1;
  sql_handle_error(__func__, sqlite3_errmsg(sql_db), 0);
  return 0;
}

sqlite_int64 sql_rowid()
{
  return sqlite3_last_insert_rowid(sql_db);
}

int sql_table_exist(const char* name)
{
  sqlite3_stmt* q;
  int ret = 0;
  _sql_erract = sql_erract;
  sql_erract = SQL_ERREXIT;

  q = sql_prep("SELECT name FROM sqlite_master WHERE type == 'table' AND name == '%q';", name);
  if (sql_step(q))
    ret = 1;
  sql_fini(q);
  
  sql_erract = _sql_erract;
  return ret;
}
