/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef SPKG__MESSAGE_H
#define SPKG__MESSAGE_H

#include <glib.h>
#include "error.h"

/** Setup messages.
 *
 * @param verbosity verbosity level
 */
extern void msg_setup(gint verbosity);

/** Print a message.
 *
 * @param type verbosity level of message
 * @param fmt just like printf
 */
extern void msg(const gint type, const gchar* fmt, ...);

/** Print a debug message. */
#define _debug(fmt, args...) msg(4, fmt, ##args)

/** Print a notice. */
#define _notice(fmt, args...) msg(3, fmt, ##args)

/** Print an information. */
#define _inform(fmt, args...) msg(2, fmt, ##args)

/** Print a warning. */
#define _warning(fmt, args...) msg(1, fmt, ##args)

#endif

/*! @} */

