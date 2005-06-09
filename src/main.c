/*----------------------------------------------------------------------*\
|* spkg - Slackware Linux Fast Package Management Tools                 *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pkgtools.h"

/*XXX: remove (move to cli) */
extern gint verbose;
extern void notice(const gchar* f,...);
extern void err(gint e, const gchar* f,...);
extern void warn(const gchar* f,...);

gint verbose = 2;

void notice(const gchar* f,...)
{
  va_list ap;
  if (verbose < 2)
    return;
  printf("notice: ");
  va_start(ap, f);
  vprintf(f, ap);
  va_end(ap);
  fflush(stdout);
}

void err(gint e, const gchar* f,...)
{
  va_list ap;
  printf("error: ");
  va_start(ap, f);
  vprintf(f, ap);
  va_end(ap);
  fflush(stdout);
  if (e)
    exit(e);
}

void warn(const gchar* f,...)
{
  va_list ap;
  if (verbose < 1)
    return;
  printf("warning: ");
  va_start(ap, f);
  vprintf(f, ap);
  va_end(ap);
  fflush(stdout);
  exit(1);
}

typedef struct {
  /* command line opts */
  gboolean install;
  gboolean upgrade;
  gboolean remove;
  gboolean verbose;
  gboolean check;
  gboolean force;
  gchar*   root;
  gchar**  files;

} opts_t;
extern opts_t opts;

opts_t opts = {
  .install = 0,
  .upgrade = 0,
  .remove = 0,
  .root = 0,
  .verbose = 0,
  .check = 0,
  .force = 0,
  .files = 0,
};

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
  
  context = g_option_context_new("- Slackware Linux(TM) Package Management Tool v." SPKG_VERSION);
  g_option_context_add_main_entries(context, entries, 0);
  g_option_context_parse(context, &ac, &av, &error);

  f = opts.files;
  if (f == 0)
    return 0;

//  db_sync_from_legacydb();
//  db_sync_to_legacydb();
  
  while (*f != 0)
  {
//    pkg_install(*f,1,1);
    printf("%s\n", *f);
    f++;
  }

  return 0;
}
