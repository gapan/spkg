/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
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
extern gint ta_initialize(const gchar* root, struct error* e);

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

