/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "sys.h"

#include "taction.h"

/* private
 ************************************************************************/

struct action {
  ta_type type;
  gchar* tmppath;
  gchar* path;
};

struct transaction {
  gboolean active;
  GSList* actions;
};

static struct transaction _ta_taction = {
  .active = 0,
  .actions = 0
};

/* public 
 ************************************************************************/

gint ta_initialize()
{
  sigset_t oldsig;

  sys_sigblock(&oldsig);

  if (_ta_taction.active)
    ta_rollback();

  sys_sigunblock(&oldsig);
  return 0;
}

gint ta_finalize()
{
  sigset_t oldsig;
  GSList* l;

  sys_sigblock(&oldsig);

  if (!_ta_taction.active)
    goto err;

  for (l=_ta_taction.actions; l!=0; l=l->next)
  {
    struct action* a = l->data;
    
  }

  sys_sigunblock(&oldsig);
  return 0;
 err:
  sys_sigunblock(&oldsig);
  return 1;
}

gint ta_rollback()
{
  sigset_t oldsig;
  GSList* l;

  sys_sigblock(&oldsig);
  if (!_ta_taction.active)
    goto err;

  for (l=_ta_taction.actions; l!=0; l=l->next)
  {
    struct action* a = l->data;
    
  }
  _ta_taction.active = 0;

  sys_sigunblock(&oldsig);
  return 0;
 err:
  sys_sigunblock(&oldsig);
  return 1;
}

gint ta_add_action(ta_type type, gchar* path1, gchar* path2)
{
  return 1;
}