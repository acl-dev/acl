#ifndef _MBOX_INCLUDE_H
#define _MBOX_INCLUDE_H

typedef struct MBOX MBOX;

#define	MBOX_T_SPSC		0	/* 单生产者单消费者 */
#define	MBOX_T_MPSC		1	/* 多生产者单消费者 */

/**
 * 创建无锁消息队列对象
 * @param type {unsigned}
 * @return {MBOX}
 */
MBOX *mbox_create(unsigned type);

/**
 * 释放无锁消息队列对象
 * @param mbox {MBOX*} 消息队列对象
 * @param free_fn {void (*)(void*)} 非空时用来释放当前存在于消息队列中的对象
 */
void mbox_free(MBOX *mbox, void (*free_fn)(void*));

/**
 * 向消息队列中添加动态消息对象
 * @param mbox {MBOX*} 消息队列对象
 * @param msg {void*}
 * @return {int} 发送成功返回 0，否则返回 -1
 */
int mbox_send(MBOX *mbox, void *msg);

/**
 * 从消息队列中读取消息
 * @param mbox {MBOX*} 消息队列对象
 * @param timeout {int} 等待超时时间(毫秒级别)，如果 < 0 则一直等待直到有数据
 * @param success {int*} 存储操作是否成功的结果， 0 表示出错，非 0 表示成功
 * @return {void*} 返回读到的消息对象，如果返回 NULL 时还需判断 success 的值，
 *  以此来判断读操作是否成功，如果返回非 NULL 表示成功读到一条消息
 */
void *mbox_read(MBOX *mbox, int timeout, int *success);

/**
 * 获得当前消息队列已经成功发送的消息数
 * @param mbox {MBOX*} 消息队列对象
 * @return {size_t}
 */
size_t mbox_nsend(MBOX *mbox);

/**
 * 获得当前消息队列已经成功接收到的消息数
 * @param mbox {MBOX*} 消息队列对象
 * @return {size_t}
 */
size_t mbox_nread(MBOX *mbox);

socket_t mbox_in(MBOX *mbox);
socket_t mbox_out(MBOX *mbox);

#endif
