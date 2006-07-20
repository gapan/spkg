/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ondøej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
/** @defgroup taction_api Filesystem transaction API

This API is used to implement rollback functionality. First you initialize
transaction using ta_initialize(), then you will build transaction using
ta_[on_finalize]_[on_rollback]() functions. And finally you will call
ta_finalize() or ta_rollback() to end transaction.

*/
/** @addtogroup taction_api */
/*! @{ */

#ifndef SPKG__TRANSACTION_H
#define SPKG__TRANSACTION_H

#include <glib.h>
#include "error.h"

#define TA_ACTIVE  E(0) /**< Another transaction is still active. */
#define TA_NACTIVE E(1) /**< Transaction is not active. */

/** Initialize transaction.
 * 
 * @param dryrun Don't touch filesystem on ta_finalize() or ta_rollback().
 * @param e Error object.
 * @return 0 on success, 1 on error.
 */
extern gint ta_initialize(gboolean dryrun, struct error* e);

/** Finalize current transaction. Registered actions will be performed
 * in the registration order. (Except for ta_remove_nothing())
 * 
 * @return 0 on success, 1 on error.
 */
extern gint ta_finalize();

/** Rollback current transaction. Registered actions will be performed
 * in the reverse registration order.
 * 
 * @return 0 on success, 1 on error.
 */
extern gint ta_rollback();

/** Keep file on finalize, remove on rollback.
 * 
 * @param path File/directory path.
 * @param is_dir Path points to a directory.
 */
extern void ta_keep_remove(gchar* path, gboolean is_dir);

/** Rename file on finalize, remove on rollback.
 * 
 * @param path File path.
 * @param dst_path Destination file path.
 */
extern void ta_move_remove(gchar* path, gchar* dst_path);

/** Create hardlink on finalize, do nothing on rollback.
 * 
 * @param path Link path.
 * @param tgt_path Link target path.
 */
extern void ta_link_nothing(gchar* path, gchar* tgt_path);

/** Forcibly create hardlink on finalize, do nothing on rollback.
 * 
 * @param path Link path.
 * @param tgt_path Link target path.
 */
extern void ta_forcelink_nothing(gchar* path, gchar* tgt_path);

/** Create symlink on finalize, do nothing on rollback.
 * 
 * @param path Link path.
 * @param tgt_path Link target path.
 */
extern void ta_symlink_nothing(gchar* path, gchar* tgt_path);

/** Forcibly create symlink on finalize, do nothing on rollback.
 * 
 * @param path Link path.
 * @param tgt_path Link target path.
 */
extern void ta_forcesymlink_nothing(gchar* path, gchar* tgt_path);

/** Change permissions on finalize, do nothing on rollback.
 * 
 * @param path File/directory path.
 * @param mode Mode.
 * @param owner Owner.
 * @param group Group.
 */
extern void ta_chperm_nothing(gchar* path, gint mode, gint owner, gint group);

/** Remove file/directory on finalize, do nothing on rollback.
 * If path is directory, it removal will be done in the reverse order
 * at the end of the transaction. This helps to simplify upgrade command.
 * 
 * @param path File/directory path.
 * @param is_dir Path points to a directory.
 */
extern void ta_remove_nothing(gchar* path, gint is_dir);

#endif

/*! @} */

