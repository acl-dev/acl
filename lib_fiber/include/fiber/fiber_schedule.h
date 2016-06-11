#ifndef FIBER_SCHEDULE_INCLUDE_H
#define FIBER_SCHEDULE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FIBER FIBER;

FIBER *fiber_create(void (*fn)(FIBER *, void *), void *arg, size_t size);
int    fiber_id(const FIBER *fiber);
int    fiber_yield(void);
void   fiber_switch(void);
void   fiber_schedule(void);

#ifdef __cplusplus
}
#endif

#endif
