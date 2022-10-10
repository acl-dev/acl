#ifndef	QUEUE_INCLUDE_H
#define	QUEUE_INCLUDE_H

#ifdef __linux

#ifdef __cplusplus
extern "C" {
#endif

#define	QUEUE_ERR_UNKNOWN      -1
#define	QUEUE_OK               0
#define	QUEUE_ERR_LOCK         1
#define	QUEUE_ERR_UNLOCK       2
#define	QUEUE_ERR_TIMEOUT      3
#define	QUEUE_ERR_COND_WAIT    4
#define	QUEUE_ERR_COND_SIGNALE 5

typedef struct QUEUE_ITEM QUEUE_ITEM;
typedef struct QUEUE QUEUE;

typedef void (*QUEUE_FREE_FN)(void *);

/**
 * 产生一个新队列对象句柄
 * @return QUEUE 结构指针
 */
QUEUE *queue_new(void);

/**
 * 设置是否严格检查队列的所有者，默认为否，需要进行该检查的有 queue_free
 * @param que QUEUE 结构指针
 * @param flag 是与否
 */
void queue_check_owner(QUEUE *que, char flag);

/**
 * 设置队列的所有者, 只有所有者才有权释放队列, 即调用 queue_free()
 * @param que QUEUE 结构指针
 * @param owner 由线程号标识的所有者的ID号
 */
void queue_set_owner(QUEUE *que, unsigned int owner);

/**
 * 释放队列对象句柄
 * @param que QUEUE 结构指针
 * @param free_fn 当释放队列时, 如果该函数不为空, 则内部通过此函数将队列中的
 *        用户注册的数据队列进行释放
 */
void queue_free(QUEUE *que, QUEUE_FREE_FN free_fn);

/**
 * 从队列中提取一个元素, 不带超时, 一直等到有元素可用或出错
 * @param que QUEUE 结构指针
 * @return 用户通过 queue_push 加入的元素指针
 */
void *queue_pop(QUEUE *que);

/**
 * 从队列中提取一个元素, 带超时, 一直等到有元素可用或超时或出错
 * @param que QUEUE 结构指针
 * @param tmo_sec 从队列中提取元素的超时时间, 单位为秒
 * @param tmo_usec 从队列中提取元素的超时时间, 单位为微秒
 * @return 用户通过 queue_push 加入的元素指针
 */
 void *queue_pop_timedwait(QUEUE *que, int tmo_sec, int tmo_usec);

/**
 * 向队列中添加一个元素
 * @param que QUEUE 结构指针
 * @param data 用户的数据指针
 * @return {int} 添加队列元素是否成功, 0: ok; < 0: error
 */
 int queue_push(QUEUE *que, void *data);

/**
 * 获得上一次队列操作的错误号, define as: QUEUE_XXX
 * @param que QUEUE 结构指针
 * @return 错误号
 */
 int queue_last_error(const QUEUE *que);

/**
 * 设置队列为退出状态
 * @param que QUEUE 结构指针
 */
 void queue_set_quit(QUEUE *que);

/**
 * 获得当前队列中队列元素的个数
 * @param que {QUEUE*}
 * @return {int} 队列中元素个数，< 0 表示出错
 */
 int queue_qlen(QUEUE* que);

#ifdef __cplusplus
}
#endif

#endif // __linux__

#endif

