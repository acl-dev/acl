#ifndef __MEMORY_HEAD_H__
#define __MEMORY_HEAD_H__

void *stack_alloc(size_t size);
void *stack_calloc(size_t size);
void stack_free(void *ptr);

#endif
