// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ond�ej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <popt.h>

#include "config.h"

#include "commands.h"
#include "pkgdb.h"
#include "sigtrap.h"
#include "message.h"
#include "misc.h"

/* commands
 ************************************************************************/

static guint command = 0;
#define CMD_INSTALL (1<<0)
#define CMD_UPGRADE (1<<1)
#define CMD_REMOVE  (1<<2)
#define CMD_LIST    (1<<3)

static struct poptOption optsCommands[] = {
{
  "install", 'i', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_INSTALL, 
  "Install packages.", NULL
},
{
  "upgrade", 'u', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_UPGRADE,
  "Upgrade packages", NULL
},
{
  "remove", 'd', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_REMOVE,
  "Remove packages.", NULL
},
{
  "list", 'l', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_LIST,
  "List all packages. You can add package names to the command line "
  "to limit listed packages. This command supports glob matching.", NULL
},
POPT_TABLEEND
};

/* options
 ************************************************************************/

static gchar* default_bl_symopts[] = {
  "aaa_base", "aaa_glibc-solibs", "bin", "glibc-solibs", "glibc", NULL
};

static struct cmd_options cmd_opts = {
  .root = "/",
  .dryrun = 0,
  .verbosity = 2,
  .safe = 0,
  .no_optsyms = 0,
  .no_scripts = 0,
  .no_ldconfig = 0,
  .no_gtk_update_icon_cache = 0,
  .reinstall = 0,
  .bl_symopts = default_bl_symopts
};

static gint verbose = 0;
static gint quiet = 0;
static gint install_new = 0;

