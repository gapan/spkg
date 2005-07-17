/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include "filedb.h"

extern unsigned int files_cnt;
extern char* files[];

int main()
{
  gint j,id,i;
  struct fdb_file f;
  struct fdb* db;
  struct error* err = e_new();
  
//  nice(-10);
//  unlink(".filedb/idx");
//  unlink(".filedb/pld");
  
  db = fdb_open(".filedb", err);
  if (!e_ok(err))
    goto err_0;

  i=0;
  f.link = 0;
  for (j=0; j<files_cnt; j++)
  {
    f.path = files[j];
    id = fdb_add_file(db, &f);
    if (!e_ok(err))
      goto err_0;
  }

  for (j=0; j<files_cnt; j++)
  {
    id = fdb_get_file_id(db,files[j]);
    if (!e_ok(err))
      goto err_0;
  }

  fdb_close(db);

 err_0:
  e_print(err);
  e_free(err);
  fdb_close(db);
  return 1;
}
