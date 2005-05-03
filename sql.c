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

/* queries array */
static sql_query* _sql_open_queries[SQL_OPEN_QUERIES_LIMIT] = {0};

static gint _sql_pushq(sql_query* q)
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
    if (_sql_open_queries[i] == 0)
    {
      _sql_open_queries[i] = q;
      return 0;
    }
  return 1;
}

static gint _sql_popq(sql_query* q)
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
    if (_sql_open_queries[i] == q)
    {
      _sql_open_queries[i] = 0;
      return 0;
    }
  return 1;
}

static __inline__ void _sql_reset_errstr()
{
  /* just an optimization */
  if (G_UNLIKELY(sql_errstr != 0))
  {
    g_free(sql_errstr);
    sql_errstr = 0;
  }
}

/* close all open queries */
static gint _sql_fini_all()
{
  guint i;
  for (i=0; i<SQL_OPEN_QUERIES_LIMIT; i++)
    if (_sql_open_queries[i] != 0)
    {
      sqlite3_finalize(_sql_open_queries[i]); /*XXX: err check?*/
    }
  return 0;
}

#define sql_on_error(fmt, args...) _sql_on_error(__func__, fmt, ##args)
static void _sql_on_error(const gchar* func, const gchar* fmt, ...)
{
  va_list ap;
  gchar* errhead;
  gchar* errtail;
  
  /* store error message */
  errhead = g_strdup_printf("error[%s]: ", func);
  va_start(ap, fmt);
  errtail = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  g_free(sql_errstr);
  sql_errstr = g_strconcat(errhead, errtail, 0);
  g_free(errhead);
  g_free(errtail);

  switch (sql_erract)
  {
    case SQL_ERRJUMP:
      longjmp(sql_errjmp,1);
    default:
    case SQL_ERREXIT:
      fprintf(stderr, "%s\n", sql_errstr);
      sql_close();
      exit(1);
    case SQL_ERRINFORM:
      fprintf(stderr, "%s\n", sql_errstr);
    break;
    case SQL_ERRIGNORE:
    break;
  }
}

/* public 
 ************************************************************************/

sql* sql_db=0;
gchar* sql_errstr=0;
jmp_buf sql_errjmp;
sql_error_action sql_erract = SQL_ERREXIT;

sql* sql_open(const gchar* file)
{
  gint rs;
  sql* db=0;

  _sql_reset_errstr();
  rs = sqlite3_open(file, &db);
  if (rs == SQLITE_OK)
  {
    if (sql_db == 0)
      sql_db = db;
    return db;
  }
  sql_on_error("%s", sqlite3_errmsg(db));
  sqlite3_close(db);
  return 0;
}

gint sql_close()
{
  gint rs;

  _sql_reset_errstr();
  _sql_fini_all();
  rs = sqlite3_close(sql_db);
  sql_db = 0;
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

  _sql_reset_errstr();
  va_start(ap, sql);
  tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  rs = sqlite3_exec(sql_db,tmp,0,0,0);
  if (rs == SQLITE_OK)
  {
    sqlite3_free(tmp);
    return 0;
  }
  sql_on_error("%s\nSQL: %s", sqlite3_errmsg(sql_db), tmp);
  sqlite3_free(tmp);
  return 1;
}

sql_query* sql_prep(const gchar* sql, ...)
{
  sql_query* s;
  va_list ap;
  gchar* tmp;
  gint rs;

  _sql_reset_errstr();
  va_start(ap, sql);
  tmp = sqlite3_vmprintf(sql, ap);
  va_end(ap);
  rs = sqlite3_prepare(sql_db, tmp, -1, &s, 0);
  if (rs == SQLITE_OK)
  {
    _sql_pushq(s); /*XXX: check for unlikely owerflow */
    sqlite3_free(tmp);
    return s;
  }
  sql_on_error("%s\nSQL: %s", sqlite3_errmsg(sql_db), tmp);
  sqlite3_free(tmp);
  return 0;
}

gint sql_fini(sql_query* s)
{
  gint rs;

  _sql_reset_errstr();
  _sql_popq(s);
  rs = sqlite3_finalize(s);
  if (rs == SQLITE_OK)
    return 0;
  sql_on_error("%s", sqlite3_errmsg(sql_db));
  return 1;
}

gint sql_rest(sql_query* s)
{
  gint rs;

  _sql_reset_errstr();
  rs = sqlite3_reset(s);
  if (rs == SQLITE_OK)
    return 0;
  sql_on_error("%s", sqlite3_errmsg(sql_db));
  return 1;
}

gint sql_step(sql_query* s)
{
  gint rs;

  _sql_reset_errstr();
  rs = sqlite3_step(s);
  if (rs == SQLITE_DONE)
    return 0;
  else if (rs == SQLITE_ROW)
    return 1;
  else if (rs == SQLITE_MISUSE)
    sql_on_error("misused ;-)");
  else
    sql_on_error("%s", sqlite3_errmsg(sql_db));
  return -1;
}

gint64 sql_rowid()
{
  _sql_reset_errstr();
  return sqlite3_last_insert_rowid(sql_db);
}

gint sql_get_int(sql_query* q, guint c)
{
  _sql_reset_errstr();
  if (c < sqlite3_data_count(q))
    return sqlite3_column_int(q, c);
  sql_on_error("column out of range (%d)", c);
  return 0;
}

const guchar* sql_get_text(sql_query* q, guint c)
{
  _sql_reset_errstr();
  if (c < sqlite3_data_count(q))
    return sqlite3_column_text(q, c);
  sql_on_error("column out of range (%d)", c);
  return 0;
}

gint64 sql_get_int64(sql_query* q, guint c)
{
  _sql_reset_errstr();
  if (c < sqlite3_data_count(q))
    return sqlite3_column_int64(q, c);
  sql_on_error("column out of range (%d)", c);
  return 0;
}

gint sql_set_int(sql_query* q, gint c, gint v)
{
  _sql_reset_errstr();
  return sqlite3_bind_int(q, c, v);
}

gint sql_set_text(sql_query* q, gint c, const gchar* v)
{
  _sql_reset_errstr();
  return sqlite3_bind_text(q, c, v, -1, SQLITE_STATIC); /*XXX: TRANSIENT? */
}

gint sql_set_int64(sql_query* q, gint c, gint64 v)
{
  _sql_reset_errstr();
  return sqlite3_bind_int64(q, c, v);
}

gint sql_set_null(sql_query* q, gint c)
{
  _sql_reset_errstr();
  return sqlite3_bind_null(q, c);
}

gint sql_table_exist(const gchar* name)
{
  sql_query* q;
  gint ret = 0;

  _sql_reset_errstr();
  q = sql_prep("SELECT name FROM sqlite_master WHERE type == 'table' AND name == '%q';", name);
  if (sql_step(q))
    ret = 1;
  sql_fini(q);
  return ret;
}
