/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sql.h"

#include "bench.h"

//#define trace() do { printf("trace[sql]: %s\n", __func__); } while(0)
#define trace() do { } while (0)

/* private data
 ************************************************************************/

/* database and error string */
static sql* _sql_db=0;
static gchar* _sql_errstr=0;

struct sql_context {
  jmp_buf errjmp;
  gboolean transact;
  sql_erract erract;
  sql_query* queries[SQL_OPEN_QUERIES_LIMIT];
};

/* context stack */
static gint _sql_context_stack_pos = 0;
static struct sql_context _sql_context_stack[SQL_CONTEXT_STACK_LIMIT];

/* current context */
static sql_query* _sql_context_queries[SQL_OPEN_QUERIES_LIMIT] = {0};
static sql_erract _sql_context_erract = SQL_ERREXIT;
static gboolean _sql_context_transact = 0;

static gboolean _sql_transaction = 0;

/* private funcs
 ************************************************************************/

/* generic error processing function */
#define _sql_on_error(fmt, args...) __sql_on_error(__func__, fmt, ##args)
static void __sql_on_error(const gchar* func, const gchar* fmt, ...)
{
  va_list ap;
  gchar* errhead;
  gchar* errtail;
  trace();  

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
    case SQL_ERRIGNORE:
    break;
  }
}

/* clean error */
static __inline__ void _sql_reset_error()
{
  if (G_UNLIKELY(_sql_errstr != 0))
  {
    g_free(_sql_errstr);
    _sql_errstr = 0;
  }
}

/* store query on the stack */
static __inline__ gint _sql_context_pushq(sql_query* query)
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
  {
    if (_sql_context_queries[i] == 0)
    {
      _sql_context_queries[i] = query;
      return 0;
    }
  }
  return 1;
}

/* remove query from the stack */
static __inline__ gint _sql_context_popq(sql_query* query)
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
  {
    if (_sql_context_queries[i] == query)
    {
      _sql_context_queries[i] = 0;
      return 0;
    }
  }
  return 1;
}

/* close all queries in the current context */
static __inline__ void _sql_context_fini_all_queries()
{
  gint i, rs;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
  {
    if (_sql_context_queries[i] != 0)
    {
      rs = sqlite3_finalize(_sql_context_queries[i]);
      _sql_context_queries[i] = 0;
      g_assert(rs == SQLITE_OK);
    }
  }
}

/* public 
 ************************************************************************/

/* where to jump on error */
jmp_buf sql_errjmp;

gint sql_open(const gchar* file)
{
  trace();  

  reset_timers();
  continue_timer(0);
  g_assert(_sql_db == 0);
  g_assert(file != 0);

  _sql_reset_error();

  sql* db=0;
  gint rs = sqlite3_open(file, &db);
  if (rs == SQLITE_OK)
  {
    _sql_db = db;
    stop_timer(0);
    return 0;
  }
  sqlite3_close(db);
  stop_timer(0);
  return 1;
}

void sql_close()
{
  trace();  

  continue_timer(1);
  g_assert(_sql_db != 0);

  _sql_reset_error();

  /* unroll contexts up to toplevel one */
  while (_sql_context_stack_pos)
    sql_pop_context(0);
  /* cleanup toplevel context */
  _sql_context_fini_all_queries();

  gint rs = sqlite3_close(_sql_db);
  g_assert(rs == SQLITE_OK);
  _sql_db = 0;
  stop_timer(1);

  print_timer(0, "[sql] sql_open");
  print_timer(1, "[sql] sql_close");
}

gchar* sql_error()
{
  return _sql_errstr;
}

void sql_transaction_begin()
{
  trace();  
  
  g_assert(_sql_db != 0);
  g_assert(!_sql_transaction);

  gint rs = sqlite3_exec(_sql_db,"BEGIN EXCLUSIVE TRANSACTION;",0,0,0);
  g_assert(rs == SQLITE_OK);

  _sql_transaction = 1;
  _sql_context_transact = 1;
}

void sql_transaction_end(gboolean commit)
{
  trace();  

  g_assert(_sql_db != 0);
  g_assert(_sql_transaction);

  gint rs;
  if (commit)
    rs = sqlite3_exec(_sql_db,"COMMIT TRANSACTION;",0,0,0);
  else
    rs = sqlite3_exec(_sql_db,"ROLLBACK TRANSACTION;",0,0,0);
  g_assert(rs == SQLITE_OK);

  _sql_transaction = 0;
  _sql_context_transact = 0;
}

void sql_push_context(sql_erract act, gboolean begin)
{
  trace();

  g_assert(_sql_db != 0);
  g_assert(_sql_context_stack_pos < SQL_CONTEXT_STACK_LIMIT);

  _sql_context_stack[_sql_context_stack_pos].transact = _sql_context_transact;
  _sql_context_stack[_sql_context_stack_pos].erract = _sql_context_erract;
  _sql_context_stack[_sql_context_stack_pos].errjmp[0] = sql_errjmp[0];
  memcpy(_sql_context_stack[_sql_context_stack_pos].queries, _sql_context_queries, sizeof(_sql_context_queries));
  memset(_sql_context_queries, 0, sizeof(_sql_context_queries));
  _sql_context_stack_pos++;
  _sql_context_erract = act;
  _sql_context_transact = 0;

  /* begin transaction in new context */
  if (begin)
    sql_transaction_begin();
}

