/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef SPKG__MISC_H
#define SPKG__MISC_H

#include <glib.h>

G_BEGIN_DECLS

/** Parse package name into parts.
 *
 * @param path Package path.
 * @param elem Package name element.
 *  @li 0 path
 *  @li 1 shortname
 *  @li 2 version
 *  @li 3 arch
 *  @li 4 build
 *  @li 5 fullname
 *  @li 6 check (returns -1 if ok)
 * @return Requested package name element (g_malloced), 0 on error.
 */
extern gchar* parse_pkgname(const gchar* path, guint elem);

/** Parse slack-desc file from buffer into parts.
 *
 * @param slackdesc Buffer filled with contents of the raw slack-desc file.
 * @param sname Short name of the package.
 * @param desc Array of prased slack-desc lines.
 * @return 0 on success, 1 on error (i.e. not a single slack-desc line)
 */
extern gint parse_slackdesc(const gchar* slackdesc, const gchar* sname, gchar* desc[11]);

/** Generate slack-desc file from parsed buffer.
 *
 * @param sname Short name of the package.
 * @param desc Array of prased slack-desc lines.
 * @return 0 on error, slack-desc string
 */
extern gchar* gen_slackdesc(const gchar* sname, gchar* desc[11]);

/** Parse link line in the doinst_sh file.
 *
 * @param line Link line.
 * @param dir Directory part.
 * @param link Link part.
 * @param target Target part.
 * @return 0 if not a valid link line, 1 if valid
 */
extern gint parse_createlink(gchar* line, gchar** dir, gchar** link, gchar** target);

/** Parse cleanup link line in the doinst_sh file.
 *
 * @param line Link line.
 * @return 0 if not a valid cleanup link line, 1 if valid
 */
extern gint parse_cleanuplink(gchar* line);

/** Iterate through null-termianted buffer line by line.
 *
 * @param b  begining of the line
 * @param e  one character after the end of the line (excluding \n)
 * @param n  next line start (set this to the buffer begining at start)
 * @param ln line (zero terminated g_strduped string, freed by user) 
 *           (could be zero if you don't want to us it)
 * @return 0 if not a valid cleanup link line, 1 if valid
 * @code
 *  gchar *b, *e, *ln, *n=buf;
 *  while(iter_lines(&b, &e, &n, &ln))
 *  {
 *    g_free(ln);
 *  }
 * @endcode
 */
extern gint iter_str_lines(gchar** b, gchar** e, gchar** n, gchar** ln);

/** Iterate through buffer line by line and terminate just before eof.
 *
 * @param b  begining of the line
 * @param e  one character after the end of the line (excluding \n)
 * @param eof end of the buffer (after the last valid character eof = b + length)
 * @param n  next line start (set this to the buffer begining at start)
 * @param ln line (zero terminated g_strduped string, freed by user) 
 *           (could be zero if you don't want to us it)
 * @return 0 if not a valid cleanup link line, 1 if valid
 * @code
 *  gchar *b, *e, *ln, *n=buf, eof=buf+strlen(buf);
 *  while(iter_lines(&b, &e, &n, eof, &ln))
 *  {
 *    g_free(ln);
 *  }
 * @endcode
 */
extern gint iter_buf_lines(gchar** b, gchar** e, gchar** n, gchar* eof, gchar** ln);

/** g_strv_length() is not in glib <= 2.6.0, so emulate it here.
 */
extern guint g_strv_length_compat(gchar **str_array);

/** Load blacklist from file. Ignore empty and commented lines.
 *
 * @param path File path.
 * @return String vector of blacklist file lines.
 */
extern gchar** load_blacklist(const gchar* path);

G_END_DECLS

#endif

/*! @} */
