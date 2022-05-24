// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/

#include "cmd-common.h"

#define e_set(n, fmt, args...) e_add(e, "common", __func__, n, fmt, ##args)

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

gint _read_doinst_sh(struct untgz_state* tgz, struct db_pkg* pkg,
                   const gchar* root, const gchar* sane_path,
                   const struct cmd_options* opts, struct error* e,
                   const gboolean do_upgrade, struct db_pkg* ipkg)
{
  gint has_doinst = 0;
  gchar* fullpath = g_strdup_printf("%s%s", root, sane_path);

  /* optimization disabled, just extract doinst script */
  if (opts->no_optsyms || is_blacklisted(pkg->shortname, opts->bl_symopts))
  {
    _debug("Symlink optimization is disabled.");
    if (opts->safe)
    {
      _warning("In safe mode, install script is not executed,"
      " and with disabled optimized symlink creation, symlinks"
      " will not be created.%s", is_blacklisted(pkg->shortname, opts->bl_symopts) ?
      " Note that this package is blacklisted for optimized"
      " symlink creation. This may change in the future as"
      " better heuristics are developed for extracting symlink"
      " creation code from install script." : "");
    }
    else
    {
      if (!opts->dryrun)
      {
        if (untgz_write_file(tgz, fullpath))
        {
          e_set(E_ERROR, "Failed to extract file %s.", sane_path);
          goto err0;
        }
      }
      has_doinst = 1;
    }
    goto done;
  }

  /* EXIT: free(fullpath) */

  /* read doinst.sh into buffer */
  gchar* buf;
  gsize len;
  if (untgz_write_data(tgz, &buf, &len))
  {
    e_set(E_ERROR, "Failed to read post-installation script into buffer.");
    goto err0;
  }

  /* EXIT: free(fullpath), free(buf) */

  _debug("Extracting symlinks from the post-installation script...");
      
  /* optimize out symlinks creation from doinst.sh */
  GSList *doinst = NULL;
  gsize doinst_len = 0;
  gchar *b, *end, *ln = NULL, *n = buf, *sane_link_path = NULL,
        *link_target = NULL, *link_fullpath = NULL;
  while(iter_str_lines(&b, &end, &n, &ln))
  { /* for each line */
    gchar *link_dir, *link_name;

    /* EXIT: free(fullpath), free(buf), free(ln) */

    /* create link line */
    if (parse_createlink(ln, &link_dir, &link_name, &link_target))
    {
      struct stat ex_stat;
      gchar* link_path = g_strdup_printf("%s/%s", link_dir, link_name);
      sane_link_path = path_simplify(link_path);
      link_fullpath = g_strdup_printf("%s%s", root, sane_link_path);
      g_free(link_dir);
      g_free(link_name);
      g_free(link_path);
      
      /* EXIT: free(fullpath), free(buf), free(ln), free(link_target),
         free(sane_link_path), free(link_fullpath) */

      /* link path checks (be careful here) */
      if (_unsafe_path(sane_link_path))
      {
        e_set(E_ERROR, "Package contains symlink with unsafe path. (%s)", sane_link_path);
        goto parse_failed;
      }

      /* treat an install just as an upgrade where the file didn't exist */
      db_path_type orig_ptype = DB_PATH_NONE;
      if (do_upgrade)
      {
        orig_ptype = db_pkg_get_path(ipkg, sane_link_path);
      }

      _debug("Symlink detected: %s -> %s", sane_link_path, link_target);
      if (db_pkg_add_path(pkg, sane_link_path, DB_PATH_SYMLINK))
      {
        e_set(E_ERROR, "Can't add path to the package, it's too long. (%s)", sane_link_path);
        goto parse_failed;
      }
      sys_ftype ex_type = sys_file_type_stat(link_fullpath, 0, &ex_stat);

      if (ex_type == SYS_ERR)
      {
        e_set(E_ERROR, "Can't check path for symlink. (%s)", sane_link_path);
        goto parse_failed;
      }
      else if (ex_type == SYS_NONE)
      {
        ta_symlink_nothing(link_fullpath, g_strdup(link_target)); /* target is freed by db_free_pkg(), dup by taction finalize/rollback */
        link_fullpath = NULL;
      }
      else
      {
        /* If this path was not in the original package if upgrading.
           This is always going to be the case if installing. */
        if (orig_ptype == DB_PATH_NONE)
        {
          if (opts->safe)
          {
            e_set(E_ERROR, "Can't create symlink over existing %s. (%s)", ex_type == SYS_DIR ? "directory" : "file", sane_link_path);
            goto parse_failed;
          }
          _warning("%s exists, where symlink should be created. It will be removed. (%s)", ex_type == SYS_DIR ? "Directory" : "File", sane_link_path);
        }
        ta_forcesymlink_nothing(link_fullpath, g_strdup(link_target));
        link_fullpath = NULL;
      }

      g_free(link_fullpath);
      g_free(sane_link_path);
    }
    /* if this is not 'delete old file' line... */
    else if (!parse_cleanuplink(ln))
    {
      /* ...append it to doinst buffer */
      doinst = g_slist_prepend(doinst, ln);
      doinst_len += end - b + 1; /* line len + eol */
      ln = NULL; /* will be freed later */
    }

    g_free(ln);
  } /* iter_lines */

  /* EXIT: free(fullpath), free(buf), free(doinst) */

  /* we always store full doinst.sh in database (buf freed by db_free_pkg()) */
  pkg->doinst = buf;

  /* EXIT: free(fullpath), free(doinst) */

  /* write stripped down version of doinst.sh to a file */
  if (doinst != NULL)
  {
    _debug("Saving optimized post-installation script...");

    if (!opts->dryrun)
    {
      gchar* doinst_buf = g_malloc(doinst_len*2);
      gchar* doinst_buf_ptr = doinst_buf;
      doinst = g_slist_reverse(doinst);
      GSList* i;
      for (i = doinst; i != 0; i = i->next)
      {
        strcpy(doinst_buf_ptr, i->data);
        doinst_buf_ptr += strlen(i->data);
        *doinst_buf_ptr = '\n';
        doinst_buf_ptr++;
      }
      if (sys_write_buffer_to_file(fullpath, doinst_buf, doinst_len, e))
      {
        e_set(E_ERROR, "Can't write buffer to file %s.", sane_path);
        g_free(doinst_buf);
        g_slist_foreach(doinst, (GFunc)g_free, 0);
        g_slist_free(doinst);
        goto err0;
      }
      g_free(doinst_buf);
    }
    g_slist_foreach(doinst, (GFunc)g_free, 0);
    g_slist_free(doinst);
    has_doinst = 1;
  }

 done:  
 err0:
  g_free(fullpath);
  return has_doinst;
 parse_failed:
  g_free(link_target);
  g_free(link_fullpath);
  g_free(sane_link_path);
  g_free(ln);
  goto err0;
}

