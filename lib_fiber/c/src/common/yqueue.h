#ifndef	_YQUEUE_INCLUDE_H
#define	_YQUEUE_INCLUDE_H

typedef struct YQUEUE YQUEUE;

YQUEUE* yqueue_new(void);
void yqueue_free(YQUEUE *yqueue, void(*free_fn)(void*));
void **yqueue_front(YQUEUE *yqueue);
void **yqueue_back(YQUEUE *yqueue);
void yqueue_push(YQUEUE *yqueue);
void yqueue_pop(YQUEUE *yqueue);

#endif
