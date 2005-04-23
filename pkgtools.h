#ifndef __PKGTOOLS_H
#define __PKGTOOLS_H

#include <glib.h>

typedef struct {
  /* command line opts */
  gboolean install;
  gboolean upgrade;
  gboolean remove;
  gboolean verbose;
  gboolean check;
  gboolean force;
  gchar*   root;
  gchar**  files;

} opts_t;
extern opts_t opts;

extern gchar* parse_pkgname(gchar* path, guint elem);

extern gint installpkg(gchar* pkgfile);

#endif
