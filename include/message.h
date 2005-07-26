/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <glib.h>
#include "error.h"

/** Setup messages.
 *
 * @param prefix textual prefix that will be printed before each message
 * @param verbosity verbosity level
 */
extern void msg_setup(const gchar* prefix, gint verbosity);

/** Print a message.
 *
 * @param type verbosity level of message
 * @param fmt just like printf
 */
extern void msg(const gint type, const gchar* fmt, ...);

/** Print a notice. */
#define _notice(fmt, args...) msg(3, fmt, ##args)

/** Print an information. */
#define _inform(fmt, args...) msg(2, fmt, ##args)

/** Print a warning. */
#define _warning(fmt, args...) msg(1, fmt, ##args)

#endif

/*! @} */

