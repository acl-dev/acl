#ifndef __MEMORY_HEAD_H__
#define __MEMORY_HEAD_H__

void *stack_alloc(size_t size);
void *stack_calloc(size_t size);
void stack_free(void *ptr);

void *mem_malloc(size_t size);
void mem_free(void *ptr);
void *mem_calloc(size_t nmemb, size_t size);
void *mem_realloc(void *ptr, size_t size);
void mem_stat(void);
char *mem_strdup(const char *s);

#endif
