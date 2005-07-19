/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __SIGTRAP_H
#define __SIGTRAP_H

#include <glib.h>
#include "error.h"

/** Will be true if process received breaking signal. */
extern gboolean sig_break;

/** Trap signals.
 *
 * @param e Error object.
 * @return 0 on success, 1 on error
 */
extern gint sig_trap(struct error* e);

#endif

/*! @} */

