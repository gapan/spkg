/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <popt.h>

#include "pkgtools.h"

/* commands
 ************************************************************************/

static guint command = 0;
#define CMD_INSTALL (1<<0)
#define CMD_UPGRADE (1<<1)
#define CMD_REMOVE  (1<<2)
#define CMD_SYNC    (1<<3)

static struct poptOption optsCommands[] = {
{
  "install", 'i', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_INSTALL, 
  "Install packages", NULL
},
{
  "upgrade", 'u', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_UPGRADE,
  "Upgrade packages", NULL
},
{
  "remove", 'd', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_REMOVE,
  "Remove packages", NULL
},
{
  "sync-cache", 's', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_SYNC,
  "Synchronize cache", NULL
},
POPT_TABLEEND
};

/* options
 ************************************************************************/

static gint verbose = 0;
static gint dryrun = 0;
static gchar* root = "/";
static gchar* mode = "normal";

static struct poptOption optsOptions[] = {
{
  "mode", 'm', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &mode, 0,
  "Set command mode of operation. This can be: paranoid (p), "
  "cautious (c), normal (n), brutal (b).", "MODE"
},
{
  "root", 'r', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &root, 0,
  "Set altrernate root directory for package operations.", "ROOT"
},
{
  "verbose", 'v', 0, &verbose, 0,
  "Be verbose about what is going on.", NULL
},
{
  "dryrun", 'n', 0, &dryrun, 0,
  "Don't modify filesystem. Good to use with -v option to show what "
  "will be done.", NULL
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
{
  NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsCommands, 0, "Commands:", NULL
},
{
  NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsOptions, 0, "Options:", NULL
},
{
  NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsHelp, 0, "Help options:", NULL
},
  POPT_TABLEEND
};

/* main
 ************************************************************************/

int main(const int ac, const char* av[])
{
  poptContext optCon;
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

  /* parse options */
  while ((rc = poptGetNextOpt(optCon)) != -1)
  {
    if (rc < -1)
    {
      fprintf(stderr, "error[main]: invalid argument: %s (%s)\n",
        poptStrerror(rc),
        poptBadOption(optCon, POPT_BADOPTION_NOALIAS));
      status = 1;
      goto out;
    }
  }

  /* these are help handlers */
  if (help)
  {
    printf(
      "spkg-" G_STRINGIFY(SPKG_VERSION) "\n"
      "Written by Ondøej Jirman, 2005\n"
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
    poptPrintUsage(optCon, stdout, 0);
    goto out;
  }
  if (version)
  {
    printf("spkg-" G_STRINGIFY(SPKG_VERSION) "\n");
    goto out;
  }

  switch (command)
  {
    case CMD_INSTALL:
      if (poptPeekArg(optCon) == 0)
      {
        fprintf(stderr, "error[main]: invalid argument: no packages given\n");
        status = 1;
        goto out;
      }
      while ((arg = poptGetArg(optCon)) != 0)
      {
      }
    break;
    case CMD_UPGRADE:
      if (poptPeekArg(optCon) == 0)
      {
        fprintf(stderr, "error[main]: invalid argument: no packages given\n");
        status = 1;
        goto out;
      }
      while ((arg = poptGetArg(optCon)) != 0)
      {
      }
    break;
    case CMD_REMOVE:
      if (poptPeekArg(optCon) == 0)
      {
        fprintf(stderr, "error[main]: invalid argument: no packages given\n");
        status = 1;
        goto out;
      }
      while ((arg = poptGetArg(optCon)) != 0)
      {
      }
    break;
    case CMD_SYNC:
      if (poptPeekArg(optCon) != 0)
      {
        fprintf(stderr, "error[main]: invalid argument: garbage on command line (%s...)\n", poptPeekArg(optCon));
        status = 1;
        goto out;
      }
    break;
    case 0:
      fprintf(stderr, "error[main]: invalid argument: no command given\n");
      status = 1;
      goto out;
    default:
      fprintf(stderr, "error[main]: invalid argument: schizofrenic command usage\n");
      status = 1;
      goto out;
  }

 out:
  e_free(err);
  optCon = poptFreeContext(optCon);
  /* 0 = all ok
   * 1 = command line error
   * 2 = package manager error
   */
  return status;
}
