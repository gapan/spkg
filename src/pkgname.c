/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <string.h>

#include "pkgname.h"

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
      if (tmp2 == name) /*  */
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
  
  XXX: strip eoln
*/
#define MAXLNLEN 128
gint parse_slackdesc(const gchar* slackdesc, const gchar* sname, gchar* desc[11])
{
  const gchar* i = slackdesc;
  const gchar* j;
  gint sl = strlen(sname);
  gint ln = 0;
  gint l = 0;
  gchar buf[MAXLNLEN];

  /*XXX: some asserts should be here */

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
