// vim:et:sta:sts=2:sw=2:ts=2:tw=79:
/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <string.h>

#include "path.h"
#include "misc.h"

/* public - high quality path manipulation functions
 ************************************************************************/

gchar* path_sanitize_slashes(const gchar* path)
{
  g_assert(path != 0);
  gchar* sanepath = (gchar*)g_malloc(strlen(path)+1);
  gchar* tmp = sanepath;
  gboolean previous_was_slash = 0;
  while (*path != '\0')
  {
    if (*path != '/' || !previous_was_slash)
      *(tmp++) = *path;
    previous_was_slash = *path == '/'?1:0;
    path++;
  }
  *tmp = '\0';
  if (tmp > sanepath+1 && *(tmp-1) == '/')
    *(tmp-1) = '\0';
  return sanepath;
}

gchar** path_get_elements(const gchar* path)
{
  gchar* sane_path = path_sanitize_slashes(path); /* always succeeds */
  gchar** pathv = g_strsplit(sane_path, "/", 0);
  g_free(sane_path);
  return pathv;
}

/* path optimization function */
gchar* path_simplify(const gchar* path)
{
  gchar **pathv, **sane_pathv;
  guint i, j = 0, pathv_len, subroot = 0;
  gboolean absolute;
  
  pathv = path_get_elements(path); /* should free */
  pathv_len = g_strv_length_compat(pathv);
  
  sane_pathv = (gchar**)g_malloc0((pathv_len + 1) * sizeof(gchar*));
  absolute = (pathv_len > 1 && **pathv == '\0');
  
  for (i = 0; i < pathv_len; i++)
  {
    if (!strcmp(pathv[i], "."))
      continue; /* ignore curdirs in path */
    else if (!strcmp(pathv[i], ".."))
    {
      if (absolute)
      {
        if (j > 1)
        {
          j--;
        }
      }
      else
      {
        if (subroot && !strcmp(sane_pathv[j - 1], "..")) /* if we are off base and last item is .. */
        {
          sane_pathv[j++] = pathv[i];
        }
        else
        {
          if (j > subroot)
          {
            j--;
          }
          else
          {
            subroot++;
            sane_pathv[j++] = pathv[i];
          }
        }
      }
    }
    else
    {
      sane_pathv[j++] = pathv[i];
    }
  }

  sane_pathv[j] = 0;
  gchar* simple_path = g_strjoinv("/", sane_pathv);

  g_strfreev(pathv);
  g_free(sane_pathv);

  return simple_path;
}

gchar* sanitize_root_path(const gchar* root)
{
  if (root == NULL) /* NULL -> "/" (default) */
    return g_strdup("/");
  if (*root == '\0') /* "" -> "" (current dir) */
    return g_strdup("/");
  gchar* simple = path_simplify(root);
  if (*simple == '\0' || !strcmp(simple, "/"))
    return simple;
  gchar* ret = g_strconcat(simple, "/", NULL);
  g_free(simple);
  return ret;
}
