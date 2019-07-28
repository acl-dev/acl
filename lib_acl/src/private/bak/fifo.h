#ifndef __FIFO_INCLUDE_H__
#define __FIFO_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FIFO_ITER FIFO_ITER;
typedef struct FIFO FIFO;

extern FIFO *fifo_new(void);
extern void fifo_free(FIFO *fifo, void (*free_fn)(void *));
extern void fifo_push(FIFO *fifo, void *data);
extern void *fifo_pop(FIFO *fifo);
extern void *fifo_head(FIFO *fifo);
extern void *fifo_tail(FIFO *fifo);
extern int fifo_size(FIFO *fifo);
extern FIFO_ITER *fifo_iterator_head(FIFO *fifo);
extern FIFO_ITER *fifo_iterator_next(FIFO_ITER *iter);
extern FIFO_ITER *fifo_iterator_tail(FIFO *fifo);
extern FIFO_ITER *fifo_iterator_prev(FIFO_ITER *iter);
extern void *fifo_iterator_data(FIFO_ITER *iter);
extern FIFO_ITER *fifo_iterator_delete(FIFO_ITER *iter, void (*free_fn)(void *));

#ifdef __cplusplus
}
#endif

#endif

