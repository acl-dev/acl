#ifndef ACL_MBOX_INCLUDE_H
#define ACL_MBOX_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

typedef struct ACL_MBOX ACL_MBOX;

/**
 * 创建无锁消息队列对象
 * @return {ACL_MBOX}
 */
ACL_API ACL_MBOX *acl_mbox_create(void);

/**
 * 释放无锁消息队列对象
 * @param mbox {ACL_MBOX*} 消息队列对象
 * @param free_fn {void (*)(void*)} 非空时用来释放当前存在于消息队列中的对象
 */
ACL_API void acl_mbox_free(ACL_MBOX *mbox, void (*free_fn)(void*));

/**
 * 向消息队列中添加动态消息对象
 * @param mbox {ACL_MBOX*} 消息队列对象
 * @param msg {void*}
 * @return {int} 发送成功返回 0，否则返回 -1
 */
ACL_API int acl_mbox_send(ACL_MBOX *mbox, void *msg);

/**
 * 从消息队列中读取消息
 * @param mbox {ACL_MBOX*} 消息队列对象
 * @param timeout {int} 等待超时时间(毫秒级别)，如果 < 0 则一直等待直到有数据
 * @param success {int*} 存储操作是否成功的结果， 0 表示出错，非 0 表示成功
 * @return {void*} 返回读到的消息对象，如果返回 NULL 时还需判断 success 的值，以此来
 *  判断读操作是否成功，如果返回非 NULL 表示成功读到一条消息
 */
ACL_API void *acl_mbox_read(ACL_MBOX *mbox, int timeout, int *success);

/**
 * 获得当前消息队列已经成功发送的消息数
 * @param mbox {ACL_MBOX*} 消息队列对象
 * @return {size_t}
 */
ACL_API size_t acl_mbox_nsend(ACL_MBOX *mbox);

/**
 * 获得当前消息队列已经成功接收到的消息数
 * @param mbox {ACL_MBOX*} 消息队列对象
 * @return {size_t}
 */
ACL_API size_t acl_mbox_nread(ACL_MBOX *mbox);

#ifdef __cplusplus
}
#endif

#endif
