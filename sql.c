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
#include <string.h>

#include "sql.h"

/* private
 ************************************************************************/

static sql_erract _sql_context_erract = SQL_ERREXIT;
static gchar* _sql_errstr=0;

/* generic error processing functions */
#define _sql_on_error(fmt, args...) __sql_on_error(__func__, fmt, ##args)
static void __sql_on_error(const gchar* func, const gchar* fmt, ...)
{
  va_list ap;
  gchar* errhead;
  gchar* errtail;
  
  /* store error message */
  errhead = g_strdup_printf("error[%s]: ", func);
  va_start(ap, fmt);
  errtail = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  g_free(_sql_errstr);
  _sql_errstr = g_strconcat(errhead, errtail, 0);
  g_free(errhead);
  g_free(errtail);

  switch (_sql_context_erract)
  {
    case SQL_ERRJUMP:
      longjmp(sql_errjmp,1);
    default:
    case SQL_ERREXIT:
      fprintf(stderr, "%s\n", _sql_errstr);
      sql_close();
      exit(1);
    case SQL_ERRINFORM:
      fprintf(stderr, "%s\n", _sql_errstr);
    break;
    case SQL_ERRIGNORE:
    break;
  }
}

static __inline__ void _sql_reset_error()
{
  /* just an optimization */
  if (G_UNLIKELY(_sql_errstr != 0))
  {
    g_free(_sql_errstr);
    _sql_errstr = 0;
  }
}

/* open queries array for the current context */
static sql_query* _sql_context_queries[SQL_OPEN_QUERIES_LIMIT] = {0};

static __inline__ gint _sql_context_pushq(sql_query* query)
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
    if (_sql_context_queries[i] == 0)
    {
      _sql_context_queries[i] = query;
      return 0;
    }
  return 1;
}

static __inline__ gint _sql_context_popq(sql_query* query)
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
    if (_sql_context_queries[i] == query)
    {
      _sql_context_queries[i] = 0;
      return 0;
    }
  return 1;
}

static __inline__ gint _sql_context_fini_all_queries()
{
  gint i, rs;
  gboolean shown=0;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
    if (_sql_context_queries[i] != 0)
    {
      rs = sqlite3_finalize(_sql_context_queries[i]);
      if (rs != SQLITE_OK && shown)
      {
        fprintf(stderr, "panic: some queries can't be properly finalized! :-(\n");
        shown = 1;
      }
    }
  return shown;
}

struct sql_context {
  jmp_buf errjmp;
  sql_erract erract;
  sql_query* queries[SQL_OPEN_QUERIES_LIMIT];
};

static struct sql_context _sql_context_stack[SQL_CONTEXT_STACK_LIMIT];
static gint _sql_context_stack_pos = 0;

static sql* _sql_db=0;

/* public 
 ************************************************************************/

jmp_buf sql_errjmp;

gint sql_push_context(sql_erract act)
{
  if (_sql_context_stack_pos < SQL_CONTEXT_STACK_LIMIT)
  {
    _sql_context_stack[_sql_context_stack_pos].erract = _sql_context_erract;
    _sql_context_stack[_sql_context_stack_pos].errjmp[0] = sql_errjmp[0];
    memcpy(_sql_context_stack[_sql_context_stack_pos].queries, _sql_context_queries, sizeof(_sql_context_queries));
    memset(_sql_context_queries, 0, sizeof(_sql_context_queries));
    _sql_context_stack_pos++;
    _sql_context_erract = act;
    return 0;
  }
  return 1;
}

gint sql_pop_context()
{
  if (_sql_context_stack_pos > 0)
  {
    _sql_context_stack_pos--;
    _sql_context_erract = _sql_context_stack[_sql_context_stack_pos].erract;
    sql_errjmp[0] = _sql_context_stack[_sql_context_stack_pos].errjmp[0];
    _sql_context_fini_all_queries();
    memcpy(_sql_context_queries, _sql_context_stack[_sql_context_stack_pos].queries, sizeof(_sql_context_queries));
    return 0;
  }
  return 1;
}

gchar* sql_error()
{
  return _sql_errstr;
}

gint sql_open(const gchar* file)
{
  gint rs;
  sql* db=0;

  _sql_reset_error();
  if (_sql_db != 0)
  {
    _sql_on_error("database is already open");
    return 1;
  }

  rs = sqlite3_open(file, &db);
  if (rs == SQLITE_OK)
  {
    _sql_db = db;
    return 0;
  }
  _sql_on_error("%s", sqlite3_errmsg(db));
  sqlite3_close(db);
  return 1;
}

gint sql_close()
{
  gint rs;

  _sql_reset_error();
  /* unroll contexts up to toplevel one */
  while (sql_pop_context() == 0);
  /* cleanup toplevel context */
  _sql_context_fini_all_queries();
  rs = sqlite3_close(_sql_db);
  _sql_db = 0;
  if (rs != SQLITE_OK)
  {
    fprintf(stderr, "panic: database can't be properly closed! :-(\n");
    return 1;
  }
  return 0;
}

