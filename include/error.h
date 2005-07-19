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

#define E_OK      0 /**< all ok */
#define E_ERROR   1 /**< nonfatal error */
#define E_BADARG  2 /**< invalid function arguments */
#define E_FATAL   4 /**< fatal error */
#define E_BREAK   5 /**< terminated by signal */

#define E_PASS    0xffffffff /**< leave previous error code (useful for longjmp error handling) */

#define E(n) (1<<(n+8)) /**< helper macro for error number definitions */

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

/** Print error message.
 * 
 * @param e Error object.
 */
extern void e_print(struct error* e);

/** Returns true if no error occured.
 * 
 * @param e Error object.
 * @return true if ok.
 */
extern gint e_ok(struct error* e);

#endif

/*! @} */

