/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <string.h>

#include "misc.h"

/* public - parsers
 ************************************************************************/

gchar* parse_pkgname(const gchar* path, guint elem)
{
  gchar *tmp, *tmp2, *name, *retval=0;

  if (path == 0)
    return 0;

  tmp = g_strrstr(path, "/");
  switch (elem)
  {
    case 0:
    {
      if (tmp == 0)
        return g_strdup(".");
      return g_strndup(path, tmp-path);
    }
    case 1: case 2: case 3: case 4: case 5: case 6:
    {
      gint i=0,j=0;
      tmp = tmp==0?(gchar*)path:tmp+1;
      name = g_strndup(tmp, strlen(tmp)-(g_str_has_suffix(path, ".tgz")?4:0));
      /* 3 dashes required */
      for (tmp=name; *tmp!=0; tmp++)
        if (*tmp == '-') i++;
      if (i<3)
        goto ret;
      i=0;j=0;
      /* something between dashes required */
      for (tmp2=tmp-1; tmp2!=name; tmp2--)
      {
        j++; /* count characters from last dash */
        if (*tmp2 == '-')
        {
          if (j == 1) /* if only one char since last dash, err */
            goto ret;
          j=0; /* reset character count */
          i++; /* count dashes */
          if (i == 3) /* if third dash break it */
            break;
        }
      }
      if (tmp2 == name)
        goto ret;
      if (elem == 5)
        return name;
      else if (elem == 6)
      {
        retval = (gchar*)(-1);
        goto ret;
      }
      i = 4;
      tmp2 = tmp-1; /* tmp2 always points to the end of the name segment */
      while (--tmp != name)
      {
        if (*tmp == '-' && i>1)
        {
          if (i == elem)
          {
            retval = g_strndup(tmp+1, tmp2-tmp);
            goto ret;
          }
          tmp2 = tmp-1;
          i--;
        }
      }
      retval = g_strndup(tmp, tmp2-tmp+1);
      break;
    }
    default:
      return 0;
  }
 ret:
  g_free(name);
  return retval;
}

/* slack-desc format is 
  <shortname>: <shortname> (desc) 
  <shortname>: longdesc
  <shortname>: longdesc
  ...
  other lines are discarded
*/
#define MAXLNLEN 128 /* slack-desc file lines length is limited anyway */
gint parse_slackdesc(const gchar* slackdesc, const gchar* sname, gchar* desc[11])
{
  const gchar* i = slackdesc;
  const gchar* j;
  gint sl;
  gint ln = 0;
  gint l = 0;
  gchar buf[MAXLNLEN];

  g_assert(slackdesc != 0);
  g_assert(sname != 0);
  g_assert(desc != 0);

  sl = strlen(sname);

  for (l=0;l<11;l++)
    desc[l] = 0;
  
  while (1)
  {
    if (strncmp(sname, i, sl) != 0 || (*(i+sl) != ':'))
    { /* line don't match sname */
      i = strchr(i,'\n');
      if (i==0)
        return ln?0:1; /* end */
      i++;
    }
    else
    { /* line matches sname: */
      if (ln > 10) /* too much valid lines */
        return 0; /* this is ok */

      i += sl+1;
      j = strchr(i,'\n');
      if (j==0)
        j = i+strlen(i)+1;
      l = j-i;
      if (l > MAXLNLEN-1)
        l = MAXLNLEN-1;
      strncpy(buf, i, l);
      buf[l] = 0;
      gchar* b = buf[0] == ' '?buf+1:buf;
      desc[ln] = g_strndup(b, MAXLNLEN-1);
      ln++;
    }
  }
  return 1;
}

gchar* gen_slackdesc(const gchar* sname, gchar* desc[11])
{
  gchar buf[MAXLNLEN*11];
  buf[0]=0;
  gint i;

  g_assert(sname != 0);
  g_assert(desc != 0);

  for (i=0;i<11;i++)
  {
    if (desc[i] == 0)
      break;
    g_strlcat(buf,sname,MAXLNLEN*11);
    g_strlcat(buf,": ",MAXLNLEN*11);
    g_strlcat(buf,desc[i],MAXLNLEN*11);
    g_strlcat(buf,"\n",MAXLNLEN*11);
  }
  return buf[0]?g_strdup(buf):0;
}

