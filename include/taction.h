/*----------------------------------------------------------------------*\
|* fastpkg                                                              *|
|*----------------------------------------------------------------------*|
|* Slackware Linux Fast Package Management Tools                        *|
|*                               designed by Ondøej (megi) Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*  No copy/usage restrictions are imposed on anybody using this work.  *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef __TRANSACTION_H
#define __TRANSACTION_H

#include <glib.h>

/** */
typedef enum { 
  TA_ADD=1,   /**< add file */
  TA_MOVE,    /**< move file */
} ta_type;

/** Initialize current transaction.
 * 
 * @return 0 on success, 1 on error
 */
extern gint ta_initialize();

/** Finalize current transaction.
 * 
 * @return 0 on success, 1 on error
 */
extern gint ta_finalize();

/** Rollback current transaction.
 * 
 * @return 0 on success, 1 on error
 */
extern gint ta_rollback();

/** Add action to the current transaction.
 * 
 * @return 0 on success, 1 on error
 */
extern gint ta_add_action(ta_type type, gchar* path1, gchar* path2);

#endif

/*! @} */

