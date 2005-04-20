#ifndef __PKGTOOLS_H
#define __PKGTOOLS_H

#include <glib.h>

typedef struct state state_t;
struct state {

};

extern gchar* parse_pkgname(gchar* path, guint elem);

extern gint installpkg(state_t* s, gchar* pkgfile);

#endif
