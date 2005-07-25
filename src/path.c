/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <string.h>

#include "path.h"

/* public - high quality path manipulation functions
 ************************************************************************/

gchar* path_sanitize_slashes(const gchar* path)
{
  g_assert(path != 0);
  gchar* sanepath = g_malloc(strlen(path)+1);
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
  gchar** pathv = path_get_elements(path);
  guint i, j=0, pathv_len = g_strv_length(pathv);
  gchar** sane_pathv = g_malloc0((pathv_len+1)*sizeof(gchar*));
  gboolean absolute = (pathv_len > 1 && **pathv == '\0');
  guint subroot = 0;
  for (i=0; i<pathv_len; i++)
  {
    if (!strcmp(pathv[i], "."))
      continue; /* ignore curdirs in path */
    else if (!strcmp(pathv[i], ".."))
    {
      if (absolute)
      {
        if (j>1)
        {
          j--;
        }
      }
      else
      {
        if (subroot && !strcmp(sane_pathv[j-1], "..")) /* if we are off base and last item is .. */
        {
          sane_pathv[j++] = pathv[i];
        }
        else
        {
          if (j>subroot)
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
      sane_pathv[j++] = pathv[i];
  }
  sane_pathv[j] = 0;
  gchar* simple_path = g_strjoinv("/", sane_pathv);
  g_strfreev(pathv);
  g_free(sane_pathv);
  return simple_path;
}
