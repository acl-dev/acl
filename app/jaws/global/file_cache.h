#ifndef	__FILE_CACHE_INCLUDE_H__
#define	__FILE_CACHE_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct BUFFER {
	void *buf;
	void *ptr;
	size_t size;
} BUFFER;

typedef struct FILE_CACHE {
	char      path[1024];
	size_t    size;
	char      tm_mtime[64];
	ACL_FIFO *fifo;
} FILE_CACHE;

typedef struct CACHE_ITER {
	FILE_CACHE *cache;
	ACL_ITER iter;
} CACHE_ITER;

/* in file_cache.c */
void file_cache_init(void);
FILE_CACHE *file_cache_new(const char *file_path, time_t last_modified);
void file_cache_free(FILE_CACHE *cache);
FILE_CACHE *file_cache_find(const char *file_path);
void file_cache_push(FILE_CACHE *cache, const void *data, size_t size);
void file_cache_iter(FILE_CACHE *cache, CACHE_ITER *iter);
BUFFER *file_cache_next_buffer(CACHE_ITER *iter);

#ifdef	__cplusplus
}
#endif

#endif
