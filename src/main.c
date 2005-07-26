/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <popt.h>

#include "pkgtools.h"
#include "pkgdb.h"
#include "sigtrap.h"

/* commands
 ************************************************************************/

static guint command = 0;
#define CMD_INSTALL (1<<0)
#define CMD_UPGRADE (1<<1)
#define CMD_REMOVE  (1<<2)
#define CMD_SYNC    (1<<3)
#define CMD_LIST    (1<<4)

static struct poptOption optsCommands[] = {
{
  "install", 'i', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_INSTALL, 
  "Install packages. ([p]aranoid|[c]autious|[n]ormal|[b]rutal)", NULL
},
#if 0
{
  "upgrade", 'u', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_UPGRADE,
  "Upgrade packages [unimplemented]", NULL
},
{
  "remove", 'd', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_REMOVE,
  "Remove packages [unimplemented]", NULL
},
{
  "list", 'l', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_LIST,
  "List packages. ([a]ll|[g]lob)", NULL
},
#endif
{
  "sync", 's', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_SYNC,
  "Synchronize databases. ([f]rom-legacy|[t]o-legacy)", NULL
},
POPT_TABLEEND
};

/* commands/modes definition
 ************************************************************************/

struct mode {
  pkg_mode mode;
  gchar* shortcut;
  gchar* longname;
};
struct cmd {
  gint cmd;
  pkg_mode default_mode;
  struct mode modes[16];
};
static struct cmd cmds[] = {
{
  CMD_INSTALL, PKG_MODE_NORMAL, {
    { PKG_MODE_PARANOID, "p", "paranoid" },
    { PKG_MODE_CAUTIOUS, "c", "cautious" },
    { PKG_MODE_NORMAL, "n", "normal" },
    { PKG_MODE_BRUTAL, "b", "brutal" },
    { 0 },
  }
},
{
  CMD_LIST, PKG_MODE_ALL, {
    { PKG_MODE_ALL, "a", "all" },
    { PKG_MODE_GLOB, "g", "glob" },
    { 0 },
  }
},
{
  CMD_SYNC, PKG_MODE_FROMLEGACY, {
    { PKG_MODE_FROMLEGACY, "f", "from-legacy" },
    { PKG_MODE_TOLEGACY, "t", "to-legacy" },
    { 0 },
  }
},
{ 0 }
};

/* options
 ************************************************************************/

static struct pkg_options pkg_opts = {
  .root = "/",
  .dryrun = 0,
  .verbosity = 1,
  .noptsym = 0,
  .nodoinst = 0,
};

static gchar* mode = 0;
static gint verbose = 0;
static gint quiet = 0;

static struct poptOption optsOptions[] = {
{
  "mode", 'm', POPT_ARG_STRING, &mode, 0,
  "Set command mode of operation. See particular command for available "
  "modes.", "MODE"
},
{
  "root", 'r', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &pkg_opts.root, 0,
  "Set altrernate root directory for package operations.", "ROOT"
},
{
  "verbose", 'v', 0, 0, 1,
  "Be verbose about what is going on.", NULL
},
{
  "quiet", 'q', 0, &quiet, 0,
  "Don't print warnings.", NULL
},
{
  "dry-run", 'n', 0, &pkg_opts.dryrun, 0,
  "Don't modify filesystem or database. Good to use this option with -v option "
  "to show what will be done.", NULL
},
{
  "no-fast-symlinks", '\0', 0, &pkg_opts.noptsym, 0,
  "Spkg by default parses doinst.sh for symlink creation code and removes "
  "it from the script. This improves execution times of doinst.sh. Use "
  "this option to disable such optimizations.", NULL
},
{
  "no-postinst", '\0', 0, &pkg_opts.nodoinst, 0,
  "Disable postinstallation script.", NULL
},
POPT_TABLEEND
};

/* help
 ************************************************************************/

static gint help = 0;
static gint usage = 0;
static gint version = 0;

static struct poptOption optsHelp[] = {
{
  "usage", '\0', POPT_ARG_NONE, &usage, 0,
  "Display brief usage message", NULL
},
{
  "help", 'h', POPT_ARG_NONE, &help, 0,
  "Show this help message", NULL
},
{
  "version", 'V', POPT_ARG_NONE, &version, 0,
  "Display spkg version", NULL
},
POPT_TABLEEND
};

/* main table
 ************************************************************************/

static struct poptOption opts[] = {
{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsCommands, 0, "Commands:", NULL },
{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsOptions, 0, "Options:", NULL },
{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsHelp, 0, "Help options:", NULL },
POPT_TABLEEND
};

/* main
 ************************************************************************/