gint sql_exec(const gchar* sql, ...)
{
  va_list ap;
  gchar* tmp;
  gint rs;

  _sql_reset_error();
  va_start(ap, sql);
  tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  rs = sqlite3_exec(_sql_db,tmp,0,0,0);
  if (rs == SQLITE_OK)
  {
    sqlite3_free(tmp);
    return 0;
  }
  _sql_on_error("%s\nSQL: %s", sqlite3_errmsg(_sql_db), tmp);
  sqlite3_free(tmp);
  return 1;
}

sql_query* sql_prep(const gchar* sql, ...)
{
  sql_query* query;
  va_list ap;
  gchar* tmp;
  gint rs;

  _sql_reset_error();
  va_start(ap, sql);
  tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  rs = sqlite3_prepare(_sql_db, tmp, -1, &query, 0);
  if (rs == SQLITE_OK)
  {
    if (_sql_context_pushq(query))
    { /* queries array full */
      sqlite3_finalize(query);
      _sql_on_error("too many queries at once (SQL_OPEN_QUERIES_LIMIT is too low)\nSQL: %s", tmp);
      sqlite3_free(tmp);
      return 0;
    }
    sqlite3_free(tmp);
    return query;
  }
  _sql_on_error("%s\nSQL: %s", sqlite3_errmsg(_sql_db), tmp);
  sqlite3_free(tmp);
  return 0;
}

gint sql_fini(sql_query* query)
{
  gint rs;

  _sql_reset_error();
  if (_sql_context_popq(query))
  { /* query not found in the current context */
    _sql_on_error("trying to finalize query that was not prepared in the current context");
    return 1;
  }
  rs = sqlite3_finalize(query);
  if (rs == SQLITE_OK)
    return 0;
  _sql_on_error("%s", sqlite3_errmsg(_sql_db));
  return 1;
}

gint sql_rest(sql_query* query)
{
  gint rs;

  _sql_reset_error();
  rs = sqlite3_reset(query);
  if (rs == SQLITE_OK)
    return 0;
  _sql_on_error("%s", sqlite3_errmsg(_sql_db));
  return 1;
}

gint sql_step(sql_query* query)
{
  gint rs;

  _sql_reset_error();
  rs = sqlite3_step(query);
  if (rs == SQLITE_DONE)
    return 0;
  else if (rs == SQLITE_ROW)
    return 1;
  else if (rs == SQLITE_MISUSE)
    _sql_on_error("misused ;-)");
  else
    _sql_on_error("%s", sqlite3_errmsg(_sql_db));
  return -1;
}

gint64 sql_rowid()
{
  _sql_reset_error();
  return sqlite3_last_insert_rowid(_sql_db);
}

gint sql_get_int(sql_query* query, guint col)
{
  _sql_reset_error();
  if (col < sqlite3_data_count(query))
    return sqlite3_column_int(query, col);
  _sql_on_error("column out of range (%d)", col);
  return 0;
}

gchar* sql_get_text(sql_query* query, guint col)
{
  _sql_reset_error();
  if (col < sqlite3_data_count(query))
    return (gchar*)sqlite3_column_text(query, col);
  _sql_on_error("column out of range (%d)", col);
  return 0;
}

gint64 sql_get_int64(sql_query* query, guint col)
{
  _sql_reset_error();
  if (col < sqlite3_data_count(query))
    return sqlite3_column_int64(query, col);
  _sql_on_error("column out of range (%d)", col);
  return 0;
}

gboolean sql_get_null(sql_query* query, guint col)
{
  _sql_reset_error();
  if (col < sqlite3_data_count(query))
    return sqlite3_column_type(query, col) == SQLITE_NULL ? 1 : 0;
  _sql_on_error("column out of range (%d)", col);
  return 0;
}

gint sql_set_int(sql_query* query, gint par, gint val)
{
  _sql_reset_error();
  return sqlite3_bind_int(query, par, val);
}

gint sql_set_text(sql_query* query, gint par, const gchar* val)
{
  _sql_reset_error();
  return sqlite3_bind_text(query, par, val, -1, SQLITE_STATIC); /*XXX: TRANSIENT? */
}

gint sql_set_int64(sql_query* query, gint par, gint64 val)
{
  _sql_reset_error();
  return sqlite3_bind_int64(query, par, val);
}

gint sql_set_null(sql_query* query, gint par)
{
  _sql_reset_error();
  return sqlite3_bind_null(query, par);
}

gint sql_table_exist(const gchar* name, ...)
{
  sql_query* query;
  gint ret = 0;

  query = sql_prep("SELECT name FROM sqlite_master WHERE type == 'table' AND name == '%q';", name);
  if (sql_step(query))
    ret = 1;
  sql_fini(query);
  return ret;
}