/* this is simple parser that returns 1 if line looks like this:
 * ( cd <dir> ; ln -sf <target> <link> )
 */
gint parse_createlink(gchar* line, gchar** dir, gchar** link, gchar** target)
{
  g_assert(line != 0);
  g_assert(dir != 0);
  g_assert(link != 0);
  g_assert(target != 0);

  gchar* it = line;

  /* check "( cd " part */
  if (strncmp(it, "( cd ", 5) != 0)
    goto fail0;
  it+=5;

  /* get dir */
  gchar* _dir = it;
  while (*it != ' ' && *it != 0) it++;
  if (it == _dir || *it == 0)
    goto fail0;
  _dir = g_strndup(_dir, it-_dir);
  
  /* check " ; ln -sf " part */
  if (strncmp(it, " ; ln -sf ", 10) != 0)
    goto fail1;
  it+=10;

  /* get target */
  gchar* _tgt = it;
  while (*it != ' ' && *it != 0) it++;
  if (it == _tgt || *it == 0)
    goto fail1;        
  _tgt = g_strndup(_tgt, it-_tgt);

  it++; /* skip space after target */
 
  /* get link */
  gchar* _lnk = it;
  while (*it != ' ' && *it != 0) it++;
  if (it == _lnk || *it == 0)
    goto fail2;        
  _lnk = g_strndup(_lnk, it-_lnk);

  /* check " )" part */
  if (strncmp(it, " )", 2) != 0)
    goto fail3;
  
  *dir = _dir;
  *link = _lnk;
  *target = _tgt;
  return 1;

 fail3:
  g_free(_lnk);
 fail2:
  g_free(_tgt);
 fail1:
  g_free(_dir);
 fail0:
  return 0;
}

gint parse_cleanuplink(gchar* line)
{
  g_assert(line != 0);
  gchar* it = line;

  /* check "( cd " part */
  if (strncmp(it, "( cd ", 5) != 0)
    return 0;
  it+=5;

  /* get dir */
  gchar* _dir = it;
  while (*it != ' ' && *it != 0) it++;
  if (it == _dir || *it == 0)
    return 0;
  
  /* check " ; ln -sf " part */
  if (strncmp(it, " ; rm -rf ", 10) != 0)
    return 0;
  it+=10;

  /* get target */
  gchar* _tgt = it;
  while (*it != ' ' && *it != 0) it++;
  if (it == _tgt || *it == 0)
    return 0;

  /* check " )" part */
  if (strncmp(it, " )", 2) != 0)
    return 0;

  return 1;
}

gint iter_lines(gchar** b, gchar** e, gchar** n, gchar** ln)
{
  g_assert(b != 0);
  g_assert(e != 0);
  g_assert(n != 0);
  *b = *n;
  if (*b == 0)
    return 0;
  if (**b == 0) /* last line is empty, discard it */
    return 0;
  *e = strchr(*b, '\n'); /* XXX: here may be CRLF check instead */
  if (*e == 0) /* eof */
    *e = *b+strlen(*b)-1, *n=0;
  else
    *n = *e+1, *e-=1;
  if (ln)
    *ln = g_strndup(*b, *e-*b+1);
  return 1;
}

/* eof points after last character */
gint iter_lines2(gchar** b, gchar** e, gchar** n, gchar* eof, gchar** ln)
{
  g_assert(b != 0);
  g_assert(e != 0);
  g_assert(n != 0);
  *b = *n; /* move to the next line */
  if (*b == 0) /* nothing? */
    return 0;
  if (eof == *b)
    return 0;
  gchar* t = *b; /* temp pointer */
  while (t < eof && *t != '\n') /* go forward until eof or \n */
    t++;
  if (t == eof) /* eof */
    *e = t-1, *n=0;
  else /* must be newline */
    *e = t-1, *n=(t+1==eof)?0:t+1; /* ignore last line if it is empty */
  if (ln)
    *ln = g_strndup(*b, *e-*b+1);
  return 1;
}

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