static struct poptOption optsOptions[] = {
{
  "root", 0, POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &cmd_opts.root, 0,
  "Set alternate root directory for package operations. You can also use "
  "ROOT environment variable.", "ROOT"
},
{
  "safe", 's', 0, &cmd_opts.safe, 0,
  "Play it safe. Don't replace existing files during --install or --upgrade. "
  "Don't run post-installation scripts. Don't remove changed files on "
  "--remove.", NULL
},
{
  "force", 'f', 0, &cmd_opts.force, 0,
  "Force installation even if package is already installed.",
  NULL
},
{
  "dry-run", 'n', 0, &cmd_opts.dryrun, 0,
  "Don't modify filesystem or database. This may be useful when used along "
  "with -v option to check what exactly would given command do.", NULL
},
{
  "verbose", 'v', 0, 0, 1,
  "Increase verbosity level. When used once, files and directories "
  "that are affected by particular operation will be shown. When used "
  "twice, everything that is done will be reported.", NULL
},
{
  "quiet", 'q', 0, 0, 2,
  "Decrease verbosity level. Default is to show info messages and warnings. "
  "This option disables warnings when used once. When used twice it will "
  "completely disable output except for error messages.", NULL
},
{
  "reinstall", 0, 0, &cmd_opts.reinstall, 0,
  "When upgrading package and package already exists in the database, "
  "force reinstall.", NULL
},
{
  "install-new", 0, 0, &install_new, 0,
  "When upgrading package that does not yet exist in the database, "
  "install it instead.", NULL
},
{
  "no-fast-symlinks", 0, 0, &cmd_opts.no_optsyms, 0,
  "Spkg by default parses doinst.sh for symlink creation code and removes "
  "it from the script. This improves execution times of doinst.sh. Use "
  "this option to disable such optimizations.", NULL
},
{
  "no-scripts", 0, 0, &cmd_opts.no_scripts, 0,
  "Disable postinstallation script.", NULL
},
{
  "no-ldconfig", 0, 0, &cmd_opts.no_ldconfig, 0,
  "Don't execute ldconfig after installation and upgrade.", NULL
},
{
  "no-gtk-update-icon-cache", 0, 0, &cmd_opts.no_gtk_update_icon_cache, 0,
  "Don't execute gtk-update-icon-cache after installation and upgrade "
  "even if a .desktop file is included in the package.", NULL
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
  "Display brief usage message.", NULL
},
{
  "help", 'h', POPT_ARG_NONE, &help, 0,
  "Show this help message.", NULL
},
{
  "version", 'V', POPT_ARG_NONE, &version, 0,
  "Display spkg version.", NULL
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

gboolean is_root()
{
#ifndef __WIN32__
  return getuid() == 0;
#else
  return 1;
#endif  
}

int main(const int ac, const char* av[])
{
  poptContext optCon=0;
  gint rc;
  gint status = 0;
  const gchar* arg;
  struct error* err;

#ifdef __DEBUG  
  g_mem_set_vtable(glib_mem_profiler_table);
#endif

  err = e_new();
  /* check if we have enough privileges */
#ifndef __WIN32__
  unsetenv("LD_LIBRARY_PATH");
#else
  putenv("LD_LIBRARY_PATH");
  putenv("LD_LIBRARY_PATH=");
#endif  

  /* load blacklists from SPKG_CONFDIR */
  gchar** bl_symopts = load_blacklist(SPKG_CONFDIR "/symopts_blacklist");
  if (bl_symopts)
    cmd_opts.bl_symopts = bl_symopts;

  /* preset ROOT */
  cmd_opts.root = getenv("ROOT");

  /* initialize popt context */
  optCon = poptGetContext("spkg", ac, av, opts, 0);
  poptSetOtherOptionHelp(optCon, "<command> [options] [packages...]");

  /* parse options */
  while ((rc = poptGetNextOpt(optCon)) != -1)
  {
    if (rc == 1)
      verbose++;
    else if (rc == 2)
      quiet++;
    if (rc < -1)
    {
      fprintf(stderr, "ERROR: Invalid argument: %s (%s)\n",
        poptStrerror(rc),
        poptBadOption(optCon, POPT_BADOPTION_NOALIAS));
      goto err_1;
    }
  }

  /* these are help handlers */
  if (help)
  {
    printf(
      PACKAGE_STRING "\n"
      "\n"
      "Written by Ondrej Jirman, 2005-2006.\n"
      "Maintained by George Vlahavas, 2012-2022\n"
      "\n"
      "This is free software. Not like a beer or like in a \"freedom\",\n"
      "but like in \"I don't care what you are going to do with it.\"\n"
      "\n"
    );
    poptPrintHelp(optCon, stdout, 0);
    printf(
      "\n"
      "Examples:\n"
      "  spkg -i <packages>     [--install]\n"
      "  spkg -u <packages>     [--upgrade]\n"
      "  spkg -vd <packages>    [--verbose --remove]\n"
      "  spkg -l kde*           [--list]\n"
      "  spkg -vnu <packages>   [--upgrade --verbose --dry-run]\n"
      "\n"
      "Official website: http://spkg.megous.com\n"
      "Bug reports can be sent to <vlahavas@gmail.com>.\n"
    );
    goto out;
  }
  if (usage)
  {
    printf("Usage: spkg [-i|-u|-d|-l] [--root=ROOT] [-n] [-s] [-q] [-v] [packages...]\n");
    goto out;
  }
  if (version)
  {
    printf("%s\n", PACKAGE_STRING);
    goto out;
  }

  /* check verbosity options */
  if (verbose && quiet)
  {
    fprintf(stderr, "ERROR: Verbose or quiet?\n");
    goto err_1;
  }
  cmd_opts.verbosity += verbose;
  cmd_opts.verbosity -= quiet;

  /* check command options */
  switch (command)
  {
    case CMD_INSTALL:
      if (!cmd_opts.dryrun && !is_root())
        goto err_noroot;
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
    break;
    case CMD_UPGRADE:
      if (!cmd_opts.dryrun && !is_root())
        goto err_noroot;
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
    break;
    case CMD_REMOVE:
      if (!cmd_opts.dryrun && !is_root())
        goto err_noroot;
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
    break;
    case CMD_LIST:
    break;
    case 0:
      if (poptPeekArg(optCon) == 0)
      {
        printf("Usage: spkg [-i|-u|-d|-l] [--root=ROOT] [-n] [-s] [-q] [-v] [packages...]\n");
        goto out;
      }
      if (!cmd_opts.dryrun && !is_root())
        goto err_noroot;
      command = CMD_UPGRADE;
      install_new = TRUE;
    break;
    default:
      fprintf(stderr, "ERROR: Schizofrenic command usage.\n");
      goto err_1;
  }

  /* init signal trap */
  if (sig_trap(err))
    goto err_2;

  /* open db */
  gboolean readonly = cmd_opts.dryrun || !is_root();
  if (db_open(cmd_opts.root, readonly, err))
    goto err_2;

  switch (command)
  {
    case CMD_INSTALL:
    {
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_install(arg, &cmd_opts, err))
        {
          if (e_errno(err) & CMD_EXIST)
          {
            gchar* pkgname = parse_pkgname(arg, 5);
            _inform("Skipping package %s (package with same base name is already installed)...", pkgname ? pkgname : arg);
            g_free(pkgname);
            e_clean(err);
          }
          else
          {
            e_print(err);
            e_clean(err);
            status = 2;
          }
        }
      }
    }
    break;
    case CMD_UPGRADE:
    {
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_upgrade(arg, &cmd_opts, err))
        {
          if (install_new && (e_errno(err) & CMD_NOTEX))
          {
            e_clean(err);
            if (cmd_install(arg, &cmd_opts, err))
            {
              e_print(err);
              e_clean(err);
              status = 2;
            }
          }
          else if (e_errno(err) & CMD_NOTEX)
          {
            gchar* pkgname = parse_pkgname(arg, 5);
            _inform("Skipping package %s (package with same base name is NOT installed)...", pkgname ? pkgname : arg);
            g_free(pkgname);
            e_clean(err);
          }
          else if (e_errno(err) & CMD_EXIST)
          {
            gchar* pkgname = parse_pkgname(arg, 5);
            _inform("Skipping package %s (already uptodate)...", pkgname ? pkgname : arg);
            g_free(pkgname);
            e_clean(err);
          }
          else
          {
            e_print(err);
            e_clean(err);
            status = 2;
          }
        }
      }
    }
    break;
    case CMD_REMOVE:
    {
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_remove(arg, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    }
    break;
    case CMD_LIST:
    {
      GSList* arglist = NULL;
      while ((arg = poptGetArg(optCon)) != 0)
        arglist = g_slist_append(arglist, g_strdup(arg));
      if (cmd_list(arglist, &cmd_opts, err))
      {
        e_print(err);
        e_clean(err);
        status = 2;
      }
      g_slist_foreach(arglist, (GFunc)g_free, 0);
      g_slist_free(arglist);
    }
    break;
  }

  db_close();

 out:
  poptFreeContext(optCon);
  e_free(err);

#ifdef __DEBUG  
  g_mem_profile();
#endif

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
  fprintf(stderr, "ERROR: No packages specified.\n");
  goto err_1;
 err_noroot:
  fprintf(stderr, "ERROR: You need root privileges to run this command. Try using --dry-run.\n");
  goto err_1;
}
