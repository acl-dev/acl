#ifndef	ACL_TIMER_INCLUDE_H
#define	ACL_TIMER_INCLUDE_H

#include "../stdlib/acl_define.h"
#include <time.h>
#include "../stdlib/acl_iterator.h"
#include "../stdlib/acl_ring.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 定时器类型定义
 */
typedef struct ACL_TIMER_INFO {               
	/* public */
	void *obj;              /**< 用户的数据对象指针 */
	acl_int64 when;         /**< 被触发的时间截(微妙级) */

	/* private */
	ACL_RING entry;         /**< 内部用的定时链 */
} ACL_TIMER_INFO;

/* 定时器句柄结构 */
typedef struct ACL_TIMER ACL_TIMER;

struct ACL_TIMER {
        acl_int64 (*request)(ACL_TIMER *timer, void *obj, acl_int64 delay);
        acl_int64 (*cancel)(ACL_TIMER *timer, void *obj);
        void* (*popup)(ACL_TIMER* timer);

        ACL_RING timer_header;
        acl_int64 present;
        acl_int64 time_left;

	/* for acl_iterator */

	/* 取迭代器头函数 */
	const void *(*iter_head)(ACL_ITER*, struct ACL_TIMER*);
	/* 取迭代器下一个函数 */
	const void *(*iter_next)(ACL_ITER*, struct ACL_TIMER*);
	/* 取迭代器尾函数 */
	const void *(*iter_tail)(ACL_ITER*, struct ACL_TIMER*);
	/* 取迭代器上一个函数 */
	const void *(*iter_prev)(ACL_ITER*, struct ACL_TIMER*);

	/* 获得与当前迭代指针相关联的 ACL_TIMER_INFO 对象 */
	const ACL_TIMER_INFO *(*iter_info)(ACL_ITER*, struct ACL_TIMER*);
};

/**
 * 添加定时任务
 * @param timer {ACL_TIMER*}，定时器句柄
 * @param obj {void*}，用户级动态变量
 * @param delay {acl_int64}，被触发的时间间隔(微秒级)
 * @return {acl_int64} 新的定时任务的解决时间截(微秒级)
 */
ACL_API acl_int64 acl_timer_request(ACL_TIMER* timer, void *obj, acl_int64 delay);

/**
 * 取消定时任务
 * @param timer {ACL_TIMER*}，定时器句柄
 * @param obj {void*}，用户级动态变量
 * @return {acl_int64}，距离下一个定时任务被触发的时间间隔(微秒级)
 */
ACL_API acl_int64 acl_timer_cancel(ACL_TIMER* timer, void *obj);

/**
 * 从定时器中获取到时的定时任务
 * @param timer {ACL_TIMER*}，定时器句柄
 * @return {void*}，用户级动态变量
 */
ACL_API void *acl_timer_popup(ACL_TIMER* timer);

/**
 * 距离下一个定时任务被触发的时间间隔
 * @param timer {ACL_TIMER*}，定时器句柄
 * @return {acl_int64} 返回值单位为微秒
 */
ACL_API acl_int64 acl_timer_left(ACL_TIMER* timer);

/**
 * 遍历定时器里的所有定时任务项
 * @param timer {ACL_TIMER*}，定时器句柄
 * @param action {void (*)(ACL_TIMER_INFO*, void*)} 用户的遍历回调函数
 * @param arg {void*} action 中的第二个参数
 */
ACL_API void acl_timer_walk(ACL_TIMER *timer, void (*action)(ACL_TIMER_INFO *, void *), void *arg);

/**
 * 创建定时器句柄
 * @return {ACL_TIMER*}
 */
ACL_API ACL_TIMER *acl_timer_new(void);

/**
 * 释放定时器句柄
 * @param timer {ACL_TIMER*}
 * @param free_fn {void (*)(void*)} 释放定时器里的用户对象的回调释放函数
 */
ACL_API void acl_timer_free(ACL_TIMER* timer, void (*free_fn)(void*));

/**
 * 获得定时器里定时任务的数量
 * @param timer {ACL_TIMER*}
 * @return {int} >= 0
 */
ACL_API int acl_timer_size(ACL_TIMER *timer);

#ifdef	__cplusplus
}
#endif

#endif
