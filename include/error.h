/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup err_api Error Handling API

This is common error handling API for all parts of spkg.

*//*--------------------------------------------------------------------*/
/** @addtogroup err_api */
/*! @{ */

#ifndef __ERROR_H
#define __ERROR_H

#include <glib.h>

#define E_OK    0 /**< no error */
#define E_ERROR 1 /**< some error */

struct error;

/** Create new error object.
 *
 * @return Error object.
 */
extern struct error* e_new();

/** Free error object.
 *
 * @param e Error object.
 */
extern void e_free(struct error* e);

/** Get string representation of the error.
 *
 * @return Error string on error, 0 otherwise
 */
extern gchar* e_string(struct error* e);

/** Get numeric representation of the error.
 *
 * @return Numeric representation of the error.
 */
extern gint e_errno(struct error* e);

/** Add error to the error object.
 * 
 * @param e Error object.
 * @param context Error context. (in which library it occured)
 * @param function Error function (in which function it occured)
 * @param errnum Error number. (error type)
 * @param errfmt Just like in printf.
 */
extern void e_add(
  struct error* e,
  const char* context,
  const char* function,
  gint errnum,
  gchar* errfmt,
  ...
);

/** Clean error.
 * 
 * @param e Error object.
 */
extern void e_clean(struct error* e);

#define e_handler(jb) if (setjmp(jb))
#define e_handle(jb) longjmp(jb,1)

#endif

/*! @} */

