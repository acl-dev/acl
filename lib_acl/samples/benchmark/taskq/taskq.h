#ifndef __TASKQ_INCLUDE_H__
#define __TASKQ_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TASKQ TASKQ;

TASKQ *taskq_create(unsigned qsize, unsigned nthreads);
void taskq_destroy(TASKQ *taskq);
void taskq_push(TASKQ *taskq, void (*callback)(void*), void *ctx);

#ifdef __cplusplus
}
#endif

#endif
