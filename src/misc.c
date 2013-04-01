/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"

/* public - parsers
 ************************************************************************/

gchar* parse_pkgname(const gchar* path, guint elem)
{
  gchar *tmp, *tmp2, *name = NULL, *retval = NULL;

  if (path == 0)
    return 0;

  tmp = g_strrstr(path, "/");
  switch (elem)
  {
    case 0:
    {
      if (tmp == NULL)
        return g_strdup(".");
      return g_strndup(path, tmp-path);
    }
    case 1: case 2: case 3: case 4: case 5: case 6:
    {
      guint i = 0, j = 0;
      tmp = tmp == NULL ? (gchar*)path : tmp+1;
      guint suffix_len =
        g_str_has_suffix(path, ".tgz") ||
        g_str_has_suffix(path, ".tlz") ||
        g_str_has_suffix(path, ".txz") ||
        g_str_has_suffix(path, ".tar") ? 4 : 0;
      name = g_strndup(tmp, strlen(tmp) - suffix_len);
      /* 3 dashes required */
      for (tmp = name; *tmp != '\0'; tmp++)
        if (*tmp == '-') i++;
      if (i < 3)
        goto ret;
      i = j = 0;
      /* something between dashes required */
      for (tmp2 = tmp-1; tmp2 != name; tmp2--)
      {
        j++; /* count characters from last dash */
        if (*tmp2 == '-')
        {
          if (j == 1) /* if only one char since last dash, err */
            goto ret;
          j = 0; /* reset character count */
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
      return NULL;
  }
 ret:
  g_free(name);
  return retval;
}

#define MAXLNLEN 128 /* slack-desc file lines length is limited anyway */

/* slack-desc format is 
  <shortname>: <shortname> (desc) 
  <shortname>: longdesc
  <shortname>: longdesc
  ...
  other lines are discarded
*/
gint parse_slackdesc(const gchar* slackdesc, const gchar* sname, gchar* desc[MAX_SLACKDESC_LINES])
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

  for (l = 0; l < MAX_SLACKDESC_LINES; l++)
    desc[l] = 0;
  
  while (1)
  {
    if (strncmp(sname, i, sl) != 0 || (*(i+sl) != ':'))
    { /* line don't match sname */
      i = strchr(i,'\n');
      if (i == 0)
        return ln ? 0 : 1; /* end */
      i++;
    }
    else
    { /* line matches sname: */
      if (ln > (MAX_SLACKDESC_LINES-1)) /* too many valid lines */
        return 0; /* this is ok */

      i += sl+1;
      j = strchr(i,'\n');
      if (j==0)
        j = i + strlen(i) + 1;
      l = j-i;
      if (l > MAXLNLEN-1)
        l = MAXLNLEN-1;
      strncpy(buf, i, l);
      buf[l] = 0;
      gchar* b = buf[0] == ' ' ? buf + 1 : buf;
      desc[ln] = g_strndup(b, MAXLNLEN-1);
      ln++;
    }
  }
  return 1;
}

gchar* gen_slackdesc(const gchar* sname, gchar* desc[MAX_SLACKDESC_LINES])
{
  gchar buf[MAXLNLEN*MAX_SLACKDESC_LINES];
  buf[0]=0;
  gint i;

  g_assert(sname != 0);
  g_assert(desc != 0);

  for (i=0;i<MAX_SLACKDESC_LINES;i++)
  {
    if (desc[i] == 0)
      break;
    g_strlcat(buf, sname, MAXLNLEN * MAX_SLACKDESC_LINES);
    g_strlcat(buf, ": ", MAXLNLEN * MAX_SLACKDESC_LINES);
    g_strlcat(buf, desc[i], MAXLNLEN * MAX_SLACKDESC_LINES);
    g_strlcat(buf, "\n", MAXLNLEN * MAX_SLACKDESC_LINES);
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

gint iter_str_lines(gchar** b, gchar** e, gchar** n, gchar** ln)
{
  g_assert(b != 0);
  g_assert(e != 0);
  g_assert(n != 0);
  *b = *n;
  if (*b == 0) /* next pointer was NULL */
    return 0;
  if (**b == 0) /* next pointer points to the end of string */
    return 0;
  *e = strchr(*b, '\n'); /* XXX: here may be CRLF check instead */
  /* *e should point after the end of line */
  if (*e == 0) /* eof */
    *e = *b + strlen(*b), *n = 0; 
  else
    *n = *e + 1;
  if (ln)
    *ln = g_strndup(*b, *e - *b);
  return 1;
}

/* eof points after last character */
gint iter_buf_lines(gchar** b, gchar** e, gchar** n, gchar* eof, gchar** ln)
{
  g_assert(b != 0);
  g_assert(e != 0);
  g_assert(n != 0);
  *b = *n; /* move to the next line */
  if (*b == 0) /* next pointer was NULL */
    return 0;
  if (*b == eof) /* after the end of the buffer */
    return 0;
  gchar* t = *b; /* temp pointer (for searching for end of line) */
  while (t < eof && *t != '\n') /* go forward until eof or \n */
    t++;
  *e = t;
  if (t == eof) /* eof */
    *n = 0;
  else /* must be newline */
    *n = (t + 1 == eof) ? 0 : t + 1; /* ignore last line if it is empty */
  if (ln)
    *ln = g_strndup(*b, *e - *b);
  return 1;
}

guint g_strv_length_compat(gchar **str_array)
{
  guint i = 0;
  g_return_val_if_fail (str_array != NULL, 0);
  while (str_array[i])
    ++i;
  return i;
}

gchar** load_blacklist(const gchar* path)
{
  gchar* line = NULL;
  size_t size = 0;
  FILE* f;
  GSList* l = NULL, *i;
  gchar **bl, **tmp;
  
  f = fopen(path, "r");
  if (f == NULL)
    return NULL;

  while (getline(&line, &size, f) >= 0)
  {
    g_strstrip(line);
    if (strlen(line) > 0 && line[0] != '#')
      l = g_slist_prepend(l, g_strdup(line));
  }
  free(line);

  bl = tmp = g_new0(gchar*, g_slist_length(l)+1);
  for (i=l; i!=NULL; i=i->next)
    *(tmp++) = i->data;

  g_slist_free(l);
  fclose(f);
  return bl;
}

gboolean is_blacklisted(const gchar* str, gchar** blacklist)
{
  gchar** i = blacklist;
  if (i == NULL || str == NULL)
    return FALSE;
  while (*i)
  {
    if (!strcmp(*i, str))
      return TRUE;
    i++;
  }
  return FALSE;
}