int main(const int ac, const char* av[])
{
  poptContext optCon=0;
  gint rc;
  gint status = 0;
  const gchar* arg;
  struct error* err = e_new();

  /* check if we have enough privileges */
  if (getuid() != 0)
  {
    fprintf(stderr, "You need root privileges to run this program. Sorry.\n");
    exit(1);
  }

  /* initialize popt context */
  optCon = poptGetContext("spkg", ac, av, opts, 0);
  poptSetOtherOptionHelp(optCon, "<command> [options] [packages...]");

  /* parse options */
  while ((rc = poptGetNextOpt(optCon)) != -1)
  {
    if (rc == 1)
      verbose++;
    if (rc < -1)
    {
      fprintf(stderr, "error[main]: invalid argument: %s (%s)\n",
        poptStrerror(rc),
        poptBadOption(optCon, POPT_BADOPTION_NOALIAS));
      goto err_1;
    }
  }

  /* these are help handlers */
  if (help)
  {
    printf(
      "spkg-" G_STRINGIFY(SPKG_VERSION) "\n"
      "\n"
      "Written by Ondrej Jirman, 2005\n"
      "\n"
      "This is free software. Not like beer or like in \"freedom\",\n"
      "but like in \"I don't care what are you going to do with it.\"\n"
      "\n"
    );
    poptPrintHelp(optCon, stdout, 0);
    printf(
      "\n"
      "Examples:\n"
      "  spkg -imb <packages>   [--install --mode=brutal]\n"
      "  spkg -ump <packages>   [--upgrade --mode=paranoid]\n"
      "  spkg -vr <packages>    [--verbose --remove]\n"
      "  spkg -vnumb <packages> [--upgrade --verbose --dry-run --mode=cautious]\n"
      "  spkg -s                [--sync-cache]\n"
      "\n"
      "Official website: http://spkg.megous.com\n"
      "Bug reports can be sent to <megous@megous.com>.\n"
    );
    goto out;
  }
  if (usage)
  {
    poptSetOtherOptionHelp(optCon, "");
    poptPrintUsage(optCon, stdout, 0);
    goto out;
  }
  if (version)
  {
    printf("spkg-" G_STRINGIFY(SPKG_VERSION) "\n");
    goto out;
  }

  /* got command? */
  if (command == 0)
  {
    fprintf(stderr, "error[main]: invalid argument: no command given\n");
    goto err_1;
  }

  /* get mode for command */
  struct cmd* c = cmds;
  while (c->cmd) /* for each command */
  {
    if (c->cmd == command)
    {
      /* command found */
      pkg_opts.mode = c->default_mode;
      if (mode == 0) /* no mode specified on command line */
        goto mode_ok;
      struct mode* m = c->modes;
      while (m->shortcut) /* for each mode */
      {
        if (!strcmp(m->shortcut, mode) || !strcmp(m->longname, mode))
        {
          pkg_opts.mode = m->mode;
          goto mode_ok;
        }
        m++;
      }
      goto no_mode;
    }
    c++;
  }
  /* command not found in a table (because it is incomplete!) */
  g_assert_not_reached();
 no_mode:
  /* mode not found in a table */
  fprintf(stderr, "error[main]: invalid argument: unknown mode (%s)\n", mode);
  goto err_1;
 mode_ok:

  /* check verbosity options */
  if (verbose && quiet)
  {
    fprintf(stderr, "error[main]: invalid argument: verbose or quiet?\n");
    goto err_1;
  }
  if (verbose)
    pkg_opts.verbosity = verbose+1;
  if (quiet)
    pkg_opts.verbosity = 0;

  /* init signal trap */
  if (sig_trap(err))
    goto err_2;

  /* open db */
  if (db_open(pkg_opts.root, err))
    goto err_2;

  switch (command)
  {
    case CMD_INSTALL:
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (pkg_install(arg, &pkg_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
    case CMD_UPGRADE:
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (pkg_upgrade(arg, &pkg_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
    case CMD_REMOVE:
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (pkg_remove(arg, &pkg_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
    case CMD_SYNC:
      if (poptPeekArg(optCon) != 0)
        goto err_garbage;
      if (pkg_sync(&pkg_opts, err))
        goto err_2;
    break;
    default:
      fprintf(stderr, "error[main]: invalid argument: schizofrenic command usage\n");
      goto err_1;
  }

  db_close();

 out:
  optCon = poptFreeContext(optCon);
  e_free(err);
  /* 0 = all ok
   * 1 = command line error
   * 2 = package manager error
   */
  return status;
 err_1:
  status = 1;
  goto out;
 err_2:
  status = 2;
  e_print(err);
  goto out;
 err_nopackages:
  fprintf(stderr, "error[main]: invalid argument: no packages given\n");
  goto err_1;
 err_garbage:
  fprintf(stderr, "error[main]: invalid argument: garbage on command line (%s...)\n", poptPeekArg(optCon));
  goto err_1;
}
