#ifndef	__MALLOC_VARS_INCLUDE_H__
#define	__MALLOC_VARS_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

extern void *(*__malloc_fn)(const char*, int, size_t);
extern void *(*__calloc_fn)(const char*, int, size_t, size_t);
extern void *(*__realloc_fn)(const char*, int, void*, size_t);
extern char *(*__strdup_fn)(const char*, int, const char*);
extern char *(*__strndup_fn)(const char*, int, const char*, size_t);
extern void *(*__memdup_fn)(const char*, int, const void*, size_t);
extern void  (*__free_fn)(const char*, int, void*);

#ifdef	__cplusplus
}
#endif

#endif