void _extract_file(struct untgz_state* tgz, struct db_pkg* pkg,
                   const gchar* sane_path, const gchar* root,
                   const struct cmd_options* opts, struct error* e,
                   gboolean do_upgrade, struct db_pkg* ipkg)
{
  gchar context[] = "install";
  if (do_upgrade)
    strcpy(context, "upgrade");
  /* following strings can be freed by the ta_* code, if so, you must zero
     variables after passing them to a ta_* function */
  gchar* fullpath = g_strdup_printf("%s%s", root, sane_path);
  gchar* temppath = g_strdup_printf("%s--###%s###", fullpath, context);

  /* EXIT: free(fullpath), free(temppath) */

  /* Here we must check interaction of following conditions:
   *
   * - type of the file we are installing (tgz->f_type)
   * - type of the file on the filesystem (existing)
   * - presence of the file in the file database (install)
   *   or original package (upgrade)
   *
   * And decide what to do:
   *
   * - install new file
   * - overwrite existing file
   * - leave existing file alone
   * - issue error and rollback installation
   * - issue notice or warning
   */

  /* EXIT: free(fullpath), free(temppath) */

  /* treat an install just as an upgrade where the file didn't exist */
  db_path_type orig_ptype = DB_PATH_NONE;
  if (do_upgrade)
    orig_ptype = db_pkg_get_path(ipkg, sane_path);

  /* get information about installed file from filesystem and filedb */
  struct stat ex_stat;
  sys_ftype ex_type = sys_file_type_stat(fullpath, 0, &ex_stat);
  sys_ftype ex_deref_type = SYS_NONE;
  if (ex_type == SYS_SYM)
    ex_deref_type = sys_file_type(fullpath, 1);

  if (ex_type == SYS_ERR)
  {
    e_set(E_ERROR, "Can't check path. (%s)", sane_path);
    goto extract_failed;
  }

  // installed file type
  switch (tgz->f_type)
  {
    case UNTGZ_DIR: /* we have directory */
    {
      if (ex_type == SYS_DIR)
      {
        /* installed directory already exist */
        if (_mode_differ(tgz, &ex_stat) || _gid_or_uid_differ(tgz, &ex_stat))
        {
          _notice("Directory already exists %s (but permissions differ)", sane_path);
          if (!opts->safe)
          {
            _notice("Permissions will be changed to owner=%d, group=%d, mode=%03o.", tgz->f_uid, tgz->f_gid, tgz->f_mode);
            ta_chperm_nothing(fullpath, tgz->f_mode, tgz->f_uid, tgz->f_gid);
            fullpath = NULL;
          }
          else
          {
            e_set(E_ERROR, "Can't change existing directory permissions in safe mode. (%s)", sane_path);
            goto extract_failed;
          }
        }
        else
        {
          _debug("Directory already exists %s", sane_path);
        }
      }
      else if (ex_type == SYS_SYM && ex_deref_type == SYS_DIR)
      {
        _warning("Directory already exists *behind the symlink* on filesystem. This may break upgrade/remove if you change that symlink in the future. (%s)", sane_path);
      }
      else if (ex_type == SYS_NONE)
      {
        _notice("Creating directory %s", sane_path);

        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;
        }

        ta_keep_remove(fullpath, 1);
        fullpath = NULL;
      }
      else /* create directory over file */
      {
        if (opts->safe)
        {
          e_set(E_ERROR, "Can't create directory over ordinary file. (%s)", sane_path);
          goto extract_failed;
        }
        _notice("Creating directory over ordinary file (%s)", sane_path);
        if (!opts->dryrun)
        {
          if (unlink(fullpath) < 0)
          {
            e_set(E_ERROR, "Couldn't remove file during upgrade. (%s)", sane_path);
            goto extract_failed;
          }
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;
        }

        ta_keep_remove(fullpath, 1);
        fullpath = NULL;
      }
    }
    break;
    case UNTGZ_SYM: /* we have a symlink */
    {
#ifdef LEGACY_CHECKS
      e_set(E_ERROR, "Symlink was found in the archive. (%s)", sane_path);
      goto extract_failed;
#else
      _debug("Symlink detected: %s -> %s", sane_path, tgz->f_link);
      if (ex_type == SYS_NONE)
      {
        ta_symlink_nothing(fullpath, g_strdup(tgz->f_link)); /* target is freed by db_free_pkg(), dup by taction finalize/rollback */
        fullpath = NULL;
      }
      else
      {
        if (opts->safe)
        {
          e_set(E_ERROR, "Can't create symlink over existing %s. (%s)", ex_type == SYS_DIR ? "directory" : "file", sane_path);
          goto extract_failed;
        }
        else
        {
          _warning("%s exists, where symlink should be created. It will be removed. (%s)", ex_type == SYS_DIR ? "Directory" : "File", sane_path);
          ta_forcesymlink_nothing(fullpath, g_strdup(tgz->f_link));
          fullpath = NULL;
        }
      }
#endif
    }
    break;
    case UNTGZ_LNK: /* we have a hardlink */
    {
      /* hardlinks are special beasts, most easy solution is to
       * postpone hardlink creation into transaction finalization phase
       */
      gchar* linkpath = g_strdup_printf("%s%s", root, tgz->f_link);

      /* check file we will be linking to (it should be a regular file) */
      sys_ftype tgt_type = sys_file_type(linkpath, 0);
      if (tgt_type == SYS_ERR)
      {
        e_set(E_ERROR, "Can't check hardlink target path. (%s)", linkpath);
        g_free(linkpath);
        goto extract_failed;
      }
      else if (tgt_type == SYS_REG)
      {
        _debug("Hardlink detected: %s -> %s", sane_path, tgz->f_link);

        /* when creating hardlink, nothing must exist on created path */
        if (ex_type == SYS_NONE)
        {
          ta_link_nothing(fullpath, linkpath); /* link path freed by ta_* */
          fullpath = NULL;
        }
        else
        {
          /* If this path was not in the original package if upgrading.
             This is always going to be the case if installing. */
          if (orig_ptype == DB_PATH_NONE)
          {
            if (opts->safe)
            {
              e_set(E_ERROR, "Can't create hardlink over existing %s. (%s)", ex_type == SYS_DIR ? "directory" : "file", sane_path);
              g_free(linkpath);
              goto extract_failed;
            }
            _warning("%s exists, where hardlink should be created. It will be removed. (%s)", ex_type == SYS_DIR ? "Directory" : "File", sane_path);
          }
          ta_forcelink_nothing(fullpath, linkpath);
          fullpath = NULL;
        }
      }
      else
      {
        if (!opts->dryrun)
        {
          e_set(E_ERROR, "Hardlink target is NOT a regular file. (%s)", linkpath);
          g_free(linkpath);
          goto extract_failed;
        }
        g_free(linkpath);
      }
    }
    break;
    case UNTGZ_NONE: /* nothing? tar is broken */
    {
      e_set(E_ERROR, "Broken tar archive.");
      goto extract_failed;
    }
    break;
    default: /* ordinary file */
    {
      _notice("Installing file %s", sane_path);

      if (ex_type == SYS_DIR)
      {
        /* target path is a directory, bad! */
        if (opts->safe) {
          e_set(E_ERROR, "Can't create file over existing directory. (%s)", sane_path);
          goto extract_failed;
        }

        _notice("Creating ordinary file over directory (%s)", sane_path);
        if (!opts->dryrun)
        {
          if (sys_rm_rf(fullpath) < 0)
          {
            e_set(E_ERROR, "Couldn't remove directory during upgrade. (%s)", sane_path);
            goto extract_failed;
          }
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;
        }

        ta_keep_remove(fullpath, 1);
        fullpath = NULL;
      }
      else if (ex_type == SYS_NONE)
      {
        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, fullpath))
            goto extract_failed;
        }

        ta_keep_remove(fullpath, 0);
        fullpath = NULL;
      }
      else /* file already exist there */
      {
        if (do_upgrade)
          _notice("Upgrading file %s", sane_path);
        /* If this path was not in the original package if upgrading.
           This is always going to be the case if installing. */
        if (orig_ptype == DB_PATH_NONE)
        {
          if (opts->safe)
          {
            if (do_upgrade)
              e_set(E_ERROR, "File already exist, but it was not installed by the upgraded package. (%s)", sane_path);
            else
              e_set(E_ERROR, "File already exist. (%s)", sane_path);
            goto extract_failed;
          }
          if (do_upgrade)
            _warning("File already exist, but it was not installed by the upgraded package. (%s)", sane_path);
        }
        if (!do_upgrade)
          _warning("File already exist %s (it will be replaced)", sane_path);
        sys_ftype tmp_type = sys_file_type(temppath, 0);
        if (tmp_type == SYS_ERR)
        {
          e_set(E_ERROR, "Can't check temporary file path. (%s)", temppath);
          goto extract_failed;
        }
        else if (tmp_type == SYS_DIR)
        {
          e_set(E_ERROR, "Temporary file path is used by a directory. (%s)", temppath);
          goto extract_failed;
        }
        else if (tmp_type != SYS_NONE)
        {
          _warning("Temporary file already exists, removing %s", temppath);
          if (!opts->dryrun)
          {
            if (unlink(temppath) < 0)
            {
              e_set(E_ERROR, "Can't remove temporary file %s. (%s)", temppath, strerror(errno));
              goto extract_failed;
            }
          }
        }

        if (!opts->dryrun)
        {
          if (untgz_write_file(tgz, temppath))
            goto extract_failed;
        }

        ta_move_remove(temppath, fullpath);
        fullpath = temppath = NULL;
      }
    }
    break;
  }

 done:
  g_free(temppath);
  g_free(fullpath);
  return;
 extract_failed:
  e_set(E_ERROR, "Failed to extract file %s.", sane_path);
  goto done;
}
