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
#include "untgz.h"
#include "pkgtools.h"
#include "pkgdb.h"

static GOptionEntry entries[] = 
{
  { "install", 'i', 0, G_OPTION_ARG_NONE,   &opts.install, "Install packages", NULL },
  { "upgrade", 'u', 0, G_OPTION_ARG_NONE,   &opts.upgrade, "Upgrade packages", NULL },
  { "remove",  'd', 0, G_OPTION_ARG_NONE,   &opts.remove,  "Remove packages", NULL },
  { "root",    'r', 0, G_OPTION_ARG_STRING, &opts.root,    "Set different root directory ", "ROOT" },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE,   &opts.verbose, "Be verbose", NULL },
  { "check",   'c', 0, G_OPTION_ARG_NONE,   &opts.check,   "Don't modify anything, just check", NULL },
  { "force",   'f', 0, G_OPTION_ARG_NONE,   &opts.force,   "Force installation or upgrade if package already exist", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &opts.files, NULL, NULL },
  { NULL }
};

int main(int ac, char* av[])
{
  GError *error = NULL;
  GOptionContext* context;
  gchar** f;

  if (getuid() != 0)
  {
    fprintf(stderr, "You need to run this program with root privileges.\n");
    exit(1);
  }
  
  context = g_option_context_new("- Slackware Linux(TM) Package Management Tool");
  g_option_context_add_main_entries(context, entries, 0);
  g_option_context_parse(context, &ac, &av, &error);

  f = opts.files;
  if (f == 0)
    return 0;

  if (db_open(""))
    return 1;

//  db_sync_legacydb_to_fastpkgdb();
//  db_sync_fastpkgdb_to_legacydb();
  
  while (*f != 0)
  {
    installpkg(*f);
//    printf("%s\n", *f);
    f++;
  }

  db_close();
  return 0;
}
