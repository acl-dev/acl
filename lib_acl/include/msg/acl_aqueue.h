#ifndef ACL_AQUEUE_INCLUDE_H
#define	ACL_AQUEUE_INCLUDE_H

#include "../stdlib/acl_define.h"
#include "../thread/acl_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	ACL_AQUEUE_ERR_UNKNOWN      -1
#define	ACL_AQUEUE_OK               0
#define	ACL_AQUEUE_ERR_LOCK         1
#define	ACL_AQUEUE_ERR_UNLOCK       2
#define	ACL_AQUEUE_ERR_TIMEOUT      3
#define	ACL_AQUEUE_ERR_COND_WAIT    4
#define	ACL_AQUEUE_ERR_COND_SIGNALE 5

typedef struct ACL_AQUEUE_ITEM ACL_AQUEUE_ITEM;
typedef struct ACL_AQUEUE ACL_AQUEUE;

typedef void (*ACL_AQUEUE_FREE_FN)(void *);
/**
 * 产生一个新队列对象句柄
 * @return ACL_AQUEUE 结构指针
 */
ACL_API ACL_AQUEUE *acl_aqueue_new(void);

/**
 * 设置是否严格检查队列的所有者，默认为否，需要进行该检查的有 acl_aqueue_free
 * @param queue ACL_AQUEUE 结构指针
 * @param flag 是与否
 */
ACL_API void acl_aqueue_check_owner(ACL_AQUEUE *queue, char flag);

/**
 * 设置队列的所有者, 只有所有者才有权释放队列, 即调用 acl_aqueue_free()
 * @param queue ACL_AQUEUE 结构指针
 * @param owner 由线程号标识的所有者的ID号
 */
ACL_API void acl_aqueue_set_owner(ACL_AQUEUE *queue, unsigned int owner);

/**
 * 释放队列对象句柄
 * @param queue ACL_AQUEUE 结构指针
 * @param free_fn 当释放队列时, 如果该函数不为空, 则内部通过此函数将队列中的
 *        用户注册的数据队列进行释放
 */
ACL_API void acl_aqueue_free(ACL_AQUEUE *queue, ACL_AQUEUE_FREE_FN free_fn);

/**
 * 从队列中提取一个元素, 不带超时, 一直等到有元素可用或出错
 * @param queue ACL_AQUEUE 结构指针
 * @return 用户通过 acl_aqueue_push 加入的元素指针
 */
ACL_API void *acl_aqueue_pop(ACL_AQUEUE *queue);

/**
 * 从队列中提取一个元素, 带超时, 一直等到有元素可用或超时或出错
 * @param queue ACL_AQUEUE 结构指针
 * @param tmo_sec 从队列中提取元素的超时时间, 单位为秒
 * @param tmo_usec 从队列中提取元素的超时时间, 单位为微秒
 * @return 用户通过 acl_aqueue_push 加入的元素指针
 */
ACL_API void *acl_aqueue_pop_timedwait(ACL_AQUEUE *queue, int tmo_sec, int tmo_usec);

/**
 * 向队列中添加一个元素
 * @param queue ACL_AQUEUE 结构指针
 * @param data 用户的数据指针
 * @return {int} 添加队列元素是否成功, 0: ok; < 0: error
 */
ACL_API int acl_aqueue_push(ACL_AQUEUE *queue, void *data);

/**
 * 获得上一次队列操作的错误号, define as: ACL_AQUEUE_XXX
 * @param queue ACL_AQUEUE 结构指针
 * @return 错误号
 */
ACL_API int acl_aqueue_last_error(const ACL_AQUEUE *queue);

/**
 * 设置队列为退出状态
 * @param queue ACL_AQUEUE 结构指针
 */
ACL_API void acl_aqueue_set_quit(ACL_AQUEUE *queue);

/**
 * 获得当前队列中队列元素的个数
 * @param queue {ACL_AQUEUE*}
 * @return {int} 队列中元素个数，< 0 表示出错
 */
ACL_API int acl_aqueue_qlen(ACL_AQUEUE* queue);

#ifdef __cplusplus
}
#endif
#endif

