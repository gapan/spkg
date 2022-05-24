// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/

#include "cmd-common.h"

/* packages that can't be optimized, until they are fixed */

/* Check for libc symlinks to libraries that are critical to running sh. If
 * these symlinks are removed, doinst fails to run, because sh (bash) fails to
 * run. Since these are recreated with ldconfig when the glibc-solibs package
 * is installed, it's same to leave them where they are
 */
gboolean _check_libc_libs(const gchar* path)
{
  if (strncmp(path, "lib/ld-linux.so", strlen("lib/ld-linux.so")) == 0 ||
    strncmp(path, "lib64/ld-linux-x86-64.so", strlen("lib64/ld-linux-x86-64.so")) == 0 ||
    strncmp(path, "lib/libc.so", strlen("lib/libc.so")) == 0 ||
    strncmp(path, "lib64/libc.so", strlen("lib64/libc.so")) == 0 ||
    strncmp(path, "lib/libdl.so", strlen("lib/libdl.so")) == 0 ||
    strncmp(path, "lib64/libdl.so", strlen("lib64/libdl.so")) == 0)
  {
    return 0;
  }
  return 1;
}

void _run_ldconfig(const gchar* root, const struct cmd_options* opts)
{
  if (access("/sbin/ldconfig", X_OK) == 0)
  {
    gchar* ldconf_file = g_strdup_printf("%setc/ld.so.conf", root);
    if (access(ldconf_file, R_OK) == 0)
    {
      gchar* qroot = g_shell_quote(root);
      gchar* cmd = g_strdup_printf("/sbin/ldconfig -r %s", qroot);
      g_free(qroot);

      _notice("Running /sbin/ldconfig...");
      if (!opts->dryrun)
      {
        gint rv = system(cmd);
        if (rv < 0)
          _warning("Can't execute /sbin/ldconfig. (%s)", strerror(errno));
        else if (rv > 0)
          _warning("Program /sbin/ldconfig failed. (%d)", rv);
      }

      g_free(cmd);
    }
    g_free(ldconf_file);
  }
  else
  {
    _warning("Program /sbin/ldconfig was not found on the system.");
  }
}

void _gtk_update_icon_cache(const gchar* root, const struct cmd_options* opts)
{
  if (access("/usr/bin/gtk-update-icon-cache", X_OK) == 0)
  {
    gchar* cmd = g_strdup_printf("/usr/bin/gtk-update-icon-cache %susr/share/icons/hicolor > /dev/null 2>&1", root);

    _notice("Running /usr/bin/gtk-update-icon-cache...");
    if (!opts->dryrun)
    {
      gint rv = system(cmd);
      if (rv < 0)
        _warning("Can't execute /usr/bin/gtk-update-icon-cache. (%s)", strerror(errno));
      else if (rv > 0)
        _warning("Program /usr/bin/gtk-update-icon-cache failed. (%d)", rv);
    }
  }
  else
  {
    _warning("Program /usr/bin/gtk-update-icon-cache was not found on the system.");
  }
}

gboolean _unsafe_path(const gchar* path)
{
  if (g_path_is_absolute(path))
    return TRUE;
  else if (!strncmp(path, "../", 3))
    return TRUE;
  return FALSE;
}

void _read_slackdesc(struct untgz_state* tgz, struct db_pkg* pkg)
{
  gchar *buf = NULL, *desc[MAX_SLACKDESC_LINES] = {0};
  gsize len;
  untgz_write_data(tgz, &buf, &len);
  parse_slackdesc(buf, pkg->shortname, desc);
  pkg->desc = gen_slackdesc(pkg->shortname, desc);
  g_free(buf);

  /* free description */
  gint i;
  for (i=0;i<MAX_SLACKDESC_LINES;i++)
  {
    _inform("| %s", desc[i] ? desc[i] : "");
    g_free(desc[i]);
  }
}

