/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @addtogroup other_api */
/*! @{ */

#ifndef SPKG__TRANSACTION_H
#define SPKG__TRANSACTION_H

#include <glib.h>
#include "error.h"

#define TA_ACTIVE  E(0) /**< another transaction is still active */
#define TA_NACTIVE E(1) /**< transaction is not active */

/** Initialize current transaction.
 * 
 * @param dryrun don't touch filesystem
 * @param e error object
 * @return 0 on success, 1 on error
 */
extern gint ta_initialize(gboolean dryrun, struct error* e);

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
 * @param path path to the transactioned object
 * @param is_dir if path points to a directory
 */
extern void ta_keep_remove(gchar* path, gboolean is_dir);

/** Add action to the current transaction.
 * 
 * @param path path to the transactioned object
 * @param fin_path destination path
 */
extern void ta_move_remove(gchar* path, gchar* fin_path);

/** Add action to the current transaction.
 * 
 * @param path path to the transactioned object
 * @param src_path link source path
 */
extern void ta_link_nothing(gchar* path, gchar* src_path);

/** Add action to the current transaction.
 * 
 * @param path path to the transactioned object
 * @param src_path link source path
 */
extern void ta_symlink_nothing(gchar* path, gchar* src_path);

/** Add action to the current transaction.
 * 
 * @param path path to the transactioned object
 * @param mode new mode of the object
 * @param owner new owner of the object
 * @param group new group of the object
 */
extern void ta_chperm_nothing(gchar* path, gint mode, gint owner, gint group);

#endif

/*! @} */

