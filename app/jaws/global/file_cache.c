#include "lib_acl.h"
#include "lib_protocol.h"
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef ACL_UNIX
#include <unistd.h>
#endif
#include "service.h"
#include "file_cache.h"
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

static ACL_HTABLE *__cache_table = NULL;

void file_cache_init(void)
{
	__cache_table = acl_htable_create(100, 0);
}

FILE_CACHE *file_cache_new(const char *file_path, time_t last_modified)
{
	FILE_CACHE *cache;

	cache = (FILE_CACHE*) acl_mymalloc(sizeof(FILE_CACHE));
	cache->size = 0;
	http_mkrfc1123(cache->tm_mtime, sizeof(cache->tm_mtime), last_modified);
	ACL_SAFE_STRNCPY(cache->path, file_path, sizeof(cache->path));
	cache->fifo = acl_fifo_new();
	(void) acl_htable_enter(__cache_table, file_path, (char*) cache);
	return (cache);
}

void file_cache_free(FILE_CACHE *cache)
{
	BUFFER *buffer;

	acl_htable_delete(__cache_table, cache->path, NULL);

	while (1) {
		buffer = (BUFFER*) acl_fifo_pop(cache->fifo);
		if (buffer == NULL)
			break;
		acl_myfree(buffer->buf);
		acl_myfree(buffer);
	}

	acl_fifo_free(cache->fifo, NULL);
	acl_myfree(cache);
}

FILE_CACHE *file_cache_find(const char *file_path)
{
	FILE_CACHE *cache = (FILE_CACHE*)
		acl_htable_find(__cache_table, file_path);
	return (cache);
}

void file_cache_push(FILE_CACHE *cache, const void *data, size_t size)
{
	BUFFER *buffer = acl_mymalloc(sizeof(BUFFER));

	buffer->buf = acl_mymalloc(size);
	buffer->size = size;
	memcpy(buffer->buf, data, size);
	buffer->ptr = buffer->buf;
	cache->size += size;
	acl_fifo_push(cache->fifo, buffer);
}

void file_cache_iter(FILE_CACHE *cache, CACHE_ITER *iter)
{
	cache->fifo->iter_head(&iter->iter, cache->fifo);
	iter->cache = cache;
}

BUFFER *file_cache_next_buffer(CACHE_ITER *iter)
{
	BUFFER *buffer;

	if (iter->iter.ptr == NULL)
		return (NULL);
	buffer = (BUFFER*) iter->iter.data;
	iter->cache->fifo->iter_next(&iter->iter, iter->cache->fifo);
	return (buffer);
}