void sql_pop_context(gboolean commit)
{
  trace();  

  g_assert(_sql_db != 0);
  g_assert(_sql_context_stack_pos > 0);

  /* if transaction was executed in contex that is being popped, end it */
  _sql_context_fini_all_queries();

  if (_sql_context_transact)
    sql_transaction_end(commit);

  _sql_context_stack_pos--;
  _sql_context_erract = _sql_context_stack[_sql_context_stack_pos].erract;
  _sql_context_transact = _sql_context_stack[_sql_context_stack_pos].transact;
  sql_errjmp[0] = _sql_context_stack[_sql_context_stack_pos].errjmp[0];
  memcpy(_sql_context_queries, _sql_context_stack[_sql_context_stack_pos].queries, sizeof(_sql_context_queries));
}

gint sql_exec(const gchar* sql, ...)
{
  va_list ap;
  gchar* tmp;
  gint rs;

  g_assert(_sql_db != 0);
  g_assert(sql != 0);

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

  g_assert(_sql_db != 0);
  g_assert(sql != 0);

  _sql_reset_error();
  va_start(ap, sql);
  gchar* tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  gint rs = sqlite3_prepare(_sql_db, tmp, -1, &query, 0);
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
  g_assert(_sql_db != 0);
  g_assert(query != 0);

  _sql_reset_error();
  if (_sql_context_popq(query))
  { /* query not found in the current context */
    _sql_on_error("trying to finalize query that was not prepared in the current context");
    return 1;
  }
  gint rs = sqlite3_finalize(query);
  if (rs == SQLITE_OK)
    return 0;
  _sql_on_error("%s", sqlite3_errmsg(_sql_db));
  return 1;
}

gint sql_rest(sql_query* query)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);

  _sql_reset_error();
  gint rs = sqlite3_reset(query);
  if (rs == SQLITE_OK)
    return 0;
  _sql_on_error("%s", sqlite3_errmsg(_sql_db));
  return 1;
}

gint sql_step(sql_query* query)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);

  _sql_reset_error();
  gint rs = sqlite3_step(query);
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
  g_assert(_sql_db != 0);
  return sqlite3_last_insert_rowid(_sql_db);
}

gint sql_get_int(sql_query* query, guint col)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(col >= 0 && col < sqlite3_data_count(query));
  return sqlite3_column_int(query, col);
}

const void* sql_get_blob(sql_query* query, guint col)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(col >= 0 && col < sqlite3_data_count(query));
  return sqlite3_column_blob(query, col);
}

gchar* sql_get_text(sql_query* query, guint col)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(col >= 0 && col < sqlite3_data_count(query));
  return (gchar*)sqlite3_column_text(query, col);
}

guint sql_get_size(sql_query* query, guint col)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(col >= 0 && col < sqlite3_data_count(query));
  return (guint)sqlite3_column_bytes(query, col);
}

gint64 sql_get_int64(sql_query* query, guint col)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(col >= 0 && col < sqlite3_data_count(query));
  return sqlite3_column_int64(query, col);
}

gboolean sql_get_null(sql_query* query, guint col)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(col >= 0 && col < sqlite3_data_count(query));
  return sqlite3_column_type(query, col) == SQLITE_NULL ? 1 : 0;
}

void sql_set_int(sql_query* query, gint par, gint val)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(par >= 1 && par <= sqlite3_bind_parameter_count(query));
  sqlite3_bind_int(query, par, val);
}

void sql_set_text(sql_query* query, gint par, const gchar* val)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(par >= 1 && par <= sqlite3_bind_parameter_count(query));
  sqlite3_bind_text(query, par, val, -1, SQLITE_STATIC);
}

void sql_set_blob(sql_query* query, gint par, void* val, guint len)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(par >= 1 && par <= sqlite3_bind_parameter_count(query));
  sqlite3_bind_blob(query, par, val, len, SQLITE_STATIC);
}

void sql_set_int64(sql_query* query, gint par, gint64 val)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(par >= 1 && par <= sqlite3_bind_parameter_count(query));
  sqlite3_bind_int64(query, par, val);
}

void sql_set_null(sql_query* query, gint par)
{
  g_assert(_sql_db != 0);
  g_assert(query != 0);
  g_assert(par >= 1 && par <= sqlite3_bind_parameter_count(query));
  sqlite3_bind_null(query, par);
}

gint sql_table_exist(const gchar* name)
{
  g_assert(name != 0);

  gint ret = 0;
  sql_query* query = sql_prep("SELECT name FROM sqlite_master WHERE type == 'table' AND name == '%q';", name);
  if (sql_step(query))
    ret = 1;
  sql_fini(query);
  return ret;
}
