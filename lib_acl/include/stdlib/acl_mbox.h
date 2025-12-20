#ifndef ACL_MBOX_INCLUDE_H
#define ACL_MBOX_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

typedef struct ACL_MBOX ACL_MBOX;

/**
 * Create a message queue object.
 * @return {ACL_MBOX}
 */
ACL_API ACL_MBOX *acl_mbox_create(void);

#define	ACL_MBOX_T_SPSC		0	/* Single producer single consumer */
#define	ACL_MBOX_T_MPSC		1	/* Multiple producer single consumer */

ACL_API ACL_MBOX *acl_mbox_create2(unsigned type);

/**
 * Free message queue object.
 * @param mbox {ACL_MBOX*} Message queue object
 * @param free_fn {void (*)(void*)} If not NULL, callback function to free
 *  objects in current message queue
 */
ACL_API void acl_mbox_free(ACL_MBOX *mbox, void (*free_fn)(void*));

/**
 * Send dynamic message to message queue.
 * @param mbox {ACL_MBOX*} Message queue object
 * @param msg {void*} Must not pass NULL object
 * @return {int} Returns 0 on success, otherwise returns -1
 */
ACL_API int acl_mbox_send(ACL_MBOX *mbox, void *msg);

/**
 * Read message from message queue.
 * @param mbox {ACL_MBOX*} Message queue object
 * @param timeout {int} Wait timeout time (milliseconds), if < 0, wait
 *  indefinitely until message arrives
 * @param success {int*} Stores whether operation succeeded, 0 indicates timeout,
 *  non-zero indicates success
 * @return {void*} Returns message object, if return value is NULL, check success
 *  value to determine whether operation succeeded. If return is NULL, indicates
 *  successfully received a message. Note: mbox does not support passing NULL objects!
 */
ACL_API void *acl_mbox_read(ACL_MBOX *mbox, int timeout, int *success);

/**
 * Get the number of messages successfully sent in current message queue.
 * @param mbox {ACL_MBOX*} Message queue object
 * @return {size_t}
 */
ACL_API size_t acl_mbox_nsend(ACL_MBOX *mbox);

/**
 * Get the number of messages successfully received in current message queue.
 * @param mbox {ACL_MBOX*} Message queue object
 * @return {size_t}
 */
ACL_API size_t acl_mbox_nread(ACL_MBOX *mbox);

#ifdef __cplusplus
}
#endif

#endif
